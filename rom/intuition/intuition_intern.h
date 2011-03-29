#ifndef INTUITION_INTERN_H
#define INTUITION_INTERN_H

/*
    Copyright  1995-2011, The AROS Development Team. All rights reserved.
    Copyright  2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$
*/

#ifndef AROS_LIBCALL_H
#   include <aros/libcall.h>
#endif
#ifndef AROS_ATOMIC_H
#   include <aros/atomic.h>
#endif
#ifndef EXEC_EXECBASE_H
#   include <exec/execbase.h>
#endif
#ifndef EXEC_SEMAPHORES_H
#   include <exec/semaphores.h>
#endif
#ifndef EXEC_TASKS_H
#   include <exec/tasks.h>
#endif
#ifndef EXEC_PORTS_H
#   include <exec/ports.h>
#endif
#ifndef EXEC_IO_H
#   include <exec/io.h>
#endif
#ifndef EXEC_TYPES_H
#   include <exec/types.h>
#endif
#ifndef EXEC_LISTS_H
#   include <exec/lists.h>
#endif
#ifndef GRAPHICS_GFXBASE_H
#   include <graphics/gfxbase.h>
#endif
#ifndef GRAPHICS_RASTPORT_H
#   include <graphics/rastport.h>
#endif
#ifndef GRAPHICS_REGIONS_H
#   include <graphics/regions.h>
#endif
#ifndef GRAPHICS_CLIP_H
#   include <graphics/clip.h>
#endif
#ifndef GRAPHICS_TEXT_H
#   include <graphics/text.h>
#endif
#ifndef INTUITION_INTUITION_H
#   include <intuition/intuition.h>
#endif
#ifndef INTUITION_INTUITIONBASE_H
#   include <intuition/intuitionbase.h>
#endif
#ifndef INTUITION_CLASSES_H
#   include <intuition/classes.h>
#endif
#ifndef INTUITION_CGHOOKS_H
#   include <intuition/cghooks.h>
#endif
#ifndef INTUITION_SGHOOKS_H
#   include <intuition/sghooks.h>
#endif
#ifndef INTUITION_SCREENS_H
#   include <intuition/screens.h>
#endif
#ifndef PREFS_ICONTROL_H
#   include <prefs/icontrol.h>
#endif
#ifndef PREFS_INPUT_H
#include <prefs/input.h>
#endif
#ifndef CLIB_ALIB_PROTOS_H
#   include <clib/alib_protos.h>
#endif
#ifdef INTUITION_NOTIFY_SUPPORT
#   include <libraries/screennotify.h>
#   include <libraries/notifyintuition.h>
#endif

#include <oop/oop.h>

#include "intuition_debug.h"

#ifdef SKINS
#include "intuition_customize.h"
#include "intuition_internmos.h"
#include "intuition_extend.h"
#endif

#include "requesters.h"

/* Needed for aros_print_not_implemented macro */
#include <aros/debug.h>

#include <aros/asmcall.h>

#if DEBUG_ASSERTS
#define ASSERT_VALID_PTR_ROMOK(ptr)                                                        \
do {                                                                                       \
    if (!((IPTR)ptr & 1)) {                                                                \
        if (TypeOfMem((APTR)ptr))                                                                \
            break;                                                                         \
        else {                                                                             \
            struct Task *me = FindTask(NULL);                                              \
                                                                                           \
            if (((IPTR)(ptr) >= (IPTR)me->tc_SPLower) && ((IPTR)(ptr) < (IPTR)me->tc_SPUpper))                         \
                break;                                                                     \
        }                                                                                  \
    }                                                                                      \
    bug("[intuition] Invalid pointer value %p at %s, line %u\n", ptr, __FILE__, __LINE__); \
} while(0);
#else
#define ASSERT_VALID_PTR_ROMOK(ptr)
#endif

// FIXME: seems only used for RefreshWindowTitles() ? -> better names
// FIXME: what are the correct values?
#define NO_DOUBLEBUFFER         (0)
#define DOUBLEBUFFER            (1)

/* features */
#ifdef __MORPHOS__
#   define USE_OPAQUESIZE 1
#else
#   define USE_OPAQUESIZE 0
#endif

/* This definition turns on compatibility mode where sprite colors
   are allocated on hi- and truecolor screens also. This may be needed
   if some software relies on this allocation. AROS itself currently does
   not need this.
#define ALWAYS_ALLOCATE_SPRITE_COLORS */

#ifdef __MORPHOS__
void    dprintf(char *, ...) __attribute__ ((format (printf, 1, 2)));
void * memclr(APTR, ULONG);
#endif

#ifdef __MORPHOS__
void * memclr(APTR, ULONG);
#define bzero(a,b) memclr(a,b)
#else /* __MORPHOS__ */
#define memclr(a,b) bzero(a,b)
#endif /* __MORPHOS__ */

/*
 * min()/max() without macro side effects.
 */
#define max(a,b) \
    ({typeof(a) _a = (a); \
    typeof(b) _b = (b); \
    _a > _b ? _a : _b;})

#define min(a,b) \
    ({typeof(a) _a = (a); \
    typeof(b) _b = (b); \
    _a > _b ? _b : _a;})

#define EXTENDWORD(x) 	    	x = (LONG)((WORD)x);
#define EXTENDUWORD(x)      	x = (ULONG)((UWORD)x);

/* SANITY CHECK MACRO */
//#define DEBUG_SANITYCHECK

#ifdef DEBUG_SANITYCHECK
#define SANITY_CHECK(x)     	if (!((IPTR)x)) {dprintf("Losing sanity in %s line %d\n",__FILE__,__LINE__); return;};
#define SANITY_CHECKR(x,v)  	if (!((IPTR)x)) {dprintf("Losing sanity in %s line %d\n",__FILE__,__LINE__); return v;};
#else
#define SANITY_CHECK(x)     	if (!((IPTR)x)) return;
#define SANITY_CHECKR(x,v)  	if (!((IPTR)x)) return v;
#endif

/* Options */

#define MENUS_BACKFILL  TRUE

#define MENUS_AMIGALOOK        ((GetPrivIBase(IntuitionBase)->IControlPrefs.ic_Flags & ICF_3DMENUS) == 0)
/* --- Values --- */
#define MENULOOK_3D            0
#define MENULOOK_CLASSIC       1

#define MENUS_UNDERMOUSE       (GetPrivIBase(IntuitionBase)->IControlPrefs.ic_Flags & ICF_POPUPMENUS)

#define AVOID_WINBORDERERASE   (GetPrivIBase(IntuitionBase)->IControlPrefs.ic_Flags & ICF_AVOIDWINBORDERERASE)

/* --- Values --- */
/* TRUE, FALSE */

#define FRAME_SIZE             (GetPrivIBase(IntuitionBase)->FrameSize)
/* --- Values --- */
#define FRAMESIZE_THIN         0 /* 1:1 thin */
#define FRAMESIZE_MEDRES       1 /* 2:1 medres like AmigaOS */
#define FRAMESIZE_THICK        2 /* 1:1 thick */

