/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    ACTION_CONSOLE_GEOMETRY on a console window: the text area in characters
    and pixels, filled to the size the caller gives, without touching the
    console's mode or input.
*/

#include <dos/dos.h>
#include <dos/dosextens.h>
#include <proto/dos.h>
#include <proto/exec.h>

#include <string.h>

static int failures;

static void check(int ok, const char *what)
{
    Printf("%s: %s\n", (IPTR)(ok ? "PASS" : "FAIL"), (IPTR)what);
    if (!ok)
        failures++;
}

static LONG geometry(BPTR fh, APTR cg, LONG size)
{
    struct FileHandle *h = BADDR(fh);

    return DoPkt(h->fh_Type, ACTION_CONSOLE_GEOMETRY, (SIPTR)h->fh_Arg1, (SIPTR)cg, size, 0, 0);
}

int main(void)
{
    struct ConsoleGeometry cg, part;
    BPTR fh;
    LONG res, res2;

    if (!(fh = Open((CONST_STRPTR)"CON:0/0/640/200/congeometry/CLOSE", MODE_OLDFILE)))
    {
        Printf("FAIL: cannot open a console window\n");
        return RETURN_FAIL;
    }

    memset(&cg, 0, sizeof(cg));
    res = geometry(fh, &cg, sizeof(cg));
    res2 = IoErr();
    check(res == DOSTRUE && res2 == sizeof(cg), "the packet is answered with the structure size");
    check(cg.cg_Columns > 0 && cg.cg_Rows > 0, "columns and rows are known");
    check(cg.cg_Width > 0 && cg.cg_Width <= 640 && cg.cg_Height > 0 && cg.cg_Height <= 200,
          "the text area fits the 640x200 window");
    check(cg.cg_Width >= cg.cg_Columns && cg.cg_Height >= cg.cg_Rows, "a character is at least a pixel");

    /* a caller with an older, shorter structure gets what fits */
    memset(&part, 0x55, sizeof(part));
    res = geometry(fh, &part, 8);
    check(res == DOSTRUE && IoErr() == sizeof(cg) && part.cg_Columns == cg.cg_Columns &&
          part.cg_Rows == cg.cg_Rows && part.cg_Width == 0x55555555 && part.cg_Height == 0x55555555,
          "a shorter structure is filled only as far as it goes");

    check(geometry(fh, NULL, sizeof(cg)) == DOSFALSE && IoErr() == ERROR_BAD_NUMBER,
          "no structure is refused");

    /* the query leaves the console as it was: raw mode stays, nothing is readable */
    check(SetMode(fh, 1), "console switched to raw");
    memset(&cg, 0, sizeof(cg));
    check(geometry(fh, &cg, sizeof(cg)) == DOSTRUE && cg.cg_Columns > 0, "answered in raw mode too");
    check(!WaitForChar(fh, 200000), "nothing became readable");
    SetMode(fh, 0);

    Close(fh);
    Printf("CONGEOMETRY %s\n", (IPTR)(failures ? "FAIL" : "PASS"));
    return failures ? RETURN_WARN : RETURN_OK;
}
