#ifndef INTUITION_INTERN_H
#define INTUITION_INTERN_H
/*
    (C) 1995-96 AROS - The Amiga Research OS
    $Id$

    Desc: Intuitions internal structure
    Lang: english
*/
#ifndef AROS_LIBCALL_H
#   include <aros/libcall.h>
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
#ifndef GRAPHICS_GFXBASE_H
#   include <graphics/gfxbase.h>
#endif
#ifndef GRAPHICS_RASTPORT_H
#   include <graphics/rastport.h>
#endif
#ifndef GRAPHICS_CLIP_H
#   include <graphics/clip.h>
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
#ifndef INTUITION_SCREENS_H
#   include <intuition/screens.h>
#endif
#include "intuition_debug.h"

/* Needed for aros_print_not_implemented macro */
#include <aros/debug.h>

#include <aros/asmcall.h>

/* ObtainGIRPort must install a 0 clipregion and
   set scrollx/scrolly of layer to 0. Since this
   will be restored only when ReleaseGIRPort is
   called, we must backup the orig values somewhere */
   
struct LayerContext
{
    struct Region *clipregion;
    WORD scroll_x;
    WORD scroll_y;
    WORD nestcount;
};

struct IntIntuitionBase
{
    struct IntuitionBase IBase;

    /* Put local shit here, invisible for the user */
    struct GfxBase	   * GfxBase;
    struct Library         * LayersBase;
    struct ExecBase	   * SysBase;
    struct UtilityBase	   * UtilBase;
    struct Library	   * BOOPSIBase;
    struct Library	   * KeymapBase;
    struct Library         * DOSBase;
    struct Library	   * TimerBase;
    struct MsgPort	   * TimerMP;
    struct timerequest	   * TimerIO;

    struct MsgPort	   * WorkBenchMP;
    struct Screen	   * WorkBench;
    struct SignalSemaphore * IBaseLock;

    /* Intuition input handlers replyport. This one is set
    int rom/inputhandler.c/InitIIH()
    */
    struct MsgPort	   * IntuiReplyPort;
    struct MsgPort	   * IntuiDeferedActionPort;
    struct IOStdReq	   * InputIO;
    struct MsgPort	   * InputMP;
    BOOL		     InputDeviceOpen;
    struct Interrupt	   * InputHandler;

    struct Hook		   *GlobalEditHook;
    /* The default global edit hook */
    struct Hook		   DefaultEditHook;
    
    APTR		     DriverData; /* Pointer which the driver may use */

    struct Screen	    *DefaultPubScreen;
    struct SignalSemaphore   PubScrListLock;
    struct MinList	     PubScreenList;
    UWORD                    pubScrGlobalMode;

    struct SignalSemaphore   GadgetLock;
    struct LayerContext      BackupLayerContext;
    
    struct IClass *dragbarclass;
    struct IClass *tbbclass; /* Titlebar button class. (close, zoom, depth) */
    struct IClass *sizebuttonclass;
    
    struct Preferences	    *DefaultPreferences;
    struct Preferences	    *ActivePreferences;
};

struct IntScreen
{
    struct Screen Screen;

    /* Private fields */
    struct DrawInfo DInfo;
    UWORD  Pens[NUMDRIPENS];
    struct PubScreenNode *pubScrNode;
    UWORD  SpecialFlags;
};

#define GetPrivScreen(s)	((struct IntScreen *)s)

/* SpecialFlags */
#define SF_IsParent	(0x0001)
#define SF_IsChild	(0x0002)


struct IntRequestUserData
{
    ULONG    IDCMP;
    STRPTR * GadgetLabels;
    struct Gadget *Gadgets;
    UWORD NumGadgets;
};




extern struct IntuitionBase * IntuitionBase;

#define GetPubIBase(ib)   ((struct IntuitionBase *)ib)
#define GetPrivIBase(ib)  ((struct IntIntuitionBase *)ib)

#ifdef GfxBase
#undef GfxBase
#endif
#define _GfxBase     (GetPrivIBase(IntuitionBase)->GfxBase)
#define GfxBase     _GfxBase

#ifdef LayersBase
#undef LayersBase
#endif
#define _LayersBase     (GetPrivIBase(IntuitionBase)->LayersBase)
#define LayersBase     _LayersBase

#ifdef SysBase
#undef SysBase
#endif
#define SysBase     (GetPrivIBase(IntuitionBase)->SysBase)

