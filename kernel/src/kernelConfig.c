#include "kernelConfig.h"

t_log* iniciar_logger(void){
    t_log* nuevo_logger = log_create("kernel.log", "LOGGER_INFO_KERNEL", 1, LOG_LEVEL_INFO);
	return nuevo_logger;
}

t_config* iniciar_config(void){
	t_config* nuevo_config = config_create("kernel.config");
	if (nuevo_config == NULL){
		perror("Error al crear el config");
		exit(EXIT_FAILURE);
	}
	return nuevo_config;
}

void leer_config(t_kernelConfig* cfg, t_config* config) {
    cfg->ip = config_get_string_value(config, "IP_MEMORIA");
    cfg->puertoMemoria = config_get_string_value(config, "PUERTO_MEMORIA");
    cfg->puertoEscuchaDispatch = config_get_string_value(config, "PUERTO_ESCUCHA_DISPATCH");
    cfg->puertoEscuchaInterrupt = config_get_string_value(config, "PUERTO_ESCUCHA_INTERRUPT");
    cfg->puertoEscuchaIO = config_get_string_value(config, "PUERTO_ESCUCHA_IO");
    cfg->algoritmoCortoPlazo = config_get_string_value(config, "ALGORITMO_CORTO_PLAZO");
    cfg->algoritmoIngresoReady = config_get_string_value(config, "ALGORITMO_INGRESO_A_READY");
    cfg->alfa = config_get_string_value(config, "ALFA");
    cfg->estimacionInicial = config_get_int_value(config, "ESTIMACION_INICIAL");
    cfg->tiempoSuspension = config_get_int_value(config, "TIEMPO_SUSPENSION");
    cfg->logLevel = config_get_string_value(config, "LOG_LEVEL");
}

void loguear_config(t_kernelConfig* cfg, t_log* loggerKernel){
    log_info(loggerKernel, "Inicio del kernel...... ");
    log_info(loggerKernel, "Se cargan las configuraciones");
    log_info(loggerKernel, "IP: %s", cfg->ip);
    log_info(loggerKernel, "PUERTO_MEMORIA: %s", cfg->puertoMemoria);
    log_info(loggerKernel, "PUERTO_ESCUCHA_DISPATCH: %s", cfg->puertoEscuchaDispatch);
    log_info(loggerKernel, "PUERTO_ESCUCHA_INTERRUPT: %s", cfg->puertoEscuchaInterrupt);
    log_info(loggerKernel, "PUERTO_ESCUCHA_IO: %s", cfg->puertoEscuchaIO);
    log_info(loggerKernel, "ALGORITMO_CORTO_PLAZO: %s", cfg->algoritmoCortoPlazo);
    log_info(loggerKernel, "ALGORITMO_INGRESO_A_READY: %s", cfg->algoritmoIngresoReady);
    log_info(loggerKernel, "ALFA: %s", cfg->alfa);
    log_info(loggerKernel, "ESTIMACION_INICIAL: %d", cfg->estimacionInicial);
    log_info(loggerKernel, "TIEMPO_SUSPENSION: %d", cfg->tiempoSuspension);
    log_info(loggerKernel, "LOG_LEVEL: %s", cfg->logLevel);
}