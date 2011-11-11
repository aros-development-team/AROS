/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    Copyright © 2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$
*/

#define	DEBUG_BUILDSYSREQUEST(x)
#define DEBUG_FREESYSREQUEST(x)
#define	DEBUG_SYSREQHANDLER(x)

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

extern UWORD BgPattern[];

/**********************************************************************************************/

struct sysreqdims
{
    UWORD width;       /* width of the requester */
    UWORD height;      /* height of the requester */
    UWORD fontheight;  /* height of the default font */
    UWORD itextleft;
    int   gadgets;     /* number of gadgets */
    UWORD gadgetwidth; /* width of a gadget */
};

/**********************************************************************************************/

static BOOL buildsysreq_calculatedims(struct sysreqdims *dims,
                                      struct Screen *scr,
                                      struct IntuiText *itext,
                                      STRPTR *gadgetlabels,
                                      struct IntuitionBase *IntuitionBase);
static struct Gadget *buildsysreq_makegadgets(struct sysreqdims *dims,
			        STRPTR *gadgetlabels,
			        struct Screen *scr,
			        struct IntuitionBase *IntuitionBase);
static void buildsysreq_draw(struct sysreqdims *dims, struct IntuiText *itext,
                             struct Window *win, struct Screen *scr,
                             struct Gadget *gadgets,
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
    struct                       sysreqdims dims;
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

    if (postext)
    {
        dims.gadgets = 2;

        gadgetlabels[0] = postext->IText;
        gadgetlabels[1] = negtext->IText;
        gadgetlabels[2] = NULL;
    }
    else
    {
        dims.gadgets = 1;

        gadgetlabels[0] = negtext->IText;
        gadgetlabels[1] = NULL;
    }

    /* EXPERIMENTAL: Obey user-supplied size if the text fits into it. Processed in buildsysreq_calculatedims().
       This is experimental. Currently DisplayAlert() relies on ability to specify requester size.
       Requester size actually determines inner text box size - sonic. */
    dims.width  = width;
    dims.height = height;

    /* create everything */

    if (buildsysreq_calculatedims(&dims, scr,
                                  bodytext, gadgetlabels, IntuitionBase))
    {
        gadgets = buildsysreq_makegadgets(&dims, gadgetlabels, scr, IntuitionBase);
        DEBUG_BUILDSYSREQUEST(dprintf("intrequest_buildsysrequest: gadgets 0x%p\n", gadgets));
        if (gadgets)
        {
            requserdata = AllocVec(sizeof(struct IntRequestUserData),
                                   MEMF_ANY|MEMF_CLEAR);
            DEBUG_BUILDSYSREQUEST(dprintf("intrequest_buildsysrequest: requserdata 0x%p\n", requserdata));
            if (requserdata)
            {
                struct TagItem win_tags[] =
                {
                    {WA_Width                                       , dims.width    	    	    	    	    	    	    	},
                    {WA_Height                                      , dims.height                    	    	    	    	    	},
                    {WA_Left                                        , (scr->Width/2) - (dims.width/2) 	    	    	    	    	},
                    {WA_Top                                         , (scr->Height/2) - (dims.height/2)     	    	    	    	},
                    {WA_IDCMP                                       , (IDCMP_GADGETUP | IDCMP_RAWKEY | (IDCMPFlags & ~IDCMP_VANILLAKEY))},
                    {WA_Gadgets                                     , (IPTR)gadgets                 	    	    	    	    	},
                    {WA_Title                                       , (IPTR)reqtitle                	    	    	    	    	},
                    {(lockedscr ? WA_PubScreen : WA_CustomScreen)   , (IPTR)scr                     	    	    	    	    	},
                    {WA_Flags                                       , WFLG_DRAGBAR     |
                     	    	    	    	    	    	      WFLG_DEPTHGADGET |
                     	    	    	    	    	    	      WFLG_ACTIVATE    |
                     	    	    	    	    	    	      WFLG_RMBTRAP   /*|
                                                            	      WFLG_SIMPLE_REFRESH*/         	    	    	    	    	},
                    {TAG_DONE                                                                       	    	    	    	    	}
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
                    requserdata->NumGadgets = dims.gadgets;
                    buildsysreq_draw(&dims, bodytext,
                                     req, scr, gadgets, IntuitionBase);
                    DEBUG_BUILDSYSREQUEST(dprintf("intrequest_buildsysrequest: gadgets 0x%p\n", gadgets));

                    return req;
                }

                /* opening requester failed -> free everything */
                FreeVec(requserdata);
            }
            intrequest_freegadgets(gadgets, IntuitionBase);
        }
    }

    if (lockedscr) UnlockPubScreen(NULL, lockedscr);

    return NULL;
}

