/*
    (C) 1997 AROS - The Amiga Research OS
    $Id$

    Desc: Diskfont initialization code.
    Lang: English.
*/

#include "initstruct.h"
#include "diskfont_intern.h"
#include "libdefs.h"
#include <stddef.h>
#include <exec/libraries.h>
#include <exec/types.h>
#include <exec/resident.h>
#include <aros/libcall.h>
#include <proto/exec.h>

#define INIT	AROS_SLIB_ENTRY(init,Diskfont)

#include <aros/debug.h>

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
    (APTR)&LIBEND,
    RTF_AUTOINIT,
    VERSION_NUMBER,
    NT_LIBRARY,
    -120,	/* priority */
    (char *)name,
    (char *)&version[6],
    (ULONG *)inittabl
};

const char name[]=NAME_STRING;

const char version[]=VERSION_STRING;

const APTR inittabl[4]=
{
    (APTR)sizeof(struct DiskfontBase_intern),
    (APTR)LIBFUNCTABLE,
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


struct ExecBase * SysBase;
struct Library *DOSBase;
/* #undef O
#undef SysBase */

AROS_LH2(struct DiskfontBase_intern *, init,
    AROS_LHA(struct DiskfontBase_intern *, LIBBASE, D0),
    AROS_LHA(BPTR,               segList,   A0),
    struct ExecBase *, sysBase, 0, BASENAME)
{
    AROS_LIBFUNC_INIT

    /* This function is single-threaded by exec by calling Forbid. */

    SysBase=sysBase;

    D(bug("Inside initfunc\n"));

    LIBBASE->seglist=segList;

    /* You would return NULL here if the init failed. */
    return LIBBASE;
    AROS_LIBFUNC_EXIT
}

/* Use This from now on * /
#define SysBase LIBBASE->sysbase */

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

    /* Hook descriptions */
    struct AFHookDescr hdescrdef[] =
    {
	{AFF_MEMORY,	{{0L, 0L}, (void*)MemoryFontFunc, 0L, 0L}},
	{AFF_DISK,		{{0L, 0L}, (void*)DiskFontFunc, 0L,     0L}}
    };

	UWORD idx;

    /* Keep compiler happy */
    version=0;

    D(bug("Inside openfunc\n"));

    if (!DOSBase)
	DOSBase = OpenLibrary("dos.library", 37);
    if (!DOSBase)
	return(NULL);

    if (!GfxBase)
    GfxBase = (GraphicsBase *)OpenLibrary("graphics.library", 37);
    if (!GfxBase)
	return(NULL);

    if (!UtilityBase)
	UtilityBase = OpenLibrary("utility.library", 37);
    if (!UtilityBase)
		return(NULL);

    /* Insert the fonthooks into the DiskfontBase */

    for (idx = 0; idx < NUMFONTHOOKS; idx ++)
    {
	LIBBASE->hdescr[idx] = hdescrdef[idx];
    }

    LIBBASE->dsh.h_Entry = (void *)dosstreamhook;
    LIBBASE->dsh.h_Data = DOSBase;

    /* I have one more opener. */
    LIBBASE->library.lib_OpenCnt++;
    LIBBASE->library.lib_Flags&=~LIBF_DELEXP;

    /* You would return NULL if the open failed. */
    return LIBBASE;
    AROS_LIBFUNC_EXIT
}

AROS_LH0(BPTR, close, struct DiskfontBase_intern *, LIBBASE, 2, BASENAME)
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

	/* Delayed expunge pending? */
	if(LIBBASE->library.lib_Flags&LIBF_DELEXP)
	    /* Then expunge the library */
	    return expunge();
    }
    return 0;
    AROS_LIBFUNC_EXIT
}

AROS_LH0(BPTR, expunge, struct DiskfontBase_intern *, LIBBASE, 3, BASENAME)
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

AROS_LH0I(int, null, struct DiskfontBase_intern *, LIBBASE, 4, BASENAME)
{
    AROS_LIBFUNC_INIT
    return 0;
    AROS_LIBFUNC_EXIT
}
