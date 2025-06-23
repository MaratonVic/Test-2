#include "kernelLargoPlazo.h"

void* planificarLargoPlazo(void* args) {

    new = list_create();
    ready = list_create();
    suspReady = list_create();
    suspBlocked = list_create();
    blocked = list_create();
    exec = list_create();

    t_args_planificador* datos = (t_args_planificador*) args;
    int socket_memoria = datos->socket;
    t_log* logger = datos->logger;
    t_kernelConfig cfg = datos->cfg;

    socketMemoriaSyscall = socket_memoria;

    static t_args_pseudocodigo args_pseudocodigo;
    args_pseudocodigo.socket = datos->pseudocodigo.socket;
    args_pseudocodigo.pid = datos->pseudocodigo.pid;
    args_pseudocodigo.pathArchivo = datos->pseudocodigo.pathArchivo;
    args_pseudocodigo.tamanio = datos->pseudocodigo.tamanio;
    args_pseudocodigo.logger =  datos->pseudocodigo.logger;


    log_info(logger, "Planificador de largo plazo frenado");
    log_info(logger, "Presione ENTER para iniciar el planificador...");
    leerConsola(logger);

    t_proceso_en_new* proceso = malloc(sizeof(t_proceso_en_new));

    pthread_mutex_lock(&mutex_pid_procesos);
    proceso->pcb = crear_pcb(pid_procesos);
    proceso->estimacionRafaga = cfg.estimacionInicial;
    pid_procesos++;
    pthread_mutex_unlock(&mutex_pid_procesos);

    proceso->pathArchivo = strdup(args_pseudocodigo.pathArchivo);
    proceso->tamanio = args_pseudocodigo.tamanio;
    int* contadorNew = dictionary_get(proceso->pcb->mestado, "NEW");
    if (contadorNew) (*contadorNew)++;

    t_temporal* tiempoNew = dictionary_get(proceso->pcb->mtiempo, "NEW");
    if (tiempoNew) 	temporal_resume (tiempoNew); 

    log_info(logger, "## PID (%d) Se crea el proceso - Estado: NEW", proceso->pcb->pid);
    proceso->estado = NEW;

    pthread_mutex_lock(&mutex_cola_new);
    list_add(new, proceso);
    pthread_mutex_unlock(&mutex_cola_new);

    if (!dictionary_has_key(proceso->pcb->mestado, "NEW")) {
        log_error(logger, "Error: no existe la clave 'NEW' en el dictionary de mestado");
    }
    
    do {
        if(list_is_empty (cpus_conectadas)){
            log_error(logger, "No hay cpu conectadas todavia");
        }
        else{

            if (strcmp(cfg.algoritmoIngresoReady, "FIFO") == 0) {
                largoPlazoFifo(logger, cfg);
            }
            else{
                masChicoPrimero(logger, cfg);
            }

        }
        
        sem_wait(&sem_procesos_en_new);
        sem_wait(&sem_procesos_en_suspReady);
    }while(1);
    
    return NULL;
}

void* planificarCortoPlazo(void* args) {
    t_args_planificador_corto* datos = (t_args_planificador_corto*) args;
    //int socket_memoria = datos->socket;
    t_log* logger = datos->logger;
    t_kernelConfig cfg = datos->cfg;

    do{
        sem_wait(&sem_procesos_en_ready);
        if (strcmp(cfg.algoritmoCortoPlazo, "FIFO") == 0) {
            cortoPlazoFifo(logger);
        }
        else if (strcmp(cfg.algoritmoCortoPlazo, "SJFSD") == 0) {
            SJF_sin_desalojo(logger, cfg);
        }
        else if (strcmp(cfg.algoritmoCortoPlazo, "SJFCD") == 0) {
            SJF_con_desalojo(logger, cfg);
        }
        else{
            log_error(logger, "Algoritmo %s, no valido", cfg.algoritmoCortoPlazo);
        }
    }while(1);

    return NULL;
}


