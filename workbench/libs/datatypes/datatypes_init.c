/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/types.h>
#include <exec/lists.h>
#include <exec/memory.h>
#include <exec/resident.h>
#include <aros/libcall.h>
#include <exec/semaphores.h>
#include <aros/asmcall.h>

#include <proto/exec.h>

#include <aros/debug.h>

#include "datatypes_intern.h"
#include LC_LIBDEFS_FILE

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
    (APTR)sizeof(struct DataTypesBase),
    (APTR)LIBFUNCTABLE,
    NULL,
    &INIT
};

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

    if (DataTypesBase->dtb_WorkbenchBase != NULL)
	CloseLibrary(DataTypesBase->dtb_WorkbenchBase);
}


AROS_UFH3(struct DataTypesBase *, AROS_SLIB_ENTRY(init,DataTypes),
    AROS_UFHA(struct DataTypesBase *,  DataTypesBase, D0),
    AROS_UFHA(BPTR,                  segList, A0),
    AROS_UFHA(struct ExecBase *, sysBase, A6)
)
{
    AROS_USERFUNC_INIT
    int i;

    DataTypesBase->dtb_SysBase = sysBase;
#undef SysBase
    SysBase = (struct Library *)sysBase;
#define SysBase ((struct DataTypesBase *)DataTypesBase)->dtb_SysBase
    DataTypesBase->dtb_SegList = segList;
    
    for (i = 0; i < SEM_MAX; i++)
    {
	InitSemaphore(&DataTypesBase->dtb_Semaphores[i]);
    }

    /*
     * Open libraries. These should all exist at init time, so there is no
     * point not opening them now. If they happen to fail, we do not load
     * the library. Under debugging mode print out why though. I don't
     * alert because a) datatypes isn't that important, and b) it may be a
     * version problem. In either case I don't want to bring down the
     * system unnecessarily.
     */

    if ((DataTypesBase->dtb_UtilityBase =
	    OpenLibrary("utility.library", 39L)) == NULL)
    {
	D(bug("datatypes.library: Cannot open utility.library\n"));
	goto error;
    }

    if ((DataTypesBase->dtb_GfxBase =
	    OpenLibrary("graphics.library", 39L)) == NULL)
    {
	D(bug("datatypes.library: Cannot open graphics.library\n"));
	goto error;
    }

    if ((DataTypesBase->dtb_LayersBase = 
	    OpenLibrary("layers.library", 39L)) == NULL)
    {
	D(bug("datatypes.library: Cannot open layers.library\n"));
	goto error;
    }

    if ((DataTypesBase->dtb_IntuitionBase =
	    OpenLibrary("intuition.library", 39L)) == NULL)
    {
	D(bug("datatypes.library: Cannot open intuition.library\n"));
	goto error;
    }

    if ((DataTypesBase->dtb_DOSBase =
	    OpenLibrary("dos.library", 37L)) == NULL)
    {
	D(bug("datatypes.library: Cannot open dos.library\n"));
	goto error;
    }

    /* We may not have these libraries, but try anyway */
    if ((DataTypesBase->dtb_IFFParseBase =
	    OpenLibrary("iffparse.library", 37L)) == NULL)
    {
	D(bug("datatypes.library: Cannot open iffparse.library\n"));
	goto error;
    }

    if ((DataTypesBase->dtb_LocaleBase =
	    OpenLibrary("locale.library", 0L)) == NULL)
    {
	D(bug("datatypes.library: Cannot open locale.library\n"));
	goto error;
    }

    if ((DataTypesBase->dtb_WorkbenchBase =
	    OpenLibrary("workbench.library", 37L)) == NULL)
    {
	D(bug("datatypes.library: Cannot open workbench.library\n"));
	goto error;
    }

    /* Get the list of datatypes */
    DataTypesBase->dtb_DTList = GetDataTypesList(DataTypesBase);

    if(!InstallClass((struct Library *)DataTypesBase))
    {
	return NULL;
    }

    /* Try opening the catalog, don't worry if we fail, just keep trying. */
    DataTypesBase->dtb_LibsCatalog =
	opencatalog
	(
	    (struct Library *)DataTypesBase,
	    NULL,
	    "Sys/libs.catalog",
	    OC_BuiltInLanguage,
	    "english",
	    TAG_DONE
	);

    return DataTypesBase;

error:
    closelibs(DataTypesBase);

    return NULL;

    AROS_USERFUNC_EXIT
}


AROS_LH1(struct DataTypesBase *, open,
    AROS_LHA(ULONG, version, D0),
    struct DataTypesBase *, DataTypesBase, 1, DataTypes)
{
    AROS_LIBFUNC_INIT

    /* Keep the compiler happy */
    version = 0;

    /* Try opening the catalog again. */
    if(DataTypesBase->dtb_LibsCatalog == NULL)
    {
	DataTypesBase->dtb_LibsCatalog =
	    opencatalog
	    (
		(struct Library *)DataTypesBase,
		NULL,
		"Sys/libs.catalog",
		OC_BuiltInLanguage,
		"english",
		TAG_DONE
	    );
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

#if 0
    /* stegerg: if later someone else re-opens datatypes.library, then
                the datatypes.class would have to be re-added with
		AddClass in libopen() (if FreeClass returned FALSE,
		where the class was not freed, but still removed),
		or re-make the class (when FreeClass returned TRUE) */
		
    TryRemoveClass((struct Library *)DataTypesBase);
#endif

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

