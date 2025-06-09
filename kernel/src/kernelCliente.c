#include "kernelCliente.h"
#include "instrucciones.h"

int conectar_cliente(char* ip, char* puerto, t_log* logger, char* nombre) {
    struct addrinfo hints, *servinfo;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(ip, puerto, &hints, &servinfo) != 0) {
        log_error(logger, "[%s] Error en getaddrinfo", nombre);
        return -1;
    }

    int sock = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);
    if (connect(sock, servinfo->ai_addr, servinfo->ai_addrlen) == -1) {
        log_error(logger, "[%s] No se pudo conectar", nombre);
        freeaddrinfo(servinfo);
        close(sock);
        return -1;
    }

    log_info(logger, "[%s] Conectado a %s:%s", nombre, ip, puerto);
    freeaddrinfo(servinfo);
    return sock;
}

int conectar_a_memoria(char* ip, char* puertoMemoria, t_log* logger) {
    return conectar_cliente(
        ip,
        puertoMemoria,
        logger, "Memoria"
    );
}

void identificar_a_memoria(int socket){
    t_paquete* paquete = crear_paquete_handshake("Kernel", HANDSHAKE, KERNEL);
    uint32_t stream_size;
    void* stream = serializar_paquete(paquete, &stream_size);
    send(socket, stream, stream_size, 0);

    free(stream);
    destruir_paquete(paquete);

}

void* enviar_handshake_kernel_io(void* arg) {
    t_args_kernel_io* args = (t_args_kernel_io*) arg;
    int socket = args->socket_servidor;
    t_log* logger = args->logger;
    char* nombre_modulo = strdup(args->nombre);
    uint32_t timer = args->timer;
    uint32_t pid = args->pid;
    t_io* io = args->t_dispositivo;
    char* nombreCpu = strdup(args->nombreCpu);

    int estado = 1;
    while(estado){
        pthread_mutex_lock(&mutex_cpu_conectadas);
        if(!queue_is_empty(exec)){
            if(!list_is_empty (cpus_conectadas)){
                t_cpu_conectada* cpu = buscar_cpu_por_nombre(nombreCpu);
                if(cpu != NULL){
                    t_proceso_en_new* pcb_a_exec = queue_peek(exec);
                    if(pcb_a_exec->pcb->pid == args->pid){
                        cpu->disponible = true;
                        pcb_a_exec = queue_pop(exec);
                        pcb_a_exec->estado = BLOCKED;
                        int* contadorBlocked = dictionary_get(pcb_a_exec->pcb->mestado, "BLOCKED");
                        if (contadorBlocked) (*contadorBlocked)++;
                        pthread_mutex_lock(&mutex_cola_blocked);
                        queue_push(blocked, pcb_a_exec);
                        pthread_mutex_unlock(&mutex_cola_blocked);
                        log_info(logger, "PID (%d) pasa de EXEC a BLOCKED", pcb_a_exec->pcb->pid);
                        estado = 0;
                    }
                }
            }
        }
        pthread_mutex_unlock(&mutex_cpu_conectadas);
        sleep(1);
    }
    sem_post(&sem_exit);
    sem_post(&sem_procesos_en_suspReady);
    sem_post(&sem_procesos_en_blocked);
    free(args);

    if (!io || socket <= 0) {
        log_error(logger, "Socket inválido o estructura IO nula para %s", nombre_modulo);
        free(nombre_modulo);
        return NULL;
    }

    t_paquete* paquete = crear_paquete_handshake_syscall_cpu_io_prueba(nombre_modulo, HANDSHAKE, INST_IO, timer, pid);
    uint32_t bytes = 0;
    void* a_enviar = serializar_paquete_syscall_cpu_io(paquete, &bytes);

    ssize_t enviados = send(socket, a_enviar, bytes, 0);
    destruir_paquete_syscall_cpu_io(paquete);
    free(a_enviar);

    if (enviados <= 0) {
        log_warning(logger, "[%s] Error al enviar syscall a socket %d", nombre_modulo, socket);
        goto desconectado;
    }

    log_info(logger, "[%s] El proceso en socket %d se pasa a estado EXIT", nombre_modulo, socket);

    pthread_mutex_lock(&mutex_diccionario_io);


    t_instancia_io* instancia = NULL;
    for (int i = 0; i < list_size(io->instancias); i++) {
        t_instancia_io* inst = list_get(io->instancias, i);
        if (inst && inst->socket_fd == socket) {
            instancia = inst;
            break;
        }
    }


    if (instancia) {
        instancia->disponible = false;
    }

    if (instancia && !queue_is_empty(io->cola_bloqueados)) {
        t_dispositivo_io* siguiente = queue_pop(io->cola_bloqueados);

        t_args_kernel_io* nuevos_args = malloc(sizeof(t_args_kernel_io));
        nuevos_args->socket_servidor = instancia->socket_fd;
        nuevos_args->nombre = strdup(nombre_modulo);
        nuevos_args->timer = siguiente->timer;
        nuevos_args->pid = siguiente->pid;
        nuevos_args->logger = logger;
        nuevos_args->t_dispositivo = io;
        nuevos_args->nombreCpu = strdup(nombreCpu);

        log_info(logger, "[%s] Reasignando proceso PID %d a socket %d", nombre_modulo, siguiente->pid, nuevos_args->socket_servidor);

        pthread_t hilo_nuevo;
        pthread_create(&hilo_nuevo, NULL, enviar_handshake_kernel_io, nuevos_args);
        pthread_detach(hilo_nuevo);

        free(siguiente);
    }

    pthread_mutex_unlock(&mutex_diccionario_io);
    free(nombre_modulo);
    return NULL;


desconectado:
    pthread_mutex_lock(&mutex_diccionario_io);
        for (int i = 0; i < list_size(io->instancias); i++) {
        t_instancia_io* inst = list_get(io->instancias, i);
            if (inst && inst->socket_fd == socket) {
                list_remove(io->instancias, i);
                free(inst);
            break;
        }
    }

    if (list_is_empty(io->instancias)) {
        t_io* eliminado = dictionary_remove(diccionario_io, nombre_modulo);
        if (eliminado) {
            queue_destroy_and_destroy_elements(eliminado->cola_bloqueados, free);
            list_destroy(eliminado->instancias);
            free(eliminado);
        }
    }

    pthread_mutex_unlock(&mutex_diccionario_io);
    close(socket);
    free(nombre_modulo);
    return NULL;
}

void* enviar_pseudocodigo_a_memoria(void* arg) {
    t_args_pseudocodigo* args = (t_args_pseudocodigo*) arg;
    int socket = args->socket;
    uint32_t pid = args->pid;
    char* pathArchivo = args->pathArchivo;
    uint32_t tamanio = args->tamanio;

    t_paquete* paquete = crear_paquete_handshake_pseudocodigo(pathArchivo, INIT_PROC, KERNEL, pid, tamanio);
    uint32_t stream_size;
    void* stream = serializar_paquete(paquete, &stream_size);
    send(socket, stream, stream_size, 0);

    free(stream);
    destruir_paquete(paquete);

    return NULL;
}

