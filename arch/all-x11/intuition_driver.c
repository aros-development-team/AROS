/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>
#include <X11/keysym.h>

#undef CurrentTime /* Defined by X.h */
#define XCurrentTime 0L

#define DEBUG_FreeMem 1
#define AROS_ALMOST_COMPATIBLE 1

#define X11_LOCK
void LockX11();
void UnlockX11();

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#define timeval sys_timeval
#include <sys/time.h>
#undef timeval
#include <exec/memory.h>
#include <exec/alerts.h>
#include <dos/dos.h>
#include <utility/tagitem.h>
#include <graphics/rastport.h>
#include <intuition/intuition.h>
#include <intuition/screens.h>
#include <devices/keymap.h>
#include <devices/input.h>

#include <hidd/unixio.h>

#include <proto/exec.h>
#include <proto/intuition.h>

#include <proto/graphics.h>
#include <proto/arossupport.h>
#include <proto/alib.h>
#include "intuition_intern.h"
#undef GfxBase
#include "graphics_internal.h"
#define GfxBase _GfxBase

static struct IntuitionBase * IntuiBase;

static struct Library *OOPBase = NULL;

extern Display * sysDisplay;
extern long sysCMap[];
extern unsigned long sysPlaneMask;
extern Cursor sysCursor;
#if defined (X11_LOCK)
extern struct SignalSemaphore * X11lock;
#endif

struct Task * inputDevice;
#define SIGID()      Signal (inputDevice, SIGBREAKF_CTRL_F)

#define WIN(x) ((struct Window *)x)

static int MyErrorHandler (Display *, XErrorEvent *);
static int MySysErrorHandler (Display *);

#define DEBUG	0
#define DEBUG_ProcessXEvents 0
#define DEBUG_OpenWindow 0

#include <aros/debug.h>

#if DEBUG_ProcessXEvents
#   define Dipxe(x) x
#else
#   define Dipxe(x) /* eps */
#endif

#if DEBUG_OpenWindow
#   define Diow(x) x
#else
#   define Diow(x) /* eps */
#endif

#if DEBUG_CloseWindow
#   define Dicw(x) x
#else
#   define Dicw(x) /* eps */
#endif

/* #define bug	    kprintf */


/* The X11 event task properties */
#define XETASK_STACKSIZE	8192
#define XETASK_PRIORITY		10
#define XETASK_NAME		"X11 event task"
static struct Task *CreateX11EventTask(struct IntuitionBase *IntuitionBase);


struct X11IntWindow
{
    struct IntWindow	iw_Window;
    int 		iw_XWindow;
    Region		iw_Region;
};

