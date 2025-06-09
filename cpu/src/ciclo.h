#ifndef CICLO_H
#define CICLO_H

#include <stdint.h>
#include "instrucciones.h"
#include "cpuCliente.h"
#include "utils.h"

typedef enum {
    CODIGO_WRITE = 1,
    CODIGO_READ  = 2,
} codigos_memoria;

bool ciclo_instruccion(uint32_t pid, uint32_t* pc);
char* fetch(uint32_t pid, uint32_t pc);
t_instruccion* decode(const char* linea);
bool execute(t_instruccion* instruccion, uint32_t pid, uint32_t* pc);
void enviar_escritura_a_memoria(uint32_t pid, uint32_t direccion, const char* valor);
void solicitar_lectura_a_memoria(uint32_t pid, uint32_t direccion);
void enviar_syscall_io_al_kernel(uint32_t pid, const char* dispositivo, uint32_t tiempo);
void enviar_syscall_exit_al_kernel(uint32_t pid); 
void enviar_syscall_init_proc(int socket, uint32_t pid);

#endif