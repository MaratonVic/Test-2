#ifndef UTILS_HANDSHAKE_H_
#define UTILS_HANDSHAKE_H_

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

typedef enum{
    CPU,
    KERNEL,
    IO
} t_modulos;

typedef struct{
    char* nombre;
    uint32_t nombreLargo;
    uint8_t modulo;
} t_package;

typedef struct{
    char* nombre;
    uint32_t nombreLargo;
    uint8_t modulo;
    uint32_t pid;
    uint32_t tamanio;
} t_package_pseudocodigo;

typedef struct {
    uint32_t size;
    uint32_t offset;
    void* stream;
} t_buffer;

typedef struct {
    uint8_t codigo_operacion; 
    t_buffer* buffer;
} t_paquete;

t_paquete* crear_paquete_handshake(char* nombre_modulo, uint8_t codigo_operacion, uint8_t modulo);
t_paquete* crear_paquete_handshake_pseudocodigo(char* pathArchivo, uint8_t codigo_operacion, uint8_t modulo, uint32_t pid, uint32_t tamanio);
void* serializar_paquete(t_paquete* paquete, uint32_t* total_size_out);
void destruir_paquete(t_paquete* paquete);
int enviar_paquete(int socket, t_paquete* paquete);

t_paquete* recibir_paquete(int socket);
t_package* deserializar_nombre(t_buffer* buffer);

t_package_pseudocodigo* deserializar_pseudocodigo(t_buffer* buffer);

#endif 