static struct _keytable
{
    KeySym keysym;
    WORD   amiga;
    UWORD  amiga_qual;
    char * normal,
	 * shifted;
    ULONG  keycode;
}
keytable[] =
{
    {XK_Return, 	0x44, 0,		      "\012",   "\012", 0 },
    {XK_Right,		0x4e, 0,		      "\233C",  "\233 A", 0 },
    {XK_Up,		0x4c, 0,		      "\233A",  "\233T",0 },
    {XK_Left,		0x4f, 0,		      "\233D",  "\233 @",0 },
    {XK_Down,		0x4d, 0,		      "\233B",  "\233S",0 },
    {XK_Help,		0x5f, 0,		      "\233?~", "\233?~",0 },
    {XK_KP_Enter,	0x43, IEQUALIFIER_NUMERICPAD, "\015",   "\015",0 },
    {XK_KP_Separator,	0x3c, IEQUALIFIER_NUMERICPAD, ".",      ".",0 },
    {XK_KP_Subtract,	0x4a, IEQUALIFIER_NUMERICPAD, "-",      "-",0 },
    {XK_KP_Decimal,	0x3c, IEQUALIFIER_NUMERICPAD, ".",      ".",0 },
    {XK_KP_0,		0x0f, IEQUALIFIER_NUMERICPAD, "0",      "0",0 },
    {XK_KP_1,		0x1d, IEQUALIFIER_NUMERICPAD, "1",      "1",0 },
    {XK_KP_2,		0x1e, IEQUALIFIER_NUMERICPAD, "2",      "2",0 },
    {XK_KP_3,		0x1f, IEQUALIFIER_NUMERICPAD, "3",      "3",0 },
    {XK_KP_4,		0x2d, IEQUALIFIER_NUMERICPAD, "4",      "4",0 },
    {XK_KP_5,		0x2e, IEQUALIFIER_NUMERICPAD, "5",      "5",0 },
    {XK_KP_6,		0x2f, IEQUALIFIER_NUMERICPAD, "6",      "6",0 },
    {XK_KP_7,		0x3d, IEQUALIFIER_NUMERICPAD, "7",      "7",0 },
    {XK_KP_8,		0x3e, IEQUALIFIER_NUMERICPAD, "8",      "8",0 },
    {XK_KP_9,		0x3f, IEQUALIFIER_NUMERICPAD, "9",      "9",0 },
    
    {XK_F1,		0x50, 0,		      "\2330~", "\23310~",0 },
    {XK_F2,		0x51, 0,		      "\2331~", "\23311~",0 },
    {XK_F3,		0x52, 0,		      "\2332~", "\23312~",0 },
    {XK_F4,		0x53, 0,		      "\2333~", "\23313~",0 },
    {XK_F5,		0x54, 0,		      "\2334~", "\23314~",0 },
    {XK_F6,		0x55, 0,		      "\2335~", "\23315~",0 },
    {XK_F7,		0x56, 0,		      "\2336~", "\23316~",0 },
    {XK_F8,		0x57, 0,		      "\2337~", "\23317~",0 },
    {XK_F9,		0x58, 0,		      "\2338~", "\23318~",0 },
    {XK_F10,		0x59, 0,		      "\2339~", "\23319~",0 },

/*    
    {XK_A,		0x20, 0,},
    {XK_B,		0x35, 0,},
    {XK_C,		0x33, 0,},
    {XK_D,		0x22, 0,},
    {XK_E,		0x12, 0,},
    {XK_F,		0x23, 0,},
    {XK_G,		0x24, 0,},
    {XK_H,		0x25, 0,},
    {XK_I,		0x17, 0,},
    {XK_J,		0x26, 0,},
    {XK_K,		0x27, 0,},
    {XK_L,		0x28, 0,},
    {XK_M,		0x37, 0,},
    {XK_N,		0x36, 0,},
    {XK_O,		0x18, 0,},
    {XK_P,		0x19, 0,},
    {XK_Q,		0x10, 0,},
    {XK_R,		0x13, 0,},
    {XK_S,		0x21, 0,},
    {XK_T,		0x14, 0,},
    {XK_U,		0x16, 0,},
    {XK_V,		0x34, 0,},
    {XK_W,		0x11, 0,},
    {XK_X,		0x32, 0,},
    {XK_Y,		0x15, 0,},
    {XK_Z,		0x31, 0,},
    
    {XK_a,		0x20, 0,},
    {XK_b,		0x35, 0,},
    {XK_c,		0x33, 0,},
    {XK_d,		0x22, 0,},
    {XK_e,		0x12, 0,},
    {XK_f,		0x23, 0,},
    {XK_g,		0x24, 0,},
    {XK_h,		0x25, 0,},
    {XK_i,		0x17, 0,},
    {XK_j,		0x26, 0,},
    {XK_k,		0x27, 0,},
    {XK_l,		0x28, 0,},
    {XK_m,		0x37, 0,},
    {XK_n,		0x36, 0,},
    {XK_o,		0x18, 0,},
    {XK_p,		0x19, 0,},
    {XK_q,		0x10, 0,},
    {XK_r,		0x13, 0,},
    {XK_s,		0x21, 0,},
    {XK_t,		0x14, 0,},
    {XK_u,		0x16, 0,},
    {XK_v,		0x34, 0,},
    {XK_w,		0x11, 0,},
    {XK_x,		0x32, 0,},
    {XK_y,		0x15, 0,},
    {XK_z,		0x31, 0,},
*/    
    {0, -1, 0, NULL, NULL, 0},
};

#define SHIFT	(IEQUALIFIER_LSHIFT | IEQUALIFIER_RSHIFT)
#define LALT	IEQUALIFIER_LALT
#define RALT	IEQUALIFIER_RALT
#define CTRL	IEQUALIFIER_CONTROL
#define CAPS	IEQUALIFIER_CAPSLOCK

#if defined (X11_LOCK)
#define LX11 LockX11(GfxBase);
#define UX11 UnlockX11(GfxBase);
#else
#define LX11
#define UX11
#endif

static int MyErrorHandler (Display * display, XErrorEvent * errevent)
{
    char buffer[256];

LX11
    XGetErrorText (display, errevent->error_code, buffer, sizeof (buffer));
UX11
    fprintf (stderr
	, "XError %d (Major=%d, Minor=%d)\n%s\n"
	, errevent->error_code
	, errevent->request_code
	, errevent->minor_code
	, buffer
    );
    fflush (stderr);

    return 0;
}

