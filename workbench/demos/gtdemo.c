/*
    (C) 1997 AROS - The Amiga Replacement OS
    $Id$

    Desc: Demo for gadtools.library
    Lang: english
*/

#define ENABLE_RT 1

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

APTR            vi;
struct Screen * scr;
struct Window * win;
struct Gadget * glist;

#define ID_BUTTON 1

struct NewGadget buttongad =
{
    10, 310, 180, 20,
    "Show _Gadgets", NULL,
    ID_BUTTON, PLACETEXT_IN, NULL, NULL
};


BOOL openlibs()
{
    IntuitionBase = (struct IntuitionBase *)OpenLibrary("intuition.library", 37);
    GadToolsBase = OpenLibrary("gadtools.library", 0);
    if (!IntuitionBase)
    {
        printf("GTDemo: Error opening intuition.library\n");
        return FALSE;
    }
    if (!GadToolsBase)
    {
        printf("GTDemo: Error opening gadtools.library\n");
        return FALSE;
    }
    return TRUE;
}

void closelibs()
{
    CloseLibrary(GadToolsBase);
    CloseLibrary(IntuitionBase);
}


struct Gadget * gt_init()
{
    struct Gadget * gad = NULL;

    glist = NULL;
    scr = LockPubScreen(NULL);
    vi = GetVisualInfoA(scr, NULL);
    if (vi != NULL)
    {
        buttongad.ng_VisualInfo = vi;
        gad = CreateContext(&glist);
    }
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
                         WA_Width, 200,
                         WA_Height, 340,
                         WA_Title, "GTDemo",
                         WA_IDCMP,
                             BUTTONIDCMP |
                             IDCMP_RAWKEY |
                             IDCMP_CLOSEWINDOW |
                             IDCMP_REFRESHWINDOW,
                         WA_SimpleRefresh, TRUE,
                         WA_Gadgets, glist,
                         WA_DragBar, TRUE,
                         WA_CloseGadget, TRUE,
                         TAG_DONE);
    if (!win)
    {
        printf("GTDemo: Error opening window\n");
        return FALSE;
    }
    return TRUE;
}


struct Gadget * makegadgets(struct Gadget *gad)
{
    gad = CreateGadgetA(BUTTON_KIND, gad, &buttongad, NULL);
    if (!gad)
        printf("GTDemo: Error creating gadgets\n");
    return gad;
}

void draw_bevels(struct Window *win, APTR vi)
{
    struct TagItem tags[4];

    tags[0].ti_Tag = GTBB_Recessed;
    tags[1].ti_Tag = GTBB_FrameType;
    tags[2].ti_Tag = GT_VisualInfo;
    tags[2].ti_Data = (IPTR)vi;
    tags[3].ti_Tag = TAG_DONE;
    tags[0].ti_Data = FALSE;
    tags[1].ti_Data = BBFT_BUTTON;
    DrawBevelBoxA(win->RPort, 10, 10, 80, 80, tags);
    tags[0].ti_Data = TRUE;
    DrawBevelBoxA(win->RPort, 110, 10, 80, 80, tags);
    tags[0].ti_Data = FALSE;
    tags[1].ti_Data = BBFT_RIDGE;
    DrawBevelBoxA(win->RPort, 10, 110, 80, 80, tags);
    tags[0].ti_Data = TRUE;
    DrawBevelBoxA(win->RPort, 110, 110, 80, 80, tags);
    tags[0].ti_Data = FALSE;
    tags[1].ti_Data = BBFT_ICONDROPBOX;
    DrawBevelBoxA(win->RPort, 10, 210, 80, 80, tags);
    tags[0].ti_Data = TRUE;
    DrawBevelBoxA(win->RPort, 110, 210, 80, 80, tags);
}

void handlewin()
{
    BOOL ready = FALSE;
    struct IntuiMessage * msg;

    while (ready == FALSE)
    {
        WaitPort(win->UserPort);
        msg = GT_GetIMsg(win->UserPort);
        if (msg != NULL)
	{
            switch(msg->Class)
	    {
            case IDCMP_REFRESHWINDOW:
                GT_BeginRefresh(win);
                draw_bevels(win,vi);
                GT_EndRefresh(win,TRUE);
                break;
            case IDCMP_CLOSEWINDOW:
            case IDCMP_RAWKEY:
                ready = TRUE;
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

    if (openlibs() != FALSE)
    {
        struct Gadget *gad;

        gad = gt_init();
	gad = makegadgets(gad);
        if (gad != NULL)
        {
            if (openwin() != FALSE)
	    {
                draw_bevels(win,vi);
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

    return(error);
}
