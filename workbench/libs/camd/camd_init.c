/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Camd initialization code.
    Lang: English
*/

#include <exec/types.h>
#include <exec/libraries.h>

#include <aros/symbolsets.h>

#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/dos.h>

#include "camd_intern.h"
#include LC_LIBDEFS_FILE

#define DEBUG 1
#include <aros/debug.h>

/****************************************************************************************/

AROS_SET_LIBFUNC(Init, LIBBASETYPE, LIBBASE)
{
    D(bug("Inside Init func of camd.library\n"));

    return InitCamd((struct CamdBase *)LIBBASE);
}



/****************************************************************************************/

AROS_SET_LIBFUNC(Expunge, LIBBASETYPE, LIBBASE)
{
    UninitCamd(LIBBASE);
    
    return TRUE;
}

ADD2INITLIB(Init, 0);
ADD2EXPUNGELIB(Expunge, 0);
