#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ncurses.h>
#include <time.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include "compartido.h"

#define corIniX 5
#define corIniY 5

#define celAnch 4
#define celAlt 2

#define columnas 13
#define filas 11

void colisionProy(MapaEspacial *mapa);
void generarEstacion(MapaEspacial *mapa, int cantidad);
void generarAsteroide(MapaEspacial *mapa, int cantidad);

int main(int argc, char *argv[])
{

    /*Esto es para validar las naves*/
    if (argc < 3) {
        printf("Error. Uso: ./servidor <cantidad_naves> <cantidad_estaciones>\n");
        exit(EXIT_FAILURE);
    }

    int shh_fd = shm_open("/mapa_espacial", O_CREAT | O_RDWR, 0666);
    if (shh_fd == -1)
    {
        perror("¡Error al crear la memoria compartida!");
        exit(EXIT_FAILURE);
    }

    ftruncate(shh_fd, sizeof(MapaEspacial));

    MapaEspacial *mapa_servidor = mmap(NULL, sizeof(MapaEspacial), PROT_READ | PROT_WRITE, MAP_SHARED, shh_fd, 0);
    if (mapa_servidor == MAP_FAILED)
    {
        perror("¡Error al mapear la memoria compartida!");
        exit(EXIT_FAILURE);
    }

    /*Guardar la cantidad de naves en la memoria compartida*/
    int limite_naves = atoi(argv[1]);
    if (limite_naves > MAX_NAVES) {
        limite_naves = MAX_NAVES; //proteccion por si piden demasiadas
    }
    mapa_servidor->cantidad_naves_permitidas = limite_naves; 

    /*Guardar la cantidad de estaciones en la memoria compartida*/
    int limite_estaciones = atoi(argv[2]);
    if (limite_estaciones > MAX_ESTACIONES) {
        limite_estaciones = MAX_ESTACIONES; //proteccion por si piden demasiadas
    }
    mapa_servidor->cantidad_estaciones = limite_estaciones;

    srand(time(NULL));
    generarAsteroide(mapa_servidor, 20);
    generarEstacion(mapa_servidor, mapa_servidor->cantidad_estaciones); // Generar estaciones según la cantidad especificada

    mapa_servidor->juego_activo = 1;

    printf("Compilando la estacion YPF...\n");
    system("gcc -o estacion estacion.c -lrt -lpthread");

    //Agregamos el for para poder levantar las estaciones al prender el servidor
    for (int i = 0; i < mapa_servidor->cantidad_estaciones; i++) {
        pid_t pid = fork();

        if (pid < 0) {
            perror("Error al hacer el fork");
            exit(EXIT_FAILURE);
        } else if (pid == 0) {
            //proceso hijo
            char id_str[20];
            sprintf(id_str, "%d", i);

            //Abrimos una terminal nueva (x-terminal-emulator) "-e" "./estacion"
            execlp("x-terminal-emulator", "x-terminal-emulator", "-e", "./estacion", id_str, NULL);

            //si falla el programa, el programa sigue por aca e imprime el error
            perror("Fallo al ejecutar la estacion");
            exit(EXIT_FAILURE);
        }

    }

    initscr();
    noecho();
    cbreak();
    curs_set(0);
    timeout(100);

    /*BUCLE PRINCIPAL DEL SERVIDOR*/
    while (mapa_servidor->juego_activo)
    {
        erase();

        colisionProy(mapa_servidor);

        mvprintw(0, 0, "--- SERVIDOR DEL SECTOR ESPACIAL ACTIVO ---");
        mvprintw(1, 0, "Asteroides generados: %d", 20);

        int naves_conectadas = 0;
        for (int i = 0; i < mapa_servidor->cantidad_naves_permitidas; i++)
        {
            if (mapa_servidor->naves[i].activa)
                naves_conectadas++;
        }

        mvprintw(2, 0, "Naves activas en el servidor: %d/%d", naves_conectadas, mapa_servidor->cantidad_naves_permitidas);
        mvprintw(3, 0, "Presione 'q' para apagar el servidor.");

        refresh();

        int tecla = getch();
        if (tecla == 'q')
        {
            mapa_servidor->juego_activo = 0;
        }
    }

    endwin();
    munmap(mapa_servidor, sizeof(MapaEspacial));
    shm_unlink("/mapa_espacial");
    close(shh_fd);
    exit(EXIT_SUCCESS);
}

