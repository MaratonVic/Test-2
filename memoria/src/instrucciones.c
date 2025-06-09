#include "instrucciones.h"


void deserializar_instruccion(t_buffer* payload,uint32_t* pid, uint32_t* pc){ 
    *pid = recibir_uint32(payload);
    *pc = recibir_uint32(payload);
}

char* obtener_instruccion(uint32_t pid, uint32_t pc) {
    t_list* list_instrucciones = dictionary_get(pid_instrucciones, string_itoa(pid));
    char* instruccion = list_get(list_instrucciones, pc);
    return instruccion;
}

char* buscar_instruccion(t_buffer* payload){
    uint32_t pid, pc;

    deserializar_instruccion(payload, &pid, &pc);
    return obtener_instruccion(pid, pc); 
    //TODO
    // usar dictionary para buscar el pid y sacar la instruccion con el pc
}