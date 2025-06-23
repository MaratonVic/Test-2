#ifndef CONTROL_H_
#define CONTROL_H_

#include <commons/bitarray.h>
#include <commons/config.h>
#include "memoria_config.h"
#include <stdint.h>
#include <stdlib.h>
#include <math.h>
#include <pthread.h>


void iniciar_memoria_bitarray(int tamanio, int tamanio_frame);
uint32_t espacio_disponible(void);
int asignar_marco_libre(void);

#endif