#define SQUARE_WIN_GADGETS     1

#ifdef __AROS__
/* FIXME: possibly enable this, once gadtools has been updated */
#else
#define GADTOOLSCOMPATIBLE
//enables some gadtools-weirdo-code, MUST be set in both gadtools & intui to work!!!
#endif

#ifdef SKINS
//#define USEGETIPREFS
#endif

/* simpleref layers have gadgets redrawn when app calls beginrefresh() */
//#define BEGINUPDATEGADGETREFRESH
//#define DAMAGECACHE

#define USEWINDOWLOCK
#ifdef USEWINDOWLOCK
#define LOCKWINDOW  	    	    	    ObtainSemaphore(&GetPrivIBase(IntuitionBase)->WindowLock);
#define UNLOCKWINDOW 	    	    	    ReleaseSemaphore(&GetPrivIBase(IntuitionBase)->WindowLock);
#else
#define LOCKWINDOW
#define UNLOCKWINDOW
#endif
/* jDc: do NOT disable this! */

#define USEGADGETLOCK
#ifdef USEGADGETLOCK
#define LOCKGADGET  	    	    	    ObtainSemaphore(&GetPrivIBase(IntuitionBase)->GadgetLock);
#define UNLOCKGADGET 	    	    	    ReleaseSemaphore(&GetPrivIBase(IntuitionBase)->GadgetLock);
#define LOCKWINDOWLAYERS(w) ;
#define UNLOCKWINDOWLAYERS(w) ;
#else
#define LOCKGADGET
#define UNLOCKGADGET
#define LOCKWINDOWLAYERS(w) LockLayerInfo(&w->WScreen->LayerInfo);  \
        if (((struct IntWindow *)(w))->borderlayer) {LockLayer(0,((struct IntWindow *)(w))->borderlayer);};   \
        if (((struct IntWindow *)(w))->wlayer) {LockLayer(0,((struct IntWindow *)(w))->wlayer);};
#define UNLOCKWINDOWLAYERS(w) if (((struct IntWindow *)(w))->wlayer) {UnlockLayer(((struct IntWindow *)(w))->wlayer);}; \
        if (((struct IntWindow *)(w))->borderlayer) {UnlockLayer(((struct IntWindow *)(w))->borderlayer);}    \
        UnlockLayerInfo(&w->WScreen->LayerInfo);
#endif

#define WLAYER(w)   	    	    	    (((struct IntWindow *)(w))->wlayer)
#define BLAYER(w)   	    	    	    (((struct IntWindow *)(w))->borderlayer)

//#define TIMEVALWINDOWACTIVATION

/* If PROP_RENDER_OPTIMIZATION is set to 1, propgadget code tries to optimize rendering
   during prop gadget knob movement. Only area of the propgadget that is affected by
   prop gadget knob movement is re-rendered.

   Unfortunately this can fail with some programs, like DOpus 4.x text viewer */

#define PROP_RENDER_OPTIMIZATION    	    0

#define SINGLE_SETPOINTERPOS_PER_EVENTLOOP  1

#ifndef LIFLG_SUPPORTS_OFFSCREEN_LAYERS
   /* Defined in <graphics/layers.h>, but apparently not on MorphOS. */
#  define LIFLG_SUPPORTS_OFFSCREEN_LAYERS   2
#endif

#define INTUITIONNAME       	    	    "intuition.library"
#define MENUBARLABELCLASS   	    	    "menubarlabelclass"

#define DEFPUBSCREEN 	    	    	    TRUE

#define USE_NEWDISPLAYBEEP  	    	    1

#define TITLEBUFFERLEN      	    	    255

#define USE_IDCMPUPDATE_MESSAGECACHE	    0

#ifdef __MORPHOS__
#if INCLUDE_VERSION < 50

/********************************************************************************/
/* imageclass.h AROS extensions */

#define SYSIA_WithBorder  IA_FGPen  /* default: TRUE */
#define SYSIA_Style       IA_BGPen  /* default: SYSISTYLE_NORMAL */

#define SYSISTYLE_NORMAL   0
#define SYSISTYLE_GADTOOLS 1        /* to get arrow images in gadtools look */

/********************************************************************************/
/* gadgetclass.h AROS extenstions */

/* This method is invoked to learn about the sizing requirements of your class,
   before an object is created. This is AROS specific. */
#define GM_DOMAIN 7
struct gpDomain
{
    STACKED ULONG           MethodID;   /* GM_DOMAIN */
    STACKED struct GadgetInfo 	*gpd_GInfo;  /* see <intuition/cghooks.h> */
    STACKED struct RastPort   	*gpd_RPort;  /* RastPort to calculate dimensions for. */
    STACKED LONG       	 gpd_Which;  /* see below */
    STACKED struct IBox          gpd_Domain; /* Resulting domain. */
    STACKED struct TagItem  	*gpd_Attrs;  /* Additional attributes. None defined,
                           yet. */
};

/********************************************************************************/

/* gpd_Which */
#define GDOMAIN_MINIMUM 0 /* Calculate minimum size. */
#define GDOMAIN_NOMINAL 1 /* Calculate nominal size. */
#define GDOMAIN_MAXIMUM 2 /* Calculate maximum size. */

#endif /* INCLUDE_VERSION < 50 */
#endif /* ifdef __MORPHOS__ */

/********************************************************************************/
#define GM_MOVETEST 8
/* this method is used by our draggad to tell if it's OK to move the mouse when window
is near screen boundaries and offscreen is disabled. msg = gpInput*/
//retvals are:
#define MOVETEST_MOVE       0
#define MOVETEST_ADJUSTPOS  1

/* ObtainGIRPort must install a 0 clipregion and
   set scrollx/scrolly of layer to 0. Since this
   will be restored only when ReleaseGIRPort is
   called, we must backup the orig values somewhere */

struct LayerContext
{
    struct Region       *clipregion;
    WORD            	 scroll_x;
    WORD            	 scroll_y;
    WORD            	 nestcount;
};


/* Preferences */

#define IP_OLDFONT      2
#define IP_OLDOVERSCAN  3
#define IP_OLDICONTROL  4
#define IP_OLDPOINTER   7
#define IP_OLDPALETTE   8
#define IP_OLDPENS      9

#define IP_SCREENMODE   1
#define IP_FONT         101//2
#define IP_ICONTROL     102//4
#define IP_POINTER      103//7
#define IP_PTRCOLOR     104//8
#define IP_PALETTE      105
#define IP_IACTIONS     20
#define IP_IEXTENSIONS  21
#define IP_INPUTEXT     22

#ifdef __MORPHOS__

struct IScreenModePrefs
{
    ULONG smp_DisplayID;
    UWORD smp_Width;
    UWORD smp_Height;
    UWORD smp_Depth;
    UWORD smp_Control;
};

struct IIControlPrefs
{
    UWORD ic_TimeOut;
    WORD  ic_MetaDrag;
    ULONG ic_Flags;
    UBYTE ic_WBtoFront;
    UBYTE ic_FrontToBack;
    UBYTE ic_ReqTrue;
    UBYTE ic_ReqFalse;
};

