#include <ncurses.h>
#include <time.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

 int vida = 100;
 int oxigeno = 100;
 int combustible = 50;

 // Mutex para proteger el acceso a las variables compartidas
 phthread_mutex_t mutex_nave = PTHREAD_MUTEX_INITIALIZER;

 void* hilo_soporte_vital(void* arg) {
    while (1) {

        sleep(10);
        pthread_mutex_lock(&mutex_nave);
        
        if (oxigeno > 0) {
            oxigeno--;
        }

        int estado_oxigeno = oxigeno;
        int estado_vida = vida;

        pthread_mutex_unlock(&mutex_nave);

        // Si morimos, este hilo también tiene que terminar
        if (estado_vida <= 0 || estado_oxigeno <= 0) {
            break; 
        }
    }
    return NULL;
}

/**
 * Lo que se mejoro en esta version fue:
 * 
 */
int main()
{
    WINDOW *ventana;
    WINDOW *panel;

    int tecla;
   
    int x = 10;
    int y = 10;

    int enemigoX;
    int enemigoY;

    int contador = 0;

    // FECHA
    time_t t;
    struct tm *fecha;

    initscr();
    noecho();
    cbreak();
    curs_set(0);

    ventana = newwin(40, 100, 1, 1);
    wtimeout(ventana, 50);
    keypad(ventana, TRUE);
    panel = newwin(7, 40, 1, 105);

    srand(time(NULL));
    enemigoX = rand() % 95 + 1;
    enemigoY = rand() % 36 + 1;

    /**
     * Agregamos box() para lo que vendria siendo el marco de la ventana y el panel, para que se vea mas bonito y organizado
     */
    box(ventana, '|', '=');
    box(panel, '|', '-');

    while (1)
    {
        // nave y enemigo
        mvwprintw(ventana, y, x, "N");
        mvwprintw(ventana, enemigoY, enemigoX, " /\\ ");
        mvwprintw(ventana, enemigoY + 1, enemigoX, "<**>");
        mvwprintw(ventana, enemigoY + 2, enemigoX, " \\/ ");

        // actualizar hora
        time(&t);
        fecha = localtime(&t);
        //panel de texto
        mvwprintw(panel, 1, 1, "VIDA: %-3d", vida);
        mvwprintw(panel, 2, 1, "OXIGENO: %-3d", oxigeno);
        mvwprintw(panel,3,  1,"TIEMPO EN EL ESPACIO: %02d:%02d:%02d :)  ",fecha->tm_hour, fecha->tm_min, fecha->tm_sec);

        wrefresh(ventana);
        wrefresh(panel);
    
        tecla = wgetch(ventana);

        if(tecla == 'q') break;

        //borramos las posiciones vieja de la nave
        mvwprintw(ventana, y, x, " ");
        //borramos las posiciones viejas del enemigo
        mvwprintw(ventana, enemigoY, enemigoX, "    ");
        mvwprintw(ventana, enemigoY + 1, enemigoX, "    ");
        mvwprintw(ventana, enemigoY + 2, enemigoX, "    ");

        /**
         * Aca agregue para que se pueda aparte de mover con 'wasd', tambien se pueda mover con las flechitas.
         */
        switch(tecla)
        {
            case 'w':
            case KEY_UP:
                if(y > 1)
                    y--;
                break;

            case 's':
            case KEY_DOWN:
                if(y < 38)
                    y++;
                break;

            case 'a':
            case KEY_LEFT:
                if(x > 1)
                    x--;
                break;

            case 'd':
            case KEY_RIGHT:
                if(x < 95)
                    x++;
                break;
        }

        // calcular distancia entre nave y enemigo
        int dist_x = abs(x - enemigoX);
        int dist_y = abs(y - enemigoY);

        // mover enemigo cada cierto tiempo
        if (contador % 3 == 0) {

            // mover enemigo hacia la nave si esta cerca
            if (dist_x < 15 && dist_y < 15) {
                if (enemigoX < x) {
                    enemigoX--;
                } else if (enemigoX > x) {
                    enemigoX++;
                }    
                if (enemigoY < y) {
                    enemigoY--;
                } else if (enemigoY > y) {
                    enemigoY++;
                }

                // limitar movimiento del enemigo dentro de la ventana
                if (enemigoX < 1) enemigoX = 1;
                if (enemigoX > 95) enemigoX = 95; // Tope corregido
                if (enemigoY < 1) enemigoY = 1;
                if (enemigoY > 36) enemigoY = 36;
            }            
        }

        /**
         *colision: por si el enemigo esta cerca de la nave, le resta vida y oxigeno a la nave,
          y mueve al enemigo a una posicion aleatoria, para que se mueva el asteroide.
         */
        if ((x >= enemigoX && x <= enemigoX + 3) && (y >= enemigoY && y <= enemigoY + 2))
        {
            vida -= 10;
            oxigeno --;
            // mover enemigo
            enemigoX = rand() % 95 + 1;
            enemigoY = rand() % 36 + 1;   
        } else if (vida <= 0 || oxigeno <= 0)
        {
            mvwprintw(ventana, 20, 40, "GAME OVER :(((");
            // esto estaba arriba del todo, lo que hacia titilar la pantalla cada vez que movia la navecita.
            wrefresh(ventana);
            wtimeout(ventana, -1);
            wgetch(ventana);
            break;
        }

        contador++;
    }

    endwin();
    return 0;
}