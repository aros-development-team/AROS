/*
    Copyright � 1995-2009, The AROS Development Team. All rights reserved.
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

#include <stddef.h>
#include <ctype.h>
#include <sys/stat.h>
#include <setjmp.h>
#include <assert.h>

#define DEBUG 0
#include <aros/debug.h>

#include "etask.h"
#include "__arosc_privdata.h"
#include "arosc_init.h"

THIS_PROGRAM_HANDLES_SYMBOLSETS

extern const char name[];
extern const char version[];
extern const APTR inittabl[4];
extern void *const arosc_functable[];
extern const struct inittable datatable;
extern struct aroscbase *AROS_SLIB_ENTRY(init,arosc)();
AROS_LD1(struct aroscbase *, open, AROS_LDA(ULONG, version, D0), struct aroscbase *, aroscbase, 1, arosc);
AROS_LD0(BPTR, close, struct aroscbase *, aroscbase, 2, arosc);
AROS_LD0(BPTR, expunge, struct aroscbase *, aroscbase, 3, arosc);
AROS_LD0I(int, null, struct aroscbase *, aroscbase, 4, arosc);
extern const char arosc_end;

struct ExecBase  *SysBase;
struct aroscbase *aroscbase;

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
    42,
    NT_LIBRARY,
    0,
    (char *)name,
    (char *)&version[6],
    (ULONG *)inittabl
};

const char name[]="arosc.library";

const char version[]="$VER: arosc.library 42.0 (" __DATE__ ")\n\015";

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
 AROS_UFHA(struct aroscbase *, __aroscbase, D0),
 AROS_UFHA(BPTR,               segList,   A0),
 AROS_UFHA(struct ExecBase *,  __SysBase,   A6)
)
{
    AROS_USERFUNC_INIT

    /* This function is single-threaded by exec by calling Forbid. */

    /* Store arguments */
    aroscbase = __aroscbase;
    SysBase   = __SysBase;

    aroscbase->seglist=segList;

    if (!set_open_libraries())
    {
        set_close_libraries();
        
	return NULL;
    }

    /* You would return NULL here if the init failed. */
    return aroscbase;
    AROS_USERFUNC_EXIT
}


AROS_LH1(struct aroscbase *, open,
 AROS_LHA(ULONG, version, D0),
	   struct aroscbase *, aroscbase, 1, arosc)
{
    AROS_LIBFUNC_INIT

    struct Task *me = FindTask(NULL);
    /*
	This function is single-threaded by exec by calling Forbid.
	If you break the Forbid() another task may enter this function
	at the same time. Take care.
    */
    if (!(me->tc_Flags & TF_ETASK) || GetIntETask(me)==NULL) {
    	/* No ETask? No arosc.library.
    	 */
    	return NULL;
    }

    /* Keep compiler happy */
    version=0;

    if (!arosc_internalinit())
    {
        arosc_internalexit();
        return NULL;
    }

    /* I have one more opener. */
    aroscbase->library.lib_OpenCnt++;
    aroscbase->library.lib_Flags &= ~LIBF_DELEXP;
    
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

