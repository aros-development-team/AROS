/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Add a device to the public list of devices.
    Lang: english
*/
#include <exec/execbase.h>
#include <exec/devices.h>
#include <aros/libcall.h>
#include <proto/exec.h>

/*****************************************************************************

    NAME */

	AROS_LH1(void, AddDevice,

/*  SYNOPSIS */
	AROS_LHA(struct Device *, device,A1),

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

******************************************************************************/
{
    AROS_LIBFUNC_INIT
    ASSERT_VALID_PTR(device);

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
    AROS_LIBFUNC_EXIT
} /* AddDevice */

