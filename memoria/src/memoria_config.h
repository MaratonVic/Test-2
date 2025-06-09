#ifndef MEMORIA_CONFIG_H_
#define MEMORIA_CONFIG_H_

#include <commons/config.h>
#include "commons/log.h"


typedef struct memoria
{
    char* puerto_escucha;
    int tam_memoria;
    int tam_pagina;
    int cantidad_niveles;
    int retardo_memoria;
    char* path_swapfile;
    int retardo_swap;
    char* log_level;
    char* dump_path;
    char* path_instrucciones;
} ConfigMemoria;

extern ConfigMemoria cfg;

extern t_log* logger;

t_config* crear_config(void);
ConfigMemoria leer_config(t_config*);

t_log* crear_logger(char*);
#endif