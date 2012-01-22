/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/memory.h>
#include <dos/dos.h>
#include <intuition/intuition.h>
#include <intuition/imageclass.h>
#include <intuition/gadgetclass.h>
#include <libraries/gadtools.h>
#include <graphics/gfx.h>
#include <utility/hooks.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/gadtools.h>
#include <proto/alib.h>
#include <proto/utility.h>

#include "global.h"
#include "req.h"

#define CATCOMP_NUMBERS
#include "strings.h"

#define DEBUG 0
#include <aros/debug.h>

/****************************************************************************************/


#define BORDER_SPACING_X 4
#define BORDER_SPACING_Y 4

#define GAD_SPACING_X 8
#define GAD_SPACING_Y 4

#define GAD_EXTRA_WIDTH 16
#define GAD_EXTRA_HEIGHT 6

enum {GAD_FIND_TEXT = 1, 
      GAD_FIND_OK, 
      GAD_FIND_CANCEL};

enum {GAD_GOTO_STRING = 1, 
      GAD_GOTO_OK, 
      GAD_GOTO_CANCEL};

/****************************************************************************************/


static struct RastPort  temprp;
static struct Window    *gotowin, *findwin;
static struct Gadget    *gotogad, *findgad, *gad, *gotogadlist, *findgadlist;
static struct NewGadget ng;
static WORD             fontwidth, fontheight;

static char             searchtext[256];

/****************************************************************************************/

static BOOL Init(void)
{
    fontwidth = dri->dri_Font->tf_XSize;
    fontheight = dri->dri_Font->tf_YSize;

    InitRastPort(&temprp);
    SetFont(&temprp, dri->dri_Font);

    memset(&ng, 0, sizeof(ng));
    ng.ng_VisualInfo = vi;

    return TRUE;
}

/****************************************************************************************/


void CleanupRequesters(void)
{
    if (gotowin) Kill_Goto_Requester();
    if (findwin) Kill_Find_Requester();
}

/****************************************************************************************/


void Make_Goto_Requester(void)
{
    WORD winwidth, winheight, gadwidth, gadheight;
    WORD strwidth, w;

    if (gotowin || !Init()) return;

    gad = CreateContext(&gotogadlist);

    gadheight = fontheight + GAD_EXTRA_HEIGHT;

    winheight = scr->WBorTop + fontheight + 1 +
                scr->WBorBottom +
                BORDER_SPACING_Y * 2 +
                gadheight * 2 +
                GAD_SPACING_Y;

    gadwidth = TextLength(&temprp, MSG(MSG_OK), strlen(MSG(MSG_OK)));
    w = TextLength(&temprp, MSG(MSG_CANCEL), strlen(MSG(MSG_CANCEL)));
    if (w > gadwidth) gadwidth = w;

    gadwidth += GAD_EXTRA_WIDTH;

    strwidth = gadwidth * 2 + GAD_SPACING_X;

    winwidth = scr->WBorLeft +
               scr->WBorRight +
               BORDER_SPACING_X * 2 +
               strwidth;

    ng.ng_LeftEdge = scr->WBorLeft + BORDER_SPACING_X;
    ng.ng_TopEdge = scr->WBorTop + fontheight + 1 + BORDER_SPACING_Y;
    ng.ng_Width = strwidth;
    ng.ng_Height = gadheight;
    ng.ng_GadgetID = GAD_GOTO_STRING;
    ng.ng_Flags = PLACETEXT_IN;

    gotogad = CreateGadget(INTEGER_KIND, gad, &ng, GTIN_MaxChars, 8, 
                                                STRINGA_Justification, GACT_STRINGCENTER, 
                                                TAG_DONE);

    ng.ng_TopEdge += gadheight + GAD_SPACING_Y;
    ng.ng_Width = gadwidth;
    ng.ng_GadgetText = MSG(MSG_OK);
    ng.ng_GadgetID = GAD_GOTO_OK;

    gad = CreateGadgetA(BUTTON_KIND, gotogad, &ng, 0);

    ng.ng_LeftEdge += gadwidth + GAD_SPACING_X;
    ng.ng_GadgetText = MSG(MSG_CANCEL);
    ng.ng_GadgetID = GAD_GOTO_CANCEL;

    gad = CreateGadgetA(BUTTON_KIND, gad, &ng, 0);

    if (!gad)
    {
        FreeGadgets(gotogadlist);
        gotogadlist = 0;
    } else {
        gotowin = OpenWindowTags(0, WA_CustomScreen, (IPTR)scr, 
                                    WA_Left, scr->MouseX - (winwidth / 2), 
                                    WA_Top, scr->MouseY - (winheight / 2), 
                                    WA_Width, winwidth, 
                                    WA_Height, winheight, 
                                    WA_AutoAdjust, TRUE, 
                                    WA_Title, (IPTR)MSG(MSG_JUMP_TITLE), 
                                    WA_CloseGadget, TRUE, 
                                    WA_DepthGadget, TRUE, 
                                    WA_DragBar, TRUE, 
                                    WA_Activate, TRUE, 
                                    WA_SimpleRefresh, TRUE, 
                                    WA_IDCMP, IDCMP_CLOSEWINDOW |
                                             IDCMP_REFRESHWINDOW |
                                             IDCMP_VANILLAKEY |
                                             BUTTONIDCMP |
                                             INTEGERIDCMP, 
                                    WA_Gadgets, (IPTR)gotogadlist, 
                                    TAG_DONE);

        if (!gotowin)
        {
            FreeGadgets(gotogadlist);gotogadlist = 0;
        } else {
            gotomask = 1L << gotowin->UserPort->mp_SigBit;
            GT_RefreshWindow(gotowin, 0);
            ActivateGadget(gotogad, gotowin, 0);
        }
    }
}

