/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.14  1996/10/01 15:48:33  digulla
    Use OpenWindowTags()
    Print error if a library couldn't be opened

    Revision 1.13  1996/09/21 15:49:08	digulla
    No need for stdlib.h

    Revision 1.12  1996/09/18 14:42:07	digulla
    Our window is simplerefresh

    Revision 1.11  1996/09/17 16:42:59	digulla
    Use general startup code

    Revision 1.10  1996/09/17 16:08:53	digulla
    Better formatting

    Revision 1.9  1996/09/11 16:50:25  digulla
    Use correct way to access "entry"

    Revision 1.8  1996/08/30 17:03:11  digulla
    Uses kprintf() now. Makes life a lot easier.

    Revision 1.7  1996/08/29 15:14:36  digulla
    Changed name
    Tests PrintIText(), too

    Revision 1.6  1996/08/28 17:58:05  digulla
    Show off all types of BOOLGADGETs and PROPGADGETs

    Revision 1.5  1996/08/23 17:05:41  digulla
    The demo crashes if kprintf() is called, so don't do it.
    New feature: Open console and use RawKeyConvert() to wait for ESC to quit the
		demo.
    New feature: Added two gadgets: One with GFLG_GADGHCOMP, the other with
		GFLG_GADGHIMAGE
    New feature: The user can select the gadgets and gets messages for them.
    New feature: More verbose and better error codes.

    Revision 1.4  1996/08/16 14:03:41  digulla
    More demos

    Revision 1.3  1996/08/15 13:17:32  digulla
    More types of IntuiMessages are checked
    Problem with empty window was due to unhandled REFRESH
    Commented some annoying debug output out

    Revision 1.2  1996/08/13 15:35:44  digulla
    Removed some comments
    Replied IntuiMessage

    Revision 1.1  1996/08/13 13:48:27  digulla
    Small Demo: Open a window, render some gfx and wait for a keypress

    Revision 1.5  1996/08/01 17:40:44  digulla
    Added standard header for all files

    Desc:
    Lang:
*/
#define ENABLE_RT	1
#define ENABLE_PURIFY	1

#include <exec/memory.h>
#include <dos/dos.h>
#include <dos/exall.h>
#include <dos/datetime.h>
#include <clib/exec_protos.h>
#include <clib/dos_protos.h>
#include <clib/aros_protos.h>
#include <clib/alib_protos.h>
#include <clib/utility_protos.h>
#include <clib/graphics_protos.h>
#include <clib/intuition_protos.h>
#include <clib/console_protos.h>
#include <intuition/intuitionbase.h>
#include <intuition/intuition.h>
#include <graphics/gfxbase.h>
#include <graphics/rastport.h>
#include <ctype.h>
#include <aros/rt.h>

#if 1
#   define D(x)    x
#else
#   define D(x)     /* eps */
#endif
#define bug	kprintf

struct Library *ConsoleDevice;
struct IntuitionBase *IntuitionBase;
struct GfxBase *GfxBase;

void Refresh (struct RastPort * rp)
{
    int len;

    SetAPen (rp, 1);
    SetDrMd (rp, JAM2);

    Move (rp, 0, 0);
    Draw (rp, 320, 150);

    Move (rp, 640, 0);
    Draw (rp, 0, 300);

    Move (rp, 300, 40);
    Text (rp, "Hello World.", 12);

    SetAPen (rp, 3);
    RectFill (rp, 90, 0, 120, 30);
    SetAPen (rp, 0);
    RectFill (rp, 100, 10, 110, 20);

    SetAPen (rp, 1);
    RectFill (rp, 150, 10, 160, 20);

    SetAPen (rp, 2);
    RectFill (rp, 200, 10, 210, 20);

    SetAPen (rp, 3);
    RectFill (rp, 250, 10, 260, 20);

    len = TextLength (rp, "Hello World.", 12);

    SetAPen (rp, 2);
    RectFill (rp, 299, 59, 300+len, 60+rp->Font->tf_YSize);

    SetAPen (rp, 1);
    Move (rp, 300, 60 + rp->Font->tf_Baseline);
    Text (rp, "Hello World.", 12);

    SetDrMd (rp, JAM1);
    SetAPen (rp, 1);
    Move (rp, 301, 101);
    Text (rp, "Hello World.", 12);
    SetAPen (rp, 2);
    Move (rp, 300, 100);
    Text (rp, "Hello World.", 12);

    Move (rp, 20, 350);
    Text (rp, "Press \"Complement\" to flip PropGadgets", 39);
}

