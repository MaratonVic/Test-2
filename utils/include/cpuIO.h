#ifndef UTILS_CPUIO_H_
#define UTILS_CPUIO_H_

#include<stdio.h>
#include<stdlib.h>
#include "commons/log.h"
#include "commons/config.h"
#include <unistd.h>
#include <pthread.h>
#include<commons/string.h>
#include<sys/socket.h>
#include<netdb.h>
#include <string.h>
#include "utilsHandshake.h"


typedef struct{
    char* nombre;
    uint32_t nombreLargo;
    uint32_t pid;
    uint8_t syscall;
    uint32_t timer;
    char* nombreCpu;
    uint32_t nombreLargoCpu;
} t_package_io;

extern uint32_t pid_io;
extern pthread_mutex_t mutex_pid_io;

t_paquete* crear_paquete_handshake_syscall_cpu_io_prueba(char* nombre_modulo, uint8_t codigo_operacion,  uint8_t syscall, uint32_t timer, uint32_t pid);
t_paquete* crear_paquete_handshake_syscall_cpu_io(char* nombre_modulo, uint8_t codigo_operacion,  uint8_t syscall, uint32_t timer);
void* serializar_paquete_syscall_cpu_io(t_paquete* paquete, uint32_t* total_size_out);
void destruir_paquete_syscall_cpu_io(t_paquete* paquete);
t_package_io* deserializar_nombre_cpu_io_kernel(t_buffer* buffer);

t_package_io* deserializar_nombre_cpu_io(t_buffer* buffer);

#endif 