#include "cpuIO.h"
#include <errno.h>

uint32_t pid_io = 0; 
pthread_mutex_t mutex_pid_io;

//cpu kernel
t_paquete* crear_paquete_handshake_syscall_cpu_io_prueba(char* nombre_modulo, uint8_t codigo_operacion,  uint8_t syscall, uint32_t timer, uint32_t pid) {
    t_package_io handshake;

    handshake.nombre = string_duplicate(nombre_modulo);
    handshake.nombreLargo = strlen(nombre_modulo) + 1;

    pthread_mutex_lock(&mutex_pid_io);
    handshake.pid = pid;
    pthread_mutex_unlock(&mutex_pid_io);

    handshake.syscall = syscall;
    handshake.timer = timer;

    t_buffer* buffer = malloc(sizeof(t_buffer));
    buffer->size = sizeof(uint32_t) + handshake.nombreLargo + sizeof(uint32_t) + sizeof(uint8_t) + sizeof(uint32_t);

    buffer->offset = 0;
    buffer->stream = malloc(buffer->size);

    memcpy(buffer->stream + buffer->offset, &handshake.nombreLargo, sizeof(uint32_t));
    buffer->offset += sizeof(uint32_t);

    memcpy(buffer->stream + buffer->offset, handshake.nombre, handshake.nombreLargo);
    buffer->offset += handshake.nombreLargo;

    memcpy(buffer->stream + buffer->offset, &handshake.pid, sizeof(uint32_t));
    buffer->offset += sizeof(uint32_t);

    memcpy(buffer->stream + buffer->offset, &handshake.syscall, sizeof(uint8_t));
    buffer->offset += sizeof(uint8_t);

    memcpy(buffer->stream + buffer->offset, &handshake.timer, sizeof(uint32_t));
    buffer->offset += sizeof(uint32_t);


    t_paquete* paquete = malloc(sizeof(t_paquete));
    paquete->codigo_operacion = codigo_operacion;
    paquete->buffer = buffer;

    free(handshake.nombre);

    return paquete;
}

t_paquete* crear_paquete_handshake_syscall_cpu_io(char* nombre_modulo, uint8_t codigo_operacion,  uint8_t syscall, uint32_t timer) {
    t_package_io handshake;

    handshake.nombre = string_duplicate(nombre_modulo);
    handshake.nombreLargo = strlen(nombre_modulo) + 1;

    pthread_mutex_lock(&mutex_pid_io);
    handshake.pid = pid_io;
    pthread_mutex_unlock(&mutex_pid_io);

    handshake.syscall = syscall;
    handshake.timer = timer;


    t_buffer* buffer = malloc(sizeof(t_buffer));
    buffer->size = sizeof(uint32_t) + handshake.nombreLargo + sizeof(uint32_t) + sizeof(uint8_t) + sizeof(uint32_t) ;

    buffer->offset = 0;
    buffer->stream = malloc(buffer->size);

    memcpy(buffer->stream + buffer->offset, &handshake.nombreLargo, sizeof(uint32_t));
    buffer->offset += sizeof(uint32_t);

    memcpy(buffer->stream + buffer->offset, handshake.nombre, handshake.nombreLargo);
    buffer->offset += handshake.nombreLargo;

    memcpy(buffer->stream + buffer->offset, &handshake.pid, sizeof(uint32_t));
    buffer->offset += sizeof(uint32_t);

    memcpy(buffer->stream + buffer->offset, &handshake.syscall, sizeof(uint8_t));
    buffer->offset += sizeof(uint8_t);

    memcpy(buffer->stream + buffer->offset, &handshake.timer, sizeof(uint32_t));
    buffer->offset += sizeof(uint32_t);


    t_paquete* paquete = malloc(sizeof(t_paquete));
    paquete->codigo_operacion = codigo_operacion;
    paquete->buffer = buffer;

    free(handshake.nombre);
    free(handshake.nombreCpu);

    return paquete;
}


