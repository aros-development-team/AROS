/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/types.h>
#include <exec/libraries.h>
#include <dos/dosextens.h>
#include <aros/libcall.h>
#include <aros/debug.h>
#include <libcore/base.h>

#include <proto/exec.h>
#include <proto/locale.h>

#include <aros/symbolsets.h>
#include LC_LIBDEFS_FILE


/*** Variables **************************************************************/
struct LocaleBase  *LocaleBase;

/*** Library startup and shutdown *******************************************/
AROS_SET_LIBFUNC( PreferencesWindow_Startup, LIBBASETYPE, LIBBASE )
{
    SysBase = LIBBASE->lh_SysBase;
    
    LocaleBase = (struct LocaleBase *) OpenLibrary( "locale.library", 0 );
    if( LocaleBase == NULL )
        return FALSE;
    else
        return TRUE;
}

AROS_SET_LIBFUNC( PreferencesWindow_Shutdown, LIBBASETYPE, LIBBASE )
{
    if( LocaleBase != NULL ) CloseLibrary( (struct Library *) LocaleBase );
}

ADD2INITLIB( PreferencesWindow_Startup, 1 );
ADD2EXPUNGELIB( PreferencesWindow_Shutdown, 1 );