static int MySysErrorHandler (Display * display)
{
    perror ("X11-Error");
    fflush (stderr);

    return 0;
}

/* Semaphore needed to lock creation of the X11 event task */
static struct SignalSemaphore x11EventTaskSema;

int intui_init (struct IntuitionBase * IntuitionBase)
{
    int t;
    

    if (!sysDisplay)
	return False;

    for (t=0; keytable[t].amiga != -1; t++)
	keytable[t].keycode = XKeysymToKeycode (sysDisplay,
		keytable[t].keysym);

    XSetErrorHandler (MyErrorHandler);
    XSetIOErrorHandler (MySysErrorHandler);

#warning FIXME: this is a hack
    IntuiBase = IntuitionBase;
    
    InitSemaphore(&x11EventTaskSema);

    return True;
}



int intui_open (struct IntuitionBase * IntuitionBase)
{
    BOOL success = FALSE;
    
    /* Hack */
    if (!OOPBase)
    {
    	OOPBase = OpenLibrary("oop.library", 0);
    	if (!OOPBase)
    	    return (FALSE);
    }	    
    
    if (GetPrivIBase(IntuitionBase)->WorkBench)
    {
	GetPrivIBase(IntuitionBase)->WorkBench->Width =
	    DisplayWidth (GetSysDisplay (), GetSysScreen ());
	GetPrivIBase(IntuitionBase)->WorkBench->Height =
	    DisplayHeight (GetSysDisplay (), GetSysScreen ());
    }
    
    /* To avoid a race condition problem where two X11 event tasks are created, we
       semaphore protect it
    */
    
    if (AttemptSemaphore(&x11EventTaskSema))
    {
    	inputDevice = FindTask("input.device");

        /* Create the X11 event task */
    	GetPrivIBase(IntuitionBase)->DriverData = (APTR)CreateX11EventTask(IntuitionBase);
    	if (GetPrivIBase(IntuitionBase)->DriverData)
    	{
    	    success = TRUE;

    	}
	else
	{
	     ReleaseSemaphore(&x11EventTaskSema);
	}

    } /* if (first time opened) */
    else
    {
        /* Already opened, no allocations necessary */
        success = TRUE;
    }


    return (success);
}

void intui_close (struct IntuitionBase * IntuitionBase)
{
    return;
}

void intui_expunge (struct IntuitionBase * IntuitionBase)
{
    return;
}

void intui_SetWindowTitles (struct Window * win, UBYTE * text, UBYTE * screen)
{
    XSizeHints hints;
    struct X11IntWindow * w;

    w = (struct X11IntWindow *)win;

    hints.x	 = win->LeftEdge;
    hints.y	 = win->TopEdge;
    hints.width  = win->Width;
    hints.height = win->Height;
    hints.flags  = PPosition | PSize;

    if (screen == (UBYTE *)~0L)
	screen = "Workbench 3.1";
    else if (!screen)
	screen = "";

    if (text == (UBYTE *)~0LL)
	text = win->Title;
    else if (!text)
	text = "";
LX11
    XSetStandardProperties (sysDisplay, w->iw_XWindow, text, screen,
	    None, NULL, 0, &hints);
UX11
    SIGID ();
}

int intui_GetWindowSize (void)
{
    return sizeof (struct X11IntWindow);
}

#define XIW(w)   ((struct X11IntWindow *)w)

int intui_OpenWindow (struct Window * w,
	struct IntuitionBase * IntuitionBase,
	struct BitMap        * SuperBitMap)
{
    XSetWindowAttributes winattr;
    
/* nlorentz: It is now driver's responsibility to create window
       rastport. See rom/intuition/openwindow.c for more info.
    */
    /*
      Reset the Window coordinates. This is necessary as
      all IntuiMessages will be created relative to these
      coordinates. But as XWindow = AmigaWindow this has
      to be set to zero.
    */
    w->LeftEdge = 0;
    w->TopEdge = 0;
    if (!(WIN(w)->RPort = CreateRastPort()))
    	return FALSE;

    if (!GetGC (WIN(w)->RPort, GfxBase))
    {
        FreeRastPort(WIN(w)->RPort);
	return FALSE;
    }

    winattr.event_mask = 0;

    if ((WIN(w)->Flags & WFLG_REFRESHBITS) == WFLG_SMART_REFRESH)
    {
	winattr.backing_store = Always;
    }
    else
    {
	winattr.backing_store = NotUseful;
	winattr.event_mask |= ExposureMask;
    }

