#ifndef COMPARTIDO_H
#define COMPARTIDO_H

#include <pthread.h>

#define FILAS 27
#define COLUMNAS 103
#define MAX_ASTEROIDES_FISICOS 100
#define MAX_NAVES 9 // -- valor modificado
#define MAX_ESTACIONES 10 // -- agregado
//#define COLA_ESTACION "/cola_ypf"  SE BORRA LA COLA HARDCODEADA

/*STRUCT DE ESTADO DE ESTACION.C*/
typedef struct {
    int MAXnaves;
    int corriendo;
    int oxigeno;
    int nafta;
    int deuterio;
    int recolector0;
    int recolector1;

    /*MUTEX PARA EVITAR PROBLEMAS AL DESCONTAR STOCK*/
    pthread_mutex_t mutex_nafta;
    pthread_mutex_t mutex_oxigeno;
} EstadoYPF;

/*STRUCT DE PASAJE DE MENSAJES EN LA COLA (INTERCAMBIOS CON LA ESTACIÓN)*/
typedef struct {
    int tipo_operacion;
    int id_nave;        
} MensajeNave;

/*STRUCTS DE LA MEMORIA COMPARTIDA*/
typedef struct {
    int x;
    int y;
    int deuterio;
    int mutexio;
    int semaforita;
    int kernelio;
    int activo;
} Asteroide;

typedef struct {
    int x;
    int y;
    int activa;
    char modo[15];         
    int combustible;       
    int oxigeno;           
    int kernelio;
    int semaforita;
    int mutexio;
    int deuterio;
    int modoDisparo;       
    int misil;             
    int disparo;
    int xProy;
    int yProy;           
} Nave;

typedef struct {
    int x;
    int y;
    int activa;
} EstacionEspacial;

/*MAPEO DE MMAP DE LA MEMORIA COMPARTIDA*/
typedef struct {
    int cantidad_naves_permitidas; // -- agregado
    int cantidad_estaciones; // -- agregado
    Asteroide asteroides[MAX_ASTEROIDES_FISICOS];
    Nave naves[MAX_NAVES];
    EstacionEspacial estaciones[MAX_ESTACIONES]; // -- modificado, ahora es un arreglo de estaciones y no un único objeto estación
    int juego_activo;
} MapaEspacial;

#endif