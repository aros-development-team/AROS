/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.3  1996/08/01 17:41:16  digulla
    Added standard header for all files

    Desc:
    Lang: english
*/
#include <exec/devices.h>
#include <exec/execbase.h>
#include <aros/libcall.h>
#include <dos/dos.h>

/*****************************************************************************

    NAME */
	#include <clib/exec_protos.h>

	__AROS_LH1(void, RemDevice,

/*  SYNOPSIS */
	__AROS_LA(struct Device *, device,A1),

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
	AddDevice(), OpenDevice(), CloseDevice().

    INTERNALS

    HISTORY

******************************************************************************/
{
    __AROS_FUNC_INIT

    /* Arbitrate for the device list */
    Forbid();

    /* Call expunge vector */
    (void)__AROS_LVO_CALL0(BPTR,3,device);
    /*
	Normally you'd expect the device to be expunged if this returns
	non-zero, but this is only exec which doesn't know anything about
	seglists - therefore dos.library has to SetFunction() into this
	vector for the additional functionality.
    */

    /* All done. */
    Permit();
    __AROS_FUNC_EXIT
} /* RemDevice */

