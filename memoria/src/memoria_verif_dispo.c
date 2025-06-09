#include "memoria_verif_dispo.h"


int calcular_memoria_disponible(MemoriaControl control) {
    int memoria_disponible = 0;

    for (int i = 0; i < control.cantidad_marcos; i++) {
        if (bitarray_test_bit(control.bitmap_marcos, i) == 0) { 
            memoria_disponible += cfg.tam_pagina;
        }
    }

    return memoria_disponible;
}
