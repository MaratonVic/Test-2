#include "memoria.h"

ConfigMemoria cfg;
t_log *logger;
static int socket_escucha;
static t_config *config;
// static t_dictionary* procesos_memoria = NULL;

int main(int argc, char *argv[])
{

    signal(SIGINT, signal_handler);

    config = crear_config();
    cfg = leer_config(config);
    logger = crear_logger(cfg.log_level);
    
    iniciar_memoria_bitarray(cfg.tam_memoria, cfg.tam_pagina);
    socket_escucha = iniciar_servidor(cfg.puerto_escucha, logger);
    if (iniciar_ram(cfg.tam_memoria) < 0)
    {
        log_error(logger, "Error al iniciar el tamanio de la memoria");
    }
    iniciar_lista_procesos();
    while (1)
    {
        int fd_cliente = accept(socket_escucha, NULL, NULL);
        t_datos_cliente* datos = malloc(sizeof(t_datos_cliente));
        datos->fd_cliente = fd_cliente;
        pthread_t nuevo_hilo;
        pthread_create(&nuevo_hilo, NULL, (void *)controlador_cliente, (void *)datos);
        pthread_detach(nuevo_hilo);
    }

    // esperar_cliente(socket_escucha);
    return 0;
}

/* TODO: Hacer recibir_operacion, completar que los clientes van a usar */
void controlador_cliente(void *args)
{
    t_datos_cliente* datos = (t_datos_cliente *)args;
    int fd_cliente = datos->fd_cliente;
    t_paquete *paquete = recibir_paquete(fd_cliente);

    if (!paquete)
    {
        log_error(logger, "Fallo al recibir paquete controlador");
        exit(EXIT_FAILURE);
    }
    t_package *handshake = deserializar_nombre(paquete->buffer);

    int cod_op = paquete->codigo_operacion;
    uint8_t cod_mod = handshake->modulo;

    free(handshake->nombre);
    free(handshake);
    destruir_paquete(paquete);

    if (cod_op == HANDSHAKE)
    {
        if (cod_mod == KERNEL)
        {
            log_info(logger, "## Kernel Conectado - FD del socket: %d\n", fd_cliente);
            controlador_kernel(fd_cliente);
        }
        else
        {
            log_info(logger, "## CPU Conectada - FD del socket: %d\n", fd_cliente);
            t_buffer* buffer_protocolo = buffer_protocolo_cpu(cfg.entradas_por_tabla, cfg.tam_pagina, cfg.cantidad_niveles);
            t_paquete* paquete_protocolo = crear_paquete(HANDSHAKE, buffer_protocolo);
            enviar_paquete(fd_cliente, paquete_protocolo);
            destruir_paquete(paquete_protocolo);

            controlador_cpu(fd_cliente);
        }
    }
    else
    {
        log_error(logger, "Primero debe realizar un handshake para entablar comunicacion");
    }

    free(datos);
}

void controlador_kernel(int fd_cliente) {
    while(1){
        t_paquete* paquete = recibir_paquete(fd_cliente);
        if (!paquete) {
            log_error(logger, "Fallo al recibir paquete kernel");
            exit(EXIT_FAILURE);
        }
        int cod_op = paquete->codigo_operacion;
        log_info(logger, "Código de operación recibido: %hhu", paquete->codigo_operacion);
        
        switch (cod_op) {
            case DESTROY_PROC:
            usleep(1000 * cfg.retardo_memoria);
            log_info(logger, "Kernel solicito la finalizacion del proceso");
            
            //destruir_proceso();
            break;

            case INIT_DUMP_MEMORY:
            usleep(1000 * cfg.retardo_memoria);
            uint32_t pid_dump = deserializar_syscall_dump_pid(paquete->buffer);
            log_info(logger, "Kernel solicito un dump de memoria del pid: %d", pid_dump);
            //realizar_memory_dump(paquete->buffer);
            
            /*enviar_paquete();
            destruir_paquete();*/
            break;

            case SOL_MOCK:
            usleep(1000 * cfg.retardo_memoria);
            log_info(logger, "Kernel solicito el espacio disponible en memoria");
            
            uint32_t memoria_disp=espacio_disponible();


            t_buffer* mem_disp_buffer = malloc(sizeof(t_buffer));
            if (!mem_disp_buffer) {
                log_error(logger, "No se pudo asignar memoria para buffer");
                return;
            }

            mem_disp_buffer->size = sizeof(uint32_t);
            mem_disp_buffer->stream = malloc(mem_disp_buffer->size);
            if (!mem_disp_buffer->stream) {
                free(mem_disp_buffer);
                log_error(logger, "No se pudo asignar memoria para stream");
            return;
            }

            memcpy(mem_disp_buffer->stream, &memoria_disp, sizeof(uint32_t));
            mem_disp_buffer->size=sizeof(uint32_t);
            t_paquete* mem_disp = crear_paquete(SOL_MOCK, mem_disp_buffer);
            
            
            enviar_paquete(fd_cliente,mem_disp);
            destruir_paquete(mem_disp);
            log_info(logger, "Fue enviado el espacio disponible a Kernel");
            break;

            case INIT_PROC:
            usleep(1000 * cfg.retardo_memoria);
            log_error(logger, "Entre");
            
            if(iniciar_proceso_en_memoria(paquete->buffer)){
                char* mensaje = "ok";
                t_paquete* respuesta = crear_paquete_instruccion(mensaje, OK);
                enviar_paquete(fd_cliente, respuesta);
                destruir_paquete(respuesta);
            }else {
                char*mensaje = "error";
                t_paquete* respuesta = crear_paquete_instruccion(mensaje, ERROR_MEMORIA);
                enviar_paquete(fd_cliente, respuesta);
                destruir_paquete(respuesta);
            }
            break;

            case SUSP:
                usleep(1000 * cfg.retardo_memoria);

                uint32_t pid_susp = deserializar_syscall_dump_pid(paquete->buffer);
                log_info(logger, "Kernel solicito un susp de memoria del pid: %d", pid_susp);
                break;
            
            default:
            log_error(logger, "Operacion invalida \n");
            break;
        }
        destruir_paquete(paquete);

    }
}

