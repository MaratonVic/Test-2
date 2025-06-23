#include "pcb.h"
#include <stdlib.h>
#include <string.h>

t_pcb* crear_pcb(uint32_t pid) {
    t_pcb* pcb = malloc(sizeof(t_pcb));
    pcb->pid = pid;
    pcb->pc = 0;
    pcb->mestado = dictionary_create();
    pcb->mtiempo = dictionary_create();

    const char* estados[] = {"NEW", "READY", "EXEC", "BLOCKED", "SUSP_READY", "SUSP_BLOCKED", "EXIT"};
    for (int i = 0; i < 6; i++) {
        int* contador = malloc(sizeof(int));
        *contador = 0;
        dictionary_put(pcb->mestado, strdup(estados[i]), contador);
        
        t_temporal* tiempo = temporal_create();
        temporal_stop(tiempo);
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
    t_list* claves_estado_list = dictionary_keys(pcb->mestado);
    t_list* claves_tiempo_list = dictionary_keys(pcb->mtiempo);

    int cantidad_estado = list_size(claves_estado_list);
    int cantidad_tiempo = list_size(claves_tiempo_list);

    uint32_t total_size = sizeof(uint32_t) * 2; // pid + pc
    total_size += sizeof(uint32_t); // cantidad de claves en mestado

    for (int i = 0; i < cantidad_estado; i++) {
        char* key = list_get(claves_estado_list, i);
        total_size += sizeof(uint32_t);            
        total_size += strlen(key) + 1;            
        total_size += sizeof(int);                 
    }

    total_size += sizeof(uint32_t); // cantidad de claves en mtiempo
    for (int i = 0; i < cantidad_tiempo; i++) {
        char* key = list_get(claves_tiempo_list, i);
        total_size += sizeof(uint32_t);             
        total_size += strlen(key) + 1;              
        total_size += sizeof(uint64_t);             
    }

    void* buffer = malloc(total_size);
    void* ptr = buffer;

    memcpy(ptr, &pcb->pid, sizeof(uint32_t)); ptr += sizeof(uint32_t);
    memcpy(ptr, &pcb->pc, sizeof(uint32_t));  ptr += sizeof(uint32_t);

    memcpy(ptr, &cantidad_estado, sizeof(uint32_t)); ptr += sizeof(uint32_t);
    for (int i = 0; i < cantidad_estado; i++) {
        char* key = list_get(claves_estado_list, i);
        int* valor = dictionary_get(pcb->mestado, key);
        uint32_t len = strlen(key) + 1;

        memcpy(ptr, &len, sizeof(uint32_t)); ptr += sizeof(uint32_t);
        memcpy(ptr, key, len);                ptr += len;
        memcpy(ptr, valor, sizeof(int));      ptr += sizeof(int);
    }

    memcpy(ptr, &cantidad_tiempo, sizeof(uint32_t)); ptr += sizeof(uint32_t);
    for (int i = 0; i < cantidad_tiempo; i++) {
        char* key = list_get(claves_tiempo_list, i);
        uint64_t* valor = dictionary_get(pcb->mtiempo, key);
        uint32_t len = strlen(key) + 1;

        memcpy(ptr, &len, sizeof(uint32_t)); ptr += sizeof(uint32_t);
        memcpy(ptr, key, len);               ptr += len;
        memcpy(ptr, valor, sizeof(uint64_t)); ptr += sizeof(uint64_t);
    }

    list_destroy(claves_estado_list);
    list_destroy(claves_tiempo_list);

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