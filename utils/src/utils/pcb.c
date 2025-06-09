#include "pcb.h"
#include <stdlib.h>
#include <string.h>

t_pcb* crear_pcb(uint32_t pid) {
    t_pcb* pcb = malloc(sizeof(t_pcb));
    pcb->pid = pid;
    pcb->pc = 0;
    pcb->mestado = dictionary_create();
    pcb->mtiempo = dictionary_create();

    const char* estados[] = { "NEW", "READY", "EXEC", "BLOCKED", "SUSPENDED", "EXIT" };
    for (int i = 0; i < 6; i++) {
        int* contador = malloc(sizeof(int));
        *contador = 0;
        dictionary_put(pcb->mestado, strdup(estados[i]), contador);

        uint64_t* tiempo = malloc(sizeof(uint64_t));
        *tiempo = 0;
        dictionary_put(pcb->mtiempo, strdup(estados[i]), tiempo);
    }

    return pcb;
}

void enviar_pcb(t_pcb* pcb, int fd, t_log* logger) {
    log_info(logger, "[Kernel] Enviando PCB por socket FD: %d", fd);

    uint32_t size;
    void* buffer = serializar_pcb(pcb, &size, logger);

    send(fd, &size, sizeof(uint32_t), 0);
    send(fd, buffer, size, 0);

    log_info(logger, "[Kernel] PCB enviado (size: %u bytes)", size);
    
    free(buffer);
}

void destruir_pcb(t_pcb* pcb) {
    dictionary_destroy_and_destroy_elements(pcb->mestado, free);
    dictionary_destroy_and_destroy_elements(pcb->mtiempo, free);
    free(pcb);
}

void* serializar_pcb(t_pcb* pcb, uint32_t* size_out, t_log* logger) {
    char** claves_estado = dictionary_keys(pcb->mestado);
    char** claves_tiempo = dictionary_keys(pcb->mtiempo);

    int cantidad_estado = dictionary_size(pcb->mestado);
    int cantidad_tiempo = dictionary_size(pcb->mtiempo);

    uint32_t total_size = sizeof(uint32_t) * 2; // pid + pc

    total_size += sizeof(uint32_t); // cantidad de claves en mestado
    for (int i = 0; i < cantidad_estado; i++) {
        total_size += sizeof(uint32_t);
        total_size += strlen(claves_estado[i]) + 1;
        total_size += sizeof(int);
    }

    total_size += sizeof(uint32_t); // cantidad de claves en mtiempo
    for (int i = 0; i < cantidad_tiempo; i++) {
        total_size += sizeof(uint32_t);
        total_size += strlen(claves_tiempo[i]) + 1;
        total_size += sizeof(uint64_t);
    }

    void* buffer = malloc(total_size);
    void* ptr = buffer;

    memcpy(ptr, &pcb->pid, sizeof(uint32_t)); ptr += sizeof(uint32_t);
    memcpy(ptr, &pcb->pc, sizeof(uint32_t)); ptr += sizeof(uint32_t);

    memcpy(ptr, &cantidad_estado, sizeof(uint32_t)); ptr += sizeof(uint32_t);
    for (int i = 0; i < cantidad_estado; i++) {
        uint32_t len = strlen(claves_estado[i]) + 1;
        int* valor = dictionary_get(pcb->mestado, claves_estado[i]);

        memcpy(ptr, &len, sizeof(uint32_t)); ptr += sizeof(uint32_t);
        memcpy(ptr, claves_estado[i], len); ptr += len;
        memcpy(ptr, valor, sizeof(int)); ptr += sizeof(int);
    }

    memcpy(ptr, &cantidad_tiempo, sizeof(uint32_t)); ptr += sizeof(uint32_t);
    for (int i = 0; i < cantidad_tiempo; i++) {
        uint32_t len = strlen(claves_tiempo[i]) + 1;
        uint64_t* valor = dictionary_get(pcb->mtiempo, claves_tiempo[i]);

        memcpy(ptr, &len, sizeof(uint32_t)); ptr += sizeof(uint32_t);
        memcpy(ptr, claves_tiempo[i], len); ptr += len;
        memcpy(ptr, valor, sizeof(uint64_t)); ptr += sizeof(uint64_t);
    }

    string_array_destroy(claves_estado);
    string_array_destroy(claves_tiempo);

    *size_out = total_size;
    log_info(logger, "Serialización PCB - Total size: %u bytes", total_size);
    return buffer;
}

t_pcb* deserializar_pcb(void* buffer, uint32_t size) {
    void* ptr = buffer;
    t_pcb* pcb = malloc(sizeof(t_pcb));

    memcpy(&pcb->pid, ptr, sizeof(int)); ptr += sizeof(int);
    memcpy(&pcb->pc, ptr, sizeof(int)); ptr += sizeof(int);

    pcb->mestado = dictionary_create();
    pcb->mtiempo = dictionary_create();

    uint32_t cant_me;
    memcpy(&cant_me, ptr, sizeof(uint32_t)); ptr += sizeof(uint32_t);
    for (int i = 0; i < cant_me; i++) {
        uint32_t len;
        memcpy(&len, ptr, sizeof(uint32_t)); ptr += sizeof(uint32_t);

        char* estado = malloc(len);
        memcpy(estado, ptr, len); ptr += len;

        int* valor = malloc(sizeof(int));
        memcpy(valor, ptr, sizeof(int)); ptr += sizeof(int);

        dictionary_put(pcb->mestado, estado, valor);
    }

    uint32_t cant_mt;
    memcpy(&cant_mt, ptr, sizeof(uint32_t)); ptr += sizeof(uint32_t);
    for (int i = 0; i < cant_mt; i++) {
        uint32_t len;
        memcpy(&len, ptr, sizeof(uint32_t)); ptr += sizeof(uint32_t);

        char* estado = malloc(len);
        memcpy(estado, ptr, len); ptr += len;

        uint64_t* valor = malloc(sizeof(uint64_t));
        memcpy(valor, ptr, sizeof(uint64_t)); ptr += sizeof(uint64_t);

        dictionary_put(pcb->mtiempo, estado, valor);
    }

    return pcb;
}

void logear_metricas(t_dictionary* dict, const char* tipo, t_log* logger) {
    void log_item(char* key, void* value) {
        if (strcmp(tipo, "Tiempo") == 0)
            log_info(logger, "[Metricas %s] %s = %lu", tipo, key, *(uint64_t*)value);
        else
            log_info(logger, "[Metricas %s] %s = %d", tipo, key, *(int*)value);
    }
    dictionary_iterator(dict, log_item);
}