#ifndef CONTROL_H_
#define CONTROL_H_

#include <commons/bitarray.h>
#include <commons/config.h>
#include "memoria_config.h"

typedef struct {
    t_bitarray *bitmap_marcos;
    int cantidad_marcos;
    
} MemoriaControl;


MemoriaControl inicializar_control(int cant_frame, char *buffer_bitmap, int bytes_bitmap);

#endif