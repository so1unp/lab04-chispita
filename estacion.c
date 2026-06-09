#include <stdio.h>
#include <stdlib.h>
#include <ncurses.h>
#include <unistd.h>
//reglas que pensamos entre los integrantes del grupo para el trueque de recursos en la estacion espacial YPF
//1. Cada nave puede realizar un trueque de recursos en la estación YPF una vez que llegue a la estación.
//2. El trueque se realiza en función de la cantidad de recursos que la nave tiene y la cantidad de recursos disponibles en la estación YPF.
//3. los precios de los recursos son fijos y no cambian durante el truque
//4. oxigeno necesitas 5 unidades de CADA UNO mutexio semaforita y kerneleio para 10 unidades de oxigeno
//5. nafta necesitas 1 unidad de cada uno de los recursos "mutexio", "semaforita" y "kernelio" para 10 unidades de oxigeno

int main()
{
    WINDOW *ventana; //ventana de la YPF
  //semaforos 
    int MAXnaves = 3; //cantidad maxima de naves que puede recibir
    int tecla; //variable para leer la tecla que se presiona

    //cantidad de los recursos en la estación YPF
  int oxigeno = 1000;
  int nafta = 1000;
 int recolector0 = 0; //CONTADOR PARA SABER CUANTAS VECES SE AGOTÓ EL OXIGENO Y TENER EN CUENTA CUANTO MINERALES HAY 1 UNIDAD DE ESTO REPRENSENTA 3 MINERALES.
int recolector1= 0; //CONTADOR PARA SABER CUANTAS VECES SE AGOTÓ LA NAFTA Y TENER EN CUENTA CUANTO MINERALES HAY 1 UNIDAD DE ESTO REPRENSENTA 3 MINERALES.

 // hay 20 de oxigeno de stock en la ypf
 //llegan 3 naves 
 // la primera se lleva 10 ENTRGAN 1
 // la segunda se lleva 10 ENTRGAN 1 
 // la tercera se lleva 0
 // no puede entrar porque no hay stock
//MINERALES QUE VA A TENER LA YPF 2 LA YPF 3+3=6 --> 1
// NOSOTROMOS PARA FABRICAR LOS RECURSOS  NECESITAMOS 1 TOTAL PARA GENERAL 20 DE OXIGENO 

    //IMPORTANTEEEEEE
    //variables que estan harcodeadas, pero que se pueden modificar para probar la logica de la ypf
    int navesEnEstacion = 3; //cantidad de naves que hay (SI ES 3 O MÁS, SE BLOQUEA Y MUESTRA EL MENSAJE)
    //Deuterio  combustible de las naves 
    int deuterio = 30;
    //Mutexio, semaforita y kernelio: minerales con múltiples usos como generación de oxígeno,
    //preparacion de aleaciones ultra-resistentes o condimento para pizzas.
    int mutexio = 5;
    int semaforita = 5;
    int kernelio = 5;
    //paso de mensajes 
// hilo que escuche por mensajes para intercambio de minerales y devuelvo otro mensakes
//suma y resta de recursos no es memoria compartida 
//mas dificil pero s eouede hacer con memoria comoartida donde las naves van a escribir baja un mutex toma el combustible e incrementa y suleta la memoria 
    initscr();
    noecho();
    cbreak();

    //crear ventana principal
    ventana = newwin(25, 80, 2, 5);

    keypad(ventana, TRUE);

    while (1)
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

        
        //informacion de la ypf
        mvwprintw(ventana, 13, 2,"Naves en la estacion: %d", navesEnEstacion);
        mvwprintw(ventana, 14, 2, "--------recursos de YPF--------");
        mvwprintw(ventana, 15, 2,  "Oxigeno: EN YPF %d",   oxigeno);
        mvwprintw(ventana, 16, 2, "Nafta: EN YPF %d",nafta);
        mvwprintw(ventana, 17, 2, "------------------------");

        //minerales 
        mvwprintw(ventana, 18, 2, "Tus minerales:");
        mvwprintw(ventana, 18, 2, "Deuterio: %d", deuterio);
        mvwprintw(ventana, 18, 22,  "Mutexio: %d",mutexio);
        mvwprintw(ventana, 18, 42, "Semaforita: %d", semaforita);
        mvwprintw(ventana, 18, 65,"Kernelio: %d",kernelio);


        //logica de ypf
        if (navesEnEstacion <= MAXnaves)
        {
            //menu 
            mvwprintw(ventana, 21, 2,"[1] 5 Deuterio -> 10 Nafta");
            mvwprintw(ventana, 22, 2, "[2] 1 Mutexio + 1 Semaforita + 1 Kernelio -> 10 Oxigeno");
            mvwprintw(ventana, 23, 2,"[q] Salir");
        }else{
            mvwprintw(ventana, 21, 2, "naves cargandoo. YPF esta haciendo un trueque");
            mvwprintw(ventana, 23, 2, "[q] Salir");
        }

    

        wrefresh(ventana);

        
        tecla = wgetch(ventana);

      //trueque
        if (navesEnEstacion <= MAXnaves){
            switch(tecla)
            {
                //cambio por nafta
               case '1':
                        if (deuterio >= 5){
                            deuterio -= 5;
                            nafta -= 10;
                            recolector1++;

                            if (nafta == 0 ){
                                mvwprintw(ventana, 16, 2, "Nafta: %d  ", nafta);
                                mvwprintw(ventana, 20, 2, " NO HAY NAFTA Reponiendo... ");
                                wrefresh(ventana);
                                sleep(1);


                                if (nafta == 0 && recolector1 >=  1){
                                    nafta =100;
                                    recolector1-- ;
                                }
                            }

                            mvwprintw(ventana, 16, 2, "Nafta: %d  ", nafta);
                            mvwprintw(ventana, 20, 2, "Trueque realizado: 5 Deuterio por 10 Nafta  ");

                        }else{
                            mvwprintw(ventana, 20, 2, "No tenes suficiente Deuterio. ");
                        }
                        break;

                //cambio por oxigeno 
                case '2':
                    if (mutexio >= 1 && semaforita >= 1 && kernelio >= 1) {
                        mutexio--;
                        semaforita--;
                        kernelio--;
                        recolector0+=3;
                        oxigeno -=10;

                        if(oxigeno==0 && recolector0 >= 3){
                         mvwprintw(ventana, 15, 2, "Oxigeno: %d   ", oxigeno); 
                           mvwprintw(ventana, 20, 2, " NO HAY OXIGENO Reponiendo... ");
                             wrefresh(ventana);  
                             sleep(2);          
                        }

                     if ( oxigeno == 0 ) {
                            oxigeno=40;
                            recolector0-=3;
                        }
                                                                         

                        mvwprintw(ventana, 20, 2,"Trueque realizado: minerales por 10 Oxigeno.  ");
                    }else{
                        mvwprintw(ventana, 20, 2,  "No tienes suficientes minerales.");
                    }
                    break;

                case 'q':
                    mvwprintw(ventana, 20, 2,  "Saliendo de la YPF.");
                    wrefresh(ventana);
                    delwin(ventana);
                    endwin();
                    return 0;

                default:
                    break;
            }
        }else{
            if (tecla == 'q') {
                mvwprintw(ventana, 20, 2,  "No hay playeros");
                wrefresh(ventana);
                delwin(ventana);
                endwin();
                return 0;
            }
        }
    }

    delwin(ventana);
    endwin();
    return 0;
}
