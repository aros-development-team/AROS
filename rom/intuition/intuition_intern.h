#ifndef INTUITION_INTERN_H
#define INTUITION_INTERN_H
/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
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
#ifndef INTUITION_SCREENS_H
#   include <intuition/screens.h>
#endif
#include "intuition_debug.h"

/* Needed for aros_print_not_implemented macro */
#include <aros/debug.h>

#include <aros/asmcall.h>

/* Options */

#define MENUS_AMIGALOOK  1 /*0 for 3D, 1 for Black and White */
#define MENUS_UNDERMOUSE 0 /*0 for normal menus, 1 for magic menus */
#define FRAME_SIZE       0 /* 0 = 1:1 thin,  1 = 2:1 medres like AmigaOS,  2 = 1:1 thick */

#define INTERNAL_BOOPSI  1

#define USE_LOCKLAYERINFO_AS_REFRESHLOCK 1

#define SINGLE_SETPOINTERPOS_PER_EVENTLOOP 1

/* ObtainGIRPort must install a 0 clipregion and
   set scrollx/scrolly of layer to 0. Since this
   will be restored only when ReleaseGIRPort is
   called, we must backup the orig values somewhere */
   
struct LayerContext
{
    struct Region 		* clipregion;
    WORD 			scroll_x;
    WORD 			scroll_y;
    WORD 			nestcount;
};

#define RESOURCELIST_HASHSIZE 256

#define RESOURCE_WINDOW 1

struct HashNode
{
    struct MinNode node;
    UWORD   	   type;
    APTR    	   resource;
};

struct IntIntuitionBase
{
    struct IntuitionBase 	IBase;

    /* Put local shit here, invisible for the user */
    struct GfxBase	   	* GfxBase;
    struct Library         	* LayersBase;
    struct ExecBase	   	* SysBase;
    struct UtilityBase	   	* UtilBase;
#if !INTERNAL_BOOPSI
    struct Library	   	* BOOPSIBase;
#endif
    struct Library	   	* KeymapBase;
    struct Library         	* DOSBase;
    struct Library	   	* TimerBase;
    struct MsgPort	   	* TimerMP;
    struct timerequest	   	* TimerIO;

    struct MsgPort	   	* WorkBenchMP;
    struct Screen	   	* WorkBench;
    struct SignalSemaphore 	* IBaseLock;

    /* Intuition input handlers replyport. This one is set
    int rom/inputhandler.c/InitIIH()
    */
    struct MsgPort	   	* IntuiReplyPort;
    struct MinList	  	* IntuiActionQueue;
    struct IOStdReq	  	* InputIO;
    struct MsgPort	  	* InputMP;
    BOOL		     	InputDeviceOpen;
    struct Interrupt	   	* InputHandler;

    struct Hook		   	* GlobalEditHook;
    /* The default global edit hook */
    struct Hook		  	DefaultEditHook;
    
    APTR		     	DriverData; /* Pointer which the driver may use */

    struct Screen	    	* DefaultPubScreen;
    struct SignalSemaphore   	PubScrListLock;
    struct MinList	     	PubScreenList;
    UWORD                    	pubScrGlobalMode;

    struct SignalSemaphore   	GadgetLock;
    struct SignalSemaphore   	MenuLock;
    struct SignalSemaphore   	IntuiActionLock;
    struct SignalSemaphore  	InputHandlerLock;
    struct LayerContext      	BackupLayerContext;
    
    struct IClass 		* dragbarclass;
    struct IClass 		* sizebuttonclass;
    
    struct Preferences	   	* DefaultPreferences;
    struct Preferences	    	* ActivePreferences;
    
    struct MsgPort	    	* MenuHandlerPort;
    BOOL		    	MenusActive;

    struct TextFont 	    	* ScreenFont;
    
    /* Dos function DisplayError() before intuition.library patched it */
    APTR                        OldDisplayErrorFunc;
    
#if INTERNAL_BOOPSI
    struct SignalSemaphore  	ClassListLock;
    struct MinList  	    	ClassList;
    struct IClass   	    	RootClass;
#endif

    struct MinList  	    	ResourceList[RESOURCELIST_HASHSIZE];
};

struct IntScreen
{
    struct Screen 		Screen;

