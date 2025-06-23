#include "utils.h"

int crear_conexion(char* ip, char* puerto) {

    struct addrinfo hints;
	struct addrinfo *server_info;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	getaddrinfo(ip, puerto, &hints, &server_info);

	int socket_cliente = socket(server_info->ai_family, server_info->ai_socktype, server_info->ai_protocol);

    // Puede devolver -1 si hay un error 
	connect(socket_cliente,server_info->ai_addr,server_info->ai_addrlen);

	freeaddrinfo(server_info);

	return socket_cliente;

}

int iniciar_servidor(char* puerto, t_log* logger) {
    struct addrinfo hints, *server_info;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    int err = getaddrinfo(NULL, puerto, &hints, &server_info);
    if (err != 0) {
        log_error(logger, "Error en getaddrinfo: %s", gai_strerror(err));
        return -1;
    }

    int fd_escucha = socket(server_info->ai_family, server_info->ai_socktype, server_info->ai_protocol);
    if (fd_escucha == -1) {
        log_error(logger, "Error creando socket");
        freeaddrinfo(server_info);
        return -1;
    }

    int opt = 1;
    setsockopt(fd_escucha, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    err = bind(fd_escucha, server_info->ai_addr, server_info->ai_addrlen);
    if (err != 0) {
        log_error(logger, "Fallo bind");
        close(fd_escucha);
        freeaddrinfo(server_info);
        return -1;
    }

    err = listen(fd_escucha, SOMAXCONN);
    if (err != 0) {
        log_error(logger, "Fallo listen");
        close(fd_escucha);
        freeaddrinfo(server_info);
        return -1;
    }

    log_info(logger, "Servidor escuchando en puerto %s", puerto);
    freeaddrinfo(server_info);
    return fd_escucha; 
}

int esperar_cliente(int socket_servidor)
{
	int socket_cliente = accept(socket_servidor, NULL, NULL);

	printf("Se conecto un cliente!");

	return socket_cliente;
}

int recibir_codigo_operacion(int socket) {
     int cod_op;
     if(recv(socket, &cod_op,sizeof(int), MSG_WAITALL) > 0 ){
         return cod_op;
     }else{
         return -1;
     }
 
}