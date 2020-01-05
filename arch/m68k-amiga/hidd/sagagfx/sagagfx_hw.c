/*
    Copyright © 1995-2020, The AROS Development Team. All rights reserved.
    $Id$

    Desc: SAGA Gfx Hidd for V4 AROS
    Lang: english
*/
#define DEBUG 1
#include <aros/debug.h>

#include <exec/exec.h>
#include <devices/inputevent.h>
#include <proto/exec.h>
#include <proto/input.h>

#include "sagagfx_hw.h"

/* Set PLL of SAGA according to the requested clock */
void SAGA_SetPLL(ULONG clock)
{
    /* Calculate AUDIO CTS value (Hz * 6144) / (128 * 48000) */
    ULONG pll = clock / 1000; //(UQUAD)clock * 6144 / (128 * 48000);

    /* PLL-CTS command */
    pll |= 0x24000000;

    D(bug("SAGA_SetPLL(%d) -> %08x\n", clock, pll));

    /* Make sure we are not interrupted when setting PLL */
    Disable();
    /* DAC OFF */
    WRITE16(SAGA_VIDEO_MODE, 0x8000);
    /* PLL CTS */
    WRITE32(SAGA_VIDEO_PLLV4, pll);
    Enable();
}

void SAGA_LoadCLUT(ULONG *palette, UWORD startIndex, UWORD count)
{
    if (palette)
    {
        if(startIndex > 255)
            return;

        if(startIndex + count > 256)
            count = 256 - startIndex;

        for (int i=0; i < count; i++)
        {
            WRITE32(SAGA_VIDEO_CLUT(startIndex + i), palette[startIndex + i]);
        }
    }
}

/* Attempts to detect SAGA. */
BOOL SAGA_Init()
{
    struct IORequest io;

    /* Do we have V4 detected? */
    if (5 != (READ16(VREG_BOARD) >> 8))
    {
        return FALSE;
    }

    /* If SHIFT key was pressed during boot, do not initialize SAGA Gfx hidd */
    if (0 == OpenDevice("input.device", 0, &io, 0))
    {
        struct Library *InputBase = (struct Library *)io.io_Device;
        UWORD qual = PeekQualifier();
        CloseDevice(&io);

        if (qual & (IEQUALIFIER_LSHIFT | IEQUALIFIER_RSHIFT))
        {
            return FALSE;
        }
    }

    return TRUE;
}
