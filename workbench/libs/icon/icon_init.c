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
           CyberGfxBase_version = 41;

/****************************************************************************************/

static int GM_UNIQUENAME(Init)(LIBBASETYPEPTR lh)
{
    LONG i;
    struct IconBase *IconBase;

    LB(lh)->dsh.h_Entry = (void *)AROS_ASMSYMNAME(dosstreamhook);
    LB(lh)->dsh.h_Data  = lh;

    InitSemaphore(&LB(lh)->iconlistlock);
    for(i = 0; i < ICONLIST_HASHSIZE; i++)
    {
    	NewList((struct List *)&LB(lh)->iconlists[i]);
    }
    
    /* Setup default global settings ---------------------------------------*/
    LB(lh)->ib_Screen               = NULL;
    LB(lh)->ib_Precision            = PRECISION_ICON;
    LB(lh)->ib_EmbossRectangle.MinX = -4;
    LB(lh)->ib_EmbossRectangle.MaxX = 4;
    LB(lh)->ib_EmbossRectangle.MinY = -4;
    LB(lh)->ib_EmbossRectangle.MaxY = 4; 
    LB(lh)->ib_Frameless            = FALSE;
    LB(lh)->ib_IdentifyHook         = NULL;
    LB(lh)->ib_MaxNameLength        = 25;
    LB(lh)->ib_NewIconsSupport      = TRUE;
    LB(lh)->ib_ColorIconSupport     = TRUE;

    IconBase = LB(lh);
    
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
    /* Drop optional libraries */
    if (CyberGfxBase)  CloseLibrary(CyberGfxBase);
    if (DataTypesBase) CloseLibrary(DataTypesBase);
    if (IFFParseBase)  CloseLibrary(IFFParseBase);

    /* Drop the rest */
    if (WorkbenchBase) CloseLibrary(WorkbenchBase);
    if (IntuitionBase) CloseLibrary(IntuitionBase);
    if (GfxBase)       CloseLibrary(GfxBase);
    if (DOSBase)       CloseLibrary(DOSBase);
    if (IntuitionBase) CloseLibrary(IntuitionBase);
    
    return TRUE;
}

ADD2INITLIB(GM_UNIQUENAME(Init), 0);
ADD2EXPUNGELIB(GM_UNIQUENAME(Expunge), 0);
