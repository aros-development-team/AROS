/*
    (C) 1997 AROS - The Amiga Replacement OS
    $Id$

    Desc: GadTools initialization code.
    Lang: English.
*/
#include <stddef.h>
#include <proto/exec.h>
#include <exec/libraries.h>
#include <exec/types.h>
#include <exec/resident.h>
#include <aros/libcall.h>
#include "initstruct.h"
#include "gadtools_intern.h"
#include "libdefs.h"

struct inittable;
extern const char name[];
extern const char version[];
extern const APTR inittabl[4];
extern void *const FUNCTABLE[];
extern const struct inittable datatable;
extern struct GadToolsBase_intern *INIT();
extern struct GadToolsBase_intern *AROS_SLIB_ENTRY(open,GadTools)();
extern BPTR AROS_SLIB_ENTRY(close,GadTools)();
extern BPTR AROS_SLIB_ENTRY(expunge,GadTools)();
extern int AROS_SLIB_ENTRY(null,GadTools)();
extern const char END;

void storelibbases(struct GadToolsBase_intern *GadToolsBase);

int entry(void)
{
    /* If the library was executed by accident return error code. */
    return -1;
}

const struct Resident resident=
{
    RTC_MATCHWORD,
    (struct Resident *)&resident,
    (APTR)&GadTools_end,
    RTF_AUTOINIT,
    LIBVERSION,
    NT_LIBRARY,
    0,
/*    -120,*/	/* priority */
    (char *)name,
    (char *)&version[6],
    (ULONG *)inittabl
};

const char name[]=LIBNAME;

const char version[]=VERSION;

const APTR inittabl[4]=
{
    (APTR)sizeof(struct GadToolsBase_intern),
    (APTR)FUNCTABLE,
    (APTR)&datatable,
    &INIT
};

struct inittable
{
    S_CPYO(1,1,B);
    S_CPYO(2,1,L);
    S_CPYO(3,1,B);
    S_CPYO(4,1,W);
    S_CPYO(5,1,W);
    S_CPYO(6,1,L);
    S_END (END);
};

#define O(n) offsetof(struct GadToolsBase_intern,n)

const struct inittable datatable=
{
    { { I_CPYO(1,B,O(library.lib_Node.ln_Type)), { NT_LIBRARY } } },
    { { I_CPYO(1,L,O(library.lib_Node.ln_Name)), { (IPTR)name } } },
    { { I_CPYO(1,B,O(library.lib_Flags       )), { LIBF_SUMUSED|LIBF_CHANGED } } },
    { { I_CPYO(1,W,O(library.lib_Version     )), { LIBVERSION } } },
    { { I_CPYO(1,W,O(library.lib_Revision    )), { LIBREVISION } } },
    { { I_CPYO(1,L,O(library.lib_IdString    )), { (IPTR)&version[6] } } },
  I_END ()
};

#undef O
#undef SysBase

AROS_LH2(struct GadToolsBase_intern *, init,
 AROS_LHA(struct GadToolsBase_intern *, LIBBASE, D0),
 AROS_LHA(BPTR,               segList,   A0),
     struct ExecBase *, sysBase, 0, BASENAME)
{
    AROS_LIBFUNC_INIT
    /* This function is single-threaded by exec by calling Forbid. */

    /* Store arguments */
    LIBBASE->sysbase=sysBase;
    LIBBASE->seglist=segList;

    /* You would return NULL here if the init failed. */
    return LIBBASE;
    AROS_LIBFUNC_EXIT
}

/* Use This from now on */
#define SysBase LIBBASE->sysbase

AROS_LH1(struct GadToolsBase_intern *, open,
 AROS_LHA(ULONG, version, D0),
     struct GadToolsBase_intern *, LIBBASE, 1, BASENAME)
{
    AROS_LIBFUNC_INIT
    /*
	This function is single-threaded by exec by calling Forbid.
	If you break the Forbid() another task may enter this function
	at the same time. Take care.
    */

    /* Keep compiler happy */
    version=0;

    if (!IntuitionBase)
        IntuitionBase = (IntuiBase *)OpenLibrary("intuition.library", 36);
    if (!IntuitionBase)
        return(NULL);

    if (!DOSBase)
        DOSBase = OpenLibrary(DOSNAME, 37);
    if (!DOSBase)
        return NULL;

    if (!GfxBase)
	GfxBase = (GraphicsBase *)OpenLibrary("graphics.library", 37);
    if (!GfxBase)
	return NULL;

    if (!UtilityBase)
	UtilityBase = OpenLibrary("utility.library", 37);
    if (!UtilityBase)
	return NULL;

    storelibbases(LIBBASE);

    /* I have one more opener. */
    LIBBASE->library.lib_OpenCnt++;
    LIBBASE->library.lib_Flags&=~LIBF_DELEXP;

    /* You would return NULL if the open failed. */
    return LIBBASE;
    AROS_LIBFUNC_EXIT
}

AROS_LH0(BPTR, close, struct GadToolsBase_intern *, LIBBASE, 2, BASENAME)
{
    AROS_LIBFUNC_INIT
    /*
	This function is single-threaded by exec by calling Forbid.
	If you break the Forbid() another task may enter this function
	at the same time. Take care.
    */

    /* I have one fewer opener. */
    if(!--LIBBASE->library.lib_OpenCnt)
    {
	if (UtilityBase)
	    CloseLibrary(UtilityBase);
	if (GfxBase)
	    CloseLibrary((struct Library *)GfxBase);
	if (DOSBase)
	    CloseLibrary(DOSBase);
	if (IntuitionBase)
	    CloseLibrary((struct Library *)IntuitionBase);

	/* Delayed expunge pending? */
	if(LIBBASE->library.lib_Flags&LIBF_DELEXP)
	    /* Then expunge the library */
	    return expunge();
    }
    return 0;
    AROS_LIBFUNC_EXIT
}

AROS_LH0(BPTR, expunge, struct GadToolsBase_intern *, LIBBASE, 3, BASENAME)
{
    AROS_LIBFUNC_INIT

    BPTR ret;
    /*
	This function is single-threaded by exec by calling Forbid.
	Never break the Forbid() or strange things might happen.
    */

    /* Test for openers. */
    if(LIBBASE->library.lib_OpenCnt)
    {
	/* Set the delayed expunge flag and return. */
	LIBBASE->library.lib_Flags|=LIBF_DELEXP;
	return 0;
    }

    /* Get rid of the library. Remove it from the list. */
    Remove(&LIBBASE->library.lib_Node);

    /* Get returncode here - FreeMem() will destroy the field. */
    ret=LIBBASE->seglist;

    /* Free the memory. */
    FreeMem((char *)LIBBASE-LIBBASE->library.lib_NegSize,
	LIBBASE->library.lib_NegSize+LIBBASE->library.lib_PosSize);

    return ret;
    AROS_LIBFUNC_EXIT
}

AROS_LH0I(int, null, struct GadToolsBase_intern *, LIBBASE, 4, BASENAME)
{
    AROS_LIBFUNC_INIT
    return 0;
    AROS_LIBFUNC_EXIT
}

#undef IntuitionBase

struct IntuitionBase *IntuitionBase;

void storelibbases(struct GadToolsBase_intern *GadToolsBase)
{
    IntuitionBase = GTB(GadToolsBase)->intuibase;
}
