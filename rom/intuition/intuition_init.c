/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Library header for intuition
    Lang: English
*/

/****************************************************************************************/


#define INIT AROS_SLIB_ENTRY(init,Intuition)

#include <string.h>
#include <exec/lists.h>
#include <exec/resident.h>
#include <exec/memory.h>
#include <exec/execbase.h>
#include <exec/alerts.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/alib.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <dos/dostags.h>
#include <proto/dos.h>
#include <proto/arossupport.h>
#include <devices/input.h>
#ifndef INTUITION_CLASSES_H
#   include <intuition/classes.h>
#endif
#ifndef UTILITY_HOOKS_H
#   include <utility/hooks.h>
#endif
#include <utility/utility.h>
#include <aros/asmcall.h>
#include LC_LIBDEFS_FILE
#include "intuition_intern.h"
#include "strgadgets.h" /* To get GlobalEditFunc prototype */
#include "inputhandler.h"
#include "menutask.h"

#define DEBUG 0
#include <aros/debug.h>

/****************************************************************************************/

static const char name[];
static const char version[];
static const APTR inittabl[4];
static void *const LIBFUNCTABLE[];

LIBBASETYPEPTR INIT();

extern const char LIBEND;

/* There has to be a better way... */

#if INTERNAL_BOOPSI

AROS_UFP3(ULONG, rootDispatcher,
    AROS_UFPA(Class *,  cl,  A0),
    AROS_UFPA(Object *, obj, A2),
    AROS_UFPA(Msg,      msg, A1)
);

#endif

struct IClass *InitICClass (LIBBASETYPEPTR LIBBASE);
struct IClass *InitModelClass (LIBBASETYPEPTR LIBBASE);
struct IClass *InitImageClass (LIBBASETYPEPTR LIBBASE);
struct IClass *InitFrameIClass (LIBBASETYPEPTR LIBBASE);
struct IClass *InitSysIClass (LIBBASETYPEPTR LIBBASE);
struct IClass *InitFillRectClass (LIBBASETYPEPTR LIBBASE);
struct IClass *InitGadgetClass (LIBBASETYPEPTR LIBBASE);
struct IClass *InitButtonGClass (LIBBASETYPEPTR LIBBASE);
struct IClass *InitFrButtonClass (LIBBASETYPEPTR LIBBASE);
struct IClass *InitPropGClass (LIBBASETYPEPTR LIBBASE);
struct IClass *InitStrGClass (LIBBASETYPEPTR LIBBASE);
struct IClass *InitGroupGClass (LIBBASETYPEPTR LIBBASE);

struct IClass *InitMenuBarLabelClass (LIBBASETYPEPTR LIBBASE);

struct IClass *InitDragBarClass (LIBBASETYPEPTR LIBBASE);
struct IClass *InitSizeButtonClass (LIBBASETYPEPTR LIBBASE);


int Intuition_entry(void)
{
    /* If the library was executed by accident return error code. */
    return -1;
}

const struct Resident Intuition_resident=
{
    RTC_MATCHWORD,
    (struct Resident *)&Intuition_resident,
    (APTR)&LIBEND,
    RTF_AUTOINIT|RTF_COLDSTART,
    VERSION_NUMBER,
    NT_LIBRARY,
    10,
    (char *)name,
    (char *)&version[6],
    (ULONG *)inittabl
};

static const char name[]=INTUITIONNAME;

static const char version[]=VERSION_STRING;

static const APTR inittabl[4]=
{
    (APTR)sizeof(struct IntIntuitionBase),
    (APTR)LIBFUNCTABLE,
    NULL,
    &INIT
};

/****************************************************************************************/

