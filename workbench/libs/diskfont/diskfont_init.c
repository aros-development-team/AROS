/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Diskfont initialization code.
    Lang: English.
*/

#include <exec/libraries.h>
#include <exec/types.h>
#include <exec/resident.h>
#include <graphics/text.h>
#include <diskfont/diskfont.h>
#include <aros/libcall.h>
#include <aros/asmcall.h>
#include <aros/debug.h>
#include <proto/exec.h>
#include <proto/dos.h>
//#include <proto/alib.h>
#include <proto/graphics.h>

#include <stddef.h>

#include "initstruct.h"
#include LC_LIBDEFS_FILE
#include "diskfont_intern.h"

/****************************************************************************************/

#define INIT	AROS_SLIB_ENTRY(init,Diskfont)

/****************************************************************************************/

#include <aros/debug.h>

/****************************************************************************************/
#ifdef __MORPHOS__
    unsigned long __abox__ = 1;
#endif

struct inittable;
extern const char name[];
extern const char version[];
extern const APTR inittabl[4];
extern void *const LIBFUNCTABLE[];
extern const struct inittable datatable;
extern struct DiskfontBase_intern *INIT();
extern struct DiskfontBase_intern *AROS_SLIB_ENTRY(open,Diskfont)();
extern BPTR AROS_SLIB_ENTRY(close,Diskfont)();
extern BPTR AROS_SLIB_ENTRY(expunge,Diskfont)();
extern int AROS_SLIB_ENTRY(null,Diskfont)();
extern const char LIBEND;

/****************************************************************************************/

#if (defined(__mc68000__) && (AROS_FLAVOUR & AROS_FLAVOUR_NATIVE))
const LONG entry = 0x70FF4E75;
#else
int entry(void)
{
    /* If the library was executed by accident return error code. */
    return -1;
}
#endif

/****************************************************************************************/

const struct Resident resident=
{
    RTC_MATCHWORD,
    (struct Resident *)&resident,
    (APTR)&LIBEND,
#ifdef __MORPHOS__
    RTF_PPC | RTF_EXTENDED | RTF_AUTOINIT,
#else
    RTF_AUTOINIT,
#endif
    VERSION_NUMBER,
    NT_LIBRARY,
    -120,	/* priority */
    (char *)name,
    (char *)&version[6],
    (ULONG *)inittabl,
#ifdef __MORPHOS__
    REVISION_NUMBER,	/* Revision */
    NULL /* Tags */
#endif
};

const char name[]=NAME_STRING;

const char version[]=VERSION_STRING;

const APTR inittabl[4]=
{
    (APTR)sizeof(struct DiskfontBase_intern),
    (APTR)LIBFUNCTABLE,
#ifdef __MORPHOS__
    NULL,
#else
    (APTR)&datatable,
#endif
    &INIT
};

#ifndef __MORPHOS__
struct inittable
{
    S_CPYO(1,1,B);
    S_CPYO(2,1,L);
    S_CPYO(3,1,B);
    S_CPYO(4,1,W);
    S_CPYO(5,1,W);
    S_CPYO(6,1,L);
    S_END (LIBEND);
};

#define O(n) offsetof(struct DiskfontBase_intern,n)

const struct inittable datatable=
{
    { { I_CPYO(1,B,O(library.lib_Node.ln_Type)), { NT_LIBRARY } } },
    { { I_CPYO(1,L,O(library.lib_Node.ln_Name)), { (IPTR)name } } },
    { { I_CPYO(1,B,O(library.lib_Flags       )), { LIBF_SUMUSED|LIBF_CHANGED } } },
    { { I_CPYO(1,W,O(library.lib_Version     )), { VERSION_NUMBER } } },
    { { I_CPYO(1,W,O(library.lib_Revision    )), { REVISION_NUMBER } } },
    { { I_CPYO(1,L,O(library.lib_IdString    )), { (IPTR)&version[6] } } },
  I_END ()
};
#endif
/****************************************************************************************/

struct ExecBase * SysBase;
struct DosLibrary *DOSBase;

/****************************************************************************************/

/* #undef O
#undef SysBase */

/****************************************************************************************/

#ifdef __MORPHOS__
struct DiskfontBase_intern *LIB_init(struct DiskfontBase_intern *LIBBASE, BPTR segList, struct ExecBase *sysBase)
{
#else
AROS_UFH3(struct DiskfontBase_intern *, AROS_SLIB_ENTRY(init,BASENAME),
    AROS_UFHA(struct DiskfontBase_intern *, LIBBASE, D0),
    AROS_UFHA(BPTR,               segList,   A0),
    AROS_UFHA(struct ExecBase *, sysBase, A6)
)
#endif
{
    AROS_USERFUNC_INIT

    /* This function is single-threaded by exec by calling Forbid. */

    SysBase = sysBase;

    D(bug("Inside initfunc\n"));

    LIBBASE->seglist = segList;

    NEWLIST(&LIBBASE->diskfontlist);
    NEWLIST(&LIBBASE->fontsdirentrylist);
    InitSemaphore(&LIBBASE->fontssemaphore);
    
#ifdef __MORPHOS__
    LIBBASE->library.lib_Revision = REVISION_NUMBER;
#endif

    /* You would return NULL here if the init failed. */
    return LIBBASE;

    AROS_USERFUNC_EXIT
}

/****************************************************************************************/

/* Use This from now on * /
#define SysBase LIBBASE->sysbase */

/****************************************************************************************/

AROS_LH1(struct DiskfontBase_intern *, open,
    AROS_LHA(ULONG, version, D0),
    struct DiskfontBase_intern *, LIBBASE, 1, BASENAME)
{
    AROS_LIBFUNC_INIT
    /*
	This function is single-threaded by exec by calling Forbid.
	If you break the Forbid() another task may enter this function
	at the same time. Take care.
    */

