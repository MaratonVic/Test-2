#include "ioConexiones.h"

int conectar_cliente(char* ip, char* puerto, t_log* logger, char* nombre) {
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

int conectar_a_kernel(char* ip, char* puertoKernel, t_log* logger) {
    return conectar_cliente(
        ip,
        puertoKernel,
        logger, "Kernel"
    );
}