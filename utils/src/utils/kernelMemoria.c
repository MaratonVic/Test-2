#include "kernelMemoria.h"

t_paquete* crear_paquete_dump_memoria(uint8_t codigo_operacion, uint32_t pid) {
    t_buffer* buffer = malloc(sizeof(t_buffer));
    buffer->size = sizeof(uint32_t);
    buffer->offset = 0;
    buffer->stream = malloc(buffer->size);

    memcpy(buffer->stream + buffer->offset, &pid, sizeof(uint32_t));
    buffer->offset += sizeof(uint32_t);


    t_paquete* paquete = malloc(sizeof(t_paquete));
    paquete->codigo_operacion = codigo_operacion;
    paquete->buffer = buffer;

    return paquete;
}

void* serializar_paquete_dump_memoria(t_paquete* paquete, uint32_t* total_size_out) {
    uint32_t total_size = sizeof(uint8_t) + sizeof(uint32_t) + paquete->buffer->size;
    void* stream = malloc(total_size);
    uint32_t offset = 0;


    memcpy(stream + offset, &paquete->codigo_operacion, sizeof(uint8_t));
    offset += sizeof(uint8_t);


    memcpy(stream + offset, &paquete->buffer->size, sizeof(uint32_t));
    offset += sizeof(uint32_t);

    memcpy(stream + offset, paquete->buffer->stream, paquete->buffer->size);

    if (total_size_out != NULL)
        *total_size_out = total_size;

    return stream;
}



void enviar_paquete_dump(int socket, uint32_t pid){
    t_paquete* paquete = crear_paquete_dump_memoria(INIT_DUMP_MEMORY, pid);
    uint32_t bytes = 0;
    void* a_enviar = serializar_paquete_dump_memoria(paquete, &bytes);
    send(socket, a_enviar, bytes, 0);
    destruir_paquete(paquete);
    free(a_enviar);

}

void enviar_paquete_susp(int socket, uint32_t pid){
    t_paquete* paquete = crear_paquete_dump_memoria(SUSP, pid);
    uint32_t bytes = 0;
    void* a_enviar = serializar_paquete_dump_memoria(paquete, &bytes);
    send(socket, a_enviar, bytes, 0);
    destruir_paquete(paquete);
    free(a_enviar);

}


uint32_t deserializar_syscall_dump_pid(t_buffer* buffer) {
    if (!buffer || !buffer->stream || buffer->size < sizeof(uint32_t)) {

        return (uint32_t)-1;
    }

    uint32_t pid = 0;
    memcpy(&pid, buffer->stream, sizeof(uint32_t));
    return pid;
}

uint32_t deserializar_tam_memoria(t_buffer* buffer) {
    uint32_t tamanio = 0;
    void* stream = buffer->stream;

    memcpy(&(tamanio), stream, sizeof(uint32_t));
    stream += sizeof(uint32_t);

    return tamanio;
}