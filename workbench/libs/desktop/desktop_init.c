
#include <exec/types.h>
#include <exec/resident.h>
#include <proto/exec.h>
#include <aros/libcall.h>
#include <libraries/mui.h>

#include <proto/exec.h>
#include <proto/muimaster.h>

#include "desktop_intern.h"
#include "libdefs.h"
#include "initstruct.h"
#include "support.h"

#include "iconcontainerclass.h"
#include "iconcontainerobserver.h"

#include "desktop_intern_protos.h"

#include <stddef.h>

#define DEBUG 1
#include <aros/debug.h>

struct inittable;
extern const char name[];
extern const char version[];
extern const APTR inittabl[4];
extern void *const LIBFUNCTABLE[];
extern const struct inittable datatable;

extern struct DesktopBase *AROS_SLIB_ENTRY(init,Desktop)();
extern struct DesktopBase *AROS_SLIB_ENTRY(open,Desktop)();
extern BPTR AROS_SLIB_ENTRY(close,Desktop)();
extern BPTR AROS_SLIB_ENTRY(expunge,Desktop)();
extern int AROS_SLIB_ENTRY(null,Desktop)();
extern ULONG AROS_SLIB_ENTRY(add,Desktop)();
extern ULONG AROS_SLIB_ENTRY(asl,Desktop)();

extern const char end;

int entry(void)
{
    /* If the library was executed by accident return error code. */
    return -1;
}

const struct Resident resident=
{
    RTC_MATCHWORD,
    (struct Resident *)&resident,
    (APTR)&end,
    RTF_AUTOINIT,
    1,
    NT_LIBRARY,
    0,
    (char *)name,
    (char *)&version[6],
    (ULONG *)inittabl
};

const char name[]="desktop.library";

const char version[]="$VER: desktop.library 41.1 (29.08.02)\n\015";

const APTR inittabl[4]=
{
    (APTR)sizeof(struct DesktopBase),
    (APTR)LIBFUNCTABLE,
    (APTR)&datatable,
    &AROS_SLIB_ENTRY(init,Desktop)
};

struct inittable
{
    S_CPYO(1,1,B);
    S_CPYO(2,1,L);
    S_CPYO(3,1,B);
    S_CPYO(4,1,W);
    S_CPYO(5,1,W);
    S_CPYO(6,1,L);
    S_END (end);
};

#define O(n) offsetof(struct DesktopBase,n)

const struct inittable datatable=
{
    { { I_CPYO(1,B,O(db_Library.lib_Node.ln_Type)), { NT_LIBRARY } } },
    { { I_CPYO(1,L,O(db_Library.lib_Node.ln_Name)), { (IPTR)name } } },
    { { I_CPYO(1,B,O(db_Library.lib_Flags       )), { LIBF_SUMUSED|LIBF_CHANGED } } },
    { { I_CPYO(1,W,O(db_Library.lib_Version     )), { 1 } } },
    { { I_CPYO(1,W,O(db_Library.lib_Revision    )), { 0 } } },
    { { I_CPYO(1,L,O(db_Library.lib_IdString    )), { (IPTR)&version[6] } } },
	I_END ()
};

#undef O

struct DesktopBase *DesktopBase;

#undef SysBase

AROS_LH2(struct DesktopBase *, init,
 AROS_LHA(struct DesktopBase *, desktopbase, D0),
 AROS_LHA(BPTR,               segList,   A0),
	   struct ExecBase *, SysBase, 0, BASENAME)
{
    AROS_LIBFUNC_INIT
    /* This function is single-threaded by exec by calling Forbid. */

	DesktopBase=desktopbase;

	InitSemaphore(&DesktopBase->db_BaseMutex);
	InitSemaphore(&DesktopBase->db_HandlerSafety);

	D(bug("*** Entering DesktopBase::init...\n"));

	DesktopBase->db_libsOpen=FALSE;

    /* Store arguments */
    DesktopBase->db_SysBase=SysBase;
    DesktopBase->db_SegList=segList;
	DesktopBase->db_HandlerPort=NULL;

	D(bug("*** Exitiing DesktopBase::init...\n"));

    /* You would return NULL here if the init failed. */
    return DesktopBase;
    AROS_LIBFUNC_EXIT
}

/* Use this from now on */
#ifdef SysBase
#undef SysBase
#endif
#define SysBase DesktopBase->db_SysBase

AROS_LH1(struct DesktopBase *, open,
 AROS_LHA(ULONG, version, D0),
	   struct DesktopBase *, desktopbase, 1, BASENAME)
{
    AROS_LIBFUNC_INIT
    /*
	This function is single-threaded by exec by calling Forbid.
	If you break the Forbid() another task may enter this function
	at the same time. Take care.
    */

	D(bug("*** Entered DesktopBase::open...\n"));

	ObtainSemaphore(&DesktopBase->db_BaseMutex);

    /* Keep compiler happy */
    version=0;

    /* I have one more opener. */
	DesktopBase->db_Library.lib_OpenCnt++;
    DesktopBase->db_Library.lib_Flags&=~LIBF_DELEXP;

