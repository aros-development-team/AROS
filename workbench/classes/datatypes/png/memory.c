#include <exec/memory.h>
#include <proto/exec.h>
#include <stdlib.h>

void *malloc(size_t size)
{
    return AllocVec(size, MEMF_ANY);
}

void free(void *mem)
{
    FreeVec(mem);
}

void *calloc(size_t nmemb, size_t size)
{
    return AllocVec(nmemb * size, MEMF_ANY | MEMF_CLEAR);
}
