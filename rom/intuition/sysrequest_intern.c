/*
    Copyright (C) 1995-2023, The AROS Development Team. All rights reserved.
    Copyright (C) 2001-2003, The MorphOS Development Team. All Rights Reserved.
*/

/**********************************************************************************************/
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/keymap.h>
#include <proto/utility.h>

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <clib/macros.h>

#include <exec/memory.h>
#include <intuition/gadgetclass.h>
#include <intuition/imageclass.h>
#include <intuition/screens.h>
#include <graphics/rastport.h>
#include <graphics/gfxmacros.h>
#include <utility/tagitem.h>
#include "intuition_intern.h"

/**********************************************************************************************/

static BOOL buildsysreq_calculatedims(struct IntReqDims *dims,
                                      struct Screen *scr,
                                      struct IntuiText *itext,
                                      STRPTR *gadgetlabels,
                                      struct IntuitionBase *IntuitionBase);
static struct Gadget *buildsysreq_makegadgets(struct IntReqDims *dims,
                                STRPTR *gadgetlabels,
                                struct Screen *scr,
                                struct IntuitionBase *IntuitionBase);

static void ReqITextSize(struct Screen *scr, struct IntuiText *itext,
                         WORD *width, WORD *height,
                         struct IntuitionBase *IntuitionBase);

static void ReqPrintIText(struct Screen *scr, struct DrawInfo *dri,
                          struct RastPort *rp, struct IntuiText *itext, WORD x, WORD y,
                          struct IntuitionBase *IntuitionBase);

/**********************************************************************************************/

struct Window *buildsysreq_intern(struct Window *window, STRPTR reqtitle, struct IntuiText *bodytext,
                                  struct IntuiText *postext, struct IntuiText *negtext,
                                  ULONG IDCMPFlags, WORD width, WORD height, struct IntuitionBase *IntuitionBase)
{
    struct Screen               *scr = NULL, *lockedscr = NULL;
    struct Window               *req;
    struct Gadget               *gadgets;
    STRPTR                       gadgetlabels[3];
    struct IntRequestUserData   *requserdata;

    DEBUG_BUILDSYSREQUEST(dprintf("intrequest_buildsysrequest: window 0x%p body <%s> postext <%s> negtext <%s> IDCMPFlags 0x%lx width %ld height %ld\n",
                                  window,
                                  bodytext ? (char *) bodytext->IText : "<NULL>",
                                  (postext && postext->IText) ? (char *) postext->IText : "<NULL>",
                                  (negtext && negtext->IText) ? (char *) negtext->IText : "<NULL>",
                                  IDCMPFlags,
                                  (LONG) width,
                                  (LONG) height));

    /* negtext and bodytest must be specified, postext is optional */
    if (!negtext || !bodytext) return NULL;

    /* get requester title */
    if (!reqtitle)
        reqtitle = window ? window->Title : (STRPTR)"System Request"; /* stegerg: should be localized */

    /* get screen and screendrawinfo */
    if (window)
        scr = window->WScreen;
    if (!scr)
    {
        scr = LockPubScreen(NULL);
        if (!scr)
            return NULL;
        lockedscr = scr;
    }

    /* create everything */
    requserdata = AllocVec(sizeof(struct IntRequestUserData),
                           MEMF_ANY|MEMF_CLEAR);
    DEBUG_BUILDSYSREQUEST(dprintf("intrequest_buildsysrequest: requserdata 0x%p\n", requserdata));
    if (requserdata)
    {
        requserdata->ReqBody = bodytext;
        requserdata->drawreq = buildreq_draw;

        if (postext)
        {
            requserdata->ReqDims.gadgets = 2;

            gadgetlabels[0] = postext->IText;
            gadgetlabels[1] = negtext->IText;
            gadgetlabels[2] = NULL;
        }
        else
        {
            requserdata->ReqDims.gadgets = 1;

            gadgetlabels[0] = negtext->IText;
            gadgetlabels[1] = NULL;
        }

        /* EXPERIMENTAL: Obey user-supplied size if the text fits into it. Processed in buildsysreq_calculatedims().
           This is experimental. Currently DisplayAlert() relies on ability to specify requester size.
           Requester size actually determines inner text box size - sonic. */
        requserdata->ReqDims.width  = width;
        requserdata->ReqDims.height = height;

        if (buildsysreq_calculatedims(&requserdata->ReqDims, scr,
                                      bodytext, gadgetlabels, IntuitionBase))
        {
            gadgets = buildsysreq_makegadgets(&requserdata->ReqDims, gadgetlabels, scr, IntuitionBase);
            DEBUG_BUILDSYSREQUEST(dprintf("intrequest_buildsysrequest: gadgets 0x%p\n", gadgets));
            if (gadgets)
            {
                struct TagItem win_tags[] =
                {
                    {WA_Width                                       , requserdata->ReqDims.width                                                        },
                    {WA_Height                                      , requserdata->ReqDims.height                                                       },
                    {WA_Left                                        , (scr->Width/2) - (requserdata->ReqDims.width/2)                                   },
                    {WA_Top                                         , (scr->Height/2) - (requserdata->ReqDims.height/2)                                 },
                    {WA_IDCMP                                       , (IDCMP_GADGETUP | IDCMP_RAWKEY | (IDCMPFlags & ~IDCMP_VANILLAKEY))},
                    {WA_Gadgets                                     , (IPTR)gadgets                                                     },
                    {WA_Title                                       , (IPTR)reqtitle                                                    },
                    {(lockedscr ? WA_PubScreen : WA_CustomScreen)   , (IPTR)scr                                                         },
                    {WA_Flags                                       , WFLG_DRAGBAR     |
                                                                      WFLG_DEPTHGADGET |
                                                                      WFLG_ACTIVATE    |
                                                                      WFLG_RMBTRAP   /*|
                                                                      WFLG_SIMPLE_REFRESH*/                                             },
                    {TAG_DONE                                                                                                           }
                };

                req = OpenWindowTagList(NULL, win_tags);
                DEBUG_BUILDSYSREQUEST(dprintf("intrequest_buildsysrequest: req 0x%p\n", req));
                if (req)
                {
                    if (lockedscr) UnlockPubScreen(NULL, lockedscr);

                    req->UserData = (BYTE *)requserdata;
                    requserdata->IDCMP = IDCMPFlags;
                    requserdata->GadgetLabels = NULL;
                    requserdata->Gadgets = gadgets;

                    buildreq_draw(&requserdata->ReqDims, requserdata->ReqBody,
                                     req, scr, requserdata->Gadgets, IntuitionBase);
                    DEBUG_BUILDSYSREQUEST(dprintf("intrequest_buildsysrequest: gadgets 0x%p\n", requserdata->Gadgets));
                    gadgets = requserdata->Gadgets;

                    return req;
                }
                intrequest_freegadgets(gadgets, IntuitionBase);
            }
        }
        /* opening requester failed -> free everything */
        FreeVec(requserdata);
    }

    if (lockedscr) UnlockPubScreen(NULL, lockedscr);

    return NULL;
}

