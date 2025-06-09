#ifndef KERNEL_CONFIG_H
#define KERNEL_CONFIG_H

#include<stdio.h>
#include<stdlib.h>
#include<commons/log.h>
#include<commons/string.h>
#include<commons/config.h>

typedef struct {
    char* ip;
    char*  puertoMemoria;
    char* puertoEscuchaDispatch;
    char* puertoEscuchaInterrupt;
    char* puertoEscuchaIO;
    char* algoritmoCortoPlazo;
    char* algoritmoIngresoReady;
    char* alfa;
    int estimacionInicial;
    int tiempoSuspension;
    char* logLevel;
} t_kernelConfig;

t_log* iniciar_logger(void);
t_config* iniciar_config(void);
void leer_config(t_kernelConfig* cfg, t_config* config);
void loguear_config(t_kernelConfig* cfg, t_log* loggerKernel);

#endif