    if ((WIN(w)->Flags & WFLG_RMBTRAP)
	|| WIN(w)->IDCMPFlags
	    & (IDCMP_MOUSEBUTTONS
		| IDCMP_GADGETDOWN
		| IDCMP_GADGETUP
		| IDCMP_MENUPICK
	    )
	|| WIN(w)->FirstGadget
    )
	winattr.event_mask |= ButtonPressMask | ButtonReleaseMask;
	
    if (WIN(w)->IDCMPFlags & IDCMP_REFRESHWINDOW)
	winattr.event_mask |= ExposureMask;

    if (WIN(w)->IDCMPFlags & IDCMP_MOUSEMOVE
	|| WIN(w)->FirstGadget
    )
	winattr.event_mask |= PointerMotionMask;

    if (WIN(w)->IDCMPFlags & (IDCMP_RAWKEY | IDCMP_VANILLAKEY))
	winattr.event_mask |= KeyPressMask | KeyReleaseMask;

    if (WIN(w)->IDCMPFlags & IDCMP_ACTIVEWINDOW)
	winattr.event_mask |= EnterWindowMask;

    if (WIN(w)->IDCMPFlags & IDCMP_INACTIVEWINDOW)
	winattr.event_mask |= LeaveWindowMask;

    /* Always show me if the window has changed */
    winattr.event_mask |= StructureNotifyMask;
    Diow(bug("Set newsize notify\n"));

#warning TODO: IDCMP_SIZEVERIFY IDCMP_DELTAMOVE
    winattr.cursor = sysCursor;
    winattr.save_under = True;
    winattr.background_pixel = sysCMap[0];
LX11
    XIW(w)->iw_XWindow = XCreateWindow (GetSysDisplay ()
	, DefaultRootWindow (GetSysDisplay ())
	, WIN(w)->LeftEdge
	, WIN(w)->TopEdge
	, WIN(w)->Width
	, WIN(w)->Height
	, 0 /* BorderWidth */
	, DefaultDepth (GetSysDisplay (), GetSysScreen ())
	, InputOutput
	, DefaultVisual (GetSysDisplay (), GetSysScreen ())
	, CWBackingStore
	    | CWCursor
	    | CWSaveUnder
	    | CWEventMask
	    | CWBackPixel
	, &winattr
    );

    SetXWindow (WIN(w)->RPort, XIW(w)->iw_XWindow, GfxBase);

    XIW(w)->iw_Region = XCreateRegion ();
UX11

    if (!XIW(w)->iw_Region)
    {
LX11
	XDestroyWindow (sysDisplay, XIW(w)->iw_XWindow);
UX11

/* nlorentz: It is now driver's responsibility to create window
       rastport. See rom/intuition/openwindow.c for more info.
    */
        FreeRastPort(WIN(w)->RPort);
	return FALSE;
    }
LX11
    XMapRaised (sysDisplay, XIW(w)->iw_XWindow);
UX11
    /* Show window *now* */
    
    XFlush (sysDisplay);
    
    SIGID ();

    Diow(bug("Opening Window %p (X=%ld)\n", w, XIW(w)->iw_XWindow));

    return 1;
}

void intui_CloseWindow (struct Window * w,
	    struct IntuitionBase * IntuitionBase)
{
    Dicw(bug("Closing Window %p (X=%ld)\n", w, XIW(w)->iw_XWindow));
LX11
    XDestroyWindow (sysDisplay, XIW(w)->iw_XWindow);

    XDestroyRegion (XIW(w)->iw_Region);

    XSync (sysDisplay, FALSE);
UX11
/* nlorentz: It is now driver's responsibility to create window
       rastport. See rom/intuition/openwindow.c for more info.
    */
    
   FreeRastPort(WIN(w)->RPort);
    SIGID ();
}

void intui_WindowToFront (struct Window * window,
                          struct IntuitionBase * IntuitionBase)
{
LX11
    XRaiseWindow (sysDisplay, XIW(window)->iw_XWindow);
UX11
    SIGID ();
}

void intui_WindowToBack (struct Window * window,
                         struct IntuitionBase * IntuitionBase)
{
LX11
    XLowerWindow (sysDisplay, XIW(window)->iw_XWindow);
UX11
    SIGID ();
}

void intui_MoveWindow (struct Window * window, WORD dx, WORD dy)
{
LX11
    XMoveWindow (sysDisplay, XIW(window)->iw_XWindow, dx, dy);
UX11
    SIGID ();
}

