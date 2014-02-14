/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Add a bootable device into the system.
    Lang: english
*/

#define DEBUG 0
#include <aros/debug.h>

#include <exec/memory.h>
#include <exec/execbase.h>
#include <dos/filehandler.h>

#include <proto/exec.h>
#include <proto/dos.h>

#include "expansion_intern.h"

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

	This allows device drivers to add devices into the system's disk
	device list at any time, without having to worry about whether DOS
	is available.

	If a device is added before DOS is running, then it is possible for
	the device to be used as a boot device. This allows for the user
	to choose which device he/she wishes to boot from, and even which
	OS they may wish to boot from.

	The bootstrap will attempt to boot from the highest priority device
	on the Expansion BootNode list, and if that fails continue
	through the list until it can succeed.

	Floppy disk devices should always be given the highest priority, to
	allow a user to prevent a hard disk or network boot by inserting a
	floppy disk.

	AddBootNode() will also perform a second bit of magic, that if there
	is no filesystem specified for this device, (i.e. dn_SegList, dn_Task
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
			If left NULL, the node cannot be BootPoint booted.

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
    APTR DOSBase;
    BOOL ok = FALSE;

    if (deviceNode == NULL)
        return ok;

    D(bug("[AddBootNode] Adding %b from Task %s\n", deviceNode->dn_Name, FindTask(NULL)->tc_Node.ln_Name));

    ObtainSemaphore(&IntExpBase(ExpansionBase)->BootSemaphore);

    /* See if DOS is up and running... */
    DOSBase = OpenLibrary("dos.library", 0);

    /* See if DOS has chosen a boot device yet */
    if (!(ExpansionBase->Flags & EBF_DOSFLAG))
    {
        /* If DOS isn't up yet, that's fine */
        ok = TRUE;

        /* Don't add the same node twice */
        ForeachNode(&ExpansionBase->MountList, bn)
        {
            if (stricmp(AROS_BSTR_ADDR(
                ((struct DeviceNode *) bn->bn_DeviceNode)->dn_Name),
                AROS_BSTR_ADDR(deviceNode->dn_Name)) == 0)
            {
                /* so there was already an entry with that DOS name */
                D(bug("[AddBootNode] Rejecting attempt to add duplicate device\n"));
                ok = FALSE;
            }
        }

        if (ok)
        {
            bn = AllocMem(sizeof(struct BootNode), MEMF_CLEAR | MEMF_PUBLIC);
            if (bn == NULL)
                ok = FALSE;
        }

        if (ok)
        {
            bn->bn_Node.ln_Name = (STRPTR)configDev;
            bn->bn_Node.ln_Type = NT_BOOTNODE;
            bn->bn_Node.ln_Pri = bootPri;
            bn->bn_Flags = flags;
            bn->bn_DeviceNode = deviceNode;
            D(bug("[AddBootNode] Add BootNode %p to the MountList\n", bn));
            Forbid();
            Enqueue(&ExpansionBase->MountList, (struct Node *)bn);
            Permit();
        }
        else
            ok = FALSE;
    }
    else if (DOSBase != NULL)
    {
        /* We should add the filesystem to the DOS device list. It will
         * be usable from this point onwards.
         *
         * The DeviceNode structure that was passed to us can be added
         * to the DOS list as it is, and we will let DOS start the
         * filesystem task if it is necessary to do so.
         */
        if (AddDosEntry((struct DosList *)deviceNode))
        {
            if (!(flags & ADNF_STARTPROC))
            {
                ok = TRUE;
            }
            else
            {
                STRPTR dosname;
                BYTE len = AROS_BSTR_strlen(deviceNode->dn_Name);

                /* append a colon to the name, DeviceProc() needs a full path */
                dosname = AllocVec(len + 1 + 1, MEMF_ANY);
                if (dosname)
                {
                    CopyMem(AROS_BSTR_ADDR(deviceNode->dn_Name), dosname, len);
                    dosname[len++] = ':';
                    dosname[len++] = 0;

                    /* DeviceProc() will see that dn_Device for this node is NULL
                     * and start up the handler.
                     */
                    D(bug("[AddBootNode] Starting up \"%s\"\n", dosname));
                    DeviceProc(dosname);
                    FreeVec(dosname);
                    ok = TRUE;
                }
            }
        }

    }
    else
        Alert(AT_DeadEnd | AG_OpenLib | AO_DOSLib);

    ReleaseSemaphore(&IntExpBase(ExpansionBase)->BootSemaphore);
    if (DOSBase != NULL)
        CloseLibrary((struct Library *)DOSBase);

    return ok;

    AROS_LIBFUNC_EXIT
} /* AddBootNode */
