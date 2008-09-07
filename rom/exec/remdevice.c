/*
    Copyright � 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Remove a device from the public list of devices.
    Lang: english
*/
#include <aros/config.h>
#include <exec/devices.h>
#include <exec/execbase.h>
#include <aros/libcall.h>
#include <dos/dos.h>
#include <proto/exec.h>

#include "exec_debug.h"
#ifndef DEBUG_RemDevice
#   define DEBUG_RemDevice 0
#endif
#undef DEBUG
#if DEBUG_RemDevice
#   define DEBUG 1
#endif
#include <aros/debug.h>
#undef kprintf

/*****************************************************************************

    NAME */

	AROS_LH1(void, RemDevice,

/*  SYNOPSIS */
	AROS_LHA(struct Device *, device,A1),

/*  LOCATION */
	struct ExecBase *, SysBase, 73, Exec)

/*  FUNCTION
	Calls the given device's expunge vector, thus trying to delete it.
	The device may refuse to do so and still be open after this call.

    INPUTS
	device - Pointer to the device structure.

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	AddDevice(), OpenDevice(), CloseDevice()

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT
    BPTR seglist;

    D(bug("RemDevice $%lx (\"%s\") by \"%s\"\n", device, 
	device ? device->dd_Library.lib_Node.ln_Name : "(null)",
	SysBase->ThisTask->tc_Node.ln_Name));

    /* Arbitrate for the device list */
    Forbid();

    /* Call expunge vector */
    seglist = AROS_LVO_CALL1(BPTR,
        AROS_LCA(struct Device *,device, D0),
        struct Device *,device,3,
    );
    /*
	Normally you'd expect the device to be expunged if this returns
	non-zero, but this is only exec which doesn't know anything about
	seglists - therefore dos.library has to SetFunction() into this
	vector for the additional functionality.
    */

    /* All done. */
    Permit();

    AROS_COMPAT_SETD0(seglist); /* For m68k compatibility */
    AROS_LIBFUNC_EXIT
} /* RemDevice */