#define GAD_WID     100
#define GAD_HEI     30
#define BORDER	    20

WORD BorderData[6*2*2] =
{
    0, GAD_HEI-1, /* Top (lighter) edge */
    1, -1,
    0, -(GAD_HEI-3),
    (GAD_WID-3), 0,
    1, -1,
    -(GAD_WID-1), 0,

    0, -(GAD_HEI-2), /* Bottom (darker) edge */
    -1, 1,
    0, (GAD_HEI-4),
    -(GAD_WID-4), 0,
    -1, 1,
    (GAD_WID-2), 0,
};
struct Border
DemoBottomBorder =
{
    GAD_WID-1, GAD_HEI-1,
    1, 2,
    JAM1,
    6,
    &BorderData[6*2],
    NULL
},
DemoTopBorder =
{
    0, 0,
    2, 1,
    JAM1,
    6,
    &BorderData[0],
    &DemoBottomBorder
};
struct Border DemoIBottomBorder =
{
    GAD_WID-1, GAD_HEI-1,
    2, 1,
    JAM1,
    6,
    &BorderData[6*2],
    NULL
},
DemoITopBorder =
{
    0, 0,
    1, 2,
    JAM1,
    6,
    &BorderData[0],
    &DemoIBottomBorder
};

struct PropInfo
DemoPropInfo1 =
{
    AUTOKNOB | FREEHORIZ | PROPNEWLOOK,
    0, 0,
    MAXBODY, MAXBODY,
    0,0,0,0,0,0
},
DemoPropInfo2 =
{
    AUTOKNOB | FREEVERT | PROPNEWLOOK,
    0, 0,
    MAXBODY, MAXBODY,
    0,0,0,0,0,0
},
DemoPropInfo3 =
{
    AUTOKNOB | FREEHORIZ | FREEVERT | PROPNEWLOOK,
    0, 0,
    MAXBODY, MAXBODY,
    0,0,0,0,0,0
};

struct IntuiText
DemoIText =
{
    1, 2, /* Pens */
    JAM1, /* Drawmode */
    10, 10, /* Left, Top */
    NULL, /* TextAttr */
    "None", /* Text */
    NULL /* Next */
};

