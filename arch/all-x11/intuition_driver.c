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
#include <dos/dos.h>
#include <utility/tagitem.h>
#include <intuition/cghooks.h>
#include <intuition/gadgetclass.h>
#include <intuition/intuition.h>
#include <intuition/screens.h>
#include <intuition/sghooks.h>
#include <devices/keymap.h>
#include <hidd/unixio.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/arossupport.h>
#include <proto/alib.h>
#include "intuition_intern.h"
#include "gadgets.h"
#include "propgadgets.h"
#include "boopsigadgets.h"
#include "strgadgets.h"
#undef GfxBase
#include "graphics_internal.h"
#define GfxBase _GfxBase

static struct IntuitionBase * IntuiBase;

extern Display * sysDisplay;
extern long sysCMap[];
extern unsigned long sysPlaneMask;
extern Cursor sysCursor;
#if defined (X11_LOCK)
extern struct SignalSemaphore * X11lock;
#endif

extern struct Task * inputDevice;
#define SIGID()      Signal (inputDevice, SIGBREAKF_CTRL_F)


static int MyErrorHandler (Display *, XErrorEvent *);
static int MySysErrorHandler (Display *);

#define DEBUG	0
#define DEBUG_ProcessXEvents 0

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

struct IntWindow
{
    struct Window iw_Window;
    int 	  iw_XWindow;
    Region	  iw_Region;
};

struct _keytable
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
    {0, -1, 0, },
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

    /* TODO this is a hack */
    IntuiBase = IntuitionBase;

    return True;
}

int intui_open (struct IntuitionBase * IntuitionBase)
{
    if (GetPrivIBase(IntuitionBase)->WorkBench)
    {
	GetPrivIBase(IntuitionBase)->WorkBench->Width =
	    DisplayWidth (GetSysDisplay (), GetSysScreen ());
	GetPrivIBase(IntuitionBase)->WorkBench->Height =
	    DisplayHeight (GetSysDisplay (), GetSysScreen ());
    }

    return True;
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
    struct IntWindow * w;

    w = (struct IntWindow *)win;

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
    return sizeof (struct IntWindow);
}

#define IW(w)   ((struct IntWindow *)w)

int intui_OpenWindow (struct Window * w,
	struct IntuitionBase * IntuitionBase)
{
    XSetWindowAttributes winattr;

    if (!GetGC (IW(w)->iw_Window.RPort, GfxBase))
	return FALSE;

    winattr.event_mask = 0;

    if ((IW(w)->iw_Window.Flags & WFLG_REFRESHBITS) == WFLG_SMART_REFRESH)
    {
	winattr.backing_store = Always;
    }
    else
    {
	winattr.backing_store = NotUseful;
	winattr.event_mask |= ExposureMask;
    }

    if ((IW(w)->iw_Window.Flags & WFLG_RMBTRAP)
	|| IW(w)->iw_Window.IDCMPFlags
	    & (IDCMP_MOUSEBUTTONS
		| IDCMP_GADGETDOWN
		| IDCMP_GADGETUP
		| IDCMP_MENUPICK
	    )
	|| IW(w)->iw_Window.FirstGadget
    )
	winattr.event_mask |= ButtonPressMask | ButtonReleaseMask;
	
    if (IW(w)->iw_Window.IDCMPFlags & IDCMP_REFRESHWINDOW)
	winattr.event_mask |= ExposureMask;

    if (IW(w)->iw_Window.IDCMPFlags & IDCMP_MOUSEMOVE
	|| IW(w)->iw_Window.FirstGadget
    )
	winattr.event_mask |= PointerMotionMask;

    if (IW(w)->iw_Window.IDCMPFlags & (IDCMP_RAWKEY | IDCMP_VANILLAKEY))
	winattr.event_mask |= KeyPressMask | KeyReleaseMask;

    if (IW(w)->iw_Window.IDCMPFlags & IDCMP_ACTIVEWINDOW)
	winattr.event_mask |= EnterWindowMask;

    if (IW(w)->iw_Window.IDCMPFlags & IDCMP_INACTIVEWINDOW)
	winattr.event_mask |= LeaveWindowMask;

    /* Always show me if the window has changed */
    winattr.event_mask |= StructureNotifyMask;

