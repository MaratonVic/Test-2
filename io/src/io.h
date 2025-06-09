#ifndef IO_H_
#define IO_H_

#include<stdio.h>
#include<stdlib.h>
#include "commons/log.h"
#include "commons/config.h"
#include <unistd.h>
#include <pthread.h>
#include "ioConexiones.h"
#include "utilsHandshake.h"
#include "utils.h"
#include "cpuIO.h"

//#include "../../utils/include/utils.h"

typedef struct {
    char* ip_kernel;
    char* puerto_kernel;
    char* log_level;
}t_configIO;


// INICIAR LOGGER Y CONFIG
t_log* iniciar_logger(void); 
t_config* iniciar_config_io(void);
void leer_config(t_configIO* cfg, t_config* config);

// //HANDSHAKE con KERNEL
// void iniciar_io(int socket, char* nombreIo);


// int manejar_peticiones(int socket);
// //esta funcion puede ir en otro lugar mas generico
// void recibir_peticion(int, void*, uint32_t, void*);
// //Creo que esto puede ir en otro lugar mas generico
// void realizar_peticion(void*, void*);

void cerrar_programa(t_log*, t_config*);


#endif