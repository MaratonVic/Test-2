#ifndef PROCESOS_H_
#define PROCESOS_H_

#include "commons/collections/list.h"
#include <commons/collections/dictionary.h>
#include <stdlib.h>
#include <unistd.h>
#include "../../utils/include/utils.h"
#include "../../utils/include/utilsHandshake.h"
#include "utils.h"
#include <stdio.h>
#include <string.h>
#include "memoria_config.h"
#include "memoria_ctrl.h"
#include "memoria_verif_dispo.h"

typedef struct {
    char* archivo;
    t_list* instrucciones;
    t_dictionary* metricas;
    uint32_t pid;
    uint32_t tam_proceso;
} t_proceso_mem;

void iniciar_lista_procesos();
void iniciar_proceso_en_memoria(t_buffer*);
t_proceso_mem* proceso_mem_create(void);
void cargar_instrucciones_proceso(t_proceso_mem* proceso);
t_proceso_mem* buscar_proceso_por_pid(t_list* , uint32_t );
void deserializar_instruccion(t_buffer* payload,uint32_t* pid, uint32_t* pc);
char* obtener_instruccion(uint32_t pid, uint32_t pc);

#endif