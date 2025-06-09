#ifndef KERNEL_LARGO_PLAZO_H
#define KERNEL_LARGO_PLAZO_H

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
#include <readline/readline.h>
#include <commons/collections/queue.h>
#include "kernel.h"
#include "pcb.h"

typedef struct {
    int socket;
    uint32_t pid;
    char* pathArchivo;
    uint32_t tamanio;
    t_log* logger;
} t_args_pseudocodigo;
typedef struct {
    int socket;
    t_kernelConfig cfg;
    t_log* logger;
    t_args_pseudocodigo pseudocodigo; 
} t_args_planificador;

typedef struct {
    int socket;
    t_kernelConfig cfg;
    t_log* logger;
} t_args_planificador_corto;



void* planificarLargoPlazo(void* args);
void* planificarCortoPlazo(void* args);
void* planificarMedianoPlazo(void* args);
void* tiempoBloqueado(void* args);
void* enviar_handshake_kernel_cpu(int socket, t_pcb* pcb, t_log* logger);
void leerConsola(t_log* logger);
#endif