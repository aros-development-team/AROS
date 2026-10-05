/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: cybergraphics.library/WritePixelArrayAlpha() global alpha test.

    Opens a white screen in the Workbench mode and blends five 64x64 blocks
    onto it, 100 pixels apart starting at (20,20):

        1. opaque black, global alpha 0xFFFFFFFF  -> black
        2. opaque black, global alpha 0x80000000  -> mid grey (127)
        3. opaque black, global alpha 0x40000000  -> light grey (191)
        4. opaque black, global alpha 0           -> untouched (white)
        5. red at alpha 128, global 0x80000000    -> effective alpha 64

    Usage: wpaaglobal [DELAY=seconds]
*/

#include <aros/debug.h>
#include <exec/types.h>
#include <exec/memory.h>
#include <intuition/screens.h>
#include <cybergraphx/cybergraphics.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/graphics.h>
#include <proto/intuition.h>
#include <proto/cybergraphics.h>

#include <stdio.h>

#define BLOCK 64

static void fill_block(UBYTE *img, UBYTE a, UBYTE r, UBYTE g, UBYTE b)
{
    ULONG i;

    for (i = 0; i < BLOCK * BLOCK; i++)
    {
        img[i * 4 + 0] = a;
        img[i * 4 + 1] = r;
        img[i * 4 + 2] = g;
        img[i * 4 + 3] = b;
    }
}

int main(void)
{
    static const ULONG galpha[4] = { 0xFFFFFFFF, 0x80000000, 0x40000000, 0x00000000 };
    IPTR args[1] = { 0 };
    struct RDArgs *rda;
    struct Screen *wb, *scr;
    ULONG modeid = INVALID_ID, width = 640, height = 480, delay = 20, i;
    UBYTE *img;

    rda = ReadArgs("DELAY/N", args, NULL);
    if (rda)
    {
        if (args[0]) delay = *(IPTR *)args[0];
        FreeArgs(rda);
    }

    wb = LockPubScreen(NULL);
    if (wb)
    {
        modeid = GetVPModeID(&wb->ViewPort);
        width  = wb->Width;
        height = wb->Height;
        UnlockPubScreen(NULL, wb);
    }
    if (modeid == INVALID_ID)
        modeid = BestModeID(BIDTAG_NominalWidth, width, BIDTAG_NominalHeight, height,
                            BIDTAG_Depth, 24, TAG_DONE);

    scr = OpenScreenTags(NULL,
                         SA_DisplayID, modeid,
                         SA_Width,     width,
                         SA_Height,    height,
                         SA_Depth,     24,
                         SA_ShowTitle, FALSE,
                         SA_Quiet,     TRUE,
                         TAG_DONE);
    if (!scr)
    {
        printf("Cannot open screen\n");
        return RETURN_FAIL;
    }

    img = AllocVec(BLOCK * BLOCK * 4, MEMF_ANY);
    if (img)
    {
        FillPixelArray(&scr->RastPort, 0, 0, width, height, 0xFFFFFF);

        fill_block(img, 0xFF, 0, 0, 0);
        for (i = 0; i < 4; i++)
        {
            ULONG n = WritePixelArrayAlpha(img, 0, 0, BLOCK * 4, &scr->RastPort,
                                           20 + i * 100, 20, BLOCK, BLOCK, galpha[i]);
            bug("[wpaaglobal] block %lu global alpha 0x%08lx: %lu pixels\n",
                (unsigned long)i, (unsigned long)galpha[i], (unsigned long)n);
        }

        fill_block(img, 128, 0xFF, 0, 0);
        WritePixelArrayAlpha(img, 0, 0, BLOCK * 4, &scr->RastPort,
                             20 + 4 * 100, 20, BLOCK, BLOCK, 0x80000000);

        bug("[wpaaglobal] painted, waiting %lu s\n", (unsigned long)delay);
        Delay(delay * TICKS_PER_SECOND);
        FreeVec(img);
    }

    CloseScreen(scr);
    return RETURN_OK;
}
