/*
    (C) 1995-96 AROS - The Amiga Research OS
    $Id$

    Desc:
    Lang: english
*/
#include "dos_intern.h"

/*****************************************************************************

    NAME */
#include <proto/dos.h>

	AROS_LH2(LONG, SameLock,

/*  SYNOPSIS */
	AROS_LHA(BPTR, lock1, D1),
	AROS_LHA(BPTR, lock2, D2),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 70, Dos)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY
	27-11-96    digulla automatically created from
			    dos_lib.fd and clib/dos_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct DosLibrary *,DOSBase)
    
    struct IOFileSys io, *iofs = &io;
    struct FileHandle *fh1;
    struct FileHandle *fh2;
    struct Process *me;

    if (!SameDevice(lock1, lock2))
    	return LOCK_DIFFERENT;
	
    me = (struct Process *)FindTask(NULL);
    fh1 = (struct FileHandle *)BADDR(lock1);
    fh2 = (struct FileHandle *)BADDR(lock2);
	
    /* Check if is the same lock */
    /* Prepare I/O request. */
    iofs->IOFS.io_Message.mn_Node.ln_Type=NT_REPLYMSG;
    iofs->IOFS.io_Message.mn_ReplyPort	 =&me->pr_MsgPort;
    iofs->IOFS.io_Message.mn_Length	 =sizeof(struct IOFileSys);
    iofs->IOFS.io_Device =fh1->fh_Device;
    iofs->IOFS.io_Unit	 =fh1->fh_Unit;
    iofs->IOFS.io_Command=FSA_SAME_LOCK;
    iofs->IOFS.io_Flags  =0;
    iofs->io_Union.io_SAME_LOCK.io_Lock[0] = fh1->fh_Unit;
    iofs->io_Union.io_SAME_LOCK.io_Lock[1] = fh2->fh_Unit;
    iofs->io_Union.io_SAME_LOCK.io_Same = LOCK_DIFFERENT;

    /* Send the request. */
    DoIO(&iofs->IOFS);

    /* Set error code and return */
    if((me->pr_Result2=iofs->io_DosError))
	return LOCK_DIFFERENT;
    else {
	if (iofs->io_Union.io_SAME_LOCK.io_Same == LOCK_SAME)
	    return LOCK_SAME;
	else
	    return LOCK_SAME_VOLUME;
    }

    return 0;
    AROS_LIBFUNC_EXIT
} /* SameLock */