/**********************************************************************************************/

static struct IntuiMessage *sysreqhandler_getmsg(struct Window *window, BOOL WaitInput)
{
    if (WaitInput)
    {
        WaitPort(window->UserPort);
    }
    return (struct IntuiMessage *)GetMsg(window->UserPort);
}

LONG sysreqhandler_intern(struct Window *window, ULONG *IDCMPFlagsPtr, BOOL WaitInput, struct IntuitionBase *IntuitionBase)
{
    struct Library *KeymapBase = GetPrivIBase(IntuitionBase)->KeymapBase;
    struct Library *UtilityBase = GetPrivIBase(IntuitionBase)->UtilityBase;
    struct IntuiMessage *msg;
    LONG                 result;

    DEBUG_SYSREQHANDLER(dprintf("SysReqHandler: window 0x%p IDCMPPtr 0x%p WaitInput 0x%lx\n",
                                window,
                                IDCMPFlagsPtr,
                                (ULONG) WaitInput));

    if (window == 0)
    {
        result = 0;
    }
    else if (window == (struct Window *)1)
    {
        result = 1;
    }
    else
    {
        result = -2;

        while ((result <= -2) && (msg = sysreqhandler_getmsg(window, WaitInput)))
        {
            DEBUG_SYSREQHANDLER(dprintf("SysReqHandler: msg 0x%p class 0x%lx\n", msg, msg->Class));
            switch (msg->Class)
            {
            /* we don't use VANILLA (filtered from useridcmp!) to get
            all events we need */
            case IDCMP_RAWKEY:
            {
#define RKBUFLEN 1
                
                struct InputEvent ie;
                char              rawbuffer[RKBUFLEN];

                DEBUG_SYSREQHANDLER(dprintf("SysReqHandler: IDCMP_RAWKEY\n"));

                ie.ie_Class         = IECLASS_RAWKEY;
                ie.ie_SubClass      = 0;
                ie.ie_Code          = msg->Code;
                ie.ie_Qualifier     = 0;
                ie.ie_EventAddress  = (APTR *) *((IPTR *)msg->IAddress);
                
                if (KeymapBase && MapRawKey(&ie,rawbuffer,RKBUFLEN,0))
                {
                    if (msg->Qualifier & IEQUALIFIER_LCOMMAND)
                    {
                        if  (ToUpper(rawbuffer[0]) == ToUpper(GetPrivIBase(IntuitionBase)->IControlPrefs.ic_ReqTrue))
                        {
                            if (((struct IntRequestUserData *)window->UserData)->ReqDims.gadgets > 1)
                            {
                                result = 1;
                            }
                            else
                            {
                                result = 0;
                            }
                        }

                        if  (ToUpper(rawbuffer[0]) == ToUpper(GetPrivIBase(IntuitionBase)->IControlPrefs.ic_ReqFalse))
                        {
                            result = 0;
                        }
                    }
                }
                break;
            }

            case IDCMP_GADGETUP:
                DEBUG_SYSREQHANDLER(dprintf("SysReqHandler: IDCMP_GADGETUP\n"));
                result = ((struct Gadget *)msg->IAddress)->GadgetID;
                break;

            case IDCMP_IDCMPUPDATE:
                DEBUG_SYSREQHANDLER(dprintf("SysReqHandler: IDCMP_IDCMPUPDATE\n"));
                UWORD scrollID = (UWORD)GetTagData(GA_ID, 0, (struct TagItem *)msg->IAddress);
                if (scrollID == (UWORD)-3)
                {
                    DEBUG_SYSREQHANDLER(dprintf("SysReqHandler: Vert Prop %u\n", msg->Code));
                    result = -3;
                    ((struct IntRequestUserData *)window->UserData)->ReqDims.offy = msg->Code;
                }
                else if (scrollID == (UWORD)-4)
                {
                    DEBUG_SYSREQHANDLER(dprintf("SysReqHandler: Horiz Prop\n"));
                    result = -4;
                    ((struct IntRequestUserData *)window->UserData)->ReqDims.offx = msg->Code;
                }
                if (((struct IntRequestUserData *)window->UserData)->drawreq)
                {
                    DEBUG_SYSREQHANDLER(dprintf("SysReqHandler: Calling render hook\n"));
                    ((struct IntRequestUserData *)window->UserData)->drawreq(&((struct IntRequestUserData *)window->UserData)->ReqDims,
                            ((struct IntRequestUserData *)window->UserData)->ReqBody, window, window->WScreen, ((struct IntRequestUserData *)window->UserData)->Gadgets, IntuitionBase);
                }
                break;

            default:
                if (result == -2)
                {
                    if (msg->Class & ((struct IntRequestUserData *)window->UserData)->IDCMP)
                    {
                        DEBUG_SYSREQHANDLER(dprintf("SysReqHandler: passing IDCMP to caller\n"));
                        if (IDCMPFlagsPtr) *IDCMPFlagsPtr = msg->Class;
                        result = -1;
                    }
                    else
                    {
                        DEBUG_SYSREQHANDLER(dprintf("SysReqHandler: unexpected IDCMP\n"));
                    }
                }
                break;
            }
            ReplyMsg((struct Message *)msg);

        } /* while ((msg = (struct IntuiMessage *)GetMsg(window->UserPort))) */

    } /* real window */

    DEBUG_SYSREQHANDLER(dprintf("SysReqHandler: Result 0x%lx\n",result));

    if (result <= -2)
        result = -2;

    return result;
}

