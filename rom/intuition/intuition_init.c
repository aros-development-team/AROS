/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc: Library header for intuition
    Lang: english
*/
#define AROS_ALMOST_COMPATIBLE
#include <string.h>
#include <exec/lists.h>
#include <exec/resident.h>
#include <exec/memory.h>
#include <exec/execbase.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/alib.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <dos/dostags.h>
#include <proto/dos.h>
#ifndef INTUITION_CLASSES_H
#   include <intuition/classes.h>
#endif
#ifndef UTILITY_HOOKS_H
#   include <utility/hooks.h>
#endif
#include <utility/utility.h>
#include <aros/asmcall.h>
#include "libdefs.h"
#include "intuition_intern.h"

static const char name[];
static const char version[];
static const APTR inittabl[4];
static void *const FUNCTABLE[];
struct LIBBASETYPE *INIT ();
extern const char END;

AROS_UFP3(static ULONG, rootDispatcher,
    AROS_UFPA(Class *,  cl,  A0),
    AROS_UFPA(Object *, obj, A2),
    AROS_UFPA(Msg,      msg, A1)
);

/* There has to be a better way... */
struct IClass *InitImageClass (struct LIBBASETYPE * LIBBASE);
struct IClass *InitFrameIClass (struct LIBBASETYPE * LIBBASE);
struct IClass *InitICClass (struct LIBBASETYPE * LIBBASE);
struct IClass *InitGadgetClass (struct LIBBASETYPE * LIBBASE);
struct IClass *InitButtonGClass (struct LIBBASETYPE * LIBBASE);

int Intuition_entry(void)
{
    /* If the library was executed by accident return error code. */
    return -1;
}

const struct Resident Intuition_resident=
{
    RTC_MATCHWORD,
    (struct Resident *)&Intuition_resident,
    (APTR)&END,
    RTF_AUTOINIT,
    LIBVERSION,
    NT_LIBRARY,
    0,
    (char *)name,
    (char *)&version[6],
    (ULONG *)inittabl
};

static const char name[]=INTUITIONNAME;

static const char version[]=VERSION;

static const APTR inittabl[4]=
{
    (APTR)sizeof(struct IntIntuitionBase),
    (APTR)FUNCTABLE,
    NULL,
    &INIT
};

static Class rootclass =
{
    { { NULL, NULL }, AROS_ASMSYMNAME(rootDispatcher), NULL, NULL },
    0,		/* reserved */
    NULL,	/* No superclass */
    (ClassID)ROOTCLASS,  /* ClassID */

    0, 0,	/* No offset and size */

    0,		/* UserData */
    0,		/* SubClassCount */
    0,		/* ObjectCount */
    0,		/* Flags */
};

void intui_ProcessEvents (void);

struct Process * inputDevice;

AROS_LH2(struct LIBBASETYPE *, init,
 AROS_LHA(struct LIBBASETYPE *, LIBBASE, D0),
 AROS_LHA(BPTR,               segList,   A0),
	   struct ExecBase *, sysBase, 0, Intuition)
{
    AROS_LIBFUNC_INIT
    SysBase = sysBase;

    NEWLIST (PublicClassList);

    if (!intui_init (LIBBASE))
	return NULL;

    /* Create semaphore and initialize it */
    GetPrivIBase(LIBBASE)->IBaseLock = AllocMem (sizeof(struct SignalSemaphore), MEMF_PUBLIC|MEMF_CLEAR);

    if (!GetPrivIBase(LIBBASE)->IBaseLock)
	return NULL;

    InitSemaphore (GetPrivIBase(LIBBASE)->IBaseLock);

    /* Create semaphore and initialize it */
    GetPrivIBase(LIBBASE)->ClassListLock = AllocMem (sizeof(struct SignalSemaphore), MEMF_PUBLIC|MEMF_CLEAR);

    if (!GetPrivIBase(LIBBASE)->ClassListLock)
	return NULL;

    InitSemaphore (GetPrivIBase(LIBBASE)->ClassListLock);

