/*
   Copyright © 1997-98, The AROS Development Team. All rights reserved.
   $Id$

   Desc: Demo for gadtools.library
   Lang: english
 */

#include <aros/config.h>

#include <stdio.h>
#include <proto/exec.h>
#include <exec/libraries.h>
#include <exec/types.h>
#include <proto/dos.h>
#include <dos/dos.h>
#include <proto/intuition.h>
#include <intuition/classusr.h>
#include <intuition/gadgetclass.h>
#include <intuition/intuition.h>
#include <intuition/intuitionbase.h>
#include <intuition/screens.h>
#include <utility/tagitem.h>
#include <proto/gadtools.h>
#include <libraries/gadtools.h>

#include <proto/alib.h>

#define SDEBUG 1
#define DEBUG 1
#include <aros/debug.h>
struct IntuitionBase *IntuitionBase;
struct Library *GadToolsBase;

APTR vi;
struct Screen *scr;
struct Window *win;
struct Gadget *glist = NULL;

struct Gadget *button, *strgad, *integergadget;
WORD topoffset;

#define ID_BUTTON 1
#define ID_CHECKBOX 2
#define ID_MX 3
#define ID_CYCLE 4
#define ID_PALETTE 5
#define ID_SLIDER 6
#define ID_SCROLLER 7
#define ID_STRING 8
#define ID_INTEGER 9
#define ID_LISTVIEW 10

struct NewGadget buttongad =
{
    210, 10, 100, 20,
    "Exit (1)", NULL,
    ID_BUTTON, PLACETEXT_IN, NULL, NULL
};

struct NewGadget checkbox =
{
    320, 10, 20, 20,
    "Disable (2)", NULL,
    ID_CHECKBOX, PLACETEXT_RIGHT, NULL, NULL
};

struct NewGadget cyclegad =
{
    210, 140, 100, 20,
    "cyclegad (4)", NULL,
    ID_CYCLE, PLACETEXT_ABOVE, NULL, NULL
};

struct NewGadget mxgad =
{
    210, 60, MX_WIDTH, 20,
    "Mutual Exclude (3)", NULL,
    ID_MX, PLACETEXT_RIGHT, NULL, NULL
};

struct NewGadget palettegad =
{
    210, 180, 120, 100,
    "Palette (5)", NULL,
    ID_PALETTE, PLACETEXT_ABOVE, NULL, NULL
};

struct NewGadget textgad =
{
    380, 40, 120, 30,
    NULL, NULL,
    0, 0, NULL, NULL
};

struct NewGadget numbergad =
{
    380, 80, 70, 20,
    NULL, NULL,
    0, 0, NULL, NULL
};

struct NewGadget slidergad =
{
    380, 130, 120, 20,
    "Slider (6)", NULL,
    ID_SLIDER, PLACETEXT_ABOVE, NULL, NULL
};

struct NewGadget scrollergad =
{
    380, 160, 20, 100,
    NULL, NULL,
    ID_SCROLLER, 0, NULL, NULL
};

struct NewGadget stringgad =
{
    420, 180, 100, 20,
    "String (8)", NULL,
    ID_STRING, PLACETEXT_ABOVE, NULL, NULL
};

struct NewGadget integergad =
{
    420, 240, 60, 20,
    "Integer (9)", NULL,
    ID_INTEGER, PLACETEXT_ABOVE, NULL, NULL
};

struct NewGadget listviewgad =
{
    530, 30, 120, 100,
    "Listview (10)", NULL,
    ID_LISTVIEW, PLACETEXT_ABOVE, NULL, NULL
};

#define NUMLVNODES  20
struct Node lv_nodes[NUMLVNODES];
struct List lv_list;
STRPTR lv_texts[] = {"This", "is", "a", "demo", "of", "the", "GadTools", "listview.",
	"Try", "scrolling", "and", "selecting", "entries",
	"One", "Two", "Three", "Four", "Five", "Six", "Seven"};


STRPTR mxlabels[] =
{
    "Label 1",
    "Label 2",
    "Label 3",
    NULL
};

STRPTR cyclelabels[] =
{
    "Label 1",
    "Label 2",
    "Label 3",
    NULL
};

VOID initlvnodes(struct List *list, struct Node *nodes, STRPTR *texts, WORD numnodes)
{
    WORD i;
    NewList(list);
    
    for (i = 0; i < numnodes; i ++)
    {
    	AddTail(list, &(nodes[i]));
    	nodes[i].ln_Name = texts[i];
    	
    }
    return;
}

