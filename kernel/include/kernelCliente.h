#ifndef KERNEL_CLIENTE_H
#define KERNEL_CLIENTE_H

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
#include "cpuIO.h"
#include "utilsHandshake.h"
#include "utils.h"
#include "kernelServer.h"

int conectar_cliente(char* ip,char* puerto, t_log* logger,char* nombre); 
int conectar_a_memoria(char* ip, char* puerto, t_log* logger);
void identificar_a_memoria(int socket);
void* enviar_handshake_kernel_io(void* arg);
void* enviar_pseudocodigo_a_memoria(void* arg);

#endif

