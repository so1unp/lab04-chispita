#include <ncurses.h>
#include <time.h>
#include <stdlib.h>

int main()
{
    WINDOW *ventana;
    WINDOW *panel;

    int tecla;
    int vida = 100;
    int oxigeno = 100;
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

    timeout(50);

    ventana = newwin(40, 100, 1, 1);

    keypad(ventana, TRUE);

    panel = newwin(7, 40, 1, 105);

    srand(time(NULL));

    enemigoX = rand() % 88 + 2;
    enemigoY = rand() % 38 + 2;

    while (1)
    {
        // actualizar hora
        time(&t);
        fecha = localtime(&t);

        werase(ventana);
        werase(panel);

    
        box(ventana, '|', '=');
        box(panel, '|', '-');

        // nave
        mvwprintw(ventana, y, x, "N");

        mvwprintw(ventana, enemigoY, enemigoX, " /\\ ");
        mvwprintw(ventana, enemigoY + 1, enemigoX, "<**>");
        mvwprintw(ventana, enemigoY + 2, enemigoX, " \\/ ");

        if ((x >= enemigoX && x <= enemigoX + 3) && (y >= enemigoY && y <= enemigoY + 2))
        {
            vida -= 10;
            oxigeno --;
            // mover enemigo
            enemigoX = rand() % 88 + 2;
            enemigoY = rand() % 38 + 2;
            
        }else if (vida==0)
        {
            mvwprintw(ventana, 20, 40, "GAME OVER :(((");
            wrefresh(ventana);
            getch();
            break;
        }
        
        // pandel DE textooooo
        mvwprintw(panel, 1, 1, "VIDA: %d", vida);
        mvwprintw(panel, 2, 1, "OXIGENO: %d", oxigeno);
        mvwprintw(panel,3,  1,"TIEMPO EN EL ESPACIO: %02d:%02d:%02d :)  ",fecha->tm_hour, fecha->tm_min, fecha->tm_sec);

        contador++;

        // mover enemigo cada cierto tiempo
        if(contador % 50 == 0)
        {
            enemigoX = rand() % 88 + 2;
            enemigoY = rand() % 38 + 2;
        }

        // mostrar
        wrefresh(ventana);
        wrefresh(panel);

        // leer tecla
        tecla = wgetch(ventana);

        if(tecla == 'q')
            break;

        switch(tecla)
        {
            case 'w':
                if(y > 1)
                    y--;
                break;

            case 's':
                if(y < 38)
                    y++;
                break;

            case 'a':
                if(x > 1)
                    x--;
                break;

            case 'd':
                if(x < 98)
                    x++;
                break;
        }
    }

    endwin();

    return 0;
}