#ifdef UtilityBase
#undef UtilityBase
#endif
#define UtilityBase (GetPrivIBase(IntuitionBase)->UtilBase)

#ifdef KeymapBase
#undef KeymapBase
#endif
#define KeymapBase (GetPrivIBase(IntuitionBase)->KeymapBase)

#ifdef TimerBase
#undef TimerBase
#endif
#define TimerBase (GetPrivIBase(IntuitionBase)->TimerBase)

#ifdef TimerMP
#undef TimerMP
#endif
#define TimerMP (GetPrivIBase(IntuitionBase)->TimerMP)

#ifdef TimerIO
#undef TimerIO
#endif
#define TimerIO (GetPrivIBase(IntuitionBase)->TimerIO)

#ifdef BOOPSIBase
#undef BOOPSIBase
#endif
#define BOOPSIBase (GetPrivIBase(IntuitionBase)->BOOPSIBase)


#ifdef DOSBase
#undef DOSBase
#endif
#define DOSBase (GetPrivIBase(IntuitionBase)->DOSBase)

#define PublicClassList ((struct List *)&(GetPrivIBase(IntuitionBase)->ClassList))

/* Needed for close() */
#define expunge() \
    AROS_LC0(BPTR, expunge, struct IntuitionBase *, IntuitionBase, 3, Intuition)


/* stegerg: one can have sysgadgets outside of window border! All sysgadgets in window
            border must have set GACT_???BORDER and, if they are in a gzz window, also
	    GTYP_GZZGADGET */
	    
#define IS_BORDER_GADGET(gad) \
	(((gad->GadgetType) & GTYP_GZZGADGET) \
	|| ((gad)->Activation & (GACT_RIGHTBORDER|GACT_LEFTBORDER|GACT_TOPBORDER|GACT_BOTTOMBORDER)))
	
/*#define IS_BORDER_GADGET(gad) \
	(((gad->GadgetType) & GTYP_SYSGADGET) \
	|| ((gad)->Activation & (GACT_RIGHTBORDER|GACT_LEFTBORDER|GACT_TOPBORDER|GACT_BOTTOMBORDER))) */

#define SET_GI_RPORT(gi, w, gad)	\
	(gi)->gi_RastPort = (IS_BORDER_GADGET(gad) ?  (w)->BorderRPort : (w)->RPort)



/* Driver prototypes */
extern int  intui_init (struct IntuitionBase *);
extern int  intui_open (struct IntuitionBase *);
extern void intui_close (struct IntuitionBase *);
extern void intui_expunge (struct IntuitionBase *);
extern int intui_GetWindowSize (void);
extern void intui_WindowLimits (struct Window * window,
	    WORD MinWidth, WORD MinHeight, UWORD MaxWidth, UWORD MaxHeight);
extern void intui_ActivateWindow (struct Window *);
extern BOOL intui_ChangeWindowBox (struct Window * window, WORD x, WORD y,
	    WORD width, WORD height);
extern void intui_CloseWindow (struct Window *, struct IntuitionBase *);
extern void intui_MoveWindow (struct Window * window, WORD dx, WORD dy);
extern int  intui_OpenWindow (struct Window *, struct IntuitionBase *, 
			      struct BitMap * SuperBitMap, struct Hook *backfillhook);
extern void intui_SetWindowTitles (struct Window *, UBYTE *, UBYTE *);
extern void intui_RefreshWindowFrame(struct Window *win);
extern struct Window *intui_FindActiveWindow(struct InputEvent *ie, BOOL *swallow_event, struct IntuitionBase *IntuitionBase);
void intui_ScrollWindowRaster(struct Window * win, WORD dx, WORD dy, WORD xmin,
                           WORD ymin, WORD xmax, WORD ymax, struct IntuitionBase * IntuitionBase);

/* Miscellaneous prototypes */
void intrequest_freelabels(STRPTR *gadgetlabels, struct IntuitionBase *IntuitionBase);
void intrequest_freegadgets(struct Gadget *gadgets, struct IntuitionBase *IntuitionBase);

void windowneedsrefresh(struct Window * w, struct IntuitionBase * IntuitionBase);

extern void CheckRectFill(struct RastPort *rp, WORD x1, WORD y1, WORD x2, WORD y2); 

