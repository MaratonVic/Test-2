#include "kernelSyscall.h"
#include "instrucciones.h"

void* hilo_syscalls_cpu(void* arg) {
    t_args_escucha* args = (t_args_escucha*) arg;
    int fd = args->socket_servidor;
    t_log* logger = args->logger;
    t_package_io* dispositivo_io = NULL; //Si FALLA IO MIRAR ACA
    t_package_syscall_exit* syscallExit = NULL;
    t_package_syscall_exit* syscallDump = NULL;
    free(args);  

    while (1) {
        t_paquete* paquete = recibir_paquete(fd);
        if (!paquete) {
            log_warning(logger, "CPU desconectada (fd %d)", fd);
            break;
        }

        pthread_mutex_lock(&mutex_pid_procesos);
        log_info(logger, "## (%d) - Solicitó syscall: %s",pid_procesos, syscall_recibida(paquete->codigo_operacion));
        pthread_mutex_unlock(&mutex_pid_procesos);
        switch (paquete->codigo_operacion)
        {
            case INIT_IO:
                dispositivo_io = deserializar_nombre_cpu_io(paquete->buffer);
                procesar_syscall_io(dispositivo_io, logger);
                free(dispositivo_io->nombre);
                free(dispositivo_io);
                break;
            case INIT_PROC:
                procesar_syscall_init_proc(logger, "pseudocodigo.txt", 256);
                break;
            case INIT_DUMP_MEMORY:
                if (paquete->buffer->size < sizeof(uint32_t) + sizeof(uint32_t)) {
                    log_error(logger, "Error: buffer recibido muy pequeño para syscall EXIT/DUMP");
                    break;
                }
                syscallDump = deserializar_sycall_exit(paquete->buffer);
                log_error(logger, "Syscall pid recibido: %d y con nombre: %s", syscallDump->pid, syscallDump->nombre);
                sleep(5);
                procesar_syscall_dump_memory(logger, syscallDump);
                free(syscallDump->nombre);
                free(syscallDump);
                break;           

            case INIT_EXIT:
                if (paquete->buffer->size < sizeof(uint32_t) + sizeof(uint32_t)) {
                    log_error(logger, "Error: buffer recibido muy pequeño para syscall EXIT");
                    break;
                }
                syscallExit = deserializar_sycall_exit(paquete->buffer);
                log_error(logger, "Syscall pid recibido: %d y con nombre: %s", syscallExit->pid, syscallExit->nombre);
                sleep(5);
                procesar_syscall_exit(logger, syscallExit);
                free(syscallExit->nombre);
                free(syscallExit);
                break;
            default:
                log_error(logger, "Syscall desconocida por el Sistema Operativo");
                break;
        }
        destruir_paquete(paquete); 

    }

    close(fd);
    return NULL;
}

void procesar_syscall_dump_memory(t_log* logger, t_package_syscall_exit* syscallDump){
    if(!queue_is_empty(exec)){
        pthread_mutex_lock(&mutex_cpu_conectadas);
        if(!list_is_empty (cpus_conectadas)){
            t_cpu_conectada* cpu = buscar_cpu_por_nombre(syscallDump->nombre);
            if(cpu != NULL){
                t_proceso_en_new* pcb_a_block = queue_peek(exec);
                if(pcb_a_block->pcb->pid == syscallDump->pid){
                    cpu->disponible = true;
                    pcb_a_block = queue_pop(exec);
                    int* contadorBlocked = dictionary_get(pcb_a_block->pcb->mestado, "BLOCKED");
                    if (contadorBlocked) (*contadorBlocked)++;
                    pcb_a_block->estado = BLOCKED;
                    pthread_mutex_lock(&mutex_cola_blocked);
                    queue_push(blocked, pcb_a_block);
                    pthread_mutex_unlock(&mutex_cola_blocked);
                    

                    log_info(logger, "PID (%d) pasa de EXEC a BLOCKED", pcb_a_block->pcb->pid);
                }
            }

        }
        pthread_mutex_unlock(&mutex_cpu_conectadas);
    }
    sem_post(&sem_exit);
    sem_post(&sem_procesos_en_suspReady);
    sem_post(&sem_procesos_en_blocked);
}

void procesar_syscall_exit(t_log* logger, t_package_syscall_exit* syscallExit){
    if(!queue_is_empty(exec)){
        pthread_mutex_lock(&mutex_cpu_conectadas);
        if(!list_is_empty (cpus_conectadas)){
            t_cpu_conectada* cpu = buscar_cpu_por_nombre(syscallExit->nombre);
            if(cpu != NULL){
                t_proceso_en_new* pcb_a_exec = queue_peek(exec);
                if(pcb_a_exec->pcb->pid == syscallExit->pid){
                    cpu->disponible = true;
                    pcb_a_exec = queue_pop(exec);
                    pcb_a_exec->estado = EXIT;
                    log_info(logger, "PID (%d) pasa de EXEC a EXIT", pcb_a_exec->pcb->pid);
                }
            }

        }
        pthread_mutex_unlock(&mutex_cpu_conectadas);
    }
    sem_post(&sem_exit);
    sem_post(&sem_procesos_en_suspReady);

}

