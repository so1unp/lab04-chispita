#include <stdio.h>
#include <stdlib.h>
#include <ncurses.h>
#include <unistd.h>
#include <pthread.h>  //hilos

//reglas que pensamos entre los integrantes del grupo para el trueque de recursos en la estacion espacial YPF
//1. Cada nave puede realizar un trueque de recursos en la estación YPF una vez que llegue a la estación.
//2. El trueque se realiza en función de la cantidad de recursos que la nave tiene y la cantidad de recursos disponibles en la estación YPF.
//3. los precios de los recursos son fijos y no cambian durante el truque
//4. oxigeno necesitas 5 unidades de CADA UNO mutexio semaforita y kerneleio para 10 unidades de oxigeno
//5. nafta necesitas 1 unidad de cada uno de los recursos "mutexio", "semaforita" y "kernelio" para 10 unidades de oxigeno
//y estas las recomendaciones de fran quiero ir a de una aplicando uno y uno 
        //variables que meti dentro del struc por recomendacion de fran 
            //1. recomendacion de fran fue ir por partes e implementar hilos por partes osea uno a la vez y ver problemas de concurrencia  a lo ultimo 
            //2. primer hilo creado solo para las acciones y dejo el main como un hilo
            //3. agarro problemas de  a no y voy adaptando lo ultimo en hacer es agg las colas de mensajes  ahi se manejan como un fifo osea el primer que llega
            //ejeucta 
//entonces ayudame con las tareas de a una 
 //un arrgelo de naves
 //moviminetos de la nave 
typedef struct {
    int MAXnaves;
    int navesEnEstacion;
    int tecla;
    int corriendo;

    int oxigeno;
    int nafta;
    int recolector0;
    int recolector1;

    int oxigeno_jugador;
    int nafta_jugador;

    int deuterio;
    int mutexio;
    int semaforita;
    int kernelio;
} EstadoYPF;

typedef struct {
    WINDOW *ventana;
    EstadoYPF *estado;
} Contexto;

// hilo de acciones lee las  teclas y aplica el trueque de la ypd

void *hilo_acciones(void *arg)
{
    Contexto *ctx = (Contexto *) arg;
    WINDOW *ventana = ctx->ventana;
    EstadoYPF *estado = ctx->estado;

    while (estado->corriendo)
    {
        if (estado->navesEnEstacion <= estado->MAXnaves)
        {
            estado->tecla = wgetch(ventana);

            switch (estado->tecla)
            {
                //cambio por nafta
                case '1':
                    if (estado->deuterio >= 5) {
                        estado->deuterio -= 5;
                        estado->nafta -= 10;
                        estado->recolector1++;
                        estado->nafta_jugador += 10;

                        if (estado->nafta == 0) {
                            mvwprintw(ventana, 16, 2, "Nafta: %d  ", estado->nafta);
                            mvwprintw(ventana, 20, 2, " NO HAY NAFTA Reponiendo... ");
                            wrefresh(ventana);
                            sleep(1);

                            if (estado->nafta == 0 && estado->recolector1 >= 1) {
                                estado->nafta = 100;
                                estado->recolector1--;
                            }
                        }
                    } else {
                        mvwprintw(ventana, 20, 2, "No tenes deuterio.");
                        wrefresh(ventana);
                        wgetch(ventana);
                    }
                    break;

                //cambio por oxigeno
                case '2':
                    if (estado->mutexio >= 1 && estado->semaforita >= 1 && estado->kernelio >= 1) {
                        estado->mutexio--;
                        estado->semaforita--;
                        estado->kernelio--;
                        estado->recolector0 += 3;
                        estado->oxigeno -= 10;
                        estado->oxigeno_jugador += 10;

                        if (estado->oxigeno == 0 && estado->recolector0 >= 3) {
                            mvwprintw(ventana, 15, 2, "Oxigeno: %d   ", estado->oxigeno);
                            mvwprintw(ventana, 20, 2, " NO HAY OXIGENO Reponiendo... ");
                            wrefresh(ventana);
                            sleep(2);
                        }

                        if (estado->oxigeno == 0) {
                            estado->oxigeno = 40;
                            estado->recolector0 -= 3;
                        }
                    } else {
                        mvwprintw(ventana, 20, 2, "no tenes minerales suficientes  ");
                        wrefresh(ventana);
                        wgetch(ventana);
                    }
                    break;

                case 'q':
                    mvwprintw(ventana, 20, 2, "Saliendo de la YPF.");
                    wrefresh(ventana);
                    estado->corriendo = 0;
                    break;

                default:
                    break;
            }
        }
        else
        {
            estado->tecla = wgetch(ventana);
            if (estado->tecla == 'q') {
                estado->corriendo = 0;
            }
        }
    }

    return NULL;
}

