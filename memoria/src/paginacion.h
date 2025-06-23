#ifndef PAGINACION_H
#define PAGINACION_H

#include <stdint.h>
#include <stdbool.h>
#include "stdio.h"
#include "stdlib.h"
#include "unistd.h"
#include "stdint.h"
#include "stdbool.h"
#include "memoria_config.h"
#include "commons/bitarray.h"
#include "../../utils/include/utils.h"
#include "../../utils/include/utilsHandshake.h"
#include "memoria_ctrl.h"

typedef struct
{
    uint32_t num_pagina;
    uint32_t marco;
    bool esta_presente;
} t_pagina;

typedef struct
{
    void** entradas;
    bool es_ultimo_nivel;
} t_tabla_pagina;

typedef struct {
    op_code codigo_operacion; 
    t_buffer* buffer;
} t_paquete_resp;

t_tabla_pagina* crear_tabla(uint32_t nivel_actual, uint32_t* paginas_restantes);
int buscar_marco(t_tabla_pagina* tabla,uint32_t* entradas, uint32_t nivel_actual);
void eliminar_tabla(t_tabla_pagina*);

t_paquete_resp* crear_paquete_simple(op_code code, void* data, uint32_t data_size);

void destruir_paquete_resp(t_paquete_resp* paquete);

#endif
