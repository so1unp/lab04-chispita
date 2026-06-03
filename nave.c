/*
    COMANDO PARA COMPILAR:

    gcc nave.c -lncursesw -o nave
*/

#include <ncurses.h>
#include <time.h>
<<<<<<< HEAD
#include <locale.h>
#include <stdbool.h>

#define corIniX 22 // coordenada x inicial de la nave 
#define corIniY 4 // coordenada y inicial de la nave

#define maxCorX 102 // coordenada x máxima para la nave
#define maxCorY 24 // coordenada y máxima para la nave

#define columnas 21
#define filas 11


int main() {
    setlocale(LC_ALL, "");
    initscr();
    noecho();
    curs_set(0); 
    nodelay(stdscr, TRUE);

    start_color();
    init_color(8, 200, 200, 200); // gris
    init_pair(1, 8, COLOR_BLACK);
    init_pair(2, COLOR_GREEN, COLOR_BLACK);
    
    time_t ultimoMov = 0;
    char *nave = "↓";
    char *modo = "NAVE";
    int x = corIniX;
    int y = corIniY;

    /*SIN LÓGICA APLICADA*/
    int combustible = 100;
    int oxigeno = 100;
    int naves = 1;
    int mutexio = 0;
    int semaforita = 0;
    int kernelio = 0;
    bool modoDisparo = false;
    /********************/

    int celAlt = 2; // altura de cada celda
    int celAnch = 4; // ancho de cada celda

    int maxY, maxX;
    getmaxyx(stdscr, maxY, maxX);
    int ancho = columnas * celAnch + 1;
    int alto  = filas * celAlt + 1;
    int inicioX = (maxX - ancho) / 2 + 1; // 1 -> desplazamiento hacia la derecha para el mayor centrado
    int inicioY = (maxY - alto) / 2;

    refresh();

    
    while (1) {
        erase();

        time_t relojAhora = time(NULL);

        // Dibujar líneas horizontales
        attron(COLOR_PAIR(1));

        for (int i = 0; i <= filas; i++) {
            for (int j = 0; j <= columnas * celAnch; j++) {
                mvaddch(inicioY + i * celAlt, inicioX + j, '-');
            }
        }

        // Dibujar líneas verticales
        for (int i = 0; i <= filas * celAlt; i++) {
            for (int j = 0; j <= columnas; j++) {
                mvaddch(inicioY + i, inicioX + j * celAnch, '|');
            }
        }

        // Dibujar intersecciones
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
=======
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
>>>>>>> 3a19ce90ec47dc953da739f1bcd98095a370368c
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
    
<<<<<<< HEAD
=======
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

>>>>>>> 3a19ce90ec47dc953da739f1bcd98095a370368c
    endwin();
    return 0;
}