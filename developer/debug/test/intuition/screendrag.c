/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Move the front (Workbench) screen down over a screen opened behind
          it and back, reporting the resulting positions. Exercises the
          display composition path (ScrollVPort -> display compositor)
          without needing pointer input.

    Usage: screendrag [DELAY=seconds] [DY=pixels]
*/

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <intuition/screens.h>
#include <graphics/gfxmacros.h>
#include <stdio.h>

int main(void)
{
    IPTR args[2] = { 0, 0 };
    struct RDArgs *rda;
    LONG delay = 10, dy = 200;
    struct Screen *wb, *back;
    int fails = 0;

    rda = ReadArgs("DELAY/N,DY/N", args, NULL);
    if (rda)
    {
        if (args[0]) delay = *(LONG *)args[0];
        if (args[1]) dy    = *(LONG *)args[1];
        FreeArgs(rda);
    }

    /* A screen behind Workbench, filled in a strong colour so it is unmistakable */
    back = OpenScreenTags(NULL,
        SA_Behind,  TRUE,
        SA_Depth,   24,
        SA_Title,   (IPTR)"screendrag back screen",
        SA_Type,    CUSTOMSCREEN,
        TAG_END);
    if (!back)
    {
        printf("FAIL: cannot open back screen\n");
        return RETURN_FAIL;
    }
    SetRGB32(&back->ViewPort, 0, 0xFFFFFFFF, 0x40404040, 0x00000000);   /* orange */

    wb = LockPubScreen("Workbench");
    if (!wb)
    {
        printf("FAIL: cannot lock Workbench\n");
        CloseScreen(back);
        return RETURN_FAIL;
    }

    printf("Workbench at %d,%d; moving down by %ld\n", wb->LeftEdge, wb->TopEdge, (long)dy);
    MoveScreen(wb, 0, dy);
    printf("Workbench now at %d,%d\n", wb->LeftEdge, wb->TopEdge);
    if (wb->TopEdge != dy)
    {
        printf("FAIL: expected TopEdge %ld\n", (long)dy);
        fails++;
    }
    Delay(delay * 50);

    MoveScreen(wb, 0, -dy);
    printf("Workbench back at %d,%d\n", wb->LeftEdge, wb->TopEdge);
    if (wb->TopEdge != 0)
    {
        printf("FAIL: expected TopEdge 0\n");
        fails++;
    }
    Delay(50);

    UnlockPubScreen(NULL, wb);
    CloseScreen(back);

    printf("%s\n", fails ? "FAIL" : "PASS");
    return fails ? RETURN_FAIL : RETURN_OK;
}
