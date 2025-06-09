#include "kernelLargoPlazo.h"

void* planificarLargoPlazo(void* args) {
    t_args_planificador* datos = (t_args_planificador*) args;
    int socket_memoria = datos->socket;
    t_log* logger = datos->logger;
    t_kernelConfig cfg = datos->cfg;

    static t_args_pseudocodigo args_pseudocodigo;
    args_pseudocodigo.socket = datos->pseudocodigo.socket;
    args_pseudocodigo.pid = datos->pseudocodigo.pid;
    args_pseudocodigo.pathArchivo = datos->pseudocodigo.pathArchivo;
    args_pseudocodigo.tamanio = datos->pseudocodigo.tamanio;
    args_pseudocodigo.logger =  datos->pseudocodigo.logger;

    pthread_t pseudocodigo;

    if (strcmp(cfg.algoritmoCortoPlazo, "FIFO") == 0) {
        log_info(logger, "Algoritmo de corto plazo: FIFO");
    }

    new = queue_create();
    ready = queue_create();
    suspReady = queue_create();
    suspBlocked = queue_create();
    blocked = queue_create();
    exec = queue_create();

    log_info(logger, "Planificador de largo plazo frenado");
    log_info(logger, "Presione ENTER para iniciar el planificador...");
    leerConsola(logger);

    if (pthread_create(&pseudocodigo, NULL, enviar_pseudocodigo_a_memoria, (void*) &args_pseudocodigo) != 0) {
        perror("Error al crear hilo del planificador");
        exit(EXIT_FAILURE);
    }


    bool recibirOK = true;
    log_error(logger, "Entre al planificador ");

    t_proceso_en_new* proceso = malloc(sizeof(t_proceso_en_new));
    pthread_mutex_lock(&mutex_pid_procesos);
    proceso->pcb = crear_pcb(pid_procesos);
    pid_procesos++;
    pthread_mutex_unlock(&mutex_pid_procesos);
    proceso->pathArchivo = strdup(args_pseudocodigo.pathArchivo);
    proceso->tamanio = args_pseudocodigo.tamanio;
    int* contadorNew = dictionary_get(proceso->pcb->mestado, "NEW");
    if (contadorNew) (*contadorNew)++; 
    log_info(logger, "## PID (%d) Se crea el proceso - Estado: NEW", proceso->pcb->pid);
    proceso->estado = NEW;
    pthread_mutex_lock(&mutex_cola_new);
    queue_push(new, proceso);
    pthread_mutex_unlock(&mutex_cola_new);

    if (!dictionary_has_key(proceso->pcb->mestado, "NEW")) {
        log_error(logger, "Error: no existe la clave 'NEW' en el dictionary de mestado");
    }
    
    do {
        if(list_is_empty (cpus_conectadas)){
            log_error(logger, "No hay cpu conectadas todavia");
        }
        else{
            if (!queue_is_empty(suspReady)) {
                if (recibirOK){
                    
                    t_proceso_en_new* pcb_a_ready = queue_pop(suspReady);

                    int* contadorReady = dictionary_get(pcb_a_ready->pcb->mestado, "READY");
                    if (contadorReady) (*contadorReady)++;
                    pcb_a_ready->estado = READY;
                    pthread_mutex_lock(&mutex_cola_ready);
                    log_info(logger, "Proceso PID %d pasa de SUSP_READY a READY", pcb_a_ready->pcb->pid);
                    queue_push(ready, pcb_a_ready);
                    pthread_mutex_unlock(&mutex_cola_ready);
                    sem_post(&sem_procesos_en_ready);
                }
            }
            else if(!queue_is_empty(new)) {
                if (recibirOK){
                    
                    t_proceso_en_new* pcb_a_ready = queue_pop(new);
                    int* contadorReady = dictionary_get(pcb_a_ready->pcb->mestado, "READY");
                    if (contadorReady) (*contadorReady)++;
                    pcb_a_ready->estado = READY;
                    pthread_mutex_lock(&mutex_cola_ready);
                    log_info(logger, "Proceso PID %d pasa de NEW a READY", pcb_a_ready->pcb->pid);
                    queue_push(ready, pcb_a_ready);
                    pthread_mutex_unlock(&mutex_cola_ready);
                    sem_post(&sem_procesos_en_ready);
                }
            }

        }
        
        sem_wait(&sem_procesos_en_new);
        sem_wait(&sem_procesos_en_suspReady);
    }while(1);
    
    return NULL;
}

