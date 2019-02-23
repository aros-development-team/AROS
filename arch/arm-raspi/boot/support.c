/*
    Copyright © 2013-2019, The AROS Development Team. All rights reserved.
    $Id$
 */

#include "boot.h"

#include <sys/types.h>

static unsigned char __tmpspace[BOOT_TMP_SIZE];
static unsigned char *first_free;
static unsigned long free_memory;

void arm_flush_cache(uint32_t addr, uint32_t length)
{
        while (length)
        {
                __asm__ __volatile__("mcr p15, 0, %0, c7, c14, 1"::"r"(addr));
                addr += 32;
                length -= 32;
        }
        __asm__ __volatile__("mcr p15, 0, %0, c7, c10, 4"::"r"(addr));
}

void arm_icache_invalidate(uint32_t addr, uint32_t length)
{
        while (length)
        {
                __asm__ __volatile__("mcr p15, 0, %0, c7, c5, 1"::"r"(addr));
                addr += 32;
                length -= 32;
        }
        __asm__ __volatile__("mcr p15, 0, %0, c7, c10, 4"::"r"(addr));
}

void arm_dcache_invalidate(uint32_t addr, uint32_t length)
{
        while (length)
        {
                __asm__ __volatile__("mcr p15, 0, %0, c7, c6, 1"::"r"(addr));
                addr += 32;
                length -= 32;
        }
        __asm__ __volatile__("mcr p15, 0, %0, c7, c10, 4"::"r"(addr));
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