void generarAsteroide(MapaEspacial *mapa, int cantidad)
{
    int generados = 0;

    while (generados < cantidad)
    {
        int col = rand() % columnas;
        int fila = rand() % filas;

        int x = corIniX + col * celAnch;
        int y = corIniY + fila * celAlt;

        if (x == corIniX && y == corIniY)
        { // si coincide con la cordenada inicial de la nave se ignora
            continue;
        }

        int ocupado = 0;
        for (int i = 0; i < generados; i++)
        {
            if (mapa->asteroides[i].x == x && mapa->asteroides[i].y == y)
            {
                ocupado = 1; // si se genera un asteroide encima de otro, ocupado valdrá 1
                break;
            }
        }

        if (!ocupado)
        { // y si es 1, entonces no generará ese asteroide
            mapa->asteroides[generados].x = x;
            mapa->asteroides[generados].y = y;
            mapa->asteroides[generados].deuterio = rand() % 2;
            mapa->asteroides[generados].mutexio = rand() % 2;
            mapa->asteroides[generados].semaforita = rand() % 2;
            mapa->asteroides[generados].kernelio = rand() % 2;
            mapa->asteroides[generados].activo = 1;
            generados++;
        }
    }
}

void colisionProy(MapaEspacial *mapa)
{
    for (int i = 0; i < mapa->cantidad_naves_permitidas; i++)
    {
        /*SI LA NAVE DISPARA SE EJECUTARÁ ESTO*/
        if (mapa->naves[i].activa && mapa->naves[i].disparo > 0)
        {

            int xProyectil = mapa->naves[i].x;
            int yProyectil = mapa->naves[i].y;
            int dir = mapa->naves[i].disparo;

            if (dir == 1)
            {
                yProyectil -= 2; // arriba
            }
            else if (dir == 2)
            {
                yProyectil += 2; // abajo
            }
            else if (dir == 3)
            {
                xProyectil -= 4; // izquierda
            }
            else if (dir == 4)
            {
                xProyectil += 4; // derecha
            }

            /*LÓGICA DE DAÑO HACIA OTRAS NAVES*/
            for (int k = 0; k < mapa->cantidad_naves_permitidas; k++)
            {
                /*SI LAS CORDENADAS DEL PROYECTIL COINCIDE CON LA OTRA NAVE SE EJECUTARÁ ESTO*/
                if (k != i && mapa->naves[k].activa && mapa->naves[k].x == xProyectil && mapa->naves[k].y == yProyectil)
                {

                    mapa->naves[k].oxigeno -= 20;
                    mapa->naves[k].combustible -= 20;

                    if (mapa->naves[k].oxigeno < 0)
                    {
                        mapa->naves[k].oxigeno = 0;
                    }
                    if (mapa->naves[k].combustible < 0)
                    {
                        mapa->naves[k].combustible = 0;
                    }

                    mapa->naves[i].disparo = 0;
                    break;
                }
            }

            /*LÓGICA DE DAÑO A ASTEROIDES*/
            for (int j = 0; j < MAX_ASTEROIDES_FISICOS; j++)
            {
                /*SI LAS CORDENADAS DEL PROYECTIL COINCIDE CON UN ASTEROIDE SE EJECUTARÁ ESTO*/
                if (mapa->asteroides[j].activo && mapa->asteroides[j].x == xProyectil && mapa->asteroides[j].y == yProyectil)
                {

                    mapa->asteroides[j].activo = 0;

                    /*RECOMPENSAS DEL ASTEROIDE*/
                    mapa->naves[i].mutexio += mapa->asteroides[j].mutexio;
                    mapa->naves[i].semaforita += mapa->asteroides[j].semaforita;
                    mapa->naves[i].kernelio += mapa->asteroides[j].kernelio;
                    mapa->naves[i].deuterio += mapa->asteroides[j].deuterio;

                    mapa->naves[i].disparo = 0;
                    break;
                }
            }
        }
    }
}

void generarEstacion(MapaEspacial *mapa, int cantidad)
{
    int generadas = 0;

    while (generadas < cantidad)
    {
        int col = rand() % columnas;
        int fila = rand() % filas;

        int x = corIniX + col * celAnch;
        int y = corIniY + fila * celAlt;

        int ocupado = 0;

        /*LÓGICA DE GENERACIÓN DE LA ESTACIÓN*/
        for (int i = 0; i < MAX_ASTEROIDES_FISICOS; i++)
        {
            /*SI LAS CORDENADAS DE LA ESTACIÓN COINCIDE CON ALGUNA ASTEROIDE EJECUTARÁ LO SIGUIENTE*/
            if (mapa->asteroides[i].activo && mapa->asteroides[i].x == x && mapa->asteroides[i].y == y)
            {
                ocupado = 1;
                break;
            }
        }

        /*Verificar que no caiga sobre otra estacion ya generada*/
        for (int i = 0; i < generadas; i++)
        {
            if (mapa->estaciones[i].activa && mapa->estaciones[i].x == x && mapa->estaciones[i].y == y)
            {
                ocupado = 1;
                break;
            }
        }

        /*Si el lugar esta libre para la estacion, la guardamos en un arreglo*/
        if (!ocupado)
        {
            mapa->estaciones[generadas].x = x;
            mapa->estaciones[generadas].y = y;
            mapa->estaciones[generadas].activa = 1;
            generadas++;
        }
    }
}