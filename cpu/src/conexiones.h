#ifndef CONEXIONES_H
#define CONEXIONES_H

#include <commons/log.h>

int conectar_cliente(const char* ip, const char* puerto, t_log* logger, const char* nombre);
int conectar_a_memoria(t_log* logger);
int conectar_a_kernel_dispatch(t_log* logger);
int conectar_a_kernel_interrupt(t_log* logger);

#endif