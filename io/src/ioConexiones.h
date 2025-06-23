#ifndef IO_CONEXIONES_H
#define IO_CONEXIONES_H

#include<stdio.h>
#include<stdlib.h>
#include<commons/log.h>
#include<commons/string.h>
#include<commons/config.h>
#include<sys/socket.h>
#include<unistd.h>
#include<netdb.h>
#include<string.h>
#include <pthread.h>

int conectar_cliente(char* ip,char* puerto, t_log* logger,char* nombre); 
int conectar_a_kernel(char* ip, char* puerto, t_log* logger);

#endif