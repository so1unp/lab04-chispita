/*
    COMANDO PARA COMPILAR:
    gcc nave.c -lncursesw -lrt -lpthread -o nave
*/ 

#include <ncurses.h>
#include <time.h>
#include <locale.h>
#include <stdbool.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <mqueue.h>
#include "compartido.h"

#define corIniX 5 
#define corIniY 5 

#define maxCorX 53 
#define maxCorY 25 

#define columnas 13
#define filas 11

#define movCooldown 900 
#define dispCooldown 1000 
#define compraCooldown 1000 

/*-----VARIABLES INICIALIZADAS-----*/
MapaEspacial *mapa;
char *nave, *modo, *proy, buffer[100];
int mi_id, x, y, celAlt, celAnch, maxY, maxX, ancho, alto, inicioX, inicioY, c, misil, ini, xProy, yProy, xProyIni, gastar, combustible, oxigeno, mutexio, semaforita, kernelio, deuterio;
long long ult, ultCompra, inicioDisp;
int volatile disparo, dibBanner, daño;
pthread_mutex_t mutex;
bool modoDisparo;
/*---------------------------------*/

/*-----HILOS Y FUNCIONES-----*/
void *banner(void *param);
void *dibujarPantalla(void *param);
void *movimientoNave(void *arg);
void *proyectil(void *arg);
void *oxigenoH(void* arg);
int posOcupada(int x, int y);
long long tiempoActual(); 
void enviarMensajeEstacion(int operacion, int id_estacion);
int colisionAsteroide(int x, int y);
/*---------------------------*/

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Uso: %s <id_nave>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    setlocale(LC_ALL, ""); 
    initscr();
    noecho();
    curs_set(0);

    /*HILOS Y MUTEX*/
    pthread_t hilo_mov, hilo_banner, hilo_pantalla, hilo_proyectil, hilo_oxigeno; 
    pthread_mutex_init(&mutex, NULL); 
    /*-------------*/

    /*MEMORIA COMPARTIDA*/
    int fd = shm_open("/mapa_espacial", O_RDWR, 0666);
    if (fd == -1) {
        perror("Error al abrir memoria compartida");
        exit(EXIT_FAILURE);
    }

    mapa = mmap(NULL, sizeof(MapaEspacial), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (mapa == MAP_FAILED) {
        perror("Error al mapear memoria");
        exit(EXIT_FAILURE);
    }
    
    mi_id = atoi(argv[1]);
    
    /*Validacion de naves permitidas*/
    if (mi_id < 0 || mi_id >= mapa->cantidad_naves_permitidas) {
        printf("ERROR: ID de nave %d no permitido.\n", mi_id);
        printf("El servidor configuró un límite máximo de %d naves (IDs válidos: 0 a %d).\n", 
               mapa->cantidad_naves_permitidas, mapa->cantidad_naves_permitidas - 1);
        
        munmap(mapa, sizeof(MapaEspacial));
        close(fd);
        exit(EXIT_FAILURE);
    }

    if (mapa->naves[mi_id].activa == 1) {
        printf("ERROR: La nave con ID %d ya está activa y siendo operada por otro jugador.\n", mi_id);
        
        munmap(mapa, sizeof(MapaEspacial));
        close(fd);
        exit(EXIT_FAILURE);
    }

    /*Spawn separado para cada nave*/
    mapa->naves[mi_id].activa = 1;
    mapa->naves[mi_id].x = corIniX + (mi_id * 5);
    mapa->naves[mi_id].y = corIniY;
    
    /*COLORES*/
    start_color();
    init_color(8, 200, 200, 200); 
    init_pair(1, 8, COLOR_BLACK);
    init_pair(2, COLOR_GREEN, COLOR_BLACK);
    /*-------*/
    
    /*ASIGNACIÓN DE VARIABLES*/
    ini = 0;
    ult = 0;
    nave = "↓";
    modo = "NAVE";
    proy = "@";
    x = corIniX + (mi_id * 5);
    y = corIniY;
    celAlt = 2; 
    celAnch = 4; 
    getmaxyx(stdscr, maxY, maxX);
    ancho = columnas * celAnch + 1;
    alto  = filas * celAlt + 1;
    inicioX = (maxX - ancho) / 2;
    inicioY = (maxY - alto) / 2 + 1;
    disparo = 0;
    dibBanner = 0;
    modoDisparo = false;
    misil = 999;
    daño = 0;
    gastar = 0;
    ultCompra = 0;
    /*-----------------------*/

    /*VARIABLES A IMPLEMENTAR EN LA MEMORIA COMPARTIDA*/
    combustible = 100;
    oxigeno = 100;
    mutexio = 0;
    semaforita = 0;
    kernelio = 0;
    deuterio = 0;
    /*------------------------------------------------*/

    /*ALMACENO LOS VALORES INICIALES A LA MEMORIA PRINCIPAL*/
    mapa->naves[mi_id].combustible = combustible;
    mapa->naves[mi_id].oxigeno = oxigeno;
    mapa->naves[mi_id].mutexio = mutexio;
    mapa->naves[mi_id].semaforita = semaforita;
    mapa->naves[mi_id].kernelio = kernelio;
    mapa->naves[mi_id].deuterio = deuterio;
    strcpy(mapa->naves[mi_id].modo, modo); // necesito guardar esto para que la memoria compartida lea el cambio de modo
    /*-----------------------------------------------------*/

    /*DISPARADOR DE HILOS*/
    pthread_create(&hilo_mov, NULL, movimientoNave, NULL);
    pthread_create(&hilo_pantalla, NULL, (void *)dibujarPantalla, NULL);
    pthread_create(&hilo_proyectil, NULL, (void *)proyectil, NULL);
    pthread_create(&hilo_banner, NULL, (void *)banner, NULL);
    pthread_create(&hilo_oxigeno, NULL, (void *)oxigenoH, NULL);
    /*-------------------*/

    pthread_join(hilo_mov, NULL);
    pthread_join(hilo_banner, NULL);
    pthread_join(hilo_pantalla, NULL);
    pthread_join(hilo_proyectil, NULL);
    pthread_join(hilo_oxigeno, NULL);

    mapa->naves[mi_id].activa = 0;

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
        for(int j = 0; j < longitud; j++){
            buffer[j] = titulo[(i + j) % longitud];
        }
        dibBanner = 1;
        i = (i + 1) % longitud; 
        usleep(200000); 
    }  
    pthread_exit(0);   
}

