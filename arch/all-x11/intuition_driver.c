#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>
#include <X11/keysym.h>

#undef CurrentTime /* Defined by X.h */
#define XCurrentTime 0L

#define DEBUG_FreeMem 1

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

static struct IntuitionBase * IntuiBase;

extern Display * sysDisplay;
extern long sysCMap[];
extern unsigned long sysPlaneMask;
extern Cursor sysCursor;

extern struct Task * inputDevice;
#define SIGID()      Signal (inputDevice, SIGBREAKF_CTRL_F)

Display * GetSysDisplay (void);
int	  GetSysScreen (void);
extern void SetGC (struct RastPort * rp, GC gc);
extern GC GetGC (struct RastPort * rp);
extern void SetXWindow (struct RastPort * rp, int win);

static int MyErrorHandler (Display *, XErrorEvent *);
static int MySysErrorHandler (Display *);

#define DEBUG	0
#define DEBUG_ProcessXEvents	0

#if DEBUG
#   define D(x)     x
#else
#   define D(x)     /* eps */
#endif

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

#define bug	    kprintf

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


static int MyErrorHandler (Display * display, XErrorEvent * errevent)
{
    char buffer[256];

    XGetErrorText (display, errevent->error_code, buffer, sizeof (buffer));

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

    XSetStandardProperties (sysDisplay, w->iw_XWindow, text, screen,
	    None, NULL, 0, &hints);

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

    if (!GetGC (IW(w)->iw_Window.RPort))
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

    SetXWindow (IW(w)->iw_Window.RPort, IW(w)->iw_XWindow);

    IW(w)->iw_Region = XCreateRegion ();

    if (!IW(w)->iw_Region)
    {
	XDestroyWindow (sysDisplay, IW(w)->iw_XWindow);
	return FALSE;
    }

    XMapRaised (sysDisplay, IW(w)->iw_XWindow);

    /* Show window *now* */
    /* XFlush (sysDisplay); */
    SIGID ();

    Diow(bug("Opening Window %p (X=%ld)\n", iw, IW(w)->iw_XWindow));

    return 1;
}

void intui_CloseWindow (struct Window * w,
	    struct IntuitionBase * IntuitionBase)
{
    Dicw(bug("Closing Window %p (X=%ld)\n", w, IW(w)->iw_XWindow));

    XDestroyWindow (sysDisplay, IW(w)->iw_XWindow);

    XDestroyRegion (IW(w)->iw_Region);

    XSync (sysDisplay, FALSE);

    SIGID ();
}

void intui_WindowToFront (struct Window * window)
{
    XRaiseWindow (sysDisplay, IW(window)->iw_XWindow);

    SIGID ();
}

void intui_WindowToBack (struct Window * window)
{
    XLowerWindow (sysDisplay, IW(window)->iw_XWindow);

    SIGID ();
}

void intui_MoveWindow (struct Window * window, WORD dx, WORD dy)
{
    XMoveWindow (sysDisplay, IW(window)->iw_XWindow, dx, dy);

    SIGID ();
}

void intui_ChangeWindowBox (struct Window * window, WORD x, WORD y,
    WORD width, WORD height)
{
    XMoveResizeWindow (sysDisplay, IW(window)->iw_XWindow, x, y, width, height);

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

    xk->state = 0;
    count = XLookupString (xk, buffer, 10, &ks, NULL);

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
    XResizeWindow (sysDisplay
	, ((struct IntWindow *)win)->iw_XWindow
	, win->Width + dx
	, win->Height + dy
    );

    SIGID ();
}

void intui_ActivateWindow (struct Window * win)
{
    XSetInputFocus (sysDisplay, IW(win)->iw_XWindow, RevertToNone, XCurrentTime);

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

    t = XLookupString (&xk, buf, size, NULL, NULL);

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
    XSetRegion (sysDisplay, GetGC(IW(win)->iw_Window.RPort), IW(win)->iw_Region);
    SIGID ();
} /* intui_BeginRefresh */

