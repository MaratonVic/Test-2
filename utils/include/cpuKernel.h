#ifndef UTILS_CPUKERNEL_H_
#define UTILS_CPUKERNEL_H_

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
    uint32_t pid;
    uint32_t pc;
} t_package_kernel_cpu;

typedef struct{
    char* nombre;
    uint32_t nombreLargo;
    uint32_t pid;
}t_package_syscall_exit;

t_paquete* crear_paquete_handshake_kernel_cpu(uint8_t codigo_operacion, uint32_t pid, uint32_t pc);
t_paquete* crear_paquete_syscall_exit(uint8_t codigo_operacion, char* nombre, uint32_t pid);
void* serializar_paquete_syscall_exit(t_paquete* paquete, uint32_t* bytes_out);
t_package_syscall_exit* deserializar_sycall_exit(t_buffer* buffer);
void destruir_paquete_syscall_exit(t_paquete* paquete);
//t_paquete* crear_paquete_handshake_syscall_cpu_io(char* nombre_modulo, uint8_t codigo_operacion, uint8_t syscall, uint32_t timer);
void* serializar_paquete_kernel_cpu(t_paquete* paquete, uint32_t* total_size_out);
void destruir_paquete_kernel_cpu(t_paquete* paquete);

t_package_kernel_cpu* deserializar_kernel_cpu(t_buffer* buffer);

#endif 