    /* Keep compiler happy */
    (void) version;

    D(bug("Inside openfunc\n"));

    /* I have one more opener. */
#if ALWAYS_ZERO_LIBCOUNT
    LIBBASE->realopencount++;
    if (LIBBASE->realopencount == 1)
#else
    LIBBASE->library.lib_OpenCnt++;
    if (LIBBASE->library.lib_OpenCnt == 1)
#endif
    {
	if (!DOSBase)
	    DOSBase = (struct DosLibrary *)OpenLibrary("dos.library", 37);
	if (!DOSBase)
	{
#if ALWAYS_ZERO_LIBCOUNT
	    LIBBASE->realopencount--;
#else
	    LIBBASE->library.lib_OpenCnt--;
#endif
	    return NULL;
	}

	if (!GfxBase)
	    GfxBase = (GraphicsBase *)OpenLibrary("graphics.library", 37);
	if (!GfxBase)
	{
#if ALWAYS_ZERO_LIBCOUNT
	    LIBBASE->realopencount--;
#else
	    LIBBASE->library.lib_OpenCnt--;
#endif
	    return NULL;
	}

	if (!UtilityBase)
	    UtilityBase = OpenLibrary("utility.library", 37);
	if (!UtilityBase)
	{
#if ALWAYS_ZERO_LIBCOUNT
	    LIBBASE->realopencount--;
#else
	    LIBBASE->library.lib_OpenCnt--;
#endif
	    return NULL;
	}

    /* Insert the fonthooks into the DiskfontBase */

	LIBBASE->dsh.h_Entry = (void *)AROS_ASMSYMNAME(dosstreamhook);
	LIBBASE->dsh.h_Data = DOSBase;
    }

    LIBBASE->library.lib_Flags&=~LIBF_DELEXP;

    /* You would return NULL if the open failed. */
    return LIBBASE;

    AROS_LIBFUNC_EXIT
}

/****************************************************************************************/

AROS_LH0(BPTR, close, struct DiskfontBase_intern *, LIBBASE, 2, BASENAME)
{
    AROS_LIBFUNC_INIT

    /*
	This function is single-threaded by exec by calling Forbid.
	If you break the Forbid() another task may enter this function
	at the same time. Take care.
    */

    /* I have one fewer opener. */

#if ALWAYS_ZERO_LIBCOUNT
    if(!--LIBBASE->realopencount)
#else
    if(!--LIBBASE->library.lib_OpenCnt)
#endif
    {
	/* Delayed expunge pending? */
	if(LIBBASE->library.lib_Flags&LIBF_DELEXP)
	    /* Then expunge the library */
	    return expunge();
    }

    return 0;

    AROS_LIBFUNC_EXIT
}

/****************************************************************************************/

AROS_LH0(BPTR, expunge, struct DiskfontBase_intern *, LIBBASE, 3, BASENAME)
{
    AROS_LIBFUNC_INIT

    struct DiskFontHeader *dfh, *dfh2;

    BPTR ret;
    /*
	This function is single-threaded by exec by calling Forbid.
	Never break the Forbid() or strange things might happen.
    */

    ForeachNodeSafe(&LIBBASE->diskfontlist, dfh, dfh2)
    {
	if (dfh->dfh_TF.tf_Accessors < 1)
	{
	    /* Possible paranoia check */
	    if (!(dfh->dfh_TF.tf_Flags & FPF_REMOVED))
	    {
		/* Unlink from GfxBase->TextFonts */
		REMOVE(&dfh->dfh_TF.tf_Message.mn_Node);

		StripFont(&dfh->dfh_TF);

		/* Unlink from DiskfontBase->diskfontlist */

		REMOVE(&dfh->dfh_DF);

		UnLoadSeg(dfh->dfh_Segment);
	    }
	}
    }

    /* Test for openers. */
#if ALWAYS_ZERO_LIBCOUNT
    if (LIBBASE->realopencount)
#else
    if(LIBBASE->library.lib_OpenCnt)
#endif
    {
	/* Set the delayed expunge flag and return. */
	LIBBASE->library.lib_Flags |= LIBF_DELEXP;
	return 0;
    }

    CleanUpFontsDirEntryList(LIBBASE);
    
    if (UtilityBase)
	CloseLibrary(UtilityBase);
    UtilityBase = NULL;

    if (GfxBase)
	CloseLibrary((struct Library *)GfxBase);
    GfxBase = NULL;

    if (DOSBase)
	CloseLibrary((struct Library *)DOSBase);
    DOSBase = NULL;

    /* Get rid of the library. Remove it from the list. */
    Forbid();
    REMOVE(&LIBBASE->library.lib_Node);
    Permit();

    /* Get returncode here - FreeMem() will destroy the field. */
    ret=LIBBASE->seglist;

    /* Free the memory. */
    FreeMem((char *)LIBBASE-LIBBASE->library.lib_NegSize,
	LIBBASE->library.lib_NegSize+LIBBASE->library.lib_PosSize);

    return ret;

    AROS_LIBFUNC_EXIT
}

/****************************************************************************************/

AROS_LH0I(int, null, struct DiskfontBase_intern *, LIBBASE, 4, BASENAME)
{
    AROS_LIBFUNC_INIT

    return 0;

    AROS_LIBFUNC_EXIT
}

/****************************************************************************************/
