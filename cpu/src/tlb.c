#include "tlb.h"
#include <stdlib.h>
#include <commons/collections/list.h>
#include <commons/log.h>
#include <pthread.h>

static t_list* tlb = NULL;
static int max_entradas = 0;
static pthread_mutex_t mutex_tlb;

extern t_log* logger_cpu;

void tlb_init(int entradas) {
    tlb = list_create();
    max_entradas = entradas;
    pthread_mutex_init(&mutex_tlb, NULL);
}

void tlb_destroy(void) {
    list_destroy_and_destroy_elements(tlb, free);
    pthread_mutex_destroy(&mutex_tlb);
}

bool tlb_buscar(uint32_t pid, uint32_t pagina, uint32_t* marco) {
    if (max_entradas == 0) return false;

    pthread_mutex_lock(&mutex_tlb);
    for (int i = 0; i < list_size(tlb); i++) {
        entrada_tlb* entrada = list_get(tlb, i);
        if (entrada->pid == pid && entrada->pagina == pagina) {
            *marco = entrada->marco;
            log_info(logger_cpu, "TLB Hit: PID: %u - TLB HIT - Pagina: %u", pid, pagina);
            pthread_mutex_unlock(&mutex_tlb);
            return true;
        }
    }
    log_info(logger_cpu, "TLB Miss: PID: %u - TLB MISS - Pagina: %u", pid, pagina);
    pthread_mutex_unlock(&mutex_tlb);
    return false;
}

void tlb_agregar(uint32_t pid, uint32_t pagina, uint32_t marco) {
    if (max_entradas == 0) return;

    pthread_mutex_lock(&mutex_tlb);
    if (list_size(tlb) >= max_entradas) {
        list_remove_and_destroy_element(tlb, 0, free); // FIFO
    }

    entrada_tlb* nueva = malloc(sizeof(entrada_tlb));
    nueva->pid = pid;
    nueva->pagina = pagina;
    nueva->marco = marco;

    list_add(tlb, nueva);
    pthread_mutex_unlock(&mutex_tlb);
}

void tlb_reset(void) {
    pthread_mutex_lock(&mutex_tlb);
    list_clean_and_destroy_elements(tlb, free);
    pthread_mutex_unlock(&mutex_tlb);
}

void tlb_reset_pid(uint32_t pid) {
    pthread_mutex_lock(&mutex_tlb);
    t_list* nuevas = list_create();

    for (int i = 0; i < list_size(tlb); i++) {
        entrada_tlb* entrada = list_get(tlb, i);
        if (entrada->pid != pid) {
            list_add(nuevas, entrada);
        } else {
            free(entrada);
        }
    }

    list_destroy(tlb);
    tlb = nuevas;

    pthread_mutex_unlock(&mutex_tlb);
}
