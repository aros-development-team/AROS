/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Dump the Workbench screen's bitmap to a binary PPM file, for
          checking rendering from scripts. Usage: screenppm [FILE]
          (default RAM:screen.ppm)
*/

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/cybergraphics.h>

#include <cybergraphx/cybergraphics.h>
#include <intuition/screens.h>

#include <stdio.h>

int main(int argc, char **argv)
{
    CONST_STRPTR name = (argc > 1) ? argv[1] : "RAM:screen.ppm";
    struct Screen *scr = LockPubScreen(NULL);
    UBYTE *row;
    BPTR fh;
    LONG y;

    if (!scr) { printf("no public screen\n"); return RETURN_FAIL; }

    row = AllocVec(scr->Width * 3, MEMF_ANY);
    fh  = Open(name, MODE_NEWFILE);
    if (!row || !fh)
    {
        printf("could not open %s\n", name);
        if (fh) Close(fh);
        FreeVec(row);
        UnlockPubScreen(NULL, scr);
        return RETURN_FAIL;
    }

    FPrintf(fh, "P6\n%ld %ld\n255\n", (LONG)scr->Width, (LONG)scr->Height);
    Flush(fh);  /* FPrintf() is buffered, Write() below is not */
    for (y = 0; y < scr->Height; y++)
    {
        ReadPixelArray(row, 0, 0, scr->Width * 3, &scr->RastPort, 0, y, scr->Width, 1, RECTFMT_RGB);
        Write(fh, row, scr->Width * 3);
    }

    Close(fh);
    FreeVec(row);
    UnlockPubScreen(NULL, scr);
    printf("wrote %s (%dx%d)\n", name, scr->Width, scr->Height);
    return RETURN_OK;
}
