#include "memoria_ctrl.h"


MemoriaControl inicializar_control(int cant_frame, char *buffer_bitmap, int bytes_bitmap ){ 
    
    MemoriaControl mem_ctrl;
    t_bitarray *bitmap_marco = bitarray_create(buffer_bitmap, bytes_bitmap);
    mem_ctrl.bitmap_marcos=cant_frame;
    
    return mem_ctrl;

}