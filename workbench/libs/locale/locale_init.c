/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Initialisation for the locale.library.
    Lang: english
*/

#define AROS_ALMOST_COMPATIBLE
#include <exec/types.h>
#include <exec/lists.h>
#include <exec/memory.h>
#include <exec/resident.h>
#include <aros/symbolsets.h>
#include <exec/semaphores.h>

#include <proto/exec.h>

#include "locale_intern.h"
#include LC_LIBDEFS_FILE

/* This global variable is needed for LocRawDoFmt */

struct LocaleBase *globallocalebase;

AROS_SET_LIBFUNC(Init, LIBBASETYPE, LIBBASE)
{
    AROS_SET_LIBFUNC_INIT;
    
    struct IntLocale *def;

    globallocalebase = LIBBASE;
    
    /* Do whatever static initialisation you need here */
    InitSemaphore(&((struct IntLocaleBase *)LIBBASE)->lb_LocaleLock);
    InitSemaphore(&((struct IntLocaleBase *)LIBBASE)->lb_CatalogLock);

    NEWLIST(&((struct IntLocaleBase *)LIBBASE)->lb_CatalogList);

    IntLB(LIBBASE)->lb_DefaultLocale = def = AllocMem(sizeof(struct IntLocale), MEMF_CLEAR|MEMF_ANY);
    if(def != NULL)
    {
	/* Copy the defaults to our new structure */
	CopyMem(&defLocale, def, sizeof(struct Locale));

	/* Set lb_CurrentLocale *BEFORE* SetLocaleLanguage */
	IntLB(LIBBASE)->lb_CurrentLocale = def;

	/* Setup the languages - will not fail here. */
	SetLocaleLanguage(def, LIBBASE);

	def->il_Count = 0;
   	InstallPatches();
	
#warning FIXME: More elegant solution in libcore needed ?
	/* Lock locale.library in memory by keeping opencount always above zero */
	LIBBASE->lb_LocaleBase.lb_LibNode.lib_OpenCnt++;

	return TRUE;
    }

    return FALSE;
    
    AROS_SET_LIBFUNC_EXIT;
}


ADD2INITLIB(Init, 0);