void* serializar_paquete_syscall_cpu_io(t_paquete* paquete, uint32_t* total_size_out){
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

void destruir_paquete_syscall_cpu_io(t_paquete* paquete) {
    if (paquete) {
        if (paquete->buffer) {
            free(paquete->buffer->stream);
            free(paquete->buffer);
        }
    free(paquete);
    }
}

t_package_io* deserializar_nombre_cpu_io(t_buffer* buffer) {
    t_package_io* paquete = malloc(sizeof(t_package_io));
    int offset = 0;

    if (buffer->size < offset + sizeof(uint32_t)) {
        printf("[deserializar_nombre_cpu_io] Error: nombreLargo no cabe en el buffer. offset=%d, size=%d", offset, buffer->size);
        free(paquete);
        return NULL;
    }
    memcpy(&paquete->nombreLargo, buffer->stream + offset, sizeof(uint32_t));
    offset += sizeof(uint32_t);

    if (buffer->size < offset + paquete->nombreLargo) {
        printf("[deserializar_nombre_cpu_io] Error: nombre no cabe en el buffer. offset=%d, nombreLargo=%d, size=%d", offset, paquete->nombreLargo, buffer->size);
        free(paquete);
        return NULL;
    }
    paquete->nombre = malloc(paquete->nombreLargo);
    memcpy(paquete->nombre, buffer->stream + offset, paquete->nombreLargo);
    offset += paquete->nombreLargo;

    if (buffer->size < offset + sizeof(uint32_t)) {
        printf("[deserializar_nombre_cpu_io] Error: pid no cabe en el buffer. offset=%d, size=%d", offset, buffer->size);
        free(paquete->nombre);
        free(paquete);
        return NULL;
    }
    memcpy(&paquete->pid, buffer->stream + offset, sizeof(uint32_t));
    offset += sizeof(uint32_t);

    if (buffer->size < offset + sizeof(uint8_t)) {
        printf("[deserializar_nombre_cpu_io] Error: syscall no cabe en el buffer. offset=%d, size=%d", offset, buffer->size);
        free(paquete->nombre);
        free(paquete);
        return NULL;
    }
    memcpy(&paquete->syscall, buffer->stream + offset, sizeof(uint8_t));
    offset += sizeof(uint8_t);

    if (buffer->size < offset + sizeof(uint32_t)) {
        printf("[deserializar_nombre_cpu_io] Error: timer no cabe en el buffer. offset=%d, size=%d", offset, buffer->size);
        free(paquete->nombre);
        free(paquete);
        return NULL;
    }
    memcpy(&paquete->timer, buffer->stream + offset, sizeof(uint32_t));
    offset += sizeof(uint32_t);

    if (buffer->size < offset + sizeof(uint32_t)) {
        printf("[deserializar_nombre_cpu_io] Error: nombreLargoCpu no cabe en el buffer. offset=%d, size=%d", offset, buffer->size);
        free(paquete->nombre);
        free(paquete);
        return NULL;
    }
    memcpy(&paquete->nombreLargoCpu, buffer->stream + offset, sizeof(uint32_t));
    offset += sizeof(uint32_t);

    if (buffer->size < offset + paquete->nombreLargoCpu) {
        printf("[deserializar_nombre_cpu_io] Error: nombreCpu no cabe en el buffer. offset=%d, nombreLargoCpu=%d, size=%d", offset, paquete->nombreLargoCpu, buffer->size);
        free(paquete->nombre);
        free(paquete);
        return NULL;
    }
    paquete->nombreCpu = malloc(paquete->nombreLargoCpu);
    memcpy(paquete->nombreCpu, buffer->stream + offset, paquete->nombreLargoCpu);
    offset += paquete->nombreLargoCpu;

    return paquete;
}

t_package_io* deserializar_nombre_cpu_io_kernel(t_buffer* buffer) {
    t_package_io* paquete = malloc(sizeof(t_package_io));
    int offset = 0;

    if (buffer->size < offset + sizeof(uint32_t)) {
        printf("[deserializar_nombre_cpu_io] Error: nombreLargo no cabe en el buffer. offset=%d, size=%d", offset, buffer->size);
        free(paquete);
        return NULL;
    }
    memcpy(&paquete->nombreLargo, buffer->stream + offset, sizeof(uint32_t));
    offset += sizeof(uint32_t);

    if (buffer->size < offset + paquete->nombreLargo) {
        printf("[deserializar_nombre_cpu_io] Error: nombre no cabe en el buffer. offset=%d, nombreLargo=%d, size=%d", offset, paquete->nombreLargo, buffer->size);
        free(paquete);
        return NULL;
    }
    paquete->nombre = malloc(paquete->nombreLargo);
    memcpy(paquete->nombre, buffer->stream + offset, paquete->nombreLargo);
    offset += paquete->nombreLargo;

    if (buffer->size < offset + sizeof(uint32_t)) {
        printf("[deserializar_nombre_cpu_io] Error: pid no cabe en el buffer. offset=%d, size=%d", offset, buffer->size);
        free(paquete->nombre);
        free(paquete);
        return NULL;
    }
    memcpy(&paquete->pid, buffer->stream + offset, sizeof(uint32_t));
    offset += sizeof(uint32_t);

    if (buffer->size < offset + sizeof(uint8_t)) {
        printf("[deserializar_nombre_cpu_io] Error: syscall no cabe en el buffer. offset=%d, size=%d", offset, buffer->size);
        free(paquete->nombre);
        free(paquete);
        return NULL;
    }
    memcpy(&paquete->syscall, buffer->stream + offset, sizeof(uint8_t));
    offset += sizeof(uint8_t);

    if (buffer->size < offset + sizeof(uint32_t)) {
        printf("[deserializar_nombre_cpu_io] Error: timer no cabe en el buffer. offset=%d, size=%d", offset, buffer->size);
        free(paquete->nombre);
        free(paquete);
        return NULL;
    }
    memcpy(&paquete->timer, buffer->stream + offset, sizeof(uint32_t));
    offset += sizeof(uint32_t);


    return paquete;
}


// Termina cpu kernel