BOOL intui_ChangeWindowBox (struct Window * window, WORD x, WORD y,
    WORD width, WORD height)
{
LX11
    XMoveResizeWindow (sysDisplay, XIW(window)->iw_XWindow, x, y, width, height);
UX11
    SIGID ();
}

long StateToQualifier (unsigned long state)
{
    long result;

    result = 0;

    if (state & ShiftMask)
	result |= SHIFT;

    if (state & ControlMask)
	result |= CTRL;

    if (state & LockMask)
	result |= CAPS;

    if (state & Mod2Mask) /* Right Alt */
	result |= LALT;

    if (state & 0x2000) /* Mode switch */
	result |= RALT;

    if (state & Mod1Mask) /* Left Alt */
	result |= AMIGAKEYS;

    if (state & Button1Mask)
	result |= IEQUALIFIER_LEFTBUTTON;

    if (state & Button2Mask)
	result |= IEQUALIFIER_RBUTTON;

    if (state & Button3Mask)
	result |= IEQUALIFIER_MIDBUTTON;

    return (result);
} /* StateToQualifier */

long XKeyToAmigaCode (XKeyEvent * xk)
{
    char buffer[10];
    KeySym ks;
    int count;
    long result;
    short t;
    
    D(bug("XKeyToAmigaCode()\n"));

    result = StateToQualifier (xk->state) << 16L;
LX11
    xk->state = 0;
    count = XLookupString (xk, buffer, 10, &ks, NULL);
UX11
    D(bug("xktac: Event was decoded into %d chars\n", count));
    for (t=0; keytable[t].amiga != -1; t++)
    {
	if (ks == keytable[t].keysym)
	{
	    D(bug("xktac: found in table\n"));
	    result |= (keytable[t].amiga_qual << 16) | keytable[t].amiga;
	    ReturnInt ("XKeyToAmigaCode", long, result);
	}
    }
    
    D(bug("xktac: Passing X keycode\n", xk->keycode & 0xffff));

    result |= xk->keycode & 0xffff;

    ReturnInt ("XKeyToAmigaCode", long, result);
} /* XKeyToAmigaCode */

void intui_SizeWindow (struct Window * win, long dx, long dy)
{
LX11
    XResizeWindow (sysDisplay
	, ((struct X11IntWindow *)win)->iw_XWindow
	, win->Width + dx
	, win->Height + dy
    );
UX11
    SIGID ();
}

void intui_WindowLimits (struct Window * win,
    WORD MinWidth, WORD MinHeight, UWORD MaxWidth, UWORD MaxHeight)
{
    XSizeHints * hints;
LX11
    hints = XAllocSizeHints();
    hints->flags += PMinSize|PMaxSize;
    hints->min_width = (int)MinWidth;
    hints->min_height = (int)MinHeight;
    hints->max_width = (int)MaxWidth;
    hints->max_height = (int)MaxHeight;

    XSetWMNormalHints (sysDisplay
	, ((struct X11IntWindow *)win)->iw_XWindow
	, hints
    );
UX11
}

void intui_ActivateWindow (struct Window * win)
{
LX11
    XSetInputFocus (sysDisplay, XIW(win)->iw_XWindow, RevertToNone, XCurrentTime);
UX11
    SIGID ();
}

struct Window *intui_FindActiveWindow(struct InputEvent *ie, BOOL *swallow_event, struct IntuitionBase * IntuitionBase)
{
    /* Just dummy implemntation for now */
    *swallow_event = FALSE;
    return NULL;
    
}

