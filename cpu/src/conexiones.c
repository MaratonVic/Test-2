#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include <conexiones.h>
#include <cpu.h>

int conectar_cliente(const char* ip, const char* puerto, t_log* logger, const char* nombre) {
    struct addrinfo hints, *servinfo;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(ip, puerto, &hints, &servinfo) != 0) {
        log_error(logger, "[%s] Error en getaddrinfo", nombre);
        return -1;
    }

    int sock = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);
    if (connect(sock, servinfo->ai_addr, servinfo->ai_addrlen) == -1) {
        log_error(logger, "[%s] No se pudo conectar", nombre);
        freeaddrinfo(servinfo);
        close(sock);
        return -1;
    }

    log_info(logger, "[%s] Conectado a %s:%s", nombre, ip, puerto);
    freeaddrinfo(servinfo);
    return sock;
}

int conectar_a_memoria(t_log* logger) {
    return conectar_cliente(
        config_cpu_data.ip_memoria,
        config_cpu_data.puerto_memoria,
        logger, "Memoria"
    );
}

int conectar_a_kernel_dispatch(t_log* logger) {
    return conectar_cliente(
        config_cpu_data.ip_kernel,
        config_cpu_data.puerto_kernel_dispatch,
        logger, "Kernel Dispatch"
    );
}

int conectar_a_kernel_interrupt(t_log* logger) {
    return conectar_cliente(
        config_cpu_data.ip_kernel,
        config_cpu_data.puerto_kernel_interrupt,
        logger, "Kernel Interrupt"
    );
}