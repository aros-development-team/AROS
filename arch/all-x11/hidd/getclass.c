/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: GetClass() - Get HIDD class.
    Lang: english
*/

#include <exec/types.h>
#include <aros/libcall.h>
#include <oop/oop.h>
#include <libdefs.h>
#include "x11gfx_intern.h"

#define DEBUG 1
#include <aros/debug.h>

#define SysBase (LIBBASE->sysbase)

/*****************************************************************************/

/*  NAME */ 
#include <proto/exec.h>

	AROS_LH0(Class *, GetClass,

/*  LOCATION */
	LIBBASETYPEPTR, LIBBASE, 5, BASENAME)

/*  FUNCTION
	Get pointer to HIDD class form library base. This allows for HIDD
	classes to be private.
    INPUTS
	None.

    RESULT
	Pointer to private HIDD class.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

******************************************************************************/
{
    AROS_LIBFUNC_INIT
    
    EnterFunc(bug("X11Gfx_GetClass(libbase=%p)\n", LIBBASE));

    ReturnPtr("X11Gfx_GetClass", Class *, LIBBASE->gfxclass);
    
    AROS_LIBFUNC_EXIT
} /* GetClass() */
