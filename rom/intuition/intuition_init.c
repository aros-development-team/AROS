/*
    Copyright  1995-2010, The AROS Development Team. All rights reserved.
    Copyright  2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$
*/

/****************************************************************************************/

#define INIT AROS_SLIB_ENTRY(init,Intuition)

#include <string.h>
#include <exec/lists.h>
#include <exec/resident.h>
#include <exec/memory.h>
#include <exec/execbase.h>
#include <exec/alerts.h>
#include <oop/oop.h>
#include <proto/graphics.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/oop.h>
#include <proto/alib.h>
//#include <proto/arossupport.h>
#include <devices/input.h>
#ifndef INTUITION_CLASSES_H
#   include <intuition/classes.h>
#endif
#include <intuition/pointerclass.h>
#ifndef UTILITY_HOOKS_H
#   include <utility/hooks.h>
#endif
#include <utility/utility.h>
#include <aros/symbolsets.h>
#include LC_LIBDEFS_FILE
#include "intuition_intern.h"
#include "strgadgets.h" /* To get GlobalEditFunc prototype */
#include "inputhandler.h"
#include "menutask.h"
#include "screennotifytask.h"

#ifdef SKINS
    #include "transplayers.h"
    #include "smallmenu.h"
#endif

#define DEBUG 0
#include <aros/debug.h>

#ifdef INTUITION_NOTIFY_SUPPORT
/* screennotify/notifyintuition init routines from notify.c */
struct Library *sn_Init(struct IntuitionBase *IntuitionBase);
struct Library *ni_Init(struct IntuitionBase *IntuitionBase);
#endif

/* There has to be a better way... */

AROS_UFP3(ULONG, rootDispatcher,
          AROS_UFPA(Class *,  cl,  A0),
          AROS_UFPA(Object *, obj, A2),
          AROS_UFPA(Msg,      msg, A1)
         );

/****************************************************************************************/

/* Default colors for the new screen, the same as in AmigaOS 3.1 */

const ULONG coltab[] =
{
    0xB3B3B3B3, 0xB3B3B3B3, 0xB3B3B3B3, /* Grey70     */
    0x00000000, 0x00000000, 0x00000000, /* Black      */
    0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, /* White      */
    0x66666666, 0x88888888, 0xBBBBBBBB, /* AMIGA Blue */

    0xEEEEEEEE, 0x44444444, 0x44444444, /* Red        */
    0x55555555, 0xDDDDDDDD, 0x55555555, /* Green      */
    0x00000000, 0x44444444, 0xDDDDDDDD, /* Dark Blue  */
    0xEEEEEEEE, 0x99999999, 0x00000000, /* Yellow     */
    
    0xbbbbbbbb, 0x00000000, 0x00000000, /* Default colors for mouse pointer */
    0xdddddddd, 0x00000000, 0x00000000,
    0xeeeeeeee, 0x00000000, 0x00000000
};