/**********************************************************************************************/

void freesysreq_intern(struct Window *window, struct IntuitionBase *IntuitionBase)
{
    struct Gadget               *gadgets;
    STRPTR                      *gadgetlabels;
    struct IntRequestUserData   *requserdata;

    DEBUG_FREESYSREQUEST(dprintf("intrequest_freesysrequest: window 0x%lx\n", (ULONG) window));

    if ((window == NULL) || (window == (void *)1L))
        return;

    requserdata = (struct IntRequestUserData *)window->UserData;

    DEBUG_FREESYSREQUEST(dprintf("intrequest_freesysrequest: requserdata 0x%lx\n", (ULONG) requserdata));

    gadgets = requserdata->Gadgets;

    DEBUG_FREESYSREQUEST(dprintf("intrequest_freesysrequest: gadgets 0x%lx\n", (ULONG) gadgets));

    /* Remove gadgets before closing window to avoid conflicts with system gadgets */
    RemoveGList(window, gadgets, requserdata->ReqDims.gadgets);

    gadgetlabels = requserdata->GadgetLabels;

    DEBUG_FREESYSREQUEST(dprintf("intrequest_freesysrequest: gadgetlabels 0x%lx\n", (ULONG) gadgetlabels));

    window->UserData = 0;
    CloseWindow(window);
#if (0)
    if (requserdata->RawBody)
    {
        if (requserdata->ReqBody)
            FreeVec(requserdata->ReqBody);
        FreeVec(requserdata->RawBody);
    }
#endif
    intrequest_freegadgets(gadgets, IntuitionBase);
    intrequest_freelabels(gadgetlabels, IntuitionBase);

#ifdef SKINS
    DEBUG_FREESYSREQUEST(dprintf("intrequest_freesysrequest: freeitext 0x%lx\n", (ULONG) requserdata->freeitext));
    if (requserdata->freeitext) intrequest_freeitext(requserdata->Text,IntuitionBase);
    if (requserdata->backfilldata.image) int_FreeCustomImage(TYPE_REQUESTERCLASS,requserdata->dri,IntuitionBase);
    if (requserdata->Logo) int_FreeCustomImage(TYPE_REQUESTERCLASS,requserdata->dri,IntuitionBase);
    if (requserdata->ReqGadgets) FreeVec(requserdata->ReqGadgets);
    if (requserdata->dri) FreeScreenDrawInfo(requserdata->ReqScreen,(struct DrawInfo *)requserdata->dri);
#endif
    FreeVec(requserdata);
} /* FreeSysRequest */

