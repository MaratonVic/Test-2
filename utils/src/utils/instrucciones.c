#include <stdlib.h>
#include <commons/collections/list.h>
#include "instrucciones.h"

void destruir_instruccion(t_instruccion* inst) {
    for (int i = 0; inst->parametros[i] != NULL; i++)
    free(inst->parametros[i]);
    free(inst->parametros);
    free(inst);
}