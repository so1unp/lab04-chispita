#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ncurses.h>
#include <time.h>
#include <unistd.h>

#define FILAS 40
#define COLUMNAS 100

#define MAX_ASTEROIDES_FISICOS 50 

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
    int combustible;
} Estacion;

typedef struct {
    int estaciones;
    int asteroides;
    int precio_deuterio;
    int precio_mutexio;
    int precio_semaforita;
    int precio_kernelio;
    int precio_combustible;
    int precio_oxigeno;
} ConfiguracionJuego;

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

void cargar_configuracion(const char *nombre_archivo, ConfiguracionJuego *config) {
    FILE *archivo = fopen(nombre_archivo, "r");
    if (archivo == NULL) {
        perror("Error al abrir el archivo de configuracion");
        exit(EXIT_FAILURE);
    }

    char clave[50];
    int valor;

    while (fscanf(archivo, "%s %d", clave, &valor) != EOF) {
        if (strcmp(clave, "ESTACIONES") == 0) {
            config->estaciones = valor;
            if (config->estaciones > 3) config->estaciones = 3;
        } 
        else if (strcmp(clave, "ASTEROIDES") == 0) {
            config->asteroides = valor;
            if (config->asteroides > MAX_ASTEROIDES_FISICOS) config->asteroides = MAX_ASTEROIDES_FISICOS;
        }
        else if (strcmp(clave, "PRECIO_DEUTERIO") == 0) config->precio_deuterio = valor;
        else if (strcmp(clave, "PRECIO_MUTEXIO") == 0) config->precio_mutexio = valor;
        else if (strcmp(clave, "PRECIO_SEMAFORITA") == 0) config->precio_semaforita = valor;
        else if (strcmp(clave, "PRECIO_KERNELIO") == 0) config->precio_kernelio = valor;
        else if (strcmp(clave, "PRECIO_COMBUSTIBLE") == 0) config->precio_combustible = valor;
        else if (strcmp(clave, "PRECIO_OXIGENO") == 0) config->precio_oxigeno = valor;
    }
    fclose(archivo);
}

void inicializar_mapa(MapaEspacial *mapa) {
    for (int i = 0; i < FILAS; i++) {
        for (int j = 0; j < COLUMNAS; j++) {
            if ((i == 0 && j == 0) || (i == 0 && j == COLUMNAS - 1) || 
                (i == FILAS - 1 && j == 0) || (i == FILAS - 1 && j == COLUMNAS - 1)) {
                mapa->matriz[i][j] = '+';
            } 
            else if (i == 0 || i == FILAS - 1) {
                mapa->matriz[i][j] = '-';
            } 
            else if (j == 0 || j == COLUMNAS - 1) {
                mapa->matriz[i][j] = '|';
            } 
            else {
                mapa->matriz[i][j] = ' '; 
            }
        }
    }
}

void generar_asteroides(MapaEspacial *mapa, int cantidad) {
    int generados = 0;
    while (generados < cantidad) {
        int x = (rand() % (COLUMNAS - 2)) + 1;
        int y = (rand() % (FILAS - 2)) + 1;
        
        if (mapa->matriz[y][x] == ' ') {
            mapa->matriz[y][x] = '*'; 
            
            mapa->asteroides[generados].x = x;
            mapa->asteroides[generados].y = y;
            mapa->asteroides[generados].deuterio = rand() % 15;
            mapa->asteroides[generados].mutexio = rand() % 15;
            mapa->asteroides[generados].semaforita = rand() % 15;
            mapa->asteroides[generados].kernelio = rand() % 15;
            mapa->asteroides[generados].activo = 1;
            
            generados++;
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Error: Falta el archivo de configuración.\n");
        printf("Uso correcto: %s <archivo_de_configuracion.txt>\n", argv[0]);
        exit(EXIT_FAILURE); 
    }

    ConfiguracionJuego config;
    cargar_configuracion(argv[1], &config);

    MapaEspacial mapa_servidor;
    srand(time(NULL));
    inicializar_mapa(&mapa_servidor);
    
    generar_asteroides(&mapa_servidor, config.asteroides);
    
    mapa_servidor.precio_deuterio = config.precio_deuterio;
    mapa_servidor.precio_mutexio = config.precio_mutexio;
    mapa_servidor.precio_semaforita = config.precio_semaforita;
    mapa_servidor.precio_kernelio = config.precio_kernelio;
    mapa_servidor.precio_combustible = config.precio_combustible;
    mapa_servidor.precio_oxigeno = config.precio_oxigeno;
    
    mapa_servidor.juego_activo = 1;

    initscr();
    noecho();
    cbreak();
    curs_set(0);  
    timeout(100); 

    for (int i = 0; i < FILAS; i++) {
        for (int j = 0; j < COLUMNAS; j++) {
            mvaddch(i, j, mapa_servidor.matriz[i][j]);
        }
    }
    mvprintw(FILAS + 1, 0, "--- SERVIDOR DEL SECTOR ESPACIAL ACTIVO ---");
    mvprintw(FILAS + 2, 0, "Archivo cargado: %s | Asteroides generados: %d", argv[1], config.asteroides); 
    mvprintw(FILAS + 3, 0, "Presione 'q' para apagar el servidor.");
    refresh(); 

    while (mapa_servidor.juego_activo) {
        
        for (int i = 1; i < FILAS - 1; i++) {
            for (int j = 1; j < COLUMNAS - 1; j++) {
                mvaddch(i, j, mapa_servidor.matriz[i][j]);
            }
        }
        refresh(); 

        int tecla = getch();
        if (tecla == 'q') {
            mapa_servidor.juego_activo = 0;
        }
    }

    endwin();
    exit(EXIT_SUCCESS);
}