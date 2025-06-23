#include <string.h>
#include <stdlib.h>
#include <commons/string.h>
#include <unistd.h>
#include <stdint.h>
#include "instrucciones.h"
#include "cpu.h"
#include "ciclo.h"
#include "cpuIO.h"
#include "cpuKernel.h"

bool ciclo_instruccion(uint32_t pid, uint32_t* pc) {
    char* linea = fetch(pid, *pc);
    if (!linea) {
        log_error(logger_cpu, "Fallo en fetch para PID %d, PC %u", pid, *pc);
        return false;
    }

    t_instruccion* instruccion = decode(linea);
    free(linea);

    if (!instruccion) {
        log_error(logger_cpu, "Fallo en decode para PID %d, PC %u", pid, *pc);
        return false;
    }

    bool continuar = execute(instruccion, pid, pc);
    destruir_instruccion(instruccion);

    if (continuar && check_interrupt(pid, *pc)) {
    return false;
    }

    return continuar;
}

char* fetch(uint32_t pid, uint32_t pc) {
    t_buffer* buffer = malloc(sizeof(t_buffer));
    buffer->size = sizeof(uint32_t) * 2;
    buffer->stream = malloc(buffer->size);

    int offset = 0;
    memcpy(buffer->stream + offset, &pid, sizeof(uint32_t));
    offset += sizeof(uint32_t);
    memcpy(buffer->stream + offset, &pc, sizeof(uint32_t));

    t_paquete* paquete = crear_paquete(SOLICITAR_INSTRUCCION, buffer);
    log_error(logger_cpu, "Entre locura");
    if (enviar_paquete(conexion_memoria, paquete) != 0) {
        log_error(logger_cpu, "Fallo al enviar solicitud de instrucción a Memoria");
        destruir_paquete(paquete);
        return NULL;
    }
    destruir_paquete(paquete);

    t_paquete* respuesta = recibir_paquete(conexion_memoria);
    if (!respuesta) {
        log_error(logger_cpu, "No se recibió respuesta de Memoria");
        return NULL;
    }

    if (respuesta->codigo_operacion != INSTRUCCION_SOLICITADA) {
        log_error(logger_cpu, "Respuesta inesperada: %d", respuesta->codigo_operacion);
        destruir_paquete(respuesta);
        return NULL;
    }

    char* instruccion = string_duplicate((char*)respuesta->buffer->stream);
    destruir_paquete(respuesta);

    return instruccion;
}


// char* fetch(uint32_t pid, uint32_t pc) {
//     log_info(logger_cpu, "[FETCH SIMULADO] PID: %d - PC: %d", pid, pc);

//     // Simulación de pseudocódigo
//     char* instrucciones[] = {
//         "NOOP",
//         "WRITE 100 hola",
//         "READ 100",
//         "IO impresora 5000000",
//         "EXIT"
//     };

//     int cantidad = sizeof(instrucciones) / sizeof(instrucciones[0]);

//     if (pc >= cantidad) return NULL;

//     return strdup(instrucciones[pc]);
// }

t_instruccion* decode(const char* linea) {
    char** partes = string_split((char*)linea, " ");
    if (!partes || !partes[0]) return NULL;

    t_instruccion* inst = malloc(sizeof(t_instruccion));

    if (strcmp(partes[0], "NOOP") == 0) inst->tipo = INST_NOOP;
    else if (strcmp(partes[0], "WRITE") == 0) inst->tipo = INST_WRITE;
    else if (strcmp(partes[0], "READ") == 0) inst->tipo = INST_READ;
    else if (strcmp(partes[0], "GOTO") == 0) inst->tipo = INST_GOTO;
    else if (strcmp(partes[0], "IO") == 0) inst->tipo = INST_IO;
    else if (strcmp(partes[0], "INIT_PROC") == 0) inst->tipo = INST_INIT_PROC;
    else if (strcmp(partes[0], "DUMP_MEMORY") == 0) inst->tipo = INST_DUMP_MEMORY;
    else if (strcmp(partes[0], "EXIT") == 0) inst->tipo = INST_EXIT;
    else {
        free(inst);
        string_array_destroy(partes);
        return NULL;
    }

    int cant_param = 0;
    for (int i = 1; partes[i] != NULL; i++) cant_param++;

    inst->parametros = malloc(sizeof(char*) * (cant_param + 1));
    for (int i = 0; i < cant_param; i++) {
        inst->parametros[i] = string_duplicate(partes[i + 1]);
    }
    inst->parametros[cant_param] = NULL;

    string_array_destroy(partes);
    return inst;
}