#else

#include <intuition/iprefs.h>

#endif

struct Color32
{
    ULONG red;
    ULONG green;
    ULONG blue;
};

#define COLORTABLEENTRIES   	11

struct IntScreen;

#define RESOURCELIST_HASHSIZE 	256

#define RESOURCE_WINDOW     	1
#define RESOURCE_SCREEN	    	2

struct HashNode
{
    struct MinNode node;
    UWORD          type;
    APTR           resource;
};

/* Internal Screen Notification Data */
struct IntScreenNotify
{
    struct Node     node;
    struct MsgPort *port;
    struct Task    *sigtask;
    struct Hook    *hook;
    ULONG           flags;
    IPTR            userdata;
    char           *pubname;
    BYTE            sigbit;
};

/* IntuitionBase */
struct IntIntuitionBase
{
    struct IntuitionBase    	 IBase;
#ifdef __MORPHOS__
    WORD                    	 _MinXMouse,_MaxXMouse;   /* Old 1.3 Base entries*/
    WORD                    	 _MinYMouse,_MaxYMouse;   /* Old 1.3 Base entries*/
    ULONG                   	 _StartSecs,_StartMicros; /* Old 1.3 Base entries*/
    char                    	*SystemRequestTitle; /* written by locale as it seems..what a crappy interface*/
    char                    	*WorkbenchTitle; /* written by locale as it seems..what a crappy interface*/

    /*
     * savety pad for intuitionbase accesses
     * probably needs to be smarter
     */
    UBYTE                   	 Pad[0x800];
#endif

    /* Put local shit here, invisible for the user */
#ifdef __MORPHOS__
    struct Library          	*MUIMasterBase;
#endif
    
    struct LayersBase       	*LayersBase;
    struct Library          	*UtilityBase;
    struct GfxBase          	*GfxBase;
    struct Library          	*OOPBase;
    struct Library          	*KeymapBase;

    struct Library          	*InputBase;
    struct Library          	*TimerBase;
    struct MsgPort          	*TimerMP;
    struct timerequest      	*TimerIO;

    struct MsgPort          	*WorkBenchMP;
    struct Screen           	*WorkBench;
    struct SignalSemaphore  	*IBaseLock;

    /* Intuition input handlers replyport. This one is set
    int rom/inputhandler.c/InitIIH()
    */
    struct MsgPort          	*IntuiReplyPort;
    struct MinList          	*IntuiActionQueue;
    struct IOStdReq         	*InputIO;
    struct MsgPort          	*InputMP;

    /* Intuition Screennotify Replyport if SNOTIFY_WAIT_REPLY is specified */

    struct MsgPort              *ScreenNotifyReplyPort;

    BOOL                    	 InputDeviceOpen;
    struct Interrupt        	*InputHandler;

    struct Hook             	*GlobalEditHook;
    /* The default global edit hook */
    struct Hook             	 DefaultEditHook;

    struct Screen           	*DefaultPubScreen;
    struct SignalSemaphore  	 PubScrListLock;
    struct MinList          	 PubScreenList;
    UWORD                   	 pubScrGlobalMode;

    struct SignalSemaphore  	 ScreenNotificationListLock;
    struct List                  ScreenNotificationList;

    struct SignalSemaphore  	 GadgetLock;
    struct SignalSemaphore  	 MenuLock;
    struct SignalSemaphore   	 WindowLock;
    struct SignalSemaphore  	 IntuiActionLock;
    struct SignalSemaphore  	 InputHandlerLock;
    struct LayerContext     	 BackupLayerContext;

    struct IClass           	*dragbarclass;
    struct IClass           	*sizebuttonclass;

    APTR                    	*mosmenuclass;

    struct Preferences      	*DefaultPreferences;
    struct Preferences      	*ActivePreferences;

    struct MsgPort          	*MenuHandlerPort;
    BOOL                    	 MenusActive;

    struct TextFont         	*ScreenFont;
    struct TextFont         	*TopazFont;

    /* Dos function DisplayError() before intuition.library patched it */
    APTR                    	 OldDisplayErrorFunc;

    struct SignalSemaphore  	 ClassListLock;
    struct MinList          	 ClassList;
    struct IClass           	 RootClass;

#ifdef __MORPHOS__
    struct ViewExtra        	*ViewLordExtra;
    LONG                    	 SpriteNum;
#else
    BOOL			ViewLord_ok;
#endif

#ifdef SKINS
    ULONG                   	*SmallMenuPool;
#endif
    ULONG                   	*IDCMPPool;

    struct IScreenModePrefs 	 ScreenModePrefs;
    struct IIControlPrefs   	 IControlPrefs;
#ifdef SKINS
    struct IAction          	*IControlActions;
    struct IControlExtensions    IControlExtensions;
    ULONG                   	 NumIControlActions;
    struct InputPrefsExt    	 InputPrefsExt;
#endif
    struct IClass           	*pointerclass;
    Object                  	*DefaultPointer;
    Object                  	*BusyPointer;
    UWORD                   	 DriPens2[NUMDRIPENS];
    UWORD                   	 DriPens4[NUMDRIPENS];
    UWORD                   	 DriPens8[NUMDRIPENS];
    struct Color32          	 Colors[COLORTABLEENTRIES];
    UWORD			 PointerAlpha;
    ULONG                    	 DMStartSecs;
    ULONG                   	 DMStartMicro;
    struct IntScreen        	*MenuVerifyScreen;
    ULONG                   	 PointerDelay;

#ifdef SKINS
    struct SignalSemaphore  	 DataTypesSem;
    struct Library          	*DataTypesBase; /* should be opened ONLY by int_InitCustomChanges!*/
#endif

    ULONG                   	 LastClickSecs;
    ULONG                   	 LastClickMicro; /* for doubleclick to front */
    ULONG                   	 DoubleClickCounter;
    ULONG                    	 DoubleClickButton;

#ifdef SKINS
    struct Hook             	 transphook; /* hook for windows with intui transp */
    struct Hook             	 notransphook; /* hook for windows with no additional transp (borderless,etc) */
#endif
#ifdef INTUITION_NOTIFY_SUPPORT
    struct Library          	*ScreenNotifyBase;
    struct Library          	*NotifyIntuitionBase;
#endif

    struct RastPort         	 DoGadgetMethodRP;
    struct GadgetInfo       	 DoGadgetMethodGI;

    struct SignalSemaphore   	 WinDecorSem;
    Object  	    	    	*WinDecorObj;
    struct SignalSemaphore   	 ScrDecorSem;
    Object  	    	    	*ScrDecorObj;
    struct SignalSemaphore   	 MenuDecorSem;
    Object  	    	    	*MenuDecorObj;

    Object                      *DefWinDecorObj;
    Object                      *DefScrDecorObj;
    Object                      *DefMenuDecorObj;

    struct List                 Decorations;
    struct NewDecorator         *Decorator;

#ifdef USEGETIPREFS
    BOOL                    	 IPrefsLoaded;
#endif

#if USE_NEWDISPLAYBEEP
    LONG                    	 BeepingScreens;
#endif

