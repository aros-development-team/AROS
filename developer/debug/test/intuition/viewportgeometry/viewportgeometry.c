/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Check legacy Screen geometry updates with a larger backing bitmap.
    The red bottom band belongs to the bitmap, but must not be displayed
    after the visible height is reduced. Leave that case up for capture.
*/
#include <aros/debug.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <intuition/screens.h>
#include <graphics/view.h>

static ULONG failures;

static void check(BOOL okay, CONST_STRPTR what)
{
    if (!okay)
    {
        bug("VIEWPORT GEOMETRY FAIL: %s\n", what);
        failures++;
    }
}

int main(void)
{
    struct Screen *screen = OpenScreenTags(NULL,
        SA_Width, 320, SA_Height, 200, SA_Depth, 2,
        SA_Type, CUSTOMSCREEN, SA_Quiet, TRUE,
        SA_ShowTitle, FALSE, TAG_DONE);
    UWORD rows;

    if (!screen)
    {
        check(FALSE, "open screen");
        goto end;
    }
    rows = screen->RastPort.BitMap->Rows;
    SetRGB4(&screen->ViewPort, 0, 0, 0, 0);
    SetRGB4(&screen->ViewPort, 1, 0, 15, 0);
    SetRGB4(&screen->ViewPort, 3, 15, 0, 0);
    SetAPen(&screen->RastPort, 1);
    RectFill(&screen->RastPort, 0, 0, 319, 174);
    SetAPen(&screen->RastPort, 3);
    RectFill(&screen->RastPort, 0, 175, 319, 199);

    screen->Height = 175;
    check(MakeScreen(screen) == 0 && RethinkDisplay() == 0, "shrink visible height");
    check(screen->ViewPort.DHeight == 175, "viewport adopts smaller screen height");
    check(screen->RastPort.BitMap->Rows == rows, "backing bitmap keeps its rows");
    Delay(5);
    screen->Height = 190;
    check(MakeScreen(screen) == 0 && RethinkDisplay() == 0, "grow visible height");
    check(screen->ViewPort.DHeight == 190, "viewport can grow again");
    Delay(5);
    screen->Height = 175;
    check(MakeScreen(screen) == 0 && RethinkDisplay() == 0, "shrink after growing");
    check(screen->ViewPort.DHeight == 175, "viewport returns to smaller height");
    bug("VIEWPORT GEOMETRY READY: height %ld, bitmap rows %lu\n",
        (long)screen->ViewPort.DHeight, (unsigned long)screen->RastPort.BitMap->Rows);
    Delay(250);
    CloseScreen(screen);
end:
    bug("VIEWPORT GEOMETRY TEST: %lu failures\n", (unsigned long)failures);
    return failures ? 20 : 0;
}