bool execute(t_instruccion* instruccion, uint32_t pid, uint32_t* pc) {
    const char* tipo_str[] = {
        "NOOP", "WRITE", "READ", "GOTO",
        "IO", "INIT_PROC", "DUMP_MEMORY", "EXIT"
    };

    log_info(logger_cpu, "[EJECUTAR] PID %d - Instrucción: %s", pid, tipo_str[instruccion->tipo]);

    switch (instruccion->tipo) {
        case INST_NOOP:
            usleep(1000 * config_cpu_data.retardo_cache);
            (*pc)++;
            break;

        case INST_WRITE: {
            // Convertir el primer parámetro a dirección lógica
            uint32_t dir_log = atoi(instruccion->parametros[0]);
            char* valor = instruccion->parametros[1];
            uint32_t pagina = dir_log / config_msj->tam_pagina;
            // Primero, si se tiene caché habilitada, actualizar la entrada correspondiente
            if (config_cpu_data.entradas_cache > 0) {
                cache_actualizar(pid, pagina, valor);
                log_info(logger_cpu, "INST_WRITE: PID %d - Cache actualizada para página %u", pid, pagina);
            } else {
                uint32_t marco;
                if (!tlb_buscar(pid, pagina, &marco)) {
                    int dir_fisica = mmu_traducir(dir_log, pid);
                    marco = dir_fisica / config_msj->tam_pagina;
                    tlb_agregar(pid, pagina, marco);
                }
                enviar_escritura_a_memoria(pid, marco, valor);
            }
            log_info(logger_cpu, "INST_WRITE: PID %d - Escribir en Dir Log %u valor: \"%s\"", pid, dir_log, valor);
            (*pc)++;
            break;
        }
        case INST_READ: {
            uint32_t dir_log = atoi(instruccion->parametros[0]);
            uint32_t pagina = dir_log / config_msj->tam_pagina;
            char* valor = NULL;
            // Primero se consulta la caché si está habilitada.
            if (config_cpu_data.entradas_cache > 0 && cache_leer(pid, pagina, &valor)) {
                log_info(logger_cpu, "INST_READ: PID %d - Cache Hit para página %u", pid, pagina);
                } else {
                    log_info(logger_cpu, "INST_READ: PID %d - Cache Miss para página %u", pid, pagina);
                    uint32_t marco;
                    if (!tlb_buscar(pid, pagina, &marco)) {
                        int dir_fisica = mmu_traducir(dir_log, pid);
                        marco = dir_fisica / config_msj->tam_pagina;
                        tlb_agregar(pid, pagina, marco);
                        }
                        // Leer desde memoria principal usando el "marco"
                        valor = solicitar_lectura_a_memoria(pid, marco);
                        // Una vez obtenido, guardar este valor en caché, si la caché está habilitada.
                        if (config_cpu_data.entradas_cache > 0 && valor != NULL) {
                            cache_cargar(pid, pagina, valor);
                            }
                    }
                log_info(logger_cpu, "INST_READ: PID %d - Leer de Dir Log %u; valor: \"%s\"", pid, dir_log, valor);
                free(valor);
                (*pc)++;
                break;
        }
        
        case INST_GOTO:
            *pc = atoi(instruccion->parametros[0]);
            break;

        case INST_IO: {
            char* dispositivo = instruccion->parametros[0];
            uint32_t tiempo = atoi(instruccion->parametros[1]);
            enviar_syscall_io_al_kernel(pid, dispositivo, tiempo);
            return false;
        }

        case INST_INIT_PROC: {
            enviar_syscall_init_proc(conexion_kernel_dispatch, pid, *pc);
            return false;
        }

        case INST_DUMP_MEMORY:
            enviar_syscall_dump(conexion_kernel_dispatch, pid, *pc);
            return false;

        case INST_EXIT:
            enviar_syscall_exit(conexion_kernel_dispatch);
            return false;

        default:
            log_error(logger_cpu, "[ERROR] Instrucción no reconocida");
            return false;
    }
    return true;
}


void enviar_escritura_a_memoria(uint32_t pid, uint32_t direccion, const char* valor) {
    codigos_memoria codigo_op = CODIGO_WRITE;
    uint32_t tam_valor = strlen(valor) + 1;
    uint32_t total_size = sizeof(uint8_t) + sizeof(uint32_t) * 3 + tam_valor;

    void* buffer = malloc(total_size);
    int offset = 0;

    memcpy(buffer + offset, &codigo_op, sizeof(uint8_t));
    offset += sizeof(uint8_t);

    memcpy(buffer + offset, &pid, sizeof(uint32_t));
    offset += sizeof(uint32_t);

    memcpy(buffer + offset, &direccion, sizeof(uint32_t));
    offset += sizeof(uint32_t);

    memcpy(buffer + offset, &tam_valor, sizeof(uint32_t));
    offset += sizeof(uint32_t);

    memcpy(buffer + offset, valor, tam_valor);

    send(conexion_memoria, buffer, total_size, 0);
    log_info(logger_cpu, "[WRITE] PID %u -> Dir %u = \"%s\"", pid, direccion, valor);

    free(buffer);
}