    WORD            	    	 prop_clickoffset_x, prop_clickoffset_y;

    struct MinList            	 ResourceList[RESOURCELIST_HASHSIZE];

    /* Menu Look Settings */
    int                          FrameSize;

    Object			*ActiveMonitor;
    Object			*NewMonitor;

    OOP_AttrBase		 HiddAttrBase;
    OOP_AttrBase		 HiddGfxAttrBase;
    OOP_AttrBase		 HiddPixFmtAttrBase;
    struct IClass		*monitorclass;
    struct MinList		 MonitorList;
    struct SignalSemaphore	 MonitorListSem;
};

struct SharedPointer
{
    struct ExtSprite   *sprite;
    WORD    	    	xoffset, yoffset;
    int     	    	ref_count;
};

struct SharedPointer *CreateSharedPointer(struct ExtSprite *, int, int, struct IntuitionBase *);
void ObtainSharedPointer(struct SharedPointer *, struct IntuitionBase *);
void ReleaseSharedPointer(struct SharedPointer *, struct IntuitionBase *);

#ifdef INTUITION_NOTIFY_SUPPORT
void sn_DoNotify(ULONG type, APTR value, struct Library *_ScreenNotifyBase);
#endif

struct IntDrawInfo
{
    struct DrawInfo 	     dri;
    struct Screen   	    *dri_Screen;
    struct SignalSemaphore   dri_WinDecorSem;
    Object  	    	    *dri_WinDecorObj;
    struct SignalSemaphore   dri_ScrDecorSem;
    Object  	    	    *dri_ScrDecorObj;    
};

#define DRI_VERSION_AROS (DRI_VERSION + 1)

#define LOCK_WINDECOR(IntuitionBase)   	 ObtainSemaphore(&((struct IntIntuitionBase *)(IntuitionBase))->WinDecorSem);
#define LOCKSHARED_WINDECOR(IntuitionBase) ObtainSemaphoreShared(&((struct IntIntuitionBase *)(IntuitionBase))->WinDecorSem);
#define UNLOCK_WINDECOR(IntuitionBase) 	 ReleaseSemaphore(&((struct IntIntuitionBase *)(IntuitionBase))->WinDecorSem);

#define LOCK_SCRDECOR(IntuitionBase)   	 ObtainSemaphore(&((struct IntIntuitionBase *)(IntuitionBase))->ScrDecorSem);
#define LOCKSHARED_SCRDECOR(IntuitionBase) ObtainSemaphoreShared(&((struct IntIntuitionBase *)(IntuitionBase))->ScrDecorSem);
#define UNLOCK_SCRDECOR(IntuitionBase) 	 ReleaseSemaphore(&((struct IntIntuitionBase *)(IntuitionBase))->ScrDecorSem);

#define LOCK_MENUDECOR(IntuitionBase)   	 ObtainSemaphore(&((struct IntIntuitionBase *)(IntuitionBase))->MenuDecorSem);
#define LOCKSHARED_MENUDECOR(IntuitionBase) ObtainSemaphoreShared(&((struct IntIntuitionBase *)(IntuitionBase))->MenuDecorSem);
#define UNLOCK_MENUDECOR(IntuitionBase) 	 ReleaseSemaphore(&((struct IntIntuitionBase *)(IntuitionBase))->MenuDecorSem);

struct IntScreen
{
    struct Screen            Screen;

    /* Private fields */
    struct HashNode          hashnode;
    struct IntDrawInfo       DInfo;
    struct TTextAttr         textattr;
    ULONG                    textattrtags[3];
    UWORD                    Pens[NUMDRIPENS];
    struct PubScreenNode    *pubScrNode;
    Object                  *depthgadget;
    UWORD                    SpecialFlags;
    struct Layer            *rootLayer;
#if !USE_NEWDISPLAYBEEP
    ULONG                    DisplayBeepColor0[3];
    struct Window           *DisplayBeepWindow;
#endif
#ifdef __MORPHOS__
    ULONG                    ModeID;
    struct MonitorSpec      *Monitor;
#endif
    Object		    *MonitorObject;
    struct SharedPointer    *Pointer;
    struct Window           *MenuVerifyActiveWindow;
    int                      MenuVerifyTimeOut;
    int                      MenuVerifyMsgCount;
    ULONG                    MenuVerifySeconds;
    ULONG                    MenuVerifyMicros;
    ULONG                    DecorUserBufferSize;
    IPTR                     DecorUserBuffer;
    Object                  *WinDecorObj;
    Object                  *ScrDecorObj;
    Object                  *MenuDecorObj;
    struct NewDecorator     *Decorator;

    struct BitMap           *AllocatedBitmap;

    STRPTR                   ID;
#ifdef SKINS
    WORD                     LastClockPos;
    WORD                     LastClockWidth;
#else
    ULONG                    Reserved;
#endif
#ifdef SKINS
    struct SkinInfo          SkinInfo;
    struct RastPort         *TitlebarBufferRP;
    struct Window           *TitlebarBufferWin;
    ULONG                    TitlebarWinWidth;
    ULONG                    TitlebarWinActive;
#endif
#if USE_NEWDISPLAYBEEP
    UBYTE                    BeepingCounter;
#endif
};

#define GetPrivScreen(s)    ((struct IntScreen *)s)

/* SpecialFlags */
#define SF_IsParent 	    (0x0001)
#define SF_IsChild  	    (0x0002)
#define SF_InvisibleBar     (0x0004)
#define SF_AppearingBar     (0x0008)
#define SF_SysFont  	    (0x0010)
#define SF_Draggable	    (0x0020) /* Screen can be dragged		     */
#define SF_ComposeAbove	    (0x0100) /* Composition capabilities	     */
#define SF_ComposeBelow	    (0x0200)
#define SF_ComposeLeft	    (0x0400)
#define SF_ComposeRight	    (0x0800)

#define SF_VertCompose (SF_ComposeAbove|SF_ComposeBelow)
#define SF_HorCompose  (SF_ComposeLeft |SF_ComposeRight)
#define SF_Compose     (SF_ComposeAbove|SF_ComposeBelow|SF_ComposeLeft|SF_ComposeRight)

struct IntIntuiMessage
{
    struct ExtIntuiMessage  eimsg;

    /*
    ** The following field is needed, because in case of IDCMP_RAWKEY
    ** IntuiMessage->IAddress is a pointer to this data, not the data
    ** itself (which is the case for IDCMP_VANILLAKEY)
    */

    APTR                    prevCodeQuals;
};

#define INT_INTUIMESSAGE(x) 	((struct IntIntuiMessage *)(x))



/*extern struct IntuitionBase * IntuitionBase;*/

#define IW(window)  	    	((struct IntWindow *) (window))    

#define GetPubIBase(ib)     	((struct IntuitionBase *)ib)
#define GetPrivIBase(ib)    	((struct IntIntuitionBase *)ib)

