/*
    Copyright (C) 1995-1998 AROS
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

#define INIT    AROS_SLIB_ENTRY(init, Locale)
static const char name[];
static const char version[];
static const APTR inittabl[4];
static const void * const LIBFUNCTABLE[];
struct LIBBASETYPE * INIT();
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
    RTF_AUTOINIT,
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

AROS_LH2(struct LIBBASETYPE *, init,
    AROS_LHA(struct LIBBASETYPE *,  LIBBASE, D0),
    AROS_LHA(BPTR,                  segList, A0),
    struct ExecBase *, sysBase, 0, Locale)
{
    AROS_LIBFUNC_INIT

    struct IntLocale *def;

    SysBase = sysBase;
  
    /* Do whatever static initialisation you need here */
    InitSemaphore(&((struct IntLocaleBase *)LIBBASE)->lb_LocaleLock);
    InitSemaphore(&((struct IntLocaleBase *)LIBBASE)->lb_CatalogLock);

    NEWLIST(&((struct IntLocaleBase *)LIBBASE)->lb_CatalogList);

    def = AllocMem(sizeof(struct IntLocale), MEMF_CLEAR|MEMF_ANY);
    if(def != NULL)
    {
	/* Copy the defaults to our new structure */
	CopyMem(&defLocale, def, sizeof(struct Locale));

	/* Setup the languages - will not fail here. */
	SetLocaleLanguage(def, LocaleBase);

	def->il_Count = 0;
	IntLB(LIBBASE)->lb_CurrentLocale = def;
	return LIBBASE;
    }
    return NULL;

    AROS_LIBFUNC_EXIT
}

AROS_LH1(struct LIBBASETYPE *, open,
    AROS_LHA(ULONG, version, D0),
    struct LIBBASETYPE *, LIBBASE, 1, Locale)
{
    AROS_LIBFUNC_INIT

    /* keep compiler happy */
    version = 0;

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

    /* What else do we have to do? */
    LIBBASE->lb_LibNode.lib_OpenCnt++;
    LIBBASE->lb_LibNode.lib_Flags &= ~LIBF_DELEXP;

    return LIBBASE;
    AROS_LIBFUNC_EXIT
}

AROS_LH0(BPTR, close,
    struct LIBBASETYPE *, LIBBASE, 2, Locale)
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

AROS_LH0(BPTR, expunge,
    struct LIBBASETYPE *, LIBBASE, 3, Locale)
{
    AROS_LIBFUNC_INIT

    /* As I said above, we cannot remove ourselves. */
    LIBBASE->lb_LibNode.lib_Flags &= ~LIBF_DELEXP;

    /* Free some memory if possible */

    return 0;
    AROS_LIBFUNC_EXIT
}

AROS_LH0I(int, null,
    struct LIBBASETYPE *, LIBBASE, 4, Locale)
{
    AROS_LIBFUNC_INIT

    return 0;

    AROS_LIBFUNC_EXIT
}