int main()
{
    WINDOW *ventana; 

    EstadoYPF estado;
    estado.MAXnaves = 3;
    estado.navesEnEstacion = 3;
    estado.tecla = 0;
    estado.corriendo = 1;

    estado.oxigeno = 20;
    estado.nafta = 20;
    estado.recolector0 = 0;
    estado.recolector1 = 0;

    estado.oxigeno_jugador = 0;
    estado.nafta_jugador = 0;

    estado.deuterio = 30;
    estado.mutexio = 5;
    estado.semaforita = 5;
    estado.kernelio = 5;

    initscr();
    noecho();
    cbreak();

    ventana = newwin(28, 85, 2, 5);
    keypad(ventana, TRUE);

    Contexto ctx;
    ctx.ventana = ventana;
    ctx.estado = &estado;

    pthread_t hilo;
    pthread_create(&hilo, NULL, hilo_acciones, &ctx);


    // hilo principal del main : solo dibuja la pantalla aca prestar atencion 
    
    while (estado.corriendo)
    {
        werase(ventana);
        box(ventana, 0, 0);

        mvwprintw(ventana, 2, 20, "========================================");
        mvwprintw(ventana, 3, 20, "|                                      |");
        mvwprintw(ventana, 4, 20, "|      ESTACION ESPACIAL YPF           |");
        mvwprintw(ventana, 5, 20, "|                                      |");
        mvwprintw(ventana, 6, 20, "|    [HANGAR 1]      [HANGAR 2]        |");
        mvwprintw(ventana, 7, 20, "|                                      |");
        mvwprintw(ventana, 8, 20, "|          O       O       O           |");
        mvwprintw(ventana, 9, 20, "|         /|\\     /|\\     /|\\          |");
        mvwprintw(ventana,10, 20, "|                                      |");
        mvwprintw(ventana,11, 20, "========================================");

        mvwprintw(ventana, 13, 2, "Naves en la estacion: %d", estado.navesEnEstacion);
        mvwprintw(ventana, 14, 2, "--------recursos de YPF--------");
        mvwprintw(ventana, 15, 2, "Oxigeno: EN YPF %d", estado.oxigeno);
        mvwprintw(ventana, 16, 2, "Nafta: EN YPF %d", estado.nafta);
        mvwprintw(ventana, 17, 2, "-------Tus minerales:-------");
        mvwprintw(ventana, 18, 2, "Deuterio: %d", estado.deuterio);
        mvwprintw(ventana, 18, 22, "Mutexio: %d", estado.mutexio);
        mvwprintw(ventana, 18, 42, "Semaforita: %d", estado.semaforita);
        mvwprintw(ventana, 18, 65, "Kernelio: %d", estado.kernelio);

        if (estado.navesEnEstacion <= estado.MAXnaves)
        {
            mvwprintw(ventana, 21, 2, "[1] 5 Deuterio -> 10 Nafta");
            mvwprintw(ventana, 22, 2, "[2] 1 Mutexio + 1 Semaforita + 1 Kernelio -> 10 Oxigeno");
            mvwprintw(ventana, 23, 2, "[q] Salir");
            mvwprintw(ventana, 24, 2, "---------Panel del juagdor----------");
            mvwprintw(ventana, 25, 2, "oxigeno jugador: %d", estado.oxigeno_jugador);
            mvwprintw(ventana, 26, 2, "nafta jugador: %d", estado.nafta_jugador);
        } else {
            mvwprintw(ventana, 21, 2, "naves cargandoo. YPF esta haciendo un trueque");
            mvwprintw(ventana, 23, 2, "[q] Salir");
        }

        wrefresh(ventana);

        usleep(100000); //10 redibujados por segundo, no hace falta mas para una UI de texto
    }

    //esperar a que el hilo de acciones termine antes de cerrar ncurses
    pthread_join(hilo, NULL);

    delwin(ventana);
    endwin();
    return 0;
}