/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/types.h>
#include <exec/lists.h>
#include <exec/memory.h>
#include <exec/resident.h>
#include <exec/semaphores.h>
#include <aros/symbolsets.h>

#include <proto/exec.h>

#define DEBUG 0
#include <aros/debug.h>

#include "datatypes_intern.h"
#include LC_LIBDEFS_FILE

static void closelibs(struct DataTypesBase *DataTypesBase)
{
    if(DataTypesBase->dtb_IntuitionBase != NULL)
	CloseLibrary(DataTypesBase->dtb_IntuitionBase);

    if(DataTypesBase->dtb_LayersBase != NULL)
	CloseLibrary(DataTypesBase->dtb_LayersBase);

    if(DataTypesBase->dtb_UtilityBase != NULL)
	CloseLibrary(DataTypesBase->dtb_UtilityBase);

    if(DataTypesBase->dtb_DOSBase != NULL)
	CloseLibrary(DataTypesBase->dtb_DOSBase);

    if(DataTypesBase->dtb_IFFParseBase != NULL)
	CloseLibrary(DataTypesBase->dtb_IFFParseBase);

    if(DataTypesBase->dtb_LocaleBase != NULL)
	CloseLibrary(DataTypesBase->dtb_LocaleBase);

    if(DataTypesBase->dtb_GfxBase != NULL)
	CloseLibrary(DataTypesBase->dtb_GfxBase);
}


AROS_SET_LIBFUNC(Init, LIBBASETYPE, LIBBASE)
{
    int i;

    LIBBASE->dtb_SysBase = SysBase;
    
    for (i = 0; i < SEM_MAX; i++)
    {
	InitSemaphore(&LIBBASE->dtb_Semaphores[i]);
    }

    /*
     * Open libraries. These should all exist at init time, so there is no
     * point not opening them now. If they happen to fail, we do not load
     * the library. Under debugging mode print out why though. I don't
     * alert because a) datatypes isn't that important, and b) it may be a
     * version problem. In either case I don't want to bring down the
     * system unnecessarily.
     */

    D(bug("Inside init of datatypes.library\n"));
    
    if ((LIBBASE->dtb_UtilityBase =
	    OpenLibrary("utility.library", 39L)) == NULL)
    {
	D(bug("datatypes.library: Cannot open utility.library\n"));
	goto error;
    }

    if ((LIBBASE->dtb_GfxBase =
	    OpenLibrary("graphics.library", 39L)) == NULL)
    {
	D(bug("datatypes.library: Cannot open graphics.library\n"));
	goto error;
    }

    if ((LIBBASE->dtb_LayersBase = 
	    OpenLibrary("layers.library", 39L)) == NULL)
    {
	D(bug("datatypes.library: Cannot open layers.library\n"));
	goto error;
    }

    if ((LIBBASE->dtb_IntuitionBase =
	    OpenLibrary("intuition.library", 39L)) == NULL)
    {
	D(bug("datatypes.library: Cannot open intuition.library\n"));
	goto error;
    }

    if ((LIBBASE->dtb_DOSBase =
	    OpenLibrary("dos.library", 37L)) == NULL)
    {
	D(bug("datatypes.library: Cannot open dos.library\n"));
	goto error;
    }

    /* We may not have these libraries, but try anyway */
    if ((LIBBASE->dtb_IFFParseBase =
	    OpenLibrary("iffparse.library", 37L)) == NULL)
    {
	D(bug("datatypes.library: Cannot open iffparse.library\n"));
	goto error;
    }

    if ((LIBBASE->dtb_LocaleBase =
	    OpenLibrary("locale.library", 0L)) == NULL)
    {
	D(bug("datatypes.library: Cannot open locale.library\n"));
	goto error;
    }

    /* Get the list of datatypes */
    LIBBASE->dtb_DTList = GetDataTypesList(LIBBASE);

    if(!InstallClass((struct Library *)LIBBASE))
	return FALSE;

    /* Try opening the catalog, don't worry if we fail, just keep trying. */
    LIBBASE->dtb_LibsCatalog =
	opencatalog
	(
	    (struct Library *)LIBBASE,
	    NULL,
	    "Sys/libs.catalog",
	    OC_BuiltInLanguage,
	    "english",
	    TAG_DONE
	);

    D(bug("datatypes.library correctly initialized\n"));

    return TRUE;

error:
    closelibs(LIBBASE);

    return FALSE;
}


AROS_SET_LIBFUNC(Open, LIBBASETYPE, LIBBASE)
{
    D(bug("Inside open of datatypes.library\n"));

    /* Try opening the catalog again. */
    if(DataTypesBase->dtb_LibsCatalog == NULL)
    {
	DataTypesBase->dtb_LibsCatalog =
	    opencatalog
	    (
		(struct Library *)DataTypesBase,
		NULL,
		"Sys/libs.catalog",
		OC_BuiltInLanguage,
		"english",
		TAG_DONE
	    );
    }

    D(bug("Return from open of datatypes.library\n"));

    return TRUE;
}


AROS_SET_LIBFUNC(Expunge, LIBBASETYPE, LIBBASE)
{
#if 0
    /* stegerg: if later someone else re-opens datatypes.library, then
                the datatypes.class would have to be re-added with
		AddClass in libopen() (if FreeClass returned FALSE,
		where the class was not freed, but still removed),
		or re-make the class (when FreeClass returned TRUE) */
		
    TryRemoveClass((struct Library *)DataTypesBase);
#endif

    return 0;
}

ADD2INITLIB(Init, 0);
ADD2OPENLIB(Open, 0);
ADD2EXPUNGELIB(Expunge, 0);