    /* TODO IDCMP_SIZEVERIFY IDCMP_DELTAMOVE */
    winattr.cursor = sysCursor;
    winattr.save_under = True;
    winattr.background_pixel = sysCMap[0];
LX11
    IW(w)->iw_XWindow = XCreateWindow (GetSysDisplay ()
	, DefaultRootWindow (GetSysDisplay ())
	, IW(w)->iw_Window.LeftEdge
	, IW(w)->iw_Window.TopEdge
	, IW(w)->iw_Window.Width
	, IW(w)->iw_Window.Height
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

    SetXWindow (IW(w)->iw_Window.RPort, IW(w)->iw_XWindow, GfxBase);

    IW(w)->iw_Region = XCreateRegion ();
UX11

    if (!IW(w)->iw_Region)
    {
LX11
	XDestroyWindow (sysDisplay, IW(w)->iw_XWindow);
UX11
	return FALSE;
    }
LX11
    XMapRaised (sysDisplay, IW(w)->iw_XWindow);
UX11
    /* Show window *now* */
    
    XFlush (sysDisplay);
    
    SIGID ();

    Diow(bug("Opening Window %p (X=%ld)\n", iw, IW(w)->iw_XWindow));

    return 1;
}

void intui_CloseWindow (struct Window * w,
	    struct IntuitionBase * IntuitionBase)
{
    Dicw(bug("Closing Window %p (X=%ld)\n", w, IW(w)->iw_XWindow));
LX11
    XDestroyWindow (sysDisplay, IW(w)->iw_XWindow);

    XDestroyRegion (IW(w)->iw_Region);

