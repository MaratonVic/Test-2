#ifndef TLB_H
#define TLB_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint32_t pid;
    uint32_t pagina;
    uint32_t marco;
} entrada_tlb;

void tlb_init(int entradas);
void tlb_destroy(void);

bool tlb_buscar(uint32_t pid, uint32_t pagina, uint32_t* marco);
void tlb_agregar(uint32_t pid, uint32_t pagina, uint32_t marco);
void tlb_reset(void);
void tlb_reset_pid(uint32_t pid);

#endif