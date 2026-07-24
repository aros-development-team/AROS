/*
    Copyright (C) 2013-2026, The AROS Development Team. All rights reserved.

    Desc: support.c - AArch64 support routines for boot
*/

#include "boot.h"

#include <sys/types.h>

static unsigned char __tmpspace[BOOT_TMP_SIZE];
static unsigned char *first_free;
static unsigned long free_memory;

void aarch64_flush_cache(uintptr_t addr, uintptr_t length)
{
        while (length)
        {
                __asm__ __volatile__("dc civac, %0" :: "r"(addr));
                addr += 64;  /* AArch64 cache line is typically 64 bytes */
                length -= (length >= 64) ? 64 : length;
        }
        __asm__ __volatile__("dsb sy" ::: "memory");
}

void aarch64_icache_invalidate(uintptr_t addr, uintptr_t length)
{
        while (length)
        {
                __asm__ __volatile__("ic ivau, %0" :: "r"(addr));
                addr += 64;
                length -= (length >= 64) ? 64 : length;
        }
        __asm__ __volatile__("dsb sy" ::: "memory");
        __asm__ __volatile__("isb" ::: "memory");
}

void *malloc(size_t size)
{
        void *ret = NULL;

        size = (size + 15) & ~15;

        if (size <= free_memory)
        {
                ret = first_free;

                first_free += size;
                free_memory -= size;
        }

        if (!ret)
                kprintf("[BOOT] malloc - OUT OF MEMORY\n");

        return ret;
}

void mem_init(void)
{
    first_free = &__tmpspace[0];
    free_memory = BOOT_TMP_SIZE;
}

void explicit_mem_init(void *first, unsigned long free)
{
    first_free = first;
    free_memory = free;
}

size_t mem_avail()
{
        return free_memory;
}

size_t mem_used()
{
        return BOOT_TMP_SIZE - free_memory;
}

int32_t strlen(const char *c)
{
        int32_t result = 0;
        while (*c++)
                result++;

        return result;
}

const char *remove_path(const char *in)
{
    const char *p = &in[strlen(in)-1];
    while (p > in && p[-1] != '/' && p[-1] != ':') p--;
    return p;
}
