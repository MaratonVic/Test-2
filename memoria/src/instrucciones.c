#include "instrucciones.h"


void deserializar_instruccion(t_buffer* payload,uint32_t* pid, uint32_t* pc){ 
    *pid = recibir_uint32(payload);
    *pc = recibir_uint32(payload);
}

char* obtener_instruccion(uint32_t pid, uint32_t pc) {
    char key_pid[16];
    sprintf(key_pid, "%u", pid);
    t_list* instrucciones = NULL;

    if(!dictionary_is_empty(pid_instrucciones)){
        instrucciones = dictionary_get(pid_instrucciones, key_pid);
        if (!instrucciones) {
            log_error(logger, "PID %u no encontrado en diccionario de instrucciones", pid);
            return NULL;
        }

        if (pc >= list_size(instrucciones)) {
            log_error(logger, "PC %u fuera de rango para PID %u (cant instrucciones: %d)", pc, pid, list_size(instrucciones));
            return NULL;
        }

    }
    if(!list_is_empty(instrucciones)){
        return list_get(instrucciones, pc);
    }
    return NULL;
}

char* buscar_instruccion(t_buffer* payload){
    uint32_t pid, pc;

    deserializar_instruccion(payload, &pid, &pc);
    return obtener_instruccion(pid, pc); 
    //TODO
    // usar dictionary para buscar el pid y sacar la instruccion con el pc
}