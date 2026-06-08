#ifndef COMPARTIDO_H // Esta libreria es para compartir estructuras y constantes entre el servidor y la nave
#define COMPARTIDO_H // Evitamos que se incluya mas de una vez

/**
 * Definimos las estructuras que vamos a usar en el servidor
 */
#define FILAS 40 
#define COLUMNAS 100
#define MAX_ASTEROIDES_FISICOS 50 


/**
 * Asteroide en el mapa 
 */
typedef struct {
    int x;
    int y;
    int deuterio;
    int mutexio;
    int semaforita;
    int kernelio;
    int activo; 
} Asteroide;


/**
 * Estacion en el mapa, son 3 estaciones maximo
 */
typedef struct {
    int x;
    int y;
    int combustible;
} Estacion;


/**
 * Mapa espacial, que contiene la matriz del mapa, una variable de si el juego esta activo o no,
 * la cantidad maxima de asteroides, las estaciones que son 3 
 * y los minerales con los precios
 */
typedef struct {
    char matriz[FILAS][COLUMNAS];
    int juego_activo;

    Asteroide asteroides[MAX_ASTEROIDES_FISICOS];
    Estacion estaciones[3];

    int precio_deuterio;
    int precio_mutexio;
    int precio_semaforita;
    int precio_kernelio;
    int precio_combustible;
    int precio_oxigeno;
} MapaEspacial;

#endif // COMPARTIDO_H