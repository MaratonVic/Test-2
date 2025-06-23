#ifndef KERNEL_H
#define KERNEL_H

#include <commons/collections/list.h>
#include <commons/collections/dictionary.h>

#include <stdlib.h>
#include "kernelConfig.h"
#include "kernelServer.h"
#include "kernelCliente.h"
#include "kernelLargoPlazo.h"
#include "utilsHandshake.h"
#include "utils.h"
#include "pcb.h"
#include <semaphore.h>
#include "cpuKernel.h"
#include "kernelMemoria.h"


#include <stdlib.h>
#include <time.h>

typedef struct {
    int fd_dispatch;
    int fd_interrupt;
    char* nombre_cpu;
    bool disponible;
    t_pcb* proceso_ejecutando;
} t_cpu_conectada;

extern t_list* cpus_conectadas;
extern pthread_mutex_t mutex_cpu_conectadas;
extern t_list* ready;
extern pthread_mutex_t mutex_cola_ready;
extern t_list* new;
extern pthread_mutex_t mutex_cola_new;
extern t_list* blocked;
extern pthread_mutex_t mutex_cola_blocked;
extern t_list* exec;
extern pthread_mutex_t mutex_cola_exec;
extern t_list* suspReady;
extern pthread_mutex_t mutex_cola_suspReady;
extern t_list* suspBlocked;
extern pthread_mutex_t mutex_cola_suspBlocked;

extern t_dictionary* diccionario_io;  
extern pthread_mutex_t mutex_diccionario_io;

extern uint32_t pid_procesos; 
extern pthread_mutex_t mutex_pid_procesos;

extern uint32_t pid_bloqueo; 
extern pthread_mutex_t mutex_pid_bloqueo;

extern uint32_t pid_susBloqueo; 
extern pthread_mutex_t mutex_pid_susBloqueo;

extern uint32_t pid_dump; 
extern pthread_mutex_t mutex_pid_dump;

extern int socketMemoriaSyscall;

extern sem_t sem_procesos_en_new;
extern sem_t sem_procesos_en_ready;
extern sem_t sem_procesos_en_blocked;
extern sem_t sem_procesos_en_suspReady;
extern sem_t sem_bloqueado;
extern sem_t sem_exit;
extern sem_t sem_ioTerminada;
extern sem_t sem_dumpTerminada;

extern t_kernelConfig cfg;
extern t_log* loggerKernel;

void iniciarKernel(int argc, char* argv[]);
void inicializar_estructuras();
void destruir_estructuras();
t_cpu_conectada* buscar_cpu_por_nombre(char* nombre);

#endif