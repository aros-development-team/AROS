/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.5  1996/10/24 15:50:55  aros
    Use the official AROS macros over the __AROS versions.

    Revision 1.4  1996/08/13 13:56:06  digulla
    Replaced AROS_LA by AROS_LHA
    Replaced some AROS_LH*I by AROS_LH*
    Sorted and added includes

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
	AddDevice(), OpenDevice(), CloseDevice().

    INTERNALS

    HISTORY

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    /* Arbitrate for the device list */
    Forbid();

    /* Call expunge vector */
    (void)AROS_LVO_CALL0(BPTR,struct Device *,device,3,);
    /*
	Normally you'd expect the device to be expunged if this returns
	non-zero, but this is only exec which doesn't know anything about
	seglists - therefore dos.library has to SetFunction() into this
	vector for the additional functionality.
    */

    /* All done. */
    Permit();
    AROS_LIBFUNC_EXIT
} /* RemDevice */

