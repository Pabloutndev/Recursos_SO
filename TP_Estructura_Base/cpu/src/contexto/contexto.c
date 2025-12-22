#include <contexto/contexto.h>
#include <string.h>

void contexto_reset(contexto_t* ctx)
{
    memset(ctx, 0, sizeof(contexto_t));
}
