#ifndef CPU_MMU_H
#define CPU_MMU_H


#include <stdint.h> 
#include <commons/log.h>                 
#include <stdlib.h>
#include <string.h>
#include "../../utils/include/utils.h"
#include "../../utils/include/utilsHandshake.h"


typedef struct {
    uint32_t tam_pagina;
    uint32_t entradas_por_tabla;
    uint32_t cantidad_niveles;
} t_mem_config_msg;


typedef struct {
    op_code   codigo_operacion;
    t_buffer* buffer;
} t_paquete_mmu;


void mmu_init(int sock_memoria, t_mem_config_msg* config_msj, t_log* logger);
int mmu_traducir(uint32_t dir_logica, int pid);
t_paquete_mmu* mmu_crear_paquete_indices(int pid, uint32_t *indices, uint32_t niveles);
int mmu_extraer_frame(t_paquete_mmu* paquete);
void mmu_destruir_paquete(t_paquete_mmu* paquete);

#endif 

