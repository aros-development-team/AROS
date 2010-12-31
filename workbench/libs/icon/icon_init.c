/*
    Copyright © 1995-2006, The AROS Development Team. All rights reserved.
    $Id$

*/

#include <utility/utility.h> /* this must be before icon_intern.h */

#include <aros/symbolsets.h>

#define __ICON_NOLIBBASE__

#include "icon_intern.h"
#include "identify.h"

#include LC_LIBDEFS_FILE

const LONG IFFParseBase_version = 39,
           GfxBase_version      = 39,
           CyberGfxBase_version = 39;

/****************************************************************************************/

static int GM_UNIQUENAME(Init)(LIBBASETYPEPTR lh)
{
    LONG i;

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
    
    /* Setup default global settings ---------------------------------------*/
    LB(lh)->ib_Screen               = NULL; // FIXME: better default
    LB(lh)->ib_Precision            = PRECISION_ICON;
    LB(lh)->ib_EmbossRectangle.MinX = 0; // FIXME: better default
    LB(lh)->ib_EmbossRectangle.MaxX = 0; 
    LB(lh)->ib_EmbossRectangle.MinY = 0; 
    LB(lh)->ib_EmbossRectangle.MaxY = 0; 
    LB(lh)->ib_Frameless            = TRUE;
    LB(lh)->ib_IdentifyHook         = NULL;
    LB(lh)->ib_MaxNameLength        = 25;
    LB(lh)->ib_NewIconsSupport      = TRUE;
    LB(lh)->ib_ColorIconSupport     = TRUE;
    
    return TRUE;
}

static int GM_UNIQUENAME(Open)(LIBBASETYPEPTR LIBBASE)
{
    UtilityBase = OpenLibrary("utility.library", 0);
    if (UtilityBase != NULL) {
    	DOSBase = OpenLibrary("dos.library", 0);
    	if (DOSBase != NULL) {
    	    GfxBase = OpenLibrary("graphics.library", GfxBase_version);
    	    if (GfxBase != NULL) {
    	    	IntuitionBase = OpenLibrary("intuition.library", 0);
    	    	if (IntuitionBase != NULL) {
		    /* Optional libraries */
		    CyberGfxBase = (APTR)OpenLibrary("cybergraphics.library", CyberGfxBase_version);
		    IFFParseBase = OpenLibrary("iffparse.library", IFFParseBase_version);
		    DataTypesBase = OpenLibrary("datatypes.library", 0);


    	    	    return TRUE;
    	    	}
    	    	CloseLibrary(GfxBase);
    	    }
    	    CloseLibrary(DOSBase);
    	}
        CloseLibrary(UtilityBase);
    }

    return FALSE;
}

static int GM_UNIQUENAME(Expunge)(LIBBASETYPEPTR LIBBASE)
{
    DeletePool(LB(LIBBASE)->ib_MemoryPool);
   
    /* Drop dynamic libraries */
    if (PNGBase) CloseLibrary(PNGBase);

    /* Drop optional libraries */
    if (CyberGfxBase)  CloseLibrary(CyberGfxBase);
    if (DataTypesBase) CloseLibrary(DataTypesBase);
    if (IFFParseBase)  CloseLibrary(IFFParseBase);

    /* Drop the rest */
    if (IntuitionBase) CloseLibrary(IntuitionBase);
    if (GfxBase)       CloseLibrary(GfxBase);
    if (DOSBase)       CloseLibrary(DOSBase);
    if (IntuitionBase) CloseLibrary(IntuitionBase);
    
    return TRUE;
}


ADD2INITLIB(GM_UNIQUENAME(Init), 0);
ADD2OPENLIB(GM_UNIQUENAME(Open), 0);
ADD2EXPUNGELIB(GM_UNIQUENAME(Expunge), 0);