    /* The rootclass is created statically */
    rootclass.cl_UserData = (IPTR) LIBBASE;
    AddClass (&rootclass);

    /* Add all other classes */
    InitImageClass (LIBBASE); /* After ROOTCLASS */
    InitFrameIClass (LIBBASE); /* After IMAGECLASS */

    InitICClass (LIBBASE); /* After ROOTCLASS */

    InitGadgetClass (LIBBASE); /* After ROOTCLASS */
    InitButtonGClass (LIBBASE); /* After GADGETCLASS */

    /* You would return NULL if the init failed */
    return LIBBASE;
    AROS_LIBFUNC_EXIT
}

AROS_LH1(struct LIBBASETYPE *, open,
 AROS_LHA(ULONG, version, D0),
	   struct LIBBASETYPE *, LIBBASE, 1, Intuition)
{
    AROS_LIBFUNC_INIT
    struct TagItem screenTags[] =
    {
	{ SA_Depth, 4			},
	{ SA_Type,  WBENCHSCREEN	},
	{ SA_Title, (IPTR)"Workbench"   },
	{ TAG_END, 0 }
    };
    struct TagItem inputTask[]=
    {
	{ NP_UserData,	0L },
	{ NP_Entry,	(IPTR)intui_ProcessEvents },
	{ NP_Input,	0L },
	{ NP_Output,	0L },
	{ NP_Name,	(IPTR)"input.device" },
	{ NP_Priority,	50 },
	{ TAG_END, 0 }
    };

    /* Keep compiler happy */
    version=0;

    /* TODO Create input.device. This is a bad hack. */
    if (!inputDevice)
    {
	inputTask[0].ti_Data = (IPTR)LIBBASE;

	inputDevice = CreateNewProc (inputTask);

	if (!inputDevice)
	    return NULL;
    }

    if (!GfxBase)
    {
	if (!(GfxBase = (void *)OpenLibrary (GRAPHICSNAME, 39)) )
	    return NULL;
    }

    if (!UtilityBase)
    {
	if (!(UtilityBase = (void *)OpenLibrary (UTILITYNAME, 39)) )
	    return NULL; /* don't close anything */
    }

    if (!GetPrivIBase(LIBBASE)->WorkBench)
    {
	struct Screen * screen;

	screen = OpenScreenTagList (NULL, screenTags);

	if (!screen)
	    return NULL;

	LIBBASE->FirstScreen =
	    LIBBASE->ActiveScreen =
	    GetPrivIBase(LIBBASE)->WorkBench = screen;
    }

    if (!intui_open (LIBBASE))
	return NULL;

    /* I have one more opener. */
    LIBBASE->LibNode.lib_OpenCnt++;
    LIBBASE->LibNode.lib_Flags&=~LIBF_DELEXP;

    /* You would return NULL if the open failed. */
    return LIBBASE;
    AROS_LIBFUNC_EXIT
}

AROS_LH0(BPTR, close,
	   struct LIBBASETYPE *, LIBBASE, 2, Intuition)
{
    AROS_LIBFUNC_INIT

    /* I have one fewer opener. */
    if(!--LIBBASE->LibNode.lib_OpenCnt)
    {
	intui_close (LIBBASE);

	/* Delayed expunge pending? */
	if(LIBBASE->LibNode.lib_Flags&LIBF_DELEXP)
	    /* Then expunge the library */
	    return expunge();
    }
    return 0;
    AROS_LIBFUNC_EXIT
}

AROS_LH0(BPTR, expunge,
	   struct LIBBASETYPE *, LIBBASE, 3, Intuition)
{
    AROS_LIBFUNC_INIT

    /* Test for openers. */
    if(LIBBASE->LibNode.lib_OpenCnt)
    {
	/* Set the delayed expunge flag and return. */
	LIBBASE->LibNode.lib_Flags|=LIBF_DELEXP;
	return 0;
    }

