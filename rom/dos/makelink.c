/*
    (C) 1995-97 AROS - The Amiga Replacement OS
    $Id$

    Desc: Create a hard- or softlink.
    Lang: english
*/
#include <dos/dosextens.h>
#include <dos/filesystem.h>
#include "dos_intern.h"

/*****************************************************************************

    NAME */
#include <exec/types.h>
#include <proto/dos.h>

	AROS_LH3(BOOL, MakeLink,

/*  SYNOPSIS */
	AROS_LHA(STRPTR, name, D1),
	AROS_LHA(void *, dest, D2),
	AROS_LHA(LONG  , soft, D3),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 74, Dos)

/*  FUNCTION
	Creates a link from one file to another. If ´soft´ is TRUE, a soft-link
	is created. A soft-link is just a reference by name and may be a
	relative or a absolute path. If `soft` is FALSE, a hard-link is
	created. Hard-links connect a filehandle to another name. This way a
	file can have multiple names. Hard-Links are only possible on the
	same volume.

    INPUTS
	name - The name of the link to create
	dest - If 'soft' is TRUE this must be a filename, if it is FALSE a BPTR
	       pointing to the file to be hard-linked must be provided
	soft - TRUE, if a soft-link is to be created, FALSE for an hard-link

    RESULT
	boolean - DOSTRUE or DOSFALSE. On error, IoErr() will contain more
	information.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS
	This function calls either FSA_CREATE_HARDLINK or FSA_CREATE_SOFTLINK
	on the filesystem of `name`.

    HISTORY
	27-11-96    digulla automatically created from
			    dos_lib.fd and clib/dos_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct DosLibrary *,DOSBase)
    struct Device *dev;
    struct Process *me=(struct Process *)FindTask(NULL);
    struct IOFileSys io;

    io.IOFS.io_Message.mn_Node.ln_Type = NT_REPLYMSG;
    io.IOFS.io_Message.mn_ReplyPort = &me->pr_MsgPort;
    io.IOFS.io_Message.mn_Length = sizeof(struct IOFileSys);
    io.IOFS.io_Flags = 0;
    if (soft)
    {
        /* We want a soft-link. */
        io.IOFS.io_Command = FSA_CREATE_SOFTLINK;
        io.io_Union.io_CREATE_SOFTLINK.io_Reference = (STRPTR)dest;
    } else
    {
        /* We want a hard-link. */
	struct FileHandle *fh = (struct FileHandle *)BADDR((BPTR)dest);
        /* We check, if name and dest are on the same device. */
        if (DevName(name, &dev, DOSBase))
	    return DOSFALSE;
	if (dev != fh->fh_Device)
	{
	    SetIoErr(ERROR_RENAME_ACROSS_DEVICES);
	    return DOSFALSE;
	}
	io.IOFS.io_Command = FSA_CREATE_HARDLINK;
	io.io_Union.io_CREATE_HARDLINK.io_OldFile = fh->fh_Unit;
    }

    if (DoName(&io, name, DOSBase))
        return DOSFALSE;
    else
        return DOSTRUE;
    AROS_LIBFUNC_EXIT
} /* MakeLink */
