#ifndef CPU_H
#define CPU_H

#include <commons/log.h>
#include <commons/config.h>
#include "cpuKernel.h"

extern t_log* logger_cpu;
extern t_config* config_cpu;
extern int conexion_memoria;
extern int conexion_kernel_dispatch;
extern int conexion_kernel_interrupt;
extern char* cpu_id;

typedef struct {
    char* ip_memoria;
    char* puerto_memoria;
    char* ip_kernel;
    char* puerto_kernel_dispatch;
    char* puerto_kernel_interrupt;
    int entradas_tlb;
    char* reemplazo_tlb;
    int entradas_cache;
    char* reemplazo_cache;
    int retardo_cache;
} ConfigCPU;


extern ConfigCPU config_cpu_data;

void* hilo_escucha_kernel_cpu(void* arg);
void cerrar_todo();
void cargar_config();

#endif