BOOL openlibs()
{
    IntuitionBase = (struct IntuitionBase *) OpenLibrary("intuition.library", 37);
    GadToolsBase = OpenLibrary("gadtools.library", 0);
    if (!IntuitionBase) {
	printf("GTDemo: Error opening intuition.library\n");
	return FALSE;
    }
    if (!GadToolsBase) {
        printf("GTDemo: Error opening gadtools.library\n");
	return FALSE;
    }
    return TRUE;
}

void closelibs()
{
D(bug("Closelibs: closing gadtools\n"));
    CloseLibrary(GadToolsBase);
D(bug("Closelibs: closing intuition\n"));
    CloseLibrary((struct Library *) IntuitionBase);
D(bug("Closelibs: libs closed\n"));
}


struct Gadget *gt_init()
{
    struct Gadget *gad = NULL;
    struct DrawInfo *dri;
    
    scr = LockPubScreen(NULL);
    vi = GetVisualInfoA(scr, NULL);
    if (vi != NULL)
	gad = CreateContext(&glist);
	
    if ((dri = GetScreenDrawInfo(scr)))
    {
    	topoffset = dri->dri_Font->tf_YSize + scr->WBorTop - 10 + 5;
	
	buttongad.ng_TopEdge   += topoffset;
	checkbox.ng_TopEdge    += topoffset;
	cyclegad.ng_TopEdge    += topoffset;
	mxgad.ng_TopEdge       += topoffset;
	palettegad.ng_TopEdge  += topoffset;
	textgad.ng_TopEdge     += topoffset;
	numbergad.ng_TopEdge   += topoffset;
	slidergad.ng_TopEdge   += topoffset;
	scrollergad.ng_TopEdge += topoffset;
	stringgad.ng_TopEdge   += topoffset;
	integergad.ng_TopEdge  += topoffset;
	listviewgad.ng_TopEdge += topoffset;
	
	FreeScreenDrawInfo(scr,dri);
    }
    
    return gad;
}

void gt_end()
{
    D(bug("gtend: Freeing gadgets\n"));
    FreeGadgets(glist);
    D(bug("gtend: Freeing visualnfo\n"));
    FreeVisualInfo(vi);
    D(bug("gtend: Unlocking screen\n"));
    UnlockPubScreen(NULL, scr);
}


BOOL openwin()
{
    win = OpenWindowTags(NULL,
			 WA_PubScreen, (IPTR)scr,
			 WA_Left, 0,
			 WA_Top, 0,
			 WA_Width, 700,
			 WA_Height, 300 + topoffset,
			 WA_Title, (IPTR)"GTDemo",
			 WA_IDCMP,
			     BUTTONIDCMP |
			     CHECKBOXIDCMP |
                             CYCLEIDCMP |
                             MXIDCMP |
                             PALETTEIDCMP |
                             SLIDERIDCMP |
			     SCROLLERIDCMP |
			     ARROWIDCMP |
                             IDCMP_GADGETUP |
			     IDCMP_VANILLAKEY |
			     IDCMP_CLOSEWINDOW |
			     IDCMP_REFRESHWINDOW,
//			 WA_SimpleRefresh, TRUE,
			 WA_Gadgets, (IPTR)glist,
			 WA_DragBar, TRUE,
			 WA_CloseGadget, TRUE,
			 WA_DepthGadget, TRUE,
			 TAG_DONE);
    if (!win) {
	printf("GTDemo: Error opening window\n");
	return FALSE;
    }
    return TRUE;
}


struct Gadget *makegadgets(struct Gadget *gad)
{
    buttongad.ng_VisualInfo = vi;
    checkbox.ng_VisualInfo = vi;
    cyclegad.ng_VisualInfo = vi;
    mxgad.ng_VisualInfo = vi;
    palettegad.ng_VisualInfo = vi;
    textgad.ng_VisualInfo = vi;
    numbergad.ng_VisualInfo = vi;
    slidergad.ng_VisualInfo = vi;
    scrollergad.ng_VisualInfo = vi;
    stringgad.ng_VisualInfo = vi;
    integergad.ng_VisualInfo = vi;
    listviewgad.ng_VisualInfo = vi;

