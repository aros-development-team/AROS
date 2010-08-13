#include <stdlib.h>

void *AllocateRO(size_t len)
{
    return malloc(len);
}

void *AllocateRW(size_t len)
{
    return malloc(len);
}

void *AllocateRAM(size_t len)
{
    return malloc(len);
}
