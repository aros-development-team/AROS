/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.8  1997/01/07 12:29:08  digulla
    Removed AROS_LVO_CALL*NR() macros

    Revision 1.7  1997/01/01 03:46:13  ldp
    Committed Amiga native (support) code

    Changed clib to proto

    Revision 1.6  1996/12/10 13:51:49  aros
    Moved all #include's in the first column so makedepend can see it.

    Revision 1.5  1996/10/24 15:50:53  aros
    Use the official AROS macros over the __AROS versions.

    Revision 1.4  1996/08/13 13:56:05  digulla
    Replaced AROS_LA by AROS_LHA
    Replaced some AROS_LH*I by AROS_LH*
    Sorted and added includes

    Revision 1.3  1996/08/01 17:41:15  digulla
    Added standard header for all files

    Desc:
    Lang: english
*/
#include <exec/execbase.h>
#include <exec/devices.h>
#include <exec/io.h>
#include <exec/errors.h>
#include <aros/libcall.h>
#include <exec/libraries.h>
#include <proto/exec.h>

/*****************************************************************************

    NAME */

	AROS_LH4(BYTE, OpenDevice,

/*  SYNOPSIS */
	AROS_LHA(STRPTR,             devName,    A0),
	AROS_LHA(ULONG,              unitNumber, D0),
	AROS_LHA(struct IORequest *, iORequest,  A1),
	AROS_LHA(ULONG,              flags,      D1),

/*  LOCATION */
	struct ExecBase *, SysBase, 74, Exec)

/*  FUNCTION
	Tries to open a device and fill the iORequest structure.
	And error is returned if this fails, 0 if all went well.

    INPUTS
	devName    - Pointer to the devices's name.
	unitNumber - The unit number. Most often 0.
	iORequest  - Pointer do device specific information.
		     Will be filled out by the device.
		     Must lie in public (or at least shared) memory.
	flags	   - Some flags to give to the device.

    RESULT
	Error code or 0 if all went well. The same value can be found
	in the io_Error field.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	CloseDevice()

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    AROS_LIBBASE_EXT_DECL(struct ExecBase *,SysBase)
    struct Device *device;
    BYTE ret=IOERR_OPENFAIL;

    /* Arbitrate for the device list */
    Forbid();

    /* Look for the device in our list */
    device=(struct Device *)FindName(&SysBase->DeviceList,devName);

    /* Something found ? */
    if(device!=NULL)
    {
	/* Init iorequest */
	iORequest->io_Error=0;
	iORequest->io_Device=device;
	iORequest->io_Flags=flags;
	iORequest->io_Message.mn_Node.ln_Type=NT_REPLYMSG;

	/* Call Open vector. */
	AROS_LVO_CALL3(void,
	    AROS_LCA(struct IORequest *,iORequest,A1),
	    AROS_LCA(ULONG,unitNumber,D0),
	    AROS_LCA(ULONG,flags,D1),
	    struct Device, device, 1,
	);

	/* Check for error */
	ret=iORequest->io_Error;
	if(ret)
	    /* Mark request as non-open */
	    iORequest->io_Device=NULL;
    }
    /*
	else
	{
	Under normal circumstances you'd expect the device loading here -
	but this is only exec which doesn't know anything about the
	filesystem level. Therefore dos.library has to SetFunction() this vector
	for the additional functionality.
	}
    */

    /* All done. */
    Permit();
    return ret;
    AROS_LIBFUNC_EXIT
} /* OpenDevice */

