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

int iniciar_proceso_en_memoria(t_buffer* payload){
    //TODO
    t_proceso_mem* proceso = proceso_mem_create();
    if(!proceso) {
        log_error(logger, "No hay espacio para crear el proceso");
        return 0;
    }
    char* path = recibir_string(payload);
    if(!path) {
        log_error(logger,"Error al crear path");
        return 0;
    }
    
    proceso->archivo = agregar_path_instrucciones(path);
    proceso->pid = recibir_uint32(payload);
    log_info(logger, "PID: %d \n", proceso->pid);
    log_info(logger, "archivo %s", proceso->archivo);
    proceso->tam_proceso = recibir_uint32(payload);
    iniciar_metricas(proceso);
    cargar_instrucciones_proceso(proceso); 
    uint32_t paginas_necesarias = ceil(proceso->tam_proceso / cfg.tam_pagina);
    proceso->tabla_paginas = crear_tabla(1, &paginas_necesarias);
    //faltaria una validacion para saber si hay espacio en memoria
    if(espacio_disponible() <= 0) {
        log_error(logger, "No hay espacio disponible en memoria");
        return 0;
    }    
    agregar_proceso(proceso);

    log_info(logger, "## PID: %d - Proceso Creado - Tamanio: %d", proceso->pid, proceso->tam_proceso);
    char* clave_pid = string_itoa(proceso->pid);
    char* clave_pid_dup = strdup(clave_pid); // LIBERAR DESPUES CUANDO SE DESTRUYA EL DICCIONARIO
    dictionary_put(pid_instrucciones, clave_pid_dup, (void*) proceso->instrucciones);
    free(clave_pid);
    free(path);
    return 1;
}

t_proceso_mem* proceso_mem_create(void) {
    t_proceso_mem* proceso = malloc(sizeof(t_proceso_mem));
    proceso->instrucciones = list_create();
    proceso->metricas = dictionary_create();
    proceso->archivo = NULL;
    return proceso;
}

void iniciar_metricas(t_proceso_mem* proceso) {
    const char* metricas[] = {"ACCESOS_TP", "INSTRUCCIONES_SOLICITADAS", "BAJADAS_SWAP", "SUBIDAS_MP", "LECTURAS", "ESCRITURAS"};
    for (int i = 0; i < 6; i++) {
        int* contador = malloc(sizeof(int));
        *contador = 0;
        dictionary_put(proceso->metricas, strdup(metricas[i]), contador);
    }
}

void aumentar_metrica(t_dictionary* dictionary, char* key) {
    if(!dictionary_has_key(dictionary, key)) {
        log_info(logger, "Metrica no encontrada");
        return;
    } 
    int* valor =  (int*)dictionary_get(dictionary, key);
    (*valor)++;
    
    dictionary_put(dictionary, key, (void *) valor);
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

/*t_proceso_mem* buscar_proceso_por_pid_mem(uint32_t pid) {
    
    bool _coincide_pid(void* elem) 
    {
        t_proceso_mem* aux_proceso = (t_proceso_mem*)elem;
        return aux_proceso->pid == pid;
    }
    pthread_mutex_lock(&mutex_lista_procesos);
    t_proceso_mem* res = list_find(lista_procesos, _coincide_pid );
    pthread_mutex_lock(&mutex_lista_procesos);
    return res;
}*/

t_proceso_mem* buscar_proceso_por_pid_mem(uint32_t pid) {
    t_proceso_mem* resultado = NULL;
    pthread_mutex_lock(&mutex_lista_procesos);

    int total = list_size(lista_procesos);
    for (int i = 0; i < total; i++) {
        t_proceso_mem* p = list_get(lista_procesos, i);
        if (p->pid == pid) {
            resultado = p;
            break;
        }
    }

    pthread_mutex_unlock(&mutex_lista_procesos);
    return resultado;
}


//falta terminar
void proceso_destroyer(void* proceso) {
    t_proceso_mem* p = (t_proceso_mem *)proceso;
    destruir_lista_instrucciones((void*)p->instrucciones);
    eliminar_metricas(p->metricas);
}

//falta terminar
void eliminar_lista_procesos(void) {
    list_destroy_and_destroy_elements(lista_procesos,proceso_destroyer);
    pthread_mutex_destroy(&mutex_lista_procesos);
}

void mostrar_metricas(t_dictionary* pid_instrucciones) {
    void logear_metricas(char* key, void* value){
        log_info(logger, "[Metrica %s] : %d", key, *(int*) value);
    }
    dictionary_iterator(pid_instrucciones, logear_metricas);
}

void eliminar_metricas(t_dictionary* diccionario){
    mostrar_metricas(diccionario);
    dictionary_destroy_and_destroy_elements(diccionario,destruir_lista_instrucciones);
}

void destruir_lista_instrucciones(void* elemento) {
    t_list* lista_instrucciones = (t_list*)elemento;
    
    for (int i = 0; i < list_size(lista_instrucciones); i++) {
        free(list_get(lista_instrucciones, i));
    }
}

t_tabla_pagina* tabla_principal_de_pid(int pid) {
    t_tabla_pagina* raiz = NULL;

    pthread_mutex_lock(&mutex_lista_procesos);

    int total = list_size(lista_procesos);
    for (int i = 0; i < total; i++) {
        t_proceso_mem* proc = list_get(lista_procesos, i);
        if (proc->pid == pid) {
            raiz = proc->tabla_paginas;
            break;
        }
    }

    pthread_mutex_unlock(&mutex_lista_procesos);
    return raiz;
}

