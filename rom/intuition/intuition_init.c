/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.10  1996/09/21 14:18:48  digulla
    Create a screen with OpenScreen()
    Hand IntuitionBase to ROOTCLASS by UserData
    Don't close any libs on failure (the open-code tries to open more on the next
    	try)

    Revision 1.9  1996/09/17 18:46:42  digulla
    Added comment to express the experimental state of inputDevice at this place.

    Revision 1.8  1996/09/17 18:07:20  digulla
    DOSBase and SysBase are now declared in the respective header files.
    The type of DOSBase is "struct DosLibrary *". Fixed everywhere

    Revision 1.7  1996/09/17 16:14:26  digulla
    OpenWindowTagList() needs Utility.library

    Revision 1.6  1996/09/13 17:57:09  digulla
    Use IPTR

    Revision 1.5  1996/09/11 16:54:31  digulla
    Always use __AROS_SLIB_ENTRY() to access shared external symbols, because
	some systems name an external symbol "x" as "_x" and others as "x".
	(The problem arises with assembler symbols which might differ)

    Revision 1.4  1996/08/29 13:33:31  digulla
    Moved common code from driver to Intuition
    More docs

    Revision 1.3  1996/08/28 17:55:37  digulla
    Proportional gadgets
    BOOPSI

    Revision 1.2  1996/08/23 17:24:11  digulla
    Opening intuition.library called intui_init() instead of intui_open(). Ooops.

    Revision 1.1  1996/08/13 15:37:26  digulla
    First function for intuition.library


    Desc:
    Lang:
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
#include "intuition_intern.h"

static const char name[];
static const char version[];
static const APTR inittabl[4];
static void *const Intuition_functable[];
struct IntuitionBase *__AROS_SLIB_ENTRY(init,Intuition) ();
extern const char Intuition_end;

extern int  intui_init (struct IntuitionBase *);
extern int  intui_open (struct IntuitionBase *);
extern void intui_close (struct IntuitionBase *);
extern void intui_expunge (struct IntuitionBase *);

static ULONG rootDispatcher (Class *, Object *, Msg);

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
    &__AROS_SLIB_ENTRY(init,Intuition)
};

static Class rootclass =
{
    { { NULL, NULL }, rootDispatcher, NULL, NULL },
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

__AROS_LH2(struct IntuitionBase *, init,
 __AROS_LHA(struct IntuitionBase *, IntuitionBase, D0),
 __AROS_LHA(BPTR,               segList,   A0),
	   struct ExecBase *, sysBase, 0, Intuition)
{
    __AROS_FUNC_INIT
    struct TagItem inputTask[]=
    {
	{ NP_UserData,	0L },
	{ NP_Entry,	(IPTR)intui_ProcessEvents },
	{ NP_Input,	0L },
	{ NP_Output,	0L },
	{ NP_Name,	(IPTR)"input.device" },
	{ NP_StackSize, 100000 },
	{ NP_Priority,	50 },
	{ TAG_END, 0 }
    };

    SysBase = sysBase;

    NEWLIST (PublicClassList);

    if (!intui_init (IntuitionBase))
	return NULL;

    /* The rootclass is created statically */
    rootclass.cl_UserData = (IPTR) IntuitionBase;
    AddClass (&rootclass);

    /* TODO Create input.device. This is a bad hack. */
    inputTask[0].ti_Data = (IPTR)IntuitionBase;

    inputDevice = CreateNewProc (inputTask);

    /* You would return NULL if the init failed */
    return IntuitionBase;
    __AROS_FUNC_EXIT
}

__AROS_LH1(struct IntuitionBase *, open,
 __AROS_LHA(ULONG, version, D0),
	   struct IntuitionBase *, IntuitionBase, 1, Intuition)
{
    __AROS_FUNC_INIT
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
    __AROS_FUNC_EXIT
}

__AROS_LH0(BPTR, close,
	   struct IntuitionBase *, IntuitionBase, 2, Intuition)
{
    __AROS_FUNC_INIT

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
    __AROS_FUNC_EXIT
}

__AROS_LH0(BPTR, expunge,
	   struct IntuitionBase *, IntuitionBase, 3, Intuition)
{
    __AROS_FUNC_INIT

    BPTR ret;

    if (GetPrivIBase(IntuitionBase)->WorkBench)
	CloseScreen (GetPrivIBase(IntuitionBase)->WorkBench);

    if (UtilityBase)
	CloseLibrary ((struct Library *)UtilityBase);

    if (GfxBase)
	CloseLibrary ((struct Library *)GfxBase);

    /* Test for openers. */
    if(IntuitionBase->LibNode.lib_OpenCnt)
    {
	/* Set the delayed expunge flag and return. */
	IntuitionBase->LibNode.lib_Flags|=LIBF_DELEXP;
	return 0;
    }

    intui_expunge (IntuitionBase);

    /* Get rid of the library. Remove it from the list. */
    Remove(&IntuitionBase->LibNode.lib_Node);

    /* Get returncode here - FreeMem() will destroy the field. */
    ret=0L;

    /* Free the memory. */
    FreeMem((char *)IntuitionBase-IntuitionBase->LibNode.lib_NegSize,
	    IntuitionBase->LibNode.lib_NegSize+IntuitionBase->LibNode.lib_PosSize);

    return ret;
    __AROS_FUNC_EXIT
}

__AROS_LH0I(int, null,
	    struct IntuitionBase *, IntuitionBase, 4, Intuition)
{
    __AROS_FUNC_INIT
    return 0;
    __AROS_FUNC_EXIT
}

#undef IntuitionBase
#define IntuitionBase	((struct IntuitionBase *)(cl->cl_UserData))

/******************************************************************************

    NAME */
	static IPTR rootDispatcher (

/*  SYNOPSIS */
	Class  * cl,
	Object * o,
	Msg	 msg)

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
} /* rootDispatcher */



