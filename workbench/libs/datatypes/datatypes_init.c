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
struct DataTypesBase * INIT();
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


static BOOL openlibs(struct DataTypesBase *DataTypesBase)
{
    if(DataTypesBase->dtb_IntuitionBase == NULL)
	DataTypesBase->dtb_IntuitionBase = OpenLibrary("intuition.library", 39);
    if(DataTypesBase->dtb_IntuitionBase == NULL)
	return FALSE;

    if(DataTypesBase->dtb_LayersBase == NULL)
	DataTypesBase->dtb_LayersBase = OpenLibrary("layers.library", 37);
    if(DataTypesBase->dtb_LayersBase == NULL)
	return FALSE;

    if(DataTypesBase->dtb_UtilityBase == NULL)
	DataTypesBase->dtb_UtilityBase = OpenLibrary("utility.library", 37);
    if(DataTypesBase->dtb_UtilityBase == NULL)
	return FALSE;

    if(DataTypesBase->dtb_DOSBase == NULL)
	DataTypesBase->dtb_DOSBase = OpenLibrary("dos.library", 37);
    if(DataTypesBase->dtb_DOSBase == NULL)
	return FALSE;

    if(DataTypesBase->dtb_IFFParseBase == NULL)
	DataTypesBase->dtb_IFFParseBase = OpenLibrary("iffparse.library", 37);
    if(DataTypesBase->dtb_IFFParseBase == NULL)
	return FALSE;

    if(DataTypesBase->dtb_LocaleBase == NULL)
	DataTypesBase->dtb_LocaleBase = OpenLibrary("locale.library", 37);
    if(DataTypesBase->dtb_LocaleBase == NULL)
	return FALSE;

    if(DataTypesBase->dtb_GfxBase == NULL)
	DataTypesBase->dtb_GfxBase = OpenLibrary("graphics.library", 37);
    if(DataTypesBase->dtb_GfxBase == NULL)
	return FALSE;

    if(DataTypesBase->dtb_IconBase == NULL)
	DataTypesBase->dtb_IconBase = OpenLibrary("icon.library", 37);
    if(DataTypesBase->dtb_IconBase == NULL)
	return FALSE;

    return TRUE;
}

static void closelibs(struct DataTypesBase *DataTypesBase)
{
    if(DataTypesBase->dtb_IntuitionBase != NULL)
	CloseLibrary(DataTypesBase->dtb_IntuitionBase);

    if(DataTypesBase->dtb_LayersBase != NULL)
	CloseLibrary(DataTypesBase->dtb_LayersBase);

    if(DataTypesBase->dtb_UtilityBase != NULL)
	CloseLibrary(DataTypesBase->dtb_UtilityBase);

    if(DataTypesBase->dtb_DOSBase != NULL)
	CloseLibrary(DataTypesBase->dtb_DOSBase);

    if(DataTypesBase->dtb_IFFParseBase != NULL)
	CloseLibrary(DataTypesBase->dtb_IFFParseBase);

    if(DataTypesBase->dtb_LocaleBase != NULL)
	CloseLibrary(DataTypesBase->dtb_LocaleBase);

    if(DataTypesBase->dtb_GfxBase != NULL)
	CloseLibrary(DataTypesBase->dtb_GfxBase);

    if(DataTypesBase->dtb_IconBase != NULL)
	CloseLibrary(DataTypesBase->dtb_IconBase);
}


AROS_LH2(struct DataTypesBase *, init,
    AROS_LHA(struct DataTypesBase *,  DataTypesBase, D0),
    AROS_LHA(BPTR,                  segList, A0),
    struct ExecBase *, sysBase, 0, DataTypes)
{
    AROS_LIBFUNC_INIT
    int i;

    DataTypesBase->dtb_SysBase = sysBase;
    __dt_GlobalSysBase = (struct Library *)sysBase;
    DataTypesBase->dtb_SegList = segList;
    
    for(i = 0; i < SEM_MAX; i++)
    {
	InitSemaphore(&DataTypesBase->dtb_Semaphores[i]);
    }

    return DataTypesBase;

    AROS_LIBFUNC_EXIT
}


AROS_LH1(struct DataTypesBase *, open,
    AROS_LHA(ULONG, version, D0),
    struct DataTypesBase *, DataTypesBase, 1, DataTypes)
{
    AROS_LIBFUNC_INIT

    /* keep compiler happy */
    version = 0;

    /* We have to open some libraries. */
    if(DataTypesBase->dtb_DOSBase == NULL )
    {
	if(!openlibs(DataTypesBase))
	{
	    closelibs(DataTypesBase);
	    return NULL;
	}

	if(DataTypesBase->dtb_LibsCatalog == NULL)
	    DataTypesBase->dtb_LibsCatalog = opencatalog((struct Library *)DataTypesBase,
						  NULL, "Sys/libs.catalog",
						  OC_BuiltInLanguage,
						  "english", TAG_DONE);

	DataTypesBase->dtb_DTList = GetDataTypesList(DataTypesBase);

	if(!InstallClass((struct Library *)DataTypesBase))
	{
	    return NULL;
	}
    }

    /* What else do we have to do? */
    DataTypesBase->dtb_LibNode.lib_OpenCnt++;
    DataTypesBase->dtb_LibNode.lib_Flags &= ~LIBF_DELEXP;

    return DataTypesBase;
    AROS_LIBFUNC_EXIT
}


AROS_LH0(BPTR, close,
    struct DataTypesBase *, DataTypesBase, 2, DataTypes)
{
    AROS_LIBFUNC_INIT

    --DataTypesBase->dtb_LibNode.lib_OpenCnt;

    /*
	We can never exit because of the system patches,
	But we can try and free some memory.
    */
    AROS_LC0(BPTR, expunge, LIBBASETYPE *, DataTypesBase, 3, DataTypes);

    return 0;
    AROS_LIBFUNC_EXIT
}


AROS_LH0(BPTR, expunge,
    struct DataTypesBase *, DataTypesBase, 3, DataTypes)
{
    AROS_LIBFUNC_INIT

    /* As I said above, we cannot remove ourselves. */
    DataTypesBase->dtb_LibNode.lib_Flags &= ~LIBF_DELEXP;

    TryRemoveClass((struct Library *)DataTypesBase);

    return 0;
    AROS_LIBFUNC_EXIT
}


AROS_LH0I(int, null,
    struct DataTypesBase *, LIBBASE, 4, DataTypes)
{
    AROS_LIBFUNC_INIT

    return 0;

    AROS_LIBFUNC_EXIT
}

