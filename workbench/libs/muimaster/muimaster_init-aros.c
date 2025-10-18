/*
    Copyright (C) 2002-2025, The AROS Development Team.
    All rights reserved.
    
*/

#include <stdio.h>

#include <proto/exec.h>
#include <proto/graphics.h>

#include <clib/alib_protos.h>

#include "muimaster_intern.h"

#include <aros/symbolsets.h>

#include LC_LIBDEFS_FILE

//#define ZUNE_FORCE_SYSPENS

struct Library *MUIMasterBase;
struct Library *MUIScreenBase;

static struct TextAttr topaz8Attr =
    { "topaz.font", 8, FS_NORMAL, FPF_ROMFONT, };


static int MUIMasterInit(LIBBASETYPEPTR lh)
{
    MUIMasterBase = (struct Library *)lh;

    InitSemaphore(&MUIMB(lh)->ZuneSemaphore);

    NewList((struct List *)&MUIMB(lh)->BuiltinClasses);
    NewList((struct List *)&MUIMB(lh)->Applications);

    MUIMB(lh)->topaz8font = OpenFont(&topaz8Attr);

    /* Attempt to allocate memory locations corresponding to Notify class's
     * special values (to avoid clashes) */
    MUIMB(lh)->SpecialMemory = AllocAbs(4, (APTR)MUIV_TriggerValue);

    /* Initalize the default "MUI" pens-specs */
    MUIMB(lh)->defaultPens = AllocMem(sizeof(struct MUI_PenSpec) * MPEN_COUNT, MEMF_ANY);
    struct MUI_PenSpec *defaultPen = MUIMB(lh)->defaultPens;

    // 'MPEN_SHINE'
    snprintf(defaultPen->buf, sizeof(defaultPen->buf), "%lc%d", (int)PST_SYS, (int)SHINEPEN);
    defaultPen++;
    // MPEN_HALFSHINE
#if !defined(ZUNE_FORCE_SYSPENS)
    defaultPen->buf[0] = 0; // Let the window class calculate the spec to use
#else
#if (MAXPENS > 8)
    snprintf(defaultPen->buf, sizeof(defaultPen->buf), "%lc%d", (int)PST_SYS, (int)BARBLOCKPEN);
#else
    snprintf(defaultPen->buf, sizeof(defaultPen->buf), "%lc%d", (int)PST_SYS, (int)SHINEPEN);
#endif
#endif
    defaultPen++;
    // MPEN_BACKGROUND
    snprintf(defaultPen->buf, sizeof(defaultPen->buf), "%lc%d", (int)PST_SYS, (int)BACKGROUNDPEN);
    defaultPen++;
    // MPEN_HALFSHADOW
#if !defined(ZUNE_FORCE_SYSPENS)
    defaultPen->buf[0] = 0; // Let the window class calculate the spec to use
#else
#if (MAXPENS > 8)
    snprintf(defaultPen->buf, sizeof(defaultPen->buf), "%lc%d", (int)PST_SYS, (int)BARTRIMPEN);
#else
    snprintf(defaultPen->buf, sizeof(defaultPen->buf), "%lc%d", (int)PST_SYS, (int)SHADOWPEN);
#endif
#endif
    defaultPen++;
    // MPEN_SHADOW
    snprintf(defaultPen->buf, sizeof(defaultPen->buf), "%lc%d", (int)PST_SYS, (int)SHADOWPEN);
    defaultPen++;
    // MPEN_TEXT
    snprintf(defaultPen->buf, sizeof(defaultPen->buf), "%lc%d", (int)PST_SYS, (int)TEXTPEN);
    defaultPen++;
    // MPEN_FILL
    snprintf(defaultPen->buf, sizeof(defaultPen->buf), "%lc%d", (int)PST_SYS, (int)FILLPEN);
    defaultPen++;
    // MPEN_MARK
    snprintf(defaultPen->buf, sizeof(defaultPen->buf), "%lc%d", (int)PST_SYS, (int)HIGHLIGHTTEXTPEN);
    defaultPen++;

    MUIScreenBase = OpenLibrary("muiscreen.library", 0);

    return TRUE;
}

static int MUIMasterExpunge(LIBBASETYPEPTR lh)
{
    MUIMasterBase = (struct Library *)lh;

    if (MUIScreenBase)
        CloseLibrary(MUIScreenBase);

    if (MUIMB(lh)->defaultPens != NULL)
        FreeMem(MUIMB(lh)->defaultPens, sizeof(struct MUI_PenSpec) * MPEN_COUNT);

    if (MUIMB(lh)->SpecialMemory != NULL)
        FreeMem(MUIMB(lh)->SpecialMemory, 4);

    if (MUIMB(lh)->topaz8font != NULL)
        CloseFont(MUIMB(lh)->topaz8font);

    return TRUE;
}

ADD2INITLIB(MUIMasterInit, 0)
ADD2EXPUNGELIB(MUIMasterExpunge, 0)
