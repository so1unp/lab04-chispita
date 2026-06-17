#include <ncurses.h>
#include <time.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <mqueue.h>
#include "compartido.h"

/**
 * ACLARACIONES IMPORTANTE;
 *
 * - Cuando colisiona la nave sin extraer recursos, no es que se le resten recursos al asteroide,
 * sino que cuando choco, se genera un asteroide nuevo con recursos nuevos.
 *
 * - Para extraer recursos del asteroide, en lo posible estar 5 casilleros cerca del asteroide,
 * puede ser por derecha, izquierda, arriba, abajo o diagonales pero 5 exactamente y de ahi apretas la tecla 'e' (minuscula).
 *
 * - Tengo entendido que cuando tengo los recursos que pesque de un asteroide, al colisionar con uno, estos no se me restan
 * por eso deje asi cuando choco con uno, que no se me reste el recurso que tengo.
 */

typedef struct
{
    int x;
    int y;
    int combustible;
    int oxigeno;
    int carga_deuterio;
    int carga_mutexio;
    int carga_semaforita;
    int carga_kernelio;
} Nave;

Nave mi_nave = {10, 10, 10000, 100, 0, 0, 0, 0};

/**
 * Variable para controlar el estado del juego
 */
int juego_activo = 1;

/**
 * Variables de extraccion minera
 */
int extraer = 0; // bandera para comunicar el teclado con el hilo minero

/**
 * El primer mutex sirve para proteger el acceso a las variables compartidas.
 * Y el segundo mutex es para proteger el acceso a la pantalla, para evitar que
 * los hilos se pisen entre ellos al escribir en la pantalla.
 */
pthread_mutex_t mutex_nave = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_pantalla = PTHREAD_MUTEX_INITIALIZER;

// Puntero al mapa de la memoria compartida
MapaEspacial *mapa_servidor; // Puntero al mapa en memoria compartida

/**
 * Hilo de soporte vital: es el que se encargara de decrementar oxigeno de manera periodica, es decir,
 * por un intervalo de tiempo determinado, en este caso cada 5 segundos, y si el oxigeno llega a 0,
 * se termina el juego, porque la nave se quedo sin oxigeno y no puede seguir funcionando.
 * Funciona de tal forma que si el oxigeno es mayor a 0, se resta 1 unidad y si el oxigeno es menor o igual a 0,
 * avisamos con la flag de juego_activo que el juego ha terminado.
 * */
void *hilo_soporte_vital(void *arg)
{
    while (juego_activo)
    {
        sleep(5);
        pthread_mutex_lock(&mutex_nave);

        if (mi_nave.oxigeno > 0)
        {
            mi_nave.oxigeno--;
        }
        if (mi_nave.oxigeno <= 0)
        {
            juego_activo = 0;
        }

        pthread_mutex_unlock(&mutex_nave);
    }
    return NULL;
}

/**
 * Hilo de extraccion: extraccion minera en el cual la nave va a estraer minerales del asteroide.
 * La extraccion se realiza con la tecla 'e'. solo si la nave esta a 5 casilleros de distancia ya sea
 * por izquierda, derecha, arriba o abajo del asteriode, si el asteroiode tiene recursos disponibles,
 * la nave tiene recursos.
 * Ahora se agrego una nueva funcionalidad, de tal manera que si el asteroide tiene recursos disponibles en la tercer condicion
 * del if(), se va a a extraer el recurso correspondiente y se agrega a la carga de la nave y se resta 2 unidades
 * de combustible obviamente. Le restamos recurso al asteroide y nos cargamos a nosotros (nave).
 */