    XSync (sysDisplay, FALSE);
UX11
    SIGID ();
}

void intui_WindowToFront (struct Window * window)
{
LX11
    XRaiseWindow (sysDisplay, IW(window)->iw_XWindow);
UX11
    SIGID ();
}

void intui_WindowToBack (struct Window * window)
{
LX11
    XLowerWindow (sysDisplay, IW(window)->iw_XWindow);
UX11
    SIGID ();
}

void intui_MoveWindow (struct Window * window, WORD dx, WORD dy)
{
LX11
    XMoveWindow (sysDisplay, IW(window)->iw_XWindow, dx, dy);
UX11
    SIGID ();
}

void intui_ChangeWindowBox (struct Window * window, WORD x, WORD y,
    WORD width, WORD height)
{
LX11
    XMoveResizeWindow (sysDisplay, IW(window)->iw_XWindow, x, y, width, height);
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

    result = StateToQualifier (xk->state) << 16L;
LX11
    xk->state = 0;
    count = XLookupString (xk, buffer, 10, &ks, NULL);
UX11
    for (t=0; keytable[t].amiga != -1; t++)
    {
	if (ks == keytable[t].keycode)
	{
	    result |= (keytable[t].amiga_qual << 16) | keytable[t].amiga;
	    return (result);
	}
    }

    result |= xk->keycode & 0xffff;

    return (result);
} /* XKeyToAmigaCode */

void intui_SizeWindow (struct Window * win, long dx, long dy)
{
LX11
    XResizeWindow (sysDisplay
	, ((struct IntWindow *)win)->iw_XWindow
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
	, ((struct IntWindow *)win)->iw_XWindow
	, hints
    );
UX11
}

void intui_ActivateWindow (struct Window * win)
{
LX11
    XSetInputFocus (sysDisplay, IW(win)->iw_XWindow, RevertToNone, XCurrentTime);
UX11
    SIGID ();
}

LONG intui_RawKeyConvert (struct InputEvent * ie, STRPTR buf,
	LONG size, struct KeyMap * km)
{
    XKeyEvent xk;
    char * ptr;
    int t;

    ie->ie_Code &= 0x7fff;

    for (t=0; keytable[t].amiga != -1; t++)
    {
	if (ie->ie_Code == keytable[t].keycode)
	{
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
    if (!*buf && t == 1) t = 0;
    if (!t) *buf = 0;

ende:
/*
    printf ("RawKeyConvert: In %02x %04x %04x Out : %d cs %02x '%c'\n",
	    ie->ie_Code, ie->ie_Qualifier, xk.state, t, (ubyte)*buf,
	    (ubyte)*buf);
*/

    return (t);
} /* intui_RawKeyConvert */

void intui_BeginRefresh (struct Window * win,
	struct IntuitionBase * IntuitionBase)
{
    /* Restrict rendering to a region */
LX11
    XSetRegion (sysDisplay, GetGC(IW(win)->iw_Window.RPort, GfxBase), IW(win)->iw_Region);
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
	XDestroyRegion (IW(win)->iw_Region);
	IW(win)->iw_Region = XCreateRegion ();
    }

    /* Dann loeschen wir das ClipRect wieder indem wir ein neues
	erzeugen, welches das ganze Fenster ueberdeckt. */
    region = XCreateRegion ();

    rect.x	= 0;
    rect.y	= 0;
    rect.width	= IW(win)->iw_Window.Width;
    rect.height = IW(win)->iw_Window.Height;

    XUnionRectWithRegion (&rect, region, region);

    /* und setzen */
    XSetRegion (sysDisplay, GetGC(IW(win)->iw_Window.RPort, GfxBase), region);
UX11
    SIGID ();
} /* intui_EndRefresh */


/**********************
**  intui_WaitEvent  **
**********************/
VOID intui_WaitEvent(struct InputEvent          *ie,
		struct Window			**wstorage)
{
    static HIDD        unixio = NULL;
    XEvent	       event;
    struct Window    * w = NULL;
    struct Screen    * screen;
    struct IntWindow * iw;
    ULONG	       lock;
    int 	       ret;
    static const struct TagItem tags[] = {{ TAG_END, 0 }};

    Dipxe(bug("intui_WaitEvent(ie=%p, wstorage=%p)\n", ie, wstorage));

    ie->ie_Class = 0;

    if (!unixio)
    {
	unixio = NewObjectA (NULL, UNIXIOCLASS, (struct TagItem *)tags);

	Dipxe(bug("unixio=%ld\n",unixio));

	if (!unixio)
	{
	    ReturnVoid ("intui_WaitEvent (No unixio.hidd)");
	}
    }

    for ( ; !ie->ie_Class; )
    {
	Dipxe(bug("iWE: Waiting for input at %ld\n", ConnectionNumber (sysDisplay)));

	/* Wait for input to arrive */
	ret = DoMethod (unixio, HIDDM_WaitForIO,
		ConnectionNumber (sysDisplay), HIDDV_UnixIO_Read);

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
		    if (((struct IntWindow *)w)->iw_XWindow == event.xany.window)
			break;
		}

		if (w)
		    break;
	    }


	    if (w)
	    {
		/* Make sure that no one closes the window while we work on it */
		w->MoreFlags |= EWFLG_DELAYCLOSE;

		Dipxe(bug("X=%d is asocciated with Window %p\n",
		    event.xany.window,
		    w
		));
	    }
	    else
		Dipxe(bug("X=%d is not asocciated with a Window\n",
		    event.xany.window));

	    /* We unlock IBase now that we have assured that the window won't
	    ** close while we are working on it. (IBase should be locked as little
	    ** time as possible
	    */
	    UnlockIBase (lock);

	    /* If this wasn't an event for AROS, then get next event in the queue */
	    if (!w)
		continue;

	    iw = (struct IntWindow *)w;

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
		if (w->Width != event.xconfigure.width ||
			w->Height != event.xconfigure.height)
		{
		    w->Width  = event.xconfigure.width;
		    w->Height = event.xconfigure.height;

		    ie->ie_Class = IECLASS_SIZEWINDOW;
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
		ie->ie_Code = xk->keycode;
		ie->ie_Qualifier = result >> 16;
		break; }

	    case KeyRelease: {
		XKeyEvent * xk = &event.xkey;
		ULONG result;

		ie->ie_Class = IECLASS_RAWKEY;
		result = XKeyToAmigaCode(xk);
		ie->ie_Code = xk->keycode | 0x8000;
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

	    *wstorage = w;

	    /* Got an event ? */
	    if (ie->ie_Class)
		break;

	} /* while (there are events in the event queue) */
    } /* Wait for event  */

    ReturnVoid ("intui_WaitEvent");
} /* intui_WaitEvent() */


#undef IntuitionBase
