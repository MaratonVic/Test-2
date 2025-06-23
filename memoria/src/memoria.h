#ifndef MEMORIA_H
#define MEMORIA_H

#include <commons/log.h>
#include <commons/config.h>
#include <pthread.h>
#include "../../utils/include/utils.h"
#include "../../utils/include/utilsHandshake.h"
#include "../../utils/include/kernelMemoria.h"
#include "memoria_wri.h"
#include "procesos.h"
#include "memoria_config.h"
#include "instrucciones.h"
#include "memoria_ctrl.h"
#include "memoria_verif_dispo.h"


typedef struct
{
    int fd_cliente;
} t_datos_cliente;


void controlador_cliente(void*);
//int recibir_operacion(int);
void controlador_kernel(int);
void controlador_cpu(int);
//uint8_t recibirCodigo(int);
int iniciar_proceso_en_memoria(t_buffer*);
//t_paquete* recibir_pseudocodigo(int socket);

void signal_handler(int signal);
static void cerrar_programa(void);

#endif