struct Gadget
DemoGadget8 =
{
    NULL, /* NextGadget */
    -(BORDER+GAD_HEI+GAD_WID), -((GAD_HEI+BORDER)*3+GAD_WID), GAD_WID, GAD_WID, /* hit box */
    GFLG_GADGHIMAGE
	| GFLG_RELRIGHT
	| GFLG_RELBOTTOM
	, /* Flags */
    GACT_IMMEDIATE
	| GACT_RELVERIFY
	| GACT_TOGGLESELECT
	, /* Activation */
    GTYP_PROPGADGET, /* Type */
    NULL, NULL, /* Render */
    NULL, /* Text */
    0L, (APTR)&DemoPropInfo3, /* MutualExcl, SpecialInfo */
    9, /* GadgetID */
    NULL /* UserData */
},
DemoGadget7 =
{
    &DemoGadget8, /* NextGadget */
    -(BORDER+GAD_HEI), BORDER, GAD_HEI, -(GAD_HEI*3+BORDER*4), /* hit box */
    GFLG_GADGHIMAGE
	| GFLG_RELRIGHT
	| GFLG_RELHEIGHT
	, /* Flags */
    GACT_IMMEDIATE
	| GACT_RELVERIFY
	| GACT_TOGGLESELECT
	, /* Activation */
    GTYP_PROPGADGET, /* Type */
    NULL, NULL, /* Render */
    NULL, /* Text */
    0L, (APTR)&DemoPropInfo2, /* MutualExcl, SpecialInfo */
    8, /* GadgetID */
    NULL /* UserData */
},
DemoGadget6 =
{
    &DemoGadget7, /* NextGadget */
    BORDER, -((GAD_HEI+BORDER)*3), -(2*BORDER), GAD_HEI, /* hit box */
    GFLG_GADGHIMAGE
	| GFLG_RELBOTTOM
	| GFLG_RELWIDTH
	, /* Flags */
    GACT_IMMEDIATE
	| GACT_RELVERIFY
	| GACT_TOGGLESELECT
	, /* Activation */
    GTYP_PROPGADGET, /* Type */
    NULL, NULL, /* Render */
    NULL, /* Text */
    0L, (APTR)&DemoPropInfo1, /* MutualExcl, SpecialInfo */
    7, /* GadgetID */
    NULL /* UserData */
},
DemoGadget5 =
{
    &DemoGadget6, /* NextGadget */
    BORDER+(GAD_WID+BORDER)*4, -((GAD_HEI+BORDER)*2), GAD_WID, GAD_HEI, /* hit box */
    GFLG_GADGHIMAGE
	| GFLG_LABELSTRING
	| GFLG_RELBOTTOM
	, /* Flags */
    GACT_IMMEDIATE
	| GACT_RELVERIFY
	| GACT_TOGGLESELECT
	, /* Activation */
    GTYP_BOOLGADGET, /* Type */
    &DemoTopBorder, &DemoITopBorder, /* Render */
    (struct IntuiText *)"Toggle", /* Text */
    0L, NULL, /* MutualExcl, SpecialInfo */
    6, /* GadgetID */
    NULL /* UserData */
},
DemoGadget4 =
{
    &DemoGadget5, /* NextGadget */
    BORDER+(GAD_WID+BORDER)*3, -((GAD_HEI+BORDER)*2), GAD_WID, GAD_HEI, /* hit box */
    GFLG_GADGHNONE
	| GFLG_LABELITEXT
	| GFLG_RELBOTTOM
	, /* Flags */
    GACT_IMMEDIATE | GACT_RELVERIFY, /* Activation */
    GTYP_BOOLGADGET, /* Type */
    &DemoTopBorder, NULL, /* Render */
    &DemoIText, /* Text */
    0L, NULL, /* MutualExcl, SpecialInfo */
    5, /* GadgetID */
    NULL /* UserData */
},
DemoGadget3 =
{
    &DemoGadget4, /* NextGadget */
    BORDER+(GAD_WID+BORDER)*2, -((GAD_HEI+BORDER)*2), GAD_WID, GAD_HEI, /* hit box */
    GFLG_GADGHIMAGE
	| GFLG_LABELSTRING
	| GFLG_RELBOTTOM
	, /* Flags */
    GACT_IMMEDIATE | GACT_RELVERIFY, /* Activation */
    GTYP_BOOLGADGET, /* Type */
    &DemoTopBorder, &DemoITopBorder, /* Render */
    (struct IntuiText *)"Image", /* Text */
    0L, NULL, /* MutualExcl, SpecialInfo */
    4, /* GadgetID */
    NULL /* UserData */
},
DemoGadget2 =
{
    &DemoGadget3, /* NextGadget */
    BORDER+(GAD_WID+BORDER)*1, -((GAD_HEI+BORDER)*2), GAD_WID, GAD_HEI, /* hit box */
    GFLG_GADGHBOX
	| GFLG_LABELSTRING
	| GFLG_RELBOTTOM
	, /* Flags */
    GACT_IMMEDIATE | GACT_RELVERIFY, /* Activation */
    GTYP_BOOLGADGET, /* Type */
    &DemoTopBorder, NULL, /* Render */
    (struct IntuiText *)"Box", /* Text */
    0L, NULL, /* MutualExcl, SpecialInfo */
    3, /* GadgetID */
    NULL /* UserData */
},
DemoGadget1 =
{
    &DemoGadget2, /* NextGadget */
    BORDER+(GAD_WID+BORDER)*0, -((GAD_HEI+BORDER)*2), GAD_WID, GAD_HEI, /* hit box */
    GFLG_GADGHCOMP
	| GFLG_LABELSTRING
	| GFLG_RELBOTTOM
	, /* Flags */
    GACT_IMMEDIATE | GACT_RELVERIFY, /* Activation */
    GTYP_BOOLGADGET, /* Type */
    &DemoTopBorder, NULL, /* Render */
    (struct IntuiText *)"Complement", /* Text */
    0L, NULL, /* MutualExcl, SpecialInfo */
    2, /* GadgetID */
    NULL /* UserData */
},
ExitGadget =
{
    &DemoGadget1, /* NextGadget */
    BORDER, -(GAD_HEI+BORDER), GAD_WID, GAD_HEI, /* hit box */
    GFLG_GADGHIMAGE
	| GFLG_LABELSTRING
	| GFLG_RELBOTTOM
	, /* Flags */
    GACT_RELVERIFY, /* Activation */
    GTYP_BOOLGADGET, /* Type */
    &DemoTopBorder, &DemoITopBorder, /* Render */
    (struct IntuiText *)"Exit", /* Text */
    0L, NULL, /* MutualExcl, SpecialInfo */
    1, /* GadgetID */
    NULL /* UserData */
};