/****************************************************************************************/

BOOL Handle_Goto_Requester(LONG *line)
{
    struct IntuiMessage *msg;
    IPTR l;
    BOOL killreq = FALSE, rc = FALSE;

    while ((msg = GT_GetIMsg(gotowin->UserPort)))
    {
        switch (msg->Class)
        {
            case IDCMP_REFRESHWINDOW:
                GT_BeginRefresh(gotowin);
                GT_EndRefresh(gotowin, TRUE);
                break;

            case IDCMP_CLOSEWINDOW:
                killreq = TRUE;
                break;

            case IDCMP_GADGETUP:
                switch (((struct Gadget *)msg->IAddress)->GadgetID)
                {
                    case GAD_GOTO_CANCEL:
                        killreq = TRUE;
                        break;

                    case GAD_GOTO_STRING:
                    case GAD_GOTO_OK:
                        GT_GetGadgetAttrs(gotogad, gotowin, 0, GTIN_Number, (IPTR)&l, 
                                                            TAG_DONE);
                        rc = TRUE;
                        break;

                } /* switch (((struct Gadget *)msg->IAddress)->GadgetID) */
                break;

            case IDCMP_VANILLAKEY:
                switch(ToUpper(msg->Code))
                {
                    case 27:
                        killreq = TRUE;
                        break;

                    case 9:
                    case 'G':
                        ActivateGadget(gotogad, gotowin, 0);
                        break;

                } /* switch(msg->Code) */
                break;

        } /* switch (msg->Class) */
        GT_ReplyIMsg(msg);

    }  /* while ((msg = GT_GetIMsg(gotowin))) */

    if (killreq) Kill_Goto_Requester();

    if (rc) *line = l;

    return rc;
}

/****************************************************************************************/

void Kill_Goto_Requester(void)
{
    if (gotowin)
    {
        CloseWindow(gotowin);gotowin = 0;gotomask = 0;
    }

    if (gotogadlist)
    {
        FreeGadgets(gotogadlist);gotogadlist = 0;
    }
}

/****************************************************************************************/

