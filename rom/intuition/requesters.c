/*
    (C) 1997 AROS - The Amiga Research OS
    $Id$

    Desc: Miscellaneous requester functions
    Lang: english
*/

#include "intuition_intern.h"
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/boopsi.h>
#include <exec/types.h>
#include <intuition/intuition.h>


/* free the array of gadgetlabels made in BuildEasyRequestArgs() */
void easyrequest_freelabels(STRPTR *gadgetlabels)
{
    FreeVec(gadgetlabels[0]);
    FreeVec(gadgetlabels);
}


/* free the gadgets made in BuildEasyRequestArgs() */
void easyrequest_freegadgets(struct Gadget *gadgets)
{
    struct Image *frame = gadgets->GadgetRender;

    while (gadgets)
    {
         struct Gadget* nextgadget = gadgets->NextGadget;
         DisposeObject(gadgets);
         gadgets = nextgadget;
    }
    DisposeObject(frame);
}
