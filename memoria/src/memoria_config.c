#include "memoria_config.h"

ConfigMemoria leer_config(t_config* config) {
    ConfigMemoria config_leido;

    config_leido.puerto_escucha = config_get_string_value(config, "PUERTO_ESCUCHA");
    config_leido.cantidad_niveles = config_get_int_value(config, "CANTIDAD_NIVELES");
    config_leido.tam_memoria = config_get_int_value(config, "TAM_MEMORIA");
    config_leido.tam_pagina = config_get_int_value(config, "TAM_PAGINA");
    config_leido.retardo_memoria = config_get_int_value(config, "RETARDO_MEMORIA");
    config_leido.dump_path = config_get_string_value(config, "DUMP_PATH");
    config_leido.path_swapfile = config_get_string_value(config, "PATH_SWAPFILE");
    config_leido.retardo_swap = config_get_int_value(config, "RETARDO_SWAP");
    config_leido.log_level = config_get_string_value(config, "LOG_LEVEL");
    config_leido.path_instrucciones = config_get_string_value(config, "PATH_INSTRUCCIONES");

    return config_leido;
}

t_config* crear_config(void){
    t_config* nuevo_config = config_create("./memoria.config");

    return nuevo_config;
}

t_log* crear_logger(char* log_level) {
    t_log* nuevo_logger = log_create("memoria.log", "mostrar-logs", true, log_level_from_string(log_level));

    return nuevo_logger;
}
