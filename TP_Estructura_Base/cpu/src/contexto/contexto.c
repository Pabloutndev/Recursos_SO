#include <contexto/contexto.h>
#include <string.h>

void contexto_reset(t_contexto_cpu* ctx)
{
    memset(ctx, 0, sizeof(t_contexto_cpu));
}
