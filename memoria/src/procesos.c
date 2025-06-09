#include "procesos.h"

static t_list* lista_procesos;
t_dictionary* pid_instrucciones;
//Para el manejo de la lista de procesos
static pthread_mutex_t mutex_lista_procesos;

static pthread_mutex_t mutex_pid_instrucciones;

void iniciar_lista_procesos(void){
    lista_procesos = list_create();
    pthread_mutex_init(&mutex_lista_procesos, NULL);
    pthread_mutex_init(&mutex_pid_instrucciones, NULL);
    pid_instrucciones = dictionary_create();
}

void agregar_proceso(t_proceso_mem* proceso) {
    pthread_mutex_lock(&mutex_lista_procesos);
    list_add(lista_procesos, proceso);
    pthread_mutex_unlock(&mutex_lista_procesos);
}

void iniciar_proceso_en_memoria(t_buffer* payload){
    //TODO
    t_proceso_mem* proceso = proceso_mem_create();
    if(!proceso) {
        // AGREGAR comprobacion de espacio en memoria
        log_error(logger, "No hay espacio para crear el proceso");
        // devolver paquete de error
    }
    char* path = recibir_string(payload);
    if(!path) {
        //devolver paquete de error 
        log_error(logger,"Error al crear path");
    }
    // comprobacion de espacio en pagina / frame

    proceso->archivo = agregar_path_instrucciones(path);
    proceso->pid = recibir_uint32(payload);
    log_info(logger, "PID: %d \n", proceso->pid);
    log_info(logger, "archivo %s", proceso->archivo);
    proceso->tam_proceso = recibir_uint32(payload);
    iniciar_metricas(proceso);
    cargar_instrucciones_proceso(proceso); 
    //faltaria una validacion para saber si hay espacio en memoria
    agregar_proceso(proceso);

    log_info(logger, "## PID: %d - Proceso Creado - Tamanio: %d", proceso->pid, proceso->tam_proceso);
    dictionary_put(pid_instrucciones, string_itoa(proceso->pid), (void*) proceso->instrucciones);
    free(path);
    
}

t_proceso_mem* proceso_mem_create(void) {
    t_proceso_mem* proceso = malloc(sizeof(t_proceso_mem));
    proceso->instrucciones = list_create();
    proceso->metricas = dictionary_create();
    proceso->archivo = NULL;
    return proceso;
}

void iniciar_metricas(t_proceso_mem* proceso) {
    dictionary_put(proceso->metricas, "ACCESOS_TP", (void*) -1);
    dictionary_put(proceso->metricas, "INSTRUCCIONES_SOLICITADAS",(void *) -1);
    dictionary_put(proceso->metricas, "BAJADAS_SWAP", (void*) -1);
    dictionary_put(proceso->metricas, "SUBIDAS_MP", (void *) -1);
    dictionary_put(proceso->metricas, "LECTURAS", (void *) -1);
    dictionary_put(proceso->metricas, "ESCRITURAS", (void *) -1);
}

void aumentar_metrica(t_dictionary* dictionary, char* key) {
    if(dictionary_has_key(dictionary, key)) {
        log_info(logger, "Metrica no encontrada");
    } 
    void* valor = dictionary_get(dictionary, key);
    int* cantidad = (int*) valor;
    if((*cantidad) == -1) {
        (*cantidad) = 1;
    }
    (*cantidad)++;
    dictionary_put(dictionary, key, (void *) cantidad);
}

void cargar_instrucciones_proceso(t_proceso_mem* proceso) {
    FILE* archivo = fopen(/*proceso->archivo*/"./pseudocodigo.txt", "r");
    if (!archivo) {
        log_error(logger, "No se pudo abrir el archivo %s", proceso->archivo);
        return;
    }

    char* linea = NULL;
    size_t len = 0;

    while (getline(&linea, &len, archivo) != -1) {
        linea[strcspn(linea, "\n")] = '\0'; 
        list_add(proceso->instrucciones, strdup(linea));
    }

    free(linea);
    fclose(archivo);

    log_info(logger, "Instrucciones cargadas para PID %d (%d instrucciones)", 
             proceso->pid, list_size(proceso->instrucciones));
}

t_proceso_mem* buscar_proceso_por_pid(t_list* lista_procesos, uint32_t pid) {
    
    bool _coincide_pid(void* elem) 
    {
        t_proceso_mem* aux_proceso = (t_proceso_mem*)elem;
        return aux_proceso->pid == pid;
    }
    pthread_mutex_lock(&mutex_lista_procesos);
    t_proceso_mem* res = list_find(lista_procesos, _coincide_pid );
    pthread_mutex_lock(&mutex_lista_procesos);
    return res;
}

//falta terminar
static void proceso_destroyer(void* proceso) {
    t_proceso_mem* p = (t_proceso_mem *)proceso;
}

//falta terminar
void eliminar_lista_procesos(void) {
    list_destroy_and_destroy_elements(lista_procesos,proceso_destroyer);
    pthread_mutex_destroy(&mutex_lista_procesos);
}
