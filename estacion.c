// librerias que use yo
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
// heard que creamos para las variables compartidas
#include "compartido.h"

#define QUEUE_PERMISSIONS 0666 // permisos de la cola de mensajes

// Memoria compartida global
MapaEspacial *mapa_compartido = NULL;
int mi_id_global; //para que el hilo sepa su id

// funciones de los hilos
void *hiloAcciones(void *arg);
void *hiloConsumo(void *arg);

int main(int argc, char *argv[])
{
    if (argc < 2) {
        printf("Error. Uso: ./estacion <id_estacion>\n");
        exit(1);
    }

    mi_id_global = atoi(argv[1]);

    // Enlazar Memoria Compartida
    int fd = shm_open("/mapa_espacial", O_RDWR, 0666);

    if (fd == -1)
    {
        perror("Ejecutar ./servidor primero");
        exit(1);
    }

    mapa_compartido = (MapaEspacial *)
        mmap(NULL, sizeof(MapaEspacial), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

    if (mapa_compartido == MAP_FAILED)
    {
        perror("Error de  mmap");
        exit(1);
    }

    // inicio de estacion como arranca la estacion
    EstadoYPF estado = {
        .MAXnaves = 3,
        .corriendo = 1,
        .oxigeno = 9999,
        .nafta = 9999,
        .deuterio = 0,
        .recolector0 = 0,
        .recolector1 = 0};

    struct mq_attr cola_attr = {
        .mq_flags = 0,
        .mq_maxmsg = 10,
        .mq_msgsize = sizeof(MensajeNave),
        .mq_curmsgs = 0};

        
    char nombre_cola[50];
    sprintf(nombre_cola, "/cola_estacion_%d", mi_id_global);

    mq_unlink(nombre_cola);
    mqd_t cola_principal = mq_open(nombre_cola, O_CREAT | O_RDWR, QUEUE_PERMISSIONS, &cola_attr);

    if (cola_principal == (mqd_t)-1)
    {
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

    // espera a que el hilo "consumo" termine
    pthread_join(hilo_desgaste, NULL);
    // -1 para cerrar el hilo estacion
    MensajeNave cierre = {
        .tipo_operacion = -1,
        .id_nave = 0};
    mq_send(cola_principal, (char *)&cierre, sizeof(MensajeNave), 0);

    // espera a que el hilo de atencion termine
    pthread_join(hilo_atencion, NULL);

    // limpia los recursos
    pthread_mutex_destroy(&estado.mutex_nafta);
    pthread_mutex_destroy(&estado.mutex_oxigeno);
    mq_close(cola_principal);
    mq_unlink(nombre_cola);

    printf("Estacion cerrada correctamente.\n");
    return 0;
}

void *hiloAcciones(void *arg)
{
    EstadoYPF *estado = (EstadoYPF *)arg;
    MensajeNave msj;

    char nombre_cola[50];
    sprintf(nombre_cola, "/cola_estacion_%d", mi_id_global);

    mqd_t cola = mq_open(nombre_cola, O_RDONLY);
    if (cola == (mqd_t)-1)
    {
        perror("Error cola");
        pthread_exit(NULL);
    }
    while (estado->corriendo)
    {

        if (mq_receive(cola, (char *)&msj, sizeof(MensajeNave), NULL) != -1)
        {

            int id = msj.id_nave;

            // La estación recibió la señal de cierre
            if (msj.tipo_operacion == -1)
            {
                break;
            }

            // vericio que la nave este activaa
            if (id < 0 || id >= mapa_compartido->cantidad_naves_permitidas)
            {
                printf("error: id negativo o mayor al maximo de naves");
                continue;
            }
            else if (!mapa_compartido->naves[id].activa)
            {
                printf("error: nave inactiva");
                continue;
            }

            // Recarga de combustible
            if (msj.tipo_operacion == 1)
            {

                pthread_mutex_lock(&estado->mutex_nafta);

                int combustible_actual = mapa_compartido->naves[id].combustible;

                int combustible_a_cargar;

                if (combustible_actual > 90)
                {
                    combustible_a_cargar = 100 - combustible_actual;
                }
                else
                {
                    combustible_a_cargar = 10;
                }

                int costo_deuterio = combustible_a_cargar / 2;
                if (combustible_actual < 100)
                {

                    if (estado->nafta >= combustible_a_cargar)
                    {

                        if (mapa_compartido->naves[id].deuterio >= costo_deuterio)
                        {

                            mapa_compartido->naves[id].deuterio -= costo_deuterio;
                            estado->deuterio += costo_deuterio;

                            estado->nafta -= combustible_a_cargar;
                            estado->recolector1++;

                            mapa_compartido->naves[id].combustible += combustible_a_cargar;

                            printf("[ESTACION] Nave %d cargo %d de combustible pagando %d deuterio.\n", id, combustible_a_cargar, costo_deuterio);
                        }
                        else
                        {
                            printf("[ESTACION] La nave %d no tiene suficiente deuterio.\n", id);
                        }
                    }
                    else
                    {
                        printf("[ESTACION] No hay nafta\n");
                    }
                }
                else
                {
                    printf("[ESTACION] La nave %d tiene el tanque lleno.\n", id);
                }

                pthread_mutex_unlock(&estado->mutex_nafta);
            }

            //  oxigeno
            else if (msj.tipo_operacion == 2)
            {

                pthread_mutex_lock(&estado->mutex_oxigeno);

                int oxigeno_actual = mapa_compartido->naves[id].oxigeno;

                bool puede_pagar = false;

                if (mapa_compartido->naves[id].mutexio >= 1 &&
                    mapa_compartido->naves[id].semaforita >= 1 &&
                    mapa_compartido->naves[id].kernelio >= 1)
                {

                    puede_pagar = true;
                }

                if (oxigeno_actual < 100)
                {

                    if (estado->oxigeno >= 10)
                    {

                        if (puede_pagar)
                        {

                            int oxigeno_a_cargar;

                            if (oxigeno_actual > 90)
                            {
                                oxigeno_a_cargar = 100 - oxigeno_actual;
                            }
                            else
                            {
                                oxigeno_a_cargar = 10;
                            }

                            mapa_compartido->naves[id].mutexio--;
                            mapa_compartido->naves[id].semaforita--;
                            mapa_compartido->naves[id].kernelio--;

                            estado->oxigeno -= oxigeno_a_cargar;
                            estado->recolector0 += 3;

                            mapa_compartido->naves[id].oxigeno += oxigeno_a_cargar;

                            printf("[ESTACION %d] Nave %d cargo %d de oxigeno.\n", mi_id_global, id, oxigeno_a_cargar);
                        }
                        else
                        {
                            printf("[ESTACION %d] La nave %d no tenes los minerales suficientes\n", mi_id_global, id);
                        }
                    }
                    else
                    {
                        printf("[ESTACION %d] No tenemos oxigeno hoy\n", mi_id_global);
                    }
                }
                else
                {
                    printf("[ESTACION %d] Nave %d ya tenes el oxigeno completo\n", mi_id_global, id);
                }

                pthread_mutex_unlock(&estado->mutex_oxigeno);
            }

            fflush(stdout);
        }
    }

    mq_close(cola);
    return NULL;
}

void *hiloConsumo(void *arg)
{
    EstadoYPF *estado = (EstadoYPF *)arg;
    while (estado->corriendo)
    {
        sleep(1);

        pthread_mutex_lock(&estado->mutex_nafta);
        // Si las naves compraron toda la nafta disponible se apaga
        if (estado->nafta <= 0)
        {
            estado->corriendo = 0;
        }
        pthread_mutex_unlock(&estado->mutex_nafta);
    }
    return NULL;
}
