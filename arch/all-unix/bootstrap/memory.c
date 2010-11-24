#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>

#ifndef MAP_32BIT
#define MAP_32BIT 0
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
void *AllocateRO(size_t len)
{
    void *ret = mmap(NULL, len, PROT_READ|PROT_WRITE, MAP_ANON|MAP_PRIVATE|MAP_32BIT, -1, 0);

    return (ret == MAP_FAILED) ? NULL : ret;
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
    return malloc(len);
}

/*
 * This routine allocates memory usable as AROS ram. This means it
 * needs to have full permissions.
 * Yes, iOS will silently mask out PROT_EXEC here. This is bad.
 * Well, iOS will be a little bit special story in InternalLoadSeg()...
 * TODO: Make it shared for passing RAM between AROS instances upon
 * warm reboot.
 */
void *AllocateRAM(size_t len)
{
    void *ret = mmap(NULL, len, PROT_READ|PROT_WRITE|PROT_EXEC, MAP_ANON|MAP_PRIVATE|MAP_32BIT, -1, 0);

    return (ret == MAP_FAILED) ? NULL : ret;
}
