/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$

*/

#include <utility/utility.h> /* this must be before icon_intern.h */

#include <aros/symbolsets.h>

#define __ICON_NOLIBBASE__

#include "icon_intern.h"
#include "identify.h"

#include <proto/datatypes.h>

#include LC_LIBDEFS_FILE

LONG IFFParseBase_version = -39,
    GfxBase_version = -39,
    CyberGfxBase_version = -39;

LIBBASETYPE *IconBase;

/****************************************************************************************/

AROS_SET_LIBFUNC(Init, LIBBASETYPE, lh)
{
    LONG i;

    IconBase = lh;
    
    /* Initialize memory pool ----------------------------------------------*/
    if (!(LB(lh)->ib_MemoryPool = CreatePool(MEMF_ANY | MEMF_SEM_PROTECTED, 8194, 8194)))
    {
        return FALSE;
    }
    
    LB(lh)->dsh.h_Entry = (void *)AROS_ASMSYMNAME(dosstreamhook);
    LB(lh)->dsh.h_Data  = lh;

    InitSemaphore(&LB(lh)->iconlistlock);
    for(i = 0; i < ICONLIST_HASHSIZE; i++)
    {
    	NewList((struct List *)&LB(lh)->iconlists[i]);
    }

    /* Setup default identify hook -----------------------------------------*/
    LB(lh)->ib_DefaultIdentifyHook.h_Entry = (HOOKFUNC) FindDefaultIcon;
    LB(lh)->ib_DefaultIdentifyHook.h_Data  = DataTypesBase;
    
    /* Setup default global settings ---------------------------------------*/
    LB(lh)->ib_Screen               = NULL; // FIXME: better default
    LB(lh)->ib_Precision            = PRECISION_ICON;
    LB(lh)->ib_EmbossRectangle.MinX = 0; // FIXME: better default
    LB(lh)->ib_EmbossRectangle.MaxX = 0; 
    LB(lh)->ib_EmbossRectangle.MinY = 0; 
    LB(lh)->ib_EmbossRectangle.MaxY = 0; 
    LB(lh)->ib_Frameless            = TRUE;
    LB(lh)->ib_IdentifyHook         = NULL; // FIXME: better default
    LB(lh)->ib_MaxNameLength        = 25;
    LB(lh)->ib_NewIconsSupport      = TRUE;
    LB(lh)->ib_ColorIconSupport     = TRUE;
    
    return TRUE;
}

AROS_SET_LIBFUNC(Expunge, LIBBASETYPE, lh)
{
    DeletePool(LB(lh)->ib_MemoryPool);
    
    if (PNGBase) CloseLibrary(PNGBase);
}


ADD2INITLIB(Init, 0);
ADD2EXPUNGELIB(Expunge, 0);