void *hilo_extraccion(void *arg)
{
    while (juego_activo)
    {

        if (extraer == 1)
        {
            pthread_mutex_lock(&mutex_nave);

            for (int i = 0; i < MAX_ASTEROIDES_FISICOS; i++)
            {
                if (mapa_servidor->asteroides[i].activo == 1)
                {
                    int dist_x = abs(mapa_servidor->asteroides[i].x - mi_nave.x);
                    int dist_y = abs(mapa_servidor->asteroides[i].y - mi_nave.y);

                    if (dist_x <= 5 && dist_y <= 5)
                    {
                        if (mapa_servidor->asteroides[i].deuterio > 0 && mi_nave.combustible >= 2)
                        {
                            mapa_servidor->asteroides[i].deuterio--;
                            mi_nave.carga_deuterio++;
                            mi_nave.combustible -= 2;
                        }
                        if (mapa_servidor->asteroides[i].mutexio > 0 && mi_nave.combustible >= 2)
                        {
                            mapa_servidor->asteroides[i].mutexio--;
                            mi_nave.carga_mutexio++;
                            mi_nave.combustible -= 2;
                        }
                        if (mapa_servidor->asteroides[i].semaforita > 0 && mi_nave.combustible >= 2)
                        {
                            mapa_servidor->asteroides[i].semaforita--;
                            mi_nave.carga_semaforita++;
                            mi_nave.combustible -= 2;
                        }
                        if (mapa_servidor->asteroides[i].kernelio > 0 && mi_nave.combustible >= 2)
                        {
                            mapa_servidor->asteroides[i].kernelio--;
                            mi_nave.carga_kernelio++;
                            mi_nave.combustible -= 2;
                        }
                        break;
                    }
                }
            }

            extraer = 0;
            pthread_mutex_unlock(&mutex_nave);
        }
        usleep(20000);
    }
    return NULL;
}

/**
 * Hilo de propulsion: es el que se encargara de decrementar combustible cada vez que hay movimiento mediante teclado
 * o porque choque contra un asteroide.
 */
