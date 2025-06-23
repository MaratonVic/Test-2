#ifndef MEMORIA_KERNEL_H
#define MEMORIA_KERNEL_H

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
#include "utils.h"

t_paquete* crear_paquete_dump_memoria(uint8_t codigo_operacion, uint32_t pid);
uint32_t deserializar_tam_memoria(t_buffer* buffer);
void enviar_paquete_dump(int socket, uint32_t pid);
uint32_t deserializar_syscall_dump_pid(t_buffer* buffer);
void enviar_paquete_susp(int socket, uint32_t pid);

#endif