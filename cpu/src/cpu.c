#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <commons/log.h>
#include <commons/config.h>

#include "cpu.h"
#include "conexiones.h"
#include "cpuCliente.h"
#include "utilsHandshake.h"
#include "dispatch.h"

t_log* logger_cpu = NULL;
ConfigCPU config_cpu_data;
t_config* config_cpu = NULL;
int conexion_memoria = -1;
int conexion_kernel_dispatch = -1;
int conexion_kernel_interrupt = -1;
char* cpu_id = NULL;

int main(int argc, char* argv[]) {

    if (argc != 2) {
        fprintf(stderr, "Uso: %s [identificador]\n", argv[0]);
        return EXIT_FAILURE;
    }

    cpu_id = argv[1];

    char log_file[64];

    sprintf(log_file, "cpu-%s.log", cpu_id);

    logger_cpu = log_create(log_file, "CPU", true, LOG_LEVEL_INFO);

    if (!logger_cpu) {
        fprintf(stderr, "No se pudo crear el logger.\n");
        return EXIT_FAILURE;
    }

    config_cpu = config_create("./cpu.config");

    if (!config_cpu) {
        log_error(logger_cpu, "No se pudo leer el archivo de configuración.");
        log_destroy(logger_cpu);
        return EXIT_FAILURE;
    }   

    cargar_config();

    signal(SIGINT, (void*)cerrar_todo);

    log_info(logger_cpu, "CPU %s inicializando...", cpu_id);

    conexion_kernel_dispatch = conectar_a_kernel_dispatch(logger_cpu);
    if (conexion_kernel_dispatch == -1) {
        log_error(logger_cpu, "Error al conectar con Kernel Dispatch.");
        cerrar_todo();
    }
    enviar_handshake_cpu(conexion_kernel_dispatch);
    log_info(logger_cpu, "[Dispatch] FD de conexión con kernel dispatch: %d", conexion_kernel_dispatch);

    enviar_handshake_cpu_io(conexion_kernel_dispatch);
    log_info(logger_cpu, "[Dispatch] Enviar segundo handshake");
    enviar_handshake_cpu_io(conexion_kernel_dispatch);
    log_info(logger_cpu, "[Dispatch] Handshake enviado (CPU ID: %s)", cpu_id);
    log_info(logger_cpu, "[Dispatch] Handshake IO/CPU enviado (CPU ID: %s)", cpu_id);
    log_info(logger_cpu, "[Dispatch] Enviar tercer handshake");
    enviar_handshake_cpu_io(conexion_kernel_dispatch);
    log_info(logger_cpu, "[Dispatch] Handshake enviado (CPU ID: %s)", cpu_id);
    log_info(logger_cpu, "[Dispatch] Handshake IO/CPU enviado (CPU ID: %s)", cpu_id);
    log_info(logger_cpu, "[Dispatch] Enviar cuarto handshake");
    enviar_handshake_cpu_io_teclado(conexion_kernel_dispatch);
    log_info(logger_cpu, "[Dispatch] Handshake enviado (CPU ID: %s)", cpu_id);
    log_info(logger_cpu, "[Dispatch] Handshake IO/CPU enviado (CPU ID: %s)", cpu_id);

    enviar_handshake_cpu_io_teclado(conexion_kernel_dispatch);
    log_info(logger_cpu, "[Dispatch] Handshake enviado (CPU ID: %s)", cpu_id);
    log_info(logger_cpu, "[Dispatch] Handshake IO/CPU enviado (CPU ID: %s)", cpu_id);

    enviar_syscall_exit(conexion_kernel_dispatch);
    enviar_syscall_dump(conexion_kernel_dispatch);

    conexion_kernel_interrupt = conectar_a_kernel_interrupt(logger_cpu);
    if (conexion_kernel_interrupt == -1) {
        log_error(logger_cpu, "Error al conectar con Kernel Interrupt.");
        cerrar_todo();
    }

    enviar_handshake_cpu(conexion_kernel_interrupt);
    log_info(logger_cpu, "[Interrupt] Handshake enviado (CPU ID: %s)", cpu_id);
    
    conexion_memoria = conectar_a_memoria(logger_cpu);
    if (conexion_memoria == -1) {
        log_error(logger_cpu, "Error al conectar con Memoria.");
        //cerrar_todo();
    }

    enviar_handshake_cpu(conexion_memoria);

    log_info(logger_cpu, "[Memoria] Handshake enviado (CPU ID: %s)", cpu_id);

    t_args_dispatch* args_dispatch = malloc(sizeof(t_args_dispatch));
    args_dispatch->socket = conexion_kernel_dispatch;
    args_dispatch->logger = logger_cpu;

    // pthread_t hilo_dispatch;
    // pthread_create(&hilo_dispatch, NULL, manejar_dispatch, args_dispatch);
    // pthread_detach(hilo_dispatch);

    pthread_t hilo_kernel_dispatch;
    pthread_create(&hilo_kernel_dispatch, NULL, hilo_escucha_kernel_cpu, args_dispatch);
    pthread_detach(hilo_kernel_dispatch);

    pause();
    
    cerrar_todo();
    
    return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////

void cerrar_todo() {
    log_info(logger_cpu, "Cerrando CPU %s...", cpu_id);

    if (conexion_memoria != -1) close(conexion_memoria);
    if (conexion_kernel_dispatch != -1) close(conexion_kernel_dispatch);
    if (conexion_kernel_interrupt != -1) close(conexion_kernel_interrupt);

    if (logger_cpu) log_destroy(logger_cpu);
    if (config_cpu) config_destroy(config_cpu);

    exit(0);
}

void cargar_config() {
    config_cpu_data.ip_memoria = config_get_string_value(config_cpu, "IP_MEMORIA");
    config_cpu_data.puerto_memoria = config_get_string_value(config_cpu, "PUERTO_MEMORIA");
    config_cpu_data.ip_kernel = config_get_string_value(config_cpu, "IP_KERNEL");
    config_cpu_data.puerto_kernel_dispatch = config_get_string_value(config_cpu, "PUERTO_KERNEL_DISPATCH");
    config_cpu_data.puerto_kernel_interrupt = config_get_string_value(config_cpu, "PUERTO_KERNEL_INTERRUPT");
    config_cpu_data.entradas_tlb = config_get_int_value(config_cpu, "ENTRADAS_TLB");
    config_cpu_data.reemplazo_tlb = config_get_string_value(config_cpu, "REEMPLAZO_TLB");
    config_cpu_data.entradas_cache = config_get_int_value(config_cpu, "ENTRADAS_CACHE");
    config_cpu_data.reemplazo_cache = config_get_string_value(config_cpu, "REEMPLAZO_CACHE");
    config_cpu_data.retardo_cache = config_get_int_value(config_cpu, "RETARDO_CACHE");
}

void* hilo_escucha_kernel_cpu(void* arg){
    t_args_dispatch* args = (t_args_dispatch*) arg;
    int socket = args->socket;
    t_log* logger = args->logger;

    free(arg);
    int estado = 1;

    while (estado) {
        t_paquete* paquete = recibir_paquete(socket);
        if (!paquete) {
            log_warning(logger, "Kernel desconectada (fd %d)", socket);
            break;
        }

        t_package_kernel_cpu* kernelCpu = deserializar_kernel_cpu(paquete->buffer);
        log_info(logger, "## PID: %d - PC: %d", kernelCpu->pid, kernelCpu->pc);
        
        bool continuar = true;
        while (continuar) {
        continuar = ciclo_instruccion(kernelCpu->pid, &(kernelCpu->pc));
        }

        pthread_mutex_lock(&mutex_pid_io);
        pid_io = kernelCpu->pid;
        pthread_mutex_unlock(&mutex_pid_io);

        // uint8_t confirmacion = 1;
        // send(socket, &confirmacion, sizeof(uint8_t), 0);
        free(kernelCpu);
        destruir_paquete(paquete);
    }

    return NULL;
}