#include "cpu_mmu.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <sys/socket.h>

static int sock_memoria = -1;
static t_mem_config_msg cfg_mmu;
static t_log* logger_cpu;

void mmu_init(int sock, t_mem_config_msg* config_msj, t_log* logger) {
    sock_memoria = sock;
    cfg_mmu = *config_msj;
    logger_cpu = logger;
    log_info(logger_cpu,
      "MMU init: tam_pagina=%u, niveles=%u, entradas_por_tabla=%u",
      cfg_mmu.tam_pagina,
      cfg_mmu.cantidad_niveles,
      cfg_mmu.entradas_por_tabla);
}

t_paquete_mmu* mmu_crear_paquete_indices(int pid, uint32_t *indices, uint32_t niveles) {
    t_paquete_mmu* paquete = malloc(sizeof(t_paquete_mmu));
    if (!paquete) return NULL;
    serializar_pedido_marco(paquete, pid, indices, niveles);
    return paquete;
}

void serializar_pedido_marco(t_paquete_mmu* paquete, int pid, uint32_t *indices, uint32_t niveles) {
    paquete->codigo_operacion = ACCESO_A_PAGINA;
    paquete->buffer = malloc(sizeof(t_buffer));
    paquete->buffer->size = sizeof(int) + sizeof(uint32_t) * niveles;
    paquete->buffer->stream = malloc(paquete->buffer->size);

    int offset = 0;
    memcpy(paquete->buffer->stream + offset, &pid, sizeof(int));
    offset += sizeof(int);
    memcpy(paquete->buffer->stream + offset, indices, sizeof(uint32_t) * niveles);
}

int mmu_extraer_frame(t_paquete_mmu* paquete) {
    int frame;
    memcpy(&frame, paquete->buffer->stream, sizeof(int));
    return frame;
}

void mmu_destruir_paquete(t_paquete_mmu* paquete) {
    if (!paquete) return;
    if (paquete->buffer) {
        free(paquete->buffer->stream);
        free(paquete->buffer);
    }
    free(paquete);
}

int mmu_traducir(uint32_t dir_logica, int pid) {
    if (sock_memoria < 0) {
        log_error(logger_cpu, "MMU no inicializada");
        return -1;
    }
    uint32_t n      = cfg_mmu.cantidad_niveles;
    uint32_t page   = dir_logica / cfg_mmu.tam_pagina;
    uint32_t offset = dir_logica % cfg_mmu.tam_pagina;
    uint32_t indices[n];
    
    // Calcular índices según la fórmula:
    // nro_página = floor(direccion_logica/tam_pagina)
    // entrada_nivel_X = floor(nro_pagina / (entradas_por_tabla^(N - X))) % entradas_por_tabla
    for (uint32_t lvl = 0; lvl < n; lvl++) {
        uint32_t exp = n - lvl - 1;
        uint32_t div = (uint32_t) pow(cfg_mmu.entradas_por_tabla, exp);
        indices[lvl] = (page / div) % cfg_mmu.entradas_por_tabla;
    }
    
    t_paquete_mmu* req = mmu_crear_paquete_indices(pid, indices, n);
    if (!req) {
        log_error(logger_cpu, "Error al crear el paquete para ACCESO_A_PAGINA");
        return -1;
    }
    
    uint32_t total_size = 1 + sizeof(uint32_t) + req->buffer->size;
    void* msg = malloc(total_size);
    int off = 0;
    memcpy(msg + off, &req->codigo_operacion, 1); off += 1;
    memcpy(msg + off, &req->buffer->size, sizeof(uint32_t)); off += sizeof(uint32_t);
    memcpy(msg + off, req->buffer->stream, req->buffer->size);
    
    if (send(sock_memoria, msg, total_size, 0) != total_size) {
        log_error(logger_cpu, "MMU: fallo al enviar ACCESO_A_PAGINA");
        free(msg);
        mmu_destruir_paquete(req);
        return -1;
    }
    free(msg);
    mmu_destruir_paquete(req);
    
    t_paquete_mmu* resp = malloc(sizeof(t_paquete_mmu));
    if (!resp) return -1;
    
    if (recv(sock_memoria, &resp->codigo_operacion, 1, MSG_WAITALL) <= 0) goto err;
    
    uint32_t sz;
    if (recv(sock_memoria, &sz, sizeof(uint32_t), MSG_WAITALL) <= 0) goto err;
    
    resp->buffer = malloc(sizeof(t_buffer));
    resp->buffer->size = sz;
    resp->buffer->stream = malloc(sz);
    if (recv(sock_memoria, resp->buffer->stream, sz, MSG_WAITALL) <= 0) goto err;
    
    if (resp->codigo_operacion != RESPUESTA_MARCO) goto err;
    
    int frame = mmu_extraer_frame(resp);
    mmu_destruir_paquete(resp);
    
    log_info(logger_cpu,
      "MMU: pag %u -> frame %d, dir_fisica=%u",
      page, frame, frame * cfg_mmu.tam_pagina + offset);
    
    return frame * cfg_mmu.tam_pagina + offset;
    
err:
    log_error(logger_cpu, "MMU: error recibiendo RESPUESTA_MARCO");
    if (resp) {
        if (resp->buffer) {
            free(resp->buffer->stream);
            free(resp->buffer);
        }
        free(resp);
    }
    return -1;
}