void *hilo_propulsion(void *arg)
{
    WINDOW *ventana = (WINDOW *)arg;
    int tecla;

    while (juego_activo)
    {
        pthread_mutex_lock(&mutex_pantalla);
        tecla = wgetch(ventana);
        pthread_mutex_unlock(&mutex_pantalla);

        if (tecla == 'q')
        {
            juego_activo = 0;
            break;
        }

        if (tecla == 'e')
        {
            extraer = 1;
        }

        // ISSUE 2: MECANICAS PARA EL COMERCIO
        if (tecla == 'v')
        {
            
            mqd_t buz_ventas = mq_open(NOMBRE_COLA_VENTAS, O_WRONLY);

            pthread_mutex_lock(&mutex_pantalla);
            WINDOW *menu_ypf = newwin(20, 60, 5, 20);
            box(menu_ypf, 0, 0);

            mvwprintw(menu_ypf, 2, 15, "ESTACION ESPACIAL YPF");
            mvwprintw(menu_ypf, 5, 5, "[1] Cambiar 5 Deuterio por 10 Combustible");
            mvwprintw(menu_ypf, 6, 5, "[2] Cambiar 1 de cada mineral por 10 Oxigeno");
            mvwprintw(menu_ypf, 8, 5, "[q] Desconectar y salir");

            if (buz_ventas == (mqd_t)-1)
            {
                mvwprintw(menu_ypf, 18, 2, "Aviso: La estacion no abrio su buzon todavia.");
            }
        
            wtimeout(menu_ypf, 50);
            pthread_mutex_unlock(&mutex_pantalla);

            int en_menu = 1;
            while (en_menu)
            {
                pthread_mutex_lock(&mutex_pantalla);
                touchwin(menu_ypf);
                wrefresh(menu_ypf);
                int opcion = wgetch(menu_ypf);
                pthread_mutex_unlock(&mutex_pantalla);

                if (opcion == '1')
                {
                    pthread_mutex_lock(&mutex_nave);
                    if (mi_nave.carga_deuterio >= 5)
                    {
                        mi_nave.carga_deuterio -= 5;
                        mi_nave.combustible += 10;

                        if (buz_ventas != (mqd_t)-1)
                        {
                            MensajeVenta msj;
                            msj.id_nave = 1;
                            msj.tipo_operacion = 1;
                            msj.carga_deuterio = 5;
                            msj.carga_mutexio = 0;
                            msj.carga_semaforita = 0;
                            msj.carga_kernelio = 0;
                            mq_send(buz_ventas, (const char *)&msj, sizeof(MensajeVenta), 0);
                        }

                        pthread_mutex_lock(&mutex_pantalla);
                        mvwprintw(menu_ypf, 10, 5, "Trueque exitoso: +10 Combustible!           ");
                        pthread_mutex_unlock(&mutex_pantalla);
                    }
                    else
                    {
                        pthread_mutex_lock(&mutex_pantalla);
                        mvwprintw(menu_ypf, 10, 5, "No tienes suficiente Deuterio.              ");
                        pthread_mutex_unlock(&mutex_pantalla);
                    }
                    pthread_mutex_unlock(&mutex_nave);
                }
                else if (opcion == '2')
                {
                    pthread_mutex_lock(&mutex_nave);
                    if (mi_nave.carga_mutexio >= 1 &&
                        mi_nave.carga_semaforita >= 1 &&
                        mi_nave.carga_kernelio >= 1)
                    {

                        mi_nave.carga_mutexio--;
                        mi_nave.carga_semaforita--;
                        mi_nave.carga_kernelio--;
                        mi_nave.oxigeno += 10;

                        if (buz_ventas != (mqd_t)-1)
                        {
                            MensajeVenta msj;
                            msj.id_nave = 1;
                            msj.tipo_operacion = 2;
                            msj.carga_deuterio = 0;
                            msj.carga_mutexio = 1;
                            msj.carga_semaforita = 1;
                            msj.carga_kernelio = 1;
                            mq_send(buz_ventas, (const char *)&msj, sizeof(MensajeVenta), 0);
                        }

                        pthread_mutex_lock(&mutex_pantalla);
                        mvwprintw(menu_ypf, 10, 5, "Trueque exitoso: +10 Oxigeno!               ");
                        pthread_mutex_unlock(&mutex_pantalla);
                    }
                    else
                    {
                        pthread_mutex_lock(&mutex_pantalla);
                        mvwprintw(menu_ypf, 10, 5, "No tienes suficientes recursos.             ");
                        pthread_mutex_unlock(&mutex_pantalla);
                    }
                    pthread_mutex_unlock(&mutex_nave);
                }
                else if (opcion == 'q')
                {
                    en_menu = 0;
                }
            }

            if (buz_ventas != (mqd_t)-1)
            {
                mq_close(buz_ventas);
            }

            pthread_mutex_lock(&mutex_pantalla);
            werase(menu_ypf);
            wrefresh(menu_ypf);
            delwin(menu_ypf);
            pthread_mutex_unlock(&mutex_pantalla);
        }

        if (tecla == 'w' || tecla == KEY_UP ||
            tecla == 's' || tecla == KEY_DOWN ||
            tecla == 'a' || tecla == KEY_LEFT ||
            tecla == 'd' || tecla == KEY_RIGHT)
        {

            pthread_mutex_lock(&mutex_nave);
            if (mi_nave.combustible > 0)
            {
                if ((tecla == 'w' || tecla == KEY_UP) && mi_nave.y > 1)
                {
                    mi_nave.y--;
                    mi_nave.combustible--;
                }
                if ((tecla == 's' || tecla == KEY_DOWN) && mi_nave.y < 38)
                {
                    mi_nave.y++;
                    mi_nave.combustible--;
                }
                if ((tecla == 'a' || tecla == KEY_LEFT) && mi_nave.x > 1)
                {
                    mi_nave.x--;
                    mi_nave.combustible--;
                }
                if ((tecla == 'd' || tecla == KEY_RIGHT) && mi_nave.x < 95)
                {
                    mi_nave.x++;
                    mi_nave.combustible--;
                }
            }
            if (mi_nave.combustible <= 0)
            {
                juego_activo = 0;
            }
            pthread_mutex_unlock(&mutex_nave);
        }
        usleep(10000);
    }
    return NULL;
}

