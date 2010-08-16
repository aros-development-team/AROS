#define _XOPEN_SOURCE 600L

#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>

static long pagesize;

void *AllocateRO(size_t len)
{
    void *ret;

    /* This routine is called first, so we set uo pagesize here */
    pagesize = sysconf(_SC_PAGESIZE);

    if (posix_memalign(&ret, pagesize, len))
	return NULL;

    /* Enable execution from the allocated area */
    if (mprotect(ret, len, PROT_READ | PROT_WRITE | PROT_EXEC)) {
	free(ret);
	return NULL;
    }

    return ret;
}

void *AllocateRW(size_t len)
{
    return malloc(len);
}

/* TODO: implement shared memory for RAM */
void *AllocateRAM(size_t len)
{
    void *ret;

    if (posix_memalign(&ret, pagesize, len))
	return NULL;

    /* Enable execution from the allocated area */
    if (mprotect(ret, len, PROT_READ | PROT_WRITE | PROT_EXEC)) {
	free(ret);
	return NULL;
    }

    return ret;
}
