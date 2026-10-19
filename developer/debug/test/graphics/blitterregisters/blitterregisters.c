/* Copyright (C) 2026, The AROS Development Team. All rights reserved. */
/* Native Amiga register ABI and guarded blit, selected by target build rules. */
#include <aros/debug.h>
#include <exec/memory.h>
#include <graphics/gfxbase.h>
#include <hardware/custom.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/dos.h>

struct GfxBase *GfxBase;

static void probe(BOOL own, APTR source, APTR target, ULONG *result)
{
    register ULONG d0 __asm("d0") = 0x441; /* 17 rows of one word */
    register ULONG d1 __asm("d1") = 0x12345678;
    register APTR a0 __asm("a0") = source;
    register APTR a1 __asm("a1") = target;
    register struct GfxBase *a6 __asm("a6") = GfxBase;

    if (own)
        __asm volatile("jsr %4@(-456)"
            : "+r"(d0), "+r"(d1), "+r"(a0), "+r"(a1) : "r"(a6) : "cc", "memory");
    else
        __asm volatile("jsr %4@(-462)"
            : "+r"(d0), "+r"(d1), "+r"(a0), "+r"(a1) : "r"(a6) : "cc", "memory");

    result[0] = d0;
    result[1] = d1;
    result[2] = (IPTR)a0;
    result[3] = (IPTR)a1;
}

static BOOL preserved(ULONG *r, APTR source, APTR target)
{
    return r[0] == 0x441 && r[1] == 0x12345678 &&
           r[2] == (IPTR)source && r[3] == (IPTR)target;
}

int main(void)
{
    volatile struct Custom *custom = (APTR)0xdff000;
    UBYTE *source, *target;
    ULONG own[4], disown[4];
    unsigned i, failures = 0;
    BOOL okay;

    GfxBase = (struct GfxBase *)OpenLibrary("graphics.library", 0);
    if (!GfxBase) return RETURN_FAIL;
    source = AllocMem(128, MEMF_CHIP);
    target = AllocMem(128, MEMF_CHIP);
    if (!source || !target)
    {
        if (source) FreeMem(source, 128);
        if (target) FreeMem(target, 128);
        CloseLibrary((struct Library *)GfxBase);
        return RETURN_FAIL;
    }
    for (i = 0; i < 128; i++)
    {
        source[i] = (i * 37) ^ 0x6d;
        target[i] = 0xa5;
    }

    probe(TRUE, source + 16, target + 16, own);
    okay = preserved(own, source + 16, target + 16);
    bug("BLITTER REGISTERS Own: %08lx %08lx %08lx %08lx\n",
        own[0], own[1], own[2], own[3]);
    if (!okay) failures++;
    /* Do not let an unchanged ROM program an oversized, destructive blit. */
    if (okay)
    {
        WaitBlit();
        custom->bltcon0 = 0x09f0; /* A -> D */
        custom->bltcon1 = 0;
        custom->bltafwm = custom->bltalwm = 0xffff;
        custom->bltamod = custom->bltdmod = 0;
        custom->bltapt = (APTR)own[2];
        custom->bltdpt = (APTR)own[3];
        custom->bltsize = own[0];
        WaitBlit();
    }
    probe(FALSE, source + 16, target + 16, disown);
    bug("BLITTER REGISTERS Disown: %08lx %08lx %08lx %08lx\n",
        disown[0], disown[1], disown[2], disown[3]);
    if (!preserved(disown, source + 16, target + 16)) failures++;

    if (okay)
        for (i = 0; i < 128; i++)
            if (target[i] != (i >= 16 && i < 50 ? source[i] : 0xa5))
            {
                failures++;
                bug("BLITTER REGISTERS: copied data or guard differs at %u\n", i);
                break;
            }

    FreeMem(source, 128);
    FreeMem(target, 128);
    CloseLibrary((struct Library *)GfxBase);
    bug("BLITTER REGISTERS TEST: %u failures\n", failures);
    return failures ? RETURN_FAIL : RETURN_OK;
}
