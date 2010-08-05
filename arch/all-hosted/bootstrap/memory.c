#include <stdlib.h>

void *AllocateROM(size_t len)
{
    return malloc(len);
}

void *AllocateRAM(size_t len)
{
    return malloc(len);
}