/**
 * Programa principal.
 */
int main()
{

    WINDOW *ventana;
    WINDOW *panel;
    time_t t;
    struct tm *fecha;

    initscr();
    noecho();
    cbreak();
    curs_set(0);

    ventana = newwin(40, 100, 1, 1);
    wtimeout(ventana, 50);
    keypad(ventana, TRUE);
    panel = newwin(16, 45, 1, 105);

    box(ventana, '|', '=');
    box(panel, '|', '-');

    // Conexion a la memoria compartida POSIX
    int shm_fd = shm_open("/mapa_espacial", O_RDWR, 0666);
    if (shm_fd == -1)
    {
        perror("Error critico: El servidor no esta corriendo o la memoria no existe");
        endwin();
        exit(EXIT_FAILURE);
    }

    mapa_servidor = mmap(NULL, sizeof(MapaEspacial), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (mapa_servidor == MAP_FAILED)
    {
        perror("Error al mapear la memoria compartida");
        endwin();
        exit(EXIT_FAILURE);
    }

    pthread_t thread_vital, thread_propulsion, thread_extraccion;
    pthread_create(&thread_vital, NULL, hilo_soporte_vital, NULL);
    pthread_create(&thread_extraccion, NULL, hilo_extraccion, NULL);
    pthread_create(&thread_propulsion, NULL, hilo_propulsion, (void *)ventana);

    while (juego_activo)
    {
        pthread_mutex_lock(&mutex_pantalla);

        // Dibujar mapa base
        for (int i = 1; i < FILAS - 1; i++)
        {
            for (int j = 1; j < COLUMNAS - 1; j++)
            {
                mvwaddch(ventana, i, j, mapa_servidor->matriz[i][j]);
            }
        }

        // Dibujar Nave
        mvwprintw(ventana, mi_nave.y, mi_nave.x, "A");

        time(&t);
        fecha = localtime(&t);

        pthread_mutex_lock(&mutex_nave);
        mvwprintw(panel, 1, 1, "ESTADO NAVE:");
        mvwprintw(panel, 2, 1, "OXIGENO:     %-3d", mi_nave.oxigeno);
        mvwprintw(panel, 3, 1, "COMBUSTIBLE: %-3d", mi_nave.combustible);
        mvwprintw(panel, 5, 1, "TU BODEGA:");
        mvwprintw(panel, 6, 1, "Deuterio:   %-3d", mi_nave.carga_deuterio);
        mvwprintw(panel, 7, 1, "Mutexio:    %-3d", mi_nave.carga_mutexio);
        mvwprintw(panel, 8, 1, "Semaforita: %-3d", mi_nave.carga_semaforita);
        mvwprintw(panel, 9, 1, "Kernelio:   %-3d", mi_nave.carga_kernelio);

        mvwprintw(panel, 14, 1, "%02d:%02d:%02d", fecha->tm_hour, fecha->tm_min, fecha->tm_sec);
        pthread_mutex_unlock(&mutex_nave);

        wrefresh(ventana);
        wrefresh(panel);
        pthread_mutex_unlock(&mutex_pantalla);

        usleep(50000);
    }

    mvwprintw(ventana, FILAS / 2, (COLUMNAS / 2) - 5, "GAME OVER :(((");
    wrefresh(ventana);
    wtimeout(ventana, -1);
    wgetch(ventana);

    pthread_cancel(thread_vital);
    pthread_cancel(thread_extraccion);
    pthread_cancel(thread_propulsion);
    pthread_join(thread_propulsion, NULL);

    endwin();
    return 0;
}