/* FIXME: Remove these #define xxxBase hacks
   Do not use this in new code !
*/
#ifdef __MORPHOS__
#ifdef MUIMasterBase
#undef MUIMasterBase
#define MUIMasterBase 	    	(GetPrivIBase(IntuitionBase)->MUIMasterBase)
#endif
#endif

/* struct Utilitybase is used in the following file so include it
   before defining Utilitybase
*/
#include <proto/utility.h>

#ifdef UtilityBase
#undef UtilityBase
#endif
#define UtilityBase   	    	(GetPrivIBase(IntuitionBase)->UtilityBase)

#ifdef LayersBase
#undef LayersBase
#endif
#define LayersBase   	    	(GetPrivIBase(IntuitionBase)->LayersBase)

#ifdef OOPBase
#undef OOPBase
#endif
#define OOPBase   	    	(GetPrivIBase(IntuitionBase)->OOPBase)

#ifdef GfxBase
#undef GfxBase
#endif
#define GfxBase   	    	(GetPrivIBase(IntuitionBase)->GfxBase)

#ifdef KeymapBase
#undef KeymapBase
#endif
#define KeymapBase   	    	(GetPrivIBase(IntuitionBase)->KeymapBase)

#ifdef InputBase
#undef InputBase
#endif
#define InputBase   	    	(GetPrivIBase(IntuitionBase)->InputBase)

#ifdef TimerBase
#undef TimerBase
#endif
#define TimerBase   	    	(GetPrivIBase(IntuitionBase)->TimerBase)

#ifdef TimerMP
#undef TimerMP
#endif
#define TimerMP     	    	(GetPrivIBase(IntuitionBase)->TimerMP)

#ifdef TimerIO
#undef TimerIO
#endif
#define TimerIO     	    	(GetPrivIBase(IntuitionBase)->TimerIO)

#define PublicClassList     	((struct List *)&(GetPrivIBase(IntuitionBase)->ClassList))


/* stegerg: one can have sysgadgets outside of window border! All sysgadgets in window
            border must have set GACT_???BORDER and, if they are in a gzz window, also
        GTYP_GZZGADGET */

#define IS_GZZ_GADGET(gad)  	(((gad)->GadgetType) & GTYP_GZZGADGET)

#define IS_BORDER_GADGET(gad) 	(IS_GZZ_GADGET(gad) || \
    	    	    	    	 ((gad)->Activation & (GACT_RIGHTBORDER|GACT_LEFTBORDER|GACT_TOPBORDER|GACT_BOTTOMBORDER)))

#define IS_SYS_GADGET(gad)  	(((gad)->GadgetType) & GTYP_SYSTYPEMASK)

#define IS_BOOPSI_GADGET(gad) 	(((gad)->GadgetType & GTYP_GTYPEMASK) == GTYP_CUSTOMGADGET)

/*#define IS_BORDER_GADGET(gad) \
    (((gad->GadgetType) & GTYP_SYSGADGET) \
    || ((gad)->Activation & (GACT_RIGHTBORDER|GACT_LEFTBORDER|GACT_TOPBORDER|GACT_BOTTOMBORDER))) */

#define IS_REQ_GADGET(gad)  	((gad)->GadgetType & GTYP_REQGADGET)

#define IS_SCREEN_GADGET(gad) 	((gad)->GadgetType & GTYP_SCRGADGET)

#define SET_GI_RPORT(gi, w, req, gad)   \
    (gi)->gi_RastPort = (IS_SCREEN_GADGET(gad) ? \
    ((gi)->gi_Screen->BarLayer ? (gi)->gi_Screen->BarLayer->rp : NULL) : \
    (IS_BORDER_GADGET(gad) ?  (w)->BorderRPort : \
    ((req) ? ((req)->ReqLayer->rp) : (w)->RPort)) \
)


#define REFRESHGAD_BOOPSI   	1   /* boopsi gadgets */
#define REFRESHGAD_BORDER   	2   /* gadgets in window border */
#define REFRESHGAD_REL      	4   /* gadgets with GFLG_RELRIGHT, GFLG_RELBOTTOM,GFLG_RELWIDTH, GFLG_RELHEIGHT */
#define REFRESHGAD_RELS     	8   /* GFLG_RELSPECIAL gadgets */
#define REFRESHGAD_TOPBORDER    16  /* used by setwindowtitle */
#define REFRESHGAD_NOGADTOOLS   32  /* used in some cases for _mustbe_ */

#define SYSGADGET_ACTIVE (iihdata->ActiveSysGadget != NULL)

VOID int_refreshglist(struct Gadget *gadgets, struct Window *window,
                      struct Requester *requester, LONG numGad, LONG mustbe, LONG mustnotbe,
                      struct IntuitionBase *IntuitionBase);

VOID int_RefreshWindowFrame(struct Window *window, LONG mustbe, LONG mustnotbe,
                            LONG mode,
                            struct IntuitionBase *IntuitionBase);

#define int_refreshwindowframe(a,b,c,d) int_RefreshWindowFrame(a,b,c,0,d);

/* Keep the system gadgets in the same order than the original intuition,
 * some programs make bad asumptions about that...
 */
enum
{
    DEPTHGAD,
    ZOOMGAD,
    SIZEGAD,
    CLOSEGAD,
    ICONIFYGAD,
    LOCKGAD,
    MUIGAD,
    POPUPGAD,
    SNAPSHOTGAD,
    JUMPGAD,
    DRAGBAR,
    NUM_SYSGADS
};

#define SYSGAD(w, idx) (((struct IntWindow *)(w))->sysgads[idx])

struct IntWindow
{
    struct Window           window;

    Object                 *sysgads[NUM_SYSGADS];
    struct Image           *AmigaKey;
    struct Image           *Checkmark;
    struct Image           *SubMenuImage;
    struct Window          *menulendwindow;
    
    struct HashNode         hashnode;

    /* When the Zoom gadget is pressed the window will have the
       dimensions stored here. The old dimensions are backed up here
       again. */
    WORD                    ZipLeftEdge;
    WORD                    ZipTopEdge;
    WORD                    ZipWidth;
    WORD                    ZipHeight;

    /* max. number of mousemove events to send to this window */
    WORD                    mousequeue;

    /* act. number of mousemove events sent to this window */
    WORD                    num_mouseevents;

    /* max. number of repeated IDCMP_RAWKEY, IDCMP_VANILLAKEY and IDCMP_IDCMPUPDATE
       messages to send to this window */
    WORD                    repeatqueue;

    /* act. number of repeated IDCMP_RAWKEY, IDCMP_VANILLAKEY and IDCMP_IDCMPUPDATE
       messages sent to this window */
    WORD                    num_repeatevents;

#if USE_IDCMPUPDATE_MESSAGECACHE
    /* number of unreplied IDCMP_IDCMPUPDATE messages sent to window
       used in few hacks :) */
    WORD                    num_idcmpupdate;
#endif

    WORD                    sizeimage_width;
    WORD                    sizeimage_height;

    ULONG                   helpflags;
    ULONG                   helpgroup;

