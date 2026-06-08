#include <ncurses.h>
#include <time.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include "compartido.h" // El archivo de contrato entre procesos
#include <sys/mman.h>   // Para mmap() y shm_open()
#include <fcntl.h>      // Para las constantes O_RDWR


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


 /**
  * NUEVAS ACLARACIONES 6/6
  * - Incorpore la libreria <sys/mman.h > y <fcntl.h> para poder usar las funciones de memoria compartida POSIX. Enlaze  el codigo
  * 
  * -  Nave mi_nave es una de prueba
  * 
  * - En el hilo de extraccion, se programo un scanner mediante un bucle for que revisa el array de asteroides en el servidor
  * 
  * -Implementamos la lógica matemática con abs() para verificar que estés a un máximo de 5 casilleros de distancia de un objetivo válido.
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

// Creamos nuestra nave real usando el molde
Nave mi_nave = {10, 10, 10000, 100, 0, 0, 0, 0};

/**
 * Variable para controlar el estado del juego
 */
int juego_activo = 1;

/**
 * Recursos del asteroide estr ESTO LO ELIMINAMOS YA QUE ESTA EN UN STRUCT EN estacion.c ENE
 */

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


MapaEspacial *mapa_servidor; // Puntero global al mapa compartido, para que lo puedan usar todos los hilos

