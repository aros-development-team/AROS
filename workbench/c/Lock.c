/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Lock - Set the write protect status of a device.
    Lang: english
*/

/**************************************************************************

    NAME
	Lock

    FORMAT
	Lock <drive> [ON|OFF] [<passkey>]

    SYNOPSIS
	DRIVE/A,ON/S,OFF/S,PASSKEY

    LOCATION
	Workbench:c

    FUNCTION
	Lock will cause the specified device or partition to be made write-
	protected or write-enabled. This write protection is a soft write
	protection which is handled by the volume filesystem. Hence the
	protection will be reset (to writable) on the next system reboot.

	It is possible to specify an optional passkey which can be used to
	password protect the locking. The same passkey that is used to lock
	the volume must be used to unlock the volume. The passkey may be
	any number of characters in length.

	The volume given MUST be the device or root volume name, not an
	assign.

    EXAMPLE
	
	1.SYS:> Lock Work:

	    This will lock the volume called Work: without a passkey.

	1.SYS:> Lock Work:
	1.SYS:> MakeDir Work:SomeDir
	Can't create directory Work:Test
	MakeDir: Disk is write-protected

	    The volume Work: is locked, so it is impossible to create a
	    directory.

	1.SYS:> Lock Work: OFF

	    This will unlock the volume work.

	1.SYS:> Lock Work: MyPassword

	    This will lock Work: with the passkey "MyPassword"

**************************************************************************/

#include <exec/types.h>
#include <exec/memory.h>
#include <dos/dosextens.h>
#include <dos/filesystem.h>
#include <dos/rdargs.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <utility/tagitem.h>

#define ARG_TEMPLATE	"DRIVE/A,ON/S,OFF/S,PASSKEY"
#define ARG_DRIVE	0
#define ARG_ON		1
#define ARG_OFF		2
#define ARG_PASSKEY	3
#define TOTAL_ARGS	4

static const char version[]= "$VER: Lock 41.2 (16.1.1999)";
static const char exthelp[] =
    "Lock : Set the write-protect state of a volume\n"
    "\tDRIVE/A   The name of the volume to lock\n"
    "\tON/S      Turn write protect on\n"
    "\tOFF/S     Turn write protect off\n"
    "\tPASSKEY   The password string for the lock (optional)\n";

int lockDevice(struct IOFileSys *iofs, STRPTR key)
{
    if( iofs->io_Union.io_MOUNT_MODE.io_MountMode & MMF_LOCKED )
    {
	/* We are already locked? What do we do? */
	return ERROR_DISK_WRITE_PROTECTED;
    }
    iofs->io_Union.io_MOUNT_MODE.io_MountMode = MMF_LOCKED;
    iofs->io_Union.io_MOUNT_MODE.io_Mask = MMF_LOCKED;
    iofs->io_Union.io_MOUNT_MODE.io_Password = key;

    DoIO((struct IORequest *)iofs);
    return iofs->io_DosError;
}

int unlockDevice(struct IOFileSys *iofs, STRPTR key)
{
    if(!(iofs->io_Union.io_MOUNT_MODE.io_MountMode & MMF_LOCKED))
    {
	/* We are not locked, so lets return success. */
	return 0;
    }
    /* We are locked, lets try and unlock it */
    iofs->io_Union.io_MOUNT_MODE.io_MountMode = 0;
    iofs->io_Union.io_MOUNT_MODE.io_Mask = MMF_LOCKED;
    iofs->io_Union.io_MOUNT_MODE.io_Password = key;

    DoIO((struct IORequest *)iofs);
    return iofs->io_DosError;
}

int __nocommandline;

int main(void)
{
    struct Process *pr = (struct Process *)FindTask(NULL);
    struct RDArgs *rd, *rda = NULL;
    IPTR args[TOTAL_ARGS] = { NULL, FALSE, FALSE, NULL };
    struct IOFileSys *iofs;
    struct DevProc *dp;
    int error = 0;

    if((rda = AllocDosObject(DOS_RDARGS, NULL)))
    {
	rda->RDA_ExtHelp = (STRPTR)exthelp;
	if((rd = ReadArgs(ARG_TEMPLATE, args, NULL)))
	{
	    /* Firstly, find out the volume */
	    dp = GetDeviceProc((STRPTR)args[ARG_DRIVE], NULL);

	    if(    dp == NULL || dp->dvp_DevNode == NULL 
		|| (   dp->dvp_DevNode->dol_Type != DLT_DEVICE 
		    && dp->dvp_DevNode->dol_Type != DLT_VOLUME )
	    )
	    {
		if( dp != NULL )
    		    error = ERROR_OBJECT_WRONG_TYPE;
		else
		    error = IoErr();
	    }
	    else
	    {
		/*
		    It is a real volume, not an assign...
		    We send the device a I/O request.
		*/
		iofs = (struct IOFileSys *)CreateIORequest(
		    	    &pr->pr_MsgPort, sizeof(struct IOFileSys)
			);
		if( iofs != NULL )
		{
		    /* First of find out current state. */
		    iofs->io_Union.io_MOUNT_MODE.io_Mask = 0;
		    iofs->IOFS.io_Device = (struct Device *)dp->dvp_Port;
		    iofs->IOFS.io_Command = FSA_MOUNT_MODE;

		    DoIO((struct IORequest *)iofs);
		    if (args[ARG_ON] && args[ARG_OFF])
		    {
			/* Both are set? */
			error = ERROR_TOO_MANY_ARGS;
		    }
		    else if (args[ARG_OFF])
		    {
			error = unlockDevice(iofs, (STRPTR)args[ARG_PASSKEY]);
		    }
		    else
		    {
			error = lockDevice(iofs, (STRPTR)args[ARG_PASSKEY]);
		    }
		    DeleteIORequest((struct IORequest *)iofs);

		} /* iofs != NULL */
		else
		    error = ERROR_NO_FREE_STORE;
	    }

	    if( dp )
		FreeDeviceProc(dp);

	    FreeArgs(rd);
	} /* if( ReadArgs() ) */
	else
	    error = IoErr();

	FreeDosObject(DOS_RDARGS, rda);
    } /* if( AllocDosObject() ) */
    else
	error = ERROR_NO_FREE_STORE;

    if( error != 0 )
    {
	PrintFault(error, "Lock");
	return RETURN_FAIL;
    }

    SetIoErr(0);
    return 0;
}
