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
#include <aros/asmcall.h>

#include "zero_handler_gcc.h"


#include <string.h>

static const char name[];
static const char version[];
static const APTR inittabl[4];
static void *const functable[];
struct zerobase *AROS_SLIB_ENTRY(init,zero_handler)();
void AROS_SLIB_ENTRY(open,zero_handler)();
BPTR AROS_SLIB_ENTRY(close,zero_handler)();
BPTR AROS_SLIB_ENTRY(expunge,zero_handler)();
int AROS_SLIB_ENTRY(null,zero_handler)();
void AROS_SLIB_ENTRY(beginio,zero_handler)();
LONG AROS_SLIB_ENTRY(abortio,zero_handler)();
static const char end;

int entry(void)
{
    /* If the handler was executed by accident return error code. */
    return -1;
}

const struct Resident zero_handler_resident=
{
    RTC_MATCHWORD,
    (struct Resident *)&zero_handler_resident,
    (APTR)&end,
    RTF_AUTOINIT,
    41,
    NT_DEVICE,
    0,
    (char *)name,
    (char *)&version[6],
    (ULONG *)inittabl
};

static const char name[]="zero.handler";

static const char version[]="$VER: zero-handler 41.1 (2.9.2001)\r\n";

static const APTR inittabl[4]=
{
    (APTR)sizeof(struct zerobase),
    (APTR)functable,
    NULL,
    &AROS_SLIB_ENTRY(init,zero_handler)
};

static void *const functable[]=
{
    &AROS_SLIB_ENTRY(open,zero_handler),
    &AROS_SLIB_ENTRY(close,zero_handler),
    &AROS_SLIB_ENTRY(expunge,zero_handler),
    &AROS_SLIB_ENTRY(null,zero_handler),
    &AROS_SLIB_ENTRY(beginio,zero_handler),
    &AROS_SLIB_ENTRY(abortio,zero_handler),
    (void *)-1
};

AROS_UFH3(struct zerobase *, AROS_SLIB_ENTRY(init,zero_handler),
 AROS_UFHA(struct zerobase *, zerobase, D0),
 AROS_UFHA(BPTR,             segList, A0),
 AROS_UFHA(struct ExecBase *, sysBase, A6)
)
{
    AROS_USERFUNC_INIT

    /* Store arguments */
    zerobase->sysbase=sysBase;
    zerobase->seglist=segList;
    zerobase->dosbase=(struct DosLibrary *)OpenLibrary("dos.library",39);
    if(zerobase->dosbase!=NULL)
	return zerobase;

    return NULL;
    AROS_USERFUNC_EXIT
}

AROS_LH3(void, open,
 AROS_LHA(struct IOFileSys *, iofs, A1),
 AROS_LHA(ULONG,              unitnum, D0),
 AROS_LHA(ULONG,              flags, D1),
	   struct zerobase *, zerobase, 1, zero_handler)
{
    AROS_LIBFUNC_INIT
    ULONG *dev;

    /* Make compiler happy */
    unitnum=flags=0;

    /* I have one more opener. */
    zerobase->device.dd_Library.lib_OpenCnt++;

    /* Mark Message as recently used. */
    iofs->IOFS.io_Message.mn_Node.ln_Type=NT_REPLYMSG;

    dev=AllocMem(sizeof(ULONG),MEMF_PUBLIC|MEMF_CLEAR);
    if(dev!=NULL)
    {
        iofs->IOFS.io_Unit=(struct Unit *)dev;
        iofs->IOFS.io_Device=&zerobase->device;
    	zerobase->device.dd_Library.lib_Flags&=~LIBF_DELEXP;
    	iofs->IOFS.io_Error=0;
    	return;
    }
    else
    {
	iofs->io_DosError=ERROR_NO_FREE_STORE;
    }
	
    iofs->IOFS.io_Error=IOERR_OPENFAIL;
    zerobase->device.dd_Library.lib_OpenCnt--;

    AROS_LIBFUNC_EXIT
}

AROS_LH1(BPTR, close,
 AROS_LHA(struct IOFileSys *, iofs, A1),
	   struct zerobase *, zerobase, 2, zero_handler)
{
    AROS_LIBFUNC_INIT
    ULONG *dev;
   
    dev=(ULONG *)iofs->IOFS.io_Unit;
    if(*dev)
    {
	iofs->io_DosError=ERROR_OBJECT_IN_USE;
	return 0;
    }

    /* Let any following attemps to use the device crash hard. */
    iofs->IOFS.io_Device=(struct Device *)-1;
    FreeMem(dev,sizeof(ULONG));
    iofs->io_DosError=0;

    /* I have one fewer opener. */
    if(!--zerobase->device.dd_Library.lib_OpenCnt)
    {
	/* Delayed expunge pending? */
	if(zerobase->device.dd_Library.lib_Flags&LIBF_DELEXP)
	    /* Then expunge the device */
	    return expunge();
    }
    return 0;
    AROS_LIBFUNC_EXIT
}

AROS_LH0(BPTR, expunge, struct zerobase *, zerobase, 3, zero_handler)
{
    AROS_LIBFUNC_INIT

    BPTR ret;
    /*
	This function is single-threaded by exec by calling Forbid.
	Never break the Forbid() or strange things might happen.
    */

    /* Test for openers. */
    if(zerobase->device.dd_Library.lib_OpenCnt)
    {
	/* Set the delayed expunge flag and return. */
	zerobase->device.dd_Library.lib_Flags|=LIBF_DELEXP;
	return 0;
    }

    /* Free all resources */
    CloseLibrary((struct Library *)zerobase->dosbase);

    /* Get rid of the device. Remove it from the list. */
    Remove(&zerobase->device.dd_Library.lib_Node);

    /* Get returncode here - FreeMem() will destroy the field. */
    ret=zerobase->seglist;

    /* Free the memory. */
    FreeMem((char *)zerobase-zerobase->device.dd_Library.lib_NegSize,
	    zerobase->device.dd_Library.lib_NegSize+zerobase->device.dd_Library.lib_PosSize);

    return ret;
    AROS_LIBFUNC_EXIT
}

AROS_LH0I(int, null, struct zerobase *, zerobase, 4, zero_handler)
{
    AROS_LIBFUNC_INIT
    return 0;
    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, beginio,
 AROS_LHA(struct IOFileSys *, iofs, A1),
	   struct zerobase *, zerobase, 5, zero_handler)
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
	    if
	    (
	       stricmp(iofs->io_Union.io_NamedFile.io_Filename, "ZERO:") != 0 &&
	       iofs->io_Union.io_NamedFile.io_Filename[0]
            )
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
	   struct zerobase *, zerobase, 6, zero_handler)
{
    AROS_LIBFUNC_INIT
    /* Everything already done. */
    return 0;
    AROS_LIBFUNC_EXIT
}

static const char end=0;
