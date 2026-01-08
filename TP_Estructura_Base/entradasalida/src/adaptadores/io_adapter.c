#include "io_adapter.h"
#include <unistd.h>

void ejecutar_sleep(t_io_sleep* io)
{
    sleep(io->tiempo);
}