static int IntuitionInit(LIBBASETYPEPTR LIBBASE)
{
    struct OOP_ABDescr attrbases[] = {
	{IID_Hidd    , &GetPrivIBase(LIBBASE)->HiddAttrBase   },
	{IID_Hidd_Gfx, &GetPrivIBase(LIBBASE)->HiddGfxAttrBase},
	{NULL        , NULL				       }
    };

    DEBUG_INIT(dprintf("LIB_Init: base 0x%lx\n", (ULONG) LIBBASE));

    if (!OOP_ObtainAttrBases(attrbases))
	return FALSE;

#warning "FIXME: This libInit is all broken if something should fail, but do we care?"
#warning "FIXME: If something fails we're screwed anyway..."

    /*  We have to open this here, but it doesn't do any allocations,
        so it shouldn't fail...
     */

    /* Create semaphore and initialize it */
    GetPrivIBase(LIBBASE)->IBaseLock = AllocMem (sizeof(struct SignalSemaphore), MEMF_PUBLIC|MEMF_CLEAR);

    if (!GetPrivIBase(LIBBASE)->IBaseLock)
	return FALSE;

    InitSemaphore(GetPrivIBase(LIBBASE)->IBaseLock);

    InitSemaphore(&GetPrivIBase(LIBBASE)->WinDecorSem);
    InitSemaphore(&GetPrivIBase(LIBBASE)->ScrDecorSem);
    InitSemaphore(&GetPrivIBase(LIBBASE)->MenuDecorSem);

    /* Initialize global stringgadget edit hook */
    GetPrivIBase(LIBBASE)->DefaultEditHook.h_Entry  = (APTR)AROS_ASMSYMNAME(GlobalEditFunc);
    GetPrivIBase(LIBBASE)->DefaultEditHook.h_SubEntry   = NULL;
    GetPrivIBase(LIBBASE)->DefaultEditHook.h_Data   = LIBBASE;

    GetPrivIBase(LIBBASE)->GlobalEditHook = &(GetPrivIBase(LIBBASE)->DefaultEditHook);

    GetPrivIBase(LIBBASE)->DefaultPubScreen = NULL;
    NEWLIST(&GetPrivIBase(LIBBASE)->PubScreenList);
    InitSemaphore(&GetPrivIBase(LIBBASE)->PubScrListLock);

    InitSemaphore(&GetPrivIBase(LIBBASE)->ScreenNotificationListLock);
    NEWLIST(&GetPrivIBase(LIBBASE)->ScreenNotificationList);

    NEWLIST(&GetPrivIBase(LIBBASE)->Decorations);


    InitSemaphore(&GetPrivIBase(LIBBASE)->GadgetLock);
    InitSemaphore(&GetPrivIBase(LIBBASE)->MenuLock);
    InitSemaphore(&GetPrivIBase(LIBBASE)->WindowLock);
    InitSemaphore(&GetPrivIBase(LIBBASE)->IntuiActionLock);
    InitSemaphore(&GetPrivIBase(LIBBASE)->InputHandlerLock);

#ifdef SKINS
    InitSemaphore(&GetPrivIBase(LIBBASE)->DataTypesSem);
    GetPrivIBase(LIBBASE)->DataTypesBase = 0;

    GetPrivIBase(LIBBASE)->transphook.h_Data = (APTR)GetPrivIBase(LIBBASE);
    GetPrivIBase(LIBBASE)->transphook.h_Entry = (HOOKFUNC)HookEntry;
    GetPrivIBase(LIBBASE)->transphook.h_SubEntry = (HOOKFUNC)WindowTranspFunc;

    GetPrivIBase(LIBBASE)->notransphook.h_Data = (APTR)GetPrivIBase(LIBBASE);
    GetPrivIBase(LIBBASE)->notransphook.h_Entry = (HOOKFUNC)HookEntry;
    GetPrivIBase(LIBBASE)->notransphook.h_SubEntry = (HOOKFUNC)WindowNoTranspFunc;
#endif

#ifdef __MORPHOS__
    memset(GetPrivIBase(LIBBASE)->Pad, 0xee, sizeof(GetPrivIBase(LIBBASE)->Pad));
    GetPrivIBase(LIBBASE)->SystemRequestTitle = "System Request";
    GetPrivIBase(LIBBASE)->WorkbenchTitle = "Ambient Screen";
#endif
    /*
     * Setup the default pens to the default
     * colors so that screens have proper color
     * even before IPrefs is loaded.
     */
    {
	struct Color32 *p;
	ULONG   	    i;

	p = GetPrivIBase(LIBBASE)->Colors;

	for (i = 0; i < COLORTABLEENTRIES; i++)
	{
	    p[i].red = coltab[i*3];
	    p[i].green = coltab[i*3+1];
	    p[i].blue = coltab[i*3+2];
	}
    }
    LIBBASE->PointerAlpha = 0x9F9F;

#ifdef __MORPHOS__
    GetPrivIBase(LIBBASE)->mosmenuclass = InitMuiMenuClass(LIBBASE);
#endif

    DEBUG_INIT(dprintf("LIB_Init: create menu handler task\n"));
    /* FIXME: no cleanup routines for MenuHandler task */
    if (!InitDefaultMenuHandler((struct IntuitionBase *)IntuitionBase))
	return FALSE;

    /* FIXME: no cleanup routines for ScreennotifyHandler task */
    if (!InitDefaultScreennotifyHandler((struct IntuitionBase *)IntuitionBase))
	return FALSE;


    DEBUG_INIT(dprintf("LIB_Init: load default preferences\n"));
    LoadDefaultPreferences(LIBBASE);
#ifdef USEGETIPREFS
    GetPrivIBase(LIBBASE)->IPrefsLoaded = FALSE;
#endif

#ifdef SKINS
    if (!GetPrivIBase(LIBBASE)->SmallMenuPool)
    {
	if (!(GetPrivIBase(LIBBASE)->SmallMenuPool = CreatePool(MEMF_SEM_PROTECTED,(sizeof (struct SmallMenuEntry))*20,(sizeof (struct SmallMenuEntry))*20))) return NULL;
    }
#endif

    if (!(GetPrivIBase(LIBBASE)->IDCMPPool = CreatePool(MEMF_SEM_PROTECTED,(sizeof (struct IntIntuiMessage)) * 100,sizeof (struct IntIntuiMessage)))) return FALSE;

#ifdef SKINS
    strcpy(GetPrivIBase(IntuitionBase)->IControlExtensions.ice_ClockFormat,"%X");
#endif
      
    GetPrivIBase(LIBBASE)->FrameSize = FRAMESIZE_THIN;

    {
	WORD i;
           
	for(i = 0; i < RESOURCELIST_HASHSIZE; i++)
	{
	    NewList((struct List *)&GetPrivIBase(LIBBASE)->ResourceList[i]);
	}
    }

    if (!(GetPrivIBase(LIBBASE)->WinDecorObj = NewObjectA(NULL, WINDECORCLASS, NULL))) return FALSE;
    if (!(GetPrivIBase(LIBBASE)->ScrDecorObj = NewObjectA(NULL, SCRDECORCLASS, NULL))) return FALSE;
    if (!(GetPrivIBase(LIBBASE)->MenuDecorObj = NewObjectA(NULL, MENUDECORCLASS, NULL))) return FALSE;

    GetPrivIBase(LIBBASE)->DefWinDecorObj = GetPrivIBase(LIBBASE)->WinDecorObj;
    GetPrivIBase(LIBBASE)->DefScrDecorObj = GetPrivIBase(LIBBASE)->ScrDecorObj;
    GetPrivIBase(LIBBASE)->DefMenuDecorObj = GetPrivIBase(LIBBASE)->MenuDecorObj;
    LIBBASE->ViewLord_ok = FALSE;

    DEBUG_INIT(dprintf("LIB_Init: Setting up pointers...\n"));

    GetPrivIBase(IntuitionBase)->DefaultPointer = MakePointerFromPrefs
    (
        IntuitionBase, GetPrivIBase(IntuitionBase)->ActivePreferences
    );
    GetPrivIBase(IntuitionBase)->BusyPointer = MakePointerFromPrefs
    (
        IntuitionBase, GetPrivIBase(IntuitionBase)->ActivePreferences
    );

    if
    (
           !GetPrivIBase(IntuitionBase)->DefaultPointer 
        || !GetPrivIBase(IntuitionBase)->BusyPointer
    )
    {
        return FALSE;
    }

    DEBUG_INIT(dprintf("LIB_Init: done\n"));

    return TRUE;
}

