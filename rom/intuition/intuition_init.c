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
#include <clib/exec_protos.h>
#include <clib/intuition_protos.h>
#include <clib/alib_protos.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <dos/dostags.h>
#include <clib/dos_protos.h>
#ifndef INTUITION_CLASSES_H
#   include <intuition/classes.h>
#endif
#ifndef UTILITY_HOOKS_H
#   include <utility/hooks.h>
#endif
#include <utility/utility.h>
#include <aros/asmcall.h>
#include "intuition_intern.h"

static const char name[];
static const char version[];
static const APTR inittabl[4];
static void *const Intuition_functable[];
struct IntuitionBase *AROS_SLIB_ENTRY(init,Intuition) ();
extern const char Intuition_end;

extern int  intui_init (struct IntuitionBase *);
extern int  intui_open (struct IntuitionBase *);
extern void intui_close (struct IntuitionBase *);
extern void intui_expunge (struct IntuitionBase *);

AROS_UFP3(static ULONG, rootDispatcher,
    AROS_UFPA(Class *,  cl,  A0),
    AROS_UFPA(Object *, obj, A2),
    AROS_UFPA(Msg,      msg, A1)
);

/* There has to be a better way... */
struct IClass *InitImageClass (struct IntuitionBase * IntuitionBase);
struct IClass *InitFrameIClass (struct IntuitionBase * IntuitionBase);
struct IClass *InitICClass (struct IntuitionBase * IntuitionBase);
struct IClass *InitGadgetClass (struct IntuitionBase * IntuitionBase);
struct IClass *InitButtonGClass (struct IntuitionBase * IntuitionBase);

int Intuition_entry(void)
{
    /* If the library was executed by accident return error code. */
    return -1;
}

const struct Resident Intuition_resident=
{
    RTC_MATCHWORD,
    (struct Resident *)&Intuition_resident,
    (APTR)&Intuition_end,
    RTF_AUTOINIT,
    39,
    NT_LIBRARY,
    0,
    (char *)name,
    (char *)&version[6],
    (ULONG *)inittabl
};

static const char name[]="intuition.library";

static const char version[]="$VER: intuition.library 39.0 (12.8.96)\n\015";

static const APTR inittabl[4]=
{
    (APTR)sizeof(struct IntIntuitionBase),
    (APTR)Intuition_functable,
    NULL,
    &AROS_SLIB_ENTRY(init,Intuition)
};

