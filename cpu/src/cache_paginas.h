#ifndef CACHE_PAGINAS_H
#define CACHE_PAGINAS_H

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    CLOCK,
    CLOCK_M
} algoritmo_cache;

typedef struct {
    uint32_t pid;         // Identificador del proceso
    uint32_t pagina;      // Número de página virtual
    char*    contenido;   // Contenido cargado (por ejemplo, una copia de la página)
    bool     bit_uso;     // Para el algoritmo CLOCK (indica que se usó recientemente)
    bool     bit_modificado; // Indica si la página fue escrita y por tanto está “sucia”
} entrada_cache;

// Inicializa la caché
void cache_init(uint32_t entradas_max, algoritmo_cache algoritmo);

// Libera la caché
void cache_destroy(void);

// Intenta leer (buscar) una página desde la caché. Retorna true si hubo "hit" y devuelve (*contenido_out)
// (Se realiza una copia interna para no modificar el original).
bool cache_leer(uint32_t pid, uint32_t pagina, char** contenido_out);

// Actualiza el contenido de una página que ya está en caché, marcándola como modificada.
void cache_actualizar(uint32_t pid, uint32_t pagina, const char* contenido);

// Carga (o inserta) una página en la caché; si ya existe, la actualiza. Si no y la caché está llena, reemplaza
// usando el algoritmo CLOCK.
void cache_cargar(uint32_t pid, uint32_t pagina, const char* contenido);

// Al desalojar un proceso, se deben escribir las páginas modificadas en memoria y eliminar las entradas del proceso.
void cache_flush_pid(uint32_t pid);

// Vacia toda la caché (por ejemplo, al terminar la CPU)
void cache_flush_all(void);

#endif // CACHE_PAGINAS_H

