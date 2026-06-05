#include <stdio.h>
#include <stdlib.h>
#include <ncurses.h>
#include <time.h>
#include <unistd.h>

#define FILAS 40
#define COLUMNAS 100
#define MAX_ASTEROIDES 15

typedef struct {
    char matriz[FILAS][COLUMNAS];
    int juego_activo;
} MapaEspacial;

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

void generar_asteroides(MapaEspacial *mapa) {
    int generados = 0;
    while (generados < MAX_ASTEROIDES) {
        int x = (rand() % (COLUMNAS - 2)) + 1;
        int y = (rand() % (FILAS - 2)) + 1;
        
        if (mapa->matriz[y][x] == ' ') {
            mapa->matriz[y][x] = '*';
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

    MapaEspacial mapa_servidor;

    srand(time(NULL));
    inicializar_mapa(&mapa_servidor);
    generar_asteroides(&mapa_servidor);
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
    mvprintw(FILAS + 2, 0, "Archivo de configuracion cargado: %s", argv[1]); 
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