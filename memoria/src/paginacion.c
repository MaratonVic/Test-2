#include "paginacion.h"

// static t_bitarray* memoria_bitarray;
// static pthread_mutex_t MUTEX_MEM_BITARRAY;

t_tabla_pagina* crear_tabla(uint32_t nivel_actual, uint32_t* paginas_restantes) {
    t_tabla_pagina* tabla = malloc(sizeof(t_tabla_pagina));
    tabla->es_ultimo_nivel = (nivel_actual == cfg.cantidad_niveles);
    tabla->entradas = malloc(sizeof(void*) * cfg.entradas_por_tabla);

    for (int i = 0; i < cfg.entradas_por_tabla; i++) {
        if (*paginas_restantes == 0) {
            tabla->entradas[i] = NULL;
            continue;
        }

        if (tabla->es_ultimo_nivel) {
            t_pagina* pag = malloc(sizeof(t_pagina));
            pag->esta_presente = false;
            pag->marco = asignar_marco_libre();
            tabla->entradas[i] = pag;
            (*paginas_restantes)--;
        } else {
            tabla->entradas[i] = crear_tabla(nivel_actual + 1, paginas_restantes);
        }
    }

    return tabla;
}

void eliminar_tabla(t_tabla_pagina* tabla) {
    if (tabla == NULL) return;

    for (int i = 0; i < cfg.entradas_por_tabla; i++) {
        if (tabla->entradas[i] == NULL) continue;

        if (tabla->es_ultimo_nivel) {
            t_pagina* pagina = (t_pagina*)tabla->entradas[i];
            free(pagina);
        } else {
            t_tabla_pagina* subtabla = (t_tabla_pagina*)tabla->entradas[i];
            eliminar_tabla(subtabla);
        }
    }

    free(tabla->entradas);
    free(tabla);
}

int buscar_marco(t_tabla_pagina* tabla,uint32_t* entradas, uint32_t nivel_actual) {
    if(tabla->es_ultimo_nivel) {
        t_pagina* pagina_buscada = (t_pagina *) tabla->entradas[entradas[nivel_actual]];
        if(pagina_buscada == NULL){
            log_error(logger, "La entrada %d, en el nivel %d no es valdia", entradas[nivel_actual], nivel_actual + 1);
            return -1;
        }
        if(pagina_buscada->esta_presente) {
            return pagina_buscada->marco;
        }
        else {
            //ACA PRODRIA IR EL MANEJO DE PAGE FAULT
            log_error(logger, "La pagina no esta presente");
            return -1;
        }
    }
    else {
        t_tabla_pagina* siguiente_nivel = (t_tabla_pagina *) tabla->entradas[entradas[nivel_actual]];
        if(siguiente_nivel == NULL) {
            log_error(logger, "La entrada %d, en el nivel %d no es valdia", entradas[nivel_actual], nivel_actual + 1);
            return -1;
        }
        return buscar_marco(siguiente_nivel, entradas, nivel_actual + 1);
    }
}

t_paquete_resp* crear_paquete_simple(op_code code, void* data, uint32_t data_size) {
    t_paquete_resp* paquete = malloc(sizeof(t_paquete_resp));
    paquete->codigo_operacion = code;

    paquete->buffer = malloc(sizeof(t_buffer));
    paquete->buffer->size   = data_size;
    paquete->buffer->stream = malloc(data_size);
    memcpy(paquete->buffer->stream, data, data_size);

    return paquete;
}

void destruir_paquete_resp(t_paquete_resp* paquete) {
    if (!paquete) return;
    if (paquete->buffer) {
        free(paquete->buffer->stream);
        free(paquete->buffer);
    }
    free(paquete);
}


// void iniciar_memoria_bitarray(int tamanio, int tamanio_frame){
//     pthread_mutex_init(&MUTEX_MEM_BITARRAY, 0);
//     int total_frames = (tamanio / tamanio_frame);
//     int total_frames_en_bytes = ceil((double) total_frames /8.0);
//     char* bitarray = calloc(total_frames_en_bytes, sizeof(char));
//     memoria_bitarray = bitarray_create_with_mode(bitarray, total_frames_en_bytes, LSB_FIRST);
// }


