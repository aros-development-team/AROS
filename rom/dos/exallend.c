/*
    (C) 1995-96 AROS - The Amiga Research OS
    $Id$
    $Log$
    Revision 1.7  2000/11/23 19:55:26  SDuvan
    Added or improved documentation

    Revision 1.6  2000/11/18 12:21:37  SDuvan
    Simplified

    Revision 1.5  1998/10/20 16:44:34  hkiel
    Amiga Research OS

    Revision 1.4  1997/01/27 00:36:17  ldp
    Polish

    Revision 1.3  1996/12/09 13:53:25  aros
    Added empty templates for all missing functions

    Moved #include's into first column

    Revision 1.2  1996/10/24 15:50:27  aros
    Use the official AROS macros over the __AROS versions.

    Revision 1.1  1996/09/11 12:54:45  digulla
    A couple of new DOS functions from M. Fleischer

    Desc:
    Lang: English
*/
#include <exec/memory.h>
#include <proto/exec.h>
#include <dos/filesystem.h>
#include <dos/dosextens.h>
#include <dos/exall.h>
#include "dos_intern.h"

/*****************************************************************************

    NAME */
#include <proto/dos.h>

	AROS_LH5(void, ExAllEnd,

/*  SYNOPSIS */
	AROS_LHA(BPTR,                  lock,    D1),
	AROS_LHA(struct ExAllData *,    buffer,  D2),
	AROS_LHA(LONG,                  size,    D3),
	AROS_LHA(LONG,                  data,    D4),
	AROS_LHA(struct ExAllControl *, control, D5),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 165, Dos)

/*  FUNCTION

    Stop an ExAll() operation before returning ERROR_NO_MORE_ENTRIES.

    INPUTS

    The inputs should correspond to the inputs for the ExAll() function.

    lock     --  lock on the directory that is being examined
    buffer   --  buffer for data returned
    size     --  size of 'buffer' in bytes
    type     --  type of data to be returned
    control  --  control data structure

    RESULT

    NOTES

    The control data structure must have been allocated with AllocDosObject().

    EXAMPLE

    BUGS

    SEE ALSO

    ExAll(), AllocDosObject()

    INTERNALS

    HISTORY
	29-10-95    digulla automatically created from
			    dos_lib.fd and clib/dos_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct DosLibrary *,DOSBase)
    
    /* Get pointer to filehandle */
    struct FileHandle *fh = (struct FileHandle *)BADDR(lock);

    /* Get pointer to I/O request. Use stackspace for now. */
    struct IOFileSys iofs;

    /* Prepare I/O request. */
    InitIOFS(&iofs, FSA_EXAMINE_ALL_END, DOSBase);

    iofs.IOFS.io_Device = fh->fh_Device;
    iofs.IOFS.io_Unit   = fh->fh_Unit;

    /* Send the request. May not fail. */
    DoIO(&iofs.IOFS);

    AROS_LIBFUNC_EXIT
} /* ExAllEnd */

