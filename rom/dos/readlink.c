/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Read the soft-link information.
    Lang: English
*/
#include "dos_intern.h"
#include <dos/filesystem.h>
#include <exec/lists.h>
#include <proto/exec.h>

struct ReadLinkDeviceUnit
{
    struct MinNode node;
    struct Device *device;
    struct Unit *unit;
};

/*****************************************************************************

    NAME */
#include <proto/dos.h>

	AROS_LH5(LONG, ReadLink,

/*  SYNOPSIS */
	AROS_LHA(struct MsgPort *, port, D1),
	AROS_LHA(BPTR            , lock, D2),
	AROS_LHA(CONST_STRPTR    , path, D3),
	AROS_LHA(STRPTR          , buffer, D4),
	AROS_LHA(ULONG           , size, D5),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 73, Dos)

/*  FUNCTION
	Read the filename referred to by the soft-linked object contained
	in |path| (relative to the lock |lock|) into the buffer |buffer|.
	The variable |path| should contain the name of the object that
	caused the original OBJECT_IS_SOFT_LINK error.

    INPUTS
	port		- The handler to send the request to.
	lock		- Object that |path| is relative to.
	path		- Name of the object that caused the error.
	buffer		- Buffer to fill with resolved filename.
	size		- Length of the buffer.

    RESULT
	>= 0	length of resolved filename in case of success
	== -1	failure, see IoErr() for more information
	== -2   buffer size was too small to store resolved filename

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	MakeLink()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct IOFileSys iofs;
    struct FileHandle *fh = BADDR(lock);	
    LONG err = 0;
    LONG ret = -1;

    InitIOFS(&iofs, FSA_READ_SOFTLINK, DOSBase);

    iofs.io_Union.io_READ_SOFTLINK.io_Buffer = buffer;
    iofs.io_Union.io_READ_SOFTLINK.io_Size   = size;

    if(fh)
    {
        iofs.IOFS.io_Device = fh->fh_Device;
        iofs.IOFS.io_Unit   = fh->fh_Unit;

        err = DoIOFS(&iofs, NULL, path, DOSBase);
        if(!err)
           ret = iofs.io_Union.io_READ_SOFTLINK.io_Size;
    }
    else
    {
	struct DosList *dl = NULL;
	struct ReadLinkDeviceUnit *deviceunit;
	struct MinList deviceunits;
	struct Node *tmpNode;
	char found = 0;
	NEWLIST(&deviceunits);

	/* Quickly get all device units with such port, store them in 
	   temporary list. */
	for (
	    dl = LockDosList(LDF_DEVICES | LDF_READ); 
	    dl != NULL; 
	    dl = BADDR(dl->dol_Next)
	)
	{
	    if(
		dl->dol_Type == DLT_DEVICE && (
		(struct MsgPort *)dl->dol_Ext.dol_AROS.dol_Device == port)
	    )
	    {
		if((deviceunit = AllocMem(
		    sizeof(struct ReadLinkDeviceUnit),
		    MEMF_ANY | MEMF_CLEAR
		)))
		{
		    deviceunit->device = dl->dol_Ext.dol_AROS.dol_Device;
		    deviceunit->unit = dl->dol_Ext.dol_AROS.dol_Unit;
		    AddTail(
			(struct List*) &deviceunits, 
			(struct Node*) deviceunit
		    );
		}
		else
		{
		    err = ERROR_NO_FREE_STORE;
		    break;
		}
	    }
	}
	UnLockDosList(LDF_DEVICES | LDF_READ);

	if(!err)
	{
    	    /* Now try all units from the list */
	    ForeachNode(&deviceunits, deviceunit)
	    {
		iofs.IOFS.io_Device = deviceunit->device;
		iofs.IOFS.io_Unit = deviceunit->unit;
		iofs.io_Union.io_READ_SOFTLINK.io_Size     = size;
    
                err = DoIOFS(&iofs, NULL, path, DOSBase);
		if(!err)
		{
		    ret = iofs.io_Union.io_READ_SOFTLINK.io_Size;
		    found = 1;
		    break;
		}
	    }
    
	    if(!found)
		err = ERROR_OBJECT_NOT_FOUND;
	}

	/* Free our temporary list */
	ForeachNodeSafe(&deviceunits, deviceunit, tmpNode)
	{
	    Remove((struct Node*) deviceunit);
	    FreeMem(deviceunit, sizeof(struct ReadLinkDeviceUnit));
	}
    }

    SetIoErr(err);

    return err == 0 ? ret : -1;

    AROS_LIBFUNC_EXIT
} /* ReadLink */
