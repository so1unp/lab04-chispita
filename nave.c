/*
    COMANDO PARA COMPILAR:
    gcc nave.c -lncursesw -o nave
*/

#include <ncurses.h>
#include <time.h>
#include <locale.h>
#include <stdbool.h>
#include <pthread.h>
#include <stdlib.h>

#define corIniX 21 // coordenada x inicial de la nave 
#define corIniY 4 // coordenada y inicial de la nave

#define maxCorX 101 // coordenada x máxima para la nave
#define maxCorY 24 // coordenada y máxima para la nave

#define columnas 21
#define filas 11

/*-----VARIABLES INICIALIZADAS-----*/
time_t ultimoMov;
char *nave, *modo;
int x, y, celAlt, celAnch, maxY, maxX, ancho, alto, inicioX, inicioY;

/*SIN LÓGICA APLICADA*/
int combustible, oxigeno, naves, mutexio, semaforita, kernelio;
bool modoDisparo;
/********************/
/*-----VARIABLES INICIALIZADAS-----*/

/*HILO DE INTERFAZ*/
void *dibujarPantalla(void *arg){
        attron(COLOR_PAIR(1));
        for (int i = 0; i <= filas; i++) {
            for (int j = 0; j <= columnas * celAnch; j++) {
                mvaddch(inicioY + i * celAlt, inicioX + j, '-');
            }
        }

        for (int i = 0; i <= filas * celAlt; i++) {
            for (int j = 0; j <= columnas; j++) {
                mvaddch(inicioY + i, inicioX + j * celAnch, '|');
            }
        }

        for (int i = 0; i <= filas; i++) {
            for (int j = 0; j <= columnas; j++) {
                mvaddch(inicioY + i * celAlt, inicioX + j * celAnch, '+');
            }
        }
        attroff(COLOR_PAIR(1)); 

        attron(COLOR_PAIR(2));
        mvprintw(0, 49, ">>>>>>-COSMIKERNEL->>>>>>");
        mvaddstr(y, x, nave);
        mvprintw(26, 21, "MODO:%s", modo);
        mvprintw(2, 21, "COMB:%d%%", combustible);
        mvprintw(2, 57, "OXÍG:%d%%", oxigeno);
        mvprintw(2, 93, "NAVES:%d/5", naves);
        mvprintw(26, 95, "KERN:%d", kernelio);
        mvprintw(26, 83, "SEMA:%d", semaforita);
        mvprintw(26, 71, "MUTE:%d", mutexio);
        attroff(COLOR_PAIR(2));
        pthread_exit(NULL);
}

int main() {
    setlocale(LC_ALL, "");
    initscr();
    noecho();
    curs_set(0); 
    nodelay(stdscr, TRUE);

    pthread_t hilo_pantalla;
    int resultado;

    start_color();
    init_color(8, 200, 200, 200); // gris
    init_pair(1, 8, COLOR_BLACK);
    init_pair(2, COLOR_GREEN, COLOR_BLACK);
    
    ultimoMov = 0;
    nave = "↓";
    modo = "NAVE";
    x = corIniX;
    y = corIniY;

    /*SIN LÓGICA APLICADA*/
    combustible = 100;
    oxigeno = 100;
    naves = 1;
    mutexio = 0;
    semaforita = 0;
    kernelio = 0;
    modoDisparo = false;
    /********************/

    celAlt = 2; // altura de cada celda
    celAnch = 4; // ancho de cada celda

    getmaxyx(stdscr, maxY, maxX);
    ancho = columnas * celAnch + 1;
    alto  = filas * celAlt + 1;
    inicioX = (maxX - ancho) / 2;
    inicioY = (maxY - alto) / 2;

    refresh();

    while (1) {
        erase();

        time_t relojAhora = time(NULL);

        resultado = pthread_create(&hilo_pantalla, NULL, dibujarPantalla, NULL);

        if(resultado!=0){
            perror("Error al crear el hilo");
            exit(1);
        }

        pthread_join(hilo_pantalla, NULL);

        refresh();
        int c = getch();

        //if(c!=ERR && relojAhora-ultimoMov>=1){ /*COOLDOWN (TODAVÍA NO SE DECIDE SI AGREGARLO)*/
            
        if(!modoDisparo){

            /*MODO NAVE*/
            switch(c) {
                
            case 'w': y=y-celAlt;
            nave= "↑";
            if(y<corIniY){
                y=maxCorY;
            }
            break;
            
            case 's': y=y+celAlt; 
            nave= "↓";
            if(y>maxCorY){
                y=corIniY;
            }
            break;
            
            case 'a': x=x-celAnch; 
            nave= "←";
            if(x<corIniX){
                x=maxCorX;
            }
            break;
            
            case 'd': x=x+celAnch; 
            nave= "→";
            if(x>maxCorX){
                x=corIniX;
            }
            break;
            }
        }

        /*MODO DISPARO (SIN LÓGICA APLICADA)*/
        switch(c){

            case 'w':
            nave= "↑";
            break;
            
            case 's': 
            nave= "↓";
            break;
            
            case 'a':
            nave= "←";
            break;
            
            case 'd':
            nave= "→";
            break;

            case 'e':
            modoDisparo = !modoDisparo;
            
            if(modoDisparo){
                modo= "DISP";
            }else{
                modo= "NAVE";
            }
            break;
        }

        //ultimoMov=relojAhora; /*COOLDOWN (TODAVÍA NO SE DECIDE SI AGREGARLO)*/
        //} /*COOLDOWN (TODAVÍA NO SE DECIDE SI AGREGARLO)*/
    }
    
    endwin();
    return 0;
}   
