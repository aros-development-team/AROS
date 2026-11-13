/*
    Copyright (C) 1995-2023, The AROS Development Team. All rights reserved.
    Copyright (C) 2001-2003, The MorphOS Development Team. All Rights Reserved.
 
    Miscellaneous requester functions.
*/

#include <proto/graphics.h>
#include <proto/layers.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/cybergraphics.h>
#include <exec/types.h>
#include <intuition/intuition.h>
#include <intuition/gadgetclass.h>
#include <intuition/imageclass.h>
#include <graphics/rpattr.h>
#include <graphics/gfxmacros.h>
#include "intuition_intern.h"
#include "requesters.h"


/**********************************************************************************************/

CONST UWORD BgPattern[2]  = { 0xAAAA, 0x5555 };

/**********************************************************************************************/

/* free the array of gadgetlabels made in BuildEasyRequestArgs() */
void intrequest_freelabels(STRPTR *gadgetlabels, struct IntuitionBase *IntuitionBase)
{
    if (gadgetlabels)
    {
        FreeVec(gadgetlabels[0]);
        FreeVec(gadgetlabels);
    }
}


/* free the gadgets made in BuildEasyRequestArgs() */
void intrequest_freegadgets(struct Gadget *gadgets, struct IntuitionBase *IntuitionBase)
{
    struct Image *frame = gadgets->GadgetRender;

    while (gadgets)
    {
        struct Gadget* nextgadget = 0;

        GetAttr(GA_Next, (Object *)gadgets, (IPTR *)&nextgadget);
        DisposeObject(gadgets);
        gadgets = nextgadget;
    }
    
    DisposeObject(frame);
}

/**********************************************************************************************/

static void ReqPrintIText(struct Screen *scr, struct DrawInfo *dri,
                          struct RastPort *rp, struct IntuiText *itext, WORD x, WORD y,
                          struct IntuitionBase *IntuitionBase)
{
    struct GfxBase *GfxBase = GetPrivIBase(IntuitionBase)->GfxBase;

    SetABPenDrMd(rp,
                 dri->dri_Pens[TEXTPEN], dri->dri_Pens[BACKGROUNDPEN], JAM1);

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

/*****************************************************************************/

/* render a standard requester */
void render_requester(struct Requester *requester, struct IntuitionBase *IntuitionBase)
{
    struct GfxBase *GfxBase = GetPrivIBase(IntuitionBase)->GfxBase;
    struct RastPort *rp = requester->ReqLayer->rp;

    if ((requester->Flags & NOREQBACKFILL) == 0)
        SetRast(rp, requester->BackFill);

    if (requester->ImageBMap && requester->Flags & PREDRAWN)
        BltBitMapRastPort(requester->ImageBMap, 0, 0,
                          rp, 0, 0, requester->Width, requester->Height, 0xc0);

    if (requester->ReqImage && requester->Flags & USEREQIMAGE)
        DrawImage(rp, requester->ReqImage, 0, 0);

    if (requester->ReqBorder)
        DrawBorder(rp, requester->ReqBorder, 0, 0);

    if (requester->ReqGadget)
        RefreshGList(requester->ReqGadget, requester->RWindow, requester, -1);

    if (requester->ReqText)
        PrintIText(rp, requester->ReqText, 0, 0);
}

/*****************************************************************************/

/* render a built requester */
void buildreq_draw(struct IntReqDims *dims, struct IntuiText *itext,
                             struct Window *req, struct Screen *scr,
                             struct Gadget *gadgets,
                             struct IntuitionBase *IntuitionBase)
{
    struct LayersBase *LayersBase = GetPrivIBase(IntuitionBase)->LayersBase;
    struct GfxBase *GfxBase = GetPrivIBase(IntuitionBase)->GfxBase;
    struct TagItem   frame_tags[] =
    {
        {IA_Left        , req->BorderLeft + OUTERSPACING_X                                      },
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
    struct Region   *clipreg, *origreg;
    struct Rectangle cliprect;

    dri = GetScreenDrawInfo(scr);
    if (!dri)
        return;

    SetFont(req->RPort, dri->dri_Font);

    /* render background pattern */
    SetABPenDrMd(req->RPort,
                 dri->dri_Pens[SHINEPEN], dri->dri_Pens[BACKGROUNDPEN],
                 JAM1);
    SetAfPt(req->RPort, BgPattern, 1);
    RectFill(req->RPort, req->BorderLeft,
             req->BorderTop,
             req->Width - req->BorderRight,
             req->Height - req->BorderBottom);
    SetAfPt(req->RPort, NULL, 0);

    /* render textframe */
    frame = (struct Image *)NewObjectA(NULL, FRAMEICLASS, frame_tags);
    if (frame)
    {
        DrawImageState(req->RPort, frame, 0, 0, IDS_NORMAL, dri);
        DisposeObject((Object *)frame);
    }

    cliprect.MinX = req->BorderLeft + OUTERSPACING_X;
    cliprect.MinY = req->BorderTop + OUTERSPACING_Y + 1;
    cliprect.MaxX = req->Width - req->BorderRight - OUTERSPACING_X - 1;
    cliprect.MaxY = cliprect.MinY + dims->disptextheight - 4;
    if (clipreg = NewRegion()) {
      OrRectRegion(clipreg, &cliprect);
      origreg = InstallClipRegion(req->WLayer, clipreg);
    }

    /* render text */
    ReqPrintIText(scr, dri, req->RPort, itext,
                  dims->textleft - dims->offx, req->BorderTop + OUTERSPACING_Y + TEXTBOXBORDER_Y - dims->offy,
                  IntuitionBase);

    if (clipreg) {
      InstallClipRegion(req->WLayer, origreg);
      DisposeRegion(clipreg);
    }

    /* render gadgets */
    RefreshGList(gadgets, req, NULL, -1L);

    FreeScreenDrawInfo(scr, dri);
}