/*HILO PARA DIBUJAR LA INTERFAZ*/
void *dibujarPantalla(void *arg){   
    usleep(300000); 
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
        combustible = mapa->naves[mi_id].combustible;
        oxigeno = mapa->naves[mi_id].oxigeno;
        mutexio = mapa->naves[mi_id].mutexio;
        semaforita = mapa->naves[mi_id].semaforita;
        kernelio = mapa->naves[mi_id].kernelio;
        deuterio = mapa->naves[mi_id].deuterio;

        mvprintw(27, 5, "MODO:%s", modo);
        mvprintw(3, 46, "COMB:%d%%", combustible);
        mvprintw(3, 5, "OXÍG:%d%%", oxigeno);
        mvprintw(27, 47, "KERN:%d", kernelio);
        mvprintw(27, 37, "SEMA:%d", semaforita);
        mvprintw(27, 27, "MUTE:%d", mutexio);
        mvprintw(27, 17, "DEUT:%d", deuterio);
        mvprintw(y, x, "%s", nave);

        /*-------------*/
        for (int i = 0; i < MAX_NAVES; i++) {
            if (i != mi_id && mapa->naves[i].activa) {
                mvprintw(mapa->naves[i].y, mapa->naves[i].x, "▲");
            }
        }
        /*-------------*/

        if(modoDisparo){
            mvprintw(28, 5, "MISIL:INF");
        }
        
        /*DISPARO*/
        if(misil >= 0){
            xProy = x;
            yProy = y;
            xProyIni = x;
            if(disparo == 1){
                if(yProy-2 < corIniY) mvprintw(maxCorY, xProy, "%s", proy);
                else mvprintw(yProy-2, xProy, "%s", proy);
            }else if(disparo == 2){
                if(yProy+2 > maxCorY) mvprintw(corIniY, xProy, "%s", proy);
                else mvprintw(yProy+2, xProy, "%s", proy);
            }else if(disparo == 3){
                if(xProy-4 < corIniX) mvprintw(yProy, maxCorX, "%s", proy);
                else mvprintw(yProy, xProy-4, "%s", proy);
            }else if(disparo == 4){
                if(xProy+4 > maxCorX) mvprintw(yProy, corIniX, "%s", proy);    
                else mvprintw(yProy, xProy+4, "%s", proy);
            }
            if(daño) proy = "🟌";
            else proy = "@";
        }

        if(dibBanner){
            mvprintw(1, 18, "%s", buffer);
        }

        /* if (mapa->estacion.activa && x == mapa->estacion.x && y == mapa->estacion.y) {
            mvprintw(29, 4, "[1 -> Recargar Combustible] [2 -> Recargar Oxigeno]");
        }  */


        // SE MODIFICÓ DE ESTACION A ESTACIONES (ARREGLO)
        for(int i = 0; i < MAX_ESTACIONES; i++){
            if (mapa->estaciones[i].activa && x == mapa->estaciones[i].x && y == mapa->estaciones[i].y) {
                mvprintw(29, 4, "[1 -> Recargar Combustible] [2 -> Recargar Oxigeno]");
            } 
        }

        attroff(COLOR_PAIR(2));

        for (int i = 0; i < MAX_ASTEROIDES_FISICOS; i++) {
            if (mapa->asteroides[i].activo) {
                mvprintw(mapa->asteroides[i].y, mapa->asteroides[i].x, "●");
            }
        }

        /* if (mapa->estacion.activa) {
            mvprintw(mapa->estacion.y, mapa->estacion.x, "E");
        } */


        // SE MODIFICÓ DE ESTACION A ESTACIONES (ARREGLO)
        for(int i = 0; i < MAX_ESTACIONES; i++){
            if (mapa->estaciones[i].activa) {
                mvprintw(mapa->estaciones[i].y, mapa->estaciones[i].x, "E");
            }
        }

        refresh();
        pthread_mutex_unlock(&mutex);
        usleep(30000);
    }
}

