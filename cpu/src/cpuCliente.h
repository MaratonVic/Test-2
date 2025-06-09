#ifndef CPU_CLIENTE_H
#define CPU_CLIENTE_H

#include <stdint.h>
#include <utilsHandshake.h>
#include "utils.h"
#include "cpuIO.h"

void enviar_handshake_cpu(int socket);
void enviar_handshake_cpu_io(int socket);
void enviar_handshake_cpu_io_teclado(int socket);
void enviar_syscall_exit(int socket);
void enviar_syscall_dump(int socket);

#endif