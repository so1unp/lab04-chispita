#include <ncurses.h>

#define corIniX 2
#define corIniY 1

#define maxCorY 21
#define maxCorX 42

#define filas 11
#define columnas 11


int main() {
    initscr();
    noecho();
    curs_set(0);
    
    int x = corIniX;
    int y = corIniY;

    int celAlt = 2; // altura de cada celda
    int celAnch = 4; // ancho de cada celda

    refresh();
    getch();
    endwin();

    while (1) {
        clear();

        // Dibujar líneas horizontales
        for (int i = 0; i <= filas; i++) {
            for (int j = 0; j <= columnas * celAnch; j++) {
                mvaddch(i * celAlt, j, '-');
            }
        }

        // Dibujar líneas verticales
        for (int i = 0; i <= filas * celAlt; i++) {
            for (int j = 0; j <= columnas; j++) {
                mvaddch(i, j * celAnch, '|');
            }
        }

        // Dibujar intersecciones
        for (int i = 0; i <= filas; i++) {
            for (int j = 0; j <= columnas; j++) {
                mvaddch(i * celAlt, j * celAnch, '+');
            }
        }

        mvaddch(y, x, 'X');

        refresh();

        char c = getch();

        switch(c) {

            case 'w': y=y-celAlt;
            if(y==-1){
                y=maxCorY;
            }
            break;

            case 's': y=y+celAlt; 
            if(y>maxCorY){
                y=corIniY;
            }
            break;

            case 'a': x=x-celAnch; 
            if(x<0){
                x=maxCorX;
            }
            break;

            case 'd': x=x+celAnch; 
            if(x>maxCorX){
                x=corIniX;
            }
            break;
        }
    }
    
    return 0;
}   
