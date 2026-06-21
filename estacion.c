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
#include "compartido.h"

#define QUEUE_PERMISSIONS 0666

// Memoria compartida global
MapaEspacial *mapa_compartido = NULL;

void *hiloAcciones(void *arg);
void *hiloConsumo(void *arg);

int main() {
    // Enlazar Memoria Compartida
    int fd = shm_open("/mapa_espacial", O_RDWR, 0666);
    if (fd == -1) { perror("Ejecutar ./servidor primero"); exit(1); }
    
    mapa_compartido = (MapaEspacial *)mmap(NULL, sizeof(MapaEspacial), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (mapa_compartido == MAP_FAILED) { perror("Error mmap"); exit(1); }

    // Subimos los recursos iniciales para que la estación opere cómodamente
    EstadoYPF estado = {
        .MAXnaves = 3, 
        .corriendo = 1, 
        .oxigeno = 9999, 
        .nafta = 9999,      // Stock inicial de nafta disponible para vender
        .deuterio = 0,     // La estación arranca sin deuterio (lo recauda de las naves)
        .recolector0 = 0, 
        .recolector1 = 0
    };

    struct mq_attr attr = { .mq_flags = 0, .mq_maxmsg = 10, .mq_msgsize = sizeof(MensajeNave), .mq_curmsgs = 0 };
    mq_unlink(COLA_ESTACION);
    mqd_t cola_principal = mq_open(COLA_ESTACION, O_CREAT | O_RDWR, QUEUE_PERMISSIONS, &attr);
    if (cola_principal == (mqd_t)-1) { perror("Error crear cola"); exit(1); }

    pthread_mutex_init(&estado.mutex_nafta, NULL);
    pthread_mutex_init(&estado.mutex_oxigeno, NULL);

    pthread_t hilo_atencion, hilo_desgaste;
    pthread_create(&hilo_atencion, NULL, hiloAcciones, &estado);
    pthread_create(&hilo_desgaste, NULL, hiloConsumo, &estado);

    system("clear");
    printf("Estacion YPF ONLINE.\n");
    fflush(stdout);

    // 1. Esperamos que el hilo de consumo termine (cuando las naves agoten la nafta vendible)
    pthread_join(hilo_desgaste, NULL);

    // 2. Mandamos la Poison Pill (mensaje -1) para destrabar y cerrar el hilo de acciones
    MensajeNave cierre = { .tipo_operacion = -1, .id_nave = 0 };
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
    if (cola == (mqd_t)-1) { perror("Error cola"); pthread_exit(NULL); }

    while (estado->corriendo) {
        if (mq_receive(cola, (char *)&msj, sizeof(MensajeNave), NULL) != -1) {
            int id = msj.id_nave;

            // MENSAJE DE ENVENENAMIENTO: Cierre ordenado
            if (msj.tipo_operacion == -1) {
                break; 
            }

            // Validación rápida de la nave
            if (id < 0 || id >= MAX_NAVES || !mapa_compartido->naves[id].activa) {
                printf("[ESTACION] Mensaje ignorado: Nave %d inválida o inactiva.\n", id);
                continue;
            }

            if (msj.tipo_operacion == 1) { // RECARGA DE NAFTA (PAGO CON DEUTERIO)
                pthread_mutex_lock(&estado->mutex_nafta);
                int actual = mapa_compartido->naves[id].combustible;
                
                // Calculamos cuánto quiere cargar la nave (máximo 10, o menos si llega a 100)
                int aCargar = (actual > 90) ? (100 - actual) : 10;
                
                // Relación: 1 deuterio = 2 de combustible. Por ende: costo = aCargar / 2
                int costoDeuterio = aCargar / 2; 

                // Verificamos stock de la estación y si la nave tiene suficiente Deuterio en compartido.h
                if (actual < 100 && estado->nafta >= aCargar && mapa_compartido->naves[id].deuterio >= costoDeuterio) {
                    // Cobrar a la nave y actualizar stock de la estación
                    mapa_compartido->naves[id].deuterio -= costoDeuterio;
                    estado->deuterio += costoDeuterio; // La estación guarda el deuterio cobrado
                    
                    estado->nafta -= aCargar;
                    estado->recolector1++; // Contador de cargas de nafta
                    mapa_compartido->naves[id].combustible += aCargar;
                    
                    printf("[ESTACION] +%d Nafta a Nave %d (Costo: %d Deuterio)\n", aCargar, id, costoDeuterio);
                } else {
                    printf("[ESTACION] Rechazado: Nave %d llena, sin Deuterio suficiente o YPF sin stock.\n", id);
                }
                pthread_mutex_unlock(&estado->mutex_nafta);
            } 
            else if (msj.tipo_operacion == 2) { // RECARGA DE OXÍGENO (PAGO CON MINERALES)
                pthread_mutex_lock(&estado->mutex_oxigeno);
                int actual = mapa_compartido->naves[id].oxigeno;
                bool puede_pagar = (mapa_compartido->naves[id].mutexio >= 1 &&
                                    mapa_compartido->naves[id].semaforita >= 1 &&
                                    mapa_compartido->naves[id].kernelio >= 1);

                if (actual < 100 && estado->oxigeno >= 10 && puede_pagar) {
                    int aCargar = (actual > 90) ? (100 - actual) : 10;
                    
                    // Cobrar minerales del mapa compartido
                    mapa_compartido->naves[id].mutexio--;
                    mapa_compartido->naves[id].semaforita--;
                    mapa_compartido->naves[id].kernelio--;
                    
                    estado->oxigeno -= aCargar;
                    estado->recolector0 += 3;
                    mapa_compartido->naves[id].oxigeno += aCargar;
                    printf("[ESTACION] +%d Oxigeno a Nave %d\n", aCargar, id);
                } else {
                    printf("[ESTACION] Recursos insuficientes/Nave %d llena.\n", id);
                }
                pthread_mutex_unlock(&estado->mutex_oxigeno);
            }
            fflush(stdout);
        }
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