    /* Private fields */
    struct DrawInfo 		DInfo;
    struct TextAttr 		textattr;
#if !USE_LOCKLAYERINFO_AS_REFRESHLOCK
    struct SignalSemaphore	RefreshLock; /* to avoid refreshing trouble when apps
    						and Intuition try to do it at the same time */
#endif
    UWORD  			Pens[NUMDRIPENS];
    struct PubScreenNode 	* pubScrNode;
    Object 			* depthgadget;
    UWORD  			SpecialFlags;
    struct Layer                * rootLayer;
    ULONG                       DisplayBeepColor0[3];
    struct Window               * DisplayBeepWindow;
};

#define GetPrivScreen(s)	((struct IntScreen *)s)

/* SpecialFlags */
#define SF_IsParent	(0x0001)
#define SF_IsChild	(0x0002)


struct IntRequestUserData
{
    ULONG    			IDCMP;
    STRPTR 			* GadgetLabels;
    struct Gadget 		* Gadgets;
    UWORD 			NumGadgets;
};




/*extern struct IntuitionBase * IntuitionBase;*/

#define IW(x) ((struct IntWindow *)x)    

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

#if INTERNAL_BOOPSI

#define BOOPSIBase BOOPSIBase_accessed_although_may_not_use

#else

#ifdef BOOPSIBase
#undef BOOPSIBase
#endif
#define BOOPSIBase (GetPrivIBase(IntuitionBase)->BOOPSIBase)

#endif


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

#define IS_GZZ_GADGET(gad) (((gad)->GadgetType) & GTYP_GZZGADGET)  

#define IS_BORDER_GADGET(gad) \
	(IS_GZZ_GADGET(gad) \
	|| ((gad)->Activation & (GACT_RIGHTBORDER|GACT_LEFTBORDER|GACT_TOPBORDER|GACT_BOTTOMBORDER)))

#define IS_SYS_GADGET(gad) (((gad)->GadgetType) & GTYP_SYSTYPEMASK)

#define IS_BOOPSI_GADGET(gad) (((gad)->GadgetType & GTYP_GTYPEMASK) == GTYP_CUSTOMGADGET)

/*#define IS_BORDER_GADGET(gad) \
	(((gad->GadgetType) & GTYP_SYSGADGET) \
	|| ((gad)->Activation & (GACT_RIGHTBORDER|GACT_LEFTBORDER|GACT_TOPBORDER|GACT_BOTTOMBORDER))) */

#define IS_SCREEN_GADGET(gad) ((gad)->GadgetType & GTYP_SCRGADGET)

#define SET_GI_RPORT(gi, w, gad)	\
	(gi)->gi_RastPort = (IS_SCREEN_GADGET(gad) ? \
				((gi)->gi_Screen->BarLayer ? (gi)->gi_Screen->BarLayer->rp : NULL) : \
				(IS_BORDER_GADGET(gad) ?  (w)->BorderRPort : (w)->RPort) \
			    )


#define REFRESHGAD_BOOPSI 1		/* boopsi gadgets */
#define REFRESHGAD_BORDER 2		/* gadgets in window border */
#define REFRESHGAD_REL    4		/* gadgets with GFLG_RELRIGHT, GFLG_RELBOTTOM, GFLG_RELWIDTH, GFLG_RELHEIGHT */
#define REFRESHGAD_RELS   8		/* GFLG_RELSPECIAL gadgets */

#define SYSGADGET_ACTIVE (iihdata->ActiveSysGadget != NULL)

VOID int_refreshglist(struct Gadget *gadgets, struct Window *window,
		      struct Requester *requester, LONG numGad, LONG mustbe, LONG mustnotbe,
		      struct IntuitionBase *IntuitionBase);

VOID int_refreshwindowframe(struct Window *window, BOOL onlytitle,
    	    	    	    struct IntuitionBase *IntuitionBase);

enum {
    DRAGBAR = 0,
    CLOSEGAD,
    DEPTHGAD,
    SIZEGAD,
    ZOOMGAD,
    NUM_SYSGADS
};

#define SYSGAD(w, idx) (((struct IntWindow *)(w))->sysgads[idx])
	      
struct IntWindow
{
    struct Window		window;
    
    /* This message is sent to the intuition input handler when
       a window should be closed. We allocate it
       in OpenWindow(), so that CloseWindow 
       since CloseWindow() may not fail.
    */
       
