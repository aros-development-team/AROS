/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.3  1996/10/10 13:14:54  digulla
    ? (Fleischer)

    Revision 1.2  1996/09/12 14:52:00  digulla
    Use correct way to access external names (was missing)

    Revision 1.1  1996/09/11 12:52:54  digulla
    Two new devices by M. Fleischer: RAM: and NIL:

    Desc:
    Lang:
*/
#include <exec/errors.h>
#include <exec/resident.h>
#include <exec/memory.h>
#include <clib/exec_protos.h>
#include <utility/tagitem.h>
#include <dos/dosextens.h>
#include <dos/filesystem.h>
#include <clib/dos_protos.h>
#include <aros/libcall.h>
#ifdef __GNUC__
    #include "nil_handler_gcc.h"
#endif

static const char name[];
static const char version[];
static const APTR inittabl[4];
static void *const functable[];
struct nilbase *__AROS_SLIB_ENTRY(init,nil_handler)();
void __AROS_SLIB_ENTRY(open,nil_handler)();
BPTR __AROS_SLIB_ENTRY(close,nil_handler)();
BPTR __AROS_SLIB_ENTRY(expunge,nil_handler)();
int __AROS_SLIB_ENTRY(null,nil_handler)();
void __AROS_SLIB_ENTRY(beginio,nil_handler)();
LONG __AROS_SLIB_ENTRY(abortio,nil_handler)();
static const char end;

int nil_handler_entry(void)
{
    /* If the handler was executed by accident return error code. */
    return -1;
}

const struct Resident nil_handler_resident=
{
    RTC_MATCHWORD,
    (struct Resident *)&nil_handler_resident,
    (APTR)&end,
    RTF_AUTOINIT,
    1,
    NT_DEVICE,
    0,
    (char *)name,
    (char *)&version[6],
    (ULONG *)inittabl
};

static const char name[]="nil.handler";

static const char version[]="$VER: nil_handler 1.0 (8.6.96)\n\015";

static const APTR inittabl[4]=
{
    (APTR)sizeof(struct nilbase),
    (APTR)functable,
    NULL,
    &__AROS_SLIB_ENTRY(init,nil_handler)
};

static void *const functable[]=
{
    &__AROS_SLIB_ENTRY(open,nil_handler),
    &__AROS_SLIB_ENTRY(close,nil_handler),
    &__AROS_SLIB_ENTRY(expunge,nil_handler),
    &__AROS_SLIB_ENTRY(null,nil_handler),
    &__AROS_SLIB_ENTRY(beginio,nil_handler),
    &__AROS_SLIB_ENTRY(abortio,nil_handler),
    (void *)-1
};

__AROS_LH2(struct nilbase *, init,
 __AROS_LHA(struct nilbase *, nilbase, D0),
 __AROS_LHA(BPTR,             segList, A0),
	   struct ExecBase *, sysBase, 0, nil_handler)
{
    __AROS_FUNC_INIT

    /* Store arguments */
    nilbase->sysbase=sysBase;
    nilbase->seglist=segList;
    nilbase->dosbase=(struct DosLibrary *)OpenLibrary("dos.library",39);
    if(nilbase->dosbase!=NULL)
	return nilbase;

    return NULL;
    __AROS_FUNC_EXIT
}

__AROS_LH3(void, open,
 __AROS_LHA(struct IOFileSys *, iofs, A1),
 __AROS_LHA(ULONG,              unitnum, D0),
 __AROS_LHA(ULONG,              flags, D0),
	   struct nilbase *, nilbase, 1, nil_handler)
{
    __AROS_FUNC_INIT
    ULONG *dev;

    /* Get compiler happy */
    unitnum=flags=0;

    /* I have one more opener. */
    nilbase->device.dd_Library.lib_OpenCnt++;

    /* Mark Message as recently used. */
    iofs->IOFS.io_Message.mn_Node.ln_Type=NT_REPLYMSG;

    dev=AllocMem(sizeof(ULONG),MEMF_PUBLIC|MEMF_CLEAR);
    if(dev!=NULL)
    {
        iofs->IOFS.io_Unit=(struct Unit *)dev;
        iofs->IOFS.io_Device=&nilbase->device;
    	nilbase->device.dd_Library.lib_Flags&=~LIBF_DELEXP;
    	iofs->IOFS.io_Error=0;
    	return;
    }else
	iofs->io_DosError=ERROR_NO_FREE_STORE;
	
