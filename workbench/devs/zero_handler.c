/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: ZERO: handler
    Lang: English
*/

#include <exec/errors.h>
#include <exec/resident.h>
#include <exec/memory.h>
#include <proto/exec.h>
#include <utility/tagitem.h>
#include <dos/dosextens.h>
#include <dos/filesystem.h>
#include <proto/dos.h>
#include <aros/libcall.h>
#include <aros/symbolsets.h>

#include "zero_handler_gcc.h"

#include LC_LIBDEFS_FILE

#include <string.h>

AROS_SET_LIBFUNC(GM_UNIQUENAME(Init), LIBBASETYPE, zerobase)
{
    AROS_SET_LIBFUNC_INIT

    zerobase->dosbase=(struct DosLibrary *)OpenLibrary("dos.library",39);
    if(zerobase->dosbase!=NULL)
	return TRUE;

    return FALSE;
    AROS_SET_LIBFUNC_EXIT
}

AROS_SET_OPENDEVFUNC(GM_UNIQUENAME(Open),
		     LIBBASETYPE, zerobase,
		     struct IOFileSys, iofs,
		     unitnum,
		     flags
)
{
    AROS_SET_DEVFUNC_INIT
    ULONG *dev;

    /* Mark Message as recently used. */
    iofs->IOFS.io_Message.mn_Node.ln_Type=NT_REPLYMSG;

    dev=AllocMem(sizeof(ULONG),MEMF_PUBLIC|MEMF_CLEAR);
    if(dev!=NULL)
    {
        iofs->IOFS.io_Unit=(struct Unit *)dev;
        iofs->IOFS.io_Device=&zerobase->device;
    	iofs->IOFS.io_Error=0;
    	return TRUE;
    }
    else
    {
	iofs->io_DosError=ERROR_NO_FREE_STORE;
    }
	
    iofs->IOFS.io_Error=IOERR_OPENFAIL;

    return FALSE;
    
    AROS_SET_DEVFUNC_EXIT
}

AROS_SET_CLOSEDEVFUNC(GM_UNIQUENAME(Close),
		      LIBBASETYPE, zerobase,
		      struct IOFileSys, iofs
)
{
    AROS_SET_DEVFUNC_INIT
    ULONG *dev;
   
    dev=(ULONG *)iofs->IOFS.io_Unit;
    if(*dev)
    {
	iofs->io_DosError=ERROR_OBJECT_IN_USE;
	return FALSE;
    }

    /* Let any following attemps to use the device crash hard. */
    FreeMem(dev,sizeof(ULONG));
    iofs->io_DosError=0;

    return TRUE;

    AROS_SET_DEVFUNC_EXIT
}

ADD2INITLIB(GM_UNIQUENAME(Init), 0)
ADD2OPENDEV(GM_UNIQUENAME(Open), 0)
ADD2CLOSEDEV(GM_UNIQUENAME(Close), 0)

AROS_LH1(void, beginio,
 AROS_LHA(struct IOFileSys *, iofs, A1),
	   struct zerobase *, zerobase, 5, Zero)
{
    AROS_LIBFUNC_INIT
    LONG error=0;
    LONG i;

    /*
	Do everything quick no matter what. This is possible
	because I never need to Wait().
    */
    switch(iofs->IOFS.io_Command)
    {
	case FSA_OPEN:
	    Forbid();
	    ++*(ULONG *)iofs->IOFS.io_Unit;
	    Permit();
	    break;

	case FSA_OPEN_FILE:
	    /* No names allowed on ZERO: */
	    if (iofs->io_Union.io_NamedFile.io_Filename[0])
	    {
		error=ERROR_OBJECT_NOT_FOUND;
		break;
	    }
	    break;

	case FSA_READ:
	    for ( i = iofs->io_Union.io_READ.io_Length ; i > 0 ; i-- )
		iofs->io_Union.io_READ.io_Buffer[i-1] = 0;
	    break;

	case FSA_WRITE:
	    break;

	case FSA_SEEK:
	    iofs->io_Union.io_SEEK.io_Offset = 0;
	    break;

	case FSA_CLOSE:
	    Forbid();
	    --*(ULONG *)iofs->IOFS.io_Unit;
	    Permit();
	    break;

	case FSA_IS_INTERACTIVE:
	    iofs->io_Union.io_IS_INTERACTIVE.io_IsInteractive = TRUE;
	    break;
	case FSA_SET_FILE_SIZE:
        case FSA_EXAMINE:
        case FSA_EXAMINE_NEXT:
        case FSA_EXAMINE_ALL:
        case FSA_CREATE_DIR:
        case FSA_CREATE_HARDLINK:
        case FSA_CREATE_SOFTLINK:
        case FSA_RENAME:
        case FSA_DELETE_OBJECT:
            error = ERROR_NOT_IMPLEMENTED;
            break;

	default:
	    error = ERROR_ACTION_NOT_KNOWN;
	    break;
    }

    /* Set error code */
    iofs->io_DosError=error;

    /* If the quick bit is not set send the message to the port */
    if(!(iofs->IOFS.io_Flags&IOF_QUICK))
	ReplyMsg(&iofs->IOFS.io_Message);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(LONG, abortio,
 AROS_LHA(struct IOFileSys *, iofs, A1),
	   struct zerobase *, zerobase, 6, Zero)
{
    AROS_LIBFUNC_INIT
    /* Everything already done. */
    return 0;
    AROS_LIBFUNC_EXIT
}
