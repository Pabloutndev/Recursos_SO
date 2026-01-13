#ifndef KERNEL_CON_IO_H
#define KERNEL_CON_IO_H

void server_io_init(char* puerto);
void* server_listen_loop_io(void* arg);

#endif
