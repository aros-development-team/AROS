/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
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
	C:

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
#define DEBUG 0
#include <aros/debug.h>

#include <exec/types.h>
#include <exec/memory.h>
#include <dos/dosextens.h>
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

const TEXT version[] = "$VER: Lock 41.3 (7.12.2011)";
static const char exthelp[] =
    "Lock : Set the write-protect state of a volume\n"
    "\tDRIVE/A   The name of the volume to lock\n"
    "\tON/S      Turn write protect on\n"
    "\tOFF/S     Turn write protect off\n"
    "\tPASSKEY   The password string for the lock (optional)\n";

BOOL lockDevice(struct DevProc *dvp, ULONG key)
{
    return DoPkt(dvp->dvp_Port, ACTION_WRITE_PROTECT, DOSTRUE, key, 0, 0, 0);
}

BOOL unlockDevice(struct DevProc *dvp, ULONG key)
{
    return DoPkt(dvp->dvp_Port, ACTION_WRITE_PROTECT, DOSFALSE, key, 0, 0, 0);
}

/* Perl's hash() algorithm
 */
#define HASH_MULTIPLIER 33

ULONG Hash(CONST_STRPTR key)
{
    ULONG hash;

    if (key == 0 || key[0] == 0)
       return 0;

    for (hash = 0; *key; key++)
        hash = HASH_MULTIPLIER * hash + *key;

    /* Improve distribution of low-order bits */
    hash += (hash >> 5);

    return hash;
}

int __nocommandline;

int main(void)
{
    struct RDArgs *rd, *rda = NULL;
    IPTR args[TOTAL_ARGS] = { 0, FALSE, FALSE, 0 };
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
	        /* First of find out current state. */
	        if (args[ARG_ON] && args[ARG_OFF])
	        {
	        	/* Both are set? */
	        	error = ERROR_TOO_MANY_ARGS;
	        }
	        else if (args[ARG_OFF])
	        {
	            error = unlockDevice(dp, Hash((CONST_STRPTR)args[ARG_PASSKEY])) ? IoErr() : 0;
	        }
	        else
	        {
                    error = lockDevice(dp, Hash((CONST_STRPTR)args[ARG_PASSKEY])) ? IoErr() : 0;
	        }
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
