/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.4  1996/08/13 13:55:55  digulla
    Replaced __AROS_LA by __AROS_LHA
    Replaced some __AROS_LH*I by __AROS_LH*
    Sorted and added includes

    Revision 1.3  1996/08/01 17:41:01  digulla
    Added standard header for all files

    Desc:
    Lang: english
*/
#include <exec/execbase.h>
#include <exec/devices.h>
#include <aros/libcall.h>

/*****************************************************************************

    NAME */
	#include <clib/exec_protos.h>

	__AROS_LH1(void, AddDevice,

/*  SYNOPSIS */
	__AROS_LHA(struct Device *, device,A1),

/*  LOCATION */
	struct ExecBase *, SysBase, 72, Exec)

/*  FUNCTION
	Adds a given device to the system's device list after checksumming
	the device vectors. This function is not for general use but
	(of course) for building devices that don't use exec's MakeLibrary()
	mechanism.

    INPUTS
	device - Pointer to a ready for use device structure.

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	RemDevice(), OpenDevice(), CloseDevice()

    INTERNALS

    HISTORY

******************************************************************************/
{
    __AROS_FUNC_INIT

    /* Just in case the user forgot them */
    device->dd_Library.lib_Node.ln_Type=NT_DEVICE;
    device->dd_Library.lib_Flags|=LIBF_CHANGED;

    /* Build checksum for device vectors */
    SumLibrary(&device->dd_Library);

    /* Arbitrate for the device list */
    Forbid();

    /* And add the device */
    Enqueue(&SysBase->DeviceList,&device->dd_Library.lib_Node);

    /* All done. */
    Permit();
    __AROS_FUNC_EXIT
} /* AddDevice */

