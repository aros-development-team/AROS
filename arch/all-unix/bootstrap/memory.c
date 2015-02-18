#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>

#include <aros/cpu.h> // for __WORDSIZE

#include "bootstrap.h"

#ifndef MAP_32BIT
#define MAP_32BIT 0
#endif

static void *data = NULL;
static void *code = NULL;
static size_t code_len = 0;

static void *RAM32  = NULL;
static size_t RAM32_len  = 0;

#if (__WORDSIZE == 64)
static void *RAM  = NULL;
static size_t RAM_len  = 0;
#endif

/*
 * Allocate memory for kickstart's .code and .rodata. We allocate is as writable
 * because we will load the kickstart into it. We will enable execution later in SetRO().
 * We have to use mmap() and not posix_memalign() here because posix_memalign()
 * does not pad the allocated memory up to next page boundary. As a result, setting
 * it read-only will affect the whole page, but the page will still have some unallocated
 * space which can be reused by malloc().
 * This causes DisplayError() function to crash on iOS. This also may cause similar effects
 * on other systems.
 */
 
 void *doMMap(void **addr_store, size_t *size_store, size_t len, int prot, int flags)
 {
    /* There's no sense to set MAP_SHARED for ROM */
    void *ret = mmap(NULL, len, prot, flags, -1, 0);

    if (ret == MAP_FAILED)
    	ret = NULL;
    else
    {
        if (addr_store)
            *addr_store = ret;
        if (size_store)
            *size_store = len;
    }
    return ret;
 }

void *AllocateRO(size_t len)
{
    /* There's no sense to set MAP_SHARED for ROM */
    return doMMap(&code, &code_len, len, PROT_READ|PROT_WRITE, MAP_ANON|MAP_PRIVATE|MAP_32BIT);
}

/*
 * Disable write and enable execution.
 * We actually have to do this in two steps because some systems
 * (Apple iOS) do not allow read-write-execute permissions.
 */
int SetRO(void *addr, size_t len)
{
    return mprotect(addr, len, PROT_READ|PROT_EXEC);
}

/*
 * Allocate kickstart's .data and .bss. Nothing is executed from there, so we
 * don't have to take some special care about it. Simple malloc() is enough here.
 */
void *AllocateRW(size_t len)
{
    data = malloc(len);
    return data;
}

/*
 * These routines allocate memory usable as AROS ram. This means it
 * needs to have full permissions.
 * Yes, iOS will silently mask out PROT_EXEC here. This is bad.
 * Well, iOS will be a little bit special story in InternalLoadSeg()...
 */
void *AllocateRAM32(size_t len)
{
    return doMMap(&RAM32, &RAM32_len, len, PROT_READ|PROT_WRITE|PROT_EXEC, MAP_ANON|MAP_SHARED|MAP_32BIT);
}

#if (__WORDSIZE == 64)
void *AllocateRAM(size_t len)
{
    return doMMap(&RAM, &RAM_len, len, PROT_READ|PROT_WRITE|PROT_EXEC, MAP_ANON|MAP_SHARED);
}
#endif

void Host_FreeMem(void)
{
    munmap(code, code_len);
    free(data);
#if (__WORDSIZE == 64)
    if (RAM)
        munmap(RAM, RAM_len);
#endif
    munmap(RAM32, RAM32_len);

    free(SystemVersion);    
    if (KernelArgs)
    	free(KernelArgs);
}