AROS_UFH3(LIBBASETYPEPTR, AROS_SLIB_ENTRY(init,Intuition),
 AROS_UFHA(LIBBASETYPEPTR,	LIBBASE,    D0),
 AROS_UFHA(BPTR,		segList,    A0),
 AROS_UFHA(struct ExecBase *,	sysBase,    A6)
)
{
    AROS_USERFUNC_INIT
    SysBase = sysBase;

    /*  We have to open this here, but it doesn't do any allocations,
	so it shouldn't fail...
    */

#if !INTERNAL_BOOPSI
    if(!(BOOPSIBase = OpenLibrary("boopsi.library", 0)))
    {
	/* Intuition couldn't open unknown library */
	Alert(AT_DeadEnd | AN_Intuition | AG_OpenLib | AO_Unknown);
	return NULL;
    }
#endif

    /* Create semaphore and initialize it */
    GetPrivIBase(LIBBASE)->IBaseLock = AllocMem (sizeof(struct SignalSemaphore), MEMF_PUBLIC|MEMF_CLEAR);

    if (!GetPrivIBase(LIBBASE)->IBaseLock)
	return NULL;

    InitSemaphore(GetPrivIBase(LIBBASE)->IBaseLock);

    /* Initialize global stringgadget edit hook */
    GetPrivIBase(LIBBASE)->DefaultEditHook.h_Entry	= (APTR)AROS_ASMSYMNAME(GlobalEditFunc);
    GetPrivIBase(LIBBASE)->DefaultEditHook.h_SubEntry	= NULL;
    GetPrivIBase(LIBBASE)->DefaultEditHook.h_Data	= LIBBASE;
    
    GetPrivIBase(LIBBASE)->GlobalEditHook = &(GetPrivIBase(LIBBASE)->DefaultEditHook);

    GetPrivIBase(LIBBASE)->DefaultPubScreen = NULL;
    NEWLIST(&GetPrivIBase(LIBBASE)->PubScreenList);
    InitSemaphore(&GetPrivIBase(LIBBASE)->PubScrListLock);

    InitSemaphore(&GetPrivIBase(LIBBASE)->GadgetLock);
    InitSemaphore(&GetPrivIBase(LIBBASE)->MenuLock);
    InitSemaphore(&GetPrivIBase(LIBBASE)->IntuiActionLock);
    InitSemaphore(&GetPrivIBase(LIBBASE)->InputHandlerLock);
    
#if INTERNAL_BOOPSI
    InitSemaphore(&GetPrivIBase(LIBBASE)->ClassListLock);
    NEWLIST(&GetPrivIBase(LIBBASE)->ClassList);

    /* Setup root class */
    
    GetPrivIBase(LIBBASE)->RootClass.cl_Dispatcher.h_Entry = rootDispatcher;
    GetPrivIBase(LIBBASE)->RootClass.cl_ID                 = (ClassID)ROOTCLASS;
    GetPrivIBase(LIBBASE)->RootClass.cl_UserData           = (IPTR)LIBBASE;
    AddClass(&(GetPrivIBase(LIBBASE)->RootClass));
#endif
    
    /* Add all other classes */

#if INTERNAL_BOOPSI
    InitICClass (LIBBASE);  	    	/* After ROOTCLASS 	*/
    InitModelClass (LIBBASE);  	    	/* After ICCLASS 	*/
#endif
    InitImageClass (LIBBASE); 		/* After ROOTCLASS 	*/
    InitFrameIClass (LIBBASE); 		/* After IMAGECLASS 	*/
    InitSysIClass (LIBBASE); 		/* After IMAGECLASS 	*/
    InitFillRectClass (LIBBASE); 	/* After IMAGECLASS 	*/
    InitGadgetClass (LIBBASE); 		/* After ROOTCLASS 	*/
    InitButtonGClass (LIBBASE); 	/* After GADGETCLASS 	*/
    InitFrButtonClass (LIBBASE); 	/* After BUTTONGCLASS 	*/
    InitPropGClass (LIBBASE);    	/* After GADGETCLASS 	*/
    InitStrGClass (LIBBASE);    	/* After GADGETCLASS 	*/
    InitGroupGClass (LIBBASE);		/* After GADGETCLASS 	*/
    
    InitMenuBarLabelClass (LIBBASE); /* After IMAGECLASS */
    
    GetPrivIBase(LIBBASE)->dragbarclass = InitDragBarClass (LIBBASE); /* After GADGETCLASS */
    if (!GetPrivIBase(LIBBASE)->dragbarclass)
    	return NULL;

    GetPrivIBase(LIBBASE)->sizebuttonclass = InitSizeButtonClass (LIBBASE); /* After GADGETCLASS */
    if (!GetPrivIBase(LIBBASE)->sizebuttonclass)
    	return NULL;
  
    LoadDefaultPreferences(LIBBASE);

    GetPrivIBase(LIBBASE)->MenuLook = MENULOOK_3D;
    GetPrivIBase(LIBBASE)->MenusUnderMouse = FALSE;
    GetPrivIBase(LIBBASE)->FrameSize = FRAMESIZE_THIN;

          
    {
    	WORD i;
	
	for(i = 0; i < RESOURCELIST_HASHSIZE; i++)
	{
	    NewList((struct List *)&GetPrivIBase(LIBBASE)->ResourceList[i]);
	}
    }
    
    /* You would return NULL if the init failed */
    return LIBBASE;
    AROS_USERFUNC_EXIT
}