void* planificarCortoPlazo(void* args) {
    t_args_planificador_corto* datos = (t_args_planificador_corto*) args;
    int socket_memoria = datos->socket;
    t_log* logger = datos->logger;
    t_kernelConfig cfg = datos->cfg;
    log_info(logger, "Planificador de corto plazo");
    bool disponible = false;
    t_cpu_conectada* cpu = NULL;

    do{
        sem_wait(&sem_procesos_en_ready);
        if(!queue_is_empty(ready)) {
            pthread_mutex_lock(&mutex_cpu_conectadas);
            for (int i = 0; i < list_size(cpus_conectadas); i++) {
                cpu = list_get(cpus_conectadas, i);
                if (cpu->disponible){
                    cpu->disponible = false;
                    disponible = true;
                    break;
                }
                disponible = false;
            }
            pthread_mutex_unlock(&mutex_cpu_conectadas);
            if(disponible){
                t_proceso_en_new* pcb_a_exec = queue_pop(ready);
                cpu->proceso_ejecutando = pcb_a_exec->pcb;
                int* contadorExec = dictionary_get(pcb_a_exec->pcb->mestado, "EXEC");
                if (contadorExec) (*contadorExec)++;
                pcb_a_exec->estado = EXEC;
                pthread_mutex_lock(&mutex_cola_exec);
                log_info(logger, "PID (%d) pasa de READY a EXEC", pcb_a_exec->pcb->pid);
                queue_push(exec, pcb_a_exec);
                pthread_mutex_unlock(&mutex_cola_exec);
                
                pthread_mutex_lock(&mutex_cpu_conectadas);

                if(!list_is_empty (cpus_conectadas)){
                    t_cpu_conectada* cpu = list_get(cpus_conectadas, 0);
                    enviar_handshake_kernel_cpu(cpu->fd_dispatch, pcb_a_exec->pcb, logger);
                }
                pthread_mutex_unlock(&mutex_cpu_conectadas);
                sleep(2);
            
            }
            sem_wait(&sem_exit);
        }
    }while(1);

    return NULL;
}


void* planificarMedianoPlazo(void* args){

    t_args_planificador_corto* datos = (t_args_planificador_corto*) args;
    int socket_memoria = datos->socket;
    t_log* logger = datos->logger;
    t_kernelConfig cfg = datos->cfg;
    log_info(logger, "Planificador de mediano plazo");

    log_info(logger, "Timer: %d", cfg.tiempoSuspension);

    do{
        sem_wait(&sem_procesos_en_blocked);
        

        pthread_t hilo_monitoreo;

        if (pthread_create(&hilo_monitoreo, NULL, tiempoBloqueado, (void*) datos) != 0) {
            perror("Error al crear hilo del planificador de Mediano Plazo");
            exit(EXIT_FAILURE);
        }
        pthread_detach(hilo_monitoreo);
        sem_post(&sem_bloqueado);

    }while(1);

    return NULL;
}

void* tiempoBloqueado(void* args){
    t_args_planificador_corto* datos = (t_args_planificador_corto*) args;
    int socket_memoria = datos->socket;
    t_log* logger = datos->logger;
    t_kernelConfig cfg = datos->cfg;
    while(1){
        sem_wait(&sem_bloqueado);

        pthread_mutex_lock(&mutex_cola_blocked);
        if (!queue_is_empty(blocked)) {
            t_proceso_en_new* proceso = queue_pop(blocked);
            pthread_mutex_unlock(&mutex_cola_blocked);
        

            sleep(cfg.tiempoSuspension / 1000);

            if (proceso->estado == BLOCKED) {
                log_info(logger, "Proceso %d pasa de BLOCKED a SUSP_BLOCKED", proceso->pcb->pid);
                proceso->estado = SUSP_BLOCKED;

                pthread_mutex_lock(&mutex_cola_suspBlocked);
                queue_push(suspBlocked, proceso);
                pthread_mutex_unlock(&mutex_cola_suspBlocked);

                // avisar_a_memoria_swap(proceso);
                sem_post(&sem_procesos_en_new);    
                sem_post(&sem_procesos_en_suspReady);
            }
        }
    }
}


void* enviar_handshake_kernel_cpu(int socket, t_pcb* pcb, t_log* logger){
    t_paquete* paquete = crear_paquete_handshake_kernel_cpu(HANDSHAKE, pcb->pid, pcb->pc);
    uint32_t bytes = 0;
    void* a_enviar = serializar_paquete_kernel_cpu(paquete, &bytes);

    //ssize_t enviados = send(socket, a_enviar, bytes, 0);
    send(socket, a_enviar, bytes, 0);
    destruir_paquete_kernel_cpu(paquete);
    free(a_enviar);

    return NULL;

}


void leerConsola(t_log* logger){
    char* leido;
	leido = readline(">> ");
	while(strcmp(leido, "") != 0){
		free(leido);
		leido = readline(">> ");
	}
	free(leido);
}