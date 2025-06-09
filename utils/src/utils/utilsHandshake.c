#include "utilsHandshake.h"
#include <errno.h>

//send()
t_paquete* crear_paquete_handshake(char* nombre_modulo, uint8_t codigo_operacion, uint8_t modulo) {
    t_package handshake;

    handshake.nombre = string_duplicate(nombre_modulo);
    handshake.nombreLargo = strlen(nombre_modulo) + 1;
    handshake.modulo = modulo;

    t_buffer* buffer = malloc(sizeof(t_buffer));
    buffer->size = sizeof(uint32_t) + handshake.nombreLargo + sizeof(uint8_t);
    buffer->offset = 0;
    buffer->stream = malloc(buffer->size);

    memcpy(buffer->stream + buffer->offset, &handshake.nombreLargo, sizeof(uint32_t));
    buffer->offset += sizeof(uint32_t);

    memcpy(buffer->stream + buffer->offset, handshake.nombre, handshake.nombreLargo);
    buffer->offset += handshake.nombreLargo;

    memcpy(buffer->stream + buffer->offset, &handshake.modulo, sizeof(uint8_t));
    buffer->offset += sizeof(uint8_t);


    t_paquete* paquete = malloc(sizeof(t_paquete));
    paquete->codigo_operacion = codigo_operacion;
    paquete->buffer = buffer;

    free(handshake.nombre);

    return paquete;
}

t_paquete* crear_paquete_handshake_pseudocodigo(char* pathArchivo, uint8_t codigo_operacion, uint8_t modulo, uint32_t pid, uint32_t tamanio) {
    t_package_pseudocodigo handshake;

    handshake.nombre = string_duplicate(pathArchivo);
    handshake.nombreLargo = strlen(pathArchivo) + 1;
    // handshake.modulo = modulo;
    handshake.pid = pid;
    handshake.tamanio = tamanio;

    t_buffer* buffer = malloc(sizeof(t_buffer));
    buffer->size = sizeof(uint32_t) + handshake.nombreLargo + /*sizeof(uint8_t) */+ sizeof(uint32_t) + sizeof(uint32_t);
    buffer->offset = 0;
    buffer->stream = malloc(buffer->size);

    memcpy(buffer->stream + buffer->offset, &handshake.nombreLargo, sizeof(uint32_t));
    buffer->offset += sizeof(uint32_t);

    memcpy(buffer->stream + buffer->offset, handshake.nombre, handshake.nombreLargo);
    buffer->offset += handshake.nombreLargo;

    //memcpy(buffer->stream + buffer->offset, &handshake.modulo, sizeof(uint8_t));
    //buffer->offset += sizeof(uint8_t);

    memcpy(buffer->stream + buffer->offset, &handshake.pid, sizeof(uint32_t));
    buffer->offset += sizeof(uint32_t);

    memcpy(buffer->stream + buffer->offset, &handshake.tamanio, sizeof(uint32_t));
    buffer->offset += sizeof(uint32_t);


    t_paquete* paquete = malloc(sizeof(t_paquete));
    paquete->codigo_operacion = codigo_operacion;
    paquete->buffer = buffer;

    free(handshake.nombre);

    return paquete;
}

t_package_pseudocodigo* deserializar_pseudocodigo(t_buffer* buffer) {
    t_package_pseudocodigo* paquete = malloc(sizeof(t_package_pseudocodigo));
    if (paquete == NULL) return NULL;

    void* stream = buffer->stream;

    memcpy(&(paquete->nombreLargo), stream, sizeof(uint32_t));
    stream += sizeof(uint32_t);

    paquete->nombre = malloc(paquete->nombreLargo + 1);
    memcpy(paquete->nombre, stream, paquete->nombreLargo);
    stream += paquete->nombreLargo;

    // memcpy(&(paquete->modulo), stream, sizeof(uint8_t));
    // stream += sizeof(uint8_t);

    memcpy(&(paquete->pid), stream, sizeof(uint32_t));
    stream += sizeof(uint32_t);

    memcpy(&(paquete->tamanio), stream, sizeof(uint32_t));
    stream += sizeof(uint32_t);

    return paquete;
}

void* serializar_paquete(t_paquete* paquete, uint32_t* total_size_out) {
    uint32_t total_size = sizeof(uint8_t) + sizeof(uint32_t) + paquete->buffer->size;
    void* a_enviar = malloc(total_size);
    int offset = 0;

    memcpy(a_enviar + offset, &paquete->codigo_operacion, sizeof(uint8_t));
    offset += sizeof(uint8_t);
    memcpy(a_enviar + offset, &paquete->buffer->size, sizeof(uint32_t));
    offset += sizeof(uint32_t);
    memcpy(a_enviar + offset, paquete->buffer->stream, paquete->buffer->size);

    if (total_size_out) *total_size_out = total_size;

    return a_enviar;
}

void destruir_paquete(t_paquete* paquete) {
    if (paquete) {
        if (paquete->buffer) {
            free(paquete->buffer->stream);
            free(paquete->buffer);
        }
    free(paquete);
    }
}

int enviar_paquete(int socket, t_paquete* paquete) {
    uint32_t tamanio_paquete_serealizado;
    void* paquete_serealizado = serializar_paquete(paquete, &tamanio_paquete_serealizado);

    if (!paquete_serealizado) return -1;

    int bytes_enviados = send(socket, paquete_serealizado, tamanio_paquete_serealizado, 0);
    free(paquete_serealizado);

    return (bytes_enviados == tamanio_paquete_serealizado) ? 0 : -1;
}
// Termina send()

// Empieza recv()
t_paquete* recibir_paquete(int socket) {
    t_paquete* paquete = malloc(sizeof(t_paquete));
    if (!paquete) return NULL;

    uint8_t codigo;
    uint32_t size;

    // Leer código de operación
    ssize_t recibidos = recv(socket, &codigo, sizeof(uint8_t), MSG_WAITALL);
    if (recibidos <= 0) {
        free(paquete);
        return NULL;
    }
    paquete->codigo_operacion = codigo;

    // Leer tamaño del buffer
    recibidos = recv(socket, &size, sizeof(uint32_t), MSG_WAITALL);
    if (recibidos <= 0) {
        free(paquete);
        return NULL;
    }

    // Leer el contenido del buffer
    void* stream = malloc(size);
    if (!stream) {
        free(paquete);
        return NULL;
    }

    recibidos = recv(socket, stream, size, MSG_WAITALL);
    if (recibidos <= 0) {
        free(stream);
        free(paquete);
        return NULL;
    }

    t_buffer* buffer = malloc(sizeof(t_buffer));
    if (!buffer) {
        free(stream);
        free(paquete);
        return NULL;
    }

    buffer->size = size;
    buffer->stream = stream;
    paquete->buffer = buffer;



    return paquete;
}


t_package* deserializar_nombre(t_buffer* buffer) {
    t_package* persona = malloc(sizeof(t_package));

    void* stream = buffer->stream;

    memcpy(&(persona->nombreLargo), stream, sizeof(uint32_t));
    stream += sizeof(uint32_t);

    persona->nombre = malloc(persona->nombreLargo);
    memcpy(persona->nombre, stream, persona->nombreLargo);
    stream += persona->nombreLargo;

    memcpy(&(persona->modulo), stream, sizeof(uint8_t));
    stream += sizeof(uint8_t);

    return persona;
}
// Termina Recv()