    iofs->IOFS.io_Error=IOERR_OPENFAIL;
    nilbase->device.dd_Library.lib_OpenCnt--;

    __AROS_FUNC_EXIT
}

__AROS_LH1(BPTR, close,
 __AROS_LHA(struct IOFileSys *, iofs, A1),
	   struct nilbase *, nilbase, 2, nil_handler)
{
    __AROS_FUNC_INIT
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
    if(!--nilbase->device.dd_Library.lib_OpenCnt)
    {
	/* Delayed expunge pending? */
	if(nilbase->device.dd_Library.lib_Flags&LIBF_DELEXP)
	    /* Then expunge the device */
	    return expunge();
    }
    return 0;
    __AROS_FUNC_EXIT
}

__AROS_LH0(BPTR, expunge, struct nilbase *, nilbase, 3, nil_handler)
{
    __AROS_FUNC_INIT

    BPTR ret;
    /*
	This function is single-threaded by exec by calling Forbid.
	Never break the Forbid() or strange things might happen.
    */

    /* Test for openers. */
    if(nilbase->device.dd_Library.lib_OpenCnt)
    {
	/* Set the delayed expunge flag and return. */
	nilbase->device.dd_Library.lib_Flags|=LIBF_DELEXP;
	return 0;
    }

    /* Free all resources */
    CloseLibrary((struct Library *)nilbase->dosbase);

    /* Get rid of the device. Remove it from the list. */
    Remove(&nilbase->device.dd_Library.lib_Node);

    /* Get returncode here - FreeMem() will destroy the field. */
    ret=nilbase->seglist;

    /* Free the memory. */
    FreeMem((char *)nilbase-nilbase->device.dd_Library.lib_NegSize,
	    nilbase->device.dd_Library.lib_NegSize+nilbase->device.dd_Library.lib_PosSize);

    return ret;
    __AROS_FUNC_EXIT
}

__AROS_LH0I(int, null, struct nilbase *, nilbase, 4, nil_handler)
{
    __AROS_FUNC_INIT
    return 0;
    __AROS_FUNC_EXIT
}

__AROS_LH1(void, beginio,
 __AROS_LHA(struct IOFileSys *, iofs, A1),
	   struct nilbase *, nilbase, 5, nil_handler)
{
    __AROS_FUNC_INIT
    LONG error=0;

    /*
	Do everything quick no matter what. This is possible
	because I never need to Wait().
    */
    switch(iofs->IOFS.io_Command)
    {
	case FSA_OPEN:
	case FSA_OPEN_FILE:
	    /* No names allowed on NIL: */
	    if(((STRPTR)iofs->io_Args[0])[0])
	    {
		error=ERROR_OBJECT_NOT_FOUND;
		break;
	    }
	    Forbid();
	    ++*(ULONG *)iofs->IOFS.io_Unit;
	    Permit();
	    break;

	case FSA_READ:
	    iofs->io_Args[1]=0;
	    break;

	case FSA_WRITE:
	    break;

	case FSA_SEEK:
	    iofs->io_Args[0]=0;
	    iofs->io_Args[1]=0;
	    break;

	case FSA_CLOSE:
	    Forbid();
	    --*(ULONG *)iofs->IOFS.io_Unit;
	    Permit();
	    break;

	default:
	    error=ERROR_NOT_IMPLEMENTED;
	    break;
    }

    /* Set error code */
    iofs->io_DosError=error;

    /* If the quick bit is not set send the message to the port */
    if(!(iofs->IOFS.io_Flags&IOF_QUICK))
	ReplyMsg(&iofs->IOFS.io_Message);

    __AROS_FUNC_EXIT
}

__AROS_LH1(LONG, abortio,
 __AROS_LHA(struct IOFileSys *, iofs, A1),
	   struct nilbase *, nilbase, 6, nil_handler)
{
    __AROS_FUNC_INIT
    /* Everything already done. */
    return 0;
    __AROS_FUNC_EXIT
}

static const char end=0;
