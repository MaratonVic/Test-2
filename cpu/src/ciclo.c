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
    return continuar;
}

char* fetch(uint32_t pid, uint32_t pc) {
    int socket = conexion_memoria;

    uint8_t cod_op = 160;
    uint32_t size = sizeof(pid) + sizeof(pc);
    void* buffer = malloc(1 + 4 + size);

    int offset = 0;
    memcpy(buffer + offset, &cod_op, 1); offset += 1;
    memcpy(buffer + offset, &size, 4); offset += 4;
    memcpy(buffer + offset, &pid, sizeof(pid)); offset += sizeof(pid);
    memcpy(buffer + offset, &pc, sizeof(pc));

    send(socket, buffer, 1 + 4 + size, 0);
    free(buffer);

    uint32_t instruccion_size;
    recv(socket, &instruccion_size, sizeof(uint32_t), MSG_WAITALL);

    char* instruccion = malloc(instruccion_size);
    recv(socket, instruccion, instruccion_size, MSG_WAITALL);

    return instruccion;
}

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

    inst->parametros = partes + 1;  
    free(partes[0]);                
    free(partes);             

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
            uint32_t direccion = atoi(instruccion->parametros[0]);
            char* valor = instruccion->parametros[1];
            enviar_escritura_a_memoria(pid, direccion, valor);
            (*pc)++;
            break;
        }

        case INST_READ: {
            uint32_t direccion = atoi(instruccion->parametros[0]);
            solicitar_lectura_a_memoria(pid, direccion);
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
            enviar_syscall_init_proc(conexion_kernel_dispatch, pid);
            return false;
        }

        case INST_DUMP_MEMORY:
            enviar_syscall_dump(conexion_kernel_dispatch);
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

void solicitar_lectura_a_memoria(uint32_t pid, uint32_t direccion) {
    codigos_memoria codigo_op = CODIGO_READ;
    uint32_t total_size = sizeof(uint8_t) + sizeof(uint32_t) * 2;

    void* buffer = malloc(total_size);
    int offset = 0;

    memcpy(buffer + offset, &codigo_op, sizeof(uint8_t));
    offset += sizeof(uint8_t);

    memcpy(buffer + offset, &pid, sizeof(uint32_t));
    offset += sizeof(uint32_t);

    memcpy(buffer + offset, &direccion, sizeof(uint32_t));

    send(conexion_memoria, buffer, total_size, 0);
    free(buffer);

    uint32_t tam_valor;
    if (recv(conexion_memoria, &tam_valor, sizeof(uint32_t), MSG_WAITALL) <= 0) {
        log_error(logger_cpu, "Error al recibir tamaño del valor leído");
        return;
    }

    char* valor = malloc(tam_valor);
    if (recv(conexion_memoria, valor, tam_valor, MSG_WAITALL) <= 0) {
        log_error(logger_cpu, "Error al recibir valor leído");
        free(valor);
        return;
    }

    log_info(logger_cpu, "[READ] PID %u <- Dir %u = \"%s\"", pid, direccion, valor);
    free(valor);
}

void enviar_syscall_io_al_kernel(uint32_t pid, const char* dispositivo, uint32_t tiempo) {
    t_paquete* paquete = crear_paquete_handshake_syscall_cpu_io((char*)dispositivo, INIT_IO, INST_IO, tiempo);    
    
    uint32_t bytes = 0;
    void* serializado = serializar_paquete_syscall_cpu_io(paquete, &bytes);

    send(conexion_kernel_dispatch, serializado, bytes, 0);

    destruir_paquete_syscall_cpu_io(paquete);
    free(serializado);

    uint8_t respuesta;
    if (recv(conexion_kernel_dispatch, &respuesta, sizeof(uint8_t), 0)<= 0){
        log_error(logger_cpu, "Error al recibir respuesta del kernel para syscall IO");
    } else {
    log_info(logger_cpu, "[SYSCALL_IO] Enviado -> PID %u, dispositivo: %s, tiempo: %u (ACK: %d)", pid, dispositivo, tiempo, respuesta);
    }   
}

void enviar_syscall_init_proc(int socket, uint32_t pid) {
    t_paquete* paquete = crear_paquete_syscall_exit(INIT_PROC, cpu_id, pid);
    uint32_t bytes = 0;
    void* a_enviar = serializar_paquete_syscall_cpu_io(paquete, &bytes);
    send(socket, a_enviar, bytes, 0);
    destruir_paquete_syscall_cpu_io(paquete);
    free(a_enviar);
}