    gad = CreateGadget(BUTTON_KIND, gad, &buttongad,
                       GA_Immediate, TRUE,
                       TAG_DONE);
D(bug("Created button gadget: %p\n", gad));
    button = gad;
    gad = CreateGadget(CHECKBOX_KIND, gad, &checkbox,
                       GTCB_Checked, FALSE,
                       GTCB_Scaled, TRUE,
                       TAG_DONE);
D(bug("Created checkbox gadget: %p\n", gad));
    gad = CreateGadget(CYCLE_KIND, gad, &cyclegad,
                       GTCY_Labels, (IPTR)&cyclelabels,
                       TAG_DONE);
D(bug("Created cycle gadget: %p\n", gad));
    gad = CreateGadget(MX_KIND, gad, &mxgad,
		       GTMX_Labels, (IPTR)&mxlabels,
                       GTMX_Scaled, TRUE,
                       GTMX_TitlePlace, PLACETEXT_ABOVE,
		       TAG_DONE);

D(bug("Created mx gadget: %p\n", gad));
    gad = CreateGadget(PALETTE_KIND, gad, &palettegad,
    		       GTPA_NumColors,		6,
    		       GTPA_IndicatorHeight,	30,
    		       GTPA_Color,		0,
		       TAG_DONE);

D(bug("Created palette gadget: %p\n", gad));
    gad = CreateGadget(TEXT_KIND, gad, &textgad,
    		       GTTX_Text,	(IPTR)"Text display",
    		       GTTX_CopyText,	TRUE,
    		       GTTX_Border,	TRUE,
    		       GTTX_Justification,	GTJ_CENTER,
		       TAG_DONE);

D(bug("Created text gadget: %p\n", gad));
    gad = CreateGadget(NUMBER_KIND, gad, &numbergad,
    		       GTNM_Number,	10,
    		       GTNM_Border,	TRUE,
    		       GTNM_Justification,	GTJ_CENTER,
		       TAG_DONE);
    
D(bug("Created number gadget: %p\n", gad));
    gad = CreateGadget(SLIDER_KIND, gad, &slidergad,
    		       GTSL_Min,		10,
    		       GTSL_Max,		20,
    		       GTSL_Level,		12,
    		       GTSL_MaxLevelLen,	3,
    		       GTSL_LevelFormat,	(IPTR)"%2ld",
    		       GTSL_LevelPlace,		PLACETEXT_RIGHT,
    		       GTSL_Justification,	GTJ_RIGHT,
    		       PGA_Freedom,		LORIENT_HORIZ,
		       TAG_DONE);


D(bug("Created slider gadget: %p\n", gad));
    gad = CreateGadget(SCROLLER_KIND, gad, &scrollergad,
    		       GTSC_Top,		2,
    		       GTSC_Total,		10,
    		       GTSC_Visible,		2,
    		       GTSC_Arrows,		10,
    		       GA_RelVerify,		TRUE,
    		       PGA_Freedom,		LORIENT_VERT,
		       TAG_DONE);

D(bug("Created scroller gadget: %p\n", gad));
    gad = strgad = CreateGadget(STRING_KIND, gad, &stringgad,
    		       GTST_String,		(IPTR)"Blahblahblah",
    		       GTST_MaxChars,		80,
    		       GTSC_Visible,		2,
    		       GA_Immediate,		TRUE,
		       TAG_DONE);

D(bug("Created string gadget: %p\n", gad));

    gad = integergadget = CreateGadget(INTEGER_KIND, gad, &integergad,
    		       GTIN_Number,		100,
    		       GTIN_MaxChars,		5,
    		       STRINGA_Justification,	GACT_STRINGCENTER,
    		       GA_Immediate,		TRUE,
		       TAG_DONE);

D(bug("Created integer gadget: %p\n", gad));

    
    initlvnodes(&lv_list, lv_nodes, lv_texts, NUMLVNODES);
D(bug("Inited lv nodes\n"));
    gad = CreateGadget(LISTVIEW_KIND, gad, &listviewgad,
    		GTLV_Labels,	(IPTR)&lv_list,
    		GTLV_ReadOnly,	FALSE,
		TAG_DONE);

D(bug("Created listview gadget: %p\n", gad));

    if (!gad) {
        FreeGadgets(glist);
        printf("GTDemo: Error creating gadgets\n");
    }
    return gad;
}

