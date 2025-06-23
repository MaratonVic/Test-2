#ifndef DISPATCH_H
#define DISPATCH_H

#include "pcb.h"
#include <commons/log.h>

typedef struct {
    int socket;
    t_log* logger;
} t_args_dispatch;

void* manejar_dispatch(void* arg);
t_pcb* recibir_pcb_serializado(int socket);

#endif