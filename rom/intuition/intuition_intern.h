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

struct IntIntuitionBase
{
    struct IntuitionBase IBase;

    /* Put local shit here, invisible for the user */
    struct GfxBase	   * GfxBase;
    struct ExecBase	   * SysBase;
    struct UtilityBase	   * UtilBase;
    struct Library	   * BOOPSIBase;
    struct Library	   * KeymapBase;
    struct Library	   * TimerBase;
    struct MsgPort	   * TimerMP;
    struct timerequest	   * TimerIO;

    struct MsgPort	   * WorkBenchMP;
    struct Screen	   * WorkBench;
    struct SignalSemaphore * IBaseLock;
    
    struct IOStdReq	   * InputIO;
    struct MsgPort	   * InputMP;
    BOOL		     InputDeviceOpen;
    struct Interrupt	   * InputHandler;

    
    struct Hook		   *GlobalEditHook;
    /* The default global edit hook */
    struct Hook		   DefaultEditHook;
    
    APTR		     DriverData; /* Pointer which the driver may use */

/*
    struct MinList	     PublicScreenList;
    struct Screen	   * DefaultPublicScreen;
    struct SignalSemaphore * PubScreenListLock;
*/
    struct Library         * LayersBase;
    
    struct IClass *dragbarclass;
    struct IClass *tbbclass; /* Titlebar button class. (close, zoom, depth) */
};

struct IntScreen
{
    struct Screen Screen;

    /* Private fields */
    struct DrawInfo DInfo;
    UWORD  Pens[NUMDRIPENS];
    UWORD  SpecialFlags;
};

#define GetPrivScreen(s)	((struct IntScreen *)s)

/* SpecialFlags */
#define SF_IsParent	(0x0001)
#define SF_IsChild	(0x0002)


struct EasyRequestUserData
{
    ULONG    IDCMP;
    STRPTR * GadgetLabels;
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

#define PublicClassList ((struct List *)&(GetPrivIBase(IntuitionBase)->ClassList))

/* Window-Flags */
#define EWFLG_DELAYCLOSE	0x00000001L /* Delay CloseWindow() */
#define EWFLG_CLOSEWINDOW	0x00000002L /* Call CloseWindow() */

/* Needed for close() */
#define expunge() \
    AROS_LC0(BPTR, expunge, struct IntuitionBase *, IntuitionBase, 3, Intuition)

/* Driver prototypes */
extern int  intui_init (struct IntuitionBase *);
extern int  intui_open (struct IntuitionBase *);
extern void intui_close (struct IntuitionBase *);
extern void intui_expunge (struct IntuitionBase *);
extern int intui_GetWindowSize (void);
extern void intui_WindowLimits (struct Window * window,
	    WORD MinWidth, WORD MinHeight, UWORD MaxWidth, UWORD MaxHeight);
extern void intui_ActivateWindow (struct Window *);
extern void intui_BeginRefresh (struct Window * window,
	    struct IntuitionBase * IntuitionBase);
extern void intui_ChangeWindowBox (struct Window * window, WORD x, WORD y,
	    WORD width, WORD height);
extern void intui_CloseWindow (struct Window *, struct IntuitionBase *);
extern void intui_EndRefresh (struct Window * window,
	    BOOL complete,
	    struct IntuitionBase * IntuitionBase);
extern void intui_MoveWindow (struct Window * window, WORD dx, WORD dy);
extern int  intui_OpenWindow (struct Window *, struct IntuitionBase *);
extern void intui_WindowToFront (struct Window *, struct IntuitionBase *);
extern void intui_WindowToBack  (struct Window *, struct IntuitionBase *);
extern void intui_MoveWindowInFrontOf (struct Window *, struct Window *, struct IntuitionBase *);
extern void intui_SetWindowTitles (struct Window *, UBYTE *, UBYTE *);
extern void intui_SizeWindow (struct Window * win, long dx, long dy);
extern void intui_RefreshWindowFrame(struct Window *win);
extern struct Window *intui_FindActiveWindow(struct InputEvent *ie, struct IntuitionBase *IntuitionBase);
/* Miscellaneous prototypes */
void easyrequest_freelabels(STRPTR *gadgetlabels);
void easyrequest_freegadgets(struct Gadget *gadgets);

#endif /* INTUITION_INTERN_H */