    Object                 *pointer;
    BOOL                    busy;
    BOOL                    free_pointer;
    UWORD                   pointer_delay;
    ULONG                   extrabuttons;
    ULONG                   extrabuttonsid; /* replaces ETI_Dummy if apps wishes it */
#ifdef SKINS
    ULONG                   titlepos; /* used by titlebar pattern fill stuff */
#else
    ULONG                   reserved1;
#endif
    ULONG                   specialflags;

#ifdef SKINS
    struct Region          *transpregion;
    struct Hook            *usertransphook;
    struct Region          *usertranspregion;
#endif

#ifdef DAMAGECACHE
    struct Region          *trashregion;
#endif
    char                    titlebuffer[TITLEBUFFERLEN+1];

#if USE_IDCMPUPDATE_MESSAGECACHE
    struct IntuiMessage    *messagecache; //for idcmpupdate cache
#endif

    struct Layer           *wlayer;
    struct Layer           *borderlayer;

    struct timeval          lastmsgsent;
    struct timeval          lastmsgreplied;

#ifdef TIMEVALWINDOWACTIVATION
    struct timeval          activationtime;
#endif

#ifdef SKINS
    struct Hook             custombackfill;
    struct HookData         hd;
#endif
    struct Hook     	    DefaultWindowShapeHook;
    BOOL                    CustomShape;
    struct Region          *OutlineShape;
    ULONG                   DecorUserBufferSize;
    IPTR                    DecorUserBuffer;
};

#define SPFLAG_ICONIFIED    	1
#define SPFLAG_NOICONIFY    	2
#define SPFLAG_SKININFO     	4
#define SPFLAG_LAYERREFRESH 	8
#define SPFLAG_TRANSPHOOK   	16
#define SPFLAG_LAYERRESIZED 	32
#define SPFLAG_USERPORT     	64
#define SPFLAG_IAMDEAD      	128
#define SPFLAG_CLOSING      	256 //used with iamdead
#define SPFLAG_WANTBUFFER   	512

#define HELPF_ISHELPGROUP   	1
#define HELPF_GADGETHELP    	2

#define IS_NOCAREREFRESH(win)   (((win)->Flags & WFLG_NOCAREREFRESH) ? TRUE  : FALSE)
#define IS_DOCAREREFRESH(win) 	(((win)->Flags & WFLG_NOCAREREFRESH) ? FALSE : TRUE )
#define IS_SIMPLEREFRESH(win) 	(((win)->Flags & WFLG_SIMPLE_REFRESH) ? TRUE : FALSE)
#define IS_GZZWINDOW(win)     	(((win)->Flags & WFLG_GIMMEZEROZERO) ? TRUE  : FALSE)


/* Flag definitions for MoreFlags */

#define WMFLG_NOTIFYDEPTH        (1 << 0)   /* Window wants notification when
it's depth arranged */

#define WMFLG_DO_UNLOCKPUBSCREEN (1 << 1)
#define WMFLG_MENUHELP           (1 << 2)
#define WMFLG_POINTERDELAY       (1 << 3)
#define WMFLG_TABLETMESSAGES     (1 << 4)

// FIXME: ehm... this one should die a horrible death!
#define WMFLG_IAMMUI             (1 << 5)

struct IntScreenBuffer
{
    struct ScreenBuffer sb;
    BOOL                free_bitmap;
};


/* Driver prototypes */

extern int  intui_init (struct IntuitionBase *);
extern int  intui_open (struct IntuitionBase *);
extern void intui_close (struct IntuitionBase *);
extern void intui_expunge (struct IntuitionBase *);
extern int intui_GetWindowSize (void);
extern void intui_WindowLimits (struct Window * window, WORD MinWidth, WORD MinHeight, UWORD MaxWidth, UWORD MaxHeight);
extern void intui_ActivateWindow (struct Window *);
extern BOOL intui_ChangeWindowBox (struct Window * window, WORD x, WORD y,
    	    	    	    	   WORD width, WORD height);
extern void intui_CloseWindow (struct Window *, struct IntuitionBase *);
extern void intui_MoveWindow (struct Window * window, WORD dx, WORD dy);
extern int  intui_OpenWindow (struct Window *, struct IntuitionBase *,
    	    	    	      struct BitMap * SuperBitMap, struct Hook *backfillhook,
    	    	    	      struct Region * shape, struct Hook * shapehook,
    	    	    	      struct Layer * parent, ULONG visible);
extern void intui_SetWindowTitles (struct Window *, CONST_STRPTR, CONST_STRPTR);
extern void intui_RefreshWindowFrame(struct Window *win);
extern struct Window *intui_FindActiveWindow(struct InputEvent *ie, struct IntuitionBase *IntuitionBase);
extern void intui_ScrollWindowRaster(struct Window * win, WORD dx, WORD dy, WORD xmin,
    	    	    	    	     WORD ymin, WORD xmax, WORD ymax,
				     struct IntuitionBase * IntuitionBase);

/* wbtasktalk protos */

ULONG TellWBTaskToCloseWindows(struct IntuitionBase *IntuitionBase);
ULONG TellWBTaskToOpenWindows(struct IntuitionBase *IntuitionBase);

/* intuition_misc protos */
extern void LoadDefaultPreferences(struct IntuitionBase * IntuitionBase);
Object* CreateStdSysImage(WORD which, WORD preferred_height, struct Screen *scr, APTR buffer,
    	    	    	  struct DrawInfo *dri, struct IntuitionBase *IntuitionBase);
extern void CheckRectFill(struct RastPort *rp, WORD x1, WORD y1, WORD x2, WORD y2, struct IntuitionBase * IntuitionBase);
extern BOOL CreateWinSysGadgets(struct Window *w, struct IntuitionBase *IntuitionBase);
extern VOID KillWinSysGadgets(struct Window *w, struct IntuitionBase *IntuitionBase);
extern void CreateScreenBar(struct Screen *scr, struct IntuitionBase *IntuitionBase);
extern void KillScreenBar(struct Screen *scr, struct IntuitionBase *IntuitionBase);
extern void RenderScreenBar(struct Screen *scr, BOOL refresh, struct IntuitionBase *IntuitionBase);
extern void UpdateMouseCoords(struct Window *win);
extern WORD SubtractRectFromRect(struct Rectangle *a, struct Rectangle *b, struct Rectangle *destrectarray);

AROS_UFP3(struct Region *, DefaultWindowShapeFunc,
    AROS_UFPA(struct Hook *, hook, A0),
    AROS_UFPA(struct Layer *, lay, A2),
    AROS_UFPA(struct ShapeHookMsg *, msg, A1));

extern LONG CalcResourceHash(APTR resource);
extern void AddResourceToList(APTR resource, UWORD resourcetype, struct IntuitionBase *IntuitionBase);
extern void RemoveResourceFromList(APTR resource, UWORD resourcetype, struct IntuitionBase *IntuitionBase);
extern BOOL ResourceExisting(APTR resource, UWORD resourcetype, struct IntuitionBase *IntuitionBase);
void FireScreenNotifyMessage(IPTR data, ULONG flag, struct IntuitionBase *IntuitionBase);
void FireScreenNotifyMessageCode(IPTR data, ULONG flag, ULONG code, struct IntuitionBase *IntuitionBase);

