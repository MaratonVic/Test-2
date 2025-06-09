#include "cpuCliente.h"
#include "cpu.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "instrucciones.h"

void enviar_handshake_cpu(int socket) {
    t_paquete* paquete = crear_paquete_handshake(cpu_id, HANDSHAKE, CPU);
    uint32_t bytes = 0;
    void* a_enviar = serializar_paquete(paquete, &bytes);
    send(socket, a_enviar, bytes, 0);
    destruir_paquete(paquete);
    free(a_enviar);
}

void enviar_handshake_cpu_io(int socket){
    t_paquete* paquete = crear_paquete_handshake_syscall_cpu_io("impresora", INIT_PROC, INST_IO, 5000000);
    uint32_t bytes = 0;
    void* a_enviar = serializar_paquete_syscall_cpu_io(paquete, &bytes);
    send(socket, a_enviar, bytes, 0);
    destruir_paquete_syscall_cpu_io(paquete);
    free(a_enviar);
}

void enviar_handshake_cpu_io_teclado(int socket){
    t_paquete* paquete = crear_paquete_handshake_syscall_cpu_io("teclado", INIT_IO, INST_IO, 5000000);
    uint32_t bytes = 0;
    void* a_enviar = serializar_paquete_syscall_cpu_io(paquete, &bytes);
    send(socket, a_enviar, bytes, 0);
    destruir_paquete_syscall_cpu_io(paquete);
    free(a_enviar);
}

void enviar_syscall_exit(int socket){
    t_paquete* paquete = crear_paquete_syscall_exit(INIT_EXIT, cpu_id, 0);
    uint32_t bytes = 0;
    void* a_enviar = serializar_paquete_syscall_cpu_io(paquete, &bytes);
    send(socket, a_enviar, bytes, 0);
    destruir_paquete_syscall_cpu_io(paquete);
    free(a_enviar);

}

void enviar_syscall_dump(int socket){
    t_paquete* paquete = crear_paquete_syscall_exit(INIT_DUMP_MEMORY, cpu_id, 1);
    uint32_t bytes = 0;
    void* a_enviar = serializar_paquete_syscall_cpu_io(paquete, &bytes);
    send(socket, a_enviar, bytes, 0);
    destruir_paquete_syscall_cpu_io(paquete);
    free(a_enviar);

}