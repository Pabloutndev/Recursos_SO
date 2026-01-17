#ifndef SERVER_H_
#define SERVER_H_

int server_init(char* port);
void server_listen_loop(void);
void server_shutdown(void);
void* memoria_client_handler(void* arg);
void* atender_cliente(void* arg);

#endif
