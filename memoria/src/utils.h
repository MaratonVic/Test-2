#include "../../utils/include/utilsHandshake.h"
#include "../../utils/include/utils.h"
#include "commons/string.h"
#include "memoria_config.h"
#include "errno.h"

char* recibir_string( t_buffer* );
uint32_t recibir_uint32(t_buffer*);
char* agregar_path_instrucciones(char*);
t_paquete* crear_paquete_instruccion(char* instruccion, uint8_t codigo_operacion);

// void enviar_paquete(t_paquete*, int socket, uint8_t);