void intui_EndRefresh (struct Window * win, BOOL free,
	struct IntuitionBase * IntuitionBase)
{
    Region region;
    XRectangle rect;

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
    XSetRegion (sysDisplay, GetGC(IW(win)->iw_Window.RPort), region);
    SIGID ();
} /* intui_EndRefresh */


#define ADDREL(gad,flag,w,field) ((gad->Flags & (flag)) ? w->field : 0)
#define GetLeft(gad,w)           (ADDREL(gad,GFLG_RELRIGHT,w,Width)   + gad->LeftEdge)
#define GetTop(gad,w)            (ADDREL(gad,GFLG_RELBOTTOM,w,Height) + gad->TopEdge)
#define GetWidth(gad,w)          (ADDREL(gad,GFLG_RELWIDTH,w,Width)   + gad->Width)
#define GetHeight(gad,w)         (ADDREL(gad,GFLG_RELHEIGHT,w,Height) + gad->Height)

#define InsideGadget(w,gad,x,y)   \
	    ((x) >= GetLeft(gad,w) && (y) >= GetTop(gad,w) \
	    && (x) < GetLeft(gad,w) + GetWidth(gad,w) \
	    && (y) < GetTop(gad,w) + GetHeight(gad,w))


struct Gadget * FindGadget (struct Window * window, int x, int y,
			struct GadgetInfo * gi)
{
    struct Gadget * gadget;
    struct gpHitTest gpht;
    int gx, gy;

    gpht.MethodID     = GM_HITTEST;
    gpht.gpht_GInfo   = gi;
    gpht.gpht_Mouse.X = x;
    gpht.gpht_Mouse.Y = y;

    for (gadget=window->FirstGadget; gadget; gadget=gadget->NextGadget)
    {
	if ((gadget->GadgetType & GTYP_GTYPEMASK) != GTYP_CUSTOMGADGET)
	{
	    gx = x - GetLeft(gadget,window);
	    gy = y - GetTop(gadget,window);

	    if (gx >= 0
		&& gy >= 0
		&& gx < GetWidth(gadget,window)
		&& gy < GetHeight(gadget,window)
	    )
		break;
	}
	else
	{
	    if (DoMethodA ((Object *)gadget, (Msg)&gpht) == GMR_GADGETHIT)
		break;
	}
    }

    return gadget;
} /* FindGadget */

#undef IntuitionBase
#define IntuitionBase IntuiBase