void LoadDefaultPreferences(struct IntuitionBase * IntuitionBase);

/* Replacement for dos.library/DisplayError() */
AROS_UFP3(LONG, Intuition_DisplayError,
    AROS_UFPA(STRPTR, formatStr , A0),
    AROS_UFPA(ULONG , IDCMPFlags, D0),
    AROS_UFPA(APTR  , args      , A1));


struct closeMessage
{
    struct Message ExecMessage;
    UWORD Code;
    struct Window *Window;
    /* Task calling CloseWindow() */
    struct Task *closeTask;
    
};

/* Called by intuition to free a window */
VOID int_closewindow(struct closeMessage *msg, struct IntuitionBase *IntuitionBase);
VOID int_activatewindow(struct Window *window, struct IntuitionBase *IntuitionBase);

#define REFRESHGAD_BOOPSI 1		/* boopsi gadgets */
#define REFRESHGAD_BORDER 2		/* gadgets in window border */
#define REFRESHGAD_REL    4		/* gadgets with GFLG_RELRIGHT, GFLG_RELBOTTOM, GFLG_RELWIDTH, GFLG_RELHEIGHT */
#define REFRESHGAD_RELS   8		/* GFLG_RELSPECIAL gadgets */

VOID int_refreshglist(struct Gadget *gadgets, struct Window *window,
		      struct Requester *requester, LONG numGad, LONG mustbe, LONG mustnotbe,
		      struct IntuitionBase *IntuitionBase);
		      
struct IntWindow
{
    struct Window window;
    
    /* This message is sent to the intuition input handler when
       a window should be closed. We allocate it
       in OpenWindow(), so that CloseWindow 
       since CloseWindow() may not fail.
    */
       
    struct closeMessage *closeMessage;
    
    /* When the Zoom gadget is pressed the window will have the
       dimensions stored here. The old dimensions are backed up here
       again. */
    WORD ZipLeftEdge;
    WORD ZipTopEdge;
    WORD ZipWidth;
    WORD ZipHeight;
    
    /* max. number of mousemove events to send to this window */
    WORD mousequeue;
    /* act. number of mousemove events sent to this window */
    WORD num_mouseevents;
    /* max. number of repeated IDCMP_RAWKEY, IDCMP_VANILLAKEY and IDCMP_IDCMPUPDATE
       messages to send to this window */
    WORD repeatqueue;
    /* act. number of repeated IDCMP_RAWKEY, IDCMP_VANILLAKEY and IDCMP_IDCMPUPDATE
       messages sent to this window */
    WORD num_repeatevents;
    
    WORD sizeimage_width;
    WORD sizeimage_height;
};


/* IMCODE_WBENCHMESSAGE parameter structs
NOTE: The first two fields of these MUST be the
same as the first ones in IntuiMessage.

However we could instead use another message port in
the inputhandler to handle these messages and then add
a pointer to this one in struct IntWindow above.



Another note: Maybe use a union here to save space.

*/


struct DeferedActionMessage
{
    struct Message  ExecMessage;
    UWORD           Code; 
    struct Window * Window;
    struct Window * BehindWindow; /* only used by MoveWindowInFrontOf */
    struct Gadget * Gadget;
    struct Task   * Task;
    WORD            dx;           /* used by MoveLayer, SizeLayer */
    WORD            dy;           /* used by MoveLayer, SizeLayer */
    LONG	    left;
    LONG	    top;
    LONG	    width;
    LONG 	    height;
    
};

/* IDCMP_WBENCHMESSAGE parameters */


enum
{
	/* Sent from application task to intuition inside CloseWindow() */
	AMCODE_CLOSEWINDOW = 0,
	AMCODE_ACTIVATEWINDOW,
	AMCODE_SIZEWINDOW,
	AMCODE_WINDOWTOBACK,
	AMCODE_WINDOWTOFRONT,
	AMCODE_MOVEWINDOW,
	AMCODE_MOVEWINDOWINFRONTOF,
	AMCODE_ZIPWINDOW,
	AMCODE_CHANGEWINDOWBOX,
	AMCODE_NEWPREFS,
	AMCODE_ACTIVATEGADGET,
};


/* Flag definitions for MoreFlags */

#define  WMFLG_NOTIFYDEPTH  (1 << 0)     /* Window wants notification when
					    it's depth arranged */


#endif /* INTUITION_INTERN_H */
