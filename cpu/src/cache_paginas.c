#include "cache_paginas.h"
#include <commons/collections/list.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

static t_list* cache = NULL;
static uint32_t max_entradas = 0;
static algoritmo_cache politica;
static uint32_t puntero_reemplazo = 0;
static pthread_mutex_t mutex_cache;

void cache_init(uint32_t entradas_max, algoritmo_cache alg) {
    cache = list_create();
    max_entradas = entradas_max;
    politica = alg;
    puntero_reemplazo = 0;
    pthread_mutex_init(&mutex_cache, NULL);
}

void cache_destroy(void) {
    cache_flush_all();
    list_destroy_and_destroy_elements(cache, free);
    pthread_mutex_destroy(&mutex_cache);
}

bool cache_leer(uint32_t pid, uint32_t pagina, char** contenido_out) {
    pthread_mutex_lock(&mutex_cache);
    for (int i = 0; i < list_size(cache); i++) {
        entrada_cache* e = list_get(cache, i);
        if (e->pid == pid && e->pagina == pagina) {
            e->bit_uso = true; // marca de "uso"
            *contenido_out = strdup(e->contenido);
            pthread_mutex_unlock(&mutex_cache);
            return true;
        }
    }
    pthread_mutex_unlock(&mutex_cache);
    return false;
}

void cache_actualizar(uint32_t pid, uint32_t pagina, const char* contenido) {
    pthread_mutex_lock(&mutex_cache);
    for (int i = 0; i < list_size(cache); i++) {
        entrada_cache* e = list_get(cache, i);
        if (e->pid == pid && e->pagina == pagina) {
            free(e->contenido);
            e->contenido = strdup(contenido);
            e->bit_uso = true;
            e->bit_modificado = true;
            break;
        }
    }
    pthread_mutex_unlock(&mutex_cache);
}

void cache_cargar(uint32_t pid, uint32_t pagina, const char* contenido) {
    if (max_entradas == 0) return;

    pthread_mutex_lock(&mutex_cache);
    // Verificar si ya existe: si es así, actualizar
    for (int i = 0; i < list_size(cache); i++) {
        entrada_cache* e = list_get(cache, i);
        if (e->pid == pid && e->pagina == pagina) {
            free(e->contenido);
            e->contenido = strdup(contenido);
            e->bit_uso = true;
            pthread_mutex_unlock(&mutex_cache);
            return;
        }
    }
    // Si hay espacio, inserta directo
    if (list_size(cache) < max_entradas) {
        entrada_cache* nueva = malloc(sizeof(entrada_cache));
        nueva->pid = pid;
        nueva->pagina = pagina;
        nueva->contenido = strdup(contenido);
        nueva->bit_uso = true;
        nueva->bit_modificado = false;
        list_add(cache, nueva);
        pthread_mutex_unlock(&mutex_cache);
        return;
    }
    // Si la caché está llena, se usa el algoritmo CLOCK (o CLOCK-M, aquí CLOCK básico)
    while (true) {
        entrada_cache* candidata = list_get(cache, puntero_reemplazo);
        if (!candidata->bit_uso) {
            // Si la entrada está modificada, se debe escribir a memoria (llamada externa, por ejemplo)
            if (candidata->bit_modificado) {
                // Ejemplo: escribir_a_memoria(candidata->pid, candidata->pagina, candidata->contenido);
            }
            free(candidata->contenido);
            candidata->pid = pid;
            candidata->pagina = pagina;
            candidata->contenido = strdup(contenido);
            candidata->bit_uso = true;
            candidata->bit_modificado = false;
            puntero_reemplazo = (puntero_reemplazo + 1) % max_entradas;
            break;
        } else {
            candidata->bit_uso = false;
            puntero_reemplazo = (puntero_reemplazo + 1) % max_entradas;
        }
    }
    pthread_mutex_unlock(&mutex_cache);
}

void cache_flush_pid(uint32_t pid) {
    pthread_mutex_lock(&mutex_cache);
    for (int i = 0; i < list_size(cache); ) {
        entrada_cache* e = list_get(cache, i);
        if (e->pid == pid) {
            if (e->bit_modificado) {
                // Escribir a memoria (llamada externa)
                // escribir_a_memoria(e->pid, e->pagina, e->contenido);
            }
            list_remove_and_destroy_element(cache, i, free);
        } else {
            i++;
        }
    }
    pthread_mutex_unlock(&mutex_cache);
}

void cache_flush_all(void) {
    pthread_mutex_lock(&mutex_cache);
    for (int i = 0; i < list_size(cache); i++) {
        entrada_cache* e = list_get(cache, i);
        if (e->bit_modificado) {
            // Escribir a memoria
            // escribir_a_memoria(e->pid, e->pagina, e->contenido);
        }
    }
    list_clean_and_destroy_elements(cache, free);
    pthread_mutex_unlock(&mutex_cache);
}