/*
    (C) 1997 AROS - The Amiga Research OS
    $Id$

    Desc: AROSPalette initialization code.
    Lang: English.
*/

#include <proto/intuition.h>
#include "initstruct.h"
#include "arospalette_intern.h"
#include "libdefs.h"
#include <stddef.h>
#include <exec/libraries.h>
#include <exec/types.h>
#include <exec/resident.h>
#include <aros/libcall.h>
#include <proto/exec.h>

#define INIT AROS_SLIB_ENTRY(init, AROSPalette)

struct inittable;
extern const char name[];
extern const char version[];
extern const APTR inittabl[4];
extern void *const FUNCTABLE[];
extern const struct inittable datatable;
extern struct PaletteBase_intern *INIT();
extern struct PaletteBase_intern *AROS_SLIB_ENTRY(open,AROSPalette)();
extern BPTR AROS_SLIB_ENTRY(close,AROSPalette)();
extern BPTR AROS_SLIB_ENTRY(expunge,AROSPalette)();
extern int AROS_SLIB_ENTRY(null,AROSPalette)();
extern const char END;

/* FIXME: egcs 1.1b and possibly other incarnations of gcc have
 * two nasty problems with entry() that prevents using the
 * C version of this function on AROS 68k native.
 *
 * First of all, if inlining is active (-O3), the optimizer will decide
 * that entry() is simple enough to be inlined, and it doesn't generate
 * its code until all other functions have been compiled. Delaying asm
 * output for a global (non static) function is probably silly because
 * the optimizer can't eliminate its stand alone istance anyway.
 *
 * The second problem is that even without inlining, the code generator
 * adds a nop instruction immediately after rts. This is probably done
 * to help the 68040/60 pipelines, but it adds two more bytes before the
 * library resident tag, which causes all kinds of problems on native
 * AmigaOS.
 *
 * The workaround is to embed the required assembler instructions
 * (moveq #-1,d0 ; rts) in a constant variable.
 */
#if (defined(__mc68000__) && (AROS_FLAVOUR & AROS_FLAVOUR_NATIVE))
const LONG entry = 0x70FF4E75;
#else
int entry(void)
{
    /* If the library was executed by accident return error code. */
    return -1;
}
#endif

const struct Resident resident=
{
    RTC_MATCHWORD,
    (struct Resident *)&resident,
    (APTR)&AROSPalette_end,
    RTF_AUTOINIT,
    LIBVERSION,
    NT_LIBRARY,
    0,	/* WARNING: residents with negative priority won't be inited on AmigaOS! */
    (char *)name,
    (char *)&version[6],
    (ULONG *)inittabl
};

const char name[]=GADGETNAME;

const char version[]=VERSION;

const APTR inittabl[4]=
{
    (APTR)sizeof(struct PaletteBase_intern),
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

#define O(n) offsetof(struct PaletteBase_intern,n)

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

/* Global IntuitionBase */
#ifdef GLOBAL_INTUIBASE
struct IntuitionBase *IntuitionBase;
#endif

/* #undef O
#undef SysBase */


AROS_LH2(struct PaletteBase_intern *, init,
    AROS_LHA(struct PaletteBase_intern *, LIBBASE, D0),
    AROS_LHA(BPTR,               segList,   A0),
    struct ExecBase *, sysBase, 0, BASENAME)
{
    AROS_LIBFUNC_INIT

    /* This function is single-threaded by exec by calling Forbid. */

    SysBase=sysBase;

    LIBBASE->seglist=segList;

    /* You would return NULL here if the init failed. */
    return LIBBASE;
    AROS_LIBFUNC_EXIT
}

/* Use This from now on * /
#define SysBase LIBBASE->sysbase */

/* Predeclaration */
struct IClass *InitPaletteClass(struct PaletteBase_intern *);

AROS_LH1(struct PaletteBase_intern *, open,
    AROS_LHA(ULONG, version, D0),
    struct PaletteBase_intern *, LIBBASE, 1, BASENAME)
{
    AROS_LIBFUNC_INIT
    /*
	This function is single-threaded by exec by calling Forbid.
	If you break the Forbid() another task may enter this function
	at the same time. Take care.
    */


    /* Keep compiler happy */
    version=0;

    if (!GfxBase)
    	GfxBase = (GraphicsBase *)OpenLibrary("graphics.library", 37);
    if (!GfxBase)
	return(NULL);

    if (!UtilityBase)
	UtilityBase = OpenLibrary("utility.library", 37);
    if (!UtilityBase)
	return(NULL);

    if (!IntuitionBase)
    	IntuitionBase = (IntuiBase *)OpenLibrary("intuition.library", 37);
    if (!IntuitionBase)
	return (NULL);


    /* ------------------------- */
    /* Create the class itself */

    LIBBASE->classptr = InitPaletteClass(LIBBASE);
    if (!LIBBASE->classptr)
    	return (NULL);

    /* ------------------------- */


    /* I have one more opener. */
    LIBBASE->library.lib_OpenCnt++;
    LIBBASE->library.lib_Flags&=~LIBF_DELEXP;

    /* You would return NULL if the open failed. */
    return LIBBASE;
    AROS_LIBFUNC_EXIT
}

AROS_LH0(BPTR, close, struct PaletteBase_intern *, LIBBASE, 2, BASENAME)
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

	CloseLibrary(UtilityBase);
	UtilityBase = NULL;

	CloseLibrary((struct Library *)GfxBase);
	GfxBase = NULL;

	CloseLibrary((struct Library *)IntuitionBase);
	IntuitionBase = NULL;

	/* Delayed expunge pending? */
	if(LIBBASE->library.lib_Flags&LIBF_DELEXP)
	    /* Then expunge the library */
	    return expunge();
    }
    return 0;
    AROS_LIBFUNC_EXIT
}

AROS_LH0(BPTR, expunge, struct PaletteBase_intern *, LIBBASE, 3, BASENAME)
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

AROS_LH0I(int, null, struct PaletteBase_intern *, LIBBASE, 4, BASENAME)
{
    AROS_LIBFUNC_INIT
    return 0;
    AROS_LIBFUNC_EXIT
}