LONG intui_RawKeyConvert (struct InputEvent * ie, STRPTR buf,
	LONG size, struct KeyMap * km)
{
    XKeyEvent xk;
    char * ptr;
    int t;
    
    D(bug("intui_RawKeyConvert(ie=%p, buf=%p, size=%ld, km=%p)\n",
    	ie, buf, size, km));

    ie->ie_Code &= 0x7fff;

    for (t=0; keytable[t].amiga != -1; t++)
    {
	if (ie->ie_Code == keytable[t].keycode)
	{
	    D(bug("irkc: Found in table at entry %d\n", t));
	    if (ie->ie_Qualifier & SHIFT)
		ptr = keytable[t].shifted;
	    else
		ptr = keytable[t].normal;

	    t = strlen(ptr);
	    if (t > size)
		t = size;

	    strncpy (buf, ptr, t);

	    goto ende;
	}
    }
    
    D(bug("irkc: Converting X Keycode %d\n", ie->ie_Code));

    xk.keycode = ie->ie_Code;
    xk.display = sysDisplay;
    xk.state = 0;

    if (ie->ie_Qualifier & SHIFT)
	xk.state |= ShiftMask;

    if (ie->ie_Qualifier & CTRL)
	xk.state |= ControlMask;

    if (ie->ie_Qualifier & CAPS)
	xk.state |= LockMask;

    if (ie->ie_Qualifier & RALT)
	xk.state |= 0x2000;

    if (ie->ie_Qualifier & LALT)
	xk.state |= Mod2Mask;

    if (ie->ie_Qualifier & AMIGAKEYS)
	xk.state |= Mod1Mask;

    if (ie->ie_Qualifier & IEQUALIFIER_LEFTBUTTON)
	xk.state |= Button1Mask;

    if (ie->ie_Qualifier & IEQUALIFIER_MIDBUTTON)
	xk.state |= Button2Mask;

    if (ie->ie_Qualifier & IEQUALIFIER_RBUTTON)
	xk.state |= Button3Mask;
LX11
    t = XLookupString (&xk, buf, size, NULL, NULL);
UX11
    D(bug("Converted into %d keys\n", t));
    if (!*buf && t == 1) t = 0;
    if (!t) *buf = 0;
    


ende:
/*
    printf ("RawKeyConvert: In %02x %04x %04x Out : %d cs %02x '%c'\n",
	    ie->ie_Code, ie->ie_Qualifier, xk.state, t, (ubyte)*buf,
	    (ubyte)*buf);
*/

    ReturnInt ("intui_RawKeyCnvert", LONG, t);
} /* intui_RawKeyConvert */

void intui_BeginRefresh (struct Window * win,
	struct IntuitionBase * IntuitionBase)
{
    /* Restrict rendering to a region */
LX11
    XSetRegion (sysDisplay, GetGC(WIN(win)->RPort, GfxBase), XIW(win)->iw_Region);
UX11
    SIGID ();
} /* intui_BeginRefresh */

void intui_EndRefresh (struct Window * win, BOOL free,
	struct IntuitionBase * IntuitionBase)
{
    Region region;
    XRectangle rect;

LX11
    /* Zuerst alte Region freigeben (Speicher sparen) */
    if (free)
    {
	XDestroyRegion (XIW(win)->iw_Region);
	XIW(win)->iw_Region = XCreateRegion ();
    }

    /* Dann loeschen wir das ClipRect wieder indem wir ein neues
	erzeugen, welches das ganze Fenster ueberdeckt. */
    region = XCreateRegion ();

    rect.x	= 0;
    rect.y	= 0;
    rect.width	= WIN(win)->Width;
    rect.height = WIN(win)->Height;

    XUnionRectWithRegion (&rect, region, region);

    /* und setzen */
    XSetRegion (sysDisplay, GetGC(WIN(win)->RPort, GfxBase), region);
UX11
    SIGID ();
} /* intui_EndRefresh */

void intui_RefreshWindowFrame(struct Window *w)
{
    /* No reason to do anything; the windowmanager takes care of this */
    return;
}

