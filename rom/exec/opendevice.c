/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
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

/*****************************************************************************

    NAME */
	#include <exec/libraries.h>
	#include <clib/exec_protos.h>

	__AROS_LH4(BYTE, OpenDevice,

/*  SYNOPSIS */
	__AROS_LA(STRPTR,             devName,    A0),
	__AROS_LA(ULONG,              unitNumber, D0),
	__AROS_LA(struct IORequest *, iORequest,  A1),
	__AROS_LA(ULONG,              flags,      D1),

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
    __AROS_FUNC_INIT

    __AROS_BASE_EXT_DECL(struct ExecBase *,SysBase)
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
	__AROS_LVO_CALL3(void,1,device,iORequest,A1,unitNumber,D0,flags,D1);

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
    __AROS_FUNC_EXIT
} /* OpenDevice */

