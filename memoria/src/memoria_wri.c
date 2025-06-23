#include "memoria_wri.h"

static void* memoria_fisica;

static pthread_mutex_t MEMORIA_MUTEX;

int iniciar_ram(int tamanio) {
    pthread_mutex_init(&MEMORIA_MUTEX, 0);
    memoria_fisica = malloc(tamanio * sizeof(char));
    if(memoria_fisica == NULL){
        return -1;
    }
    return 0;
}