void intui_ProcessEvents (void)
{
    struct IntuiMessage * im;
    struct Window	* w;
    struct IntWindow	* iw;
    struct Gadget	* gadget;
    struct MsgPort	* intuiReplyPort;
    struct Screen	* screen;
    struct GadgetInfo	* gi;
    char * ptr;
    int    mpos_x, mpos_y;
    int    wait;
    XEvent event;
    ULONG  lock;
    ULONG  waitmask;

    intuiReplyPort = CreateMsgPort ();
    wait = 0;

    gadget = NULL;

    gi = AllocMem (sizeof (struct GadgetInfo), MEMF_ANY|MEMF_CLEAR);

    waitmask = (1L << intuiReplyPort->mp_SigBit) | SIGBREAKF_CTRL_F;

    for (;;)
    {
	im = NULL;
	w = NULL;

	while (XPending (sysDisplay))
	{
	    XNextEvent (sysDisplay, &event);

	    Dipxe(bug("Got Event for X=%d\n", event.xany.window));

	    if (event.type == MappingNotify)
	    {
		XRefreshKeyboardMapping ((XMappingEvent*)&event);
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

	    UnlockIBase (lock);

	    if (!w)
		continue;

	    gi->gi_Screen	  = screen;
	    gi->gi_Window	  = w;
	    gi->gi_Domain	  = *((struct IBox *)&w->LeftEdge);
	    gi->gi_RastPort	  = w->RPort;
	    gi->gi_Pens.DetailPen = gi->gi_Screen->DetailPen;
	    gi->gi_Pens.BlockPen  = gi->gi_Screen->BlockPen;
	    gi->gi_DrInfo	  = &(((struct IntScreen *)screen)->DInfo);

	    iw = (struct IntWindow *)w;

	    if (!im)
	    {
		im = AllocMem (sizeof (struct IntuiMessage), MEMF_CLEAR);
	    }

	    im->Class	    = 0L;
	    im->IDCMPWindow = w;
	    im->MouseX	    = mpos_x;
	    im->MouseY	    = mpos_y;

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

		XUnionRectWithRegion (&rect, iw->iw_Region, iw->iw_Region);

		if (count == 0)
		{
		    RefreshGadgets (w->FirstGadget, w, NULL);

		    im->Class = IDCMP_REFRESHWINDOW;
		    ptr       = "REFRESHWINDOW";
		}
	    } break;

	    case ConfigureNotify:
		if (w->Width != event.xconfigure.width ||
			w->Height != event.xconfigure.height)
		{
		    w->Width  = event.xconfigure.width;
		    w->Height = event.xconfigure.height;

		    im->Class = IDCMP_NEWSIZE;

		    /* Send GM_LAYOUT to all GA_RelSpecial BOOPSI gadgets */
		    DoGMLayout(w->FirstGadget, w, NULL, -1, FALSE, IntuitionBase);

		    ptr       = "NEWSIZE";
		}

		break;

	    case ButtonPress: {
		XButtonEvent * xb = &event.xbutton;

		im->Class = IDCMP_MOUSEBUTTONS;
		im->Qualifier = StateToQualifier (xb->state);
		im->MouseX = xb->x;
		im->MouseY = xb->y;
		D(bug("ipe: GADGETDOWN\n"));
		switch (xb->button)
		{
		case Button1: {
		    struct Gadget *newgad;
		    
		    im->Code = SELECTDOWN;

		    newgad = FindGadget (w, xb->x, xb->y, gi);
		    
		    /* If the active gadget was a stringgadget, and we clicked
		    ** outside it, then deactivate it. Make sure that there
		    ** was an active gadget to begin with.
		    */
		    if (gadget && gadget != newgad)
		    {
		    	if (gadget->GadgetType & GTYP_STRGADGET)
		    	{
		    	    gadget->Flags &= ~GFLG_SELECTED;
		    	    
		    	    RefreshStrGadget(gadget, w, IntuitionBase);
		    	}
		    }
		    gadget = newgad;

		    if (gadget)
		    {
			if (gadget->Activation & GACT_IMMEDIATE)
			{
			    im->Class = IDCMP_GADGETDOWN;
			    im->IAddress = gadget;
			    ptr       = "GADGETDOWN";
			}

			switch (gadget->GadgetType & GTYP_GTYPEMASK)
			{
			case GTYP_BOOLGADGET:
			    if (gadget->Activation & GACT_TOGGLESELECT)
				gadget->Flags ^= GFLG_SELECTED;
			    else
				gadget->Flags |= GFLG_SELECTED;

			    RefreshGList (gadget, w, NULL, 1);

			    break;

			case GTYP_PROPGADGET:
			    HandlePropSelectDown(gadget, w, NULL, xb->x, xb->y, IntuitionBase);
			    break;
			    
			case GTYP_STRGADGET:
			{
			    struct InputEvent ie;
			    UWORD imsgcode;
			    
			    ie.ie_Class = IECLASS_RAWMOUSE;
			    ie.ie_Code	= SELECTDOWN;
			    ie.ie_position.ie_xy.ie_x = xb->x;
			    ie.ie_position.ie_xy.ie_y = xb->y;
			    
			    HandleStrInput(gadget, gi, &ie, &imsgcode, IntuitionBase);
			    
			} break;
			    

			case GTYP_CUSTOMGADGET: {
			    struct gpInput gpi;
			    struct InputEvent ie; /* TODO */
			    IPTR retval;
			    ULONG termination;

			    ie.ie_Class = IECLASS_RAWMOUSE;
			    ie.ie_Code	= SELECTDOWN;
		    	    gettimeofday ((struct sys_timeval *)&ie.ie_TimeStamp, NULL);
		    	    
			    gpi.MethodID	= GM_GOACTIVE;
			    gpi.gpi_GInfo	= gi;
			    gpi.gpi_IEvent	= &ie;
			    gpi.gpi_Termination = &termination;
			    gpi.gpi_Mouse.X	= xb->x;
			    gpi.gpi_Mouse.Y	= xb->y;
			    gpi.gpi_TabletData	= NULL;

			    retval = DoMethodA ((Object *)gadget, (Msg)&gpi);

			    D(bug("\tProcessing GADGETDOWN\n"));

			    if (retval & GMR_VERIFY)
			    {
				D(bug("GMR_VERIFY gotten\n"));
				im->Class = IDCMP_GADGETUP;
				im->IAddress = gadget;
				ptr	 = "GADGETUP";
				im->Code = termination & 0x0000FFFF;
			    }
			    else
			    {
				if (retval != GMR_MEACTIVE)
				{
				    im->Class = 0L;
				    gadget = NULL;
				}
			    }

			    break; }

			} /* GadgetType */
		    } /* Over some gadget ? */

		}break;

		case Button2:
		    im->Code = MIDDLEDOWN;
		    break;

		case Button3:
		    im->Code = MENUDOWN;
		    break;
		}

		if (im->Class == IDCMP_MOUSEBUTTONS)
		    ptr = "MOUSEBUTTONS";
	    } break;

	    case ButtonRelease: {
		XButtonEvent * xb = &event.xbutton;

		im->Class = IDCMP_MOUSEBUTTONS;
		im->Qualifier = StateToQualifier (xb->state);
		im->MouseX = xb->x;
		im->MouseY = xb->y;

		switch (xb->button)
		{
		case Button1:
		    im->Code = SELECTUP;

		    if (gadget)
		    {
			int inside = InsideGadget(w,gadget,xb->x,xb->y);
			int selected = (gadget->Flags & GFLG_SELECTED) != 0;

			if (inside && (gadget->Activation & GACT_RELVERIFY))
			{
			    im->Class	 = IDCMP_GADGETUP;
			    im->IAddress = gadget;
			    ptr 	 = "GADGETUP";
			}

			switch (gadget->GadgetType & GTYP_GTYPEMASK)
			{
			case GTYP_BOOLGADGET:
			    if (!(gadget->Activation & GACT_TOGGLESELECT) )
				gadget->Flags &= ~GFLG_SELECTED;

			    if (selected)
				RefreshGList (gadget, w, NULL, 1);
			    gadget = NULL;
			    break;

			case GTYP_PROPGADGET:
			    HandlePropSelectUp(gadget, w, NULL, IntuitionBase);
			    gadget = NULL;
			    break;

			case GTYP_CUSTOMGADGET: {
			    struct gpInput gpi;
			    struct InputEvent ie; /* TODO */
			    IPTR retval;
			    ULONG termination;

			    ie.ie_Class = IECLASS_RAWMOUSE;
			    ie.ie_Code	= SELECTUP;
			    gettimeofday ((struct sys_timeval *)&ie.ie_TimeStamp, NULL);
			    
			    gpi.MethodID	= GM_HANDLEINPUT;
			    gpi.gpi_GInfo	= gi;
			    gpi.gpi_IEvent	= &ie;
			    gpi.gpi_Termination = &termination;
			    gpi.gpi_Mouse.X	= xb->x;
			    gpi.gpi_Mouse.Y	= xb->y;
			    gpi.gpi_TabletData	= NULL;

			    retval = DoMethodA ((Object *)gadget, (Msg)&gpi);

			    if (retval & GMR_NOREUSE && !(retval & GMR_VERIFY))
				im->Class = 0L; /* Swallow event */

			    if (retval & GMR_VERIFY)
				im->Code = termination & 0x0000FFFF;
				
			    /* This is a temporary kludge. We should NOT do this
			    ** if GMR_MEACTIVE is returned. Example of
			    ** a BOOPSI gadget that stays active after 
			    ** SELECTUP is strgclass gagdets.
			    */
			    gadget = NULL;

			    break; }

			} /* switch GadgetType */

		    } /* if (gadget) */

		    break;

		case Button2:
		    im->Code = MIDDLEUP;
		    break;

		case Button3:
		    im->Code = MENUUP;
		    break;
		}

		ptr = "MOUSEBUTTONS";
	    } break;

	    case KeyPress: {
		XKeyEvent * xk = &event.xkey;
		ULONG result;

		im->Class = IDCMP_RAWKEY;
		result = XKeyToAmigaCode(xk);
		im->Code = xk->keycode;
		im->Qualifier = result >> 16;
		
		D(bug("ipe: RAWKEY Press: gadget=%p\n", gadget));
		
		if (gadget)
		{
		    /* Both GTYP_STRGADGET and GTYP_CUSTOMGADGET
		    ** requires a struct InputEvent.
		    */
		    
		    struct InputEvent ie;
		    ie.ie_Class 	= IECLASS_RAWKEY;
		    ie.ie_Code		= im->Code;
		    ie.ie_Qualifier	= im->Qualifier;
		    
		    switch (gadget->GadgetType & GTYP_GTYPEMASK)
		    {
		    case GTYP_STRGADGET: {
		    	UWORD imsgcode;
		    	ULONG ret = HandleStrInput(gadget, gi, &ie, &imsgcode, IntuitionBase);
		    	if (ret == SGA_END)
		    	{
		    	    if (gadget->Activation & GACT_RELVERIFY)
		    	    {
		    	    	im->Class = IDCMP_GADGETUP;
		    	    	im->Code  = imsgcode;
		    	    	im->IAddress = gadget;
		    	    	gadget = NULL;
		    	    }
		    	}
		    } break;
		    	
		    case GTYP_CUSTOMGADGET: {
		    	struct gpInput gpi;
			IPTR retval;
			ULONG termination;

			XButtonEvent * xb = &event.xbutton;		    	
			
			gettimeofday ((struct sys_timeval *)&ie.ie_TimeStamp, NULL);
			gpi.MethodID	    = GM_HANDLEINPUT;
			gpi.gpi_GInfo	    = gi;
			gpi.gpi_IEvent	    = &ie;
			gpi.gpi_Termination = &termination;
			/* These _should_ be set to suitable values */
			gpi.gpi_Mouse.X	    = xb->x;
			gpi.gpi_Mouse.Y	    = xb->y;
			gpi.gpi_TabletData  = NULL;
					
			retval = DoMethodA((Object *)gadget, (Msg)&gpi);
			if (!(retval & GMR_MEACTIVE))
			{
			    gadget = NULL;
			    if (retval & GMR_NOREUSE)
			    	im->Class = 0L;
			}
			 
		    
		    	} break;
		    }
		
		}
		

		ptr = NULL;
	    } break;

	    case KeyRelease: {
		XKeyEvent * xk = &event.xkey;
		ULONG result;

		im->Class = IDCMP_RAWKEY;
		result = XKeyToAmigaCode(xk);
		im->Code = xk->keycode | 0x8000;
		im->Qualifier = result >> 16;

		D(bug("ipe: RAWKEY Release: gadget=%p\n", gadget));
		
		ptr = NULL;
	    } break;

	    case MotionNotify: {
		XMotionEvent * xm = &event.xmotion;
		struct IntuiMessage *msg, *succ;

		/* Check if there is already a MOUSEMOVE in the msg queue
		    of the task */
		msg = (struct IntuiMessage *)w->UserPort->mp_MsgList.lh_Head;

		Forbid ();

		while ((succ = (struct IntuiMessage *)msg->ExecMessage.mn_Node.ln_Succ))
		{
		    if (msg->Class == IDCMP_MOUSEMOVE)
		    {
			/* TODO allow a number of such messages */
			break;
		    }

		    msg = succ;
		}

		Permit ();

		/* If there is, don't add another one */
		if (succ)
		    break;

		im->Code = IECODE_NOBUTTON;
		im->Class = IDCMP_MOUSEMOVE;
		im->Qualifier = StateToQualifier (xm->state);
		im->MouseX = xm->x;
		im->MouseY = xm->y;

		if (gadget)
		{
		    int inside = InsideGadget(w,gadget,xm->x,xm->y);
		    int selected = (gadget->Flags & GFLG_SELECTED) != 0;

		    switch (gadget->GadgetType & GTYP_GTYPEMASK)
		    {
		    case GTYP_BOOLGADGET:
			if (inside != selected)
			{
			    gadget->Flags ^= GFLG_SELECTED;

			    RefreshGList (gadget, w, NULL, 1);
			}

			break;

		    case GTYP_PROPGADGET:

			HandlePropMouseMove
			(
			    gadget,
			    w,
			    NULL,
			    /* Delta movement */
			    xm->x - mpos_x,
			    xm->y - mpos_y,
			    IntuitionBase
			);

			break;
			/* PROPGADGET */


		    case GTYP_CUSTOMGADGET: {
			struct gpInput gpi;
			struct InputEvent ie; /* TODO */
			IPTR retval;
			ULONG termination;

			ie.ie_Class = IECLASS_RAWMOUSE;
			ie.ie_Code  = IECODE_NOBUTTON;
			gettimeofday ((struct sys_timeval *)&ie.ie_TimeStamp, NULL);
			
			gpi.MethodID	    = GM_HANDLEINPUT;
			gpi.gpi_GInfo	    = gi;
			gpi.gpi_IEvent	    = &ie;
			gpi.gpi_Termination = &termination;
			gpi.gpi_Mouse.X     = xm->x;
			gpi.gpi_Mouse.Y     = xm->y;
			gpi.gpi_TabletData  = NULL;

			retval = DoMethodA ((Object *)gadget, (Msg)&gpi);

			if (retval == GMR_MEACTIVE)
			{
			    im->Class = 0L;
			}

			if (retval & GMR_NOREUSE)
			    im->Class = 0L; /* Swallow event */

			break; }

		    } /* switch GadgetType */
		} /* if (gadget) */

		ptr = "MOUSEMOVE";
	    } break; /* MotioNotify */

	    case EnterNotify: {
		XCrossingEvent * xc = &event.xcrossing;

		im->Class = IDCMP_ACTIVEWINDOW;
		im->MouseX = xc->x;
		im->MouseY = xc->y;

		ptr = "ACTIVEWINDOW";
	    } break;

	    case LeaveNotify: {
		XCrossingEvent * xc = &event.xcrossing;

		im->Class = IDCMP_ACTIVEWINDOW;
		im->MouseX = xc->x;
		im->MouseY = xc->y;

		ptr = "INACTIVEWINDOW";
	    } break;

	    default:
		ptr = NULL;
	    break;
	    } /* switch */

	    mpos_x = im->MouseX;
	    mpos_y = im->MouseY;

	    if (ptr)
		Dipxe(bug("Msg=%s\n", ptr));

	    if (im->Class)
	    {
		if ((im->Class & w->IDCMPFlags) && w->UserPort)
		{
		    im->ExecMessage.mn_ReplyPort = intuiReplyPort;

		    lock = LockIBase (0L);

		    w->MoreFlags &= ~EWFLG_DELAYCLOSE;

		    if (w->MoreFlags & EWFLG_CLOSEWINDOW)
			CloseWindow (w);
		    else
		    {
			gettimeofday ((struct sys_timeval *)&im->Seconds, NULL);
			PutMsg (w->UserPort, (struct Message *)im);
			im = NULL;
			wait ++;
		    }

		    UnlockIBase (lock);
		}
		else
		    im->Class = 0;
	    }
	} /* while */

	if (im)
	{
	    FreeMem (im, sizeof (struct IntuiMessage));
	    im = NULL;
	}

	Wait (waitmask);

	/* Empty port */
	while ((im = (struct IntuiMessage *)GetMsg (intuiReplyPort)))
	{
	    FreeMem (im, sizeof (struct IntuiMessage));
	    wait --;
	}
    }
}

