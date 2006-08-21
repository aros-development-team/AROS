/*
    Copyright © 1995-2006, The AROS Development Team. All rights reserved.
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

static int Init(LIBBASETYPEPTR LIBBASE)
{
    int i;

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
}


static int Open(LIBBASETYPEPTR LIBBASE)
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


static int Expunge(LIBBASETYPEPTR LIBBASE)
{
    /* stegerg: if later someone else re-opens datatypes.library, then
                the datatypes.class would have to be re-added with
		AddClass in libopen() (if FreeClass returned FALSE,
		where the class was not freed, but still removed),
		or re-make the class (when FreeClass returned TRUE) */
		
    return TryRemoveClass((struct Library *)DataTypesBase);
}

ADD2INITLIB(Init, 0);
ADD2OPENLIB(Open, 0);
ADD2EXPUNGELIB(Expunge, 0);
