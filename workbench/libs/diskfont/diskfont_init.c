/*
    Copyright � 1995-2003, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Diskfont initialization code.
    Lang: English.
*/

#include <exec/types.h>
#include <exec/memory.h>
#include <graphics/text.h>
#include <diskfont/diskfont.h>
#include <aros/symbolsets.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/graphics.h>

#include LC_LIBDEFS_FILE
#include "diskfont_intern.h"

/****************************************************************************************/

#include <aros/debug.h>

/****************************************************************************************/

struct DosLibrary *DOSBase;

AROS_UFH3(int, CleanMem,
    AROS_UFHA(struct MemHandlerData *, mhdata, A0),
    AROS_UFHA(LIBBASETYPEPTR, LIBBASE, A1),
    AROS_UFHA(struct ExecBase *, SysBase, A6)
);

/****************************************************************************************/

AROS_SET_LIBFUNC(Init, LIBBASETYPE, LIBBASE)
{
    /* This function is single-threaded by exec by calling Forbid. */
    D(bug("Inside initfunc\n"));

    NEWLIST(&LIBBASE->diskfontlist);
    NEWLIST(&LIBBASE->fontsdirentrylist);
    InitSemaphore(&LIBBASE->fontssemaphore);

    DOSBase = (struct DosLibrary *)OpenLibrary("dos.library", 37);
    if (!DOSBase)
	return FALSE;

    GfxBase = (GraphicsBase *)OpenLibrary("graphics.library", 37);
    if (!GfxBase)
	return FALSE;

    UtilityBase = OpenLibrary("utility.library", 37);
    if (!UtilityBase)
	return FALSE;

    /* Insert the fonthooks into the DiskfontBase */

    LIBBASE->dsh.h_Entry = (void *)AROS_ASMSYMNAME(dosstreamhook);
    LIBBASE->dsh.h_Data = DOSBase;

    LIBBASE->memint.is_Data = (APTR)LIBBASE;
    LIBBASE->memint.is_Code = CleanMem;
    LIBBASE->memint.is_Node.ln_Pri = 1; /* Just above RamLib */
    AddMemHandler(&LIBBASE->memint);
    
    D(bug("diskfont.library initialized succesfully\n"));

    return TRUE;
}

/****************************************************************************************/


AROS_SET_LIBFUNC(Expunge, LIBBASETYPE, LIBBASE)
{
    /*
	This function is single-threaded by exec by calling Forbid.
	Never break the Forbid() or strange things might happen.
    */
    D(bug("Inside expungefunc\n"));

    RemMemHandler(&LIBBASE->memint);
    
    CleanUpFontsDirEntryList(LIBBASE);
    
    if (UtilityBase)
	CloseLibrary(UtilityBase);
    UtilityBase = NULL;

    if (GfxBase)
	CloseLibrary((struct Library *)GfxBase);
    GfxBase = NULL;

    if (DOSBase)
	CloseLibrary((struct Library *)DOSBase);
    DOSBase = NULL;

    D(bug("diskfont.library expunged\n"));

    return TRUE;
}

/****************************************************************************************/

AROS_UFH3(int, CleanMem,
    AROS_UFHA(struct MemHandlerData *, mhdata, A0),
    AROS_UFHA(LIBBASETYPEPTR, LIBBASE, A1),
    AROS_UFHA(struct ExecBase *, SysBase, A6)
)
{
    struct DiskFontHeader *dfh, *dfh2;

    D(bug("Inside CleanMem\n"));
    
    ForeachNodeSafe(&LIBBASE->diskfontlist, dfh, dfh2)
    {
	if (dfh->dfh_TF.tf_Accessors < 1)
	{
	    /* Possible paranoia check */
	    if (!(dfh->dfh_TF.tf_Flags & FPF_REMOVED))
	    {
		/* Unlink from GfxBase->TextFonts */
		REMOVE(&dfh->dfh_TF.tf_Message.mn_Node);

		StripFont(&dfh->dfh_TF);

		/* Unlink from DiskfontBase->diskfontlist */

		REMOVE(&dfh->dfh_DF);

		UnLoadSeg(dfh->dfh_Segment);
	    }
	}
    }
    
    D(bug("CleanMem Finished\n"));
    
    return MEM_ALL_DONE;
}

/****************************************************************************************/

ADD2INITLIB(Init, 0);
ADD2EXPUNGELIB(Expunge, 0);
