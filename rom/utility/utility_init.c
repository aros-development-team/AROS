/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.7  1997/01/27 00:32:33  ldp
    Polish

    Revision 1.6  1996/10/24 15:49:28  aros
    Use the new "AROS" macros

    Fixed severe bug: The size of the library was "struct UtilityBase" instead
    of "struct IntUtilityBase" which caused a MemCorrupt-Alert.

    Use GetIntUtilityBase() to access the private fields.

    Revision 1.5  1996/09/11 16:54:31  digulla
    Always use AROS_SLIB_ENTRY() to access shared external symbols, because
	some systems name an external symbol "x" as "_x" and others as "x".
	(The problem arises with assembler symbols which might differ)

    Revision 1.4  1996/09/11 14:03:56  digulla
    Quick hack to make it work again.

    Revision 1.3  1996/08/13 14:11:54  digulla
    Replaced AROS_LA by AROS_LHA

    Revision 1.2  1996/08/01 17:41:42  digulla
    Added standard header for all files

    Desc:
    Lang:
*/
#include <exec/resident.h>
#include <proto/exec.h>
#include <aros/libcall.h>
#include <dos/dos.h>
#include "utility_intern.h"
/* #include <utility/utility.h> */

static const char name[];
static const char version[];
static const APTR inittabl[4];
static void *const Utility_functable[];
struct UtilityBase *AROS_SLIB_ENTRY(init,Utility) ();
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
    (APTR)sizeof(struct IntUtilityBase),
    (APTR)Utility_functable,
    NULL,
    &AROS_SLIB_ENTRY(init,Utility)
};

#ifdef SysBase
#undef SysBase
#endif
#define SysBase GetIntUtilityBase(UtilityBase)->ub_SysBase

AROS_LH2(struct UtilityBase *, init,
 AROS_LHA(struct UtilityBase *, UtilityBase, D0),
 AROS_LHA(BPTR,               segList,   A0),
	   struct ExecBase *, sysBase, 0, Utility)
{
    AROS_LIBFUNC_INIT

    /* Store arguments */
    GetIntUtilityBase(UtilityBase)->ub_SysBase=sysBase;
    GetIntUtilityBase(UtilityBase)->ub_SegList=segList;

    /* You would return NULL if the init failed */
    return UtilityBase;
    AROS_LIBFUNC_EXIT
}

AROS_LH1(struct UtilityBase *, open,
 AROS_LHA(ULONG, version, D0),
	   struct UtilityBase *, UtilityBase, 1, Utility)
{
    AROS_LIBFUNC_INIT

    /* Keep compiler happy */
    version=0;

    /* I have one more opener. */
    UtilityBase->ub_LibNode.lib_OpenCnt++;
    UtilityBase->ub_LibNode.lib_Flags&=~LIBF_DELEXP;

    /* You would return NULL if the open failed. */
    return UtilityBase;
    AROS_LIBFUNC_EXIT
}

AROS_LH0(BPTR, close,
	   struct UtilityBase *, UtilityBase, 2, Utility)
{
    AROS_LIBFUNC_INIT

    /* I have one fewer opener. */
    if(!--UtilityBase->ub_LibNode.lib_OpenCnt)
    {
	/* Delayed expunge pending? */
	if(UtilityBase->ub_LibNode.lib_Flags&LIBF_DELEXP)
	    /* Then expunge the library */
	    return expunge();
    }
    return 0;
    AROS_LIBFUNC_EXIT
}

AROS_LH0(BPTR, expunge,
	   struct UtilityBase *, UtilityBase, 3, Utility)
{
    AROS_LIBFUNC_INIT

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
    ret=GetIntUtilityBase(UtilityBase)->ub_SegList;

    /* Free the memory. */
    FreeMem((char *)UtilityBase-UtilityBase->ub_LibNode.lib_NegSize,
	    UtilityBase->ub_LibNode.lib_NegSize+UtilityBase->ub_LibNode.lib_PosSize);

    return ret;
    AROS_LIBFUNC_EXIT
}

AROS_LH0I(int, null,
	    struct UtilityBase *, UtilityBase, 4, Utility)
{
    AROS_LIBFUNC_INIT
    return 0;
    AROS_LIBFUNC_EXIT
}
