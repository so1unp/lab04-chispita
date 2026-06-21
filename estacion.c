//libreas que use yo
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <mqueue.h>
#include <fcntl.h>
#include <sys/stat.h>
//esto es el header que creamos nosotros 
#include "compartido.h"
//esto es la constante que define los permisos de la cola 
#define QUEUE_PERMISSIONS 0666

//descriptores de mi colas cada uno tiene un uso distinto
mqd_t cola_ypf;
mqd_t cola_respuesta;


//primer hilo de la estacion 
void* hilo_consumo(void* arg) {
    EstadoYPF* estado = (EstadoYPF*) arg;
   while(estado->corriendo) {
     sleep(5);
     pthread_mutex_lock(&estado->mutex_nafta);
     if(estado->nafta > 0) {
        estado->nafta--;
     }
     if(estado->nafta <= 0) {
        estado->corriendo = 0;
     }
     pthread_mutex_unlock(&estado->mutex_nafta);
   }
    return NULL;
}


void* hilo_acciones(void* arg) {
    EstadoYPF* estado = (EstadoYPF*) arg;

    MensajeNave msj; //variable para recibir el mensaje de nave.c 
    RespuestaEstacion resp; //variable para enviar la respuesta
    unsigned int prioridad_Del_mensaje; //variable para recibir la prioridad del mensaje
    
    //abro la cola de mensaje   "primer cola"
    mqd_t cola_ypf = mq_open(COLA_ESTACION, O_RDONLY);  
    //esto no se si era necesario pero por las duas lo pongo
     if (cola_ypf == (mqd_t)-1) {
        perror("Error no se puede abrir la colaaaaaaaaa!!!!!");
        pthread_exit(NULL);
    }

    // segunda cola esta la cree para poder responder con 2 colas separadas 
    mqd_t cola_respuesta = mq_open(COLA_RESPUESTA, O_WRONLY);
    //verifico que se abrio correctamente
    if (cola_respuesta == (mqd_t)-1) {
        perror("Error no se puede abrir la cola de respuesta!!!!!!!!!!");
        mq_close(cola_ypf);
        pthread_exit(NULL);
    }

  
    while (estado->corriendo) {

        //hago un recive de la cola de mensajea
        if (mq_receive(cola_ypf, (char *)&msj, sizeof(MensajeNave), &prioridad_Del_mensaje) != -1) {
           resp.tipo = msj.tipo_operacion;
           //ahora depende del tipo de operacion que sea es la respuesta que le voy a mandar
           //por ahora solo lo dejo en 0
           resp.cantidad = 0;

           switch (msj.tipo_operacion) {

                case 1:
                    pthread_mutex_lock(&estado->mutex_nafta);
                    if (estado->deuterio >= 5 && estado->nafta >= 10) {
                        estado->deuterio -= 5;
                        estado->nafta -= 10;
                        estado->recolector1++;
                        estado->nafta_jugador += 10;
                        resp.cantidad = 10;
                    }
                    pthread_mutex_unlock(&estado->mutex_nafta);
                    break;

                case 2:
                    pthread_mutex_lock(&estado->mutex_oxigeno);
                    if (estado->mutexio >= 1 &&
                        estado->semaforita >= 1 &&
                        estado->kernelio >= 1 &&
                        estado->oxigeno >= 10) {
                        estado->mutexio--;
                        estado->semaforita--;
                        estado->kernelio--;
                        estado->recolector0 += 3;
                        estado->oxigeno -= 10;
                        estado->oxigeno_jugador += 10;
                        resp.cantidad = 10;
                    }
                    pthread_mutex_unlock(&estado->mutex_oxigeno);
                    break;

                case -1:
                    mq_close(cola_ypf);
                    mq_close(cola_respuesta);
                    pthread_exit(NULL);
            }

            mq_send(cola_respuesta, (char *)&resp, sizeof(RespuestaEstacion), 0);
        }
    }
 //cierro las colas y retorno
    mq_close(cola_ypf);
    mq_close(cola_respuesta);
    return NULL;
}


//funcion del main modificada para que use las colas nuevas esto esta totalmente modificado de las versiones anteriores 
int main(){
    //relleno los valores iniciales de la estacion (si lo dejaba en 0 el hilo_consumo apagaba todo de una)
    EstadoYPF estado = {
        .MAXnaves = 3,
        .corriendo = 1,
        .oxigeno = 100,
        .nafta = 100,
        .deuterio = 100,
        .mutexio = 5,
        .semaforita = 5,
        .kernelio = 5,
        .recolector0 = 0,
        .recolector1 = 0,
        .oxigeno_jugador = 0,
        .nafta_jugador = 0
    };

    //configuro la cola de mensajes para recibir de las naves
    struct mq_attr attr;
    attr.mq_flags = 0;
    attr.mq_maxmsg = 10;
    attr.mq_msgsize = sizeof(MensajeNave);
    attr.mq_curmsgs = 0;
    
    // configuro la cola de mensajes para enviar respuestas
    struct mq_attr attr_respuesta;
    attr_respuesta.mq_flags = 0;
    attr_respuesta.mq_maxmsg = 10;
    attr_respuesta.mq_msgsize = sizeof(RespuestaEstacion);
    attr_respuesta.mq_curmsgs = 0;

    //por si quedo una cola vieja de otra corrida con otro tamaño de mensaje, la borro antes de crear la mia
    mq_unlink(COLA_ESTACION);
    mq_unlink(COLA_RESPUESTA);

    // aca creo las colas de mensajes para que cada una haga su funcion 
    mqd_t cola_ypf = mq_open(COLA_ESTACION, O_CREAT | O_RDWR, QUEUE_PERMISSIONS, &attr);
    if (cola_ypf == (mqd_t)-1) {
        perror("Fallo al crear la cola de estacion");
        exit(1);
    }

    mqd_t cola_respuesta = mq_open(COLA_RESPUESTA, O_CREAT | O_RDWR, QUEUE_PERMISSIONS, &attr_respuesta);
    if (cola_respuesta == (mqd_t)-1) {
        perror("Fallo al crear la cola de respuesta");
        exit(1);
    }

    // Inicializo los mutexes
    pthread_mutex_init(&estado.mutex_nafta, NULL);
    pthread_mutex_init(&estado.mutex_oxigeno, NULL);

    // Creo los dos hilos: uno atiende los mensajes de las naves, el otro gasta nafta con el tiempo
    pthread_t hilo_atencion, hilo_desgaste;
    pthread_create(&hilo_atencion, NULL, hilo_acciones, &estado);
    pthread_create(&hilo_desgaste, NULL, hilo_consumo, &estado);

    // Espero que se acabe la nafta (ahi hilo_consumo pone corriendo en 0 y sale)
    pthread_join(hilo_desgaste, NULL);

    // Aviso al hilo_acciones que tiene que cerrar, mandandole un mensaje con tipo_operacion -1
    estado.corriendo = 0;
    MensajeNave cierre;
    cierre.tipo_operacion = -1;
    mq_send(cola_ypf, (char *)&cierre, sizeof(MensajeNave), 0);

    pthread_join(hilo_atencion, NULL);

    // Cierro colas y destruyo mutexes
    mq_close(cola_ypf);
    mq_close(cola_respuesta);
    mq_unlink(COLA_ESTACION);
    mq_unlink(COLA_RESPUESTA);
    pthread_mutex_destroy(&estado.mutex_nafta);
    pthread_mutex_destroy(&estado.mutex_oxigeno);

    printf("Estacion cerrada correctamente.\n");
    return 0;
}