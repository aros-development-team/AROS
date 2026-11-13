/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: ScreenShot - write the front screen as a binary PPM file, after
          an optional delay. A test aid for looking at hosted runs.

    ScreenShot FILE/A,DELAY/N
*/

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/cybergraphics.h>
#include <cybergraphx/cybergraphics.h>
#include <intuition/intuitionbase.h>
#include <exec/memory.h>
#include <stdio.h>
#include <string.h>

int main(void)
{
    IPTR args[2] = { 0 };
    struct RDArgs *rda;
    struct Screen *scr;
    ULONG lock, w, h;
    UBYTE *buf;
    BPTR fh;
    char header[40];
    int rc = RETURN_FAIL;

    if (!(rda = ReadArgs("FILE/A,DELAY/N", args, NULL)))
    {
        PrintFault(IoErr(), "ScreenShot");
        return RETURN_FAIL;
    }
    if (args[1])
        Delay(*(LONG *)args[1] * 50);

    lock = LockIBase(0);
    scr = IntuitionBase->FirstScreen;
    w = scr ? scr->Width : 0;
    h = scr ? scr->Height : 0;
    UnlockIBase(lock);
    if (!scr || !(buf = AllocVec(w * h * 3, MEMF_ANY)))
    {
        printf("ScreenShot: no screen or no memory\n");
        FreeArgs(rda);
        return RETURN_FAIL;
    }
    ReadPixelArray(buf, 0, 0, w * 3, &scr->RastPort, 0, 0, w, h, RECTFMT_RGB);
    if ((fh = Open((CONST_STRPTR)args[0], MODE_NEWFILE)))
    {
        sprintf(header, "P6\n%lu %lu\n255\n", (unsigned long)w, (unsigned long)h);
        Write(fh, header, strlen(header));
        Write(fh, buf, w * h * 3);
        Close(fh);
        rc = RETURN_OK;
    }
    FreeVec(buf);
    FreeArgs(rda);
    return rc;
}
