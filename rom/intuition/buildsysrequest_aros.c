/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    Copyright © 2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$
*/

#define	DEBUG_BUILDSYSREQUEST(x)	x;

/**********************************************************************************************/

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
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

#define OUTERSPACING_X 		4
#define OUTERSPACING_Y 		4
#define GADGETGADGETSPACING 	8
#define TEXTGADGETSPACING 	4
#define TEXTBOXBORDER_X 	16
#define TEXTBOXBORDER_Y 	4
#define BUTTONBORDER_X 		8
#define BUTTONBORDER_Y 		4

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

/*****************************************************************************
 
    NAME */
#include <proto/intuition.h>
#include <exec/types.h>
#include <intuition/intuition.h>

AROS_LH7(struct Window *, BuildSysRequest,

         /*  SYNOPSIS */
         AROS_LHA(struct Window *   , window, A0),
         AROS_LHA(struct IntuiText *, bodytext, A1),
         AROS_LHA(struct IntuiText *, postext, A2),
         AROS_LHA(struct IntuiText *, negtext, A3),
         AROS_LHA(ULONG             , IDCMPFlags , D0),
         AROS_LHA(WORD              , width, D2),
         AROS_LHA(WORD              , height, D3),

         /*  LOCATION */
         struct IntuitionBase *, IntuitionBase, 60, Intuition)

/*  FUNCTION
 
    INPUTS
	window - The window in which the requester will appear
	bodytext - The Text to be shown in the body of the requester
	postext - The Text to be shown in the positive choice gadget
	negtext - The Text to be shown in the negative choice gadget
	IDCMPFlags - The IDCMP Flags for this requester
	width, height - The dimensions of the requester
 
    RESULT
 
    NOTES
 
    EXAMPLE
 
    BUGS
 
    SEE ALSO
	FreeSysRequest(), DisplayAlert(), ModifyIDCMP(), exec-library/Wait(),
	Request(), AutoRequest(), EasyRequest(), BuildEasyRequestArgs()
 
    INTERNALS
 
    HISTORY
 
*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)

    struct Screen               *scr = NULL, *lockedscr = NULL;
    struct Window               *req;
    struct Gadget               *gadgets;
    STRPTR                       reqtitle;
    STRPTR                       gadgetlabels[3];
    struct                       sysreqdims dims;
    struct IntRequestUserData   *requserdata;

    DEBUG_BUILDSYSREQUEST(dprintf("intrequest_buildsysrequest: window 0x%lx body <%s> postext <%s> negtext <%s> IDCMPFlags 0x%lx width %ld height %ld\n",
                                  (ULONG) window,
                                  bodytext ? (char *) bodytext->IText : "<NULL>",
                                  (postext && postext->IText) ? (char *) postext->IText : "<NULL>",
                                  (negtext && negtext->IText) ? (char *) negtext->IText : "<NULL>",
                                  IDCMPFlags,
                                  (LONG) width,
                                  (LONG) height));

    /* negtext and bodytest must be specified, postext is optional */
    if (!negtext || !bodytext) return NULL;

    /* get requester title */

    reqtitle = NULL;
    if (window) reqtitle = window->Title;
    if (!reqtitle) reqtitle = "System Request"; /* stegerg: should be localized */

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

    /* create everything */

    if (buildsysreq_calculatedims(&dims, scr,
                                  bodytext, gadgetlabels, IntuitionBase))
    {
        gadgets = buildsysreq_makegadgets(&dims, gadgetlabels, scr, IntuitionBase);
        DEBUG_BUILDSYSREQUEST(dprintf("intrequest_buildsysrequest: gadgets 0x%lx\n", (ULONG) gadgets));
        if (gadgets)
        {
            requserdata = AllocVec(sizeof(struct IntRequestUserData),
                                   MEMF_ANY|MEMF_CLEAR);
            DEBUG_BUILDSYSREQUEST(dprintf("intrequest_buildsysrequest: requserdata 0x%lx\n", (ULONG) requserdata));
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
                DEBUG_BUILDSYSREQUEST(dprintf("intrequest_buildsysrequest: req 0x%lx\n", (ULONG) req));
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
                    DEBUG_BUILDSYSREQUEST(dprintf("intrequest_buildsysrequest: gadgets 0x%lx\n", (ULONG) gadgets));

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

    AROS_LIBFUNC_EXIT

} /* BuildSysRequest */

/**********************************************************************************************/

/* draw the contents of the requester */
static void buildsysreq_draw(struct sysreqdims *dims, struct IntuiText *itext,
                             struct Window *req, struct Screen *scr,
                             struct Gadget *gadgets,
                             struct IntuitionBase *IntuitionBase)
{
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

    LONG  currentgadget = 0;
    WORD  itextwidth, itextheight;
    UWORD textboxwidth = 0, gadgetswidth; /* width of upper/lower part */

    /* calculate height of requester */
    dims->fontheight = scr->RastPort.Font->tf_YSize;

    ReqITextSize(scr, itext, &itextwidth, &itextheight, IntuitionBase);

    dims->height = scr->WBorTop + dims->fontheight + 1 +
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
    if (textboxwidth > gadgetswidth)
    {
        dims->width = textboxwidth;
    }
    else
    {
        dims->itextleft += (gadgetswidth - textboxwidth) / 2;
        dims->width = gadgetswidth;
    }

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
    SetDrMd(rp, JAM1);
    SetAPen(rp, dri->dri_Pens[TEXTPEN]);

    while(itext)
    {
        Move(rp, x + itext->LeftEdge,
             y + itext->TopEdge + scr->RastPort.Font->tf_Baseline);
        Text(rp, itext->IText, strlen(itext->IText));

        itext = itext->NextText;
    }
}

/**********************************************************************************************/

