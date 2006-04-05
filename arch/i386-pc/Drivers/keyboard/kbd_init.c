/*
    Copyright © 1995-2006, The AROS Development Team. All rights reserved.
    $Id$

    Desc: kbd Hidd for standalone i386 AROS
    Lang: english
*/

#include <exec/types.h>
#include <proto/exec.h>
#include <utility/utility.h>
#include <aros/symbolsets.h>

#include "kbd.h"

#include LC_LIBDEFS_FILE

#undef  SDEBUG
#undef  DEBUG
#define DEBUG 0 
#include <aros/debug.h>

AROS_SET_LIBFUNC(PCKbd_Init, LIBBASETYPE, LIBBASE)
{
    AROS_SET_LIBFUNC_INIT

    struct kbd_staticdata *xsd = &LIBBASE->ksd;
	
    InitSemaphore( &xsd->sema );
	
    return TRUE;
    
    AROS_SET_LIBFUNC_EXIT
}
