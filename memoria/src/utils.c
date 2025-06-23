#include "utils.h"

uint32_t recibir_uint32(t_buffer* payload) {
    uint32_t valor;
    memcpy(&valor, payload->stream + payload->offset, sizeof(uint32_t));
    payload->offset += sizeof(uint32_t);
    return valor;
}

char* recibir_string(t_buffer* payload) {
    uint32_t largo = recibir_uint32(payload);

    char* cadena = malloc(largo + 1);
    memcpy(cadena, payload->stream + payload->offset, largo);
    cadena[largo] = '\0';
    payload->offset += largo;

    return cadena;
}

//agrega el path donde estan las instrucciones al nombre del archivo
char* agregar_path_instrucciones(char* path) {
    char* base = strdup(cfg.path_instrucciones);
    string_append(&base,path);
    return base;
}

t_paquete* crear_paquete_instruccion(char* instruccion, uint8_t codigo_operacion) {
    t_paquete* paquete = malloc(sizeof(t_paquete));
    if (!paquete) return NULL;

    t_buffer* buffer = malloc(sizeof(t_buffer));
    if (!buffer) {
        free(paquete);
        return NULL;
    }

    uint32_t longitud = strlen(instruccion) + 1;

    buffer->size = longitud;
    buffer->stream = malloc(longitud);
    if (!buffer->stream) {
        free(buffer);
        free(paquete);
        return NULL;
    }

    memcpy(buffer->stream, instruccion, longitud);

    paquete->codigo_operacion = codigo_operacion;
    paquete->buffer = buffer;

    return paquete;
}

t_buffer* buffer_protocolo_cpu(u_int32_t entradas, u_int32_t tamanio_pag, u_int32_t niveles_tabla) {
    t_buffer* buffer = malloc(sizeof(t_buffer));
    if (!buffer) {
        return NULL;
    }
    uint32_t longitud = sizeof(u_int32_t) * 3;

    buffer->size = longitud;
    buffer->stream = malloc(longitud);
    if (!buffer->stream) {
        free(buffer);
        return NULL;
    }

    int offset = 0;

    memcpy(buffer->stream, &entradas, sizeof(u_int32_t));
    offset += sizeof(u_int32_t);

    memcpy(buffer->stream + offset, &tamanio_pag, sizeof(u_int32_t));
    offset += sizeof(u_int32_t);

    memcpy(buffer->stream + offset, &niveles_tabla, sizeof(u_int32_t));

    return buffer;

}
