/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.4  1996/09/11 14:03:56  digulla
    Quick hack to make it work again.

    Revision 1.3  1996/08/13 14:11:54  digulla
    Replaced __AROS_LA by __AROS_LHA

    Revision 1.2  1996/08/01 17:41:42  digulla
    Added standard header for all files

    Desc:
    Lang:
*/
#include <exec/resident.h>
#include <clib/exec_protos.h>
#include <aros/libcall.h>
#include <dos/dos.h>
#include "utility_intern.h"
/* #include <utility/utility.h> */

static const char name[];
static const char version[];
static const APTR inittabl[4];
static void *const Utility_functable[];
struct UtilityBase *Utility_init();
extern const char Utility_end;

int Utility_entry(void)
{
    /* If the library was executed by accident return error code. */
    return -1;
}

const struct Resident Utility_resident=
{
    RTC_MATCHWORD,
    (struct Resident *)&Utility_resident,
    (APTR)&Utility_end,
    RTF_AUTOINIT,
    39,
    NT_LIBRARY,
    0,
    (char *)name,
    (char *)&version[6],
    (ULONG *)inittabl
};

static const char name[]=UTILITYNAME;

static const char version[]="$VER: utility 39.0 (5.6.96)\n\015";

static const APTR inittabl[4]=
{
    (APTR)sizeof(struct UtilityBase),
    (APTR)Utility_functable,
    NULL,
    &Utility_init
};

#ifdef SysBase
#undef SysBase
#endif
#define SysBase UtilityBase->ub_SysBase

__AROS_LH2(struct UtilityBase *, init,
 __AROS_LHA(struct UtilityBase *, UtilityBase, D0),
 __AROS_LHA(BPTR,               segList,   A0),
	   struct ExecBase *, sysBase, 0, Utility)
{
    __AROS_FUNC_INIT

    /* Store arguments */
    UtilityBase->ub_SysBase=sysBase;
    UtilityBase->ub_SegList=segList;

    /* You would return NULL if the init failed */
    return UtilityBase;
    __AROS_FUNC_EXIT
}

__AROS_LH1(struct UtilityBase *, open,
 __AROS_LHA(ULONG, version, D0),
	   struct UtilityBase *, UtilityBase, 1, Utility)
{
    __AROS_FUNC_INIT

    /* Keep compiler happy */
    version=0;

    /* I have one more opener. */
    UtilityBase->ub_LibNode.lib_OpenCnt++;
    UtilityBase->ub_LibNode.lib_Flags&=~LIBF_DELEXP;

    /* You would return NULL if the open failed. */
    return UtilityBase;
    __AROS_FUNC_EXIT
}

__AROS_LH0(BPTR, close,
	   struct UtilityBase *, UtilityBase, 2, Utility)
{
    __AROS_FUNC_INIT

    /* I have one fewer opener. */
    if(!--UtilityBase->ub_LibNode.lib_OpenCnt)
    {
	/* Delayed expunge pending? */
	if(UtilityBase->ub_LibNode.lib_Flags&LIBF_DELEXP)
	    /* Then expunge the library */
	    return expunge();
    }
    return 0;
    __AROS_FUNC_EXIT
}

__AROS_LH0(BPTR, expunge,
	   struct UtilityBase *, UtilityBase, 3, Utility)
{
    __AROS_FUNC_INIT

    BPTR ret;

    /* Test for openers. */
    if(UtilityBase->ub_LibNode.lib_OpenCnt)
    {
	/* Set the delayed expunge flag and return. */
	UtilityBase->ub_LibNode.lib_Flags|=LIBF_DELEXP;
	return 0;
    }

    /* Get rid of the library. Remove it from the list. */
    Remove(&UtilityBase->ub_LibNode.lib_Node);

    /* Get returncode here - FreeMem() will destroy the field. */
    ret=UtilityBase->ub_SegList;

    /* Free the memory. */
    FreeMem((char *)UtilityBase-UtilityBase->ub_LibNode.lib_NegSize,
	    UtilityBase->ub_LibNode.lib_NegSize+UtilityBase->ub_LibNode.lib_PosSize);

    return ret;
    __AROS_FUNC_EXIT
}

__AROS_LH0I(int, null,
	    struct UtilityBase *, UtilityBase, 4, Utility)
{
    __AROS_FUNC_INIT
    return 0;
    __AROS_FUNC_EXIT
}
