//librerias que use yo 
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <mqueue.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <string.h>
#include <stdbool.h>
//heard que creamos para las variables compartidas
#include "compartido.h"

#define QUEUE_PERMISSIONS 0666

// Memoria compartida global
MapaEspacial *mapa_compartido = NULL;

// funciones de los hilos 
void *hiloAcciones(void *arg);
void *hiloConsumo(void *arg);

int main() {
    // Enlazar Memoria Compartida
    int fd = shm_open("/mapa_espacial", O_RDWR, 0666);
    
    if (fd == -1) {
        perror("Ejecutar ./servidor primero");
        exit(1); 
    }
    
    mapa_compartido = (MapaEspacial *)mmap(NULL, sizeof(MapaEspacial), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (mapa_compartido == MAP_FAILED) { 
        perror("Error mmap"); 
        exit(1); 
    }

   //inicio de estacion como arranca la estacion 
    EstadoYPF estado = {
        .MAXnaves = 3, 
        .corriendo = 1, 
        .oxigeno = 9999, 
        .nafta = 9999,      
        .deuterio = 0,     
        .recolector0 = 0, 
        .recolector1 = 0
    };

    struct mq_attr cola_attr = { 
        .mq_flags = 0, 
        .mq_maxmsg = 10, 
        .mq_msgsize = sizeof(MensajeNave), 
        .mq_curmsgs = 0 
    };

    mq_unlink(COLA_ESTACION);

    // crea una cola de mensaje 
    mqd_t cola_principal = mq_open(COLA_ESTACION, O_CREAT | O_RDWR, QUEUE_PERMISSIONS, &cola_attr);

    if (cola_principal == (mqd_t)-1) { 
        perror("Error crear cola"); 
        exit(1); 
    }

    // inicio lo mutex 
    pthread_mutex_init(&estado.mutex_nafta, NULL);
    pthread_mutex_init(&estado.mutex_oxigeno, NULL);

    pthread_t hilo_atencion, hilo_desgaste;
    pthread_create(&hilo_atencion, NULL, hiloAcciones, &estado);
    pthread_create(&hilo_desgaste, NULL, hiloConsumo, &estado);

    system("clear");
    printf("Estacion YPF ONLINE.\n");
    fflush(stdout);

    // 1. se espera a que el hilo de consumo termine cuando las naves terminen de consumir nafta
    pthread_join(hilo_desgaste, NULL);

    // 2. Mandamos la Poison Pill (mensaje -1) para destrabar y cerrar el hilo de acciones
    MensajeNave cierre = { 
        .tipo_operacion = -1, 
        .id_nave = 0 
    };
    mq_send(cola_principal, (char *)&cierre, sizeof(MensajeNave), 0);

    // 3. Esperamos al hilo de atención ahora que ya salió del bucle
    pthread_join(hilo_atencion, NULL);

    // Limpieza de recursos
    pthread_mutex_destroy(&estado.mutex_nafta);
    pthread_mutex_destroy(&estado.mutex_oxigeno);
    mq_close(cola_principal);
    mq_unlink(COLA_ESTACION);

    printf("Estacion cerrada correctamente.\n");
    return 0;
}

void *hiloAcciones(void *arg) {
    EstadoYPF *estado = (EstadoYPF *) arg;
    MensajeNave msj;
    
    mqd_t cola = mq_open(COLA_ESTACION, O_RDONLY);
    if (cola == (mqd_t)-1) {
        perror("Error cola"); 
        pthread_exit(NULL); 
    }

while (estado->corriendo) {

        // espero a que llegue un mensaje de alguna nave
        if (mq_receive(cola_ypf, (char *)&msj, sizeof(MensajeNave), NULL) != -1) {

            int id = msj.id_nave;
            resp.tipo = msj.tipo_operacion;
            resp.cantidad = 0; // si no hay stock o algo falla, mando 0

            // si llega -1 es la señal de cierre, salgo del bucle
            if (msj.tipo_operacion == -1) {
                break;
            }

            // valido que el id de la nave tenga sentido
            if (id < 0 || id >= MAX_NAVES) {
                printf("[ESTACION] Nave %d invalida, ignoro el mensaje.\n", id);
                continue;
            }

            switch (msj.tipo_operacion) {

                case 1: // nafta: la nave paga 5 deuterio y recibe 10 de nafta
                    pthread_mutex_lock(&estado->mutex_nafta);
                    if (estado->deuterio >= 5 && estado->nafta >= 10) {
                        estado->deuterio -= 5;
                        estado->nafta -= 10;
                        estado->recolector1++;
                        estado->nafta_jugador += 10;
                        resp.cantidad = 10;
                        printf("[ESTACION] +10 nafta a Nave %d (costo: 5 deuterio)\n", id);
                    } else {
                        printf("[ESTACION] Sin stock para atender a Nave %d\n", id);
                    }
                    pthread_mutex_unlock(&estado->mutex_nafta);
                    break;

                case 2: // oxigeno: la nave paga 1 de cada mineral y recibe 10 de oxigeno
                    pthread_mutex_lock(&estado->mutex_oxigeno);
                    if (estado->mutexio >= 1 &&
                        estado->semaforita >= 1 &&
                        estado->kernelio >= 1 &&
                        estado->oxigeno >= 10) {
                        estado->mutexio--;
                        estado->semaforita--;
                        estado->kernelio--;
                        estado->oxigeno -= 10;
                        estado->recolector0 += 3;
                        estado->oxigeno_jugador += 10;
                        resp.cantidad = 10;
                        printf("[ESTACION] +10 oxigeno a Nave %d\n", id);
                    } else {
                        printf("[ESTACION] Recursos insuficientes para Nave %d\n", id);
                    }
                    pthread_mutex_unlock(&estado->mutex_oxigeno);
                    break;
            }

            // mando la respuesta a nave.c con lo que se pudo cargar
            mq_send(cola_respuesta, (char *)&resp, sizeof(RespuestaEstacion), 0);
            fflush(stdout);
        }
    }

    mq_close(cola_ypf);
    mq_close(cola_respuesta);
    return NULL;
}
    mq_close(cola);
    return NULL;
}

void *hiloConsumo(void *arg) {
    EstadoYPF *estado = (EstadoYPF *) arg;
    while (estado->corriendo) {
        sleep(1); // Monitoreo de stock continuo
        
        pthread_mutex_lock(&estado->mutex_nafta);
        // Si las naves compraron toda la nafta disponible, se inicia el apagado ordenado
        if (estado->nafta <= 0) {
            estado->corriendo = 0;
        }
        pthread_mutex_unlock(&estado->mutex_nafta);
    }
    return NULL;
}
}