/*HILO DE MOVIMIENTO Y ACCIONES DE TECLADO*/
void *movimientoNave(void *arg){
    while(1){
        c = getch();
        pthread_mutex_lock(&mutex);
        
        int xNuevo = x;
        int yNuevo = y;

        /*SWITCH PARA CAPTURAR INTERCAMBIOS CON LA ESTACIÓN YPF*/
        long long ahoraCompra = tiempoActual();
        /* if((ahoraCompra - ultCompra) >= compraCooldown){ 
            if (mapa->estacion.activa && x == mapa->estacion.x && y == mapa->estacion.y) {
                switch(c) {
                    case '1':
                        enviarMensajeEstacion(1);
                        ultCompra = ahoraCompra;
                        break;

                    case '2':
                        enviarMensajeEstacion(2);
                        ultCompra = ahoraCompra;
                        break;
                }
            }
        } */

        // Dentro de movimientoNave, cuando apretás 1 o 2:
        int estacion_actual = -1;

        // Revisamos en cuál de todas las estaciones estamos parados
        for(int i = 0; i < mapa->cantidad_estaciones; i++) {
            if (mapa->estaciones[i].activa && x == mapa->estaciones[i].x && y == mapa->estaciones[i].y) {
                estacion_actual = i;
                break;
            }
        }

        // Si efectivamente estamos sobre una estación (estacion_actual ya no es -1)
        if (estacion_actual != -1) {
            if((ahoraCompra - ultCompra) >= compraCooldown){ 
                switch(c) {
                    case '1':
                        enviarMensajeEstacion(1, estacion_actual); // Le pasamos el ID!
                        ultCompra = ahoraCompra;
                        break;
                    case '2':
                        enviarMensajeEstacion(2, estacion_actual);
                        ultCompra = ahoraCompra;
                        break;
                }
            }
        }

        /*SWITCH PARA DIBUJAR LA FLECHA SEGÚN LA TECLA*/
        switch(c) {         
            case 'w': 
                nave = "↑"; 
                break;

            case 's': 
                nave = "↓"; 
                break;

            case 'a': 
                nave = "←"; 
                break;

            case 'd': 
                nave = "→"; 
                break;
        }
        
        /*SWITCH PARA ALTERNAR EN MODO NAVE Y MODO DISPARO*/
        switch(c){
            case 'e':
                modoDisparo = !modoDisparo;
                if(modoDisparo){
                    modo = "DISP";
                }else{
                    modo = "NAVE";
                }
                    
                strcpy(mapa->naves[mi_id].modo, modo);
                break;
        }
        
        /*MODO NAVE*/
        if(!modoDisparo){
            long long ahora = tiempoActual();
            if((ahora - ult) >= movCooldown){ 

                gastar = 0;

                if(combustible==0){

                }else{
                    switch (c) {
                        case 'w':
                            yNuevo -= celAlt;
                            if (yNuevo < corIniY) yNuevo = maxCorY;
                            break;
                        case 's':
                            yNuevo += celAlt;
                            if (yNuevo > maxCorY) yNuevo = corIniY;
                            break;
                        case 'a':
                            xNuevo -= celAnch;
                            if (xNuevo < corIniX) xNuevo = maxCorX;
                            break;
                        case 'd':
                            xNuevo += celAnch;
                            if (xNuevo > maxCorX) xNuevo = corIniX;
                            break;
                        default:
                        gastar = 1;
                        break;
                    }
    
                    int movio = 0;
    
                    if(!posOcupada(xNuevo, yNuevo) && !colisionAsteroide(xNuevo, yNuevo)){
                        x = xNuevo;
                        y = yNuevo;
                        movio = 1;
                    }
    
                    if(movio){
                        ult = ahora;
    
                        if(gastar != 1 && mapa->naves[mi_id].combustible > 0){
                            mapa->naves[mi_id].combustible--;
                        }
                    }
                }
            }
        }

        /*MODO DISPARO*/
        if(modoDisparo){
            long long ahora = tiempoActual();
            if((ahora - ult) >= dispCooldown){
                if(misil > 0){
                    switch(c){
                        case 'w': 
                            misil--; 
                            disparo = 1; 
                            break;

                        case 's': 
                            misil--; 
                            disparo = 2; 
                            break;

                        case 'a': 
                            misil--; 
                            disparo = 3; 
                            break;

                        case 'd': 
                            misil--; 
                            disparo = 4; 
                            break;
                    }

                    mapa->naves[mi_id].misil = misil;
                    mapa->naves[mi_id].disparo = disparo;
                }
                inicioDisp = tiempoActual();
                ult = ahora;
            }
        }

        /*SALIR DEL JUEGO DE FORMA INMEDIATA*/
        if(c == 'p'){ 
            mapa->naves[mi_id].activa = 0;
            endwin();
            exit(0);
        }

        mapa->naves[mi_id].x = x;
        mapa->naves[mi_id].y = y;
        mapa->naves[mi_id].xProy = x;
        mapa->naves[mi_id].yProy = y;
        mapa->naves[mi_id].modoDisparo = modoDisparo;

        pthread_mutex_unlock(&mutex);
    }
}

