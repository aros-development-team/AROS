/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Initialisation for the locale.library.
    Lang: english
*/

#define DEBUG 1
#include <aros/debug.h>

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
struct LocaleBase *globallocalebase = NULL;

AROS_MAKE_ALIAS(globallocalebase, LocaleBase);

static int Init(LIBBASETYPEPTR LIBBASE)
{
    /* A few internal functions need to access a global LocaleBase pointer,
       because they're used to patch dos.library functions, and thus don't
       directly get a LocaleBase pointer. Someday, with TLS, this will go away. */
    globallocalebase = (struct LocaleBase *)LIBBASE;

    /* perform static initialisation */
    InitSemaphore(&((struct IntLocaleBase *)LIBBASE)->lb_LocaleLock);
    InitSemaphore(&((struct IntLocaleBase *)LIBBASE)->lb_CatalogLock);

    NEWLIST(&((struct IntLocaleBase *)LIBBASE)->lb_CatalogList);

    if ((IntLB(LIBBASE)->lb_DefaultLocale = AllocMem(sizeof(struct IntLocale), MEMF_CLEAR | MEMF_ANY)) != NULL)
    {
        /* Copy the defaults to our new structure */
        CopyMem(&defLocale, IntLB(LIBBASE)->lb_DefaultLocale, sizeof(struct Locale));

        /* Set lb_CurrentLocale *BEFORE* SetLocaleLanguage */
        IntLB(LIBBASE)->lb_CurrentLocale = IntLB(LIBBASE)->lb_DefaultLocale;

        /* Setup the languages - will not fail here. */
        SetLocaleLanguage(IntLB(LIBBASE)->lb_DefaultLocale, (struct LocaleBase *)LIBBASE);

        IntLB(LIBBASE)->lb_DefaultLocale->il_Count = 0;
        InstallPatches();

        return TRUE;
    }

    return FALSE;
}

ADD2INITLIB(Init, 0);
