/*
    Copyright © 1995-2002, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/errors.h>
#include <exec/types.h>
#include <exec/resident.h>
#include <clib/exec_protos.h>
#include <aros/libcall.h>
#ifdef __GNUC__
#   include "dummydev_gcc.h"
#endif
#include "initstruct.h"

struct inittable;
extern const char name[];
extern const char version[];
extern const APTR inittabl[4];
extern void *const functable[];
extern const struct inittable datatable;
extern struct dummybase *dummy_init();
extern void dummy_open();
extern BPTR dummy_close();
extern BPTR dummy_expunge();
extern int dummy_null();
extern void dummy_beginio();
extern LONG dummy_abortio();
extern const char end;

int entry(void)
{
    /* If the device was executed by accident return error code. */
    return -1;
}

const struct Resident resident=
{
    RTC_MATCHWORD,
    (struct Resident *)&resident,
    (APTR)&end,
    RTF_AUTOINIT,
    1,
    NT_LIBRARY,
    0,
    (char *)name,
    (char *)&version[6],
    (ULONG *)inittabl
};

const char name[]="dummy.device";

const char version[]="$VER: dummy 1.0 (28.3.96)\n\015";

const APTR inittabl[4]=
{
    (APTR)sizeof(struct dummybase),
    (APTR)functable,
    (APTR)&datatable,
    &dummy_init
};

void *const functable[]=
{
    &dummy_open,
    &dummy_close,
    &dummy_expunge,
    &dummy_null,
    &dummy_beginio,
    &dummy_abortio,
    (void *)-1
};

struct inittable
{
    S_CPYO(1,1,B);
    S_CPYO(2,1,L);
    S_CPYO(3,1,B);
    S_CPYO(4,1,W);
    S_CPYO(5,1,W);
    S_CPYO(6,1,L);
    S_END (end);
};

#define O(n) offsetof(struct dummybase,n)

const struct inittable datatable=
{
    { { I_CPYO(1,B,O(device.dd_Library.lib_Node.ln_Type)), { NT_LIBRARY } } },
    { { I_CPYO(1,L,O(device.dd_Library.lib_Node.ln_Name)), { (IPTR)name } } },
    { { I_CPYO(1,B,O(device.dd_Library.lib_Flags       )), { LIBF_SUMUSED|LIBF_CHANGED } } },
    { { I_CPYO(1,W,O(device.dd_Library.lib_Version     )), { 1 } } },
    { { I_CPYO(1,W,O(device.dd_Library.lib_Revision    )), { 0 } } },
    { { I_CPYO(1,L,O(device.dd_Library.lib_IdString    )), { (IPTR)&version[6] } } },
	I_END ()
};

#undef O

AROS_LH2(struct dummybase *, init,
 AROS_LA(struct dummybase *, dummybase, D0),
 AROS_LA(BPTR,               segList,   A0),
	   struct ExecBase *, SysBase, 0, dummy)
{
    AROS_LIBFUNC_INIT
    /* This function is single-threaded by exec by calling Forbid. */

    /* Store arguments */
    dummybase->sysbase=SysBase;
    dummybase->seglist=segList;

    /* You would return NULL here if the init failed. */
    return dummybase;
    AROS_LIBFUNC_EXIT
}

/* Use This from now on */
#ifdef SysBase
#undef SysBase
#endif
#define SysBase dummybase->sysbase

AROS_LH3(void, open,
 AROS_LA(struct dummyrequest *, iob, A1),
 AROS_LA(ULONG,                 unitnum, D0),
 AROS_LA(ULONG,                 flags, D0),
	   struct dummybase *, dummybase, 1, dummy)
{
    AROS_LIBFUNC_INIT
    /*
	This function is single-threaded by exec by calling Forbid.
	If you break the Forbid() another task may enter this function
	at the same time. Take care.
    */

    /* Keep compiler happy */
    unitnum=0;
    flags=0;

    /*
	Normally you'd init the unit and set the unit field here -
	but this is only a dummy without child tasks.
    */

