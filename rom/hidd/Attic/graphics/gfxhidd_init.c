/*
    (C) 1998 AROS - The Amiga Replacement OS
    $Id$

    Desc: Graphics hidd initialization code.
    Lang: English.
*/
#define AROS_ALMOST_COMPATIBLE
#include <stddef.h>
#include <exec/libraries.h>
#include <exec/types.h>
#include <exec/resident.h>
#include <aros/libcall.h>
#include <utility/utility.h>
#include <proto/exec.h>
#include "initstruct.h"
#include "gfxhidd_intern.h"
#include "libdefs.h"
#define DEBUG 1
#include <aros/debug.h>

#define INIT    AROS_SLIB_ENTRY(init,GfxHidd)

struct inittable;
extern const char name[];
extern const char version[];
extern const APTR inittabl[4];
extern void *const FUNCTABLE[];
extern const struct inittable datatable;
extern struct GfxHiddBase_intern *INIT();
extern struct GfxHiddBase_intern *AROS_SLIB_ENTRY(open,GfxHidd)();
extern BPTR AROS_SLIB_ENTRY(close,GfxHidd)();
extern BPTR AROS_SLIB_ENTRY(expunge,GfxHidd)();
extern int AROS_SLIB_ENTRY(null,GfxHidd)();
extern const char END;

void storelibbases(struct GfxHiddBase_intern *GfxHiddBase);

int entry(void)
{
    /* If the library was executed by accident return error code. */
    return -1;
}

const struct Resident resident=
{
    RTC_MATCHWORD,
    (struct Resident *)&resident,
    (APTR)&GfxHidd_end,
    RTF_AUTOINIT,
    LIBVERSION,
    NT_LIBRARY,
    0,
/*    -120,*/   /* priority */
    (char *)name,
    (char *)&version[6],
    (ULONG *)inittabl
};

const char name[]=LIBNAME;

const char version[]=VERSION;

const APTR inittabl[4]=
{
    (APTR)sizeof(struct GfxHiddBase_intern),
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

#define O(n) offsetof(struct GfxHiddBase_intern,n)

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

struct ExecBase * SysBase;

/* Predeclaration */
struct IClass *InitGfxHiddClass(struct GfxHiddBase_intern *);
struct IClass *InitGfxHiddBitMapClass(struct GfxHiddBase_intern * GfxHiddBase);
struct IClass *InitGfxHiddGCClass(struct GfxHiddBase_intern * GfxHiddBase);


AROS_LH2(struct GfxHiddBase_intern *, init,
 AROS_LHA(struct GfxHiddBase_intern *, LIBBASE, D0),
 AROS_LHA(BPTR,               segList,   A0),
     struct ExecBase *, sysBase, 0, BASENAME)
{
    AROS_LIBFUNC_INIT
    /* This function is single-threaded by exec by calling Forbid. */

    /* Store arguments */
    SysBase                 = sysBase;
    LIBBASE->sysbase        = sysBase;
    LIBBASE->seglist        = segList;

    /* Init pointers */
    LIBBASE->intuibase      = NULL;
    LIBBASE->dosbase        = NULL;
    LIBBASE->gfxbase        = NULL;
    LIBBASE->utilitybase    = NULL;
    LIBBASE->boopsibase     = NULL;

    LIBBASE->classptr       = NULL;
    LIBBASE->bitMapClassptr = NULL;
    LIBBASE->gcClassptr     = NULL;

    /* You would return NULL here if the init failed. */
    return LIBBASE;
    AROS_LIBFUNC_EXIT
}


AROS_LH1(struct GfxHiddBase_intern *, open,
 AROS_LHA(ULONG, version, D0),
     struct GfxHiddBase_intern *, LIBBASE, 1, BASENAME)
{
    AROS_LIBFUNC_INIT
    /*
        This function is single-threaded by exec by calling Forbid.
        If you break the Forbid() another task may enter this function
        at the same time. Take care.
    */

    /* Keep compiler happy */
    version=0;


    /* Will work with Amiga-OS V37 later */

    if (!IntuitionBase)
        IntuitionBase = (IntuiBase *)OpenLibrary(INTUITIONNAME, 39);
    if (!IntuitionBase)
        return(NULL);

    if (!DOSBase)
        DOSBase = OpenLibrary(DOSNAME, 39);
    if (!DOSBase)
        return NULL;

    if (!GfxBase)
        GfxBase = (GraphicsBase *)OpenLibrary(GRAPHICSNAME, 39);
    if (!GfxBase)
        return NULL;

    if (!UtilityBase)
        UtilityBase = OpenLibrary(UTILITYNAME, 39);
    if (!UtilityBase)
        return NULL;

/* Use intuition boopsi on amigaos systems */
#ifndef __amigaos__
    if (!BOOPSIBase)
        BOOPSIBase = OpenLibrary(BOOPSINAME, 37);
    if (!BOOPSIBase)
        return NULL;
#endif

    D(bug("ok 1\n"));

    /* ------------------------- */
    /* Create the class itself */
    if (!LIBBASE->classptr)
        LIBBASE->classptr = InitGfxHiddClass(LIBBASE);
    if (!LIBBASE->classptr)
        return (NULL);

    /* Create the bitmap class */
    if (!LIBBASE->bitMapClassptr)
        LIBBASE->bitMapClassptr = InitGfxHiddBitMapClass(LIBBASE);
    if (!LIBBASE->bitMapClassptr)
        return (NULL);

    /* Create the graphics context class */
    if (!LIBBASE->gcClassptr)
        LIBBASE->gcClassptr = InitGfxHiddGCClass(LIBBASE);
    if (!LIBBASE->gcClassptr)
        return (NULL);
    /* ------------------------- */

    D(bug("ok 2\n"));

    /* I have one more opener. */
    LIBBASE->library.lib_OpenCnt++;
    LIBBASE->library.lib_Flags&=~LIBF_DELEXP;

    /* You would return NULL if the open failed. */
    return LIBBASE;
    AROS_LIBFUNC_EXIT
}

AROS_LH0(BPTR, close, struct GfxHiddBase_intern *, LIBBASE, 2, BASENAME)
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
        if (LIBBASE->classptr)
        {
            RemoveClass(LIBBASE->classptr);
            FreeClass(LIBBASE->classptr);
            LIBBASE->classptr = NULL;
        }

        if (LIBBASE->bitMapClassptr)
        {
            RemoveClass(LIBBASE->bitMapClassptr);
            FreeClass(LIBBASE->bitMapClassptr);
            LIBBASE->bitMapClassptr = NULL;
        }

        if (LIBBASE->gcClassptr)
        {
            RemoveClass(LIBBASE->gcClassptr);
            FreeClass(LIBBASE->gcClassptr);
            LIBBASE->gcClassptr = NULL;
        }

/* Use intuition boopsi on amigaos systems */
#ifndef __amigaos__
        if (BOOPSIBase)
            CloseLibrary(BOOPSIBase);
#endif
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

AROS_LH0(BPTR, expunge, struct GfxHiddBase_intern *, LIBBASE, 3, BASENAME)
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

AROS_LH0I(int, null, struct GfxHiddBase_intern *, LIBBASE, 4, BASENAME)
{
    AROS_LIBFUNC_INIT
    return 0;
    AROS_LIBFUNC_EXIT
}