/********************
**  XETaskEntry()  **
********************/
VOID XETaskEntry(struct IntuitionBase *IntuitionBase)
{
    XEvent	       event;
    struct Window    * w = NULL;
    struct Screen    * screen;
    struct X11IntWindow * iw;
    struct MsgPort   * inputmp;
    struct IOStdReq  * inputio;
    HIDD unixio;
    
    ULONG	       lock;
    int 	       ret;
    struct InputEvent stack_ie, *ie = &stack_ie;
  
    inputmp = CreateMsgPort();
    if (!inputmp)
    	Alert(AT_DeadEnd | AG_NoMemory | AN_Unknown);
    
    inputio = (struct IOStdReq *)CreateIORequest(inputmp, sizeof (struct IOStdReq));
    if (!inputio)
    	Alert(AT_DeadEnd | AG_NoMemory | AN_Unknown);
    	
    if ( 0 != OpenDevice("input.device", -1, (struct IORequest *)inputio, 0))
    	Alert(AT_DeadEnd | AG_OpenDev | AN_Unknown);
    	
    unixio = (HIDD)New_UnixIO(OOPBase);
    if (!unixio)
    	Alert(AT_DeadEnd | AG_NoMemory | AN_Unknown);
    	
    
    inputio->io_Command = IND_WRITEEVENT;
    inputio->io_Data    = (APTR)ie;
    inputio->io_Length  = sizeof (struct InputEvent);
    
    ie->ie_NextEvent = NULL;

    
    for (;;)
    {
    
	ie->ie_Class = 0;

    	for (;!ie->ie_Class; )
    	{
    	
	    Dipxe(bug("iWE: Waiting for input at %ld\n", ConnectionNumber (sysDisplay)));

	    /* Wait for input to arrive */
	    ret = (int)Hidd_UnixIO_Wait(  unixio
	    				, ConnectionNumber (sysDisplay)
					, vHidd_UnixIO_Read
					, NULL
					, NULL);
			

	    Dipxe(bug("iWE: Got input %ld\n", ret));

	    if (ret != 0)
	    	continue;

	    while (XPending (sysDisplay))
	    {
LX11
	    	XNextEvent (sysDisplay, &event);
UX11
	        Dipxe(bug("Got Event for X=%d\n", event.xany.window));

	        if (event.type == MappingNotify)
	    	{
LX11
		    XRefreshKeyboardMapping ((XMappingEvent*)&event);
UX11
		    continue;
	    	}

	        lock = LockIBase (0L);


	    	/* Search window */
	    	for (screen=IntuitionBase->FirstScreen; screen; screen=screen->NextScreen)
	    	{
		    for (w=screen->FirstWindow; w; w=w->NextWindow)
		    {
		    	if (((struct X11IntWindow *)w)->iw_XWindow == event.xany.window)
			    break;
		    }

		    if (w)
		    	break;
	    	}
	    	
	    	/* Update active window (We are allready inside LockIBase()/UnlockIBase(). */
	    	IntuitionBase->ActiveWindow = w;


	    	if (w)
	    	{
		    /* Make sure that no one closes the window while we work on it */
#warning Handle race condition problem here
/*		    w->MoreFlags |= EWFLG_DELAYCLOSE;
*/
		    Dipxe(bug("X=%d is asocciated with Window %p\n",
		    event.xany.window, w ));
	    	}
	    	else
		    Dipxe(bug("X=%d is not asocciated with a Window\n",
		    		event.xany.window));

	    	/* We unlock IBase now that we have assured that the window won't
	    	** close while we are working on it. (IBase should be locked as little
	    	** time as possible). The window will eventually be closed int
	    	** the intuition inputhandler.
	    	*/
	    	UnlockIBase (lock);

	    	/* If this wasn't an event for AROS, then get next event in the queue */
	    	if (!w)
		    continue;

	    	iw = (struct X11IntWindow *)w;
	    	
	    	ie->ie_Class = 0;

	    	switch (event.type)
	    	{
	    	case GraphicsExpose:
	    	case Expose: {
		    XRectangle rect;
		    UWORD	   count;

		    if (event.type == Expose)
		    {
		    	rect.x	= event.xexpose.x;
		    	rect.y	= event.xexpose.y;
		    	rect.width	= event.xexpose.width;
		    	rect.height = event.xexpose.height;
		    	count	= event.xexpose.count;
		    }
		    else
		    {
		    	rect.x	= event.xgraphicsexpose.x;
		    	rect.y	= event.xgraphicsexpose.y;
		    	rect.width	= event.xgraphicsexpose.width;
		    	rect.height = event.xgraphicsexpose.height;
		    	count	= event.xgraphicsexpose.count;
		    }
LX11
		    XUnionRectWithRegion (&rect, iw->iw_Region, iw->iw_Region);
UX11
		    if (count == 0)
		    {
		    	ie->ie_Class = IECLASS_REFRESHWINDOW;
		    }
		    break; }

	        case ConfigureNotify:
		    Dipxe(bug("Newsize\n"));
		    if (w->Width != event.xconfigure.width ||
			w->Height != event.xconfigure.height)
		    {
		    	w->Width  = event.xconfigure.width;
		    	w->Height = event.xconfigure.height;

		    	ie->ie_Class = IECLASS_SIZEWINDOW;

		    	Dipxe(bug("Really\n"));
		    }
		    
		    break;

	    	case ButtonPress: {
		    XButtonEvent * xb = &event.xbutton;

		    ie->ie_Class	 = IECLASS_RAWMOUSE;
		    ie->ie_Qualifier = StateToQualifier (xb->state);
		    ie->ie_X	 = xb->x;
		    ie->ie_Y	 = xb->y;

		    switch (xb->button)
		    {
		    case Button1:
		    	ie->ie_Code = SELECTDOWN;
		    	break;

		    case Button2:
		    	ie->ie_Code = MIDDLEDOWN;
		    	break;

		    case Button3:
		    	ie->ie_Code = MENUDOWN;
		    	break;
		    } /* switch (which button was clicked ?) */

		    break; }
		    
	        case ButtonRelease: {
		    XButtonEvent * xb = &event.xbutton;

		    ie->ie_Class = IECLASS_RAWMOUSE;
		    ie->ie_Qualifier = StateToQualifier (xb->state);

		    switch (xb->button)
		    {
		    case Button1:
		    	ie->ie_Code = SELECTUP;
		    	break;

		    case Button2:
		    	ie->ie_Code = MIDDLEUP;
		    	break;

		    case Button3:
		    	ie->ie_Code = MENUUP;
		    	break;
		    }
		    break; }

	    	case KeyPress: {
		    XKeyEvent * xk = &event.xkey;
		    ULONG result;

		    ie->ie_Class = IECLASS_RAWKEY;
		    result = XKeyToAmigaCode(xk);
		    ie->ie_Code = result & 0x0000FFFF;
		    ie->ie_Qualifier = result >> 16;
		    
		    Dipxe(bug("xe: keypress: code=%04x, qual=%04x\n",
		    	ie->ie_Code, ie->ie_Qualifier));
		    break; }

	    	case KeyRelease: {
		    XKeyEvent * xk = &event.xkey;
		    ULONG result;

		    ie->ie_Class = IECLASS_RAWKEY;
		    result = XKeyToAmigaCode(xk);
		    ie->ie_Code = (result & 0x0000FFFF) | 0x80;
		    ie->ie_Qualifier = result >> 16;
		    break; }

	    	case MotionNotify: {
		    XMotionEvent * xm = &event.xmotion;

		    ie->ie_Code = IECODE_NOBUTTON;
		    ie->ie_Class = IECLASS_RAWMOUSE;
		    ie->ie_Qualifier = StateToQualifier (xm->state);
		    ie->ie_X = xm->x;
		    ie->ie_Y = xm->y;
		    break; }

	   	case EnterNotify: {
		    XCrossingEvent * xc = &event.xcrossing;

		    ie->ie_Class	= IECLASS_ACTIVEWINDOW;
		    ie->ie_X	= xc->x;
		    ie->ie_Y	= xc->y;
		    break; }

	    	case LeaveNotify: {
		    XCrossingEvent * xc = &event.xcrossing;

		    ie->ie_Class = IECLASS_INACTIVEWINDOW;
		    ie->ie_X = xc->x;
		    ie->ie_Y = xc->y;
		    break; }

	    	} /* switch (X11 event type) */


	    	/* Got an event ? */
	    	if (ie->ie_Class)
		    break;

	    } /* while there are events in the event queue */
	    
    	} /* Until there is an event for AROS  */
    	
    	/* Send the event to input device for processing */
    	Dipxe(bug("xe: Sending event of class %d\n", ie->ie_Class));
    	DoIO((struct IORequest *)inputio);
    	Dipxe(bug("xe: Event sent\n"));
    	
    } /* Forever */

} /* intui_WaitEvent() */

