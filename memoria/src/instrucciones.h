#ifndef INSTRUCCIONES_H_
#define INSTRUCCIONES_H_

#include "commons/log.h"
#include "../../utils/include/utils.h"
#include "../../utils/include/utilsHandshake.h"
#include "procesos.h"

extern t_dictionary* pid_instrucciones;

char* buscar_instruccion(t_buffer* payload);
void deserializar_instruccion(t_buffer* payload,uint32_t* pid, uint32_t* pc);
char* obtener_instruccion(uint32_t pid, uint32_t pc);

#endif