/**********************************************************************************************/

/* calculate dimensions of the requester */
static BOOL buildsysreq_calculatedims(struct IntReqDims *dims,
                                      struct Screen *scr,
                                      struct IntuiText *itext,
                                      STRPTR *gadgetlabels,
                                      struct IntuitionBase *IntuitionBase)
{
    struct GfxBase *GfxBase = GetPrivIBase(IntuitionBase)->GfxBase;
    LONG  currentgadget = 0;
    WORD  itextwidth, itextheight;
    UWORD textboxwidth = 0, gadgetswidth; /* width of upper/lower part */
    UWORD textboxheight;

    /* calculate height of requester */
    dims->fontheight = scr->RastPort.Font->tf_YSize;

    ReqITextSize(scr, itext, &itextwidth, &itextheight, IntuitionBase);

    textboxheight = scr->WBorTop + dims->fontheight + 1 +
                   OUTERSPACING_Y +
                   TEXTBOXBORDER_Y +
                   itextheight +
                   TEXTBOXBORDER_Y +
                   TEXTGADGETSPACING +
                   BUTTONBORDER_Y +
                   dims->fontheight +
                   BUTTONBORDER_Y +
                   OUTERSPACING_Y +
                   scr->WBorBottom;

    /*
     * Ensure that text fits into requested height.
     * Note that calculated size will override user-supplied size if the latter
     * is not large enough, but not vice versa.
     * This behavior is experimental. DisplayAlert() currently relies on it - sonic
     * See also similar check for width below.
     */
    if (textboxheight > dims->height)
        dims->height = textboxheight;

    if (dims->height > scr->Height)
        return FALSE;

    textboxwidth = itextwidth + TEXTBOXBORDER_X * 2;

    /* calculate width of gadgets */
    dims->gadgetwidth = 0;
    while (gadgetlabels[currentgadget])
    {
        UWORD gadgetwidth; /* width of current gadget */

        gadgetwidth = TextLength(&scr->RastPort, gadgetlabels[currentgadget],
                                 strlen(gadgetlabels[currentgadget]));
        if (gadgetwidth > dims->gadgetwidth)
            dims->gadgetwidth = gadgetwidth;
        currentgadget++;
    }
    dims->gadgetwidth += BUTTONBORDER_X * 2;
    gadgetswidth = (dims->gadgetwidth + GADGETGADGETSPACING) * dims->gadgets - GADGETGADGETSPACING;

