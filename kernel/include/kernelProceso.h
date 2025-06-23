#ifndef KERNEL_PROCESO_H
#define KERNEL_PROCESO_H

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
#include "kernelCliente.h"  

void destruir_valor_pcb(void* pcb_void);
void destruir_proceso_en_new(void* proceso_void);
void imprimir_mestado(t_pcb* pcb, t_log* logger);
void imprimir_clave_valor_mestado(char* clave, void* valor);

#endif