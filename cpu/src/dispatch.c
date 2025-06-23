#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <commons/log.h>
#include <sys/socket.h>
#include "dispatch.h"
#include "cpu.h"

void* manejar_dispatch(void* arg) {

    t_args_dispatch* args = (t_args_dispatch*) arg;
    int socket = args->socket;
    t_log* logger = args->logger;

    while (1) {
        log_info(logger, "[Dispatch] Esperando PCB desde Kernel...");

        t_pcb* pcb = recibir_pcb_serializado(socket);
        if (!pcb) {
            log_error(logger, "[Dispatch] Error al recibir PCB.");
            continue;
        }

        log_info(logger, "[Dispatch] PCB recibido - PID: %d, PC: %d", pcb->pid, pcb->pc);
        logear_metricas(pcb->mestado, "Estado", logger);
        logear_metricas(pcb->mtiempo, "Tiempo", logger);

        destruir_pcb(pcb);
        }

    free(args);
    return NULL;
}

t_pcb* recibir_pcb_serializado(int socket) {
    log_info(logger_cpu, "[Dispatch] Recibiendo PCB por socket FD: %d", socket);
uint32_t size;

    if (recv(socket, &size, sizeof(uint32_t), MSG_WAITALL) <= 0) {
        log_error(logger_cpu, "[Dispatch] Error al recibir tamaño del PCB");
        return NULL;
    }

    void* buffer = malloc(size);
    if (!buffer) {
        log_error(logger_cpu, "[Dispatch] No se pudo alocar buffer de %u bytes", size);
        return NULL;
    }

    if (recv(socket, buffer, size, MSG_WAITALL) <= 0) {
        log_error(logger_cpu, "[Dispatch] Error al recibir el buffer del PCB");
        free(buffer);
        return NULL;
    }

    t_pcb* pcb = deserializar_pcb(buffer, size);
    
    free(buffer);
    return pcb;
}