    /* I have one more opener. */
    dummybase->device.dd_Library.lib_OpenCnt++;
    dummybase->device.dd_Library.lib_Flags&=~LIBF_DELEXP;

    /* Set returncode */
    iob->iorequest.io_Error=0;

    /* Mark Message as recently used. */
    iob->iorequest.io_Message.mn_Node.ln_Type=NT_REPLYMSG;
    AROS_LIBFUNC_EXIT
}

AROS_LH1(BPTR, close,
 AROS_LA(struct dummyrequest *, iob, A1),
	   struct dummybase *, dummybase, 2, dummy)
{
    AROS_LIBFUNC_INIT
    /*
	This function is single-threaded by exec by calling Forbid.
	If you break the Forbid() another task may enter this function
	at the same time. Take care.
    */

    /* Let any following attemps to use the device crash hard. */
    iob->iorequest.io_Device=(struct Device *)-1;

    /* I have one fewer opener. */
    if(!--dummybase->device.dd_Library.lib_OpenCnt)
    {
	/* Delayed expunge pending? */
	if(dummybase->device.dd_Library.lib_Flags&LIBF_DELEXP)
	    /* Then expunge the device */
	    return expunge();
    }
    return 0;
    AROS_LIBFUNC_EXIT
}

AROS_LH0(BPTR, expunge, struct dummybase *, dummybase, 3, dummy)
{
    AROS_LIBFUNC_INIT

    BPTR ret;
    /*
	This function is single-threaded by exec by calling Forbid.
	Never break the Forbid() or strange things might happen.
    */

    /* Test for openers. */
    if(dummybase->device.dd_Library.lib_OpenCnt)
    {
	/* Set the delayed expunge flag and return. */
	dummybase->device.dd_Library.lib_Flags|=LIBF_DELEXP;
	return 0;
    }

    /* Get rid of the device. Remove it from the list. */
    Remove(&dummybase->device.dd_Library.lib_Node);

    /* Get returncode here - FreeMem() will destroy the field. */
    ret=dummybase->seglist;

    /* Free the memory. */
    FreeMem((char *)dummybase-dummybase->device.dd_Library.lib_NegSize,
	    dummybase->device.dd_Library.lib_NegSize+dummybase->device.dd_Library.lib_PosSize);

    return ret;
    AROS_LIBFUNC_EXIT
}
AROS_LH0I(int, null, struct dummybase *, dummybase, 4, dummy)
{
    AROS_LIBFUNC_INIT
    return 0;
    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, beginio,
 AROS_LA(struct dummyrequest *, iob, A1),
	   struct dummybase *, dummybase, 5, dummy)
{
    AROS_LIBFUNC_INIT

    /* WaitIO will look into this */
    iob->iorequest.io_Message.mn_Node.ln_Type=NT_MESSAGE;

    /*
	Dispatch command (the quick bit tells me that it is allowed to
	wait on the user's context. I'll ignore it here and do everything quick.)
    */
    switch(iob->iorequest.io_Command)
    {
	case 0x1:
	    iob->iorequest.io_Error=0;
	    iob->id=++(dummybase->count);
	    break;
	default:
	    iob->iorequest.io_Error=IOERR_NOCMD;
	    break;
	/*
	    Commands dispatched to a cild task and processed asynchronbously
	    clear the quick bit before the end of beginio.
	*/
    }

    /*
	The request is finished now - so send it back.
	Note that something that was done quick and had the quick bit set
	doesn't need a ReplyMsg().
    */
    if(!(iob->iorequest.io_Flags&IOF_QUICK))
	ReplyMsg(&iob->iorequest.io_Message);
    AROS_LIBFUNC_EXIT
}

AROS_LH1I(LONG, abortio,
 AROS_LA(struct dummyrequest *, iob, A1),
	   struct dummybase *, dummybase, 6, dummy)
{
    AROS_LIBFUNC_INIT
    /* Get compiler happy */
    iob=0;

    /* Since everything is finished quick nothing needs an abort. */
    return IOERR_NOCMD;
    AROS_LIBFUNC_EXIT
}

const char end=0;

