/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Add a bootable device into the system.
    Lang: english
*/

#include "expansion_intern.h"
#include <exec/memory.h>
#include <exec/execbase.h>
#include <dos/filehandler.h>
#include <proto/exec.h>

/*****************************************************************************

    NAME */
#include <libraries/configvars.h>
#include <proto/expansion.h>

	AROS_LH4(BOOL, AddBootNode,

/*  SYNOPSIS */
	AROS_LHA(LONG               , bootPri, D0),
	AROS_LHA(ULONG              , flags, D1),
	AROS_LHA(struct DeviceNode *, deviceNode, A0),
	AROS_LHA(struct ConfigDev  *, configDev, A1),

/*  LOCATION */
	struct ExpansionBase *, ExpansionBase, 6, Expansion)

/*  FUNCTION
	AddBootNode() will add a device into the system. It does this in
	one of two ways:

	1. If DOS is running, add the device to DOS's list of devices
	   immediately.
	2. Otherwise, save the information for later use by DOS, possibly
	   as a boot device.

	This allows device drivers to add devices into the systems disk
	device list at any time, without having to worry about whether DOS
	is available.

	If a device is added before DOS is running, then it is possible for
	the device to be used as a boot device. This allows for the user
	to choose which device he/she wishes to boot from, and even which
	OS they may wish to boot from.

	The bootstrap will attempt to boot from the highest priority device
	on the Expansion eb_BootNode list, and if that fails continue
	through the list until it can succeed.

	Floppy disk devices should always be given the highest priority, to
	allow a user to prevent a hard disk or network boot by inserting a
	floppy disk.

	AddBootNode() will also perform a second bit of magic, that if there
	is no filesystem specified for this device, (ie dn_SegList, dn_Task
	and dn_Handler are all NULL), then the standard DOS filesystem
	will be used for this device.

    INPUTS
	bootPri     -   a BYTE describing the boot priority for this disk.
			Recommended priorities are:

			+5      -   unit 0 on the floppy disk. The floppy
				    should be the highest priority.
			0       -   standard hard disk priority
			-5      -   recommended for a network disk
			-128    -   don't bother trying

	flags       -   Additional information:

			ADNF_STARTPROC (bit 0)-
			    if set this will cause AddBootNode() to start
			    a filesystem handler for the device node from
			    the information contained in the deviceNode
			    packet. This bit is only useful when there is
			    no running handler for this task (ie dn_Task
			    is NULL).

	deviceNode  -   DOS device node for this device. Typically created
			by MakeDosNode().

	configDev   -   A valid expansion board ConfigDev structure, this
			is required for an autoboot before DOS is running.
			If left NULL, this will make the node non-bootable.

    RESULT
	TRUE if everything was ok,
	FALSE if for some reason we failed (lack of memory etc).

    NOTES
	The address of the ConfigDev structure is stored in the ln_Name
	field of the BootNode structure.

    EXAMPLE

    BUGS

    SEE ALSO
	AddDosNode()

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct BootNode *bn;

    /* This basically uses AddDosNode() to do all its work, however we
       ourselves have to add the bootnode. Unfortunately
    */

    /* Is this enough of a test? */
    if(!(ExpansionBase->Flags & EBF_BOOTFINISHED))
    {
	/* Don't add the same node twice */
	ForeachNode(&ExpansionBase->MountList, bn)
	{
	    if(stricmp(AROS_BSTR_ADDR(((struct DeviceNode *) bn->bn_DeviceNode)->dn_Name), AROS_BSTR_ADDR(deviceNode->dn_Name)) == 0)
	    {
		// so there was already an entry with that DOS name.
		return FALSE;
	    }
	}
	if((bn = AllocMem(sizeof(struct BootNode), MEMF_CLEAR|MEMF_PUBLIC)))
	{
	    bn->bn_Node.ln_Name = (STRPTR)configDev;
	    bn->bn_Node.ln_Type = NT_BOOTNODE;
	    bn->bn_Node.ln_Pri = bootPri;
	    bn->bn_Flags = flags;
	    bn->bn_DeviceNode = deviceNode;
	    Forbid();
	    Enqueue( &ExpansionBase->MountList, (struct Node *)bn );
	    Permit();
	}
	else
	    return FALSE;
    }

    /* We now let AddDosNode() do all the hard work */
    return AddDosNode(bootPri, flags, deviceNode);

    AROS_LIBFUNC_EXIT
} /* AddBootNode */
