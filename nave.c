
/*
    COMANDO PARA COMPILAR:
    gcc nave.c -lncursesw -pthread -o nave

    !!IMPORTANTE!!
    el tamaño de la fuente en la terminal la tengo en 14, muy probablemente deban usar este tamaño 
    para que la interfaz se vea correctamente!
*/

#include <ncurses.h>
#include <time.h>
#include <locale.h>
#include <stdbool.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

#define corIniX 21 // coordenada x inicial de la nave 
#define corIniY 4 // coordenada y inicial de la nave

#define maxCorX 101 // coordenada x máxima para la nave
#define maxCorY 24 // coordenada y máxima para la nave

#define columnas 21
#define filas 11

/*-----VARIABLES INICIALIZADAS-----*/
char *nave, *modo;
int x, y, celAlt, celAnch, maxY, maxX, ancho, alto, inicioX, inicioY, c, ini;
clock_t ult;

/*SIN LÓGICA APLICADA*/
int combustible, oxigeno, naves, mutexio, semaforita, kernelio;
bool modoDisparo;
/*********************/
/*---------------------------------*/

/*FUNCIÓN PARA DIBUJAR LA INTERFAZ*/
void dibujarPantalla(){
    erase();
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
    mvprintw(26, 21, "MODO:%s", modo);
    mvprintw(2, 21, "COMB:%d%%", combustible);
    mvprintw(2, 57, "OXÍG:%d%%", oxigeno);
    mvprintw(2, 93, "NAVES:%d/5", naves);
    mvprintw(26, 95, "KERN:%d", kernelio);
    mvprintw(26, 83, "SEMA:%d", semaforita);
    mvprintw(26, 71, "MUTE:%d", mutexio);
    attroff(COLOR_PAIR(2));
    refresh();
}

/*HILO DE MOVIMIENTO*/
void *movimientoNave(void *arg){

    while(1){
        c = getch();
        if(!modoDisparo){
            if(c!=ERR){
                clock_t ahora = clock();
                if(!ini || (ahora-ult)>=CLOCKS_PER_SEC){
                    
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
                    ult=ahora;
                    ini=1;
                    dibujarPantalla(); // DIBUJO LA INTERFAZ CADA VEZ QUE MUEVA LA NAVE (REDUCE CARGA DE CPU)
                }
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
                
            if(modoDisparo){ // cambio el texto de 'modo' y dibujo
                modo= "DISP";
                dibujarPantalla(); 
            }else{
                modo= "NAVE";
                dibujarPantalla();
            }
            break;
        }

        switch(c){ // salir del juego de forma inmediata
            case 'p': 
            endwin();
            exit(0);
            break;
        }

        attron(COLOR_PAIR(2));
        mvaddstr(y, x, nave);
        attroff(COLOR_PAIR(2));
    }
}

int main() {
    setlocale(LC_ALL, "");
    initscr();
    noecho();
    curs_set(0);
    nodelay(stdscr, TRUE); // SI LO QUITO REDUCE PARPADEOS Y USA MENOS CPU

    pthread_t hilo_mov;
    int resHilo;

    ini = 0;
    ult = 0;

    start_color();
    init_color(8, 200, 200, 200); // gris
    init_pair(1, 8, COLOR_BLACK);
    init_pair(2, COLOR_GREEN, COLOR_BLACK);

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

    /* 
        al quitar nodelay(), getch() queda esperando alguna tecla y luego se 
        dibuja la pantalla, entonces dibujo la pantalla por lo menos una vez
        para evitar la pantalla en negro
    */
    dibujarPantalla();
    attron(COLOR_PAIR(2));
    mvaddstr(y, x, nave);
    attroff(COLOR_PAIR(2));
    /*----------------------------------------------------------------------*/

    resHilo = pthread_create(&hilo_mov, NULL, movimientoNave, NULL);
    if(resHilo!=0){
        perror("¡Error al crear el hilo 'hilo_mov'!");
        exit(1);
    }

    pthread_join(hilo_mov, NULL);
    
    endwin();
    return 0;
}   