void draw_bevels(struct Window *win, APTR vi)
{
    DrawBevelBox(win->RPort, 10, 10 + topoffset, 80, 80,
                 GT_VisualInfo, (IPTR) vi, TAG_DONE);
    DrawBevelBox(win->RPort, 110, 10 + topoffset, 80, 80,
                 GTBB_Recessed, TRUE,
                 GT_VisualInfo, (IPTR) vi, TAG_DONE);
    DrawBevelBox(win->RPort, 10, 110 + topoffset, 80, 80,
                 GTBB_FrameType, BBFT_RIDGE,
                 GT_VisualInfo, (IPTR) vi, TAG_DONE);
    DrawBevelBox(win->RPort, 110, 110 + topoffset, 80, 80,
                 GTBB_FrameType, BBFT_RIDGE, GTBB_Recessed, TRUE,
                 GT_VisualInfo, (IPTR) vi, TAG_DONE);
    DrawBevelBox(win->RPort, 10, 210 + topoffset, 80, 80,
                 GTBB_FrameType, BBFT_ICONDROPBOX,
                 GT_VisualInfo, (IPTR) vi, TAG_DONE);
    DrawBevelBox(win->RPort, 110, 210 + topoffset, 80, 80,
                 GTBB_FrameType, BBFT_ICONDROPBOX, GTBB_Recessed, TRUE,
                 GT_VisualInfo, (IPTR) vi, TAG_DONE);
}

void handlewin()
{
    BOOL ready = FALSE;
    struct IntuiMessage *msg;

    GT_SetGadgetAttrs(strgad,win,NULL,GTST_String, (IPTR)"Hello",
    				      TAG_DONE);
	
    GT_SetGadgetAttrs(integergadget,win,NULL,GTIN_Number,1000,TAG_DONE);
    			      
    while (ready == FALSE) {
	WaitPort(win->UserPort);
	msg = GT_GetIMsg(win->UserPort);
	if (msg != NULL) {
	    switch (msg->Class) {
	    case IDCMP_REFRESHWINDOW:
		GT_BeginRefresh(win);
		draw_bevels(win, vi);
		GT_EndRefresh(win, TRUE);
		D(bug("Got IDCMP_REFRESHWINDOW msg\n"));
		break;
            case IDCMP_VANILLAKEY:
                if (msg->Code != 0x1B) /* if escape, quit */
                    break;
	    case IDCMP_CLOSEWINDOW:
		ready = TRUE;
		break;
	    case IDCMP_GADGETDOWN:
		printf("Gadget %d pressed",
		       ((struct Gadget *) msg->IAddress)->GadgetID);
                switch (((struct Gadget *) msg->IAddress)->GadgetID) {
                case ID_MX:
                    printf(" (active: %d)", msg->Code);
                    break;
                }
                printf("\n");
                break;
            case IDCMP_MOUSEMOVE:
            	if (msg->IAddress)
            	{
            	    switch (((struct Gadget *) msg->IAddress)->GadgetID) {
            	    case ID_SLIDER:
            	    	printf("Slider moved to value %d\n", msg->Code);
            	    	break;
            	    	
            	    case ID_SCROLLER:
            	    	printf("Scroller moved to value %d\n", msg->Code);
            	    	break;

            	    }

            	}
            	break;
            	
	    case IDCMP_GADGETUP:
		printf("Gadget %d released",
		       ((struct Gadget *) msg->IAddress)->GadgetID);
		switch (((struct Gadget *) msg->IAddress)->GadgetID) {
		case ID_BUTTON:
		    ready = TRUE;
		    break;
		    
		case ID_PALETTE:
		    printf(" (color: %d)", msg->Code);
		    break;
		
		case ID_LISTVIEW:
		    printf(" (lv item: %d)", msg->Code);
		    break;
		
		case ID_CYCLE:
		    printf(" (cycle item: %d)", msg->Code);
		    break;
		           
		case ID_CHECKBOX:{
                    BOOL checked;

                    checked = msg->Code;
                    if (checked)
                        printf(" (checked)");
                    else
                        printf(" (not checked)");
                    GT_SetGadgetAttrs(button, win, NULL,
                                      GA_Disabled, (IPTR)checked, TAG_DONE);
                    break;
                }
		}
		printf("\n");
		break;
	    }
	    GT_ReplyIMsg(msg);
	}
    }
}


int main()
{
    int error = RETURN_OK;



#if SDEBUG     /* Debugging hack */
    struct Task *idtask;
    SDInit();
    if ((idtask = FindTask("input.device")))
    	idtask->tc_UserData = NULL;
#endif    	

    if (openlibs() != FALSE) {
	struct Gadget *gad;

	gad = gt_init();
	gad = makegadgets(gad);
	if (gad != NULL) {
	    if (openwin() != FALSE) {
		draw_bevels(win, vi);
		handlewin();
		CloseWindow(win);
	    } else
		error = RETURN_FAIL;
	} else
	    error = RETURN_FAIL;
	    
	D(bug("Doing gt_end()\n"));
	gt_end();
    } else
	error = RETURN_FAIL;

D(bug("closing libs\n"));
    closelibs();

    return (error);
}
