/*
    (C) 1995-96 AROS - The Amiga Research OS
    $Id$

    Desc: A short demo for the features of Intuition and Graphics
    Lang: english
*/
#define ENABLE_RT	1
#define ENABLE_PURIFY	1

#include <exec/memory.h>
#include <dos/dos.h>
#include <dos/exall.h>
#include <dos/datetime.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/arossupport.h>
#include <proto/alib.h>
#include <proto/utility.h>
#include <proto/graphics.h>
#include <proto/intuition.h>
#include <proto/console.h>
#include <intuition/intuitionbase.h>
#include <intuition/intuition.h>
#include <graphics/gfxbase.h>
#include <graphics/rastport.h>
#include <graphics/gfxmacros.h>
#include <ctype.h>
#include <stdio.h>
#include <aros/rt.h>
#include <intuition/classusr.h>
#include <intuition/gadgetclass.h>
#include <intuition/imageclass.h>
#include <devices/keymap.h>

#if 0
#   define D(x)    x
#else
#   define D(x)     /* eps */
#endif
#define bug	kprintf

static const char version[] = "$VER: demowin 41.1 (14.3.1997)\n";

struct Device *ConsoleDevice;
struct IntuitionBase *IntuitionBase;
struct GfxBase *GfxBase;
Object * frame, * gadget;

void Refresh (struct RastPort * rp)
{
    int len;
    int t, tend, pen;
    int x, y;
    WORD xy[8];

    SetAPen (rp, 1);
    SetDrMd (rp, JAM2);

    Move (rp, 0, 0);
    Draw (rp, 320, 150);

    Move (rp, 640, 0);
    Draw (rp, 0, 300);

    SetDrPt (rp, 0xF0F0);
    len = 150;
    x = 340;
    y = 150;

    xy[0] = x; xy[1] = y+len;
    xy[2] = x+len; xy[3] = y+len;
    xy[4] = x; xy[5] = y;
    Move(rp,x,y);
    PolyDraw (rp, 3, xy);

    SetDrPt (rp, 0xAAAA);
    x += 10;
    y += 20;
    len -= 30;
    xy[0] = x; xy[1] = y+len;
    xy[2] = x+len; xy[3] = y+len;
    xy[4] = x; xy[5] = y;
    Move(rp,x,y);
    PolyDraw (rp, 3, xy);

    SetDrPt (rp, 0x3333);
    x += 10;
    y += 20;
    len -= 30;
    xy[0] = x; xy[1] = y+len;
    xy[2] = x+len; xy[3] = y+len;
    xy[4] = x; xy[5] = y;
    Move(rp,x,y);
    PolyDraw (rp, 3, xy);

    SetDrPt (rp, 0xF731);
    x += 10;
    y += 20;
    len -= 30;
    xy[0] = x; xy[1] = y;
    xy[2] = x; xy[3] = y+len;
    xy[4] = x+len; xy[5] = y+len;
    xy[6] = x; xy[7] = y;
    PolyDraw (rp, 4, xy);

    SetDrPt (rp, ~0);

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

    tend = 10;
    t = 0;

    for (pen=1; pen<16; pen++)
    {
	SetAPen (rp, pen);

	for ( ; t<tend; t++)
	{
	    DrawEllipse (rp, 160, 150, t, t>>1);
	    DrawEllipse (rp, 160, 151, t, t>>1);
	}

	tend += 10;
    }

    SetAPen (rp, 0);
    RectFill (rp, 450, 140, 549, 239);

    for (y=0; y<20; y++)
    {
	for (x=0; x<100; x++)
	{
	    pen = (LONG)ReadPixel (rp, x+100,y+100);
	    SetAPen (rp, pen);
	    WritePixel (rp, x+450, y+140);
	}
    }
}

#define GAD_WID     100
#define GAD_HEI     30
#define BORDER	    20

WORD BorderData[6*2*2] =
{
    0, GAD_HEI-1, /* Top (lighter) edge */
    1, GAD_HEI-2,
    1, 1,
    GAD_WID-2, 1,
    GAD_WID-1, 0,
    0, 0,

    0, -(GAD_HEI-2), /* Bottom (darker) edge */
    -1, -(GAD_HEI-3),
    -1, -1,
    -(GAD_WID-3), -1,
    -(GAD_WID-2), 0,
    -1, 0,
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
    0, 0, /* Left, Top */
    NULL, /* TextAttr */
    "None", /* Text */
    NULL /* Next */
};