    /* Free unecessary memory */
    if (GetPrivIBase(LIBBASE)->WorkBench)
	CloseScreen (GetPrivIBase(LIBBASE)->WorkBench);

    if (UtilityBase)
	CloseLibrary ((struct Library *)UtilityBase);

    if (GfxBase)
	CloseLibrary ((struct Library *)GfxBase);

    /* Let the driver do the same */
    intui_expunge (LIBBASE);

#ifdef DISK_BASED /* Don't remove a ROM library */
    FreeImageClass ();

    /* Get rid of the library. Remove it from the list. */
    Remove(&LIBBASE->LibNode.lib_Node);

    /* Free the memory. */
    FreeMem((char *)LIBBASE-LIBBASE->LibNode.lib_NegSize,
	    LIBBASE->LibNode.lib_NegSize+LIBBASE->LibNode.lib_PosSize);
#endif

    return 0L;
    AROS_LIBFUNC_EXIT
}

AROS_LH0I(int, null,
	    struct LIBBASETYPE *, LIBBASE, 4, Intuition)
{
    AROS_LIBFUNC_INIT
    return 0;
    AROS_LIBFUNC_EXIT
}

Class * FindClass (ClassID classID, struct LIBBASETYPE * LIBBASE)
{
    Class * classPtr;

    /* Lock the list */
    ObtainSemaphoreShared (GetPrivIBase(LIBBASE)->ClassListLock);

    /* Search for the class */
    ForeachNode (PublicClassList, classPtr)
    {
	if (!strcmp (classPtr->cl_ID, classID))
	    goto found;
    }

    classPtr = NULL; /* Nothing found */

found:
    /* Unlock list */
    ReleaseSemaphore (GetPrivIBase(LIBBASE)->ClassListLock);

    return classPtr;
}

#undef IntuitionBase
#define IntuitionBase	((struct LIBBASETYPE *)(cl->cl_UserData))

/******************************************************************************

    NAME */
	AROS_UFH3(static IPTR, rootDispatcher,

/*  SYNOPSIS */
	AROS_UFHA(Class  *, cl,  A0),
	AROS_UFHA(Object *, o,   A2),
	AROS_UFHA(Msg,      msg, A1))

/*  FUNCTION
	internal !

	Processes all messages sent to the RootClass. Unknown messages are
	silently ignored.

    INPUTS
	cl - Pointer to the RootClass
	o - This object was the destination for the message in the first
		place
	msg - This is the message.

    RESULT
	Processes the message. The meaning of the result depends on the
	type of the message.

    NOTES
	This is a good place to debug BOOPSI objects since every message
	should eventually show up here.

    EXAMPLE

    BUGS

    SEE ALSO

    HISTORY
	14.09.93    ada created

******************************************************************************/
{
    AROS_USERFUNC_INIT
    IPTR retval = 0;

    switch (msg->MethodID)
    {
    case OM_NEW: {
	cl = _OBJECT(o)->o_Class;

	/* Get memory. The objects shows how much is needed.
	   (The object is not an object, it is a class pointer!) */
	retval = (IPTR) AllocMem (cl->cl_InstOffset
		+ cl->cl_InstSize
		+ sizeof (struct _Object)
	    , MEMF_ANY
	    );

	retval = (IPTR) BASEOBJECT(retval);
	break; }

    case OM_DISPOSE:
	/* Free memory. Caller is responsible that everything else
	   is already cleared! */
	FreeMem (_OBJECT(o)
	    , cl->cl_InstOffset
		+ cl->cl_InstSize
		+ sizeof (struct _Object)
	    );
	break;

    case OM_ADDTAIL:
	/* Add <o> to list. */
	AddTail (((struct opAddTail *)msg)->opat_List,
		    (struct Node *) _OBJECT(o));
	break;

    case OM_REMOVE:
	/* Remove object from list. */
	Remove ((struct Node *) _OBJECT(o));
	break;

    default:
	/* Ignore */
	break;

    } /* switch */

    return (retval);
    AROS_USERFUNC_EXIT
} /* rootDispatcher */