    struct IntuiActionMessage  	* closeMessage;
    Object 			* sysgads[NUM_SYSGADS];
    struct Image		* AmigaKey;
    struct Image 		* Checkmark;
    struct Window		* menulendwindow;
    struct HashNode 	    	hashnode;
    
    /* When the Zoom gadget is pressed the window will have the
       dimensions stored here. The old dimensions are backed up here
       again. */
    WORD 			ZipLeftEdge;
    WORD 			ZipTopEdge;
    WORD 			ZipWidth;
    WORD 			ZipHeight;
    
    /* max. number of mousemove events to send to this window */
    WORD 			mousequeue;
    
    /* act. number of mousemove events sent to this window */
    WORD 			num_mouseevents;
    
    /* max. number of repeated IDCMP_RAWKEY, IDCMP_VANILLAKEY and IDCMP_IDCMPUPDATE
       messages to send to this window */
    WORD 			repeatqueue;
    
    /* act. number of repeated IDCMP_RAWKEY, IDCMP_VANILLAKEY and IDCMP_IDCMPUPDATE
       messages sent to this window */
    WORD 			num_repeatevents;
    
    WORD 			sizeimage_width;
    WORD 			sizeimage_height;
    
    ULONG			helpflags;
    ULONG			helpgroup;
};

#define HELPF_ISHELPGROUP  1
#define HELPF_GADGETHELP   2

#define IS_NOCAREREFRESH(win) (((win)->Flags & WFLG_NOCAREREFRESH) ? TRUE  : FALSE)
#define IS_DOCAREREFRESH(win) (((win)->Flags & WFLG_NOCAREREFRESH) ? FALSE : TRUE )
#define IS_GZZWINDOW(win)     (((win)->Flags & WFLG_GIMMEZEROZERO) ? TRUE  : FALSE)

/* 

Another note: Maybe use a union here to save space.

*/

struct IntIntuiMessage
{
    struct ExtIntuiMessage  eimsg;
    
    /*
    ** The following field is needed, because in case of IDCMP_RAWKEY
    ** IntuiMessage->IAddress is a pointer to this data, not the data
    ** itself (which is the case for IDCMP_VANILLAKEY)
    */
    
    APTR    	    	    prevCodeQuals;
};

#define INT_INTUIMESSAGE(x) ((struct IntIntuiMessage *)(x))

struct IntuiActionMessage
{
    struct Message  		ExecMessage;
    struct Window		* Window;
    struct Task			* Task;
    UWORD           		Code;
    union
    {
        struct 
	{
	    struct Gadget	* Gadget;
	}			iam_activategadget;
	struct
	{
	    LONG		Left;
	    LONG		Top;
	    LONG		Width;
	    LONG		Height;
	}			iam_changewindowbox;
	struct
	{
	    WORD		dx;
	    WORD		dy;
	}   			iam_movewindow;
	struct
	{
	    struct Window	* BehindWindow;
	}			iam_movewindowinfrontof;
	struct
	{
	    struct Screen	* Screen;
	    BOOL		ShowIt;
	}			iam_showtitle;
	struct
	{
	    WORD		dx;
	    WORD		dy;
	}			iam_sizewindow;
	struct
	{
	    struct Screen	* Screen;
	    ULONG		Flags;
	}			iam_screendepth;
	struct
	{
	    int			yesno;
	} 			iam_showwindow;
	struct
	{
	    struct Region   	* shape;
	    struct Hook     	* callback;
	}   	    	    	iam_changewindowshape;
	
    } 				iam;    
};

#define iam_ActivateGadget 	iam.iam_activategadget
#define iam_ChangeWindowBox	iam.iam_changewindowbox
#define iam_MoveWindow		iam.iam_movewindow
#define iam_MoveWindowInFrontOf iam.iam_movewindowinfrontof
#define iam_ShowTitle		iam.iam_showtitle
#define	iam_SizeWindow		iam.iam_sizewindow
#define iam_ScreenDepth		iam.iam_screendepth
#define iam_ShowWindow		iam.iam_showwindow
#define iam_ChangeWindowShape	iam.iam_changewindowshape

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
    AMCODE_SCREENSHOWTITLE,
    AMCODE_SCREENDEPTH,
    AMCODE_SHOWWINDOW,
    AMCODE_CHANGEWINDOWSHAPE
};

/* Flag definitions for MoreFlags */

