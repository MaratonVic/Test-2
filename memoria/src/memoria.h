#ifndef MEMORIA_H
#define MEMORIA_H

#include <commons/log.h>
#include <commons/config.h>
#include <pthread.h>
#include "../../utils/include/utils.h"
#include "../../utils/include/utilsHandshake.h"
#include "memoria_wri.h"
#include "procesos.h"
#include "memoria_config.h"
#include "instrucciones.h"
#include "memoria_ctrl.h"
#include "memoria_verif_dispo.h"

void controlador_cliente(int);
//int recibir_operacion(int);
void controlador_kernel(int);
void controlador_cpu(int);
//uint8_t recibirCodigo(int);
void iniciar_proceso_en_memoria(t_buffer*);
//t_paquete* recibir_pseudocodigo(int socket);

void signal_handler(int signal);
static void cerrar_programa(void);

#endif