/****************************************************************************************/

AROS_LH1(LIBBASETYPEPTR, open,
 AROS_LHA(ULONG, version, D0),
	   LIBBASETYPEPTR, LIBBASE, 1, Intuition)
{
    AROS_LIBFUNC_INIT

    /* Keep compiler happy */
    version=0;
    
    /* Open the input device */
    
    if (!GetPrivIBase(LIBBASE)->InputMP)
    {
    	if (!(GetPrivIBase(LIBBASE)->InputMP = CreateMsgPort()))
	    return (NULL);
    }
    
    if (!GetPrivIBase(LIBBASE)->InputIO)
    {
	if (!(GetPrivIBase(LIBBASE)->InputIO = (struct IOStdReq *)
		CreateIORequest(GetPrivIBase(LIBBASE)->InputMP, sizeof (struct IOStdReq))) )
    	    return (NULL);
    }
    
    if (!GetPrivIBase(LIBBASE)->InputDeviceOpen)
    {
    	if (!OpenDevice("input.device", -1, (struct IORequest *)GetPrivIBase(LIBBASE)->InputIO, NULL))
	{
    	    GetPrivIBase(LIBBASE)->InputDeviceOpen = TRUE;
	    InputBase = (struct Library *)GetPrivIBase(LIBBASE)->InputIO->io_Device;
	}
    	else
    	    return (NULL);
    	
    }

    if (!UtilityBase)
    {
	if (!(UtilityBase = (void *)OpenLibrary (UTILITYNAME, 39)) )
	    return NULL; /* don't close anything */
    }

    if (!GetPrivIBase(LIBBASE)->InputHandler)
    {
    	D(bug("Initializing inputhandler\n"));
    	if ( !(GetPrivIBase(LIBBASE)->InputHandler = InitIIH(LIBBASE)) )
    	    return (NULL);
    	    
    	D(bug("Adding inputhandler\n"));
    	GetPrivIBase(LIBBASE)->InputIO->io_Data = (APTR)GetPrivIBase(LIBBASE)->InputHandler;
    	GetPrivIBase(LIBBASE)->InputIO->io_Command = IND_ADDHANDLER;
    
    	D(bug("Calling DoIO()\n"));
    	DoIO((struct IORequest *)GetPrivIBase(LIBBASE)->InputIO);
    	D(bug("DoIO() called\n"));
    }
    
    if (!GfxBase)
    {
	if (!(GfxBase = (void *)OpenLibrary (GRAPHICSNAME, 39)) )
	    return NULL;
    	
	GetPrivIBase(LIBBASE)->ScreenFont = GfxBase->DefaultFont;
    }

    if (!LayersBase)
    {
	if (!(LayersBase = (void *)OpenLibrary ("layers.library", 39)) )
	    return NULL;
    }

    if (!KeymapBase)
    {
	if (!(KeymapBase = OpenLibrary ("keymap.library", 39)) )
	    return NULL; /* don't close anything */
    }
    
    if (!TimerBase)
    {
	if (!(TimerMP = CreateMsgPort()))
	    return NULL; /* don't close anything */

	if (!(TimerIO = (struct timerequest *)CreateIORequest(TimerMP, sizeof(struct timerequest))))
	    return NULL; /* don't close anything */

	if (OpenDevice(TIMERNAME,UNIT_VBLANK, (struct IORequest *)TimerIO,0))
	    return NULL; /* don't close anything */

	TimerBase = (struct Library *)TimerIO->tr_node.io_Device;
    }

#if 0
    if(!DOSBase)
    {
	DOSBase = OpenLibrary("dos.library", 0);

	((struct DosLibrary *)DOSBase)->dl_IntuitionBase = 
	    (struct Library *)LIBBASE;

	/* Install intuition's version of DisplayError() that puts up
	   a requester with Retry/Cancel options */
	GetPrivIBase(LIBBASE)->OldDisplayErrorFunc = 
	    SetFunction(DOSBase, -81*LIB_VECTSIZE,
			AROS_SLIB_ENTRY(DisplayError, Intuition));
    }
#endif

    /* FIXME: no cleanup routines for MenuHandler task */
    if (!GetPrivIBase(LIBBASE)->MenuHandlerPort)
    {
    	if (!InitDefaultMenuHandler(LIBBASE))
            return NULL;
    }

    /* I have one more opener. */
    LIBBASE->LibNode.lib_OpenCnt++;
    LIBBASE->LibNode.lib_Flags&=~LIBF_DELEXP;
    
    /* You would return NULL if the open failed. */
    return LIBBASE;
    AROS_LIBFUNC_EXIT
}

