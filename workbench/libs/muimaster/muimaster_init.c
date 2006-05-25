/*
    Copyright © 2002, The AROS Development Team. 
    All rights reserved.
    
    $Id$
*/

#include <exec/types.h>
#include <exec/libraries.h>

#include <proto/exec.h>
#include <clib/alib_protos.h>

#include "muimaster_intern.h"
#include "mui.h"

/****************************************************************************************/

/* #define MYDEBUG 1 */
#include "debug.h"

/* Global libbase vars */
#undef IntuitionBase

/* Undef the following bases because the casts struct Libaray * -> struct <Lib>Base * casts 
 * would use them */
#undef UtilityBase
#undef DOSBase
#undef GfxBase

struct Library       *MUIMasterBase;
struct IntuitionBase *IntuitionBase;
struct Library	     *DataTypesBase;

struct Library  **MUIMasterBasePtr = &MUIMasterBase;

/****************************************************************************************/

ULONG SAVEDS STDARGS LC_BUILDNAME(L_InitLib) (LC_LIBHEADERTYPEPTR MUIMasterBase)
{
    D(bug("Inside Init func of muimaster.library\n"));

    *MUIMasterBasePtr = MUIMasterBase;

    if (!MUIMB(MUIMasterBase)->dosbase)
    {
	if (!(MUIMB(MUIMasterBase)->dosbase = (struct DosLibrary*)OpenLibrary("dos.library", 37)))
	    return FALSE;
    }

    if (!MUIMB(MUIMasterBase)->utilitybase)
    {
        if (!(MUIMB(MUIMasterBase)->utilitybase = (struct UtilityBase*)OpenLibrary("utility.library", 37)))
	    return FALSE;
    }

    if (!MUIMB(MUIMasterBase)->gfxbase)
    {
        if (!(MUIMB(MUIMasterBase)->gfxbase = (struct GfxBase*)OpenLibrary("graphics.library", 39)))
	    return FALSE;
    }

    if (!AslBase)
    	AslBase = OpenLibrary("asl.library", 37);
    if (!AslBase)
    	return FALSE;

    if (!LayersBase)
    	LayersBase = OpenLibrary("layers.library", 37);
    if (!LayersBase)
    	return FALSE;

    if (!IntuitionBase)
    	IntuitionBase = (struct IntuitionBase*)OpenLibrary("intuition.library", 39);
    if (!IntuitionBase)
    	return FALSE;

    if (!CxBase)
    	CxBase = OpenLibrary("commodities.library", 37);
    if (!CxBase)
    	return FALSE;

    if (!GadToolsBase)
    	GadToolsBase = OpenLibrary("gadtools.library", 37);
    if (!GadToolsBase)
    	return FALSE;

    if (!KeymapBase)
    	KeymapBase = OpenLibrary("keymap.library", 37);
    if (!KeymapBase)
    	return FALSE;

    if (!DataTypesBase)
    	DataTypesBase = OpenLibrary("datatypes.library", 37);
    if (!DataTypesBase)
    	return FALSE;

    if (!IFFParseBase)
    	IFFParseBase = OpenLibrary("iffparse.library", 37);
    if (!IFFParseBase)
    	return FALSE;

    if (!DiskfontBase)
    	DiskfontBase = OpenLibrary("diskfont.library", 37);
    if (!DiskfontBase)
    	return FALSE;

    if (!IconBase)
    	IconBase = OpenLibrary("icon.library", 37); /* V44 really */
    if (!IconBase)
    	return FALSE;

    if (!CyberGfxBase)
    	CyberGfxBase = OpenLibrary("cybergraphics.library", 0);
    /* continue even if cybergraphics.library is not available */

#ifdef HAVE_COOLIMAGES
    if (!CoolImagesBase)
    	CoolImagesBase = OpenLibrary("coolimages.library", 0);
#endif
    
    MUIMB(MUIMasterBase)->intuibase = IntuitionBase;

    InitSemaphore(&MUIMB(MUIMasterBase)->ZuneSemaphore);
    
    NewList((struct List *)&MUIMB(MUIMasterBase)->BuiltinClasses);
    NewList((struct List *)&MUIMB(MUIMasterBase)->Applications);
    return TRUE;
}

/****************************************************************************************/

ULONG SAVEDS STDARGS LC_BUILDNAME(L_OpenLib) (LC_LIBHEADERTYPEPTR MUIMasterBase)
{
    D(bug("Inside Open func of muimaster.library\n"));

    return TRUE;
}

/****************************************************************************************/

void  SAVEDS STDARGS LC_BUILDNAME(L_CloseLib) (LC_LIBHEADERTYPEPTR MUIMasterBase)
{
    D(bug("Inside Close func of muimaster.library\n"));
}

/****************************************************************************************/

void  SAVEDS STDARGS LC_BUILDNAME(L_ExpungeLib) (LC_LIBHEADERTYPEPTR MUIMasterBase)
{
    D(bug("Inside Expunge func of muimaster.library\n"));

    /* CloseLibrary() checks for NULL-pointers */

    CloseLibrary((struct Library *)MUIMB(MUIMasterBase)->gfxbase);
    MUIMB(MUIMasterBase)->gfxbase = NULL;

    CloseLibrary((struct Library *)MUIMB(MUIMasterBase)->utilitybase);
    MUIMB(MUIMasterBase)->utilitybase = NULL;

    CloseLibrary(AslBase);
    CloseLibrary((struct Library *)MUIMB(MUIMasterBase)->dosbase);
    MUIMB(MUIMasterBase)->dosbase = NULL;

    AslBase = NULL;

    CloseLibrary(LayersBase);
    LayersBase = NULL;

    CloseLibrary((struct Library *)MUIMB(MUIMasterBase)->intuibase);
    MUIMB(MUIMasterBase)->intuibase = IntuitionBase = NULL;

    CloseLibrary(CxBase);
    CxBase = NULL;

    CloseLibrary(GadToolsBase);
    GadToolsBase = NULL;

    CloseLibrary(KeymapBase);
    KeymapBase = NULL;
    
    CloseLibrary(DataTypesBase);
    DataTypesBase = NULL;
    
    CloseLibrary(IFFParseBase);
    IFFParseBase = NULL;
    
    CloseLibrary(DiskfontBase);
    DiskfontBase = NULL;
    
    CloseLibrary(IconBase);
    IconBase = NULL;
    
    CloseLibrary(CyberGfxBase);
    CyberGfxBase = NULL;

#ifdef HAVE_COOLIMAGES
    CloseLibrary(CoolImagesBase);
    CoolImagesBase = NULL;
#endif
}

/****************************************************************************************/
