#include "kernel.h"

t_log* loggerKernel;
t_kernelConfig cfg;

t_list* ready = NULL;
pthread_mutex_t mutex_cola_ready;
t_list* new = NULL;
pthread_mutex_t mutex_cola_new;
t_list* blocked = NULL;
pthread_mutex_t mutex_cola_blocked;
t_list* suspReady = NULL;
pthread_mutex_t mutex_cola_suspReady;
t_list* suspBlocked = NULL;
pthread_mutex_t mutex_cola_suspBlocked;
t_list* exec = NULL;
pthread_mutex_t mutex_cola_exec;
t_dictionary* diccionario_io = NULL;
pthread_mutex_t mutex_diccionario_io;
t_list* cpus_conectadas = NULL;
pthread_mutex_t mutex_cpu_conectadas;

uint32_t pid_procesos = 0; 
pthread_mutex_t mutex_pid_procesos;

uint32_t pid_bloqueo= 0; 
pthread_mutex_t mutex_pid_bloqueo;

uint32_t pid_susBloqueo= 0; 
pthread_mutex_t mutex_pid_susBloqueo;

uint32_t pid_dump = 0;
pthread_mutex_t mutex_pid_dump;


sem_t sem_procesos_en_new;
sem_t sem_procesos_en_ready;
sem_t sem_procesos_en_blocked;
sem_t sem_procesos_en_suspReady;
sem_t sem_bloqueado;
sem_t sem_exit;
sem_t sem_ioTerminada;
sem_t sem_dumpTerminada;

int socketMemoriaSyscall = -1;

void liberar_lista_si_existe(t_list* lista) {
    if (lista != NULL) {
        for (int i = 0; i < list_size(lista); i++) {
            t_proceso_en_new* proceso = list_get(lista, i);
            log_info(loggerKernel, "%d", proceso->pcb->pid);
            destruir_proceso_en_new(proceso);
        }
        list_destroy(lista);
    }
}

void manejador_sigint(int sig) {
    liberar_lista_si_existe(new);
    liberar_lista_si_existe(ready);
    liberar_lista_si_existe(exec);
    liberar_lista_si_existe(blocked);
    liberar_lista_si_existe(suspBlocked);
    liberar_lista_si_existe(suspReady);
    log_warning(loggerKernel, "Terminando ordenadamente...");
    // liberar_memoria();
    // cerrar_sockets();
    exit(0);
}


int main(int argc, char* argv[]) {
    signal(SIGINT, manejador_sigint);

    srand(time(NULL)); 
    inicializar_estructuras();
    iniciarKernel(argc, argv);
    return 0;
}

