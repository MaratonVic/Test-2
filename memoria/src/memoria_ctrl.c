#include "memoria_ctrl.h"

static t_bitarray* memoria_bitarray;
static pthread_mutex_t MUTEX_MEM_BITARRAY;


void iniciar_memoria_bitarray(int tamanio, int tamanio_frame){
    pthread_mutex_init(&MUTEX_MEM_BITARRAY, 0);
    int total_frames = (tamanio / tamanio_frame);
    int total_frames_en_bytes = ceil((double) total_frames /8.0);
    char* bitarray = calloc(total_frames_en_bytes, sizeof(char));
    memoria_bitarray = bitarray_create_with_mode(bitarray, total_frames_en_bytes, LSB_FIRST);
}

uint32_t espacio_disponible () {
    uint32_t espacio_libre = 0;
    pthread_mutex_lock(&MUTEX_MEM_BITARRAY);
    for (int i = 0; i < bitarray_get_max_bit(memoria_bitarray); i++)
    {
        if(bitarray_test_bit(memoria_bitarray, i) == 0) {
            espacio_libre ++;
        }
    }
    pthread_mutex_unlock(&MUTEX_MEM_BITARRAY);
    
    return espacio_libre*cfg.tam_pagina;
}

int asignar_marco_libre() {
    pthread_mutex_lock(&MUTEX_MEM_BITARRAY);
    for (int i = 0; i < bitarray_get_max_bit(memoria_bitarray); i++)
    {
        if (bitarray_test_bit(memoria_bitarray, i) == 0)
        {
            bitarray_set_bit(memoria_bitarray, i);
            pthread_mutex_unlock(&MUTEX_MEM_BITARRAY);
            return i;
        }
    }
    return -1;
}