#include "images/ArrowUp0.h"
#include "images/ArrowUp1.h"
#include "images/ArrowDown0.h"
#include "images/ArrowDown1.h"
#include "images/ArrowLeft0.h"
#include "images/ArrowLeft1.h"
#include "images/ArrowRight0.h"
#include "images/ArrowRight1.h"
#include "images/ImageButton0.h"
#include "images/ImageButton1.h"

struct Gadget
DemoGadget12 =
{
    NULL, /* &DemoGadget, / * NextGadget */
    -(BORDER+2*ARROWLEFT1_WIDTH), -((GAD_HEI+BORDER)*3),
    ARROWLEFT1_WIDTH, ARROWLEFT1_HEIGHT, /* hit box */
    GFLG_GADGHIMAGE
	| GFLG_RELRIGHT
	| GFLG_RELBOTTOM
	| GFLG_GADGIMAGE
	, /* Flags */
    GACT_IMMEDIATE | GACT_RELVERIFY, /* Activation */
    GTYP_BOOLGADGET, /* Type */
    &ArrowLeft0Image, &ArrowLeft1Image, /* Render */
    NULL, /* Text */
    0L, NULL, /* MutualExcl, SpecialInfo */
    13, /* GadgetID */
    NULL /* UserData */
},
DemoGadget11 =
{
    &DemoGadget12, /* NextGadget */
    -(BORDER+1*ARROWLEFT1_WIDTH), -((GAD_HEI+BORDER)*3),
    ARROWLEFT1_WIDTH, ARROWLEFT1_HEIGHT, /* hit box */
    GFLG_GADGHIMAGE
	| GFLG_RELRIGHT
	| GFLG_RELBOTTOM
	| GFLG_GADGIMAGE
	, /* Flags */
    GACT_IMMEDIATE | GACT_RELVERIFY, /* Activation */
    GTYP_BOOLGADGET, /* Type */
    &ArrowRight0Image, &ArrowRight1Image, /* Render */
    NULL, /* Text */
    0L, NULL, /* MutualExcl, SpecialInfo */
    12, /* GadgetID */
    NULL /* UserData */
},
DemoGadget10 =
{
    &DemoGadget11, /* NextGadget */
    -(BORDER+ARROWDOWN1_WIDTH), -((GAD_HEI+BORDER)*3+ARROWLEFT1_HEIGHT+0*ARROWDOWN1_HEIGHT-5),
    ARROWDOWN1_WIDTH, ARROWDOWN1_HEIGHT, /* hit box */
    GFLG_GADGHIMAGE
	| GFLG_RELRIGHT
	| GFLG_RELBOTTOM
	| GFLG_GADGIMAGE
	, /* Flags */
    GACT_IMMEDIATE | GACT_RELVERIFY, /* Activation */
    GTYP_BOOLGADGET, /* Type */
    &ArrowDown0Image, &ArrowDown1Image, /* Render */
    NULL, /* Text */
    0L, NULL, /* MutualExcl, SpecialInfo */
    11, /* GadgetID */
    NULL /* UserData */
},
DemoGadget9 =
{
    &DemoGadget10, /* NextGadget */
    -(BORDER+ARROWDOWN1_WIDTH), -((GAD_HEI+BORDER)*3+ARROWLEFT1_HEIGHT+1*ARROWDOWN1_HEIGHT-5),
    ARROWDOWN1_WIDTH, ARROWDOWN1_HEIGHT, /* hit box */
    GFLG_GADGHIMAGE
	| GFLG_RELRIGHT
	| GFLG_RELBOTTOM
	| GFLG_GADGIMAGE
	, /* Flags */
    GACT_IMMEDIATE | GACT_RELVERIFY, /* Activation */
    GTYP_BOOLGADGET, /* Type */
    &ArrowUp0Image, &ArrowUp1Image, /* Render */
    NULL, /* Text */
    0L, NULL, /* MutualExcl, SpecialInfo */
    10, /* GadgetID */
    NULL /* UserData */
},
DemoGadget8 =
{
    &DemoGadget9, /* NextGadget */
    -(BORDER+ARROWDOWN1_WIDTH+GAD_WID), -((GAD_HEI+BORDER)*3+GAD_WID),
    GAD_WID, GAD_WID, /* hit box */
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
    -(BORDER+ARROWDOWN1_WIDTH), BORDER,
    ARROWDOWN1_WIDTH, -(GAD_HEI*3+BORDER*4+2*ARROWDOWN1_HEIGHT), /* hit box */
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
    BORDER, -((GAD_HEI+BORDER)*3),
    -(1+2*BORDER+2*ARROWLEFT0_WIDTH), ARROWLEFT0_HEIGHT, /* hit box */
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
    (struct IntuiText *)"_Toggle", /* Text */
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
    BORDER+(GAD_WID+BORDER)*2, -((GAD_HEI+BORDER)*2),
    IMAGEBUTTON0_WIDTH, IMAGEBUTTON0_HEIGHT, /* hit box */
    GFLG_GADGHIMAGE
	| GFLG_LABELSTRING
	| GFLG_RELBOTTOM
	| GFLG_GADGIMAGE
	, /* Flags */
    GACT_IMMEDIATE | GACT_RELVERIFY, /* Activation */
    GTYP_BOOLGADGET, /* Type */
    &ImageButton0Image, &ImageButton1Image, /* Render */
    (struct IntuiText *)"_Image", /* Text */
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
    (struct IntuiText *)"_Box", /* Text */
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
    (struct IntuiText *)"_Complement", /* Text */
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
    (struct IntuiText *)"E_xit", /* Text */
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

    RT_Init ();

    printf ("Welcome to the window demo of AROS\n");

    GfxBase=(struct GfxBase *)OpenLibrary(GRAPHICSNAME,39);
    IntuitionBase=(struct IntuitionBase *)OpenLibrary(INTUITIONNAME,39);

    if (!GfxBase)
    {
	fprintf (stderr, "Couldn't open %s\n", GRAPHICSNAME);
	goto end;
    }

    if (!IntuitionBase)
    {
	fprintf (stderr, "Couldn't open %s\n", INTUITIONNAME);
	goto end;
    }

    cioreq.io_Message.mn_Length = sizeof(struct IOStdReq);
    
    OpenDevice ("console.device", -1, (struct IORequest *)&cioreq, 0);
    ConsoleDevice = cioreq.io_Device;
    printf ("Opening console.device=%p (%s)\n", ConsoleDevice,
	ConsoleDevice && ConsoleDevice->dd_Library.lib_Node.ln_Name ? ConsoleDevice->dd_Library.lib_Node.ln_Name : "(NULL)");

    if (!ConsoleDevice)
    {
	fprintf (stderr, "Couldn't open console\n");
	return 10;
    }

    frame = NewObject (NULL, FRAMEICLASS
	, IA_Width,  GAD_WID
	, IA_Height, GAD_HEI
	, TAG_END
    );

    gadget = NewObject (NULL, FRBUTTONCLASS
	, GA_Left,	BORDER*2+GAD_WID
	, GA_RelBottom, -(GAD_HEI+BORDER)
	, GA_Width,	GAD_WID
	, GA_Height,	GAD_HEI
	, GA_Previous,	(IPTR)&DemoGadget12
	, GA_Text,	(IPTR)"_Exit"
	, GA_RelVerify, TRUE
	, GA_ID,	1
	, GA_Image,	(IPTR)frame
	, TAG_END
    );

    win = OpenWindowTags (NULL
	, WA_Title,	    (IPTR)"Open a window demo"
	, WA_Left,	    100
	, WA_Top,	    50
	, WA_Width,	    640
	, WA_Height,	    512
	, WA_DragBar,	    TRUE
	, WA_CloseGadget,   TRUE
	, WA_DepthGadget,   TRUE
	, WA_IDCMP,	    IDCMP_RAWKEY
			    | IDCMP_REFRESHWINDOW
			    | IDCMP_MOUSEBUTTONS
			    | IDCMP_MOUSEMOVE
			    | IDCMP_GADGETDOWN
			    | IDCMP_GADGETUP
			    | IDCMP_CLOSEWINDOW
	, WA_SimpleRefresh, TRUE
	, WA_Gadgets,	    (IPTR)&ExitGadget
	, TAG_END
    );
    D(printf("OpenWindow win=%p\n", win));

    if (!win)
    {
	fprintf (stderr, "Couldn't open window\n");
	return 10;
    }

    rp = win->RPort;

    DemoIText.LeftEdge = GAD_WID/2 - rp->Font->tf_XSize*2;
    DemoIText.TopEdge = GAD_HEI/2 - rp->Font->tf_YSize/2;
    RefreshGList(&DemoGadget4,win,NULL,1);
    
    if (!gadget)
	printf ("Warning: Couldn't create gadget\n");

    if (!frame)
	printf ("Warning: Couldn't create frame\n");

    cont = 1;
    draw = 0;
    prop = 0;
    
    /* Refresh the window contents */
    Refresh(win->RPort);

    while (cont)
    {
	if ((im = (struct IntuiMessage *)GetMsg (win->UserPort)))
	{
	    /* D("Got msg\n"); */
	    switch (im->Class)
	    {
	    case IDCMP_CLOSEWINDOW:
	    	cont = 0;
		break;
		
	    case IDCMP_RAWKEY: {
		UBYTE buf[10];
		int   len;
		int   t;

		ievent.ie_Code	    = im->Code;
		ievent.ie_Qualifier = im->Qualifier;

		len = RawKeyConvert (&ievent, buf, sizeof (buf), NULL);

		printf ("Key %s %3ld + Qual %08lx=\""
		    , (im->Code & 0x8000) ? "up  " : "down"
		    , (LONG)(im->Code & 0xFF)
		    , (LONG)im->Qualifier
		);

		if (len < 0)
		{
		    printf ("\" (buffer too short)");
		    break;
		}

		for (t=0; t<len; t++)
		{
		    if (buf[t] < 32 || (buf[t] >= 127 && buf[t] < 160))
		    {
			switch (buf[t])
			{
			case '\n':
			    printf ("\\n");
			    break;

			case '\t':
			    printf ("\\t");
			    break;

			case '\b':
			    printf ("\\b");
			    break;

			case '\r':
			    printf ("\\r");
			    break;

			case 0x1B:
			    printf ("^[");
			    break;

			default:
			    printf ("\\x%02x", buf[t]);
			    break;
			} /* switch */
		    }
		    else
			putc (buf[t], stdout);
		}
		printf ("\"\n");

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
		printf ("REFRESHWINDOW\n");
		BeginRefresh (win);
		Refresh (rp);
		EndRefresh (win, TRUE);
		break;

	    case IDCMP_GADGETDOWN: {
		struct Gadget * gadget;
		LONG pot;

		gadget = (struct Gadget *)im->IAddress;

		printf ("User pressed gadget %d\n", gadget->GadgetID);

		switch (gadget->GadgetID)
		{
		case 10: /* Up */
		    pot = DemoPropInfo3.VertPot - DemoPropInfo3.VertBody;

		    if (pot < 0)
			pot = 0;

		    NewModifyProp (&DemoGadget7
			, win
			, NULL
			, DemoPropInfo2.Flags
			, DemoPropInfo2.HorizPot
			, pot
			, DemoPropInfo2.HorizBody
			, DemoPropInfo2.VertBody
			, 1
		    );
		    NewModifyProp (&DemoGadget8
			, win
			, NULL
			, DemoPropInfo3.Flags
			, DemoPropInfo3.HorizPot
			, pot
			, DemoPropInfo3.HorizBody
			, DemoPropInfo3.VertBody
			, 1
		    );

		    break;

		case 11: /* Down */
		    pot = DemoPropInfo3.VertPot + DemoPropInfo3.VertBody;

		    if (pot > MAXPOT)
			pot = MAXPOT;

		    NewModifyProp (&DemoGadget7
			, win
			, NULL
			, DemoPropInfo2.Flags
			, DemoPropInfo2.HorizPot
			, pot
			, DemoPropInfo2.HorizBody
			, DemoPropInfo2.VertBody
			, 1
		    );
		    NewModifyProp (&DemoGadget8
			, win
			, NULL
			, DemoPropInfo3.Flags
			, DemoPropInfo3.HorizPot
			, pot
			, DemoPropInfo3.HorizBody
			, DemoPropInfo3.VertBody
			, 1
		    );

		    break;

		case 12: /* Right */
		    pot = DemoPropInfo1.HorizPot + DemoPropInfo1.HorizBody;

		    if (pot > MAXPOT)
			pot = MAXPOT;

		    NewModifyProp (&DemoGadget6
			, win
			, NULL
			, DemoPropInfo1.Flags
			, pot
			, DemoPropInfo1.VertPot
			, DemoPropInfo1.HorizBody
			, DemoPropInfo1.VertBody
			, 1
		    );
		    NewModifyProp (&DemoGadget8
			, win
			, NULL
			, DemoPropInfo3.Flags
			, pot
			, DemoPropInfo3.VertPot
			, DemoPropInfo3.HorizBody
			, DemoPropInfo3.VertBody
			, 1
		    );

		    break;

		case 13: /* Left */
		    pot = DemoPropInfo1.HorizPot - DemoPropInfo1.HorizBody;

		    if (pot < 0)
			pot = 0;

		    NewModifyProp (&DemoGadget6
			, win
			, NULL
			, DemoPropInfo1.Flags
			, pot
			, DemoPropInfo1.VertPot
			, DemoPropInfo1.HorizBody
			, DemoPropInfo1.VertBody
			, 1
		    );
		    NewModifyProp (&DemoGadget8
			, win
			, NULL
			, DemoPropInfo3.Flags
			, pot
			, DemoPropInfo3.VertPot
			, DemoPropInfo3.HorizBody
			, DemoPropInfo3.VertBody
			, 1
		    );

		    break;

		}

		break; }

	    case IDCMP_GADGETUP: {
		struct Gadget * gadget;

		gadget = (struct Gadget *)im->IAddress;

		printf ("User released gadget %d\n", gadget->GadgetID);

		if (gadget->GadgetID == 1)
		    cont = 0;
		else if (gadget->GadgetID == 2)
		{
		    prop ++;

		    switch (prop)
		    {
		    case 0:
			NewModifyProp (&DemoGadget6, win, NULL,
			    DemoPropInfo1.Flags,
			    0, 0, MAXBODY, MAXBODY, 1);
			NewModifyProp (&DemoGadget7, win, NULL,
			    DemoPropInfo2.Flags,
			    0, 0, MAXBODY, MAXBODY, 1);
			NewModifyProp (&DemoGadget8, win, NULL,
			    DemoPropInfo3.Flags,
			    0, 0, MAXBODY, MAXBODY, 1);
			break;

		    case 1:
			NewModifyProp (&DemoGadget6, win, NULL,
			    DemoPropInfo1.Flags,
			    0, 0, MAXBODY/2, MAXBODY, 1);
			NewModifyProp (&DemoGadget7, win, NULL,
			    DemoPropInfo2.Flags,
			    0, 0, MAXBODY, MAXBODY/2, 1);
			NewModifyProp (&DemoGadget8, win, NULL,
			    DemoPropInfo3.Flags,
			    0, 0, MAXBODY/2, MAXBODY/2, 1);
			break;

		    case 2:
			NewModifyProp (&DemoGadget6, win, NULL,
			    DemoPropInfo1.Flags,
			    MAXPOT, 0, MAXBODY/2, MAXBODY, 1);
			NewModifyProp (&DemoGadget7, win, NULL,
			    DemoPropInfo2.Flags,
			    0, MAXPOT, MAXBODY, MAXBODY/2, 1);
			NewModifyProp (&DemoGadget8, win, NULL,
			    DemoPropInfo3.Flags,
			    MAXPOT, MAXPOT, MAXBODY/2, MAXBODY/2, 1);
			break;

		    default:
			NewModifyProp (&DemoGadget6, win, NULL,
			    DemoPropInfo1.Flags,
			    0, 0, MAXBODY/9, MAXBODY, 1);
			NewModifyProp (&DemoGadget7, win, NULL,
			    DemoPropInfo2.Flags,
			    0, 0, MAXBODY, MAXBODY/9, 1);
			NewModifyProp (&DemoGadget8, win, NULL,
			    DemoPropInfo3.Flags,
			    0, 0, MAXBODY/9, MAXBODY/9, 1);
			prop = -1;
			break;

		    }
		}

		break; }

	    } /* switch */

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
    if (gadget)
	DisposeObject (gadget);

    if (frame)
	DisposeObject (frame);

    if (GfxBase)
	CloseLibrary ((struct Library *)GfxBase);

    if (IntuitionBase)
	CloseLibrary ((struct Library *)IntuitionBase);

    if (ConsoleDevice)
	CloseDevice ((struct IORequest *)&cioreq);

    RT_Exit ();

    return 0;
}
