/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Init of workbench.library
    Lang: english
*/

#include <libraries/locale.h>
#include <aros/symbolsets.h>

#include "lowlevel_intern.h"
#include LC_LIBDEFS_FILE

struct LocaleBase * LocaleBase;

AROS_SET_LIBFUNC(Init, LIBBASETYPE, lh)
{
    LocaleBase = (struct LocaleBase *)OpenLibrary("locale.library",39);
    if (LocaleBase == NULL)
    {
        return FALSE;
    }

    InitSemaphore(&lh->ll_Lock);
    lh->ll_VBlank.is_Data = NULL;
    lh->ll_VBlank.is_Code = NULL;

    return TRUE;
} /* L_InitLib */


AROS_SET_LIBFUNC(Expunge, LIBBASETYPE, LIBBASE)
{
    if (LocaleBase != NULL) CloseLibrary((struct Library *) LocaleBase);
}

ADD2INITLIB(Init, 0);
ADD2EXPUNGELIB(Expunge, 0);
