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
#include <exec/memory.h>
#include <dos/dos.h>
#include <utility/tagitem.h>
#include <clib/exec_protos.h>
#include <clib/intuition_protos.h>
#include <clib/graphics_protos.h>
#include <clib/aros_protos.h>
#include "intuition_intern.h"

static struct IntuitionBase * IntuiBase;

extern Display * sysDisplay;
extern long sysCMap[];
extern unsigned long sysPlaneMask;
extern Cursor sysCursor;

Display * GetSysDisplay (void);
int	  GetSysScreen (void);
extern void SetGC (struct RastPort * rp, GC gc);
extern GC GetGC (struct RastPort * rp);
extern void SetXWindow (struct RastPort * rp, int win);

extern int CalcKnobSize (struct Gadget * propGadget, long * knobleft,
			long * knobtop, long * knobwidth, long * knobheight);

static int MyErrorHandler (Display *, XErrorEvent *);
static int MySysErrorHandler (Display *);

#define DEBUG			0
#define DEBUG_OpenWindow	0
#define DEBUG_CloseWindow	0
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

    exit (10);
}

static int MySysErrorHandler (Display * display)
{
    perror ("X11-Error");
    fflush (stderr);

    exit (10);
}

int intui_init (struct IntuitionBase * IntuitionBase)
{
    int t;

    IntuitionBase->ActiveScreen->Width =
	DisplayWidth (GetSysDisplay (), GetSysScreen ());
    IntuitionBase->ActiveScreen->Height =
	DisplayHeight (GetSysDisplay (), GetSysScreen ());

    for (t=0; keytable[t].amiga != -1; t++)
	keytable[t].keycode = XKeysymToKeycode (sysDisplay,
		keytable[t].keysym);

    XSetErrorHandler (MyErrorHandler);
    XSetIOErrorHandler (MySysErrorHandler);

    IntuiBase = IntuitionBase;

    return True;
}

