/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Examine a directory.
    Lang: English
*/
#include <exec/memory.h>
#include <proto/exec.h>
#include <dos/filesystem.h>
#include <dos/exall.h>
#include <stddef.h>
#include "dos_intern.h"
#include <aros/asmcall.h>

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

    SEE ALSO

    Examine(), ExNext(), MatchPatternNoCase(), ParsePatternNoCase(),
    AllocDosObject(), ExAllEnd()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct DosLibrary *,DOSBase)

    /* Get pointer to filehandle */
    struct FileHandle *fh = (struct FileHandle *)BADDR(lock);

    /* Get pointer to I/O request. Use stackspace for now. */
    struct IOFileSys iofs;

    if (control->eac_MatchString || control->eac_MatchFunc)
    {
    	#warning "Hack because of problem with our filesystem API regarding"
	#warning "ExAll handling. Filesystems don't get passed matchstring/matchfunc"
	#warning "so they cannot handle it. As workaround for now we force emulation"
	#warning "of ExAll() if any of matchstring/matchfunc is set."
	#warning "It would be better to pass ExAllControl structure to filesystems."
	#warning "Ie. change filesystem API. Also because of eac_LastKey!!!"
	
    	iofs.io_DosError = ERROR_ACTION_NOT_KNOWN;
    }
    else
    {
	/* Prepare I/O request. */
	InitIOFS(&iofs, FSA_EXAMINE_ALL, DOSBase);

	iofs.IOFS.io_Device = fh->fh_Device;
	iofs.IOFS.io_Unit   = fh->fh_Unit;

	iofs.io_Union.io_EXAMINE_ALL.io_ead  = buffer;
	iofs.io_Union.io_EXAMINE_ALL.io_eac  = control;
	iofs.io_Union.io_EXAMINE_ALL.io_Size = size;
	iofs.io_Union.io_EXAMINE_ALL.io_Mode = data;

	/* Send the request. */
	DosDoIO(&iofs.IOFS);
    }
    
    if
    (
        iofs.io_DosError == ERROR_NOT_IMPLEMENTED ||
	iofs.io_DosError == ERROR_ACTION_NOT_KNOWN
    )
    {

	/* Try to emulate it */
	struct FileInfoBlock *fib = AllocDosObject(DOS_FIB, NULL);

	STRPTR end  = (STRPTR)buffer + size;
	STRPTR next;

	struct ExAllData *last = buffer, *curr = buffer;

	control->eac_Entries = 0;
	if
	(
	    control->eac_LastKey  == 0 &&
	    fib                        &&
	    Examine(lock, fib)         &&
	    fib->fib_DirEntryType <= 0
	)
	{
 	    SetIoErr(ERROR_OBJECT_WRONG_TYPE);
	}
	else
	{
 	    static const ULONG sizes[]=
	    {
    	        0,
    		offsetof(struct ExAllData,ed_Type),
    		offsetof(struct ExAllData,ed_Size),
    		offsetof(struct ExAllData,ed_Prot),
  		offsetof(struct ExAllData,ed_Days),
		offsetof(struct ExAllData,ed_Comment),
    		offsetof(struct ExAllData,ed_OwnerUID),
    		sizeof(struct ExAllData)
	    };

	    #define ReturnOverflow()                         \
	    {                                                \
       		if (last == buffer)                          \
		    SetIoErr(ERROR_BUFFER_OVERFLOW);         \
		goto end;                                    \
	    }

	    #define CopyStringSafe(_source)              \
	    {                                            \
		STRPTR source = _source;                 \
							 \
		for (;;)                                 \
	    	{                                        \
		    if (next >= end)                     \
			ReturnOverflow();                \
		    if (!(*next++ = *source++))          \
		    	 break;                          \
	        }                                        \
	    }

	    if (data > ED_OWNER)
	        SetIoErr(ERROR_BAD_NUMBER);
	    else
	    {
	    	if (control->eac_LastKey)
	            fib->fib_DiskKey = control->eac_LastKey;

	    	while (ExNext(lock, fib))
	    	{
		    if (control->eac_MatchString &&
			!MatchPatternNoCase(control->eac_MatchString, fib->fib_FileName))
			continue;

		    next = (STRPTR)curr + sizes[data];

		    if (next>end)
		        ReturnOverflow();

		    switch(data)
    		    {
    		    	case ED_OWNER:
			    curr->ed_OwnerUID = fib->fib_OwnerUID;
			    curr->ed_OwnerGID = fib->fib_OwnerGID;

			    /* Fall through */
    		        case ED_COMMENT:
			    curr->ed_Comment = next;
			    CopyStringSafe(fib->fib_Comment);

			    /* Fall through */
    		        case ED_DATE:
			    curr->ed_Days  = fib->fib_Date.ds_Days;
			    curr->ed_Mins  = fib->fib_Date.ds_Minute;
			    curr->ed_Ticks = fib->fib_Date.ds_Tick;

			    /* Fall through */
    		    	case ED_PROTECTION:
			    curr->ed_Prot = fib->fib_Protection;

			    /* Fall through */
    		        case ED_SIZE:
			    curr->ed_Size = fib->fib_Size;

			    /* Fall through */
    		    	case ED_TYPE:
			    curr->ed_Type = fib->fib_DirEntryType;

			    /* Fall through */
    		    	case ED_NAME:
			    curr->ed_Name = next;
			    CopyStringSafe(fib->fib_FileName);

			    /* Fall through */
		    	case 0:
			    curr->ed_Next = (struct ExAllData *)(((IPTR)next + AROS_PTRALIGN - 1) & ~(AROS_PTRALIGN - 1));
		    }

		    if (control->eac_MatchFunc &&
			!AROS_UFC3(LONG, control->eac_MatchFunc,
				   AROS_UFCA(struct Hook *, control->eac_MatchFunc, A0),
				   AROS_UFCA(struct ExAllData *, curr, A2),
				   AROS_UFCA(LONG *, &data, A1)))
			continue;

		    last = curr;
		    curr = curr->ed_Next;
	 	    control->eac_Entries++;
	        }
	    }
	}
end:
	last->ed_Next = NULL;
        control->eac_LastKey = fib->fib_DiskKey;

	if (fib)
	    FreeDosObject(DOS_FIB, fib);

        return IoErr()?DOSFALSE:DOSTRUE;
    }

    /* Set error code and return */
    SetIoErr(iofs.io_DosError);
    return iofs.io_DosError == 0 ? DOSTRUE : DOSFALSE;

    AROS_LIBFUNC_EXIT
} /* ExAll */
