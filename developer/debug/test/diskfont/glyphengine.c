/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Regression test for freetype2.library OpenEngine/CloseEngine: a closed
    engine must give back all of its memory. CloseEngine used to release the
    FreeType library object with FT_Done_Library, which leaves the memory pool
    made by FT_Init_FreeType allocated (12288 bytes per engine on x86_64).

    Usage: glyphengine [LIB=<name or path>] [CYCLES=<n>]
    Silent and RETURN_OK on success; prints the loss and fails otherwise.
*/

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/bullet.h>
#include <exec/memory.h>
#include <dos/dos.h>

struct Library *BulletBase;

int main(void)
{
    IPTR args[2] = { (IPTR)"freetype2.library", 0 };
    struct RDArgs *rda;
    struct GlyphEngine *ge;
    LONG cycles = 20, i, loss;
    ULONG before, after;
    int rc = RETURN_OK;

    rda = ReadArgs("LIB,CYCLES/N", args, NULL);
    if (!rda)
    {
        PrintFault(IoErr(), "glyphengine");
        return RETURN_FAIL;
    }
    if (args[1])
        cycles = *(LONG *)args[1];

    BulletBase = OpenLibrary((STRPTR)args[0], 0);
    if (!BulletBase)
    {
        Printf("glyphengine: cannot open %s\n", args[0]);
        FreeArgs(rda);
        return RETURN_FAIL;
    }

    /* The first engine may pay one-time costs; they are not a leak. */
    ge = OpenEngine();
    if (ge)
        CloseEngine(ge);

    before = AvailMem(MEMF_ANY);
    for (i = 0; i < cycles; i++)
    {
        ge = OpenEngine();
        if (!ge)
        {
            Printf("glyphengine: OpenEngine failed in cycle %ld\n", i);
            rc = RETURN_FAIL;
            break;
        }
        CloseEngine(ge);
    }
    after = AvailMem(MEMF_ANY);

    /* Other tasks may allocate meanwhile; allow less than 1 KB per cycle. */
    loss = (LONG)(before - after);
    if (rc == RETURN_OK && cycles > 0 && loss >= cycles * 1024)
    {
        Printf("glyphengine: %ld bytes lost over %ld engines (%ld per engine)\n",
               loss, cycles, loss / cycles);
        rc = RETURN_ERROR;
    }

    CloseLibrary(BulletBase);
    FreeArgs(rda);
    return rc;
}