char* solicitar_lectura_a_memoria(uint32_t pid, uint32_t marco) {
    codigos_memoria codigo_op = CODIGO_READ;
    // Se envían 3 elementos: op_code (uint8_t), pid (uint32_t) y marco (uint32_t).
    uint32_t total_size = sizeof(uint8_t) + sizeof(uint32_t) * 2;
    
    // Reservar buffer para el pedido
    void* buffer = malloc(total_size);
    if (!buffer) {
        log_error(logger_cpu, "Error al asignar buffer para CODIGO_READ");
        return NULL;
    }
    int offset = 0;
    memcpy(buffer + offset, &codigo_op, sizeof(uint8_t));
    offset += sizeof(uint8_t);
    memcpy(buffer + offset, &pid, sizeof(uint32_t));
    offset += sizeof(uint32_t);
    memcpy(buffer + offset, &marco, sizeof(uint32_t));
    
    // Enviar el pedido a memoria
    if (send(conexion_memoria, buffer, total_size, 0) != total_size) {
        log_error(logger_cpu, "Error al enviar CODIGO_READ");
        free(buffer);
        return NULL;
    }
    free(buffer);
    
    // Recibir el tamaño del valor leído
    uint32_t tam_valor;
    if (recv(conexion_memoria, &tam_valor, sizeof(uint32_t), MSG_WAITALL) <= 0) {
        log_error(logger_cpu, "Error al recibir tamaño del valor leído");
        return NULL;
    }
    
    // Reservar memoria para el contenido leído
    char* valor = malloc(tam_valor);
    if (!valor) {
        log_error(logger_cpu, "Error al asignar memoria para valor leído");
        return NULL;
    }
    if (recv(conexion_memoria, valor, tam_valor, MSG_WAITALL) <= 0) {
        log_error(logger_cpu, "Error al recibir valor leído");
        free(valor);
        return NULL;
    }
    
    log_info(logger_cpu, "[READ] PID %u <- Marco %u = \"%s\"", pid, marco, valor);
    // Se retorna el puntero; el caller (la instrucción READ en execute) debe liberarlo cuando ya no se use.
    return valor;
}


void enviar_syscall_io_al_kernel(uint32_t pid, const char* dispositivo, uint32_t tiempo) {
    t_paquete* paquete = crear_paquete_handshake_syscall_cpu_io((char*)dispositivo, INIT_IO, INST_IO, tiempo, cpu_id);    
    
    uint32_t bytes = 0;
    void* serializado = serializar_paquete_syscall_cpu_io(paquete, &bytes);

    send(conexion_kernel_dispatch, serializado, bytes, 0);

    destruir_paquete_syscall_cpu_io(paquete);
    free(serializado);

    // uint8_t respuesta;
    // if (recv(conexion_kernel_dispatch, &respuesta, sizeof(uint8_t), 0)<= 0){
    //     log_error(logger_cpu, "Error al recibir respuesta del kernel para syscall IO");
    // } else {
    // log_info(logger_cpu, "[SYSCALL_IO] Enviado -> PID %u, dispositivo: %s, tiempo: %u (ACK: %d)", pid, dispositivo, tiempo, respuesta);
    // }   
}

void enviar_syscall_init_proc(int socket, uint32_t pid, uint32_t pc ) {
    t_paquete* paquete = crear_paquete_syscall_exit(INIT_PROC, cpu_id, pid, pc);
    uint32_t bytes = 0;
    void* a_enviar = serializar_paquete_syscall_cpu_io(paquete, &bytes);
    send(socket, a_enviar, bytes, 0);
    destruir_paquete_syscall_cpu_io(paquete);
    free(a_enviar);
}

bool syscall_exit(uint32_t pid, uint32_t* pc) {
    log_info(logger_cpu, "[EXIT] PID %d finalizado correctamente", pid);

    t_paquete* paquete = crear_paquete_syscall_exit(INIT_EXIT, cpu_id, pid, *pc);
    uint32_t bytes = 0;
    void* a_enviar = serializar_paquete_syscall_cpu_io(paquete, &bytes);
    send(conexion_kernel_dispatch, a_enviar, bytes, 0);

    destruir_paquete_syscall_cpu_io(paquete);
    free(a_enviar);

    return false;
}

void enviar_syscall_dump(int socket, uint32_t pid, uint32_t pc) {
    t_paquete* paquete = crear_paquete_syscall_exit(INIT_DUMP_MEMORY, cpu_id, pid, pc);
    uint32_t bytes = 0;
    void* a_enviar = serializar_paquete_syscall_cpu_io(paquete, &bytes);
    send(socket, a_enviar, bytes, 0);
    destruir_paquete_syscall_cpu_io(paquete);
    free(a_enviar);

    // uint8_t respuesta;
    // if (recv(socket, &respuesta, sizeof(uint8_t), 0) <= 0) {
    //     log_error(logger_cpu, "Fallo al recibir confirmación de syscall DUMP");
    // } else {
    //     log_info(logger_cpu, "[SYSCALL_DUMP] Kernel respondió con código: %d", respuesta);
    // }
}

bool check_interrupt(uint32_t pid, uint32_t pc) {
    if (hay_interrupt) {
        hay_interrupt = 0;
        log_info(logger_cpu, "PID %u interrumpido en PC %u", pid, pc);

        t_paquete* paquete = crear_paquete_handshake_kernel_cpu(DEVOLVER_PCB, pid, pc);
        uint32_t bytes;
        void* serializado = serializar_paquete_kernel_cpu(paquete, &bytes);
        send(conexion_kernel_dispatch, serializado, bytes, 0);
        destruir_paquete_kernel_cpu(paquete);
        free(serializado);

        return true;
    }
    return false;
}