/* misc.c */
extern void MySetPointerPos(struct IntuitionBase *IntuitionBase);
extern BOOL ResetPointer(struct IntuitionBase *IntuitionBase);
extern void ActivateMonitor(Object *newmonitor, WORD x, WORD y, struct IntuitionBase *IntuitionBase);
extern struct Screen *FindFirstScreen(Object *monitor, struct IntuitionBase *IntuitionBase);
struct TextFont *SafeReopenFont(struct IntuitionBase *, struct TextFont **);
extern Object *MakePointerFromPrefs(struct IntuitionBase *, struct Preferences *);
extern Object *MakePointerFromData(struct IntuitionBase *, UWORD *, int, int, int, int);
void InstallPointer(struct IntuitionBase *, UWORD, Object **, Object *);
void SetPointerColors(struct IntuitionBase *IntuitionBase);
struct IClass *InitITextIClass (struct IntuitionBase * IntuitionBase);
struct Gadget *DoActivateGadget(struct Window *win, struct Requester *req, struct Gadget *gad,
struct IntuitionBase *IntuitionBase);
VOID DoGMLayout(struct Gadget *glist, struct Window *win, struct Requester *req,
    	    	UWORD numgad, BOOL initial, struct IntuitionBase *IntuitionBase);
BOOL ih_fire_intuimessage(struct Window * w, ULONG Class, UWORD Code, APTR IAddress,
    	    	    	  struct IntuitionBase *IntuitionBase);
void NotifyDepthArrangement(struct Window *w, struct IntuitionBase *IntuitionBase);

/* printitext.c */

void int_PrintIText(struct RastPort * rp, struct IntuiText * iText,
            	    LONG leftOffset, LONG topOffset, BOOL ignore_attributes,
		    struct IntuitionBase *IntuitionBase);

#ifdef __MORPHOS__
BOOL IsLayerHiddenBySibling(struct Layer *layer, BOOL xx);
LONG IsLayerVisible(struct Layer *layer);
#endif

void SetupGInfo(struct GadgetInfo *gi, struct Window *win, struct Requester *req,
struct Gadget *gad, struct IntuitionBase *IntuitionBase);

IPTR Custom_DoMethodA(struct IntuitionBase *, struct Gadget *, Msg);

#ifdef __MORPHOS__
#ifdef DoMethodA
#undef DoMethodA
#endif
#define DoMethodA(x, ...)   	    (REG_A6=(LONG)IntuitionBase, DoMethodA(x, __VA_ARGS__))
#endif

#define Custom_DoMethodA(x, ...)    Custom_DoMethodA(IntuitionBase, x, __VA_ARGS__)

#define HAS_CHILDREN(w)     	    (NULL != w->firstchild)

#ifdef __MORPHOS__
#define DeinitRastPort(rp)  	    ((void)0)
#define CreateRastPort()            MyCreateRastPort(IntuitionBase)
#define CloneRastPort(rp)           MyCloneRastPort(IntuitionBase, rp)
#define FreeRastPort(rp)            MyFreeRastPort(IntuitionBase, rp)
#define SetPointerPos(x, y)         MySetPointerPos(IntuitionBase, x, y)
#define MouseCoordsRelative()       1
#endif

/* Replacement for dos.library/DisplayError() */
AROS_UFP3(LONG, Intuition_DisplayError,
AROS_UFPA(STRPTR, formatStr , A0),
AROS_UFPA(ULONG , IDCMPFlags, D0),
AROS_UFPA(APTR  , args      , A1));

#define POINTERA_SharedPointer  0x80039010

#ifndef __MORPHOS__
#define dprintf kprintf
#endif

#define DEBUG_ACTIVATEGADGET(x)     	;
#define DEBUG_ACTIVATEWINDOW(x)     	;
#define DEBUG_ADDCLASS(x)           	;
#define DEBUG_ADDGADGET(x)          	;
#define DEBUG_ADDGLIST(x)           	;
#define DEBUG_ALLOCINTUIMESSAGE(x)  	;
#define DEBUG_ALLOCSCREENBUFFER(x)  	;
#define DEBUG_ALOHAWORKBENCH(x)     	;
#define DEBUG_CHANGEWINDOWBOX(x)    	;
#define DEBUG_CLOSESCREEN(x)        	;
#define DEBUG_CLOSEWINDOW(x)        	;
#define DEBUG_CLOSEWORKBENCH(x)     	;
#define DEBUG_DISPOSEOBJECT(x)      	;
#define DEBUG_DOGADGETMETHOD(x)     	;
#define DEBUG_DRAWBORDER(x)         	;
#define DEBUG_FINDCLASS(x)          	;
#define DEBUG_FREEICDATA(x)         	;
#define DEBUG_FREEINTUIMESSAGE(x)   	;
#define DEBUG_FREESCREENBUFFER(x)   	;
#define DEBUG_FREESCREENDRAWINFO(x) 	;
#define DEBUG_GADGETMOUSE(x)        	;
#define DEBUG_GETATTR(x)            	;
#define DEBUG_GETDEFAULTPUBSCREEN(x)    ;
#define DEBUG_GETDEFPREFS(x)        	;
#define DEBUG_GETGADGETIBOX(x)      	;
#define DEBUG_GETPREFS(x)           	;
#define DEBUG_GETSCREENDATA(x)      	;
#define DEBUG_GETSCREENDRAWINFO(x)  	;
#define DEBUG_HIDEWINDOW(x)         	;
#define DEBUG_OPEN(x)               	;
#define DEBUG_CLOSE(x)              	;
#define DEBUG_INPUTEVENT(x)         	;
#define DEBUG_INTREFRESHGLIST(x)    	;
#define DEBUG_INTUITEXTLENGTH(x)    	;
#define DEBUG_LENDMENUS(x)          	;
#define DEBUG_LOCKPUBSCREEN(x)      	;
#define DEBUG_MAKECLASS(x)          	;
#define DEBUG_MODIFYIDCMP(x)        	;
#define DEBUG_NEWOBJECT(x)          	;
#define DEBUG_NEXTOBJECT(x)         	;
#define DEBUG_OBTAINGIRPORT(x)      	;
#define DEBUG_OFFGADGET(x)          	;
#define DEBUG_OFFMENU(x)            	;
#define DEBUG_ONGADGET(x)           	;
#define DEBUG_ONMENU(x)             	;
#define DEBUG_OPENSCREEN(x)         	;
#define DEBUG_OPENSCREENTAGLIST(x)  	;
#define DEBUG_OPENWINDOW(x)         	;
#define DEBUG_OPENWINDOWTAGLIST(x)  	;
#define DEBUG_OPENWORKBENCH(x)      	;
#define DEBUG_POINTER(x)            	;
#define DEBUG_PRINTITEXT(x)         	;
#define DEBUG_QUERYOVERSCAN(x)      	;
#define DEBUG_REFRESH(x)            	;
#define DEBUG_RELEASEGIRPORT(x)     	;
#define DEBUG_REMEMBER(x)           	;
#define DEBUG_REMOVEGLIST(x)        	;
#define DEBUG_REPORTMOUSE(x)        	;
#define DEBUG_REQUEST(x)            	;
#define DEBUG_SCROLLWINDOWRASTER(x) 	;
#define DEBUG_SENDINTUIMESSAGE(x)   	;
#define DEBUG_SETATTRS(x)           	;
#define DEBUG_SETGADGETATTRS(x)     	;
#define DEBUG_SETPOINTER(x)         	;
#define DEBUG_SHOWWINDOW(x)         	;
#define DEBUG_UNLOCKPUBSCREEN(x)    	;
#define DEBUG_WINDOWLIMITS(x)       	;
#define DEBUG_WINDOWTOBACK(x)       	;
#define DEBUG_WINDOWTOFRONT(x)      	;
#define DEBUG_ZIPWINDOW(x)          	;
#define DEBUG_VISITOR(x)            	;
#define DEBUG_WORKBENCH(x)          	;
#define DEBUG_LOCKPUBSCREENLIST(x)  	;
#define DEBUG_UNLOCKPUBSCREENLIST(x)    ;
#define DEBUG_RETHINKDISPLAY(x)   	;