int main (int argc, char ** argv)
{
    struct Window * win;
    struct RastPort * rp;
    struct IntuiMessage * im;
    struct IOStdReq cioreq;
    struct InputEvent ievent =
    {
	NULL,
	IECLASS_RAWKEY,
	/* ... */
    };
    int cont, draw;
    int prop;

    bug("Welcome to the window demo of AROS\n");

    GfxBase=(struct GfxBase *)OpenLibrary(GRAPHICSNAME,39);
    IntuitionBase=(struct IntuitionBase *)OpenLibrary("intuition.library",39);

    if (!GfxBase)
    {
	bug ("Couldn't open %s\n", GRAPHICSNAME);
	goto end;
    }

    if (!IntuitionBase)
    {
	bug ("Couldn't open intuition.library\n");
	goto end;
    }

    D(bug("main=%p Refresh=%p\n"
	, main
	, Refresh
    ));

    OpenDevice ("console.device", -1, (struct IORequest *)&cioreq, 0);
    ConsoleDevice = (struct Library *)cioreq.io_Device;
    bug ("Opening console.device=%p\n", ConsoleDevice);

    if (!ConsoleDevice)
    {
	D(bug("Couldn't open console\n"));
	return 10;
    }

    win = OpenWindowTags (NULL
	, WA_Title,	    "Open a window demo"
	, WA_Left,	    100
	, WA_Top,	    100
	, WA_Width,	    640
	, WA_Height,	    512
	, WA_IDCMP,	    IDCMP_RAWKEY
			    | IDCMP_REFRESHWINDOW
			    | IDCMP_MOUSEBUTTONS
			    | IDCMP_MOUSEMOVE
			    | IDCMP_GADGETDOWN
			    | IDCMP_GADGETUP
	, WA_SimpleRefresh, TRUE
	, WA_Gadgets,	    &ExitGadget
	, TAG_END
    );
    D(bug("OpenWindow win=%p\n", win));

    if (!win)
    {
	D(bug("Couldn't open window\n"));
	return 10;
    }

    rp = win->RPort;

    DemoIText.LeftEdge = GAD_WID/2 - rp->Font->tf_XSize*2;
    DemoIText.TopEdge = GAD_HEI/2 - rp->Font->tf_YSize/2 + rp->Font->tf_Baseline;

    cont = 1;
    draw = 0;
    prop = 0;

    while (cont)
    {
	if ((im = (struct IntuiMessage *)GetMsg (win->UserPort)))
	{
	    /* D("Got msg\n"); */
	    switch (im->Class)
	    {
	    case IDCMP_RAWKEY: {
		UBYTE buf[10];
		int   len;
		int   t;

		ievent.ie_Code	    = im->Code;
		ievent.ie_Qualifier = im->Qualifier;

		len = RawKeyConvert (&ievent, buf, sizeof (buf), NULL);

		bug ("Key %s %3ld + Qual %08lx=\""
		    , (im->Code & 0x8000) ? "up  " : "down"
		    , (LONG)(im->Code & 0xFF)
		    , (LONG)im->Qualifier
		);

		if (len < 0)
		{
		    bug ("\" (buffer too short)");
		    break;
		}

		for (t=0; t<len; t++)
		{
		    if (buf[t] < 32 || (buf[t] >= 127 && buf[t] < 160))
		    {
			switch (buf[t])
			{
			case '\n':
			    bug ("\\n");
			    break;

			case '\t':
			    bug ("\\t");
			    break;

			case '\b':
			    bug ("\\b");
			    break;

			case '\r':
			    bug ("\\r");
			    break;

			case 0x1B:
			    bug ("^[");
			    break;

			default:
			    bug ("\\x%02x", buf[t]);
			    break;
			} /* switch */
		    }
		    else
			bug ("%c", buf[t]);
		}
		bug ("\"\n");

		if (*buf == '\x1b' && len == 1)
		{
		    if (len == 1)
			cont = 0;
		}

		break; }

	    case IDCMP_MOUSEBUTTONS:
		switch (im->Code)
		{
		case SELECTDOWN:
		    SetAPen (rp, 2);
		    Move (rp, im->MouseX, im->MouseY);
		    draw = 1;
		    break;

		case SELECTUP:
		    draw = 0;
		    break;

		case MIDDLEDOWN:
		    SetAPen (rp, 1);
		    Move (rp, im->MouseX, im->MouseY);
		    draw = 1;
		    break;

		case MIDDLEUP:
		    draw = 0;
		    break;

		case MENUDOWN:
		    SetAPen (rp, 3);
		    Move (rp, im->MouseX, im->MouseY);
		    draw = 1;
		    break;

		case MENUUP:
		    draw = 0;
		    break;

		}

		break;

	    case IDCMP_MOUSEMOVE:
		if (draw)
		    Draw (rp, im->MouseX, im->MouseY);

		break;

	    case IDCMP_REFRESHWINDOW:
		Refresh (rp);
		break;

	    case IDCMP_GADGETDOWN: {
		struct Gadget * gadget;

		gadget = (struct Gadget *)im->IAddress;

		bug ("User pressed gadget %ld\n", gadget->GadgetID);

		break; }

	    case IDCMP_GADGETUP: {
		struct Gadget * gadget;

		gadget = (struct Gadget *)im->IAddress;

		bug ("User released gadget %ld\n", gadget->GadgetID);

		if (gadget->GadgetID == 1)
		    cont = 0;
		else if (gadget->GadgetID == 2)
		{
		    prop ++;

		    switch (prop)
		    {
		    case 0:
			ModifyProp (&DemoGadget6, win, NULL,
			    DemoPropInfo1.Flags,
			    0, 0, MAXBODY, MAXBODY);
			ModifyProp (&DemoGadget7, win, NULL,
			    DemoPropInfo2.Flags,
			    0, 0, MAXBODY, MAXBODY);
			ModifyProp (&DemoGadget8, win, NULL,
			    DemoPropInfo3.Flags,
			    0, 0, MAXBODY, MAXBODY);
			break;

		    case 1:
			ModifyProp (&DemoGadget6, win, NULL,
			    DemoPropInfo1.Flags,
			    0, 0, MAXBODY/2, MAXBODY);
			ModifyProp (&DemoGadget7, win, NULL,
			    DemoPropInfo2.Flags,
			    0, 0, MAXBODY, MAXBODY/2);
			ModifyProp (&DemoGadget8, win, NULL,
			    DemoPropInfo3.Flags,
			    0, 0, MAXBODY/2, MAXBODY/2);
			break;

		    case 2:
			ModifyProp (&DemoGadget6, win, NULL,
			    DemoPropInfo1.Flags,
			    MAXPOT, 0, MAXBODY/2, MAXBODY);
			ModifyProp (&DemoGadget7, win, NULL,
			    DemoPropInfo2.Flags,
			    0, MAXPOT, MAXBODY, MAXBODY/2);
			ModifyProp (&DemoGadget8, win, NULL,
			    DemoPropInfo3.Flags,
			    MAXPOT, MAXPOT, MAXBODY/2, MAXBODY/2);
			break;

		    default:
			ModifyProp (&DemoGadget6, win, NULL,
			    DemoPropInfo1.Flags,
			    0, 0, MAXBODY/9, MAXBODY);
			ModifyProp (&DemoGadget7, win, NULL,
			    DemoPropInfo2.Flags,
			    0, 0, MAXBODY, MAXBODY/9);
			ModifyProp (&DemoGadget8, win, NULL,
			    DemoPropInfo3.Flags,
			    0, 0, MAXBODY/9, MAXBODY/9);
			prop = -1;
			break;

		    }
		}

		break; }

	    } /* switch */

	    Flush (Output ());

	    ReplyMsg ((struct Message *)im);
	}
	else
	{
	    /* D("Waiting\n"); */
	    Wait (1L << win->UserPort->mp_SigBit);
	}
    }

    D(bug("CloseWindow (%p)\n", win));
    CloseWindow (win);

end:
    if (GfxBase)
	CloseLibrary ((struct Library *)GfxBase);

    if (IntuitionBase)
	CloseLibrary ((struct Library *)IntuitionBase);

    return 0;
}
