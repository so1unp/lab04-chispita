
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

#define velocidad 900 // velocidad/cooldown del movimiento de la nave
#define velocidadDisp 1300 // velocidad/cooldown del disparo de la nave

/*-----VARIABLES INICIALIZADAS-----*/
char *nave, *modo, *proy, buffer[100];
int x, y, celAlt, celAnch, maxY, maxX, ancho, alto, inicioX, inicioY, c, misil, ini, xProy, yProy, xProyIni;
long long ult, inicioDisp;
int volatile disparo, dibBanner;
pthread_mutex_t mutex;

/*SIN LÓGICA APLICADA*/
int combustible, oxigeno, naves, mutexio, semaforita, kernelio;
bool modoDisparo;
/*-------------------*/
/*---------------------------------*/

void *banner(void *param);
void *dibujarPantalla(void *param);
void *movimientoNave(void *arg);
void *proyectil(void *arg);
long long tiempo_actual_ms(); // nueva forma de calcular el tiempo en lugar de usar clock()

int main() {
    setlocale(LC_ALL, ""); // esto permite printear caracteres especiales, como la flecha de la nave
    initscr();
    noecho();
    curs_set(0);

    pthread_t hilo_mov, hilo_banner, hilo_pantalla, hilo_proyectil; // declaración de hilos
    pthread_mutex_init(&mutex, NULL); // declaración de mutex
    
    /*COLORES*/
    start_color();
    init_color(8, 200, 200, 200); // gris
    init_pair(1, 8, COLOR_BLACK);
    init_pair(2, COLOR_GREEN, COLOR_BLACK);
    /*-------*/
    
    /*ASIGNACIÓN DE VARIABLES*/
    ini = 0;
    ult = 0;
    nave = "↓";
    modo = "NAVE";
    proy = "@";
    x = corIniX;
    y = corIniY;
    celAlt = 2; // altura de cada celda
    celAnch = 4; // ancho de cada celda
    getmaxyx(stdscr, maxY, maxX);
    ancho = columnas * celAnch + 1;
    alto  = filas * celAlt + 1;
    inicioX = (maxX - ancho) / 2;
    inicioY = (maxY - alto) / 2;
    disparo=0;
    dibBanner=0;
    modoDisparo = false;
    misil = 8;
    /*-----------------------*/

    /*SIN LÓGICA APLICADA*/
    combustible = 100;
    oxigeno = 100;
    naves = 1;
    mutexio = 0;
    semaforita = 0;
    kernelio = 0;
    /*-------------------*/

    /*DISPARADOR DE HILOS*/
    pthread_create(&hilo_mov, NULL, movimientoNave, NULL);
    pthread_create(&hilo_pantalla,NULL,(void *)dibujarPantalla, NULL);
    pthread_create(&hilo_proyectil,NULL,(void *)proyectil, NULL);
    pthread_create(&hilo_banner,NULL,(void *)banner, NULL);
    /*-------------------*/

    pthread_join(hilo_mov, NULL);
    pthread_join(hilo_banner, NULL);
    pthread_join(hilo_pantalla, NULL);
    pthread_join(hilo_proyectil, NULL);
    
    endwin();
    return 0;
}   

/*HILO PARA ANIMACIÓN DEL BANNER*/
void *banner(void *param) {  
    char titulo[] = ">>>>>>COSMIKERNEL>>>>>>";
    int longitud = strlen(titulo);
    int i = 0;

    while(1){
        memset(buffer, 0, sizeof(buffer)); 
        for(int j=0; j<longitud; j++){
            buffer[j] = titulo[(i+j) % longitud];
        }
        dibBanner=1;
        i = (i + 1) % longitud; 
        usleep(200000); 
    }  
    
    pthread_exit(0);   
}

