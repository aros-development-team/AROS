/*
    (C) 1995-99 AROS - The Amiga Research OS
    $Id$

    Desc: initialize datatypes.library
    Lang: English.
*/

#define AROS_ALMOST_COMPATIBLE
#include <exec/types.h>
#include <exec/lists.h>
#include <exec/memory.h>
#include <exec/resident.h>
#include <aros/libcall.h>
#include <exec/semaphores.h>

#include <proto/exec.h>

#include "datatypes_intern.h"
#include "libdefs.h"

#define INIT AROS_SLIB_ENTRY(init, DataTypes)
static const char name[];
static const char version[];
static const APTR inittabl[4];
static const void * const LIBFUNCTABLE[];
LIBBASETYPE * INIT();
extern const char LIBEND;

int entry(void)
{
    return -1;
}

const struct Resident DataTypes_resident =
{
    RTC_MATCHWORD,
    (struct Resident *)&DataTypes_resident,
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
    (APTR)sizeof(LIBBASETYPE),
    (APTR)LIBFUNCTABLE,
    NULL,
    &INIT
};


static BOOL openlibs(LIBBASETYPE *DTBase)
{
    if(DTBase->dtb_IntuitionBase == NULL)
	DTBase->dtb_IntuitionBase = OpenLibrary("intuition.library", 39);
    if(DTBase->dtb_IntuitionBase == NULL)
	return FALSE;

    if(DTBase->dtb_LayersBase == NULL)
	DTBase->dtb_LayersBase = OpenLibrary("layers.library", 37);
    if(DTBase->dtb_LayersBase == NULL)
	return FALSE;

    if(DTBase->dtb_UtilityBase == NULL)
	DTBase->dtb_UtilityBase = OpenLibrary("utility.library", 37);
    if(DTBase->dtb_UtilityBase == NULL)
	return FALSE;

    if(DTBase->dtb_DOSBase == NULL)
	DTBase->dtb_DOSBase = OpenLibrary("dos.library", 37);
    if(DTBase->dtb_DOSBase == NULL)
	return FALSE;

    if(DTBase->dtb_IFFParseBase == NULL)
	DTBase->dtb_IFFParseBase = OpenLibrary("iffparse.library", 37);
    if(DTBase->dtb_IFFParseBase == NULL)
	return FALSE;

    if(DTBase->dtb_LocaleBase == NULL)
	DTBase->dtb_LocaleBase = OpenLibrary("locale.library", 37);
    if(DTBase->dtb_LocaleBase == NULL)
	return FALSE;

    if(DTBase->dtb_GfxBase == NULL)
	DTBase->dtb_GfxBase = OpenLibrary("graphics.library", 37);
    if(DTBase->dtb_GfxBase == NULL)
	return FALSE;

    if(DTBase->dtb_IconBase == NULL)
	DTBase->dtb_IconBase = OpenLibrary("icon.library", 37);
    if(DTBase->dtb_IconBase == NULL)
	return FALSE;

    return TRUE;
}

static void closelibs(LIBBASETYPE *DTBase)
{
    if(DTBase->dtb_IntuitionBase != NULL)
	CloseLibrary(DTBase->dtb_IntuitionBase);

    if(DTBase->dtb_LayersBase != NULL)
	CloseLibrary(DTBase->dtb_LayersBase);

    if(DTBase->dtb_UtilityBase != NULL)
	CloseLibrary(DTBase->dtb_UtilityBase);

    if(DTBase->dtb_DOSBase != NULL)
	CloseLibrary(DTBase->dtb_DOSBase);

    if(DTBase->dtb_IFFParseBase != NULL)
	CloseLibrary(DTBase->dtb_IFFParseBase);

    if(DTBase->dtb_LocaleBase != NULL)
	CloseLibrary(DTBase->dtb_LocaleBase);

    if(DTBase->dtb_GfxBase != NULL)
	CloseLibrary(DTBase->dtb_GfxBase);

    if(DTBase->dtb_IconBase != NULL)
	CloseLibrary(DTBase->dtb_IconBase);
}


AROS_LH2(LIBBASETYPE *, init,
    AROS_LHA(LIBBASETYPE *,  DTBase, D0),
    AROS_LHA(BPTR,                  segList, A0),
    struct ExecBase *, sysBase, 0, DataTypes)
{
    AROS_LIBFUNC_INIT
    int i;

    DTBase->dtb_SysBase = sysBase;
    __dt_GlobalSysBase = (struct Library *)sysBase;
    DTBase->dtb_SegList = segList;
    
    for(i = 0; i < SEM_MAX; i++)
    {
	InitSemaphore(&DTBase->dtb_Semaphores[i]);
    }

    return DTBase;

    AROS_LIBFUNC_EXIT
}


AROS_LH1(LIBBASETYPE *, open,
    AROS_LHA(ULONG, version, D0),
    LIBBASETYPE *, DTBase, 1, DataTypes)
{
    AROS_LIBFUNC_INIT

    /* keep compiler happy */
    version = 0;

    /* We have to open some libraries. */
    if(DTBase->dtb_DOSBase == NULL )
    {
	if(!openlibs(DTBase))
	{
	    closelibs(DTBase);
	    return NULL;
	}

	if(DTBase->dtb_LibsCatalog == NULL)
	    DTBase->dtb_LibsCatalog = opencatalog((struct Library *)DTBase,
						  NULL, "Sys/libs.catalog",
						  OC_BuiltInLanguage,
						  "english", TAG_DONE);

	DTBase->dtb_DTList = GetDataTypesList(DTBase);

	if(!InstallClass((struct Library *)DTBase))
	{
	    return NULL;
	}
    }

    /* What else do we have to do? */
    DTBase->dtb_LibNode.lib_OpenCnt++;
    DTBase->dtb_LibNode.lib_Flags &= ~LIBF_DELEXP;

    return DTBase;
    AROS_LIBFUNC_EXIT
}


AROS_LH0(BPTR, close,
    LIBBASETYPE *, DTBase, 2, DataTypes)
{
    AROS_LIBFUNC_INIT

    --DTBase->dtb_LibNode.lib_OpenCnt;

    /*
	We can never exit because of the system patches,
	But we can try and free some memory.
    */
    AROS_LC0(BPTR, expunge, LIBBASETYPE *, DTBase, 3, DataTypes);

    return 0;
    AROS_LIBFUNC_EXIT
}


AROS_LH0(BPTR, expunge,
    LIBBASETYPE *, DTBase, 3, DataTypes)
{
    AROS_LIBFUNC_INIT

    /* As I said above, we cannot remove ourselves. */
    DTBase->dtb_LibNode.lib_Flags &= ~LIBF_DELEXP;

    TryRemoveClass((struct Library *)DTBase);

    return 0;
    AROS_LIBFUNC_EXIT
}


AROS_LH0I(int, null,
    LIBBASETYPE *, LIBBASE, 4, DataTypes)
{
    AROS_LIBFUNC_INIT

    return 0;

    AROS_LIBFUNC_EXIT
}

