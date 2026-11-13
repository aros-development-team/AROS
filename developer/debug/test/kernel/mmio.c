/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Read a block of physical memory.

          For looking at a register block no driver has claimed - a pin
          controller, a clock gate - when the question is what the
          firmware left in it.

          SETW writes one 32 bit word. It takes the whole word rather
          than a bit to set, so the value has to be worked out from a
          dump first, and it prints the old value so a write can be
          undone.

          The boot page tables cover RAM and nothing else, so the block
          has to be mapped before it can be touched at all. That is the
          only reason this needs kernel.resource.
*/

#include <exec/types.h>
#include <dos/dos.h>
#include <aros/kernel.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/kernel.h>

#include <stdio.h>
#include <stdlib.h>

static const char version[] __attribute__((used)) =
    "$VER: MMIO 1.0 (13.10.2026)";

#define TEMPLATE "ADDR/A,LEN/N,BYTES/S,SETW/K"

enum { ARG_ADDR, ARG_LEN, ARG_BYTES, ARG_SETW, ARG_COUNT };

#define MAP_GRAIN   0x1000

int main(void)
{
    IPTR args[ARG_COUNT] = { 0 };
    struct RDArgs *rda;
    APTR KernelBase;
    UQUAD addr, base;
    ULONG len = 64, span, off;
    int rc = RETURN_FAIL;

    rda = ReadArgs(TEMPLATE, args, NULL);
    if (!rda)
    {
        PrintFault(IoErr(), "MMIO");
        return RETURN_FAIL;
    }

    addr = (UQUAD)strtoull((char *)args[ARG_ADDR], NULL, 0);
    if (args[ARG_LEN])
        len = (ULONG)*(LONG *)args[ARG_LEN];

    if (!len || len > 0x1000)
    {
        printf("LEN must be 1..4096\n");
        goto out;
    }

    KernelBase = OpenResource("kernel.resource");
    if (!KernelBase)
    {
        printf("no kernel.resource\n");
        goto out;
    }

    /* Mapping is done in whole pages */
    base = addr & ~(UQUAD)(MAP_GRAIN - 1);
    span = (ULONG)(((addr + len) - base + MAP_GRAIN - 1) & ~(UQUAD)(MAP_GRAIN - 1));

    if (!KrnMapGlobal((APTR)(IPTR)base, (APTR)(IPTR)base, span,
                      args[ARG_SETW] ? (MAP_Readable | MAP_Writable)
                                     : MAP_Readable))
    {
        printf("could not map %p+%lx - already claimed by a driver?\n",
               (APTR)(IPTR)base, (unsigned long)span);
        goto out;
    }

    if (args[ARG_SETW])
    {
        volatile ULONG *p = (volatile ULONG *)(IPTR)addr;
        ULONG want = (ULONG)strtoul((char *)args[ARG_SETW], NULL, 0);

        if (addr & 3)
        {
            printf("SETW needs a 4 byte aligned address\n");
            KrnUnmapGlobal((APTR)(IPTR)base, span);
            goto out;
        }

        printf("%p: was %08lx\n", (APTR)(IPTR)addr, (unsigned long)*p);
        *p = want;
        printf("%p: now %08lx (asked for %08lx)\n", (APTR)(IPTR)addr,
               (unsigned long)*p, (unsigned long)want);

        KrnUnmapGlobal((APTR)(IPTR)base, span);
        rc = RETURN_OK;
        goto out;
    }

    printf("physical %p, %lu byte(s):\n", (APTR)(IPTR)addr,
           (unsigned long)len);

    if (args[ARG_BYTES])
    {
        for (off = 0; off < len; off++)
        {
            volatile UBYTE *p = (volatile UBYTE *)(IPTR)(addr + off);

            if ((off & 15) == 0)
                printf("%s%p:", off ? "\n" : "", (APTR)(IPTR)(addr + off));
            printf(" %02x", *p);
        }
    }
    else
    {
        /* 32 bit reads by default: a byte read of a word register can
           return rubbish or hang the bus */
        len &= ~3UL;
        if (!len)
            len = 4;

        for (off = 0; off < len; off += 4)
        {
            volatile ULONG *p = (volatile ULONG *)(IPTR)(addr + off);

            if ((off & 15) == 0)
                printf("%s%p:", off ? "\n" : "", (APTR)(IPTR)(addr + off));
            printf(" %08lx", (unsigned long)*p);
        }
    }

    printf("\n");

    KrnUnmapGlobal((APTR)(IPTR)base, span);

    rc = RETURN_OK;

out:
    FreeArgs(rda);
    return rc;
}