#ifdef NO_RUNTIME_DEBUG

#define DEBUG_INIT(x)               	;
#define DEBUG_SETIPREFS(x)          	;
#define DEBUG_SETPREFS(x)           	;

#else

#define DEBUG_INIT(x)               	if (SysBase->ex_DebugFlags & EXECDEBUGF_INIT) x;
#define DEBUG_SETIPREFS(x)          	if (SysBase->ex_DebugFlags & EXECDEBUGF_INIT) x;
#define DEBUG_SETPREFS(x)           	if (SysBase->ex_DebugFlags & EXECDEBUGF_INIT) x;

#endif

/*
 * Private data structures of the classes defined by intuition.library
 */

/* ICClass */
struct ICData
{
    Object          *ic_Target;
    struct TagItem  *ic_Mapping;
    struct TagItem  *ic_CloneTags;
    ULONG            ic_LoopCounter;
};

/* ModelClass */
struct ModelData
{
    struct MinList memberlist;
};

/* FrameIClass */
struct FrameIData
{
    /* render bevel only with no fill? */
    BOOL fid_EdgesOnly;

    /* inverted bevel pens? */
    BOOL fid_Recessed;

    /* frame style? */
    WORD fid_FrameType;

    WORD fid_HOffset;
    WORD fid_VOffset;
};

/* SysIClass */
struct SysIData
{
    struct DrawInfo *dri;
    struct Image    *frame;
    APTR            userbuffer;
    ULONG            type;
    UWORD            flags;
};

/* FillRectClass */
struct FillRectData
{
    WORD apatsize;
    WORD mode;
};

/* GadgetClass */
struct GadgetData
{
    struct ExtGadget    EG;
    struct ICData       IC;
};

/* PropGClass */
struct PropGData
{
    /* We use a propinfo structure, because we use the same routines
    for intuition propgadtets and in propgclass */

    struct PropInfo  propinfo;

    /* A little kludge: since the HandleMouseMove function
    wants dx/dy and not absolute mouse coords, we
    have to remember the last ones */

    UWORD            last_x;
    UWORD            last_y;

    /* One only have to store total or visble, and use some other
    formulas than those in the RKRM:L, but for
    code simplicity I store them all.  */
    UWORD            top, visible, total;

    UWORD            flags;
    struct BBox     *old_knobbox;
    struct Hook     *DisplayHook;
};

/* StrGClass */
struct StrGData
{
    struct StringInfo   StrInfo;
    struct StringExtend StrExtend;
    UBYTE           	Flags;
};

/* GroupGClass */
struct GroupGData
{
    struct MinList   memberlist;
    struct Gadget    *activegad;
};

/* DragBarClass */
struct dragbar_data
{
    /* Current left- and topedge of moving window. Ie when the user releases
    the LMB after a windowdrag, the window's new coords will be (curleft, curtop)
    */

    LONG    	     curleft;
    LONG    	     curtop;

    /* The current x and y coordinates relative to curleft/curtop */
    LONG    	     mousex;
    LONG    	     mousey;

    /* Whether the dragframe is currently drawn or erased */
    BOOL    	     isrendered;

    /* Used to tell GM_GOINACTIVE whether the drag was canceled or not */
    BOOL    	     drag_canceled;

    /* Used to tell GM_GOINACTIVE whether UnlockLayer() or not */
    BOOL    	     drag_layerlock;
#ifdef USEGADGETLOCK
    BOOL    	     drag_gadgetlock;
    BOOL    	     drag_inputhandlerlock;
#endif
    BOOL    	     drag_refreshed;

    /* Rastport to use during update */
    struct RastPort *rp;

    UQUAD   	     lasteventtime;
    
    LONG    	     startleft;
    LONG    	     starttop;

#ifdef USEWINDOWLOCK
    /* used to prevent windows from opening/closing while user drags a window */
    BOOL    	     drag_windowlock;
#endif

    struct Task     *movetask;

};

/* SizeButtonClass */
struct sizebutton_data
{

    /* The current width and height of the rubber band frame */
    ULONG   	     width;
    ULONG   	     height;
    ULONG   	     top;
    ULONG   	     left;

    /* holds original sizes */
    ULONG   	     Width;
    ULONG   	     Height;
    ULONG   	     TopEdge;
    ULONG   	     LeftEdge;

    /* the offset of the mouse pointer to the rubber band frame*/
    LONG    	     mouseoffsetx;
    LONG    	     mouseoffsety;

    /* Whether the dragframe is currently drawn or erased */
    BOOL    	     isrendered;

    /* Used to tell GM_GOINACTIVE whether the drag was canceled or not */
    BOOL    	     drag_canceled;

    /* Used to tell GM_GOINACTIVE whether UnlockLayer() or not */
    BOOL    	     drag_layerlock;
#ifdef USEGADGETLOCK
    BOOL    	     drag_gadgetlock;
    BOOL    	     drag_inputhandlerlock;
#endif

    BOOL    	     drag_refreshed;

    /* Rastport to use during update */
    struct RastPort *rp;

    UQUAD   	     lasteventtime;

#ifdef USEWINDOWLOCK
    /* used to prevent windows from opening/closing while user drags a window */
    BOOL    	     drag_windowlock;
#endif

#ifdef SKINS
    ULONG   	     drag_type;
#endif

    ULONG   	     drag_ticks;

};

/* MenuBarLabelClass */
struct MenuBarLabelData
{
    struct DrawInfo *dri;
};

/* PointerClass */
struct PointerData
{
    struct SharedPointer *shared_pointer;
};

/* WinDecorClass */
struct windecor_data
{
	ULONG	userbuffersize;
};

/* ScrDecorClass */
struct scrdecor_data
{
	ULONG	userbuffersize;
};

/* MenuDecorClass */
struct menudecor_data
{
	ULONG	userbuffersize;
};

#endif /* INTUITION_INTERN_H */
