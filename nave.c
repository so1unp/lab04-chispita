
/*
    COMANDO PARA COMPILAR:
    gcc nave.c -lncursesw -pthread -o nave

    !!IMPORTANTE!!
    para que la interfaz se vea de forma correcta usar la fuente en tamaño 14 y la terminal 
    maximizada (no pantalla completa)
*/ 

#include <ncurses.h>
#include <time.h>
#include <locale.h>
#include <stdbool.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define corIniX 21 // coordenada x inicial de la nave 
#define corIniY 4 // coordenada y inicial de la nave

#define maxCorX 101 // coordenada x máxima para la nave
#define maxCorY 24 // coordenada y máxima para la nave

#define columnas 21
#define filas 11

#define velocidad 900000 // velocidad/cooldown del movimiento de la nave

/*-----VARIABLES INICIALIZADAS-----*/
char *nave, *modo;
int x, y, celAlt, celAnch, maxY, maxX, ancho, alto, inicioX, inicioY, c, ini, misil;
clock_t ult;
pthread_mutex_t mutex;
int volatile dibujar;

/*SIN LÓGICA APLICADA*/
int combustible, oxigeno, naves, mutexio, semaforita, kernelio;
bool modoDisparo;
/*********************/
/*---------------------------------*/

void *banner(void *param);
void *dibujarPantalla(void *param);
void *movimientoNave(void *arg);

int main() {
    setlocale(LC_ALL, "");
    initscr();
    noecho();
    curs_set(0);
    nodelay(stdscr, TRUE); // SE DEBE USAR SI O SI, POR AHORA

    pthread_t hilo_mov, hilo_banner, hilo_pantalla; // declaración de hilos
    pthread_mutex_init(&mutex, NULL); // declaración de mutex
    
    /*COLORES*/
    start_color();
    init_color(8, 200, 200, 200); // gris
    init_pair(1, 8, COLOR_BLACK);
    init_pair(2, COLOR_GREEN, COLOR_BLACK);
    
    ini = 0;
    ult = 0;
    nave = "↓";
    modo = "NAVE";
    x = corIniX;
    y = corIniY;
    celAlt = 2; // altura de cada celda
    celAnch = 4; // ancho de cada celda
    getmaxyx(stdscr, maxY, maxX);
    ancho = columnas * celAnch + 1;
    alto  = filas * celAlt + 1;
    inicioX = (maxX - ancho) / 2;
    inicioY = (maxY - alto) / 2;
    dibujar=1;

    /*SIN LÓGICA APLICADA*/
    combustible = 100;
    oxigeno = 100;
    naves = 1;
    mutexio = 0;
    semaforita = 0;
    kernelio = 0;
    modoDisparo = false;
    misil = 3;
    /********************/

    /*DISPARADOR DE HILOS*/
    pthread_create(&hilo_mov, NULL, movimientoNave, NULL);
    pthread_create(&hilo_banner,NULL,(void *)banner, NULL);
    pthread_create(&hilo_pantalla,NULL,(void *)dibujarPantalla, NULL);

    pthread_join(hilo_mov, NULL);
    pthread_join(hilo_banner, NULL);
    pthread_join(hilo_pantalla, NULL);
    
    endwin();
    return 0;
}   

/*HILO PARA ANIMACIÓN DEL BANNER*/
void *banner(void *param) {  
    
    char titulo[] = ">>>>>>COSMIKERNEL>>>>>>";
    int longitud = strlen(titulo);
    char buffer[100];
    int i = 0;

    while(1){
        memset(buffer, 0, sizeof(buffer)); 
        for(int j=0; j<longitud; j++){
            buffer[j] = titulo[(i+j) % longitud];
        }

        pthread_mutex_lock(&mutex);
        attron(COLOR_PAIR(2));
        mvprintw(0, 49, "%s", buffer);
        attroff(COLOR_PAIR(2)); 
        pthread_mutex_unlock(&mutex);
        
        i = (i + 1) % longitud; 
        usleep(200000); 
    }  
    
    pthread_exit(0);   
}

/*HILO PARA DIBUJAR LA INTERFAZ*/
void *dibujarPantalla(void *arg){   
    while(1){
        pthread_mutex_lock(&mutex);
        if(dibujar==1){
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
            mvprintw(26, 21, "MODO:%s", modo);
            mvprintw(2, 21, "COMB:%d%%", combustible);
            mvprintw(2, 57, "OXÍG:%d%%", oxigeno);
            mvprintw(2, 93, "NAVES:%d/5", naves);
            mvprintw(26, 95, "KERN:%d", kernelio);
            mvprintw(26, 83, "SEMA:%d", semaforita);
            mvprintw(26, 71, "MUTE:%d", mutexio);

            if(modoDisparo){
                mvprintw(26, 34, "MISIL:%d", misil);
            }else{

            }
            attroff(COLOR_PAIR(2));
            refresh();
            dibujar = 0;
        }
        pthread_mutex_unlock(&mutex);
    }
}

/*HILO DE MOVIMIENTO*/
void *movimientoNave(void *arg){
    while(1){
        pthread_mutex_lock(&mutex);
        c = getch();

        /*SWITCH PARA EL MODO DISPARO*/
        switch(c){
            case 'e':
            modoDisparo = !modoDisparo;
                
            if(modoDisparo){ // cambio el texto de 'modo' y dibujo
                modo= "DISP";
                dibujar=1;
            }else{
                modo= "NAVE";
                dibujar=1;
            }
            break;
        }

        if(!modoDisparo){
            if(c!=ERR){
                clock_t ahora = clock();
                if(!ini || (ahora-ult)>=velocidad){
                    
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
                    dibujar=1;
                }
            }
        }

        /*MODO DISPARO*/
        if(modoDisparo){
            if(c!=ERR){
                clock_t ahora = clock();
                if((ahora-ult)>=velocidad){
                    switch(c){

                        case 'w': misil--;
                        nave= "↑!";
                        break;
                            
                        case 's': misil--;
                        nave= "↓!";
                        break;
                            
                        case 'a': misil--;
                        nave= "←!";
                        break;
                            
                        case 'd': misil--;
                        nave= "→!";
                        break;
                    }
                    ult=ahora;
                    ini=1;
                    if(misil<0){
                        misil=0;
                    }
                    dibujar=1;
                }
            }
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
        pthread_mutex_unlock(&mutex);
    }

}