    /* calculate width of requester and req text position */
    dims->textleft = scr->WBorLeft + OUTERSPACING_X + TEXTBOXBORDER_X;
    if (textboxwidth <= gadgetswidth)
    {
        dims->textleft += (gadgetswidth - textboxwidth) / 2;
        textboxwidth = gadgetswidth;
    }

    /* EXPERIMENTAL: Ensure that text fits into requested width */
    if (textboxwidth > dims->width)
        dims->width = textboxwidth;

    dims->width += OUTERSPACING_X * 2 + scr->WBorLeft + scr->WBorRight;
    if (dims->width > scr->Width)
        return FALSE;

    return TRUE;
}

/**********************************************************************************************/

/* make all the gadgets */
static struct Gadget *buildsysreq_makegadgets(struct IntReqDims *dims,
                    STRPTR *gadgetlabels,
                    struct Screen *scr,
                    struct IntuitionBase *IntuitionBase)
{
    struct TagItem  frame_tags[] =
    {
        {IA_FrameType, FRAME_BUTTON                         },
        {IA_Width    , dims->gadgetwidth                    },
        {IA_Height   , dims->fontheight + BUTTONBORDER_Y * 2},
        {TAG_DONE                                           }
    };
    struct Gadget   *gadgetlist, *thisgadget = NULL;
    struct Image    *gadgetframe;
    WORD             currentgadget;
    UWORD            xoffset, restwidth;

    if (gadgetlabels[0] == NULL)
        return NULL;

    gadgetframe = (struct Image *)NewObjectA(NULL, FRAMEICLASS, frame_tags);
    if (!gadgetframe)
        return NULL;

    restwidth = dims->width - scr->WBorLeft - scr->WBorRight - OUTERSPACING_X * 2;
    if (dims->gadgets == 1)
        xoffset = scr->WBorLeft + OUTERSPACING_X + (restwidth - dims->gadgetwidth) / 2;
    else
    {
        xoffset = scr->WBorLeft + OUTERSPACING_X;
        restwidth -= dims->gadgets * dims->gadgetwidth;
    }

    gadgetlist = NULL;

    for (currentgadget = 0; gadgetlabels[currentgadget]; currentgadget++)
    {
        WORD           gadgetid = (currentgadget == (dims->gadgets - 1)) ? 0 : currentgadget + 1;
        struct TagItem gad_tags[] =
        {
            {GA_ID          , gadgetid                              },
            {GA_Previous    , (IPTR)thisgadget                      },
            {GA_Left        , xoffset                               },
            {GA_Top         , dims->height -
                              scr->WBorBottom - dims->fontheight -
                              OUTERSPACING_Y - BUTTONBORDER_Y * 2   },
            {GA_Image       , (IPTR)gadgetframe                     },
            {GA_RelVerify   , TRUE                                  },
            {TAG_DONE                                               }
        };
        struct TagItem gad2_tags[] =
        {
            {GA_Text        , (IPTR)gadgetlabels[currentgadget]     },
            {TAG_DONE                                               }
        };

        thisgadget = NewObjectA(NULL, FRBUTTONCLASS, gad_tags);

        if (currentgadget == 0)
            gadgetlist = thisgadget;

        if (!thisgadget)
        {
            intrequest_freegadgets(gadgetlist, IntuitionBase);
            return NULL;
        }

        SetAttrsA(thisgadget, gad2_tags);

        if ((currentgadget + 1) != dims->gadgets)
        {
            xoffset += dims->gadgetwidth +
                       restwidth / (dims->gadgets - currentgadget - 1);
            restwidth -= restwidth / (dims->gadgets - currentgadget - 1);
        }
    }

    return gadgetlist;
}

/**********************************************************************************************/

static void ReqITextSize(struct Screen *scr, struct IntuiText *itext,
                         WORD *width, WORD *height,
                         struct IntuitionBase *IntuitionBase)
{
    struct GfxBase *GfxBase = GetPrivIBase(IntuitionBase)->GfxBase;
    WORD w, h;

    *width  = 0;
    *height = 0;

    while(itext)
    {
        w = TextLength(&scr->RastPort, itext->IText, strlen(itext->IText));
        h = scr->RastPort.Font->tf_YSize;

        if (itext->LeftEdge > 0) w += itext->LeftEdge;
        if (itext->TopEdge > 0)  h += itext->TopEdge;

        if (w > *width)  *width  = w;
        if (h > *height) *height = h;

        itext = itext->NextText;
    }
}


/**********************************************************************************************/
