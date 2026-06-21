#ifndef COMPARTIDO_H
#define COMPARTIDO_H

#include <pthread.h>

#define COLA_ESTACION  "/cola_ypf"
#define COLA_RESPUESTA "/cola_respuesta"

typedef struct {
    int MAXnaves;
    int corriendo;
    int oxigeno;
    int nafta;
    int deuterio;
    int mutexio;
    int semaforita;
    int kernelio;
    int recolector0;
    int recolector1;
    int oxigeno_jugador;
    int nafta_jugador;
    pthread_mutex_t mutex_nafta;
    pthread_mutex_t mutex_oxigeno;
} EstadoYPF;

typedef struct {
    int tipo_operacion; // 1: nafta, 2: oxigeno
} MensajeNave;

typedef struct {
    int tipo;        // 1: nafta, 2: oxigeno
    int cantidad;    // cuánto se recibió (0 si no había stock)
} RespuestaEstacion;

#endif