/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    Copyright © 2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$
 
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
#include "intuition_intern.h"
#include "requesters.h"

#define DEBUG_REQUESTER(x)  ;

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

        GetAttr(GA_Next,gadgets,(ULONG*)&nextgadget);
        DisposeObject(gadgets);
        gadgets = nextgadget;
    }
    
    DisposeObject(frame);
}


/*****************************************************************************/


/* render a standard requester */
void render_requester(struct Requester *requester, struct IntuitionBase *IntuitionBase)
{
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
