/*
   (C) 1997 AROS - The Amiga Replacement OS
   $Id$

   Desc: Demo for gadtools.library
   Lang: english
 */

#include <aros/config.h>

#if (AROS_FLAVOUR != AROS_FLAVOUR_NATIVE)
#define ENABLE_RT 1
#endif

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

#include <aros/rt.h>

struct IntuitionBase *IntuitionBase;
struct Library *GadToolsBase;

APTR vi;
struct Screen *scr;
struct Window *win;
struct Gadget *glist = NULL;

struct Gadget *button;

#define ID_BUTTON 1
#define ID_CHECKBOX 2
#define ID_MX 3

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

struct NewGadget mxgad =
{
    210, 40, 0, 0,
    "Mutual Exclude (3)", NULL,
    ID_MX, PLACETEXT_ABOVE, NULL, NULL
};

STRPTR mxlabels[] =
{
    "Label 1",
    "Label 2",
    "Label 3",
    NULL
};


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
    CloseLibrary(GadToolsBase);
    CloseLibrary((struct Library *) IntuitionBase);
}


struct Gadget *gt_init()
{
    struct Gadget *gad = NULL;

    scr = LockPubScreen(NULL);
    vi = GetVisualInfoA(scr, NULL);
    if (vi != NULL)
	gad = CreateContext(&glist);
    return gad;
}

void gt_end()
{
    FreeGadgets(glist);
    FreeVisualInfo(vi);
    UnlockPubScreen(NULL, scr);
}


BOOL openwin()
{
    win = OpenWindowTags(NULL,
			 WA_PubScreen, scr,
			 WA_Left, 0,
			 WA_Top, 0,
			 WA_Width, 400,
			 WA_Height, 300,
			 WA_Title, "GTDemo",
			 WA_IDCMP,
			     BUTTONIDCMP |
			     CHECKBOXIDCMP |
			     MXIDCMP |
                             IDCMP_GADGETUP |
			     IDCMP_RAWKEY |
			     IDCMP_CLOSEWINDOW |
			     IDCMP_REFRESHWINDOW,
			 WA_SimpleRefresh, TRUE,
			 WA_Gadgets, glist,
			 WA_DragBar, TRUE,
			 WA_CloseGadget, TRUE,
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
    mxgad.ng_VisualInfo = vi;

    gad = CreateGadget(BUTTON_KIND, gad, &buttongad,
                       GA_Immediate, TRUE,
                       TAG_DONE);
    button = gad;
    gad = CreateGadget(CHECKBOX_KIND, gad, &checkbox,
                       GTCB_Checked, FALSE,
                       GTCB_Scaled, TRUE,
                       TAG_DONE);
    gad = CreateGadget(MX_KIND, gad, &mxgad,
		       GTMX_Labels, &mxlabels,
		       TAG_DONE);


    if (!gad) {
        FreeGadgets(glist);
        printf("GTDemo: Error creating gadgets\n");
    }
    return gad;
}

void draw_bevels(struct Window *win, APTR vi)
{
    DrawBevelBox(win->RPort, 10, 10, 80, 80,
                 GT_VisualInfo, (IPTR) vi, TAG_DONE);
    DrawBevelBox(win->RPort, 110, 10, 80, 80,
                 GTBB_Recessed, TRUE,
                 GT_VisualInfo, (IPTR) vi, TAG_DONE);
    DrawBevelBox(win->RPort, 10, 110, 80, 80,
                 GTBB_FrameType, BBFT_RIDGE,
                 GT_VisualInfo, (IPTR) vi, TAG_DONE);
    DrawBevelBox(win->RPort, 110, 110, 80, 80,
                 GTBB_FrameType, BBFT_RIDGE, GTBB_Recessed, TRUE,
                 GT_VisualInfo, (IPTR) vi, TAG_DONE);
    DrawBevelBox(win->RPort, 10, 210, 80, 80,
                 GTBB_FrameType, BBFT_ICONDROPBOX,
                 GT_VisualInfo, (IPTR) vi, TAG_DONE);
    DrawBevelBox(win->RPort, 110, 210, 80, 80,
                 GTBB_FrameType, BBFT_ICONDROPBOX, GTBB_Recessed, TRUE,
                 GT_VisualInfo, (IPTR) vi, TAG_DONE);
}

void handlewin()
{
    BOOL ready = FALSE;
    struct IntuiMessage *msg;

    while (ready == FALSE) {
	WaitPort(win->UserPort);
	msg = GT_GetIMsg(win->UserPort);
	if (msg != NULL) {
	    switch (msg->Class) {
	    case IDCMP_REFRESHWINDOW:
		GT_BeginRefresh(win);
		draw_bevels(win, vi);
		GT_EndRefresh(win, TRUE);
		break;
	    case IDCMP_CLOSEWINDOW:
	    case IDCMP_RAWKEY:
		ready = TRUE;
		break;
	    case IDCMP_GADGETDOWN:
		printf("Gadget %d pressed",
		       ((struct Gadget *) msg->IAddress)->GadgetID);
                switch (((struct Gadget *) msg->IAddress)->GadgetID) {
                case ID_MX:{
                    IPTR active;
                    struct TagItem gettags[] =
                    {
                        {GTMX_Active, (IPTR) NULL},
                        {TAG_DONE, 0UL}};
                    gettags[0].ti_Data = (IPTR) & active;

                    GT_GetGadgetAttrs((struct Gadget *) msg->IAddress, win, NULL,
                                      GTMX_Active, (IPTR) & active, TAG_DONE);
                    printf(" (%ld)", active);
                    break;
                }
                }
                printf("\n");
                break;
	    case IDCMP_GADGETUP:
		printf("Gadget %d released",
		       ((struct Gadget *) msg->IAddress)->GadgetID);
		switch (((struct Gadget *) msg->IAddress)->GadgetID) {
		case ID_BUTTON:
		    ready = TRUE;
		    break;
		case ID_CHECKBOX:{
                    IPTR checked;
                    struct TagItem gettags[] =
                    {
                        {GTCB_Checked, (IPTR) NULL},
                        {TAG_DONE, 0UL}};
                    gettags[0].ti_Data = (IPTR) & checked;

                    GT_GetGadgetAttrs((struct Gadget *) msg->IAddress, win, NULL,
                                      GTCB_Checked, (IPTR) & checked, TAG_DONE);
                    if (checked)
                        printf(" (checked)");
                    else
                        printf(" (not checked)");
                    GT_SetGadgetAttrs(button, win, NULL,
                                      GA_Disabled, checked, TAG_DONE);
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

    RT_Init();

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
	gt_end();
    } else
	error = RETURN_FAIL;
    closelibs();

    RT_Exit();

    return (error);
}
