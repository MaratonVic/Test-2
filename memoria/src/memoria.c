#include "./memoria.h"

ConfigMemoria cfg;
t_log* logger;
static int socket_escucha;
static t_config* config;
//static t_dictionary* procesos_memoria = NULL;

int main(int argc, char* argv[]) {

    signal(SIGINT, signal_handler);

    config = crear_config();
    cfg = leer_config(config);
    logger = crear_logger(cfg.log_level);
    
    int cant_frame=cfg.tam_memoria/cfg.tam_pagina; 
    int bytes_bitmap = (cant_frame / 8) + ((cant_frame % 8 != 0) ? 1 : 0);
    char *buffer_bitmap = calloc(1, bytes_bitmap); 
    MemoriaControl control = inicializar_control (cant_frame, *buffer_bitmap, bytes_bitmap);

    socket_escucha = iniciar_servidor(cfg.puerto_escucha, logger);
    if(iniciar_ram(cfg.tam_memoria) < 0) {
        log_error(logger, "Error al iniciar el tamanio de la memoria");
    }
    iniciar_lista_procesos();
    while (1)
    {
        int fd_cliente = accept(socket_escucha, NULL, NULL);

        pthread_t nuevo_hilo;
        pthread_create(&nuevo_hilo, NULL,(void *) controlador_cliente,(void *) fd_cliente);
        pthread_detach(nuevo_hilo);
        }
    
    //esperar_cliente(socket_escucha);
    return 0;
}


/* TODO: Hacer recibir_operacion, completar que los clientes van a usar */
void controlador_cliente(int fd_cliente) {
    t_paquete* paquete = recibir_paquete(fd_cliente);
    if (!paquete) {
        log_error(logger, "Fallo al recibir paquete");
        EXIT_FAILURE;
    }
    t_package* handshake = deserializar_nombre(paquete->buffer);

    int cod_op = paquete->codigo_operacion;
    uint8_t cod_mod = handshake->modulo;

    free(handshake->nombre);
    free(handshake);
    destruir_paquete(paquete);

    if(cod_op == HANDSHAKE) {
        if (cod_mod == KERNEL){
            log_info(logger, "## Kernel Conectado - FD del socket: %d\n", fd_cliente);
            controlador_kernel(fd_cliente);
        }
        else {
            log_info(logger, "## CPU Conectada - FD del socket: %d\n", fd_cliente);
            controlador_cpu(fd_cliente);
        }
    }
    else
    {
        log_error(logger, "Primero debe realizar un handshake para entablar comunicacion");
    }
}

void controlador_kernel(int fd_cliente) {
    t_paquete* paquete = recibir_paquete(fd_cliente);
    int cod_op = paquete->codigo_operacion;
    log_info(logger, "Código de operación recibido: %hhu", paquete->codigo_operacion);
    switch (cod_op)
    {
    case INIT_PROC:
        usleep(1000 * cfg.retardo_memoria);
        log_error(logger, "Entre");
        iniciar_proceso_en_memoria(paquete->buffer);
        break;
    default:
        log_error(logger, "Operacion invalida \n");
        break;
    }
    //destruir_paquete(paquete);
}

void controlador_cpu(int fd_cliente) {
    t_paquete* paquete = recibir_paquete(fd_cliente);
    int cod_op = paquete->codigo_operacion;
    while(1){
        switch (cod_op)
        {
            case SOLICITAR_INSTRUCCION:
                usleep(cfg.retardo_memoria * 1000);
                char* instruccion = buscar_instruccion(paquete->buffer);
                if(!instruccion) {
                    log_error(logger, "Instruccion no encontrada");
                    //enviar_paquete(paquete, fd_cliente, NO_INSTRUCCION);
                }
                t_paquete* paquete_instruccion = crear_paquete_instruccion(instruccion, INSTRUCCION_SOLICITADA);
                if(enviar_paquete(fd_cliente, paquete_instruccion) == 0) {
                    log_info(logger, "Instruccion enviada correctamente");
                }
                else {
                    log_error(logger, "Fallo al enviar un instruccion");
                }
                destruir_paquete(paquete_instruccion);
                break;
            default: 
                log_error(logger, "Operacion invalida \n");        
            break;
        }
        }
    destruir_paquete(paquete);
}

static void cerrar_programa(void) {
    log_destroy(logger);
    config_destroy(config);
}

void signal_handler(int signal) {
    cerrar_programa();
    close(socket_escucha);
    exit(0);
}

/*
uint8_t recibirCodigo(int fd_cliente){
    t_paquete* paquete = recibir_paquete(fd_cliente);
    if (!paquete) {
        log_error(logger, "Fallo al recibir paquete");
        EXIT_FAILURE;
    }

    t_package* handshake = deserializar_nombre(paquete->buffer);

    uint8_t codigo = handshake->modulo;
    free(handshake->nombre);
    free(handshake);
    destruir_paquete(paquete);

    return codigo;
}

int recibir_operacion(int fd_cliente) {
    t_paquete* paquete = recibir_paquete(fd_cliente);
    int codigo = paquete->codigo_operacion;
    destruir_paquete(paquete);  // Liberás memoria que pediste en recibir_paquete
    
    return codigo;
}*/

/*TODO:
    - Funcion inicializar_proceso: si hay espacio crea las estructuras administrativas y responde OK
    - Funcion suspender_proceso: se libera el espacio en memoria, y se pone en SWAP solo lo necesario. No hay que swapear las tablas
    - Funcion reanudar_proceso: Si hay espacio, la memoria lee del SWAP la info del procesos a reanudar, escribirlo en donde corresponde, liberar espacio del SWAP, actualizar estructuras administrativas y responder con OK
    - Funcion finalizar_proceso: se libera el espacio de memoria y se marca como libres sus entradas al SWAP
    - Funcion acceder_tabla: responde con el numero de marco correspondiente
    - Funcion leer_memoria: responde con el valor que esta en la posicion pedida
    - Funcion escribir_memoria: se escribe donde se pidio y se responde con OK
    - Funcion leer_pagina: se devuelve el contenido d ela pagina a partir del byte enviado que teine que coincidir con el byte 0 de la pagina (dentro memoria usuario)
    - Funcion actualiza_pagina: se escribe en la pagina a partir de byte 0 que igual sera enviado como direccion fisica (dentro memoria usuario)
    - Funcion memory_dump: crea un archivo con el tamaño de la memoria reservada por el proceso y escribir en el archivo todo el contenido actual de la memoria del mismo. El archivo de tiene que llamar “<PID>-<TIMESTAMP>.dmp” y guardarlo en el path que dice el .config
    - A medidad que se completen las funciones ver que cumplan los logs obligatorios 
*/