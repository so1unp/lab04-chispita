
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

#define velocidad 800 // velocidad/cooldown del movimiento de la nave

/*-----VARIABLES INICIALIZADAS-----*/
char *nave, *modo;
int x, y, celAlt, celAnch, maxY, maxX, ancho, alto, inicioX, inicioY, c, ini, misil;
long long ult;
pthread_mutex_t mutex;

/*SIN LÓGICA APLICADA*/
int combustible, oxigeno, naves, mutexio, semaforita, kernelio;
bool modoDisparo;
/*-------------------*/
/*---------------------------------*/

void *banner(void *param);
void *dibujarPantalla(void *param);
void *movimientoNave(void *arg);
//void *proyectil(void *arg);
long long tiempo_actual_ms(); // nueva forma de calcular el tiempo en lugar de usar clock()

int main() {
    setlocale(LC_ALL, "");
    initscr();
    noecho();
    cbreak();
    curs_set(0);
    //nodelay(stdscr, TRUE); // NO USAR JAMÁS, CONSUME MUCHA CPU JUNTO CON getch()

    pthread_t hilo_mov, hilo_banner, hilo_pantalla; // declaración de hilos
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
    x = corIniX;
    y = corIniY;
    celAlt = 2; // altura de cada celda
    celAnch = 4; // ancho de cada celda
    getmaxyx(stdscr, maxY, maxX);
    ancho = columnas * celAnch + 1;
    alto  = filas * celAlt + 1;
    inicioX = (maxX - ancho) / 2;
    inicioY = (maxY - alto) / 2;
    /*-----------------------*/

    /*SIN LÓGICA APLICADA*/
    combustible = 100;
    oxigeno = 100;
    naves = 1;
    mutexio = 0;
    semaforita = 0;
    kernelio = 0;
    modoDisparo = false;
    misil = 10;
    /*-------------------*/

    /*DISPARADOR DE HILOS*/
    pthread_create(&hilo_mov, NULL, movimientoNave, NULL);
    pthread_create(&hilo_pantalla,NULL,(void *)dibujarPantalla, NULL);
    //pthread_create(&hilo_banner,NULL,(void *)banner, NULL);
    /*-------------------*/

    pthread_join(hilo_mov, NULL);
    //pthread_join(hilo_banner, NULL);
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
        refresh();
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
        mvprintw(0, 49, ">>>>>>COSMIKERNEL>>>>>>");
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
            if(c!=ERR){
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
        }

        /*MODO DISPARO*/
        if(modoDisparo){
            if(c!=ERR){
                long long ahora = tiempo_actual_ms();
                if((ahora-ult)>=velocidad){
                    switch(c){

                        case 'w': misil--;
                        break;
                            
                        case 's': misil--;
                        break;
                            
                        case 'a': misil--;
                        break;
                            
                        case 'd': misil--;
                        break;
                    }
                    ult=ahora;
                    ini=1;
                    if(misil<0){
                        misil=0;
                    }
                }
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

/*FUNCIÓN PARA CALCULAR EL TIEMPO DE COOLDOWN*/
long long tiempo_actual_ms() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);

    return (long long)ts.tv_sec * 1000LL +
           ts.tv_nsec / 1000000LL;
}