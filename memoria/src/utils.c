#include "utils.h"

char* recibir_string(t_buffer* payload) {
    uint32_t largo = recibir_uint32(payload);
    char* cadena = malloc(largo+1);
    memcpy(cadena, payload->stream, largo);
    cadena[largo] = '\0';
    payload->stream += largo;

    return cadena;
}

uint32_t recibir_uint32(t_buffer* payload) {
    uint32_t valor;
    memcpy(&(valor), payload->stream, sizeof(uint32_t));
    payload->stream += sizeof(uint32_t);
    return valor;
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

