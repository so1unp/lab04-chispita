#include <ncurses.h>
#include <time.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

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
  * Variables compartidas para el estado de la nave
  */
 int oxigeno = 100;
 int combustible = 10000;



 /**
  * Variable para controlar el estado del juego
  */
 int juego_activo = 1;



 /**
  * Bodega de minerales
  */
 int carga_deuterio = 0;
 int carga_mutexio = 0;
 int carga_semaforita = 0;
 int carga_kernelio = 0;



 /**
  * Recursos del asteroide
  */
 int asteroide_deuterio = 0;
 int asteroide_mutexio = 0;
 int asteroide_semaforita = 0;
 int asteroide_kernelio = 0;



 /**
  * Variables de extraccion minera
  */
 int extraer = 0; // bandera para comunicar el teclado con el hilo minero



 /**
  * Posiciones de la nave y de(los) asteroide(s).
  * Las dos primeras corresponden a la posicion x e y de la nave, y las
  * otras dos a la(s) posicion(es) del asteroide(s).
  */
 int x = 10;
 int y = 10;
 int enemigoX;
 int enemigoY;



 /**
  * El primer mutex sirve para proteger el acceso a las variables compartidas. 
  * Y el segundo mutex es para proteger el acceso a la pantalla, para evitar que 
  * los hilos se pisen entre ellos al escribir en la pantalla.
  */
 pthread_mutex_t mutex_nave = PTHREAD_MUTEX_INITIALIZER;
 pthread_mutex_t mutex_pantalla = PTHREAD_MUTEX_INITIALIZER;



 /**
  * Funcion para generar recursos del asteroide de manera aleatoria,
  * para que cada vez que se inicie el juego, 
  * el asteroide tenga una cantidad diferente de recursos,
  * y asi sea mas divertido jugar.
  */
 void generar_recursos_asteroide() {
    asteroide_deuterio = rand() % 15;
    asteroide_mutexio = rand() % 15;
    asteroide_semaforita = rand() % 15;
    asteroide_kernelio = rand() % 15;
 }



 /**
  * Hilo de soporte vital: es el que se encargara de decrementar oxigeno de manera periodica, es decir,
  * por un intervalo de tiempo determinado, en este caso cada 5 segundos, y si el oxigeno llega a 0, 
  * se termina el juego, porque la nave se quedo sin oxigeno y no puede seguir funcionando.
  * Funciona de tal forma que si el oxigeno es mayor a 0, se resta 1 unidad y si el oxigeno es menor o igual a 0,
  * avisamos con la flag de juego_activo que el juego ha terminado.
  * */
 void* hilo_soporte_vital(void* arg) {
    while (juego_activo) {

        sleep(5);        
        pthread_mutex_lock(&mutex_nave); // Bloqueamos el mutex para modificar el estado de la nave de forma segura

        if (oxigeno > 0) {
            oxigeno--;
        }

        if (oxigeno <= 0) {
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
void* hilo_extraccion(void* arg) {
    while (juego_activo) {

        if (extraer == 1) {
            pthread_mutex_lock(&mutex_nave);

            //distancia del enemigo, lo vamos a declarar en un rango de 5 casilleros
            int dist_x = abs(x - enemigoX);
            int dist_y = abs(y - enemigoY);

            //aca si, verificamos que este en un rango de 5 casilleros respecto del asteroide
            if (dist_x <= 5 && dist_y <= 5) {

                // Si el asteroide tiene recursos disponibles, y la nave tiene combustible suficiente, se realiza la extraccion
                if ((asteroide_deuterio > 0 || asteroide_mutexio > 0 || asteroide_semaforita > 0 || asteroide_kernelio > 0 ) && combustible >= 2) {
                    
                    combustible -= 2; // Extraer consume combustible

                    if (asteroide_deuterio > 0) {
                        asteroide_deuterio--;
                        carga_deuterio++;
                    }
                    if (asteroide_mutexio > 0) {
                        asteroide_mutexio--;
                        carga_mutexio++;
                    }
                    if (asteroide_semaforita > 0) {
                        asteroide_semaforita--;
                        carga_semaforita++;
                    }
                    if (asteroide_kernelio > 0) {
                        asteroide_kernelio--;
                        carga_kernelio++;
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
void* hilo_propulsion(void* arg) {
    
    WINDOW *ventana = (WINDOW *)arg;    
    int tecla;

    while (juego_activo) {
        pthread_mutex_lock(&mutex_pantalla);
        tecla = wgetch(ventana);
        pthread_mutex_unlock(&mutex_pantalla);

        if (tecla == 'q') {
            juego_activo = 0; // Avisa al resto que perdimos
            break;
        }

        //AGREGAMOS NUEVA FUNCIONALIDAD: EXTRAER RECURSOS DEL ASTEROIDE CON LA TECLA 'e'
        if (tecla == 'e') {
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
            if (combustible > 0) {
                if ((tecla == 'w' || tecla == KEY_UP) && y > 1) { y--; combustible--; }
                if ((tecla == 's' || tecla == KEY_DOWN) && y < 38) { y++; combustible--; }
                if ((tecla == 'a' || tecla == KEY_LEFT) && x > 1) { x--; combustible--; }
                if ((tecla == 'd' || tecla == KEY_RIGHT) && x < 95) { x++; combustible--; }
            }

            if (combustible <= 0) {
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
     * Variable para la tecla que se presiona.
     */
    int contador = 0;



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
     */
    srand(time(NULL));
    enemigoX = rand() % 95 + 1;
    enemigoY = rand() % 36 + 1;



    /**
     * llamamos a la funcion de generar recursos del asteroide
     */
    generar_recursos_asteroide();



    /**
     * Agregamos box() para lo que vendria siendo el marco de la ventana y el panel, 
     * para que se vea mas bonito y organizado
     */
    box(ventana, '|', '=');
    box(panel, '|', '-');
    


    /**
     * Iniciamos los hilos y les pasamos lo que necesitan
     * agregamos el hilo de extraccion
     */
    pthread_t thread_vital, thread_propulsion, thread_extraccion;
    pthread_create(&thread_vital, NULL, hilo_soporte_vital, NULL);
    pthread_create(&thread_extraccion, NULL, hilo_extraccion, NULL);
    pthread_create(&thread_propulsion, NULL, hilo_propulsion, (void *)ventana);



    /**
     * Agregamos estas variables antes del while para que se puedan borrar las 
     * posiciones viejas de la nave y del enemigo, para que no queden fantasmas en la pantalla,
     * y asi se vea mas limpio el movimiento de la nave y del enemigo.
     */
    int viejo_x = x;
    int viejo_y = y;
    int viejo_enemigoX = enemigoX;
    int viejo_enemigoY = enemigoY;



    /**
     * Bucle principal del juego, en el cual se dibuja la nave, el enemigo,
     * el panel de informacion, y se actualiza la hora cada cierto tiempo.
     */
    while (juego_activo) {


        /**
         * Bloqueamos la pantalla para dibujar tranquilos sin que el hilo de propulsión interrumpa.
         */
        pthread_mutex_lock(&mutex_pantalla);



        /**
         * Guardamos las posiciones viejas de la nave y del enemigo, para que se puedan borrar en la siguiente iteracion
         * del while, y asi no queden "fantasmas" en la pantalla, y se vea mas limpio el movimiento de la nave y del enemigo.
         */
        mvwprintw(ventana, viejo_y, viejo_x, " ");
        mvwprintw(ventana, viejo_enemigoY, viejo_enemigoX, "    ");
        mvwprintw(ventana, viejo_enemigoY + 1, viejo_enemigoX, "    ");
        mvwprintw(ventana, viejo_enemigoY + 2, viejo_enemigoX, "    ");



        /**
         * Calcular distancia entre nave y enemigo
         */
        int dist_x = abs(x - enemigoX);
        int dist_y = abs(y - enemigoY);



        /*
        * Mover enemigo cada cierto tiempo
        */
        if (contador % 3 == 0) {

            /**
             * Mover enemigo hacia la nave si esta cerca
             */
            if (dist_x < 15 && dist_y < 15) {
                if (enemigoX < x) enemigoX--; else if (enemigoX > x) enemigoX++;    
                if (enemigoY < y) enemigoY--; else if (enemigoY > y) enemigoY++;

                if (enemigoX < 1) enemigoX = 1; if (enemigoX > 95) enemigoX = 95; 
                if (enemigoY < 1) enemigoY = 1; if (enemigoY > 36) enemigoY = 36;
            }
        }



        /**
         * Colision: si la nave colisiona con el enemigo, se descuenta 1 de oxigeno y 3 de combustible, y el enemigo se reposiciona
         * en una nueva posicion aleatoria, para que el juego sea mas dinamico y divertido, y asi no quede estatico el enemigo en una sola posicion.
         */
        if ((x >= enemigoX && x <= enemigoX + 3) && (y >= enemigoY && y <= enemigoY + 2)) {
            pthread_mutex_lock(&mutex_nave);
            oxigeno -= 1;
            combustible -= 3;
            if (oxigeno <= 0 || combustible <= 0) juego_activo = 0;
            pthread_mutex_unlock(&mutex_nave);

            enemigoX = rand() % 95 + 1;
            enemigoY = rand() % 36 + 1;   
            generar_recursos_asteroide();
        }



        /**
         * Guardamos las posiciones viejas de la nave y del enemigo, para que se puedan borrar en la siguiente iteracion
         * del while, y asi no queden "fantasmas" en la pantalla, y se vea mas limpio el movimiento de la nave y del enemigo.
         */
        viejo_x = x;
        viejo_y = y;
        viejo_enemigoX = enemigoX;
        viejo_enemigoY = enemigoY;
        
        

        /**
         * Nave y asteroide
         */
        mvwprintw(ventana, y, x, "A");
        mvwprintw(ventana, enemigoY, enemigoX, " /\\ ");
        mvwprintw(ventana, enemigoY + 1, enemigoX, "<**>");
        mvwprintw(ventana, enemigoY + 2, enemigoX, " \\/ ");



        /**
         * Actualizar hora
         */
        time(&t);
        fecha = localtime(&t);

        pthread_mutex_lock(&mutex_nave); // Bloqueamos el mutex para leer el estado de la nave de forma segura
        /**
         * Panel de texto
         */
        mvwprintw(panel, 1, 1, "ESTADO NAVE:");
        mvwprintw(panel, 2, 1, "OXIGENO:     %-3d", oxigeno);
        mvwprintw(panel, 3, 1, "COMBUSTIBLE: %-3d", combustible);
        mvwprintw(panel, 5, 1, "TU BODEGA:");
        mvwprintw(panel, 6, 1, "Deuterio:   %-3d", carga_deuterio);
        mvwprintw(panel, 7, 1, "Mutexio:    %-3d", carga_mutexio);
        mvwprintw(panel, 8, 1, "Semaforita: %-3d", carga_semaforita);
        mvwprintw(panel, 9, 1, "Kernelio:   %-3d", carga_kernelio);
        mvwprintw(panel, 11, 1, "RECURSOS EN ASTEROIDE:");
        mvwprintw(panel, 12, 1, "D:%-2d M:%-2d S:%-2d K:%-2d", asteroide_deuterio, asteroide_mutexio, asteroide_semaforita, asteroide_kernelio);
        
        mvwprintw(panel, 14, 1, "%02d:%02d:%02d", fecha->tm_hour, fecha->tm_min, fecha->tm_sec);
        pthread_mutex_unlock(&mutex_nave);



        /**
         * Refrescamos pantalla
         */
        wrefresh(ventana);
        wrefresh(panel);
        pthread_mutex_unlock(&mutex_pantalla); // Liberamos la pantalla



        /**
         * Pausa del radar para no consumir 100% de CPU
         */
        usleep(50000);
        contador++;
    }



    /**
     * Fuera del while (juego_activo == 0 significa Game Over)
     */
    mvwprintw(ventana, 20, 40, "GAME OVER :(((");
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