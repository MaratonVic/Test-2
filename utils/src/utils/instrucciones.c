#include <stdlib.h>
#include <commons/collections/list.h>
#include "instrucciones.h"

void destruir_instruccion(t_instruccion* instruccion) {
    for (int i = 0; instruccion->parametros[i] != NULL; i++) {
        free(instruccion->parametros[i]);
    }
    free(instruccion->parametros);
    free(instruccion);
}