int intui_open (struct IntuitionBase * IntuitionBase)
{
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

void intui_SetWindowTitles (struct Window * win, char * text, char * screen)
{
    XSizeHints hints;
    struct IntWindow * w;

    w = (struct IntWindow *)win;

    hints.x	 = win->LeftEdge;
    hints.y	 = win->TopEdge;
    hints.width  = win->Width;
    hints.height = win->Height;
    hints.flags  = PPosition | PSize;

    if (screen == (char *)~0L)
	screen = "Workbench 3.1";
    else if (!screen)
	screen = "";

    if (text == (char *)~0LL)
	text = win->Title;
    else if (!text)
	text = "";

    XSetStandardProperties (sysDisplay, w->iw_XWindow, text, screen,
	    None, NULL, 0, &hints);
}

int intui_GetWindowSize (void)
{
    return sizeof (struct IntWindow);
}

int intui_OpenWindow (struct IntWindow * iw,
	struct IntuitionBase * IntuitionBase)
{
    XGCValues gcval;
    XSetWindowAttributes winattr;
    GC gc;

    winattr.event_mask = 0;

    if ((iw->iw_Window.Flags & WFLG_REFRESHBITS) == WFLG_SMART_REFRESH)
    {
	winattr.backing_store = Always;
    }
    else
    {
	winattr.backing_store = NotUseful;
	winattr.event_mask |= ExposureMask;
    }

    if ((iw->iw_Window.Flags & WFLG_RMBTRAP)
	|| iw->iw_Window.IDCMPFlags
	    & (IDCMP_MOUSEBUTTONS
		| IDCMP_GADGETDOWN
		| IDCMP_GADGETUP
		| IDCMP_MENUPICK
	    )
    )
	winattr.event_mask |= ButtonPressMask | ButtonReleaseMask;

    if (iw->iw_Window.IDCMPFlags & IDCMP_REFRESHWINDOW)
	winattr.event_mask |= ExposureMask;

    if (iw->iw_Window.IDCMPFlags & IDCMP_MOUSEMOVE)
	winattr.event_mask |= PointerMotionMask;

    if (iw->iw_Window.IDCMPFlags & (IDCMP_RAWKEY | IDCMP_VANILLAKEY))
	winattr.event_mask |= KeyPressMask | KeyReleaseMask;


    if (iw->iw_Window.IDCMPFlags & IDCMP_ACTIVEWINDOW)
	winattr.event_mask |= EnterWindowMask;

    if (iw->iw_Window.IDCMPFlags & IDCMP_INACTIVEWINDOW)
	winattr.event_mask |= LeaveWindowMask;

    if (iw->iw_Window.IDCMPFlags & (IDCMP_NEWSIZE | IDCMP_CHANGEWINDOW))
	winattr.event_mask |= StructureNotifyMask;

    /* TODO IDCMP_SIZEVERIFY IDCMP_DELTAMOVE */

    winattr.cursor = sysCursor;
    winattr.save_under = True;
    winattr.background_pixel = sysCMap[0];

    iw->iw_XWindow = XCreateWindow (GetSysDisplay ()
	, DefaultRootWindow (GetSysDisplay ())
	, iw->iw_Window.LeftEdge
	, iw->iw_Window.TopEdge
	, iw->iw_Window.Width
	, iw->iw_Window.Height
	, 5 /* BorderWidth */
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

    SetXWindow (iw->iw_Window.RPort, iw->iw_XWindow);

    /*TODO __SetSizeHints (w); */

    gcval.plane_mask = sysPlaneMask;
    gcval.graphics_exposures = True;

    gc = XCreateGC (sysDisplay
	, iw->iw_XWindow
	, GCPlaneMask
	    | GCGraphicsExposures
	, &gcval
    );

    if (!gc)
    {
	XDestroyWindow (sysDisplay, iw->iw_XWindow);
	return FALSE;
    }

    /* XSetGraphicsExposures (sysDisplay, gc, TRUE); */

    SetGC (iw->iw_Window.RPort, gc);

    iw->iw_Region = XCreateRegion ();

    if (!iw->iw_Region)
    {
	XDestroyWindow (sysDisplay, iw->iw_XWindow);
	XFreeGC (sysDisplay, gc);
	return FALSE;
    }

    /* XSelectInput (sysDisplay
	, iw->iw_XWindow
	, ExposureMask
	    | ButtonPressMask
	    | ButtonReleaseMask
	    | PointerMotionMask
	    | KeyPressMask
	    | KeyReleaseMask
	    | EnterWindowMask
	    | LeaveWindowMask
	    | StructureNotifyMask
    ); */

    /* XDefineCursor (sysDisplay, iw->iw_XWindow, sysCursor); */
    XMapRaised (sysDisplay, iw->iw_XWindow);

    XFlush (sysDisplay);
    /* XSync (sysDisplay, FALSE); */

    Diow(bug("Opening Window %08lx (X=%ld)\n", w, iw->iw_XWindow));

    return 1;
}

void intui_CloseWindow (struct IntWindow * iw,
	    struct IntuitionBase * IntuitionBase)
{
    Dicw(bug("Closing Window %08lx (X=%ld)\n", iw, iw->iw_XWindow));

    XDestroyWindow (sysDisplay, iw->iw_XWindow);

    XDestroyRegion (iw->iw_Region);

    XFreeGC (sysDisplay, GetGC(iw->iw_Window.RPort));

    XSync (sysDisplay, FALSE);
}

void intui_WindowToFront (struct IntWindow * window)
{
    XRaiseWindow (sysDisplay, window->iw_XWindow);
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
}

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
}

void intui_SizeWindow (struct Window * win, long dx, long dy)
{
    XResizeWindow (sysDisplay
	, ((struct IntWindow *)win)->iw_XWindow
	, win->Width + dx
	, win->Height + dy
    );
}

void intui_ActivateWindow (struct IntWindow * win)
{
    XSetInputFocus (sysDisplay, win->iw_XWindow, RevertToNone, XCurrentTime);
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

ende:	/*printf ("RawKeyConvert: In %02x %04x %04x Out : %d cs %02x '%c'\n",
	    ie->ie_Code, ie->ie_Qualifier, xk.state, t, (ubyte)*buf,
	    (ubyte)*buf);*/

    return (t);
}


#ifdef TODO
void RefreshWindowFrame (w)
WIN * w;
{
    __SetSizeHints (w);
}

int IntuiTextLength (itext)
struct IntuiText * itext;
{
    return (100);
}

void ClearMenuStrip (win)
WIN * win;
{
    return;
}

void BeginRefresh (win)
WIN * win;
{
   /* Region aufpropfen */
   XSetRegion (sysDisplay, win->RPort->gc, win->region);
}


void EndRefresh (win, free)
WIN * win;
BOOL free;
{
   Region region;
   XRectangle rect;

   /* Zuerst alte Region freigeben (Speicher sparen) */
   if (free)
   {
      XDestroyRegion (win->region);

      win->region = XCreateRegion ();
   }

   /* Dann loeschen wir das ClipRect wieder indem wir ein neues
      erzeugen, welches das ganze Fenster ueberdeckt. */
   region = XCreateRegion ();

   rect.x      = 0;
   rect.y      = 0;
   rect.width  = win->Width;
   rect.height = win->Height;

   XUnionRectWithRegion (&rect, region, region);

   /* und setzen */
   XSetRegion (sysDisplay, win->RPort->gc, region);
}

int AutoRequest (win, body, pos, neg, f_pos, f_neg, width, height)
WIN * win;
struct IntuiText * body, * pos, * neg;
ULONG f_pos, f_neg;
SHORT width, height;
{
    return (TRUE);
}

void GetScreenData (scr, size, type, screen)
struct Screen * scr, * screen;
long size, type;
{
    if (type != WBENCHSCREEN)
    {
	movmem (scr, screen, size);
    }
    else
    {
	WB.Width = DisplayWidth(sysDisplay, sys_screen);
	WB.Height = DisplayHeight (sysDisplay, sys_screen);
	movmem (scr, &WB, size);
    }
}

#endif

#define ADDREL(gad,flag,w,field) ((gad->Flags & (flag)) ? w->field : 0)
#define GetLeft(gad,w)           (ADDREL(gad,GFLG_RELRIGHT,w,Width)   + gad->LeftEdge)
#define GetTop(gad,w)            (ADDREL(gad,GFLG_RELBOTTOM,w,Height) + gad->TopEdge)
#define GetWidth(gad,w)          (ADDREL(gad,GFLG_RELWIDTH,w,Width)   + gad->Width)
#define GetHeight(gad,w)         (ADDREL(gad,GFLG_RELHEIGHT,w,Height) + gad->Height)

#define InsideGadget(w,gad,x,y)   \
	    ((x) >= GetLeft(gad,w) && (y) >= GetTop(gad,w) \
	    && (x) < GetLeft(gad,w) + GetWidth(gad,w) \
	    && (y) < GetTop(gad,w) + GetHeight(gad,w))


struct Gadget * FindGadget (struct Window * window, int x, int y)
{
    struct Gadget * gadget;
    int gx, gy;

    for (gadget=window->FirstGadget; gadget; gadget=gadget->NextGadget)
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

    return gadget;
} /* FindGadget */

/* Use local copy of IntuitionBase */
#define IntuitionBase IntuiBase

void intui_ProcessEvents (void)
{
    struct IntuiMessage * im;
    struct Window	* w;
    struct IntWindow	* iw;
    struct Gadget	* gadget;
    struct MsgPort	* intuiReplyPort;
    struct Screen	* screen;
    char * ptr;
    int    mpos_x, mpos_y;
    int    wait;
    XEvent event;

    intuiReplyPort = CreateMsgPort ();
    wait = 0;

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

	    /* Search window */
	    for (screen=IntuitionBase->FirstScreen; screen; screen=screen->NextScreen)
	    {
		for (w=screen->FirstWindow; w; w=w->NextWindow)
		{
		    if (((struct IntWindow *)w)->iw_XWindow == event.xany.window)
			break;
		}
	    }

	    if (w)
	    {
		Dipxe(bug("X=%d is asocciated with Window %08lx\n",
		    event.xany.window,
		    (ULONG)w));
	    }
	    else
		Dipxe(bug("X=%d is not asocciated with a Window\n",
		    event.xany.window));

	    if (!w)
		continue;

	    iw = (struct IntWindow *)w;

	    if (!im)
		im = AllocMem (sizeof (struct IntuiMessage), MEMF_CLEAR);

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

		if (count != 0)
		    break;

		RefreshGadgets (w->FirstGadget, w, NULL);

		im->Class = IDCMP_REFRESHWINDOW;
		ptr	  = "REFRESHWINDOW";
	    } break;

	    case ConfigureNotify:
		if (w->Width != event.xconfigure.width ||
			w->Height != event.xconfigure.height)
		{
		    w->Width  = event.xconfigure.width;
		    w->Height = event.xconfigure.height;

		    im->Class = NEWSIZE;
		    ptr       = "NEWSIZE";
		}

		break;

	    case ButtonPress: {
		XButtonEvent * xb = &event.xbutton;

		im->Class = MOUSEBUTTONS;
		im->Qualifier = StateToQualifier (xb->state);
		im->MouseX = xb->x;
		im->MouseY = xb->y;

		switch (xb->button)
		{
		case Button1:
		    im->Code = SELECTDOWN;

		    gadget = FindGadget (w, xb->x, xb->y);

		    if (gadget)
		    {
			if (gadget->Activation & GACT_IMMEDIATE)
			{
			    im->Class = GADGETDOWN;
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

			    break;

			case GTYP_PROPGADGET: {
			    long knobleft, knobtop, knobwidth, knobheight;
			    struct PropInfo * pi;

			    pi = (struct PropInfo *)gadget->SpecialInfo;

			    if (!pi)
				break;

			    knobleft   = GetLeft (gadget, w);
			    knobtop    = GetTop (gadget, w);
			    knobwidth  = GetWidth (gadget, w);
			    knobheight = GetHeight (gadget, w);

			    if (!CalcKnobSize (gadget
				, &knobleft, &knobtop, &knobwidth, &knobheight)
			    )
				break;

			    if (pi->Flags & FREEHORIZ)
			    {
				if (xb->x < knobleft)
				{
				    if (pi->HorizPot > pi->HPotRes)
					pi->HorizPot -= pi->HPotRes;
				    else
					pi->HorizPot = 0;
				}
				else if (xb->x >= knobleft + knobwidth)
				{
				    if (pi->HorizPot + pi->HPotRes < MAXPOT)
					pi->HorizPot += pi->HPotRes;
				    else
					pi->HorizPot = MAXPOT;
				}
			    }

			    if (pi->Flags & FREEVERT)
			    {
				if (xb->y < knobtop)
				{
				    if (pi->VertPot > pi->VPotRes)
					pi->VertPot -= pi->VPotRes;
				    else
					pi->VertPot = 0;
				}
				else if (xb->y >= knobtop + knobheight)
				{
				    if (pi->VertPot + pi->VPotRes < MAXPOT)
					pi->VertPot += pi->VPotRes;
				    else
					pi->VertPot = MAXPOT;
				}
			    }

			    if (xb->x >= knobleft
				&& xb->y >= knobtop
				&& xb->x < knobleft + knobwidth
				&& xb->y < knobtop + knobheight
			    )
				pi->Flags |= KNOBHIT;
			    else
				pi->Flags &= ~KNOBHIT;

			    gadget->Flags |= GFLG_SELECTED;

			    break; }
			}

			RefreshGList (gadget, w, NULL, 1);
		    }

		    break;

		case Button2:
		    im->Code = MIDDLEDOWN;
		    break;

		case Button3:
		    im->Code = MENUDOWN;
		    break;
		}

		if (im->Class == MOUSEBUTTONS)
		    ptr = "MOUSEBUTTONS";
	    } break;

	    case ButtonRelease: {
		XButtonEvent * xb = &event.xbutton;

		im->Class = MOUSEBUTTONS;
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
			    im->Class = GADGETUP;
			    im->IAddress = gadget;
			    ptr       = "GADGETDOWN";
			}

			switch (gadget->GadgetType & GTYP_GTYPEMASK)
			{
			case GTYP_BOOLGADGET:
			    if (!(gadget->Activation & GACT_TOGGLESELECT) )
				gadget->Flags &= ~GFLG_SELECTED;

			    if (selected)
				RefreshGList (gadget, w, NULL, 1);

			    break;

			case GTYP_PROPGADGET: {
			    struct PropInfo * pi;

			    pi = (struct PropInfo *)gadget->SpecialInfo;

			    if (pi)
				pi->Flags &= ~KNOBHIT;

			    gadget->Flags &= ~GFLG_SELECTED;

			    RefreshGList (gadget, w, NULL, 1);

			    break; }

			}

			gadget = NULL;
		    }

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

		im->Class = RAWKEY;
		result = XKeyToAmigaCode(xk);
		im->Code = xk->keycode;
		im->Qualifier = result >> 16;

		ptr = NULL;
	    } break;

	    case KeyRelease: {
		XKeyEvent * xk = &event.xkey;
		ULONG result;

		im->Class = RAWKEY;
		result = XKeyToAmigaCode(xk);
		im->Code = xk->keycode | 0x8000;
		im->Qualifier = result >> 16;

		ptr = NULL;
	    } break;

	    case MotionNotify: {
		XMotionEvent * xm = &event.xmotion;

		im->Code = IECODE_NOBUTTON;
		im->Class = MOUSEMOVE;
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

		    case GTYP_PROPGADGET: {
			long knobleft, knobtop, knobwidth, knobheight;
			long dx, dy;
			struct PropInfo * pi;

			pi = (struct PropInfo *)gadget->SpecialInfo;

			/* Has propinfo and the mouse was over the
			    knob */
			if (pi && (pi->Flags & KNOBHIT))
			{
			    knobleft   = GetLeft (gadget, w);
			    knobtop    = GetTop (gadget, w);
			    knobwidth  = GetWidth (gadget, w);
			    knobheight = GetHeight (gadget, w);

			    if (!CalcKnobSize (gadget
				, &knobleft, &knobtop
				, &knobwidth, &knobheight)
			    )
				break;

			    /* Delta movement */
			    dx = xm->x - mpos_x;
			    dy = xm->y - mpos_y;

			    /* Move the knob the same amount, ie.
				knobleft += dx; knobtop += dy;

				knobleft = knobleft
				    + (pi->CWidth - knobwidth)
				    * pi->HorizPot / MAXPOT;

				ie. dx = (pi->CWidth - knobwidth)
				    * pi->HorizPot / MAXPOT;

				or

				pi->HorizPot = (dx * MAXPOT) /
				    (pi->CWidth - knobwidth);
			    */
			    if (pi->Flags & FREEHORIZ
				&& pi->CWidth != knobwidth)
			    {
				dx = (dx * MAXPOT) /
					(pi->CWidth - knobwidth);

				if (dx < 0)
				{
				    dx = -dx;

				    if (dx > pi->HorizPot)
					pi->HorizPot = 0;
				    else
					pi->HorizPot -= dx;
				}
				else
				{
				    if (dx + pi->HorizPot > MAXPOT)
					pi->HorizPot = MAXPOT;
				    else
					pi->HorizPot += dx;
				}
			    } /* FREEHORIZ */

			    if (pi->Flags & FREEVERT
				&& pi->CHeight != knobheight)
			    {
				dy = (dy * MAXPOT) /
					(pi->CHeight - knobheight);

				if (dy < 0)
				{
				    dy = -dy;

				    if (dy > pi->VertPot)
					pi->VertPot = 0;
				    else
					pi->VertPot -= dy;
				}
				else
				{
				    if (dy + pi->VertPot > MAXPOT)
					pi->VertPot = MAXPOT;
				    else
					pi->VertPot += dy;
				}
			    } /* FREEVERT */
			} /* Has PropInfo and Mouse is over knob */

			RefreshGList (gadget, w, NULL, 1);

			break; } /* PROPGADGET */

		    } /* switch GadgetType */
		} /* if (gadget) */

		ptr = "MOUSEMOVE";
	    } break; /* MotioNotify */

	    case EnterNotify: {
		XCrossingEvent * xc = &event.xcrossing;

		im->Class = ACTIVEWINDOW;
		im->MouseX = xc->x;
		im->MouseY = xc->y;

		ptr = "ACTIVEWINDOW";
	    } break;

	    case LeaveNotify: {
		XCrossingEvent * xc = &event.xcrossing;

		im->Class = ACTIVEWINDOW;
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

		    PutMsg (w->UserPort, (struct Message *)im);
		    im = NULL;
		    wait ++;
		}
		else
		    im->Class = 0;
	    }
	}

	if (im)
	    FreeMem (im, sizeof (struct IntuiMessage));

	Wait (1L << intuiReplyPort->mp_SigBit | SIGBREAKF_CTRL_F);

	/* Empty port */
	while ((im = (struct IntuiMessage *)GetMsg (intuiReplyPort)))
	{
	    FreeMem (im, sizeof (struct IntuiMessage));
	    wait --;
	}
    }
}