    arosc_internalexit();

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

int arosc_internalinit(void)
{
    struct arosc_privdata *oldprivdata, *privdata;
    struct Process *me = (struct Process *)FindTask(NULL);
    int res = TRUE, makenew = FALSE;

    privdata = oldprivdata = GetIntETask(me)->iet_acpd;

    D(bug("\nEntering arosc_internalinit(): me(%x)->name = %s\n",
          me, me->pr_Task.tc_Node.ln_Name
    ));
    D(bug("arosc_internalinit(): oldprivdata = %p\n", oldprivdata));

    if (!oldprivdata)
    {
        /* This process does not have arosc_privdata. Check parent */
        struct arosc_privdata * parentprivdata = __get_arosc_privdata();

        if (parentprivdata)
        {
            /* Parent has arosc_privdata. Is is shared it with this process? */
            if (parentprivdata->acpd_flags & SHARE_ACPD_WITH_CHILD)
            {
                /* Yes. Do nothing. (reuse parents arosc_privdata) */
                return res;
            }
        }
    }

    if (!oldprivdata)
    {
        makenew = TRUE;
    }
    else
    {
        assert(!((oldprivdata->acpd_flags & CREATE_NEW_ACPD)
                 && (oldprivdata->acpd_flags & KEEP_OLD_ACPD)
                )
        );
        if (oldprivdata->acpd_flags & CREATE_NEW_ACPD)
        {
            makenew = TRUE;
            oldprivdata->acpd_flags &= ~CREATE_NEW_ACPD;
        }
        else if (oldprivdata->acpd_flags & KEEP_OLD_ACPD)
        {
            /* Currently KEEP_OLD_ACPD should only be set just after opening a fresh
               aroscbase and then running the child, arosc_internalexit() depends on this
            */
            assert(oldprivdata->acpd_usercount == 1);
            oldprivdata->acpd_flags &= ~KEEP_OLD_ACPD;
            oldprivdata->acpd_flags |= ACPD_FROM_PARENT;
            oldprivdata->acpd_process_returnaddr = me->pr_ReturnAddr;
        }
        else
        {
            makenew = oldprivdata->acpd_process_returnaddr != me->pr_ReturnAddr;
        }
    }
    
    if (makenew)
    {
        D(bug("arosc_internalinit(): AllocMem()\n"));
        privdata = AllocMem(sizeof *privdata, MEMF_CLEAR|MEMF_ANY);

        if (!privdata)
        {
            SetIoErr(ERROR_NO_FREE_STORE);
            return FALSE;
        }

        D(bug("arosc_internalinit(): newprivdata = %p\n", privdata));
        privdata->acpd_oldprivdata = oldprivdata;
	privdata->acpd_process_returnaddr = me->pr_ReturnAddr;

	GetIntETask(me)->iet_acpd = privdata;

        res = set_call_funcs(SETNAME(INIT), 1, 1);
    }

    D(bug("arosc_internalinit(): acpd_usercount=%d\n", privdata->acpd_usercount));
    privdata->acpd_usercount++;

    D(bug("Exiting arosc_internalinit(): me(%x)->name = %s, acpd_usercount = %d\n\n",
          FindTask(NULL), FindTask(NULL)->tc_Node.ln_Name, privdata->acpd_usercount
    ));

    return res;
}

int arosc_internalexit(void)
{
    struct arosc_privdata *privdata = GetIntETask(FindTask(NULL))->iet_acpd;

    D(bug("\nEntering arosc_internalexit(): me(%x)->name = %s\n",
          FindTask(NULL), FindTask(NULL)->tc_Node.ln_Name
    ));

    if (!privdata)
    {
        /* This process does not have priv data. Check if parent has */
        struct arosc_privdata * parentprivdata = __get_arosc_privdata();

        if (parentprivdata)
        {
            /* Parent has arosc_privdata. Is is shared it with this process? */
            if (parentprivdata->acpd_flags & SHARE_ACPD_WITH_CHILD)
            {
                /* Yes. Do nothing. */
                return 0;
            }
        }
        /* Hosted FreeBSD won't run without that */
        privdata = parentprivdata;
    }

    D(bug("arosc_internalexit(): --acpd_usercount\n"));

    ASSERT_VALID_PTR(privdata);
    --privdata->acpd_usercount;

    D(bug("arosc_internalexit(): acpd_usercount = %d\n", privdata->acpd_usercount));

    /* If ACPD_FROM_PARENT is set, the parent has done an OpenLibrary() and
       will do a final CloseLibrary(); but the child should call the EXIT
       functions when the last CloseLibrary is called there; otherwise
       atexit functions will be called after client has finished
    */
    if (privdata->acpd_usercount ==
           ((privdata->acpd_flags & ACPD_FROM_PARENT) ? 1 : 0)
    )
    {
        set_call_funcs(SETNAME(EXIT), -1, 0);
    }

    if (privdata->acpd_usercount == 0)
    {
        /*restore the old value */
        GetIntETask(FindTask(NULL))->iet_acpd = privdata->acpd_oldprivdata;

        D(bug("arosc_internalexit(): FreeMem()\n"));
        FreeMem(privdata, sizeof(*privdata));
    }

    D(bug("Exiting arosc_internalexit(): me(%x)->name = %s\n\n",
          FindTask(NULL), FindTask(NULL)->tc_Node.ln_Name
    ));

    return 0;
}

DEFINESET(INIT);
DEFINESET(EXIT);