void controlador_cpu(int fd_cliente) {
    while (1) {
        t_paquete* paquete = recibir_paquete(fd_cliente);
        if (!paquete) {
            log_error(logger, "Fallo al recibir paquete");
            exit(EXIT_FAILURE);
        }

        int cod_op = paquete->codigo_operacion;

        switch (cod_op) {
            case ACCESO_A_PAGINA: {
                usleep(cfg.retardo_memoria * 1000);

                int frame=NULL;
                int pid;
                uint32_t niveles = cfg.cantidad_niveles;
                uint32_t entradas[niveles];
                int offset = 0;
                memcpy(&pid,paquete->buffer->stream + offset, sizeof(int));
                offset += sizeof(int);
                memcpy(entradas,paquete->buffer->stream + offset,sizeof(uint32_t)*niveles);
                log_info(logger, "ACCESO_A_PAGINA | PID=%d | Índices=[%u",pid, entradas[0]);
                for (uint32_t i = 1; i < niveles; i++) {
                    log_info(logger, ",%u", entradas[i]);
                }
                log_info(logger, "]");
                
                
                t_tabla_pagina* raiz = tabla_principal_de_pid(pid);
                if (!raiz) {
                    log_error(logger, "PID %d sin tabla de páginas", pid);
                    int err = -1;
                    t_paquete_resp* resp = crear_paquete_simple(RESPUESTA_MARCO, &frame, sizeof(int));
                    enviar_paquete(fd_cliente, resp);
                    destruir_paquete_resp(resp);
                    }
                    
                
                frame = buscar_marco(raiz, entradas, 0);
                log_info(logger, "PID %d -> Frame=%d", pid, frame);
                
                
                t_paquete_resp* resp = crear_paquete_simple(RESPUESTA_MARCO, &frame, sizeof(int));
                enviar_paquete(fd_cliente, resp);
                destruir_paquete_resp(resp);
                break;
                }

            
            case SOLICITAR_INSTRUCCION: {
                usleep(cfg.retardo_memoria * 1000);
                char* instruccion = buscar_instruccion(paquete->buffer);
                if (!instruccion) {
                    log_error(logger, "Instrucción no encontrada");
                } else {
                    t_paquete* paquete_instruccion = crear_paquete_instruccion(instruccion, INSTRUCCION_SOLICITADA);
                    if (enviar_paquete(fd_cliente, paquete_instruccion) == 0) {
                        log_info(logger, "Instrucción enviada correctamente");
                    } else {
                        log_error(logger, "Fallo al enviar instrucción");
                    }
                    destruir_paquete(paquete_instruccion);
                }
                break;
            }

            default:
                log_error(logger, "Operación inválida recibida: %d", cod_op);
                break;
        }

        destruir_paquete(paquete);
    }
}

static void cerrar_programa(void)
{
    log_destroy(logger);
    config_destroy(config);

}

void signal_handler(int signal)
{
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