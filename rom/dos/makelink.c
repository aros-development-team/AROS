/*
    Copyright (C) 1995-1998 AROS - The Amiga Replacement OS
    $Id$

    Desc: Make a hard or soft link.
    Lang: english
*/
#include "dos_intern.h"
#include <dos/filesystem.h>

/*****************************************************************************

    NAME */
#include <proto/dos.h>

	AROS_LH3(LONG, MakeLink,

/*  SYNOPSIS */
	AROS_LHA(STRPTR, name, D1),
	AROS_LHA(LONG  , dest, D2),
	AROS_LHA(LONG  , soft, D3),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 74, Dos)

/*  FUNCTION
	MakeLink() will create a link between two files or directories.
	A link is a filesystem object that refers to another file.

	A soft link refers to another file by name, and is resolved by
	the filesystem and the caller. Soft links are not restricted to
	the same volume. The |dest| argument is a NUL terminated pathname
	to the pre-existing object. Soft links can be used on directories.

	A hard link refers to another file by the location on a disk, and
	is resolved by the filesystem. Hard links are restricted to files
	on the same volume. The |dest| argument is a lock on another file.

    INPUTS
	name		- The name of the link to create.
	dest		- The destination file/lock.
	soft		- if 1 - create a soft link
			  if 0 - create a hard link.

    RESULT

    NOTES

    EXAMPLE

    BUGS
	Soft links were not working in the ROM filesystem before version
	37.

    SEE ALSO
	ReadLink()

    INTERNALS

    HISTORY
	27-11-96    digulla automatically created from
			    dos_lib.fd and clib/dos_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct DosLibrary *,DOSBase)

    struct IOFileSys io,*iofs=&io;
    struct Process *pr = (struct Process *)FindTask(NULL);
    
    /* Prepare I/O request */
    iofs->IOFS.io_Message.mn_Node.ln_Type = NT_REPLYMSG;
    iofs->IOFS.io_Message.mn_ReplyPort	  = &me->pr_MsgPort;
    iofs->IOFS.io_Message.mn_Length	  = sizeof(struct IOFileSys);
    iofs->IOFS.io_Flags = 0;

    if( soft == 0 )
    {
	iofs->IOFS.io_Command = FSA_CREATE_HARDLINK;
	iofs->io_Union.io_CREATE_HARDLINK.io_OldFile = dest;
    }
    else
    {
	iofs->IOFS.io_Command = FSA_CREATE_SOFTLINK;
	iofs->io_Union.io_CREATE_SOFTLINK.io_Reference = dest;
    }

    return (DoName(iofs,name,DOSBase) == 0);

    AROS_LIBFUNC_EXIT
} /* MakeLink */