void iniciarKernel(int argc, char* argv[]){
    t_config* configKernel = NULL;
    

    loggerKernel = iniciar_logger();    
    configKernel = iniciar_config();              
    
    leer_config(&cfg, configKernel);

    if(argc < 3){
        log_error(loggerKernel, "No se ejecuto de la manera correcta el inicio de kernel ./bin/kernel [archivo_pseudocodigo] [tamanio_proceso]");
        exit(EXIT_FAILURE);
    }

    //loguear_config(&cfg, loggerKernel);
    int socket_memoria = conectar_a_memoria(cfg.ip, cfg.puertoMemoria, loggerKernel);
    identificar_a_memoria(socket_memoria);
    socketMemoriaSyscall = socket_memoria;


    int socket_dispatch = levantar_socket_servidor(cfg.puertoEscuchaDispatch, loggerKernel);
    int socket_interrupt = levantar_socket_servidor(cfg.puertoEscuchaInterrupt, loggerKernel);
    int socket_IO = levantar_socket_servidor(cfg.puertoEscuchaIO, loggerKernel);

    if (socket_dispatch == -1 || socket_interrupt == -1 || socket_IO == -1) {
    log_error(loggerKernel, "No se pudo levantar alguno de los sockets.");
    exit(EXIT_FAILURE);
}

    static t_args_planificador args_planificador;
    args_planificador.socket = socket_memoria;
    args_planificador.cfg = cfg;
    args_planificador.logger = loggerKernel;
    args_planificador.pseudocodigo.socket = socket_memoria;
    args_planificador.pseudocodigo.pid = 0;
    args_planificador.pseudocodigo.pathArchivo = argv[1];
    args_planificador.pseudocodigo.tamanio = (uint32_t) strtoul(argv[2], NULL, 10);
    args_planificador.pseudocodigo.logger = loggerKernel;
    
    pthread_t hilo_planificador;

    if (pthread_create(&hilo_planificador, NULL, planificarLargoPlazo, (void*) &args_planificador) != 0) {
        perror("Error al crear hilo del planificador de largo plazo");
        exit(EXIT_FAILURE);
    }
    pthread_detach(hilo_planificador);

    static t_args_planificador_corto args_planificador_corto;
    args_planificador_corto.socket = socket_memoria;
    args_planificador_corto.cfg = cfg;
    args_planificador_corto.logger = loggerKernel;

    pthread_t hilo_planificador_corto;

    if (pthread_create(&hilo_planificador_corto, NULL, planificarCortoPlazo, (void*) &args_planificador_corto) != 0) {
        perror("Error al crear hilo del planificador de corto Plazo");
        exit(EXIT_FAILURE);
    }
    pthread_detach(hilo_planificador);


    pthread_t hilo_planificador_mediano;

    if (pthread_create(&hilo_planificador_mediano, NULL, planificarMedianoPlazo, (void*) &args_planificador_corto) != 0) {
        perror("Error al crear hilo del planificador de Mediano Plazo");
        exit(EXIT_FAILURE);
    }
    pthread_detach(hilo_planificador);

    iniciar_hilos_conexiones(socket_dispatch, socket_interrupt, socket_IO, loggerKernel);

    log_destroy(loggerKernel);
    config_destroy(configKernel);      
}

void inicializar_estructuras() {
    if (sem_init(&sem_procesos_en_new, 0, 0) != 0) {
        perror("Error al inicializar el semáforo");
        exit(EXIT_FAILURE);
    }
    if (sem_init(&sem_procesos_en_ready, 0, 0) != 0) {
        perror("Error al inicializar el semáforo");
        exit(EXIT_FAILURE);
    }
    if (sem_init(&sem_procesos_en_blocked, 0, 0) != 0) {
        perror("Error al inicializar el semáforo");
        exit(EXIT_FAILURE);
    }
    if (sem_init(&sem_exit, 0, 0) != 0) {
        perror("Error al inicializar el semáforo");
        exit(EXIT_FAILURE);
    }
    if (sem_init(&sem_procesos_en_suspReady, 0, 0) != 0) {
        perror("Error al inicializar el semáforo");
        exit(EXIT_FAILURE);
    }
    if (sem_init(&sem_bloqueado, 0, 0) != 0) {
        perror("Error al inicializar el semáforo");
        exit(EXIT_FAILURE);
    }
    if (sem_init(&sem_ioTerminada, 0, 0) != 0) {
        perror("Error al inicializar el semáforo");
        exit(EXIT_FAILURE);
    }   
    if (sem_init(&sem_dumpTerminada, 0, 0) != 0) {
        perror("Error al inicializar el semáforo");
        exit(EXIT_FAILURE);
    }
    diccionario_io = dictionary_create();
    pthread_mutex_init(&mutex_diccionario_io, NULL);
    cpus_conectadas = list_create();
}

void destruir_estructuras() {
    dictionary_destroy_and_destroy_elements(diccionario_io, (void*) queue_destroy);
    pthread_mutex_destroy(&mutex_diccionario_io);
}

t_cpu_conectada* buscar_cpu_por_nombre(char* nombre) {
    for (int i = 0; i < list_size(cpus_conectadas); i++) {
        t_cpu_conectada* cpu = list_get(cpus_conectadas, i);
        if (strcmp(cpu->nombre_cpu, nombre) == 0) {
            return cpu;
        }
    }
    return NULL;
}