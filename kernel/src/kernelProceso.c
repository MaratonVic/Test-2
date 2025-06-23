#include "kernelProceso.h"


void destruir_valor_pcb(void* pcb_void) {
    t_pcb* pcb = (t_pcb*) pcb_void;

    if (pcb->mestado) {
        dictionary_destroy_and_destroy_elements(pcb->mestado, free);
    }

    if (pcb->mtiempo) {
        dictionary_destroy_and_destroy_elements(pcb->mtiempo, free);
    }

    free(pcb);
}


void destruir_proceso_en_new(void* proceso_void) {
    t_proceso_en_new* proceso = (t_proceso_en_new*) proceso_void;

    if (proceso->pcb)
        destruir_valor_pcb(proceso->pcb);

    if (proceso->pathArchivo)
        free(proceso->pathArchivo);

    free(proceso);
}

void imprimir_mestado(t_pcb* pcb, t_log* logger) {
    if (pcb == NULL || pcb->mestado == NULL || pcb->mtiempo == NULL) {
        log_error(logger, "El PCB, mestado o mtiempo es NULL");
        return;
    }

    const char* claves_esperadas[] = {
        "NEW", "READY", "EXEC", "BLOCKED", "SUSP_READY", "SUSP_BLOCKED", "EXIT"
    };
    const int cant_claves = sizeof(claves_esperadas) / sizeof(claves_esperadas[0]);

    char buffer[1024] = {0};
    snprintf(buffer, sizeof(buffer), "## PID (%d)  Métricas de estado:", pcb->pid);

    for (int i = 0; i < cant_claves - 1; i++) {
        const char* estado = claves_esperadas[i];

        int* contador = dictionary_get(pcb->mestado, (char*)estado);
        t_temporal* tiempo_ms = dictionary_get(pcb->mtiempo, (char*)estado);

        int veces = (contador != NULL) ? *contador : 0;
        double tiempo = 0;

        if (tiempo_ms != NULL)
            tiempo = temporal_gettime(tiempo_ms);

        char entrada[128];
        snprintf(entrada, sizeof(entrada), " %s (%d veces, %lf s)", estado, veces, tiempo/1000);
        strncat(buffer, entrada, sizeof(buffer) - strlen(buffer) - 1);

        if (i < cant_claves - 1) {
            strncat(buffer, ",", sizeof(buffer) - strlen(buffer) - 1);
        }
    }

    log_info(logger, "%s", buffer);
}