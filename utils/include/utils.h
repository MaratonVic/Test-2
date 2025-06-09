#ifndef UTILS_H_ 
#define UTILS_H_

#include<stdio.h>
#include<stdlib.h>
#include<sys/socket.h>
#include<signal.h>
#include<unistd.h>
#include<netdb.h>
#include<string.h>
#include<commons/log.h>
#include<commons/config.h>
#include<commons/string.h>
 
 typedef enum {
    MENSAJE,
    PAQUETE,
    INICIARIO,
    PETICIONAIO,
    DESPERTOIO,
    HANDSHAKE,
    INIT_PROC,
    INIT_IO,
    INIT_DUMP_MEMORY,
    INIT_EXIT,
    SOLICITAR_INSTRUCCION,
    INSTRUCCION_SOLICITADA
 }op_code;
 

int crear_conexion(char* ip, char* puerto);

int iniciar_servidor(char* puerto, t_log* logger);

int esperar_cliente(int socket_servidor);




#endif 