static int InitRootClass(LIBBASETYPEPTR LIBBASE)
{
    InitSemaphore(&GetPrivIBase(LIBBASE)->ClassListLock);
    NEWLIST(&GetPrivIBase(LIBBASE)->ClassList);

    /* Setup root class */

    GetPrivIBase(LIBBASE)->RootClass.cl_Dispatcher.h_Entry = (APTR)AROS_ASMSYMNAME(rootDispatcher);
    GetPrivIBase(LIBBASE)->RootClass.cl_ID                 = (ClassID)ROOTCLASS;
    GetPrivIBase(LIBBASE)->RootClass.cl_UserData           = (IPTR)LIBBASE;
    DEBUG_INIT(dprintf("LIB_Init: create rootclass\n"));
    AddClass(&(GetPrivIBase(LIBBASE)->RootClass));
    
    return TRUE;
}

/****************************************************************************************/

static int IntuitionOpen(LIBBASETYPEPTR LIBBASE)
{
    DEBUG_OPEN(dprintf("LIB_Open: base 0x%lx\n", LIBBASE));

    /* Open the input device */

    if (!GetPrivIBase(LIBBASE)->InputMP)
    {
	if (!(GetPrivIBase(LIBBASE)->InputMP = CreateMsgPort()))
	{
	    DEBUG_OPEN(dprintf("LIB_Open: can't create port\n"));
	    return FALSE;
	}
    }

    if (!GetPrivIBase(LIBBASE)->InputIO)
    {
	if (!(GetPrivIBase(LIBBASE)->InputIO = (struct IOStdReq *)
	      CreateIORequest(GetPrivIBase(LIBBASE)->InputMP, sizeof (struct IOStdReq))) )
	{
	    DEBUG_OPEN(dprintf("LIB_Open: can't create iorequest\n"));
	    return FALSE;
	}
    }

    if (!GetPrivIBase(LIBBASE)->InputDeviceOpen)
    {
	if (!OpenDevice("input.device", -1, (struct IORequest *)GetPrivIBase(LIBBASE)->InputIO, 0))
	{
	    GetPrivIBase(LIBBASE)->InputDeviceOpen = TRUE;
	    InputBase = (struct Library *)GetPrivIBase(LIBBASE)->InputIO->io_Device;
	}
	else
	{
	    DEBUG_OPEN(dprintf("LIB_Open: can't open input.device\n"));
	    return FALSE;
	}
    }

    if (!GetPrivIBase(LIBBASE)->InputHandler)
    {
	D(bug("Initializing inputhandler\n"));
	if ( !(GetPrivIBase(LIBBASE)->InputHandler = InitIIH(LIBBASE)) )
	{
	    DEBUG_OPEN(dprintf("LIB_Open: can't init input handler\n"));
	    return FALSE;
	}
	
	D(bug("Adding inputhandler\n"));
	GetPrivIBase(LIBBASE)->InputIO->io_Data = (APTR)GetPrivIBase(LIBBASE)->InputHandler;
	GetPrivIBase(LIBBASE)->InputIO->io_Command = IND_ADDHANDLER;

	D(bug("Calling DoIO()\n"));
	DoIO((struct IORequest *)GetPrivIBase(LIBBASE)->InputIO);
	D(bug("DoIO() called\n"));
    }

    if (!LIBBASE->ViewLord_ok)
    {
#ifdef __MORPHOS__
	struct ViewExtra *ve;

	if (!(ve = GfxNew(VIEW_EXTRA_TYPE)))
	{
	    GfxBase = NULL;
	    DEBUG_OPEN(dprintf("LIB_Open: can't create view extra\n"));
	    return FALSE;
	}
#endif
	D(bug("[intuition] Calling InitView()\n"));
	InitView(&LIBBASE->IBase.ViewLord);
#ifdef __MORPHOS__
	GfxAssociate(&LIBBASE->IBase.ViewLord, ve);

	GetPrivIBase(LIBBASE)->ViewLordExtra = ve;
	GetPrivIBase(LIBBASE)->SpriteNum = -1;
#endif
	LIBBASE->ViewLord_ok = TRUE;
    }
    
    if (!GetPrivIBase(LIBBASE)->ScreenFont)
	GetPrivIBase(LIBBASE)->ScreenFont = GfxBase->DefaultFont;

#if 0 /* CHECKME: stegerg: backport, disabled */
    if (!(GetPrivIBase(LIBBASE)->TopazFont))
    {
	struct TextAttr textattr = {"topaz.font",8,0,FPF_ROMFONT};
	GetPrivIBase(LIBBASE)->TopazFont = OpenFont(&textattr);
    }
#endif

    if (!TimerBase)
    {
	if (!(TimerMP = CreateMsgPort()))
	{
	    DEBUG_OPEN(dprintf("LIB_Open: can't create timer port\n"));
	    return FALSE; /* don't close anything */
	}
	
	if (!(TimerIO = (struct timerequest *)CreateIORequest(TimerMP, sizeof(struct timerequest))))
	{
	    DEBUG_OPEN(dprintf("LIB_Open: can't create timer ioreq\n"));
	    return FALSE; /* don't close anything */
	}

	if (OpenDevice(TIMERNAME,UNIT_VBLANK, (struct IORequest *)TimerIO,0))
	{
	    DEBUG_OPEN(dprintf("LIB_Open: can't open timer.device\n"));
	    return FALSE; /* don't close anything */
	}

	TimerBase = (struct Library *)TimerIO->tr_node.io_Device;

	SetPrefs(GetPrivIBase(LIBBASE)->DefaultPreferences, sizeof(struct Preferences), FALSE);
    }

#if 0
    if (((struct Library *)LIBBASE)->lib_OpenCnt == 0)
    {
	//check if dos is opened!!!
	((struct DosLibrary *)DOSBase)->dl_IntuitionBase =
	    (struct Library *)LIBBASE;

	/* Install intuition's version of DisplayError() that puts up
	 a requester with Retry/Cancel options */
	GetPrivIBase(LIBBASE)->OldDisplayErrorFunc =
	    SetFunction(DOSBase, -81*LIB_VECTSIZE,
			AROS_SLIB_ENTRY(DisplayError, Intuition));
    }
#else
# ifdef SKINS
    if (((struct Library *)LIBBASE)->lib_OpenCnt == 0)
    {
	InitSkinManager(IntuitionBase);
    }
# endif
#endif

    /* FIXME: no cleanup routines for MenuHandler task */
    if (!GetPrivIBase(LIBBASE)->MenuHandlerPort)
    {
	if (!InitDefaultMenuHandler((struct IntuitionBase *)LIBBASE))
	    return FALSE;
    }

    /* FIXME: no cleanup routines for ScreennotifyHandler task */
    if (!GetPrivIBase(LIBBASE)->ScreenNotifyReplyPort)
    {
	if (!InitDefaultScreennotifyHandler((struct IntuitionBase *)LIBBASE))
	    return FALSE;
    }

#ifdef INTUITION_NOTIFY_SUPPORT
    /* Add screennotify.library base if not there yet - Piru
     */
    if (!GetPrivIBase(LIBBASE)->ScreenNotifyBase)
    {
	GetPrivIBase(LIBBASE)->ScreenNotifyBase = sn_Init(IntuitionBase);
    }

# if 0 /* not finished yet - Piru */
    /* Add notifyintuition.library base if not there yet - Piru
     */
    if (!GetPrivIBase(LIBBASE)->NotifyIntuitionBase)
    {
	GetPrivIBase(LIBBASE)->NotifyIntuitionBase = ni_Init(IntuitionBase);
    }
# endif
#endif

    return TRUE;
}

DECLARESET(CLASSESINIT);
ADD2SET(InitRootClass, classesinit, -20);
ADD2INITLIB(IntuitionInit, 0);
ADD2OPENLIB(IntuitionOpen, 0);

