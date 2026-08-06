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

/*
 * The bootstrap runs with the MMU off, where every access is treated as
 * Device memory and must be aligned. The C library memset/memcpy use NEON
 * STP Q stores and unaligned accesses that fault in that state, so the
 * bootstrap carries its own alignment-safe versions. optnone keeps the
 * compiler from recognising the loops and calling the routines recursively
 * or re-vectorising them.
 */
__attribute__((optnone))
void *memset(void *dst, int c, size_t n)
{
    unsigned char *d = dst;
    uint64_t fill = (unsigned char)c;

    fill |= fill << 8;
    fill |= fill << 16;
    fill |= fill << 32;

    while (n && ((uintptr_t)d & 7))
    {
        *d++ = c;
        n--;
    }
    while (n >= 8)
    {
        *(uint64_t *)d = fill;
        d += 8;
        n -= 8;
    }
    while (n--)
        *d++ = c;

    return dst;
}

__attribute__((optnone))
void *memcpy(void *dst, const void *src, size_t n)
{
    unsigned char *d = dst;
    const unsigned char *s = src;

    if ((((uintptr_t)d ^ (uintptr_t)s) & 7) == 0)
    {
        while (n && ((uintptr_t)d & 7))
        {
            *d++ = *s++;
            n--;
        }
        while (n >= 8)
        {
            *(uint64_t *)d = *(const uint64_t *)s;
            d += 8;
            s += 8;
            n -= 8;
        }
    }
    while (n--)
        *d++ = *s++;

    return dst;
}

__attribute__((optnone))
void *memmove(void *dst, const void *src, size_t n)
{
    unsigned char *d = dst;
    const unsigned char *s = src;

    if (d <= s || d >= s + n)
        return memcpy(dst, src, n);

    while (n--)
        d[n] = s[n];

    return dst;
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
