#ifndef SERVER_H_
#define SERVER_H_

int server_init(const char* port);
void server_listen_loop(void);
void server_shutdown(void);

#endif