/***************************
**  CreateX11EventTask()  **
***************************/
STATIC struct Task *CreateX11EventTask(struct IntuitionBase *IntuitionBase)
{
    struct Task *task;
    APTR stack;
    
    task = AllocMem(sizeof (struct Task), MEMF_PUBLIC|MEMF_CLEAR);
    if (task)
    {
    	NEWLIST(&task->tc_MemEntry);
    	task->tc_Node.ln_Type=NT_TASK;
    	task->tc_Node.ln_Name= XETASK_NAME;
    	task->tc_Node.ln_Pri = XETASK_PRIORITY;

    	stack=AllocMem(XETASK_STACKSIZE, MEMF_PUBLIC);
    	if(stack != NULL)
    	{
	    task->tc_SPLower=stack;
	    task->tc_SPUpper=(BYTE *)stack + XETASK_STACKSIZE;

#if AROS_STACK_GROWS_DOWNWARDS
	    task->tc_SPReg = (BYTE *)task->tc_SPUpper-SP_OFFSET-sizeof(APTR);
	    ((APTR *)task->tc_SPUpper)[-1] = IntuitionBase;
#else
	    task->tc_SPReg=(BYTE *)task->tc_SPLower-SP_OFFSET + sizeof(APTR);
	    *(APTR *)task->tc_SPLower = IntuitionBase;
#endif


	    if(AddTask(task, XETaskEntry, NULL) != NULL)
	    {
	    	/* Everything went OK */
	    	return (task);
	    }	
	    FreeMem(stack, XETASK_STACKSIZE);
    	}
        FreeMem(task,sizeof(struct Task));
    }
    return (NULL);

}

