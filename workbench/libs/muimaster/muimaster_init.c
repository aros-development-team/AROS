/*
    Copyright © 2002, The AROS Development Team. 
    All rights reserved.
    
    $Id$
*/

#include <exec/types.h>
#include <exec/libraries.h>
#include <aros/libcall.h>

#include <proto/exec.h>

#include "muimaster_intern.h"
#include "mui.h"
#include "prefs.h"
#include "libdefs.h"

/****************************************************************************************/

#undef SysBase

/* Customize libheader.c */
#define LC_SYSBASE_FIELD(lib)   (MUIMB(lib)->sysbase)
#define LC_SEGLIST_FIELD(lib)   (MUIMB(lib)->seglist)
#define LC_LIBBASESIZE		sizeof(struct MUIMasterBase_intern)
#define LC_LIBHEADERTYPEPTR	LIBBASETYPEPTR
#define LC_LIB_FIELD(lib)	(MUIMB(lib)->library)

/* #define LC_NO_INITLIB    */
/* #define LC_NO_OPENLIB    */
/* #define LC_NO_CLOSELIB   */
/* #define LC_NO_EXPUNGELIB */

#include <libcore/libheader.c>

#undef DEBUG
#define DEBUG 1
#include <aros/debug.h>

/* Global libbase vars */
#undef IntuitionBase

struct ExecBase      *SysBase;
struct Library       *MUIMasterBase;
struct IntuitionBase *IntuitionBase;
struct Library	     *DataTypesBase;

struct ExecBase **SysBasePtr = &SysBase;
struct Library  **MUIMasterBasePtr = &MUIMasterBase;

#define SysBase			(LC_SYSBASE_FIELD(MUIMasterBase))

/****************************************************************************************/

ULONG SAVEDS STDARGS LC_BUILDNAME(L_InitLib) (LC_LIBHEADERTYPEPTR MUIMasterBase)
{
    D(bug("Inside Init func of muimaster.library\n"));

    *SysBasePtr = SysBase;
    *MUIMasterBasePtr = MUIMasterBase;

    if (!DOSBase)
        (struct Library *)DOSBase = OpenLibrary("dos.library", 37);
    if (!DOSBase)
        return FALSE;

    if (!UtilityBase)
        (struct Library *)UtilityBase = OpenLibrary("utility.library", 37);
    if (!UtilityBase)
        return FALSE;

    if (!AslBase)
    	AslBase = OpenLibrary("asl.library", 37);
    if (!AslBase)
    	return FALSE;

    if (!GfxBase)
    	(struct Library *)GfxBase = OpenLibrary("graphics.library", 37);
    if (!GfxBase)
    	return FALSE;

    if (!LayersBase)
    	LayersBase = OpenLibrary("layers.library", 37);
    if (!LayersBase)
    	return FALSE;

    if (!IntuitionBase)
    	(struct Library *)IntuitionBase = OpenLibrary("intuition.library", 37);
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
    
    MUIMB(MUIMasterBase)->intuibase = IntuitionBase;

    InitSemaphore(&MUIMB(MUIMasterBase)->ZuneSemaphore);

    __zune_prefs_init(&__zprefs);

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

    CloseLibrary((struct Library *)DOSBase);
    DOSBase = NULL;

    CloseLibrary((struct Library *)UtilityBase);
    UtilityBase = NULL;

    CloseLibrary(AslBase);
    AslBase = NULL;

    CloseLibrary((struct Library *)GfxBase);
    GfxBase = NULL;

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
}

/****************************************************************************************/