void* planificarMedianoPlazo(void* args){

    t_args_planificador_corto* datos = (t_args_planificador_corto*) args;
    //int socket_memoria = datos->socket;
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

bool tiene_pid(void* elem) {
    t_proceso_en_new* proceso = (t_proceso_en_new*) elem;
    return proceso->pcb->pid == pid_susBloqueo;
}

bool tiene_pid_io(void* elem) {
    t_proceso_en_new* proceso = (t_proceso_en_new*) elem;
    return proceso->pcb->pid == pid_bloqueo;
}

bool tiene_pid_dump(void* elem) {
    t_proceso_en_new* proceso = (t_proceso_en_new*) elem;
    return proceso->pcb->pid == pid_dump;
}

void* tiempoBloqueado(void* args){
    t_args_planificador_corto* datos = (t_args_planificador_corto*) args;
    //int socket_memoria = datos->socket;
    t_log* logger = datos->logger;
    t_kernelConfig cfg = datos->cfg;
    
    while(1){
        sem_wait(&sem_bloqueado);
        pthread_mutex_lock(&mutex_cola_blocked);
        if (!list_is_empty(blocked)){
            t_proceso_en_new* proceso = list_find(blocked, tiene_pid);
            pthread_mutex_unlock(&mutex_cola_blocked);
        

            sleep(cfg.tiempoSuspension / 1000);

            if (proceso->estado == BLOCKED) {
                if(list_remove_element(blocked, proceso)){

                    t_temporal* tiempoBlocked = dictionary_get(proceso->pcb->mtiempo,  "BLOCKED");
                    if (tiempoBlocked) temporal_stop(tiempoBlocked);
                    log_info(logger, "Proceso %d pasa de BLOCKED a SUSP_BLOCKED", proceso->pcb->pid);
                    proceso->estado = SUSP_BLOCKED;
                    int* contadorSusBlocked = dictionary_get(proceso->pcb->mestado, "SUSP_BLOCKED");
                    if (contadorSusBlocked) (*contadorSusBlocked)++;

                    t_temporal* tiempoSuspBlocked = dictionary_get(proceso->pcb->mtiempo,  "SUSP_BLOCKED");
                    if (tiempoSuspBlocked) temporal_resume(tiempoSuspBlocked);

                    pthread_mutex_lock(&mutex_cola_suspBlocked);
                    list_add(suspBlocked, proceso);
                    pthread_mutex_unlock(&mutex_cola_suspBlocked);

                    enviar_paquete_susp(socketMemoriaSyscall, proceso->pcb->pid);;
                    sem_post(&sem_procesos_en_new);    
                    sem_post(&sem_procesos_en_suspReady);
                }
            }
        }
    }
}


void* termina_IO(void* args){
    t_args_planificador_corto* datos = (t_args_planificador_corto*) args;
    //int socket_memoria = datos->socket;
    t_log* logger = datos->logger;
    //t_kernelConfig cfg = datos->cfg;
    free(datos);

    while (1) {
        sem_wait(&sem_ioTerminada);
        proceso_blocked_ready( logger,tiene_pid_io);
 
    }
}

void* termina_dump(void* args){
    t_args_planificador_corto* datos = (t_args_planificador_corto*) args;
    //int socket_memoria = datos->socket;
    t_log* logger = datos->logger;
    //t_kernelConfig cfg = datos->cfg;
    free(datos);

    while (1) {
        sem_wait(&sem_dumpTerminada);
        proceso_blocked_ready( logger,tiene_pid_dump);
 
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


bool recibirMemoriaOK(t_log* logger, t_proceso_en_new* pcb) {
    // Enviar pseudocódigo con datos de creación a Memoria
    t_args_pseudocodigo args;
    args.socket = socketMemoriaSyscall;
    args.pid = pcb->pcb->pid;
    args.pathArchivo = strdup(pcb->pathArchivo);
    args.tamanio = pcb->tamanio;
    args.logger = logger;

    t_paquete* paquete = crear_paquete_handshake_pseudocodigo(
        args.pathArchivo, INIT_PROC, KERNEL, args.pid, args.tamanio
    );

    uint32_t bytes;
    void* stream = serializar_paquete(paquete, &bytes);
    send(args.socket, stream, bytes, 0);
    free(stream);
    destruir_paquete(paquete);

    // Recibir respuesta de Memoria
    paquete = recibir_paquete(args.socket);
    if (!paquete) {
        log_error(logger, "Fallo al recibir paquete");
        free(args.pathArchivo);
        return false;
    }

    char* respuesta = malloc(paquete->buffer->size + 1);
    memcpy(respuesta, paquete->buffer->stream, paquete->buffer->size);
    respuesta[paquete->buffer->size] = '\0';

    log_info(logger, "Respuesta de memoria: %s", respuesta);
    bool ok = strcmp(respuesta, "ok") == 0;

    free(respuesta);
    destruir_paquete(paquete);
    free(args.pathArchivo);

    return ok;
}

void largoPlazoFifo(t_log* logger, t_kernelConfig cfg){
    bool noSalir = true; 
    while(noSalir){
        if (!list_is_empty(suspReady)) {
            if (recibirMemoriaOK(logger, list_get(suspReady, 0))){
                
                t_proceso_en_new* pcb_a_ready = list_remove(suspReady, 0);

                int* contadorReady = dictionary_get(pcb_a_ready->pcb->mestado, "READY");
                if (contadorReady) (*contadorReady)++;

                t_temporal* tiempoSuspReady= dictionary_get(pcb_a_ready->pcb->mtiempo, "SUSP_READY");
                if (tiempoSuspReady) temporal_stop(tiempoSuspReady);

                pcb_a_ready->estado = READY;
                pthread_mutex_lock(&mutex_cola_ready);
                log_info(logger, "Proceso PID %d pasa de SUSP_READY a READY", pcb_a_ready->pcb->pid);
                list_add(ready, pcb_a_ready);
                pthread_mutex_unlock(&mutex_cola_ready);

                t_temporal* tiempoReady = dictionary_get(pcb_a_ready->pcb->mtiempo, "READY");
                if (tiempoReady) temporal_resume (tiempoReady);

                sem_post(&sem_procesos_en_ready);
            }
        }
        else if(!list_is_empty(new)) {
            if (recibirMemoriaOK(logger, list_get(new, 0))){
                
                t_proceso_en_new* pcb_a_ready = list_remove(new, 0);
                int* contadorReady = dictionary_get(pcb_a_ready->pcb->mestado, "READY");
                if (contadorReady) (*contadorReady)++;

                t_temporal* tiempoNew = dictionary_get(pcb_a_ready->pcb->mtiempo, "NEW");
                if (tiempoNew) temporal_stop(tiempoNew);

                pcb_a_ready->estado = READY;
                pcb_a_ready->estimacionRafaga = cfg.estimacionInicial;
                pthread_mutex_lock(&mutex_cola_ready);
                log_info(logger, "Proceso PID %d pasa de NEW a READY", pcb_a_ready->pcb->pid);
                list_add(ready, pcb_a_ready);
                pthread_mutex_unlock(&mutex_cola_ready);

                t_temporal* tiempoReady = dictionary_get(pcb_a_ready->pcb->mtiempo, "READY");
                if (tiempoReady) temporal_resume (tiempoReady);

                sem_post(&sem_procesos_en_ready);
            }
        }
        else{
            noSalir = false;
        }

    }
}

// Empiezan funciones para Planificador de largo plazo.
void* comparar_tamanio(void* a, void* b) {
    t_proceso_en_new* pa = (t_proceso_en_new*) a;
    t_proceso_en_new* pb = (t_proceso_en_new*) b;

    return (pa->tamanio < pb->tamanio) ? pa : pb;
}

void masChicoPrimero(t_log* logger, t_kernelConfig cfg){
    bool noSalir = true; 
    while(noSalir){ 
        if (!list_is_empty(suspReady)) {
            if (recibirMemoriaOK(logger, list_get_minimum(suspReady, comparar_tamanio))){

                t_proceso_en_new* pcb_a_ready = list_get_minimum(suspReady, comparar_tamanio);
                if (pcb_a_ready != NULL) {
                    list_remove_element(suspReady, pcb_a_ready);
                }
                

                int* contadorReady = dictionary_get(pcb_a_ready->pcb->mestado, "READY");
                if (contadorReady) (*contadorReady)++;

                t_temporal* tiempoSuspReady= dictionary_get(pcb_a_ready->pcb->mtiempo, "SUSP_READY");
                if (tiempoSuspReady) temporal_stop(tiempoSuspReady);

                pcb_a_ready->estado = READY;
                pcb_a_ready->estimacionRafaga = cfg.estimacionInicial;
                pthread_mutex_lock(&mutex_cola_ready);
                log_info(logger, "Proceso PID %d pasa de SUSP_READY a READY, tamanio %d", pcb_a_ready->pcb->pid, pcb_a_ready->tamanio);
                list_add(ready, pcb_a_ready);
                pthread_mutex_unlock(&mutex_cola_ready);

                t_temporal* tiempoReady = dictionary_get(pcb_a_ready->pcb->mtiempo, "READY");
                if (tiempoReady) temporal_resume (tiempoReady);

                sem_post(&sem_procesos_en_ready);
            }
        }
        else if(!list_is_empty(new)) {
            if (recibirMemoriaOK(logger, list_get_minimum(new, comparar_tamanio))){
                
                t_proceso_en_new* pcb_a_ready = list_get_minimum(new, comparar_tamanio);
                if (pcb_a_ready != NULL) {
                    list_remove_element(new, pcb_a_ready);
                }

                int* contadorReady = dictionary_get(pcb_a_ready->pcb->mestado, "READY");
                if (contadorReady) (*contadorReady)++;
                pcb_a_ready->estado = READY;

                t_temporal* tiempoNew = dictionary_get(pcb_a_ready->pcb->mtiempo, "NEW");
                if (tiempoNew) temporal_stop(tiempoNew);

                pthread_mutex_lock(&mutex_cola_ready);
                log_info(logger, "Proceso PID %d pasa de NEW a READY, tamanio %d", pcb_a_ready->pcb->pid, pcb_a_ready->tamanio);
                list_add(ready, pcb_a_ready);
                pthread_mutex_unlock(&mutex_cola_ready);

                t_temporal* tiempoReady = dictionary_get(pcb_a_ready->pcb->mtiempo, "READY");
                if (tiempoReady) temporal_resume (tiempoReady); 

                sem_post(&sem_procesos_en_ready);
            }
        }
        else{
            noSalir = false;
        }
    }
}

//Terminan

//Planificador de corto plazo
void cortoPlazoFifo(t_log* logger){
    bool disponible = false;
    t_cpu_conectada* cpu = NULL;
    if(!list_is_empty(ready)) {
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
            t_proceso_en_new* pcb_a_exec = list_remove(ready, 0);
            cpu->proceso_ejecutando = pcb_a_exec->pcb;

            int* contadorExec = dictionary_get(pcb_a_exec->pcb->mestado, "EXEC");
            if (contadorExec) (*contadorExec)++;
            
            t_temporal* tiempoReady = dictionary_get(pcb_a_exec->pcb->mtiempo, "READY");
            if (tiempoReady) temporal_stop (tiempoReady);

            pcb_a_exec->estado = EXEC;
            pthread_mutex_lock(&mutex_cola_exec);
            log_info(logger, "PID (%d) pasa de READY a EXEC", pcb_a_exec->pcb->pid);
            list_add(exec, pcb_a_exec);
            pthread_mutex_unlock(&mutex_cola_exec);
            
            pthread_mutex_lock(&mutex_cpu_conectadas);

            if(!list_is_empty (cpus_conectadas)){
                enviar_handshake_kernel_cpu(cpu->fd_dispatch, pcb_a_exec->pcb, logger);
            }
            pthread_mutex_unlock(&mutex_cpu_conectadas);

            t_temporal* tiempoExec = dictionary_get(pcb_a_exec->pcb->mtiempo, "EXEC");
            if (tiempoExec) temporal_resume (tiempoExec);

        
        }
        sem_wait(&sem_exit);
    }

}

bool comparador_estimacion(void* a, void* b) {
    t_proceso_en_new* pcb_a = (t_proceso_en_new*) a;
    t_proceso_en_new* pcb_b = (t_proceso_en_new*) b;
    return pcb_a->estimacionRafaga < pcb_b->estimacionRafaga;
}

void SJF_sin_desalojo(t_log* logger, t_kernelConfig cfg){
    bool disponible = false;
    t_cpu_conectada* cpu = NULL;
    if(!list_is_empty(ready)) {
        list_sort(ready, comparador_estimacion);
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
            t_proceso_en_new* pcb_a_exec = list_remove(ready, 0);
            cpu->proceso_ejecutando = pcb_a_exec->pcb;
            int* contadorExec = dictionary_get(pcb_a_exec->pcb->mestado, "EXEC");
            if (contadorExec) (*contadorExec)++;

            t_temporal* tiempoReady = dictionary_get(pcb_a_exec->pcb->mtiempo, "READY");
            if (tiempoReady) temporal_stop (tiempoReady);

            pcb_a_exec->estado = EXEC;
            pthread_mutex_lock(&mutex_cola_exec);
            log_info(logger, "PID (%d) pasa de READY a EXEC con una estimacion de %lf", pcb_a_exec->pcb->pid, pcb_a_exec->estimacionRafaga);
            log_info(logger, "PID (%d) pasa de READY a EXEC", pcb_a_exec->pcb->pid);
            list_add(exec, pcb_a_exec);
            pthread_mutex_unlock(&mutex_cola_exec);

            pcb_a_exec->inicioRafaga = temporal_create();
            
            pthread_mutex_lock(&mutex_cpu_conectadas);
            if(!list_is_empty (cpus_conectadas)){
                enviar_handshake_kernel_cpu(cpu->fd_dispatch, pcb_a_exec->pcb, logger);
            }
            pthread_mutex_unlock(&mutex_cpu_conectadas);

            t_temporal* tiempoExec = dictionary_get(pcb_a_exec->pcb->mtiempo, "EXEC");
            if (tiempoExec) temporal_resume (tiempoExec);

        
        }        
        sem_wait(&sem_exit);
    }

}

void SJF_con_desalojo(t_log* logger, t_kernelConfig cfg){

    if(!list_is_empty(ready)) {
        return;
    }

    list_sort(ready, comparador_estimacion);
    t_proceso_en_new* mejor_ready = list_get(ready, 0);

    bool hay_en_exec = !list_is_empty(exec);
    t_proceso_en_new* en_exec = hay_en_exec ? list_get(exec, 0) : NULL;

    if (hay_en_exec && en_exec->estimacionRafaga > mejor_ready->estimacionRafaga) {
        log_info(logger, "SRT: Desalojando PID %d por PID %d", en_exec->pcb->pid, mejor_ready->pcb->pid);
        // Señal de interrupción: enviar al FD correspondiente
        pthread_mutex_lock(&mutex_cpu_conectadas);
        for (int i = 0; i < list_size(cpus_conectadas); i++) {
            t_cpu_conectada* cpu = list_get(cpus_conectadas, i);
            if (cpu->proceso_ejecutando && cpu->proceso_ejecutando->pid == en_exec->pcb->pid) {
                uint8_t interrupcion = 1;
                send(cpu->fd_interrupt, &interrupcion, sizeof(uint8_t), 0);
                break;
            }
        }
        pthread_mutex_unlock(&mutex_cpu_conectadas);
        return;
    }
    t_cpu_conectada* cpu_libre = NULL;
    for (int i = 0; i < list_size(cpus_conectadas); i++) {
        t_cpu_conectada* cpu = list_get(cpus_conectadas, i);
        if (cpu->disponible) {
            cpu_libre = cpu;
            cpu->disponible = false;
            break;
        }
    }

    t_proceso_en_new* a_ejecutar = list_remove(ready, 0);

    // Preparar estructura y enviar a EXEC
    a_ejecutar->estado = EXEC;
    int* contadorExec = dictionary_get(a_ejecutar->pcb->mestado, "EXEC");
    if (contadorExec) (*contadorExec)++;

    t_temporal* tiempoReady = dictionary_get(a_ejecutar->pcb->mtiempo, "READY");
    if (tiempoReady) temporal_stop (tiempoReady);

    list_add(exec, a_ejecutar);


    log_info(logger, "PID (%d) pasa de READY a EXEC", a_ejecutar->pcb->pid);

    t_temporal* tiempoExec = dictionary_get(a_ejecutar->pcb->mtiempo, "EXEC");
    if (tiempoExec) temporal_stop (tiempoExec);

    a_ejecutar->inicioRafaga = temporal_create();
    cpu_libre->proceso_ejecutando = a_ejecutar->pcb;

    enviar_handshake_kernel_cpu(cpu_libre->fd_dispatch, a_ejecutar->pcb, logger);

    sem_wait(&sem_exit);
}


void proceso_blocked_ready(t_log* logger, bool (*criterio)(void*)){


    t_proceso_en_new* proceso = list_find(blocked, criterio);

    if (proceso != NULL) {

        if (list_remove_element(blocked, proceso)) {
            
            t_temporal* tiempoBlocked= dictionary_get(proceso->pcb->mtiempo,  "BLOCKED");
            if (tiempoBlocked) temporal_stop(tiempoBlocked);

            log_info(logger, "Proceso %d pasa de BLOCKED a READY", proceso->pcb->pid);
            proceso->estado = READY;

            int* contadorReady = dictionary_get(proceso->pcb->mestado, "READY");
            if (contadorReady) (*contadorReady)++;

            t_temporal* tiempoReady = dictionary_get(proceso->pcb->mtiempo,  "READY");
            if (tiempoReady) temporal_resume(tiempoReady);

            pthread_mutex_lock(&mutex_cola_ready);
            list_add(ready, proceso);
            pthread_mutex_unlock(&mutex_cola_ready);

        }
    }

    // Luego buscar en SUSP_BLOCKED
    pthread_mutex_lock(&mutex_cola_suspBlocked);
    proceso = list_find(suspBlocked, criterio);
    if (proceso != NULL) {
        if (list_remove_element(suspBlocked, proceso)) {
            t_temporal* tiempoBlocked= dictionary_get(proceso->pcb->mtiempo,  "SUSP_BLOCKED");
            if (tiempoBlocked) temporal_stop(tiempoBlocked);

            log_info(logger, "Proceso %d pasa de SUSP_BLOCKED a SUSP_READY", proceso->pcb->pid);
            proceso->estado = SUSP_READY;

            int* contadorSuspReady = dictionary_get(proceso->pcb->mestado, "SUSP_READY");
            if (contadorSuspReady) (*contadorSuspReady)++;

            t_temporal* tiempoSuspReady= dictionary_get(proceso->pcb->mtiempo,  "SUSP_READY");
            if (tiempoSuspReady) temporal_resume(tiempoSuspReady);

            pthread_mutex_lock(&mutex_cola_suspReady);
            list_add(suspReady, proceso);
            pthread_mutex_unlock(&mutex_cola_suspReady);

        }
    }
    pthread_mutex_unlock(&mutex_cola_suspBlocked);
}