void Make_Find_Requester(void)
{
    WORD winwidth, winheight, gadwidth, gadheight;
    WORD strwidth, w;

    if (findwin || !Init()) return;

    gad = CreateContext(&findgadlist);

    gadheight = fontheight + GAD_EXTRA_HEIGHT;

    winheight = scr->WBorTop + fontheight + 1 +
                scr->WBorBottom +
                BORDER_SPACING_Y * 2 +
                gadheight * 2 +
                GAD_SPACING_Y;

    gadwidth = TextLength(&temprp, MSG(MSG_OK), strlen(MSG(MSG_OK)));
    w = TextLength(&temprp, MSG(MSG_CANCEL), strlen(MSG(MSG_CANCEL)));
    if (w > gadwidth) gadwidth = w;

    gadwidth += GAD_EXTRA_WIDTH;

    strwidth = gadwidth * 2 + GAD_SPACING_X;

    if (strwidth < 250) strwidth = 250;

    winwidth = scr->WBorLeft +
               scr->WBorRight +
               BORDER_SPACING_X * 2 +
               strwidth;

    ng.ng_LeftEdge = scr->WBorLeft + BORDER_SPACING_X;
    ng.ng_TopEdge = scr->WBorTop + fontheight + 1 + BORDER_SPACING_Y;
    ng.ng_Width = strwidth;
    ng.ng_Height = gadheight;
    ng.ng_GadgetID = GAD_FIND_TEXT;
    ng.ng_Flags = PLACETEXT_IN;

    findgad = CreateGadget(STRING_KIND, gad, &ng, GTST_MaxChars, 256, 
                                               TAG_DONE);

    ng.ng_TopEdge += gadheight + GAD_SPACING_Y;
    ng.ng_Width = gadwidth;
    ng.ng_GadgetText = MSG(MSG_OK);
    ng.ng_GadgetID = GAD_FIND_OK;

    gad = CreateGadgetA(BUTTON_KIND, findgad, &ng, 0);

    ng.ng_LeftEdge = winwidth - scr->WBorRight - BORDER_SPACING_X - gadwidth;
    ng.ng_GadgetText = MSG(MSG_CANCEL);
    ng.ng_GadgetID = GAD_FIND_CANCEL;

    gad = CreateGadgetA(BUTTON_KIND, gad, &ng, 0);

    if (!gad)
    {
        FreeGadgets(findgadlist);
        findgadlist = 0;
    } else {
        findwin = OpenWindowTags(0, WA_CustomScreen, (IPTR)scr, 
                                    WA_Left, scr->MouseX - (winwidth / 2), 
                                    WA_Top, scr->MouseY - (winheight / 2), 
                                    WA_Width, winwidth, 
                                    WA_Height, winheight, 
                                    WA_AutoAdjust, TRUE, 
                                    WA_Title, (IPTR)MSG(MSG_FIND_TITLE), 
                                    WA_CloseGadget, TRUE, 
                                    WA_DepthGadget, TRUE, 
                                    WA_DragBar, TRUE, 
                                    WA_Activate, TRUE, 
                                    WA_SimpleRefresh, TRUE, 
                                    WA_IDCMP, IDCMP_CLOSEWINDOW |
                                              IDCMP_REFRESHWINDOW |
                                              IDCMP_VANILLAKEY |
                                              BUTTONIDCMP |
                                              INTEGERIDCMP, 
                                    WA_Gadgets, (IPTR)findgadlist, 
                                    TAG_DONE);

        if (!findwin)
        {
            FreeGadgets(findgadlist);findgadlist = 0;
        } else {
            findmask = 1L << findwin->UserPort->mp_SigBit;
            GT_RefreshWindow(findwin, 0);
            ActivateGadget(findgad, findwin, 0);
        }
    }
}

/****************************************************************************************/

WORD Handle_Find_Requester(char **text)
{
    struct IntuiMessage *msg;
    char *sp;
    BOOL killreq = FALSE, rc = 0;

    while ((msg = GT_GetIMsg(findwin->UserPort)))
    {
        switch (msg->Class)
        {
            case IDCMP_REFRESHWINDOW:
                GT_BeginRefresh(findwin);
                GT_EndRefresh(findwin, TRUE);
                break;

            case IDCMP_CLOSEWINDOW:
                killreq = TRUE;
                break;

            case IDCMP_GADGETUP:
                switch (((struct Gadget *)msg->IAddress)->GadgetID)
                {
                    case GAD_FIND_CANCEL:
                        killreq = TRUE;
                        break;

                    case GAD_FIND_TEXT:
                    case GAD_FIND_OK:
                        GT_GetGadgetAttrs(findgad, findwin, 0, GTST_String, (IPTR)&sp, 
                                                            TAG_DONE);
                        strcpy(searchtext, sp);

                        rc = TRUE;
                            break;

                } /* switch (((struct Gadget *)msg->IAddress)->GadgetID) */
                break;

            case IDCMP_VANILLAKEY:
                switch(ToUpper(msg->Code))
                {
                    case 27:
                        killreq = TRUE;
                        break;

                    case 9:
                    case 'S':
                    case 'F':
                        ActivateGadget(findgad, findwin, 0);
                        break;

                    case 13:
                    case 'N':
                        rc = SEARCH_NEXT;
                        break;

                    case 'P':
                        rc = SEARCH_PREV;
                        break;

                } /* switch(msg->Code) */
                break;

        } /* switch (msg->Class) */
        GT_ReplyIMsg(msg);

    } /* while ((msg = GT_GetIMsg(findwin))) */

    if (killreq) Kill_Find_Requester();

    if (rc) *text = searchtext;

    return rc;
}

/****************************************************************************************/

void Kill_Find_Requester(void)
{
    if (findwin)
    {
        CloseWindow(findwin);findwin = 0;findmask = 0;
    }

    if (findgadlist)
    {
        FreeGadgets(findgadlist);findgadlist = 0;
    }
}

/****************************************************************************************/