/**********************************************************************************************/

LONG sysreqhandler_intern(struct Window *window, ULONG *IDCMPFlagsPtr, BOOL WaitInput, struct IntuitionBase *IntuitionBase)
{
    struct Library *KeymapBase = GetPrivIBase(IntuitionBase)->KeymapBase;
    struct Library *UtilityBase = GetPrivIBase(IntuitionBase)->UtilityBase;
    struct IntuiMessage *msg;
    LONG                 result;

    DEBUG_SYSREQHANDLER(dprintf("SysReqHandler: window 0x%lx IDCMPPtr 0x%lx WaitInput 0x%lx\n",
                                (ULONG) window,
                                (ULONG) IDCMPFlagsPtr,
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

        if (WaitInput)
        {
            WaitPort(window->UserPort);
        }
        while ((msg = (struct IntuiMessage *)GetMsg(window->UserPort)))
        {
            DEBUG_SYSREQHANDLER(dprintf("SysReqHandler: msg 0x%lx class 0x%lx\n", (ULONG) msg, msg->Class));
            switch (msg->Class)
            {
            /* we don't use VANILLA (filtered from useridcmp!) to get
            all events we need */
            case IDCMP_RAWKEY:
            {
    	    	#define RKBUFLEN 1
		
                struct InputEvent ie;
                char 	    	  rawbuffer[RKBUFLEN];
                
                ie.ie_Class 	    = IECLASS_RAWKEY;
                ie.ie_SubClass      = 0;
                ie.ie_Code  	    = msg->Code;
                ie.ie_Qualifier     = 0;
                ie.ie_EventAddress  = (APTR *) *((IPTR *)msg->IAddress);
		
                if (KeymapBase && MapRawKey(&ie,rawbuffer,RKBUFLEN,0))
                {
                    if (msg->Qualifier & IEQUALIFIER_LCOMMAND)
                    {
                        if  (ToUpper(rawbuffer[0]) == ToUpper(GetPrivIBase(IntuitionBase)->IControlPrefs.ic_ReqTrue))
                        {
                            if (((struct IntRequestUserData *)window->UserData)->NumGadgets > 1)
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
                result = ((struct Gadget *)msg->IAddress)->GadgetID;
                break;

            default:
                DEBUG_SYSREQHANDLER(dprintf("SysReqHandler: unknown IDCMP\n"));
                if (result == -2)
                {
                    if (msg->Class & ((struct IntRequestUserData *)window->UserData)->IDCMP)
                    {
                        if (IDCMPFlagsPtr) *IDCMPFlagsPtr = msg->Class;
                        result = -1;
                    }
                }
                break;
            }
            ReplyMsg((struct Message *)msg);

        } /* while ((msg = (struct IntuiMessage *)GetMsg(window->UserPort))) */

    } /* real window */

    DEBUG_SYSREQHANDLER(dprintf("SysReqHandler: Result 0x%lx\n",result));

    return result;
}

/**********************************************************************************************/

void freesysreq_intern(struct Window *window, struct IntuitionBase *IntuitionBase)
{
    struct Gadget   	    	*gadgets;
    STRPTR  	    	     	*gadgetlabels;
    struct IntRequestUserData 	*requserdata;

    DEBUG_FREESYSREQUEST(dprintf("intrequest_freesysrequest: window 0x%lx\n", (ULONG) window));

    if ((window == NULL) || (window == (void *)1L))
        return;

    requserdata = (struct IntRequestUserData *)window->UserData;

    DEBUG_FREESYSREQUEST(dprintf("intrequest_freesysrequest: requserdata 0x%lx\n", (ULONG) requserdata));

    gadgets = requserdata->Gadgets;

    DEBUG_FREESYSREQUEST(dprintf("intrequest_freesysrequest: gadgets 0x%lx\n", (ULONG) gadgets));

    /* Remove gadgets before closing window to avoid conflicts with system gadgets */
    RemoveGList(window, gadgets, requserdata->NumGadgets);

    gadgetlabels = requserdata->GadgetLabels;

    DEBUG_FREESYSREQUEST(dprintf("intrequest_freesysrequest: gadgetlabels 0x%lx\n", (ULONG) gadgetlabels));

    window->UserData = 0;
    CloseWindow(window);
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

/* draw the contents of the requester */
static void buildsysreq_draw(struct sysreqdims *dims, struct IntuiText *itext,
                             struct Window *req, struct Screen *scr,
                             struct Gadget *gadgets,
                             struct IntuitionBase *IntuitionBase)
{
    struct GfxBase *GfxBase = GetPrivIBase(IntuitionBase)->GfxBase;
    struct TagItem   frame_tags[] =
    {
        {IA_Left        , req->BorderLeft + OUTERSPACING_X  	    	    	    	    	},
        {IA_Top         , req->BorderTop + OUTERSPACING_Y                                       },
        {IA_Width       , req->Width - req->BorderLeft - req->BorderRight - OUTERSPACING_X * 2  },
        {IA_Height      , req->Height - req->BorderTop - req->BorderBottom -
            	    	  dims->fontheight - OUTERSPACING_Y * 2 -
            	    	  TEXTGADGETSPACING - BUTTONBORDER_Y * 2                                },
        {IA_Recessed    , TRUE                                                                  },
        {IA_EdgesOnly   , FALSE                                                                 },
        {TAG_DONE                                                                               }
    };
    struct DrawInfo *dri;
    struct Image    *frame;

    dri = GetScreenDrawInfo(scr);
    if (!dri)
        return;

    SetFont(req->RPort, dri->dri_Font);

    /* draw background pattern */
    SetABPenDrMd(req->RPort,
                 dri->dri_Pens[SHINEPEN], dri->dri_Pens[BACKGROUNDPEN],
                 JAM1);
    SetAfPt(req->RPort, BgPattern, 1);
    RectFill(req->RPort, req->BorderLeft,
             req->BorderTop,
             req->Width - req->BorderRight,
             req->Height - req->BorderBottom);
    SetAfPt(req->RPort, NULL, 0);

    /* draw textframe */
    frame = (struct Image *)NewObjectA(NULL, FRAMEICLASS, frame_tags);
    if (frame)
    {
        DrawImageState(req->RPort, frame, 0, 0, IDS_NORMAL, dri);
        DisposeObject((Object *)frame);
    }

    /* draw text */
    ReqPrintIText(scr, dri, req->RPort, itext,
                  dims->itextleft, req->BorderTop + OUTERSPACING_Y + TEXTBOXBORDER_Y,
                  IntuitionBase);

    /* draw gadgets */
    RefreshGList(gadgets, req, NULL, -1L);

    FreeScreenDrawInfo(scr, dri);
}

/**********************************************************************************************/

/* calculate dimensions of the requester */
static BOOL buildsysreq_calculatedims(struct sysreqdims *dims,
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
    dims->itextleft = scr->WBorLeft + OUTERSPACING_X + TEXTBOXBORDER_X;
    if (textboxwidth <= gadgetswidth)
    {
	dims->itextleft += (gadgetswidth - textboxwidth) / 2;
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
static struct Gadget *buildsysreq_makegadgets(struct sysreqdims *dims,
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
            {GA_ID          , gadgetid 	    	    	    	    },
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

static void ReqPrintIText(struct Screen *scr, struct DrawInfo *dri,
                          struct RastPort *rp, struct IntuiText *itext, WORD x, WORD y,
                          struct IntuitionBase *IntuitionBase)
{
    struct GfxBase *GfxBase = GetPrivIBase(IntuitionBase)->GfxBase;

    SetDrMd(rp, JAM1);
    SetAPen(rp, dri->dri_Pens[TEXTPEN]);

/* Experimental: obey font specified in supplied IntuiText structures.
   Makes sense because coordinates specified in these structures are taken
   into account, but i guess they are specified according to font size.
   Currently DisplayAlert() relies on this behavior - sonic.
    while(itext)
    {
        Move(rp, x + itext->LeftEdge,
             y + itext->TopEdge + scr->RastPort.Font->tf_Baseline);
        Text(rp, itext->IText, strlen(itext->IText));

        itext = itext->NextText;
    }*/
    int_PrintIText(rp, itext, x, y, TRUE, IntuitionBase);
}

/**********************************************************************************************/