/*HILO PARA DIBUJAR LA INTERFAZ*/
void *dibujarPantalla(void *arg){   
    usleep(100000); // aveces al ejecutar el juego la interfaz se rompe, entonces hago que espere un momento
    while(1){
        pthread_mutex_lock(&mutex); 
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
        //mvprintw(0, 49, ">>>>>>COSMIKERNEL>>>>>>");
        mvprintw(26, 21, "MODO:%s", modo);
        mvprintw(2, 21, "COMB:%d%%", combustible);
        mvprintw(2, 57, "OXÍG:%d%%", oxigeno);
        mvprintw(2, 93, "NAVES:%d/5", naves);
        mvprintw(26, 95, "KERN:%d", kernelio);
        mvprintw(26, 83, "SEMA:%d", semaforita);
        mvprintw(26, 71, "MUTE:%d", mutexio);
        mvprintw(y, x, "%s", nave);

        if(modoDisparo){
            mvprintw(26, 34, "MISIL:%d", misil);
        }else{
        }
        
        /*DISPARO*/
        if(misil>=0){
            xProy=x;
            yProy=y;
            xProyIni=x;
            if(disparo==1){
                if(yProy-2<corIniY){ // max cordenada Y superior del proyectil
                    mvprintw(maxCorY, xProy, "%s", proy);
                }else{
                    mvprintw(yProy-2, xProy, "%s", proy);
                }
            }else if(disparo==2){
                if(yProy+2>maxCorY){ // max cordenada Y inferior del proyectil
                    mvprintw(corIniY, xProy, "%s", proy);
                }else{
                    mvprintw(yProy+2, xProy, "%s", proy);
                }
            }else if(disparo==3){
                if(xProy-4<corIniX){ // max cordenada X izquierda del proyectil
                    mvprintw(yProy, maxCorX, "%s", proy);
                }else{
                    mvprintw(yProy, xProy-4, "%s", proy);
                }
            }else if(disparo==4){
                if(xProy+4>maxCorX){ // max cordenada X derecha del proyectil
                    mvprintw(yProy, corIniX, "%s", proy);    
                }else{
                    mvprintw(yProy, xProy+4, "%s", proy);
                }
            }
        }
        // CUALQUIER NAVE/ASTEROIDE QUE ESTÉ EN LAS COORDENADAS DE PROY DEBERÍAN SER DAÑADAS
        /*-------*/

        if(dibBanner){
            mvprintw(0, 49, "%s", buffer);
        }

        attroff(COLOR_PAIR(2));
        refresh();
        pthread_mutex_unlock(&mutex);
        usleep(30000);
    }
}

/*HILO DE MOVIMIENTO*/
void *movimientoNave(void *arg){
    while(1){
        c = getch();
        pthread_mutex_lock(&mutex);

        /*SWITCH PARA DIBUJAR LA FLECHA SEGÚN LA TECLA*/
        switch(c) {         
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
        }
        
        /*SWITCH PARA ALTERNAR EN MODO NAVE Y MODO DISPARO*/
        switch(c){
            case 'e':
            modoDisparo = !modoDisparo;
                
            if(modoDisparo){
                modo= "DISP";
            }else{
                modo= "NAVE";
            }
            break;
        }
        
        /*MODO NAVE*/
        if(!modoDisparo){
            long long ahora = tiempo_actual_ms();
            if(!ini || (ahora-ult)>=velocidad){ 
                switch(c) {
                    case 'w': y=y-celAlt;
                    if(y<corIniY){
                        y=maxCorY;
                    }
                    break;
                
                    case 's': y=y+celAlt; 
                    if(y>maxCorY){
                        y=corIniY;
                    }
                    break;
                        
                    case 'a': x=x-celAnch; 
                    if(x<corIniX){
                        x=maxCorX;
                    }
                    break;
                    
                    case 'd': x=x+celAnch; 
                    if(x>maxCorX){
                        x=corIniX;
                    }
                    break;
                }
                ult=ahora;
                ini=1;
            }
        }

        /*MODO DISPARO*/
        if(modoDisparo){
            long long ahora = tiempo_actual_ms();
            if((ahora-ult)>=velocidadDisp){
                if(misil==0){
                    // si no tengo misiles no tengo la posibilidad de disparar
                }else{
                    switch(c){
                        case 'w': misil--;
                        disparo=1;
                        break;
                        
                        case 's': misil--;
                        disparo=2;
                        break;
                            
                        case 'a': misil--;
                        disparo=3;
                        break;
                        
                        case 'd': misil--;
                        disparo=4;
                        break;
                    }
                }
                inicioDisp = tiempo_actual_ms();
                ult=ahora;
            }
        }

        /*SALIR DEL JUEGO DE FORMA INMEDIATA*/
        switch(c){
            case 'p': 
            endwin();
            exit(0);
            break;
        }
        pthread_mutex_unlock(&mutex);
    }

}

/*HILO DEL PROYECTIL*/
void *proyectil(void *arg){
    while(1){
        pthread_mutex_lock(&mutex);
        if(disparo){
            long long ahora2 = tiempo_actual_ms();
            if((ahora2-inicioDisp)>=500){
                disparo=0;
            }
        }
        pthread_mutex_unlock(&mutex);
        usleep(30000);
    }
}

/*FUNCIÓN PARA CALCULAR EL TIEMPO DE COOLDOWN*/
long long tiempo_actual_ms() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);

    return (long long)ts.tv_sec * 1000LL +
           ts.tv_nsec / 1000000LL;
}
