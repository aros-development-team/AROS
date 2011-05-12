/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Open a device.
    Lang: english
*/

#include <aros/debug.h>
#include <exec/execbase.h>
#include <exec/devices.h>
#include <exec/io.h>
#include <exec/errors.h>
#include <aros/libcall.h>
#include <exec/libraries.h>
#include <proto/exec.h>

/*****************************************************************************

    NAME */

	AROS_LH4(LONG, OpenDevice,

/*  SYNOPSIS */
	AROS_LHA(CONST_STRPTR,       devName,    A0),
	AROS_LHA(IPTR,               unitNumber, D0),
	AROS_LHA(struct IORequest *, iORequest,  A1),
	AROS_LHA(ULONG,              flags,      D1),

/*  LOCATION */
	struct ExecBase *, SysBase, 74, Exec)

/*  FUNCTION
	Tries to open a device and fill the iORequest structure.  An error
	is returned if this fails, 0 if all went well.

	If the device doesn't exist in the current system device list, then
	first the system ROMtag module list, then if the DOS is running,
	then the DEVS: directory will be tried.

    INPUTS
	devName    - Pointer to the devices's name.
	unitNumber - The unit number. Most often 0. In some special cases this can be
		     a pointer to something (device-dependent).
	iORequest  - Pointer to device specific information.
		     Will be filled out by the device.
		     Must lie in public (or at least shared) memory.
	flags	   - Some flags to give to the device.

    RESULT
	Error code or 0 if all went well. The same value can be found
	in the io_Error field.

    NOTES
	Return type is internally extended to LONG in all existing official ROMs
	(EXT.W D0 + EXT.L D0) DoIO() and WaitIO() do the same.
	Many programs assume LONG return code, even some WB utilities.

    EXAMPLE

    BUGS

    SEE ALSO
	OpenDevice()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    /*
     * Kludge for compatibility with V40 kickstart. DO NOT depend on this!
     * See TaggedOpenLibrary() for more info.
     */
    switch ((IPTR)devName)
    {
    case 0:
    	devName = "timer.device";
    	break;

    case 1:
        devName = "input.device";
        break;
    }

    D(bug("[exec] OpenDevice(\"%s\", %ld, 0x%p, %d) by \"%s\"\n", devName, unitNumber, iORequest,
    	  flags, SysBase->ThisTask->tc_Node.ln_Name));

    /* Arbitrate for the device list */
    Forbid();

    /* Look for the device in our list */
    iORequest->io_Unit   = NULL;
    iORequest->io_Device = (struct Device *)FindName(&SysBase->DeviceList, devName);
    D(bug("[OpenDevice] Found resident 0x%p\n", iORequest->io_Device));

    /* Something found ? */
    if (iORequest->io_Device)
    {
	iORequest->io_Error = 0;

	/* Call Open vector. */
	AROS_LVO_CALL3NR(void,
	    AROS_LCA(struct IORequest *,iORequest,A1),
	    AROS_LCA(IPTR, unitNumber,D0),
	    AROS_LCA(ULONG,flags,D1),
	    struct Device *, iORequest->io_Device, 1, dev);

	/* Check for error */
	if (iORequest->io_Error)
	    /* Mark request as non-open */
	    iORequest->io_Device=NULL;
    }
    else
    	iORequest->io_Error = IOERR_OPENFAIL;

    /*
     *	We cannot handle loading devices from disk. But thankfully this is
     *	taken care of by dos.library (well lddemon really). It replaces
     *	this function with one of its own via the SetFunction() call.
     */

    /* All done. */
    Permit();

    D(bug("[OpenDevice] Returning device 0x%p, unit 0x%p, error %d\n", iORequest->io_Device, iORequest->io_Unit, iORequest->io_Error));

    return iORequest->io_Error;

    AROS_LIBFUNC_EXIT
} /* OpenDevice */