static Class rootclass =
{
    { { NULL, NULL }, AROS_ASMFUNC_NAME(rootDispatcher), NULL, NULL },
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

AROS_LH2(struct IntuitionBase *, init,
 AROS_LHA(struct IntuitionBase *, IntuitionBase, D0),
 AROS_LHA(BPTR,               segList,   A0),
	   struct ExecBase *, sysBase, 0, Intuition)
{
    AROS_LIBFUNC_INIT
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

    SysBase = sysBase;

    NEWLIST (PublicClassList);

    if (!intui_init (IntuitionBase))
	return NULL;

    /* Create semaphore and initialize it */
    GetPrivIBase(IntuitionBase)->SigSem = AllocMem (sizeof(struct SignalSemaphore), MEMF_PUBLIC|MEMF_CLEAR);

    if (!GetPrivIBase(IntuitionBase)->SigSem)
	return NULL;

    InitSemaphore (GetPrivIBase(IntuitionBase)->SigSem);

    /* The rootclass is created statically */
    rootclass.cl_UserData = (IPTR) IntuitionBase;
    AddClass (&rootclass);

    /* Add all other classes */
    InitImageClass (IntuitionBase); /* After ROOTCLASS */
    InitFrameIClass (IntuitionBase); /* After IMAGECLASS */

    InitICClass (IntuitionBase); /* After ROOTCLASS */

    InitGadgetClass (IntuitionBase); /* After ROOTCLASS */
    InitButtonGClass (IntuitionBase); /* After GADGETCLASS */

    /* TODO Create input.device. This is a bad hack. */
    inputTask[0].ti_Data = (IPTR)IntuitionBase;

    inputDevice = CreateNewProc (inputTask);

    /* You would return NULL if the init failed */
    return IntuitionBase;
    AROS_LIBFUNC_EXIT
}

AROS_LH1(struct IntuitionBase *, open,
 AROS_LHA(ULONG, version, D0),
	   struct IntuitionBase *, IntuitionBase, 1, Intuition)
{
    AROS_LIBFUNC_INIT
    struct TagItem screenTags[] =
    {
	{ SA_Depth, 4			},
	{ SA_Type,  WBENCHSCREEN	},
	{ SA_Title, (IPTR)"Workbench"   },
	{ TAG_END, 0 }
    };

    /* Keep compiler happy */
    version=0;

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

    if (!GetPrivIBase(IntuitionBase)->WorkBench)
    {
	struct Screen * screen;

	screen = OpenScreenTagList (NULL, screenTags);

	if (!screen)
	    return NULL;

	IntuitionBase->FirstScreen =
	    IntuitionBase->ActiveScreen =
	    GetPrivIBase(IntuitionBase)->WorkBench = screen;
    }

    if (!intui_open (IntuitionBase))
	return NULL;

    /* I have one more opener. */
    IntuitionBase->LibNode.lib_OpenCnt++;
    IntuitionBase->LibNode.lib_Flags&=~LIBF_DELEXP;

    /* You would return NULL if the open failed. */
    return IntuitionBase;
    AROS_LIBFUNC_EXIT
}

AROS_LH0(BPTR, close,
	   struct IntuitionBase *, IntuitionBase, 2, Intuition)
{
    AROS_LIBFUNC_INIT

    /* I have one fewer opener. */
    if(!--IntuitionBase->LibNode.lib_OpenCnt)
    {
	intui_close (IntuitionBase);

	/* Delayed expunge pending? */
	if(IntuitionBase->LibNode.lib_Flags&LIBF_DELEXP)
	    /* Then expunge the library */
	    return expunge();
    }
    return 0;
    AROS_LIBFUNC_EXIT
}

AROS_LH0(BPTR, expunge,
	   struct IntuitionBase *, IntuitionBase, 3, Intuition)
{
    AROS_LIBFUNC_INIT

    /* Test for openers. */
    if(IntuitionBase->LibNode.lib_OpenCnt)
    {
	/* Set the delayed expunge flag and return. */
	IntuitionBase->LibNode.lib_Flags|=LIBF_DELEXP;
	return 0;
    }

    /* Free unecessary memory */
    if (GetPrivIBase(IntuitionBase)->WorkBench)
	CloseScreen (GetPrivIBase(IntuitionBase)->WorkBench);

    if (UtilityBase)
	CloseLibrary ((struct Library *)UtilityBase);

    if (GfxBase)
	CloseLibrary ((struct Library *)GfxBase);

    /* Let the driver do the same */
    intui_expunge (IntuitionBase);

#ifdef DISK_BASED /* Don't remove a ROM library */
    FreeImageClass ();

    /* Get rid of the library. Remove it from the list. */
    Remove(&IntuitionBase->LibNode.lib_Node);

    /* Free the memory. */
    FreeMem((char *)IntuitionBase-IntuitionBase->LibNode.lib_NegSize,
	    IntuitionBase->LibNode.lib_NegSize+IntuitionBase->LibNode.lib_PosSize);
#endif

    return 0L;
    AROS_LIBFUNC_EXIT
}

AROS_LH0I(int, null,
	    struct IntuitionBase *, IntuitionBase, 4, Intuition)
{
    AROS_LIBFUNC_INIT
    return 0;
    AROS_LIBFUNC_EXIT
}

#undef IntuitionBase
#define IntuitionBase	((struct IntuitionBase *)(cl->cl_UserData))

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

    HISTORY:
	14.09.93    ada created

******************************************************************************/
{
    AROS_USERFUNC_INIT
    IPTR retval = 0;

    switch (msg->MethodID)
    {
    case OM_NEW: {
	cl = _OBJECT(o)->o_Class;

	/* Nur Speicher besorgen. Im Object steht, wieviel.
	   (Das Object ist keines. Es ist der Class-Pointer selbst !) */
	retval = (IPTR) AllocMem (cl->cl_InstOffset
		+ cl->cl_InstSize
		+ sizeof (struct _Object)
	    , MEMF_ANY
	    );

	retval = (IPTR) BASEOBJECT(retval);
	break; }

    case OM_DISPOSE:
	/* Speicher freigeben. Aufrufer ist verantwortlich,
	   dass bereits alles andere freigegeben wurde ! */
	FreeMem (_OBJECT(o)
	    , cl->cl_InstOffset
		+ cl->cl_InstSize
		+ sizeof (struct _Object)
	    );
	break;

    case OM_ADDTAIL:
	/* Fuege <o> an Liste an. */
	AddTail (((struct opAddTail *)msg)->opat_List,
		    (struct Node *) _OBJECT(o));
	break;

    case OM_REMOVE:
	/* Entferne Object aus der Liste */
	Remove ((struct Node *) _OBJECT(o));
	break;

    default:
	/* Ignore */
	break;

    } /* switch */

    return (retval);
    AROS_USERFUNC_EXIT
} /* rootDispatcher */



