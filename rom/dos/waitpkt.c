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

	AROS_LH0(struct DosPacket *, WaitPkt,

/*  SYNOPSIS */
	/* void */

/*  LOCATION */
	struct DosLibrary *, DOSBase, 42, Dos)

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

    struct Process * me = (struct Process *)FindTask(NULL);
    struct IOFileSys * iofs;
    struct DosPacket * dp;
    
    if (me->pr_PktWait)
    {
#if 0
      dp = me->pr_PktWait();
#endif
    }
    else
    {
      WaitPort(&me->pr_MsgPort);
      iofs = (struct IOFileSys *)GetMsg(&me->pr_MsgPort);
//    dp = iofs->IOFS.io_oldpacket;
    }
    

    /*
    ** The Result code 1 is most of the time a DOS boolean
    ** but unfortunately this is not always true...
    */

#warning do_Res1 is always DOSTRUE!
    dp->dp_Res1 = DOSTRUE;
    dp->dp_Res2 = iofs->io_DosError;
    SetIoErr(iofs->io_DosError);
    
    FreeMem(iofs, sizeof(struct IOFileSys));    
    
    return dp;

    AROS_LIBFUNC_EXIT
} /* WaitPkt */
