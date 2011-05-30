/*
    Copyright © 2002-2011, The AROS Development Team. 
    All rights reserved.
    
    $Id$
*/

#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/graphics.h>

#include <clib/alib_protos.h>

#include "muimaster_intern.h"
#include "mui.h"

#include <aros/symbolsets.h>

#include <dos/dos.h>
#include <aros/debug.h>

#include LC_LIBDEFS_FILE

struct Library *MUIMasterBase;

static struct TextAttr topaz8Attr =
    { "topaz.font", 8, FS_NORMAL, FPF_ROMFONT, };

/****************************************************************************************/

static int MUIMasterInit(LIBBASETYPEPTR lh)
{   
    MUIMasterBase = (struct Library *)lh;
    
    InitSemaphore(&MUIMB(lh)->ZuneSemaphore);
    
    NewList((struct List *)&MUIMB(lh)->BuiltinClasses);
    NewList((struct List *)&MUIMB(lh)->Applications);

    ((struct MUIMasterBase_intern *)MUIMasterBase)->topaz8font = OpenFont(&topaz8Attr);

    return TRUE;
}

static int MUIMasterExpunge(LIBBASETYPEPTR lh)
{
    MUIMasterBase = (struct Library *)lh;
    
    if (((struct MUIMasterBase_intern *)MUIMasterBase)->topaz8font)
        CloseFont(((struct MUIMasterBase_intern *)MUIMasterBase)->topaz8font);

    return TRUE;
}

ADD2INITLIB(MUIMasterInit, 0);
ADD2EXPUNGELIB(MUIMasterExpunge, 0);

/****************************************************************************************/
