/*
    Copyright (C) 2002-2015, The AROS Development Team.
    All rights reserved.
    
*/

#include <proto/exec.h>
#include <proto/graphics.h>

#include <clib/alib_protos.h>

#include "muimaster_intern.h"

#include <aros/symbolsets.h>

#include LC_LIBDEFS_FILE

struct Library *MUIMasterBase;

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

    return TRUE;
}

static int MUIMasterExpunge(LIBBASETYPEPTR lh)
{
    MUIMasterBase = (struct Library *)lh;
    
    if (MUIMB(lh)->SpecialMemory != NULL)
        FreeMem(MUIMB(lh)->SpecialMemory, 4);

    if (MUIMB(lh)->topaz8font != NULL)
        CloseFont(MUIMB(lh)->topaz8font);

    return TRUE;
}

ADD2INITLIB(MUIMasterInit, 0)
ADD2EXPUNGELIB(MUIMasterExpunge, 0)
