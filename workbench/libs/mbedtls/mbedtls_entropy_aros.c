/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
    Author: Fabian Schmieder (@metaneutrons)

    mbedTLS hardware entropy source for AROS.
    Uses BCM2711/BCM2712 RNG200 via peripheral base, or falls back
    to timer-based jitter entropy.
*/

#include "mbedtls/entropy.h"
#include <proto/exec.h>
#include <proto/kernel.h>
#include <string.h>

/* kernel.resource, opened on first use - linking programs need not
 * provide a KernelBase of their own */
static APTR krnres;

/*
 * mbedtls_hardware_poll — called by mbedTLS entropy collector.
 * Must fill 'output' with 'len' random bytes and set *olen.
 */
int mbedtls_hardware_poll(void *data, unsigned char *output,
                          size_t len, size_t *olen)
{
    /* shadows proto/kernel.h's extern so the Krn inline uses our base */
    APTR KernelBase;
    IPTR peribase;

    (void)data;

    if (krnres == NULL)
        krnres = OpenResource("kernel.resource");
    KernelBase = krnres;

    /* Try RNG200 hardware (BCM2711 at peribase+0x104000) */
    peribase = KernelBase ? KrnGetSystemAttr(KATTR_PeripheralBase) : 0;
    if (peribase)
    {
        volatile ULONG *rng_data = (volatile ULONG *)(peribase + 0x104000 + 0x20);
        volatile ULONG *rng_count = (volatile ULONG *)(peribase + 0x104000 + 0x24);
        size_t filled = 0;

        while (filled < len)
        {
            /* Wait for data available */
            int timeout = 10000;
            while (!(*rng_count & 0xFF) && --timeout > 0)
                ;
            if (timeout <= 0)
                break;

            ULONG val = *rng_data;
            size_t chunk = len - filled;
            if (chunk > 4) chunk = 4;
            memcpy(output + filled, &val, chunk);
            filled += chunk;
        }

        if (filled == len)
        {
            *olen = len;
            return 0;
        }
    }

    /* Fallback: timer jitter (less secure but functional) */
    {
        size_t i;
        ULONG seed = 0;
        for (i = 0; i < len; i++)
        {
#if defined(__aarch64__)
            ULONG cnt;
            __asm__ volatile("mrs %0, cntpct_el0" : "=r"(cnt));
            seed ^= cnt;
#elif defined(__x86_64__) || defined(__i386__)
            ULONG lo, hi;
            __asm__ volatile("rdtsc" : "=a"(lo), "=d"(hi));
            seed ^= lo ^ hi;
#else
            seed ^= (ULONG)(IPTR)&seed ^ (ULONG)i;
#endif
            seed = (seed * 1103515245 + 12345);
            output[i] = (unsigned char)(seed >> 16);
        }
        *olen = len;
    }

    return 0;
}
