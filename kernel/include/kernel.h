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

typedef struct {
    int fd_dispatch;
    int fd_interrupt;
    char* nombre_cpu;
    bool disponible;
    t_pcb* proceso_ejecutando;
} t_cpu_conectada;

extern t_list* cpus_conectadas;
extern pthread_mutex_t mutex_cpu_conectadas;
extern t_queue* ready;
extern pthread_mutex_t mutex_cola_ready;
extern t_queue* new;
extern pthread_mutex_t mutex_cola_new;
extern t_queue* blocked;
extern pthread_mutex_t mutex_cola_blocked;
extern t_queue* exec;
extern pthread_mutex_t mutex_cola_exec;
extern t_queue* suspReady;
extern pthread_mutex_t mutex_cola_suspReady;
extern t_queue* suspBlocked;
extern pthread_mutex_t mutex_cola_suspBlocked;

extern t_dictionary* diccionario_io;  
extern pthread_mutex_t mutex_diccionario_io;

extern uint32_t pid_procesos; 
extern pthread_mutex_t mutex_pid_procesos;

extern int socketMemoriaSyscall;

extern sem_t sem_procesos_en_new;
extern sem_t sem_procesos_en_ready;
extern sem_t sem_procesos_en_blocked;
extern sem_t sem_procesos_en_suspReady;
extern sem_t sem_bloqueado;
extern sem_t sem_exit;


void iniciarKernel(int argc, char* argv[]);
void inicializar_estructuras();
void destruir_estructuras();
t_cpu_conectada* buscar_cpu_por_nombre(char* nombre);

#endif