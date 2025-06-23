// #ifndef IOOPERACIONES_H_
//  #define IOOPERACIONES_H_
 
//  #include <stdio.h>
//  #include <stdlib.h>
//  #include "commons/log.h"
//  #include "commons/config.h"
//  #include <unistd.h>
//  #include <pthread.h>
//  #include "utils.h"
//  #include "sys/socket.h"
 
//  //HANDSHAKE con KERNEL
//  //Se queda recibiendo peticiones, si recibe bien un mensaje es != 1, 
//  int manejar_peticiones(int socket,char*, t_log*);
//  //esta funcion puede ir en otro lugar mas generico, recibe un codigo de operacion, 
//  int recibir_peticion(int socket);
//  //mando el NOMBRE DEL IO que desperto, supongo que kernel deberia tener un diccionario con los modulos de io conectados
//  void avisar_despertar(int socket, char*);
 
 
//  #endif