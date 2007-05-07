/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: dos.library function Examine().
    Lang: English
*/
#include <exec/memory.h>
#include <proto/exec.h>
#include <dos/filesystem.h>
#include <dos/exall.h>
#include "dos_intern.h"

/*****************************************************************************

    NAME */
#include <proto/dos.h>

	AROS_LH2(BOOL, Examine,

/*  SYNOPSIS */
	AROS_LHA(BPTR,                   lock, D1),
	AROS_LHA(struct FileInfoBlock *, fib,  D2),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 17, Dos)

/*  FUNCTION

    Fill in a FileInfoBlock structure concerning a file or directory 
    associated with a particular lock.

    INPUTS

    lock  --  lock to examine
    fib   --  FileInfoBlock where the result of the examination is stored

    RESULT

    A boolean telling whether the operation was successful or not.

    NOTES

    FileInfoBlocks should be allocated with AllocDosObject(). You may make
    a copy of the FileInfoBlock but, however, this copy may NOT be passed
    to ExNext()!

    EXAMPLE

    BUGS

    SEE ALSO

    Lock(), UnLock(), ExNext(), AllocDosObject(), ExAll(), <dos/dos.h>

    INTERNALS

*****************************************************************************/

/*****************************************************************************

    NAME
#include <clib/dos_protos.h>

	AROS_LH2(BOOL, ExamineFH,

    SYNOPSIS
	AROS_LHA(BPTR                  , fh, D1),
	AROS_LHA(struct FileInfoBlock *, fib, D2),

    LOCATION
	struct DosLibrary *, DOSBase, 65, Dos)

    FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
/*AROS alias ExamineFH Examine */
{
    AROS_LIBFUNC_INIT

    UBYTE buffer[512];
    struct ExAllData *ead=(struct ExAllData *)buffer;
    STRPTR src, dst;
    ULONG i;

    /* Get pointer to filehandle */
    struct FileHandle *fh = (struct FileHandle *)BADDR(lock);

    /* Get pointer to I/O request. Use stackspace for now. */
    struct IOFileSys iofs;

    /* Prepare I/O request. */
    InitIOFS(&iofs, FSA_EXAMINE, DOSBase);

    iofs.IOFS.io_Device = fh->fh_Device;
    iofs.IOFS.io_Unit	= fh->fh_Unit;

    iofs.io_Union.io_EXAMINE.io_ead  = (struct ExAllData *)buffer;
    iofs.io_Union.io_EXAMINE.io_Size = sizeof(buffer);
    iofs.io_Union.io_EXAMINE.io_Mode = ED_OWNER;

    /* Send the request. */
    DosDoIO(&iofs.IOFS);

    /* Set error code and return */
    SetIoErr(iofs.io_DosError);

    if(iofs.io_DosError)
	return DOSFALSE;
    else
    {
        /* in fib_DiskKey the result from telldir is being stored which
           gives us important info for a call to ExNext() */
	fib->fib_DiskKey      = iofs.io_DirPos;
	fib->fib_DirEntryType = ead->ed_Type;

	src = ead->ed_Name;
	dst = fib->fib_FileName;

	if(src != NULL)
	{
	    for(i = 0; i < MAXFILENAMELENGTH - 1; i++)
	    {
		if(!(*dst++ = *src++))
		    break;
	    }
	}

	*dst++ = 0;

	fib->fib_Protection	= ead->ed_Prot;
	fib->fib_EntryType	= ead->ed_Type;
	fib->fib_Size		= ead->ed_Size;
	fib->fib_Date.ds_Days	= ead->ed_Days;
	fib->fib_Date.ds_Minute	= ead->ed_Mins;
	fib->fib_Date.ds_Tick	= ead->ed_Ticks;

	src = ead->ed_Comment;
	dst = fib->fib_Comment;

	if(src != NULL)
	{
	    for(i = 0; i < 79; i++)
	    {
		if(!(*dst++ = *src++))
		    break;
	    }
	}

	*dst++ = 0;
	fib->fib_OwnerUID = ead->ed_OwnerUID;
	fib->fib_OwnerGID = ead->ed_OwnerGID;

	return DOSTRUE;
    }

    AROS_LIBFUNC_EXIT
} /* Examine */