void procesar_syscall_init_proc(t_log* logger, char* archivo, uint32_t tamanio ){

    // static t_args_pseudocodigo args_pseudocodigo;
    // args_pseudocodigo.socket = socketMemoriaSyscall;
    // args_pseudocodigo.pid = pid_procesos;
    // args_pseudocodigo.pathArchivo = archivo;
    // args_pseudocodigo.tamanio = tamanio;
    // args_pseudocodigo.logger = logger;

    // pthread_t pseudocodigo;

    // if (pthread_create(&pseudocodigo, NULL, enviar_pseudocodigo_a_memoria, (void*) &args_pseudocodigo) != 0) {
    //     perror("Error al crear hilo del planificador");
    //     exit(EXIT_FAILURE);
    // }

    // pthread_detach(pseudocodigo);
    t_proceso_en_new* proceso = malloc(sizeof(t_proceso_en_new));
    pthread_mutex_lock(&mutex_pid_procesos);
    proceso->pcb  = crear_pcb(pid_procesos);
    pid_procesos++;
    pthread_mutex_unlock(&mutex_pid_procesos);
    proceso->pathArchivo = strdup(archivo);
    proceso->tamanio = tamanio;

    int* contadorNew = dictionary_get(proceso->pcb->mestado, "NEW");
    if (contadorNew) (*contadorNew)++; 

    pthread_mutex_lock(&mutex_cola_new);
    queue_push(new, proceso);
    pthread_mutex_unlock(&mutex_cola_new);
    sem_post(&sem_procesos_en_new);
    
    log_info(logger, "PID (%d) creado y enviado a NEW", proceso->pcb->pid);

}

void procesar_syscall_io(t_package_io* dispositivo_io, t_log* logger) {
    pthread_mutex_lock(&mutex_diccionario_io);
    t_io* io = dictionary_get(diccionario_io, dispositivo_io->nombre);
    pthread_mutex_unlock(&mutex_diccionario_io);

    if (!io) {
        log_warning(logger, "El módulo de I/O '%s' no está conectado. Proceso [%d] será finalizado (EXIT).", dispositivo_io->nombre, dispositivo_io->pid);
        // int estado = 1;
        // while(estado){
        //     pthread_mutex_lock(&mutex_cpu_conectadas);
        //     if(!queue_is_empty(exec)){
        //         if(!list_is_empty (cpus_conectadas)){
        //             t_cpu_conectada* cpu = buscar_cpu_por_nombre(dispositivo_io->nombreCpu);
        //             if(cpu != NULL){
        //                 t_proceso_en_new* pcb_a_exec = queue_peek(exec);
        //                 if(pcb_a_exec->pcb->pid == dispositivo_io->pid){
        //                     cpu->disponible = true;
        //                     pcb_a_exec = queue_pop(exec);
        //                     pcb_a_exec->estado = EXIT;
        //                     log_info(logger, "PID (%d) pasa de EXEC a EXIT", pcb_a_exec->pcb->pid);
        //                     estado = 0;
        //                 }
        //             }
        //         }
        //     }
        //     pthread_mutex_unlock(&mutex_cpu_conectadas);
        //     sleep(1);
        // }
        // sem_post(&sem_exit);
        // sem_post(&sem_procesos_en_suspReady);
        // sem_post(&sem_procesos_en_blocked);
        return;
    }

    bool instancia_libre_encontrada = false;
    t_instancia_io* instancia_libre = NULL;

    pthread_mutex_lock(&mutex_diccionario_io);
    for (int i = 0; i < list_size(io->instancias); i++) {
        t_instancia_io* inst = list_get(io->instancias, i);
        if (inst->disponible == false) {  
            instancia_libre_encontrada = true;
            instancia_libre = inst;
            inst->disponible = true;  
            break;
        }
    }
    pthread_mutex_unlock(&mutex_diccionario_io);

    if (instancia_libre_encontrada) {
        t_args_kernel_io* args = malloc(sizeof(t_args_kernel_io));
        args->socket_servidor = instancia_libre->socket_fd;
        args->nombre = strdup(dispositivo_io->nombre);
        args->timer = dispositivo_io->timer;
        args->pid = dispositivo_io->pid;
        args->logger = logger;
        args->t_dispositivo = io;
        args->nombreCpu = strdup(dispositivo_io->nombreCpu);

        log_info(logger, "[PID %d] Enviando syscall de I/O '%s' con duración %d ms (socket %d)", dispositivo_io->pid, args->nombre, args->timer, args->socket_servidor);

        pthread_t hilo;
        pthread_create(&hilo, NULL, enviar_handshake_kernel_io, args);
        pthread_detach(hilo);
    } else {
        t_dispositivo_io* bloqueado = malloc(sizeof(t_dispositivo_io));
        bloqueado->pid = dispositivo_io->pid;
        bloqueado->timer = dispositivo_io->timer;

        pthread_mutex_lock(&mutex_diccionario_io);
        queue_push(io->cola_bloqueados, bloqueado);
        pthread_mutex_unlock(&mutex_diccionario_io);

        log_info(logger, "[PID %d] Todas las instancias de '%s' están ocupadas. Se encola en BLOCKED por %d ms", bloqueado->pid, dispositivo_io->nombre, bloqueado->timer);
    }
}


const char* syscall_recibida(int modulo) {
    switch (modulo) {
        case CPU: return "CPU";
        case KERNEL: return "KERNEL";
        case INIT_IO: return "IO";
        case INIT_PROC: return "INIT_PROC";
        case INIT_DUMP_MEMORY: return "DUMP_MEMORY";
        case INIT_EXIT: return "EXIT";
        default: return "DESCONOCIDO";
    }
}
