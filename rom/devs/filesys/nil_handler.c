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
struct nilbase *nil_handler_init();
void nil_handler_open();
BPTR nil_handler_close();
BPTR nil_handler_expunge();
int nil_handler_null();
void nil_handler_beginio();
LONG nil_handler_abortio();
static const char end;

struct device
{
    struct DosList *doslist;
    ULONG usecount;
};

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
    &nil_handler_init
};

static void *const functable[]=
{
    &nil_handler_open,
    &nil_handler_close,
    &nil_handler_expunge,
    &nil_handler_null,
    &nil_handler_beginio,
    &nil_handler_abortio,
    (void *)-1
};

__AROS_LH2(struct nilbase *, init,
 __AROS_LA(struct nilbase *, nilbase, D0),
 __AROS_LA(BPTR,             segList, A0),
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
 __AROS_LA(struct IOFileSys *, iofs, A1),
 __AROS_LA(ULONG,              unitnum, D0),
 __AROS_LA(ULONG,              flags, D0),
	   struct nilbase *, nilbase, 1, nil_handler)
{
    __AROS_FUNC_INIT

    /* Keep compiler happy */
    unitnum=0;
    flags=0;

    /* I have one more opener. */
    nilbase->device.dd_Library.lib_OpenCnt++;
    nilbase->device.dd_Library.lib_Flags&=~LIBF_DELEXP;

    /* Set returncode */
    iofs->IOFS.io_Error=0;

    /* Mark Message as recently used. */
    iofs->IOFS.io_Message.mn_Node.ln_Type=NT_REPLYMSG;
    __AROS_FUNC_EXIT
}

__AROS_LH1(BPTR, close,
 __AROS_LA(struct IOFileSys *, iofs, A1),
	   struct nilbase *, nilbase, 2, nil_handler)
{
    __AROS_FUNC_INIT

    /* Let any following attemps to use the device crash hard. */
    iofs->IOFS.io_Device=(struct Device *)-1;
    
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
 __AROS_LA(struct IOFileSys *, iofs, A1),
	   struct nilbase *, nilbase, 5, nil_handler)
{
    __AROS_FUNC_INIT
    LONG error=0;
    struct device *dev;
    struct DosList *dl;

    /*
	Do everything quick no matter what. This is possible
	because I never need to Wait().
    */
    switch(iofs->IOFS.io_Command)
    {
	case FSA_STARTUP:
	    /* AddDosEntry() may Wait(), so return error code if necessary */
	    if(!(iofs->IOFS.io_Flags&IOF_QUICK))
	    {
		error=ERROR_NOT_IMPLEMENTED;
		break;
	    }
	    dev=AllocMem(sizeof(struct device),MEMF_PUBLIC|MEMF_CLEAR);
	    if(dev!=NULL)
	    {
		dl=MakeDosEntry((STRPTR)iofs->io_Args[0],DLT_DEVICE);
		if(dl==NULL)
		{
		    dl->dol_Unit=(struct Unit *)dev;
		    dl->dol_Device=&nilbase->device;
		    dev->doslist=dl;
		    if(AddDosEntry(dl))
			break;
		    else
			error=ERROR_OBJECT_EXISTS;
		    FreeDosEntry(dl);
		}else
		    error=ERROR_NO_FREE_STORE;
		FreeMem(dev,sizeof(struct device));
	    }
	    else
		error=ERROR_NO_FREE_STORE;
	    break;

	case FSA_DIE:
	    /* RemDosEntry() may wait, so return error code if necessary */
	    if(!(iofs->IOFS.io_Flags&IOF_QUICK))
	    {
		error=ERROR_NOT_IMPLEMENTED;
		break;
	    }
	    LockDosList(LDF_DEVICES|LDF_WRITE);
	    dev=(struct device *)iofs->io_Args[0];
	    if(dev->usecount==1)
	    {
		RemDosEntry(dev->doslist);
		FreeDosEntry(dev->doslist);
		FreeMem(dev,sizeof(struct device));
	    }else
	    {
		Disable();
		dev->usecount--;
		Enable();
		error=ERROR_OBJECT_IN_USE;
	    }
	    UnLockDosList(LDF_DEVICES|LDF_WRITE);
	    break;
	    
	case FSA_LOCATE_OBJECT:
	    /* No names allowed on NIL: */
	    if(((STRPTR)iofs->io_Args[0])[0])
	    {
		error=ERROR_OBJECT_NOT_FOUND;
		break;
	    }
	    Disable();
	    ((struct device *)iofs->IOFS.io_Unit)->usecount++;
	    Enable();
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
		    
	case FSA_FREE_LOCK:
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
 __AROS_LA(struct IOFileSys *, iofs, A1),
	   struct nilbase *, nilbase, 6, nil_handler)
{
    __AROS_FUNC_INIT
    /* Everything already done. */
    return 0;
    __AROS_FUNC_EXIT
}

static const char end=0;
