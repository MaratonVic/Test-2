#ifndef INSTRUCCIONES_H
#define INSTRUCCIONES_H

#include <stdint.h>
#include <commons/collections/list.h>

typedef enum {
    INST_NOOP,
    INST_WRITE,
    INST_READ,
    INST_GOTO,
    INST_IO,
    INST_INIT_PROC,
    INST_DUMP_MEMORY,
    INST_EXIT
} t_tipo_instruccion;

typedef struct {
    t_tipo_instruccion tipo;
    char** parametros;
} t_instruccion;

void destruir_instruccion(t_instruccion* inst);

#endif