#ifndef COMPARTIDO_H
#define COMPARTIDO_H

#include <pthread.h>

// Definición de la cola
#define COLA_ESTACION "/cola_ypf"

// Estructura de estado compartido
typedef struct {
    int MAXnaves;
    int navesEnEstacion;
    int corriendo;
    int oxigeno;
    int nafta;
    int deuterio;
    int mutexio;
    int semaforita;
    int kernelio;
    
    // Contadores
    int recolector0;
    int recolector1;
    int oxigeno_jugador;
    int nafta_jugador;

    // Mutexes
    pthread_mutex_t mutex_nafta;
    pthread_mutex_t mutex_oxigeno;
} EstadoYPF;

// Estructura del mensaje
typedef struct {
    int tipo_operacion; // 1: Deuterio, 2: Minerales
    // Puedes añadir más campos aquí si los necesitas
} MensajeNave;

#endif