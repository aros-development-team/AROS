/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/types.h>
#include <exec/resident.h>
#include <exec/memory.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <aros/libcall.h>
#include <aros/asmcall.h>
#include <aros/symbolsets.h>
#if defined(__GNUC__) || defined(__INTEL_COMPILER)
#   include "arosc_gcc.h"
#endif

#include <libraries/arosc.h>

#include <stddef.h>
#include <ctype.h>
#include <sys/stat.h>

extern const char name[];
extern const char version[];
extern const APTR inittabl[4];
extern void *const arosc_functable[];
extern const struct inittable datatable;
extern struct aroscbase *AROS_SLIB_ENTRY(init,arosc)();
extern struct aroscbase *AROS_SLIB_ENTRY(open,arosc)();
extern BPTR AROS_SLIB_ENTRY(close,arosc)();
extern BPTR AROS_SLIB_ENTRY(expunge,arosc)();
extern int AROS_SLIB_ENTRY(null,arosc)();
extern const char arosc_end;

extern struct ExecBase   *SysBase;
extern struct DosLibrary *DOSBase;

int entry(void)
{
    /* If the library was executed by accident return error code. */
    return -1;
}

const struct Resident resident=
{
    RTC_MATCHWORD,
    (struct Resident *)&resident,
    (APTR)&arosc_end,
    RTF_AUTOINIT,
    41,
    NT_LIBRARY,
    0,
    (char *)name,
    (char *)&version[6],
    (ULONG *)inittabl
};

const char name[]="arosc.library";

const char version[]="$VER: arosc.library 41.1 (28.3.96)\n\015";

const APTR inittabl[4]=
{
    (APTR)sizeof(struct aroscbase),
    (APTR)arosc_functable,
    NULL,
    &AROS_SLIB_ENTRY(init,arosc)
};

DECLARESET(INIT);
DECLARESET(EXIT);

AROS_UFH3(struct aroscbase *, AROS_SLIB_ENTRY(init,arosc),
 AROS_UFHA(struct aroscbase *, aroscbase, D0),
 AROS_UFHA(BPTR,               segList,   A0),
 AROS_UFHA(struct ExecBase *,  sysBase,   A6)
)
{
    AROS_USERFUNC_INIT
    /* This function is single-threaded by exec by calling Forbid. */

    /* Store arguments */
    SysBase = sysBase;
    aroscbase->seglist=segList;

    if (set_open_libraries())
        return NULL;

    /* You would return NULL here if the init failed. */
    return aroscbase;
    AROS_USERFUNC_EXIT
}

struct ExecBase *SysBase;

AROS_LH1(struct aroscbase *, open,
 AROS_LHA(ULONG, version, D0),
	   struct aroscbase *, aroscbase, 1, arosc)
{
    AROS_LIBFUNC_INIT
    /*
	This function is single-threaded by exec by calling Forbid.
	If you break the Forbid() another task may enter this function
	at the same time. Take care.
    */

    /* Keep compiler happy */
    version=0;

    /* I have one more opener. */
    aroscbase->library.lib_OpenCnt++;
    aroscbase->library.lib_Flags&=~LIBF_DELEXP;

    /* You would return NULL if the open failed. */
    return aroscbase;
    AROS_LIBFUNC_EXIT
}

AROS_LH0(BPTR, close, struct aroscbase *, aroscbase, 2, arosc)
{
    AROS_LIBFUNC_INIT
    /*
	This function is single-threaded by exec by calling Forbid.
	If you break the Forbid() another task may enter this function
	at the same time. Take care.
    */

    /* I have one fewer opener. */
    if(!--aroscbase->library.lib_OpenCnt)
    {
	/* Delayed expunge pending? */
	if(aroscbase->library.lib_Flags&LIBF_DELEXP)
	    /* Then expunge the library */
	    return expunge();
    }
    return 0;
    AROS_LIBFUNC_EXIT
}

AROS_LH0(BPTR, expunge, struct aroscbase *, aroscbase, 3, arosc)
{
    AROS_LIBFUNC_INIT

    BPTR ret;
    /*
	This function is single-threaded by exec by calling Forbid.
	Never break the Forbid() or strange things might happen.
    */

    /* Test for openers. */
    if(aroscbase->library.lib_OpenCnt)
    {
	/* Set the delayed expunge flag and return. */
	aroscbase->library.lib_Flags|=LIBF_DELEXP;
	return 0;
    }

    /* Get rid of the library. Remove it from the list. */
    Remove(&aroscbase->library.lib_Node);

    /* Get returncode here - FreeMem() will destroy the field. */
    ret=aroscbase->seglist;

    set_close_libraries();

    /* Free the memory. */
    FreeMem((char *)aroscbase-aroscbase->library.lib_NegSize,
	    aroscbase->library.lib_NegSize+aroscbase->library.lib_PosSize);

    return ret;
    AROS_LIBFUNC_EXIT
}
AROS_LH0I(int, null, struct aroscbase *, aroscbase, 4, arosc)
{
    AROS_LIBFUNC_INIT
    return 0;
    AROS_LIBFUNC_EXIT
}

int arosc_internalinit(struct AroscUserData *userdata)
{
    /*save the old value of tc_UserData */
    userdata->olduserdata = AROSC_USERDATA(0);

    /*store the new one */
    AROSC_USERDATA(0) = userdata;

    /* passes these value to the program */
    userdata->ctype_b       = __ctype_b;
    userdata->ctype_toupper = __ctype_toupper;
    userdata->ctype_tolower = __ctype_tolower;

    if (userdata->olduserdata)
        userdata->umask = userdata->olduserdata->umask;
    else
        userdata->umask = S_IWGRP|S_IWOTH;

    return set_call_funcs(SETNAME(INIT), 1);
}

int arosc_internalexit(void)
{
    struct AroscUserData *userdata = AROSC_USERDATA(0);

    set_call_funcs(SETNAME(EXIT), -1);

    /*restore the old value */
    AROSC_USERDATA(0) = userdata->olduserdata;

    /* Free the memory the program has allocated for us */
    FreeVec(userdata);

    return 0;
}


DEFINESET(INIT);
DEFINESET(EXIT);
