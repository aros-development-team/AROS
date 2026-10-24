/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Print the pointer position as the frontmost screen sees it, a few
          times. With the host pointer moved from outside (harness/hosted/
          xpointer on X11) this checks the display -> screen coordinate
          conversion of scaled screens (COMPF_SCALE / SF_ScaledCoords).

    Usage: pointerpos [DELAY=seconds] [COUNT=n]
*/

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <intuition/intuitionbase.h>
#include <stdio.h>

int main(void)
{
    IPTR args[2] = { 0, 0 };
    struct RDArgs *rda;
    LONG delay = 2, count = 5, i;

    rda = ReadArgs("DELAY/N,COUNT/N", args, NULL);
    if (rda)
    {
        if (args[0]) delay = *(LONG *)args[0];
        if (args[1]) count = *(LONG *)args[1];
        FreeArgs(rda);
    }

    /* Make sure a screen exists (there is none before the first window on hosted) */
    {
        struct Screen *wb = LockPubScreen("Workbench");
        if (wb)
            UnlockPubScreen(NULL, wb);
    }

    for (i = 0; i < count; i++)
    {
        struct Screen *s;
        ULONG lock;

        Delay(delay * 50);
        lock = LockIBase(0);
        s = IntuitionBase->FirstScreen;
        printf("SAMPLE %ld: display %d,%d screen %dx%d at %d,%d mouse %d,%d\n", (long)i,
            IntuitionBase->MouseX, IntuitionBase->MouseY,
            s ? s->Width : 0, s ? s->Height : 0, s ? s->LeftEdge : 0, s ? s->TopEdge : 0,
            s ? s->MouseX : 0, s ? s->MouseY : 0);
        UnlockIBase(lock);
    }
    return RETURN_OK;
}