/*HILO DEL PROYECTIL*/
void *proyectil(void *arg){
    while(1){
        pthread_mutex_lock(&mutex);
        if(disparo){
            long long ahora2 = tiempoActual();
            if((ahora2 - inicioDisp) >= 400){
                daño = 1;
                if((ahora2 - inicioDisp) >= 800){
                    disparo = 0;
                    daño = 0;
                    mapa->naves[mi_id].disparo = 0;
                }
            }
        }
        pthread_mutex_unlock(&mutex);
        usleep(30000);
    }
}

/*HILO DE CONSUMO DE OXÍGENO*/
void *oxigenoH(void *arg) {
    while (1) {
        sleep(4);
        pthread_mutex_lock(&mutex);

        if (mapa->naves[mi_id].oxigeno > 0) {
            mapa->naves[mi_id].oxigeno--;
        } else {
            mapa->naves[mi_id].activa = 0;
            endwin();
            exit(EXIT_SUCCESS);
        }

        pthread_mutex_unlock(&mutex);
    }
    return NULL;
}

/*FUNCIÓN PARA CALCULAR EL TIEMPO*/
long long tiempoActual() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (long long)ts.tv_sec * 1000LL + ts.tv_nsec / 1000000LL;
}

/*FUNCIÓN PARA CALCULAR LA COLISIÓN DE DOS NAVES*/
int posOcupada(int x, int y) {
    for (int i = 0; i < MAX_NAVES; i++) {
        if (i != mi_id && mapa->naves[i].activa && mapa->naves[i].x == x && mapa->naves[i].y == y) {
            return 1;
        }
    }
    return 0;
}

/*FUNCIÓN PARA CALCULAR LA COLISIÓN DE LOS ASTEROIDES*/
int colisionAsteroide(int x, int y) {
    for (int i = 0; i < MAX_ASTEROIDES_FISICOS; i++) {
        if (mapa->asteroides[i].activo &&
            mapa->asteroides[i].x == x &&
            mapa->asteroides[i].y == y) {
            return 1;
        }
    }
    return 0;
}   

/*FUNCIÓN PARA ENVIAR MENSAJES A LA COLA DE ESTACIÓN*/
void enviarMensajeEstacion(int operacion, int id_estacion) {
    char nombre_cola[50];
    sprintf(nombre_cola, "/cola_estacion_%d", id_estacion);
    mqd_t cola = mq_open(nombre_cola, O_WRONLY | O_NONBLOCK);
    if (cola == (mqd_t)-1) {
        return; 
    }

    MensajeNave msj;
    msj.tipo_operacion = operacion;
    msj.id_nave = mi_id;

    mq_send(cola, (const char *)&msj, sizeof(MensajeNave), 0);
    mq_close(cola);
}