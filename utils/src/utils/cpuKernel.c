#include "cpuKernel.h"
#include <errno.h>


t_paquete* crear_paquete_handshake_kernel_cpu(uint8_t codigo_operacion, uint32_t pid, uint32_t pc) {
    t_buffer* buffer = malloc(sizeof(t_buffer));
    buffer->size = sizeof(uint32_t) + sizeof(uint32_t);
    buffer->offset = 0;
    buffer->stream = malloc(buffer->size);

    memcpy(buffer->stream + buffer->offset, &pid, sizeof(uint32_t));
    buffer->offset += sizeof(uint32_t);

    memcpy(buffer->stream + buffer->offset, &pc, sizeof(uint32_t));
    buffer->offset += sizeof(uint32_t);

    t_paquete* paquete = malloc(sizeof(t_paquete));
    paquete->codigo_operacion = codigo_operacion;
    paquete->buffer = buffer;

    return paquete;
}

t_paquete* crear_paquete_syscall_exit(uint8_t codigo_operacion, char* nombre, uint32_t pid) {
    t_package_syscall_exit exit;

    exit.nombre = string_duplicate(nombre);
    exit.nombreLargo = strlen(nombre) + 1;
    exit.pid = pid;

    t_buffer* buffer = malloc(sizeof(t_buffer));
    buffer->size = sizeof(uint32_t) + exit.nombreLargo + sizeof(uint32_t);
    buffer->offset = 0;
    buffer->stream = malloc(buffer->size);

    memcpy(buffer->stream + buffer->offset, &exit.nombreLargo, sizeof(uint32_t));
    buffer->offset += sizeof(uint32_t);

    memcpy(buffer->stream + buffer->offset, exit.nombre, exit.nombreLargo);
    buffer->offset += exit.nombreLargo;

    memcpy(buffer->stream + buffer->offset, &exit.pid, sizeof(uint32_t));
    buffer->offset += sizeof(uint32_t);


    t_paquete* paquete = malloc(sizeof(t_paquete));
    paquete->codigo_operacion = codigo_operacion;
    paquete->buffer = buffer;

    free(exit.nombre);

    return paquete;
}


void* serializar_paquete_kernel_cpu(t_paquete* paquete, uint32_t* total_size_out){
    uint32_t total_size = sizeof(uint8_t) + sizeof(uint32_t) + paquete->buffer->size;
    void* a_enviar = malloc(total_size);
    int offset = 0;

    memcpy(a_enviar + offset, &paquete->codigo_operacion, sizeof(uint8_t));
    offset += sizeof(uint8_t);
    memcpy(a_enviar + offset, &paquete->buffer->size, sizeof(uint32_t));
    offset += sizeof(uint32_t);
    memcpy(a_enviar + offset, paquete->buffer->stream, paquete->buffer->size);

    if (total_size_out) *total_size_out = total_size;
    // for (int i = 0; i < total_size; i++) {
    //     printf("%02X ", ((unsigned char*)a_enviar)[i]);
    // }
    // printf("\n");

    return a_enviar;
}

void destruir_paquete_kernel_cpu(t_paquete* paquete) {
    if (paquete) {
        if (paquete->buffer) {
            free(paquete->buffer->stream);
            free(paquete->buffer);
        }
    free(paquete);
    }
}

t_package_kernel_cpu* deserializar_kernel_cpu(t_buffer* buffer) {
    t_package_kernel_cpu* kernelCpu = malloc(sizeof(t_package_kernel_cpu));
    int offset = 0;

    memcpy(&kernelCpu->pid, buffer->stream + offset, sizeof(uint32_t));
    offset += sizeof(uint32_t);

    memcpy(&kernelCpu->pc, buffer->stream + offset, sizeof(uint32_t));
    offset += sizeof(uint32_t);

    return kernelCpu;
}

t_package_syscall_exit* deserializar_sycall_exit(t_buffer* buffer){
    t_package_syscall_exit* exit = malloc(sizeof(t_package_syscall_exit));

    void* stream = buffer->stream;

    memcpy(&(exit->nombreLargo), stream, sizeof(uint32_t));
    stream += sizeof(uint32_t);

    exit->nombre = malloc(exit->nombreLargo + 1);
    memcpy(exit->nombre, stream, exit->nombreLargo);
    exit->nombre[exit->nombreLargo] = '\0';
    stream += exit->nombreLargo;

    memcpy(&(exit->pid), stream, sizeof(uint32_t));
    stream += sizeof(uint32_t);

    return exit;
}

void* serializar_paquete_syscall_exit(t_paquete* paquete, uint32_t* bytes_out) {
    uint32_t total_size = sizeof(uint8_t) + sizeof(uint32_t) + paquete->buffer->size;
    void* buffer = malloc(total_size);
    int offset = 0;

    memcpy(buffer + offset, &paquete->codigo_operacion, sizeof(uint8_t));
    offset += sizeof(uint8_t);

    memcpy(buffer + offset, &paquete->buffer->size, sizeof(uint32_t));
    offset += sizeof(uint32_t);

    memcpy(buffer + offset, paquete->buffer->stream, paquete->buffer->size);

    if (bytes_out) *bytes_out = total_size;
    return buffer;
}