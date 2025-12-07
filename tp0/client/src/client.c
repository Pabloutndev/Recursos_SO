#include "client.h"

/// NOTE: VARIABLES GLOBALES DEL PROYECTO
t_log* logger;

int main(void)
{
	int conexion;

	char* ip;
	char* puerto;
	//char* valor;

	t_config* config;

	/* ---------------- LOGGING ---------------- */

	logger = iniciar_logger();

	log_info(logger, "Hola! Soy un log");

	/* ---------------- ARCHIVOS DE CONFIGURACION ---------------- */

	config = iniciar_config();

	ip = config_get_string_value(config, "IP");
	puerto = config_get_string_value(config, "PUERTO");

	log_info(logger, "Cliente: IP-PUERTO: %s:%s", ip, puerto);

	/* ---------------- LEER DE CONSOLA ---------------- */

	//leer_consola(logger);

	conexion = crear_conexion(ip, puerto);
	if ( conexion <= 0 ) {
		log_error(logger, "No se pudo conectar al servidor");
		terminar_programa(conexion, logger, config);
		return EXIT_FAILURE;
	}

	paquete(conexion);

	terminar_programa(conexion, logger, config);

	return EXIT_SUCCESS;
}

t_log* iniciar_logger(void)
{
	t_log* nuevo_logger = log_create("tp0.log", "TP0_LOG", 1, LOG_LEVEL_INFO);
	
	if( nuevo_logger == NULL ) {
		printf("No se pudo crear el logger\n");
		exit(EXIT_FAILURE);
	}

	return nuevo_logger;
}

t_config* iniciar_config(void)
{
	t_config* nuevo_config = config_create("cliente.config");
	
	if( nuevo_config == NULL ) {
		log_info(logger,"No se pudo crear el archivo config");
		free(logger);
		exit(EXIT_FAILURE);
	}

	return nuevo_config;
}

void leer_consola(t_log* logger)
{
	char* leido;

	while( (leido = readline("> ")) != NULL &&	strcmp(leido, "") != 0) {
		log_info(logger,"%s",leido);
		free(leido);
	}

	free(leido); 
}

void paquete(int conexion)
{
	char* leido;
	t_paquete* paquete = crear_paquete();

	while( (leido = readline("> ")) != NULL && strcmp(leido,"") != 0 )
	{
		agregar_a_paquete(paquete, leido, strlen(leido) + 1);
		free(leido);
	}
	
	free(leido);

	enviar_paquete(paquete, conexion);
	eliminar_paquete(paquete);
}

void terminar_programa(int conexion, t_log* logger, t_config* config)
{
	if ( logger != NULL ) log_destroy(logger);
	if ( config != NULL ) config_destroy(config);
	liberar_conexion(conexion);
}
