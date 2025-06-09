#ifndef PCB_H
#define PCB_H

#include <stdint.h>
#include <commons/collections/list.h>
#include <commons/log.h>
#include <commons/collections/dictionary.h>
#include <stdlib.h>
typedef enum {
    NEW,
    READY,
    EXEC,
    BLOCKED,
    SUSP_READY,
    SUSP_BLOCKED,
    EXIT
} Estados;

typedef struct {
    uint32_t pid;
    uint32_t pc;
    t_dictionary* mestado;
    t_dictionary* mtiempo;
} t_pcb;

typedef struct {
    t_pcb* pcb;
    char* pathArchivo;
    uint32_t tamanio;
    uint8_t estado;
} t_proceso_en_new;

t_pcb* crear_pcb(uint32_t pid);
void enviar_pcb(t_pcb* pcb, int fd, t_log* logger);
void destruir_pcb(t_pcb* pcb);
void* serializar_pcb(t_pcb* pcb, uint32_t* size_out, t_log* logger);
t_pcb* deserializar_pcb(void* buffer, uint32_t size);
void logear_metricas(t_dictionary* dict, const char* tipo, t_log* logger);

#endif