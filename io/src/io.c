#include "io.h"

int main(int argc, char* argv[]) {
    t_log* loggerIo = NULL;
    t_config* configIO = NULL;
    t_configIO cfg;

    loggerIo = iniciar_logger();
    configIO = iniciar_config_io();

    char* nombre = argv[1];
    if(nombre == NULL) {
        perror("No se ingreso el nombre del modulo IO");
        exit(EXIT_FAILURE);
    }
    log_info(loggerIo, "soy: %s",nombre);
    leer_config(&cfg, configIO);

    int socket_kernel = conectar_a_kernel(cfg.ip_kernel, cfg.puerto_kernel, loggerIo);
    
    t_paquete* paquete = crear_paquete_handshake(argv[1], HANDSHAKE, IO);
    uint32_t stream_size;
    void* stream = serializar_paquete(paquete, &stream_size);
    send(socket_kernel, stream, stream_size, 0);

    free(stream);
    destruir_paquete(paquete);
    int estado = 1;

    while (estado) {
        t_paquete* paquete = recibir_paquete(socket_kernel);
        if (!paquete) {
            log_warning(loggerIo, "Kernel desconectada (fd %d)", socket_kernel);
            break;
        }

        t_package_io* dispositivo_io = deserializar_nombre_cpu_io_kernel(paquete->buffer);
        if (dispositivo_io == NULL) {
            log_error(loggerIo, "Fallo al deserializar paquete de IO.");
            destruir_paquete(paquete);
            return EXIT_FAILURE;
        }

        log_info(loggerIo, "## PID: %d - Inicio de IO - Tiempo: %d",dispositivo_io->pid, dispositivo_io->timer);
        usleep(dispositivo_io->timer);
        log_info(loggerIo, "## PID: %d - Fin de IO",dispositivo_io->pid);

    }
    close(socket_kernel);
    log_info(loggerIo, "Terminaron las peticiones de IO");

    //cerrar_programa();
    cerrar_programa(loggerIo, configIO);

    return 0;
}


t_log* iniciar_logger(void) {
    t_log* logger = log_create("io.log", "LOGGER_INFO_IO",1, LOG_LEVEL_INFO);
    return logger;
}

t_config* iniciar_config_io(void) {
    t_config* nuevo_config = config_create("./io.config");
    return nuevo_config; 
}

void leer_config(t_configIO* cfg, t_config* config) {
    cfg->ip_kernel = config_get_string_value(config,"IP_KERNEL");
    cfg ->puerto_kernel = config_get_string_value(config, "PUERTO_KERNEL");
    cfg -> log_level = config_get_string_value(config, "LOG_LEVEL");
}

void cerrar_programa(t_log* logger, t_config* config){
    log_destroy(logger);
    config_destroy(config);
}