/**
 * La funcion de generar_recursos_aleatorio() se borro ya que estan en la funcion generar_asteroides() por lo que veo
 */

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
        pthread_mutex_lock(&mutex_nave); // Bloqueamos el mutex para modificar el estado de la nave de forma segura

        if (mi_nave.oxigeno > 0)
        {
            mi_nave.oxigeno--;
        }

        if (mi_nave.oxigeno <= 0)
        {
            juego_activo = 0;
        }

        pthread_mutex_unlock(&mutex_nave); // Liberamos el mutex para que otros hilos puedan acceder al estado de la nave
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
                        // restar recursos al al asteroide usando el indice i
                        if (mapa_servidor->asteroides[i].deuterio > 0)
                        {
                            mapa_servidor->asteroides[i].deuterio--;
                            mi_nave.carga_deuterio++;
                            mi_nave.combustible -= 2;
                        }
                        if (mapa_servidor->asteroides[i].mutexio > 0)
                        {
                            mapa_servidor->asteroides[i].mutexio--;
                            mi_nave.carga_mutexio++;
                            mi_nave.combustible -= 2;
                        }
                        if (mapa_servidor->asteroides[i].semaforita > 0)
                        {
                            mapa_servidor->asteroides[i].semaforita--;
                            mi_nave.carga_semaforita++;
                            mi_nave.combustible -= 2;
                        }
                        if (mapa_servidor->asteroides[i].kernelio > 0)
                        {
                            mapa_servidor->asteroides[i].kernelio--;
                            mi_nave.carga_kernelio++;
                            mi_nave.combustible -= 2;
                        }
                        break;
                    }
                }
            }

            extraer = 0; // Reiniciamos la bandera
            pthread_mutex_unlock(&mutex_nave);
        }
        usleep(20000); // Pequeña pausa para no consumir 100% de CPU
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
            juego_activo = 0; // Avisa al resto que perdimos
            break;
        }

        // AGREGAMOS NUEVA FUNCIONALIDAD: EXTRAER RECURSOS DEL ASTEROIDE CON LA TECLA 'e'
        if (tecla == 'e')
        {
            extraer = 1; // Le avisa1mos al hilo de extraccion que queremos extraer recursos
        }

        // Si se apretó una tecla de movimiento, operamos
        if (tecla == 'w' || tecla == KEY_UP ||
            tecla == 's' || tecla == KEY_DOWN ||
            tecla == 'a' || tecla == KEY_LEFT ||
            tecla == 'd' || tecla == KEY_RIGHT)
        {
            pthread_mutex_lock(&mutex_nave);

            // Solo nos movemos si hay combustible
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
                juego_activo = 0; // Nos quedamos sin nafta
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

    /**
     * Variables para la ventana y el panel de informacion
     */
    WINDOW *ventana;
    WINDOW *panel;

    /**
     * Variables de fecha y hora
     */
    time_t t;
    struct tm *fecha;

    /**
     * Inicializamos ncurses, configuramos la ventana y el panel, y generamos la posicion inicial del enemigo de manera aleatoria.
     * Tambien se agrego box() para que se vea mas bonito y organizado, y se creo el hilo de soporte vital para que se ejecute en
     * paralelo con el hilo principal del juego.
     */
    initscr();
    noecho();
    cbreak();
    curs_set(0);

    /**
     * Se configuro la ventana principal del juego con un tamaño de 40 filas y 100 columnas, y se posiciona en la coordenada (1, 1) de la terminal.
     * Se configuro el panel de informacion con un tamaño de 7 filas y 40 columnas, y se posiciona en la coordenada (1, 105) de la terminal,
     * para que quede al lado derecho de la ventana principal.
     */
    ventana = newwin(40, 100, 1, 1);
    wtimeout(ventana, 50);
    keypad(ventana, TRUE);
    panel = newwin(16, 45, 1, 105);

    /**
     * Generamos la posicion inicial del enemigo de manera aleatoria, para que cada vez que se inicie el juego, el enemigo aparezca en una posicion diferente, y asi sea mas divertido jugar.
    A estas variables: enemigoX = rand() % 95 + 1;, enemigoY = rand() % 36 + 1; las borramos
    */

    /**
     * Agregamos box() para lo que vendria siendo el marco de la ventana y el panel,
     * para que se vea mas bonito y organizado
     */
    box(ventana, '|', '=');
    box(panel, '|', '-');


    /**
     * Conexion a la memoria compartida POSIX 
     */
    int shm_fd = shm_open("/mapa_espacial", O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("Error critico: El servidor no esta corriendo o la memoria no existe");
        exit(EXIT_FAILURE);
    }

    /**
     * Mapeamos la memoria compartida en el espacio de direcciones del proceso para poder acceder a ella como si fuera una variable normal.
     */
    mapa_servidor = mmap(NULL, sizeof(MapaEspacial), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (mapa_servidor == MAP_FAILED) {
        perror("Error al mapear la memoria compartida");
        exit(EXIT_FAILURE);
    }


    /**
     * Iniciamos los hilos y les pasamos lo que necesitan
     * agregamos el hilo de extraccion
     */
    pthread_t thread_vital, thread_propulsion, thread_extraccion;
    pthread_create(&thread_vital, NULL, hilo_soporte_vital, NULL);
    pthread_create(&thread_extraccion, NULL, hilo_extraccion, NULL);
    pthread_create(&thread_propulsion, NULL, hilo_propulsion, (void *)ventana);


    /**
     * Bucle principal del juego, en el cual se dibuja la nave, el enemigo,
     * el panel de informacion, y se actualiza la hora cada cierto tiempo.
     */
    while (juego_activo)
    {

        /**
         * Bloqueamos la pantalla para dibujar tranquilos sin que el hilo de propulsión interrumpa.
         */
        pthread_mutex_lock(&mutex_pantalla);

        for (int i = 1; i < FILAS - 1; i++)
        {
            for (int j = 1; j < COLUMNAS - 1; j++) {
                mvwaddch(ventana, i, j, mapa_servidor->matriz[i][j]);
            }
        }

         /**
         * Nave y asteroide
         */
        mvwprintw(ventana, mi_nave.y, mi_nave.x, "A");
       
        /**
         * Actualizar hora
         */
        time(&t);
        fecha = localtime(&t);

        pthread_mutex_lock(&mutex_nave); // Bloqueamos el mutex para leer el estado de la nave de forma segura
        mvwprintw(panel, 1, 1, "ESTADO NAVE:");
        mvwprintw(panel, 2, 1, "OXIGENO:     %-3d", mi_nave.oxigeno);
        mvwprintw(panel, 3, 1, "COMBUSTIBLE: %-3d", mi_nave.combustible);
        mvwprintw(panel, 5, 1, "TU BODEGA:");
        mvwprintw(panel, 6, 1, "Deuterio:   %-3d", mi_nave.carga_deuterio);
        mvwprintw(panel, 7, 1, "Mutexio:    %-3d", mi_nave.carga_mutexio);
        mvwprintw(panel, 8, 1, "Semaforita: %-3d", mi_nave.carga_semaforita);
        mvwprintw(panel, 9, 1, "Kernelio:   %-3d", mi_nave.carga_kernelio);

        // Ya no imprimimos recursos del asteroide porque no los conocemos directamente
        //mvwprintw(panel, 11, 1, "                        ");
        //mvwprintw(panel, 12, 1, "                           ");

        mvwprintw(panel, 14, 1, "%02d:%02d:%02d", fecha->tm_hour, fecha->tm_min, fecha->tm_sec);
        pthread_mutex_unlock(&mutex_nave);

        wrefresh(ventana);
        wrefresh(panel);
        pthread_mutex_unlock(&mutex_pantalla);

        usleep(50000);
    }

    /**
     * Fuera del while (juego_activo == 0 significa Game Over)
     */
    mvwprintw(ventana, FILAS / 2, (COLUMNAS / 2) - 5, "GAME OVER :(((");
    wrefresh(ventana);
    wtimeout(ventana, -1);
    wgetch(ventana);
    /**
     * Esperamos a que los hilos terminen prolijamente antes de cerrar
     */
    pthread_cancel(thread_vital);
    pthread_cancel(thread_extraccion);
    pthread_cancel(thread_propulsion); // Detenemos el hilo de propulsión
    pthread_join(thread_propulsion, NULL);

    endwin();
    return 0;
}