/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.2  1996/08/23 17:24:11  digulla
    Opening intuition.library called intui_init() instead of intui_open(). Ooops.

    Revision 1.1  1996/08/13 15:37:26  digulla
    First function for intuition.library


    Desc:
    Lang:
*/
#include <exec/resident.h>
#include <exec/execbase.h>
#include <clib/exec_protos.h>
#include <dos/dos.h>
#include "intuition_intern.h"

static const char name[];
static const char version[];
static const APTR inittabl[4];
static void *const Intuition_functable[];
struct IntuitionBase *Intuition_init();
extern const char Intuition_end;

extern int  intui_init (struct IntuitionBase *);
extern int  intui_open (struct IntuitionBase *);
extern void intui_close (struct IntuitionBase *);
extern void intui_expunge (struct IntuitionBase *);

int Intuition_entry(void)
{
    /* If the library was executed by accident return error code. */
    return -1;
}

const struct Resident Intuition_resident=
{
    RTC_MATCHWORD,
    (struct Resident *)&Intuition_resident,
    (APTR)&Intuition_end,
    RTF_AUTOINIT,
    39,
    NT_LIBRARY,
    0,
    (char *)name,
    (char *)&version[6],
    (ULONG *)inittabl
};

static const char name[]="intuition.library";

static const char version[]="$VER: intuition.library 39.0 (12.8.96)\n\015";

static const APTR inittabl[4]=
{
    (APTR)sizeof(struct IntIntuitionBase),
    (APTR)Intuition_functable,
    NULL,
    &Intuition_init
};

__AROS_LH2(struct IntuitionBase *, init,
 __AROS_LHA(struct IntuitionBase *, IntuitionBase, D0),
 __AROS_LHA(BPTR,               segList,   A0),
	   struct ExecBase *, sysBase, 0, Intuition)
{
    __AROS_FUNC_INIT

    SysBase = sysBase;

    if (!intui_init (IntuitionBase))
	return NULL;

    /* You would return NULL if the init failed */
    return IntuitionBase;
    __AROS_FUNC_EXIT
}

__AROS_LH1(struct IntuitionBase *, open,
 __AROS_LHA(ULONG, version, D0),
	   struct IntuitionBase *, IntuitionBase, 1, Intuition)
{
    __AROS_FUNC_INIT

    /* Keep compiler happy */
    version=0;

    if (!GfxBase)
    {
	if (!(GfxBase = (void *)OpenLibrary (GRAPHICSNAME, 39)) )
	    return NULL;
    }

    if (!intui_open (IntuitionBase))
	return NULL;

    /* I have one more opener. */
    IntuitionBase->LibNode.lib_OpenCnt++;
    IntuitionBase->LibNode.lib_Flags&=~LIBF_DELEXP;

    /* You would return NULL if the open failed. */
    return IntuitionBase;
    __AROS_FUNC_EXIT
}

__AROS_LH0(BPTR, close,
	   struct IntuitionBase *, IntuitionBase, 2, Intuition)
{
    __AROS_FUNC_INIT

    /* I have one fewer opener. */
    if(!--IntuitionBase->LibNode.lib_OpenCnt)
    {
	intui_close (IntuitionBase);

	/* Delayed expunge pending? */
	if(IntuitionBase->LibNode.lib_Flags&LIBF_DELEXP)
	    /* Then expunge the library */
	    return expunge();
    }
    return 0;
    __AROS_FUNC_EXIT
}

__AROS_LH0(BPTR, expunge,
	   struct IntuitionBase *, IntuitionBase, 3, Intuition)
{
    __AROS_FUNC_INIT

    BPTR ret;

    /* Test for openers. */
    if(IntuitionBase->LibNode.lib_OpenCnt)
    {
	/* Set the delayed expunge flag and return. */
	IntuitionBase->LibNode.lib_Flags|=LIBF_DELEXP;
	return 0;
    }

    intui_expunge (IntuitionBase);

    /* Get rid of the library. Remove it from the list. */
    Remove(&IntuitionBase->LibNode.lib_Node);

    /* Get returncode here - FreeMem() will destroy the field. */
    ret=0L;

    /* Free the memory. */
    FreeMem((char *)IntuitionBase-IntuitionBase->LibNode.lib_NegSize,
	    IntuitionBase->LibNode.lib_NegSize+IntuitionBase->LibNode.lib_PosSize);

    return ret;
    __AROS_FUNC_EXIT
}

__AROS_LH0I(int, null,
	    struct IntuitionBase *, IntuitionBase, 4, Intuition)
{
    __AROS_FUNC_INIT
    return 0;
    __AROS_FUNC_EXIT
}
