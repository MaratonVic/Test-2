#ifndef KERNEL_SERVER_H
#define KERNEL_SERVER_H

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
#include "utilsHandshake.h"
#include "kernel.h"
#include "kernelCliente.h"
#include <commons/collections/queue.h>
#include "cpuIO.h"
#include "kernelSyscall.h"
#include "pcb.h"
#include "kernel.h" 

typedef struct {
    int socket_servidor;
    t_log* logger;
} t_args_escucha;

typedef struct {
    int socket_fd;
    bool disponible;
} t_instancia_io;

typedef struct {
    t_list* instancias;      
    t_queue* cola_bloqueados;   
    bool ocupado;               
} t_io;

typedef struct {
    int socket_servidor;
    char* nombre;
    uint32_t timer;
    t_log* logger;
    t_io* t_dispositivo;
    uint32_t pid;
    char* nombreCpu;
} t_args_kernel_io;

typedef struct 
{
    uint32_t pid;
    uint32_t timer;
}t_dispositivo_io;


char* recibirNombre(int fd_cliente, t_log* logger);
char* recibirNombreIO(int fd_cliente, t_log* logger);
int levantar_socket_servidor(char* puerto, t_log* logger);
void* escuchar_dispatch(void* arg);
void* escuchar_interrupt(void* arg);
void* escuchar_IO(void* arg);
void iniciar_hilos_conexiones(int socket_dispatch, int socket_interrupt, int socket_IO, t_log* logger);
void registrarInstanciaIo(char* nombre, int socket_fd, t_log* logger);
void destruir_diccionario_io();
void mostrar_diccionario_io(t_log* logger);

#endif