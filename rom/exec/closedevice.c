/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc:
    Lang: english
*/
#include <aros/config.h>
#include <exec/execbase.h>
#include <exec/io.h>
#include <dos/dos.h>
#include <aros/libcall.h>
#include <proto/exec.h>

/*****************************************************************************

    NAME */

	AROS_LH1(void, CloseDevice,

/*  SYNOPSIS */
	AROS_LHA(struct IORequest *, iORequest, A1),

/*  LOCATION */
	struct ExecBase *, SysBase, 75, Exec)

/*  FUNCTION
	Closes a previously opened device. Any outstanding I/O requests must
	be finished. It is safe to call CloseDevice with a cleared iorequest
	structure or one that failed to open.

    INPUTS
	iORequest - Pointer to iorequest structure.

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	OpenDevice().

    INTERNALS

    HISTORY

******************************************************************************/
{
    AROS_LIBFUNC_INIT

#if (AROS_FLAVOUR == AROS_FLAVOUR_NATIVE)
    struct Device *dev = iORequest->io_Device;
#endif

    /* Single-thread the close routine. */
    Forbid();

    /* Something to do? */
    if(iORequest->io_Device!=NULL)
    {
	(void)AROS_LVO_CALL1(BPTR,
	    AROS_LCA(struct IORequest *,iORequest, A1),
	    struct Device *,iORequest->io_Device,2,
	);
	/*
	    Normally you'd expect the device to be expunged if this returns
	    non-zero, but this is only exec which doesn't know anything about
	    seglists - therefore dos.library has to SetFunction() into this
	    vector for the additional functionality.
	*/

	/* Trash device field */
	iORequest->io_Device=(struct Device *)-1;
    }

    /* All done. */
    Permit();

#if (AROS_FLAVOUR == AROS_FLAVOUR_NATIVE)
    /*
	Kludge to force the device base to register d0. Ramlib patches this
	vector for seglist expunge capability and expects the device base in
	d0 after it has called the original (this) function.
    */
    {
	/* Put the library base in register d0 */
	register struct Device *ret __asm("d0") = dev;

	/* Make sure the above assignment isn't optimized away */
	asm volatile("": : "r" (ret));
    }
#endif

    AROS_LIBFUNC_EXIT
} /* CloseDevice */