#define WMFLG_NOTIFYDEPTH        (1 << 0)   /* Window wants notification when
					       it's depth arranged */

#define WMFLG_DO_UNLOCKPUBSCREEN (1 << 1)
#define WMFLG_MENUHELP		 (1 << 2)
#define WMFLG_POINTERDELAY	 (1 << 3)
#define WMFLG_TABLETMESSAGES	 (1 << 4)
					      
/* Called by intuition to free a window */
extern VOID int_closewindow(struct IntuiActionMessage *msg, struct IntuitionBase *IntuitionBase);
extern VOID int_activatewindow(struct Window *window, struct IntuitionBase *IntuitionBase);
extern VOID int_screendepth(struct Screen *screen, ULONG flags, struct IntuitionBase *IntuitionBase);

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
			      struct BitMap * SuperBitMap, struct Hook *backfillhook,
			      struct Region * shape,
			      struct Hook * shapehook,
			      struct Layer * parent,
			      ULONG  visible);
extern void intui_SetWindowTitles (struct Window *, CONST_STRPTR, CONST_STRPTR);
extern void intui_RefreshWindowFrame(struct Window *win);
extern struct Window *intui_FindActiveWindow(struct InputEvent *ie, BOOL *swallow_event, struct IntuitionBase *IntuitionBase);
extern void intui_ScrollWindowRaster(struct Window * win, WORD dx, WORD dy, WORD xmin,
                           	     WORD ymin, WORD xmax, WORD ymax, struct IntuitionBase * IntuitionBase);

/* Miscellaneous prototypes */
extern void intrequest_freelabels(STRPTR *gadgetlabels, struct IntuitionBase *IntuitionBase);
extern void intrequest_freegadgets(struct Gadget *gadgets, struct IntuitionBase *IntuitionBase);

/* wbtasktalk protos */

void TellWBTaskToCloseWindows(struct IntuitionBase *IntuitionBase);
void TellWBTaskToOpenWindows(struct IntuitionBase *IntuitionBase);

/* intuition_misc protos */
extern void LoadDefaultPreferences(struct IntuitionBase * IntuitionBase);
extern void CheckRectFill(struct RastPort *rp, WORD x1, WORD y1, WORD x2, WORD y2, struct IntuitionBase * IntuitionBase); 
extern BOOL CreateWinSysGadgets(struct Window *w, struct IntuitionBase *IntuitionBase);
extern VOID KillWinSysGadgets(struct Window *w, struct IntuitionBase *IntuitionBase);
extern void CreateScreenBar(struct Screen *scr, struct IntuitionBase *IntuitionBase);
extern void KillScreenBar(struct Screen *scr, struct IntuitionBase *IntuitionBase);
extern void RenderScreenBar(struct Screen *scr, BOOL refresh, struct IntuitionBase *IntuitionBase);
extern struct IntuiActionMessage *AllocIntuiActionMsg(UWORD code, struct Window *win, struct IntuitionBase *IntuitionBase);
extern void FreeIntuiActionMsg(struct IntuiActionMessage *msg, struct IntuitionBase *IntuitionBase);
extern void SendIntuiActionMsg(struct IntuiActionMessage *msg, struct IntuitionBase *IntuitionBase);
extern BOOL AllocAndSendIntuiActionMsg(UWORD code, struct Window *win, struct IntuitionBase *IntuitionBase);

extern void UpdateMouseCoords(struct Window *win);
extern WORD SubtractRectFromRect(struct Rectangle *a, struct Rectangle *b, struct Rectangle *destrectarray);

extern LONG CalcResourceHash(APTR resource);
extern void AddResourceToList(APTR resource, UWORD resourcetype, struct IntuitionBase *IntuitionBase);
extern void RemoveResourceFromList(APTR resource, UWORD resourcetype, struct IntuitionBase *IntuitionBase);
extern BOOL ResourceExisting(APTR resource, UWORD resourcetype, struct IntuitionBase *IntuitionBase);

/* Replacement for dos.library/DisplayError() */
AROS_UFP3(LONG, Intuition_DisplayError,
    AROS_UFPA(STRPTR, formatStr , A0),
    AROS_UFPA(ULONG , IDCMPFlags, D0),
    AROS_UFPA(APTR  , args      , A1));


#endif /* INTUITION_INTERN_H */
