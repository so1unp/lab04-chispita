#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <mqueue.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "compartido.h"

#define QUEUE_PERMISSIONS 0666

void *hilo_consumo(void *arg) {
    EstadoYPF *estado = (EstadoYPF *) arg;

    while (estado->corriendo) {
        sleep(5);

        pthread_mutex_lock(&estado->mutex_nafta);

        if (estado->nafta > 0) {
            estado->nafta--;
        }

        if (estado->nafta <= 0) {
            estado->corriendo = 0;
        }

        pthread_mutex_unlock(&estado->mutex_nafta);
    }

    return NULL;
}

void *hilo_acciones(void *arg) {
    EstadoYPF *estado = (EstadoYPF *) arg;
    MensajeNave msj;
    unsigned int prio;

    mqd_t cola = mq_open(COLA_ESTACION, O_RDONLY);

    if (cola == (mqd_t)-1) {
        perror("Error al abrir la cola");
        pthread_exit(NULL);
    }

    while (estado->corriendo) {

        if (mq_receive(cola,
                       (char *)&msj,
                       sizeof(MensajeNave),
                       &prio) != -1) {

            switch (msj.tipo_operacion) {

                case 1:

                    pthread_mutex_lock(&estado->mutex_nafta);

                    if (estado->deuterio >= 5 &&
                        estado->nafta >= 10) {

                        estado->deuterio -= 5;
                        estado->nafta -= 10;

                        estado->recolector1++;
                        estado->nafta_jugador += 10;
                    }

                    pthread_mutex_unlock(&estado->mutex_nafta);

                    break;

                case 2:

                    pthread_mutex_lock(&estado->mutex_oxigeno);

                    if (estado->mutexio >= 1 &&
                        estado->semaforita >= 1 &&
                        estado->kernelio >= 1 &&
                        estado->oxigeno >= 10) {

                        estado->mutexio--;
                        estado->semaforita--;
                        estado->kernelio--;

                        estado->recolector0 += 3;
                        estado->oxigeno -= 10;
                        estado->oxigeno_jugador += 10;
                    }

                    pthread_mutex_unlock(&estado->mutex_oxigeno);

                    break;
            }
        }
    }

    mq_close(cola);
    return NULL;
}

int main() {

    EstadoYPF estado = {
        .MAXnaves = 3,
        .navesEnEstacion = 0,
        .corriendo = 1,
        .oxigeno = 20,
        .nafta = 30,
        .deuterio = 20,
        .mutexio = 5,
        .semaforita = 5,
        .kernelio = 5,
        .recolector0 = 0,
        .recolector1 = 0,
        .oxigeno_jugador = 0,
        .nafta_jugador = 0
    };

    struct mq_attr attr;

    attr.mq_flags = 0;
    attr.mq_maxmsg = 10;
    attr.mq_msgsize = sizeof(MensajeNave);
    attr.mq_curmsgs = 0;

    mq_unlink(COLA_ESTACION);

    mqd_t cola_principal = mq_open(
        COLA_ESTACION,
        O_CREAT | O_RDWR,
        QUEUE_PERMISSIONS,
        &attr
    );

    if (cola_principal == (mqd_t)-1) {
        perror("Fallo al crear la cola");
        exit(1);
    }

    pthread_mutex_init(&estado.mutex_nafta, NULL);
    pthread_mutex_init(&estado.mutex_oxigeno, NULL);

    pthread_t hilo_atencion;
    pthread_t hilo_desgaste;

    pthread_create(
        &hilo_atencion,
        NULL,
        hilo_acciones,
        &estado
    );

    pthread_create(
        &hilo_desgaste,
        NULL,
        hilo_consumo,
        &estado
    );

    pthread_join(hilo_desgaste, NULL);

    estado.corriendo = 0;

    pthread_join(hilo_atencion, NULL);

    pthread_mutex_destroy(&estado.mutex_nafta);
    pthread_mutex_destroy(&estado.mutex_oxigeno);

    mq_close(cola_principal);
    mq_unlink(COLA_ESTACION);

    printf("Estacion cerrada correctamente.\n");

    return 0;
}