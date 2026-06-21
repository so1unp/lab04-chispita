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

    // Esperamos el próximo mensaje de cualquier nave
    if (mq_receive(cola, (char *)&msj, sizeof(MensajeNave), NULL) == -1)
        continue;

    // Mensaje especial de cierre: salimos limpiamente
    if (msj.tipo_operacion == -1)
        break;

    int nave_id = msj.id_nave;

    // Ignoramos mensajes de naves que no existen o ya se desconectaron
    bool nave_valida = nave_id >= 0
                    && nave_id < MAX_NAVES
                    && mapa_compartido->naves[nave_id].activa;

    if (!nave_valida) {
        printf("[ESTACION] Mensaje ignorado: nave %d inválida o inactiva.\n", nave_id);
        continue;
    }

    // ── Operación 1: Recarga de nafta (la nave paga con deuterio) ──────────
    if (msj.tipo_operacion == 1) {

        pthread_mutex_lock(&estado->mutex_nafta);

        int combustible_actual  = mapa_compartido->naves[nave_id].combustible;
        int deuterio_nave       = mapa_compartido->naves[nave_id].deuterio;

        // Cargamos hasta 10 unidades, o lo que falte para llegar a 100
        int cantidad_a_cargar   = (combustible_actual > 90) ? (100 - combustible_actual) : 10;

        // Tasa de cambio: cada 2 unidades de nafta cuestan 1 deuterio
        int costo_deuterio      = cantidad_a_cargar / 2;

        bool nave_necesita      = combustible_actual < 100;
        bool estacion_tiene     = estado->nafta >= cantidad_a_cargar;
        bool nave_puede_pagar   = deuterio_nave   >= costo_deuterio;

        if (nave_necesita && estacion_tiene && nave_puede_pagar) {
            // Cobramos a la nave y acreditamos a la estación
            mapa_compartido->naves[nave_id].deuterio  -= costo_deuterio;
            estado->deuterio                          += costo_deuterio;

            // Transferimos la nafta
            estado->nafta                             -= cantidad_a_cargar;
            mapa_compartido->naves[nave_id].combustible += cantidad_a_cargar;
            estado->recolector1++;

            printf("[ESTACION] Nave %d recargó %d de nafta (pagó %d deuterio).\n",
                   nave_id, cantidad_a_cargar, costo_deuterio);
        } else {
            printf("[ESTACION] Recarga rechazada para nave %d: "
                   "tanque lleno, deuterio insuficiente, o estación sin stock.\n", nave_id);
        }

        pthread_mutex_unlock(&estado->mutex_nafta);
    }

    // ── Operación 2: Recarga de oxígeno (la nave paga con minerales) ───────
    else if (msj.tipo_operacion == 2) {

        pthread_mutex_lock(&estado->mutex_oxigeno);

        int oxigeno_actual  = mapa_compartido->naves[nave_id].oxigeno;

        // El precio es 1 unidad de cada mineral: mutexio, semaforita y kernelio
        bool nave_tiene_minerales = mapa_compartido->naves[nave_id].mutexio   >= 1
                                 && mapa_compartido->naves[nave_id].semaforita >= 1
                                 && mapa_compartido->naves[nave_id].kernelio   >= 1;

        bool nave_necesita    = oxigeno_actual < 100;
        bool estacion_tiene   = estado->oxigeno >= 10;

        if (nave_necesita && estacion_tiene && nave_tiene_minerales) {
            int cantidad_a_cargar = (oxigeno_actual > 90) ? (100 - oxigeno_actual) : 10;

            // Cobramos los tres minerales
            mapa_compartido->naves[nave_id].mutexio--;
            mapa_compartido->naves[nave_id].semaforita--;
            mapa_compartido->naves[nave_id].kernelio--;

            // Transferimos el oxígeno
            estado->oxigeno                          -= cantidad_a_cargar;
            mapa_compartido->naves[nave_id].oxigeno  += cantidad_a_cargar;
            estado->recolector0 += 3; // 3 minerales cobrados

            printf("[ESTACION] Nave %d recargó %d de oxígeno (pagó 1 mutexio + 1 semaforita + 1 kernelio).\n",
                   nave_id, cantidad_a_cargar);
        } else {
            printf("[ESTACION] Recarga rechazada para nave %d: "
                   "oxígeno lleno, minerales insuficientes, o estación sin stock.\n", nave_id);
        }

        pthread_mutex_unlock(&estado->mutex_oxigeno);
    }

    fflush(stdout);
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