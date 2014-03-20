/*
    Copyright © 2014, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/

#include "boot.h"

#include <sys/types.h>

void arm_flush_cache(uint32_t addr, uint32_t length) {
        while (length) {
                __asm__ __volatile__("mcr p15, 0, %0, c7, c14, 1"::"r"(addr));
                addr += 32;
                length -= 32;
        }
        __asm__ __volatile__("mcr p15, 0, %0, c7, c10, 4"::"r"(addr));
}

void arm_icache_invalidate(uint32_t addr, uint32_t length) {
        while (length) {
                __asm__ __volatile__("mcr p15, 0, %0, c7, c5, 1"::"r"(addr));
                addr += 32;
                length -= 32;
        }
        __asm__ __volatile__("mcr p15, 0, %0, c7, c10, 4"::"r"(addr));
}

void arm_dcache_invalidate(uint32_t addr, uint32_t length) {
        while (length) {
                __asm__ __volatile__("mcr p15, 0, %0, c7, c6, 1"::"r"(addr));
                addr += 32;
                length -= 32;
        }
        __asm__ __volatile__("mcr p15, 0, %0, c7, c10, 4"::"r"(addr));
}

int32_t strlen(const char *c) {
        int32_t result = 0;
        while (*c++)
                result++;

        return result;
}

const char *remove_path(const char *in) {
    const char *p = &in[strlen(in)-1];
    while (p > in && p[-1] != '/' && p[-1] != ':') p--;
    return p;
}