	if(DesktopBase->db_libsOpen==FALSE)
	{
		// Any of these could potentially break the Forbid(),
		// so we have a semaphore
		DesktopBase->db_DOSBase=OpenLibrary("dos.library", 0);
		if(!DesktopBase->db_DOSBase)
			return NULL;

		DesktopBase->db_GfxBase=OpenLibrary("graphics.library", 0);
		if(!DesktopBase->db_GfxBase)
			return NULL;

		DesktopBase->db_IntuitionBase=OpenLibrary("intuition.library", 0);
		if(!DesktopBase->db_IntuitionBase)
			return NULL;

		DesktopBase->db_LayersBase=OpenLibrary("layers.library", 0);
		if(!DesktopBase->db_LayersBase)
			return NULL;

		DesktopBase->db_UtilityBase=OpenLibrary("utility.library", 0);
		if(!DesktopBase->db_UtilityBase)
			return NULL;

		DesktopBase->db_MUIMasterBase=OpenLibrary("muimaster.library", 0);
		if(!DesktopBase->db_MUIMasterBase)
			return NULL;

		DesktopBase->db_IconContainer=MUI_CreateCustomClass(NULL, MUIC_Area, NULL, sizeof(struct IconContainerClassData), iconContainerDispatcher);
		if(!DesktopBase->db_IconContainer)
			return NULL;

		DesktopBase->db_IconContainerObserver=MUI_CreateCustomClass(NULL, MUIC_Notify, NULL, sizeof(struct IconContainerObserverClassData), iconContainerObserverDispatcher);
		if(!DesktopBase->db_IconContainerObserver)
			return NULL;

		DesktopBase->db_libsOpen=TRUE;
	}

	if(!DesktopBase->db_HandlerPort)
		startDesktopHandler();

	handlerAddUser();

	D(bug("*** Exiting DesktopBase::open...\n"));

	ReleaseSemaphore(&DesktopBase->db_BaseMutex);

    /* You would return NULL if the open failed. */
	return DesktopBase;
    AROS_LIBFUNC_EXIT
}

AROS_LH0(BPTR, close, struct DesktopBase *, DesktopBase, 2, Desktop)
{
    AROS_LIBFUNC_INIT
    /*
	This function is single-threaded by exec by calling Forbid.
	If you break the Forbid() another task may enter this function
	at the same time. Take care.
    */

	D(bug("*** Entering DesktopBase::close...\n"));

	handlerSubUser();

    /* I have one fewer opener. */
    if(!--DesktopBase->db_Library.lib_OpenCnt)
    {
		/* Delayed expunge pending? */
		if(DesktopBase->db_Library.lib_Flags&LIBF_DELEXP)
	    	/* Then expunge the library */
			/* At this point the handler should have exited
			   on its own, and completed the final CloseLibrary() */
		    return expunge();
    }

	D(bug("*** Exiting DesktopBase::close...\n"));

	return 0;
    AROS_LIBFUNC_EXIT
}

AROS_LH0(BPTR, expunge, struct DesktopBase *, DesktopBase, 3, BASENAME)
{
    AROS_LIBFUNC_INIT

    BPTR ret;
    /*
	This function is single-threaded by exec by calling Forbid.
	Never break the Forbid() or strange things might happen.
    */
	D(bug("*** Entering DesktopBase::expunge...\n"));

    /* Test for openers. */
    if(DesktopBase->db_Library.lib_OpenCnt)
    {
		/* Set the delayed expunge flag and return. */
		DesktopBase->db_Library.lib_Flags|=LIBF_DELEXP;
		return 0;
    }

	if(DesktopBase->db_IconContainerObserver)
		MUI_DeleteCustomClass(DesktopBase->db_IconContainerObserver);
	if(DesktopBase->db_IconContainer)
		MUI_DeleteCustomClass(DesktopBase->db_IconContainer);
	if(DesktopBase->db_MUIMasterBase)
		CloseLibrary(DesktopBase->db_MUIMasterBase);
	if(DesktopBase->db_UtilityBase)
		CloseLibrary(DesktopBase->db_UtilityBase);
	if(DesktopBase->db_LayersBase)
		CloseLibrary(DesktopBase->db_LayersBase);
	if(DesktopBase->db_IntuitionBase)
		CloseLibrary(DesktopBase->db_IntuitionBase);
	if(DesktopBase->db_GfxBase)
		CloseLibrary(DesktopBase->db_GfxBase);
	if(DesktopBase->db_DOSBase)
		CloseLibrary(DesktopBase->db_DOSBase);

    /* Get rid of the library. Remove it from the list. */
    Remove(&DesktopBase->db_Library.lib_Node);

    /* Get returncode here - FreeMem() will destroy the field. */
    ret=DesktopBase->db_SegList;

    /* Free the memory. */
    FreeMem((char *)DesktopBase-DesktopBase->db_Library.lib_NegSize,
	    DesktopBase->db_Library.lib_NegSize+DesktopBase->db_Library.lib_PosSize);

	D(bug("*** Exiting DesktopBase::expunge...\n"));

    return ret;
    AROS_LIBFUNC_EXIT
}

AROS_LH0I(int, null, struct DesktopBase *, DesktopBase, 4, Desktop)
{
    AROS_LIBFUNC_INIT
	return 0;
    AROS_LIBFUNC_EXIT
}

AROS_LH2I(ULONG, add,
    AROS_LHA(ULONG,a,D0),
    AROS_LHA(ULONG,b,D1),
    struct DesktopBase *,DesktopBase,5,Desktop)
{
    AROS_LIBFUNC_INIT
    return a+b;
    AROS_LIBFUNC_EXIT
}

AROS_LH2I(ULONG, asl,
    AROS_LHA(ULONG,a,D0),
    AROS_LHA(ULONG,b,D1),
    struct DesktopBase *,DesktopBase,6,Desktop)
{
    AROS_LIBFUNC_INIT
    return a<<b;
    AROS_LIBFUNC_EXIT
}

const char end=0;
