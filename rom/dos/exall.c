/*
    (C) 1995-2000 AROS - The Amiga Research OS
    $Id$

    Desc: Examine a directory.
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

	AROS_LH5(BOOL, ExAll,

/*  SYNOPSIS */
	AROS_LHA(BPTR,                  lock,    D1),
	AROS_LHA(struct ExAllData *,    buffer,  D2),
	AROS_LHA(LONG,                  size,    D3),
	AROS_LHA(LONG,                  data,    D4),
	AROS_LHA(struct ExAllControl *, control, D5),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 72, Dos)

/*  FUNCTION

    Examine an entire directory.

    INPUTS

    lock     --  lock on the directory to be examined
    buffer   --  buffer for the data that is returned (must be aligned)
                 which is filled with (partial) ExAllData structures
		 (see NOTES)
    size     --  size of 'buffer' in bytes
    type     --  type of the data to be returned
    control  --  a control structure allocated by AllocDosObject()

    RESULT

    An indicator of if ExAll() is done. If FALSE is returned, either ExAll()
    has completed in which case IoErr() is ERROR_NO_MORE_ENTRIES or an
    error occurred. If a non-zero value is returned ExAll() must be called
    again until it returns FALSE.

    NOTES
    
    The following information is essential information on the ExAllData
    structure:

    ead_type :

    ED_NAME        --  filename
    ED_TYPE        --  type
    ED_SIZE        --  size in bytes
    ED_PROTECTION  --  protection bits
    ED_DATE        --  date information (3 longwords)
    ED_COMMENT     --  file comment (NULL if no comment exists)
    ED_OWNER       --  owner user and group id

    This is an incremental list meaning that if you specify ED_OWNER you
    will get ALL attributes!


    Filesystems that support ExAll() must support at least up to ED_COMMENT.
    If a filesystem doesn't support a particular type, ERROR_BAD_NUMBER must
    be returned.

    ead_Next : pointer to the next entry in the buffer. The last entry
               has a NULL value for ead_Next.


    The control structure have the following fields.

    eac_Entries : the number of entries in the buffer after a call to ExAll().
                  Make sure that your code handles the case when eac_Entries
                  is 0 and ExAll() returns TRUE.

    eac_LastKey : must be initialized to 0 before calling ExAll() for the
                  first time.

    eac_MatchString : if NULL then information on all files will be returned.
                      If non-NULL it's interpreted as a pointer to a string
                      used for pattern matching which files to return
		      information on. This string must have been parsed by
		      ParsePatternNoCase()!

    eac_MatchFunc : pointer to a hook that will be called to decide if an
                    entry should be included in the buffer. If NULL, no
		    matching function will be called. The hook is called as
		    follows

		        BOOL = MatchFunc(hook, data, typeptr)

    EXAMPLE

    BUGS

    No ExAll() emulation using Examine()/ExNext() is currently provided.

    SEE ALSO

    Examine(), ExNext(), MatchPatternNoCase(), ParsePatternNoCase(),
    AllocDosObject(), ExAllEnd()

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
    InitIOFS(&iofs, FSA_EXAMINE_ALL, DOSBase);

    iofs.IOFS.io_Device = fh->fh_Device;
    iofs.IOFS.io_Unit   = fh->fh_Unit;

    iofs.io_Union.io_EXAMINE_ALL.io_ead  = buffer;
    iofs.io_Union.io_EXAMINE_ALL.io_Size = size;
    iofs.io_Union.io_EXAMINE_ALL.io_Mode = data;

    /* Send the request. */
    DoIO(&iofs.IOFS);

    /* Set error code and return */
    SetIoErr(iofs.io_DosError);

    if(iofs.io_DosError != 0)
    {
	control->eac_Entries = 0;
	return DOSFALSE;
    }

    for(size = 1; buffer != NULL; size++)
	buffer = buffer->ed_Next;

    control->eac_Entries = size;

    return DOSTRUE;

    AROS_LIBFUNC_EXIT
} /* ExAll */