/****************************************************************************************/

AROS_LH0(BPTR, close,
	   LIBBASETYPEPTR, LIBBASE, 2, Intuition)
{
    AROS_LIBFUNC_INIT

    /* I have one fewer opener. */
    if(!--LIBBASE->LibNode.lib_OpenCnt)
    {
    
#if 0 /* intuition_driver stuff is dead */
	intui_close(LIBBASE);
#endif

	/* Delayed expunge pending? */
	if(LIBBASE->LibNode.lib_Flags & LIBF_DELEXP)
	    /* Then expunge the library */
	    return expunge();
    }
    return 0;
    AROS_LIBFUNC_EXIT
}

/****************************************************************************************/

AROS_LH0(BPTR, expunge,
	   LIBBASETYPEPTR, LIBBASE, 3, Intuition)
{
    AROS_LIBFUNC_INIT

    /* Test for openers. */
    if(LIBBASE->LibNode.lib_OpenCnt)
    {
	/* Set the delayed expunge flag and return. */
	LIBBASE->LibNode.lib_Flags |= LIBF_DELEXP;
	return 0;
    }


    /* We are no longer patching DOS */
    ((struct DosLibrary *)DOSBase)->dl_IntuitionBase = NULL;

    /* Reset the old DisplayError() function */
    SetFunction(DOSBase, -81*LIB_VECTSIZE,  
		GetPrivIBase(LIBBASE)->OldDisplayErrorFunc);

    /* Free unecessary memory. */
    
    /* The WB screen is opened in ./lateintuiinit.c */
    if (GetPrivIBase(LIBBASE)->WorkBench)
	CloseScreen (GetPrivIBase(LIBBASE)->WorkBench);
    
    if (KeymapBase)
	CloseLibrary (KeymapBase);

    if (UtilityBase)
	CloseLibrary ((struct Library *)UtilityBase);

    if (GfxBase)
	CloseLibrary ((struct Library *)GfxBase);

    if (DOSBase)
	CloseLibrary(DOSBase);


    if (GetPrivIBase(LIBBASE)->InputHandler)
    {
    	CleanupIIH(GetPrivIBase(LIBBASE)->InputHandler, LIBBASE);

    	/* Remove inputandler */
    	GetPrivIBase(LIBBASE)->InputIO->io_Data = (APTR)GetPrivIBase(LIBBASE)->InputHandler;
    	GetPrivIBase(LIBBASE)->InputIO->io_Command = IND_REMHANDLER;
    	DoIO((struct IORequest *)GetPrivIBase(LIBBASE)->InputIO);
    }
    

    if (GetPrivIBase(LIBBASE)->InputDeviceOpen)
    	CloseDevice((struct IORequest *)GetPrivIBase(LIBBASE)->InputIO);

    if (GetPrivIBase(LIBBASE)->InputIO)
    	DeleteIORequest((struct IORequest *)GetPrivIBase(LIBBASE)->InputIO);
    	
    if (GetPrivIBase(LIBBASE)->InputMP)
    	DeleteMsgPort(GetPrivIBase(LIBBASE)->InputMP);


#if 0 /* intuition_driver stuff is dead */ 
    /* Let the driver do the same */
    intui_expunge (LIBBASE);
#endif

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

/****************************************************************************************/

AROS_LH0I(int, null,
	    LIBBASETYPEPTR, LIBBASE, 4, Intuition)
{
    AROS_LIBFUNC_INIT
    return 0;
    AROS_LIBFUNC_EXIT
}

/****************************************************************************************/
