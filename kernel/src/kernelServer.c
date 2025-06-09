#include "kernelServer.h"
#include "cpuIO.h"

void mostrar_diccionario_io(t_log* logger) {
    void mostrar_elemento(char* key, void* value) {
        t_io* modulo_io = (t_io*) value;

        log_info(logger, "Dispositivo IO: %s", key);
        log_info(logger, "  - Cantidad de instancias: %d", list_size(modulo_io->instancias));
        if (!list_is_empty(modulo_io->instancias)){
            for (int i = 0; i < list_size(modulo_io->instancias); i++) {
                t_instancia_io* instancia = list_get(modulo_io->instancias, i);
                log_info(logger, "    - Socket FD: %d", instancia->socket_fd);
            }
        

            log_info(logger, "  - Procesos bloqueados: %d", queue_size(modulo_io->cola_bloqueados));
        }
    }

    dictionary_iterator(diccionario_io, mostrar_elemento);
}


int levantar_socket_servidor(char* puerto, t_log* logger) {
    struct addrinfo hints, *server_info;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    int err = getaddrinfo(NULL, puerto, &hints, &server_info);
    if (err != 0) {
        log_error(logger, "Error en getaddrinfo: %s", gai_strerror(err));
        return -1;
    }

    int fd_escucha = socket(server_info->ai_family, server_info->ai_socktype, server_info->ai_protocol);
    if (fd_escucha == -1) {
        log_error(logger, "Error creando socket");
        freeaddrinfo(server_info);
        return -1;
    }

    int opt = 1;
    setsockopt(fd_escucha, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    err = bind(fd_escucha, server_info->ai_addr, server_info->ai_addrlen);
    if (err != 0) {
        log_error(logger, "Fallo bind");
        close(fd_escucha);
        freeaddrinfo(server_info);
        return -1;
    }

    err = listen(fd_escucha, SOMAXCONN);
    if (err != 0) {
        log_error(logger, "Fallo listen");
        close(fd_escucha);
        freeaddrinfo(server_info);
        return -1;
    }

    log_info(logger, "Servidor escuchando en puerto %s", puerto);
    freeaddrinfo(server_info);
    return fd_escucha; 
}

void* escuchar_dispatch(void* arg) {
    t_args_escucha* args = (t_args_escucha*) arg;

    int socket_servidor = args->socket_servidor;
    t_log* logger = args->logger;

    while(1){
        int fd_cliente = accept(socket_servidor, NULL, NULL);

        if (fd_cliente == -1 ) {
            log_error(logger, "Error al aceptar conexión en DISPATCH");
            continue;
        }
        log_info(logger, "Se conectó una CPU (dispatch) - FD: %d", fd_cliente);

        char* nombre_cpu = recibirNombre(fd_cliente, logger);
        if (!nombre_cpu) {
            log_error(logger, "No se pudo recibir el nombre del CPU");
            close(fd_cliente);
            continue;
        }

        t_cpu_conectada* cpu = malloc(sizeof(t_cpu_conectada));
        cpu->fd_dispatch = fd_cliente;
        cpu->fd_interrupt = -1;
        cpu->nombre_cpu = nombre_cpu;
        cpu->disponible = true;

        list_add(cpus_conectadas, cpu);

        pthread_t hilo;
        t_args_escucha* args_hilo = malloc(sizeof(t_args_escucha));
        args_hilo->socket_servidor = fd_cliente;
        args_hilo->logger = logger;
        pthread_create(&hilo, NULL, hilo_syscalls_cpu, args_hilo);
        pthread_detach(hilo);
    }

    return NULL;
}

void* escuchar_interrupt(void* arg) {
    t_args_escucha* args = (t_args_escucha*) arg;

    int socket_servidor = args->socket_servidor;
    t_log* logger = args->logger;
    
    while(1){
        int fd_cliente = accept(socket_servidor, NULL, NULL);
        if (fd_cliente == -1) {
            log_error(logger, "Error al aceptar conexión en INTERRUPT");
            continue;
        }
        log_info(logger, "Se conectó una CPU (interrupt) - FD: %d", fd_cliente);


        char* nombre = recibirNombre(fd_cliente, logger);
        t_cpu_conectada* cpu = buscar_cpu_por_nombre(nombre);
        if (cpu) {
        cpu->fd_interrupt = fd_cliente;

        }
    }

    return NULL;
}

void* escuchar_IO(void* arg) {
    t_args_escucha* args = (t_args_escucha*) arg;

    int socket_servidor = args->socket_servidor;
    t_log* logger = args->logger;
    char* nombre;

    while(1){
        int fd_cliente = accept(socket_servidor, NULL, NULL);
        if (fd_cliente == -1) {
            log_error(logger, "Error al aceptar conexión en IO");
            return NULL;
        }

        log_info(logger, "Se conectó una IO - FD: %d", fd_cliente);

        nombre = recibirNombre(fd_cliente,  logger);


        registrarInstanciaIo(nombre, fd_cliente, logger);

        mostrar_diccionario_io(logger);
    }
    destruir_diccionario_io();

    return NULL;
}

void iniciar_hilos_conexiones(int socket_dispatch, int socket_interrupt, int socket_IO, t_log* logger) {
    pthread_t hilo_dispatch, hilo_interrupt, hilo_IO;

    static t_args_escucha args_dispatch;
    static t_args_escucha args_interrupt;
    static t_args_escucha args_IO;

    args_dispatch.socket_servidor = socket_dispatch;
    args_dispatch.logger = logger;

    args_interrupt.socket_servidor = socket_interrupt;
    args_interrupt.logger = logger;

    args_IO.socket_servidor = socket_IO;
    args_IO.logger = logger;

    pthread_create(&hilo_dispatch, NULL, escuchar_dispatch, &args_dispatch);
    pthread_create(&hilo_interrupt, NULL, escuchar_interrupt, &args_interrupt);
    pthread_create(&hilo_IO, NULL, escuchar_IO, &args_IO);

    pthread_join(hilo_dispatch, NULL);
    pthread_join(hilo_interrupt, NULL);
    pthread_join(hilo_IO, NULL);
}

char* recibirNombre(int fd_cliente, t_log* logger){
    t_paquete* paquete = recibir_paquete(fd_cliente);

    if (!paquete) {
        log_error(logger, "Fallo al recibir paquete");
        return NULL;
    }

    t_package* persona = deserializar_nombre(paquete->buffer);

    log_info(logger, "Código de operación recibido: %hhu", paquete->codigo_operacion);
    log_info(logger, "Nombre recibido correctamente: %s", persona->nombre);

    char* nombre = strdup(persona->nombre); 
    free(persona->nombre);
    free(persona);
    destruir_paquete(paquete);

    return nombre;
}

char* recibirNombreIO(int fd_cliente, t_log* logger){
    t_paquete* paquete = recibir_paquete(fd_cliente);
    if (!paquete) {
        log_error(logger, "Fallo al recibir paquete");
        EXIT_FAILURE;
    }

    t_package_io* dispositivo_io = deserializar_nombre_cpu_io(paquete->buffer);

    log_info(logger, "Código de operación recibido: %hhu", paquete->codigo_operacion);
    log_info(logger, "Nombre recibido correctamente: %s", dispositivo_io->nombre);
    log_info(logger, "Nombre recibido correctamente: %d", dispositivo_io->syscall);
    log_info(logger, "Nombre recibido correctamente: %d", dispositivo_io->timer);

    uint8_t ack = 100;
    send(fd_cliente, &ack, sizeof(uint8_t), 0);

    char* nombre = strdup(dispositivo_io->nombre); 
    free(dispositivo_io->nombre);
    free(dispositivo_io);
    destruir_paquete(paquete);

    return nombre;
}

void registrarInstanciaIo(char* nombre, int socket_fd, t_log* logger) {
    pthread_mutex_lock(&mutex_diccionario_io);

    t_io* modulo_io = dictionary_get(diccionario_io, nombre);

    if (!modulo_io) {
        modulo_io = malloc(sizeof(t_io));
        modulo_io->instancias = list_create();
        modulo_io->cola_bloqueados = queue_create();
        modulo_io->ocupado = false;
        dictionary_put(diccionario_io, strdup(nombre), modulo_io);
    }
    t_instancia_io* nueva_instancia = malloc(sizeof(t_instancia_io));
    nueva_instancia->socket_fd = socket_fd;
    nueva_instancia->disponible = false;

    list_add(modulo_io->instancias, nueva_instancia); 
    if (!queue_is_empty(modulo_io->cola_bloqueados)) {
        t_dispositivo_io* siguiente = queue_pop(modulo_io->cola_bloqueados);


        nueva_instancia->disponible = true;

        t_args_kernel_io* args = malloc(sizeof(t_args_kernel_io));
        args->socket_servidor = nueva_instancia->socket_fd;
        args->nombre = strdup(nombre);
        args->timer = siguiente->timer;
        args->pid = siguiente->pid;
        args->logger = logger; 
        args->t_dispositivo = modulo_io;

        pthread_t hilo;
        pthread_create(&hilo, NULL, enviar_handshake_kernel_io, args);
        pthread_detach(hilo);

        free(siguiente);
    }
    pthread_mutex_unlock(&mutex_diccionario_io);
}

void destruir_diccionario_io() {
    void destruir_io(void* elemento) {
        t_io* io = (t_io*) elemento;
        list_destroy(io->instancias);
        queue_destroy(io->cola_bloqueados);
        free(io);
    }

    dictionary_destroy_and_destroy_elements(diccionario_io, destruir_io);
    pthread_mutex_destroy(&mutex_diccionario_io);
}
