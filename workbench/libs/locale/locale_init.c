/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Initialisation for the locale.library.
    Lang: english
*/

#define AROS_ALMOST_COMPATIBLE
#include <exec/types.h>
#include <exec/lists.h>
#include <exec/memory.h>
#include <exec/resident.h>
#include <aros/libcall.h>
#include <exec/semaphores.h>

#include <proto/exec.h>

#include "locale_intern.h"
#include "libdefs.h"

/*
    Why don't I use C_Lib, simple, it doesn't seem to be able to do
    non-expunge libraries, which locale is.
*/

#ifdef __MORPHOS__
    unsigned long __amigappc__ = 1;
#endif

#define INIT    AROS_SLIB_ENTRY(init, Locale)
static const char name[];
static const char version[];
static const APTR inittabl[4];
static const void * const LIBFUNCTABLE[];
LIBBASETYPE * INIT();
extern const char LIBEND;

extern void SetLocaleLanguage(struct IntLocale *, struct LocaleBase *);
extern struct Locale defLocale;

int entry(void)
{
    return -1;
}

const struct Resident Locale_resident =
{
    RTC_MATCHWORD,
    (struct Resident *)&Locale_resident,
    (APTR)&LIBEND,
#ifdef __MORPHOS__
    RTF_PPC |RTF_AUTOINIT,
#else
    RTF_AUTOINIT,
#endif
    VERSION_NUMBER,
    NT_LIBRARY,
    -120,
    (char *)name,
    (char *)&version[6],
    (ULONG *)inittabl
};

static const char name[]=NAME_STRING;
static const char version[]=VERSION_STRING;

static const APTR inittabl[4] =
{
    (APTR)sizeof(struct IntLocaleBase),
    (APTR)LIBFUNCTABLE,
    NULL,
    &INIT
};
#undef DOSBase
#undef UtilityBase
#undef IFFParseBase

/* This global variable is needed for LocRawDoFmt */

struct LocaleBase *globallocalebase;

#ifdef __MORPHOS__
LIBBASETYPE *LIB_init(LIBBASETYPE *LIBBASE, BPTR segList, struct ExecBase *sysBase)
#else
AROS_LH2(LIBBASETYPE *, init,
    AROS_LHA(LIBBASETYPE *,  LIBBASE, D0),
    AROS_LHA(BPTR,                  segList, A0),
    struct ExecBase *, sysBase, 0, Locale)
#endif
{
    AROS_LIBFUNC_INIT

    struct IntLocale *def;

    SysBase = sysBase;
  
    globallocalebase = LIBBASE;
    
    /* Do whatever static initialisation you need here */
    InitSemaphore(&((struct IntLocaleBase *)LIBBASE)->lb_LocaleLock);
    InitSemaphore(&((struct IntLocaleBase *)LIBBASE)->lb_CatalogLock);

    NEWLIST(&((struct IntLocaleBase *)LIBBASE)->lb_CatalogList);

    /* We have to open some libraries. */
    if( IntLB(LIBBASE)->lb_DosBase == NULL )
    {
	if(!( IntLB(LIBBASE)->lb_DosBase = 
		    (struct DosLibrary *)OpenLibrary("dos.library", 37L)))
	{
	    return NULL;
	}

	if(!( IntLB(LIBBASE)->lb_UtilityBase =
		    OpenLibrary("utility.library", 37L)))
	{
	    CloseLibrary((struct Library *)IntLB(LIBBASE)->lb_DosBase);
	    IntLB(LIBBASE)->lb_DosBase = NULL;
	    return NULL;
	}

	if(!( IntLB(LIBBASE)->lb_IFFParseBase =
		    OpenLibrary("iffparse.library", 37L)))
	{
	    CloseLibrary((struct Library *)IntLB(LIBBASE)->lb_DosBase);
	    CloseLibrary(IntLB(LIBBASE)->lb_UtilityBase);
	    IntLB(LIBBASE)->lb_DosBase = NULL;
	    return NULL;
	}
    }

    def = AllocMem(sizeof(struct IntLocale), MEMF_CLEAR|MEMF_ANY);
    if(def != NULL)
    {
	/* Copy the defaults to our new structure */
	CopyMem(&defLocale, def, sizeof(struct Locale));

	/* Setup the languages - will not fail here. */
	SetLocaleLanguage(def, LocaleBase);

	def->il_Count = 0;
	IntLB(LIBBASE)->lb_CurrentLocale = def;
    	InstallPatches();
	return LIBBASE;
    }
    return NULL;

#ifndef __MORPHOS__
    AROS_LIBFUNC_EXIT
#endif
}

AROS_LH1(LIBBASETYPE *, open,
    AROS_LHA(ULONG, version, D0),
    LIBBASETYPE *, LIBBASE, 1, Locale)
{
    AROS_LIBFUNC_INIT

    /* keep compiler happy */
    version = 0;


    /* What else do we have to do? */
    LIBBASE->lb_LibNode.lib_OpenCnt++;
    LIBBASE->lb_LibNode.lib_Flags &= ~LIBF_DELEXP;

    return LIBBASE;
    AROS_LIBFUNC_EXIT
}

AROS_LH0(BPTR, expunge,
    LIBBASETYPE *, LIBBASE, 3, Locale)
{
    AROS_LIBFUNC_INIT

    /* As I said above, we cannot remove ourselves. */
    LIBBASE->lb_LibNode.lib_Flags &= ~LIBF_DELEXP;

    /* Free some memory if possible */

    return 0;
    AROS_LIBFUNC_EXIT
}

AROS_LH0(BPTR, close,
    LIBBASETYPE *, LIBBASE, 2, Locale)
{
    AROS_LIBFUNC_INIT

    --LIBBASE->lb_LibNode.lib_OpenCnt;

    /*
	We can never exit because of the system patches,
	But we can try and free some memory.
    */
    AROS_LC0(BPTR, expunge, LIBBASETYPE *, LIBBASE, 3, Locale);

    return 0;
    AROS_LIBFUNC_EXIT
}

AROS_LH0I(int, null,
    LIBBASETYPE *, LIBBASE, 4, Locale)
{
    AROS_LIBFUNC_INIT

    return 0;

    AROS_LIBFUNC_EXIT
}
