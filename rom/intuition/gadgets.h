#ifndef _GADGETS_H_
#define _GADGETS_H_

/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    Copyright © 2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$
*/

#ifndef EXEC_TYPES_H
#   include <exec/types.h>
#endif
#ifndef INTUITION_INTUITION_H
#   include <intuition/intuition.h>
#endif
#ifndef INTUITION_CLASSES_H
#   include <intuition/classes.h>
#endif
#ifndef INTUITION_CLASSUSR_H
#   include <intuition/classusr.h>
#endif
#ifndef INTUITION_GADGETCLASS_H
#   include <intuition/gadgetclass.h>
#endif

#define ADDREL(gad,flag,w,field) ((gad->Flags & (flag)) ?  w->field : 0)

struct BBox
{
    WORD Left, Top, Width, Height;
};

void printgadgetlabel(Class *cl, Object *o, struct gpRender *msg,
                      struct IntuitionBase *IntuitionBase);

/* Calculate the size of the Bounding Box of the gadget */
void CalcBBox (struct Window *, struct Requester *, struct Gadget *, struct BBox *);
void GetGadgetIBox(Object *o, struct GadgetInfo *gi, struct IBox *ibox);

/* Render a label */
ULONG LabelWidth (struct RastPort *, STRPTR label, ULONG len,
                  struct IntuitionBase *);
void RenderLabel (struct RastPort *, STRPTR label, ULONG len,
                  struct IntuitionBase *);

VOID drawrect(struct RastPort *rp
              , WORD x1, WORD y1
              , WORD x2, WORD y2
              , struct IntuitionBase *IntuitionBase);

void GetGadgetDomain(struct Gadget *gad, struct Screen *scr, struct Window *win,
                     struct Requester *req, struct IBox *box);

/* gadget coords relative to their domain! */

WORD GetGadgetLeft(struct Gadget *gad, struct Screen *scr, struct Window *win, struct Requester *req);
WORD GetGadgetTop(struct Gadget *gad, struct Screen *scr, struct Window *win, struct Requester *req);
WORD GetGadgetWidth(struct Gadget *gad, struct Screen *scr, struct Window *win, struct Requester *req);
WORD GetGadgetHeight(struct Gadget *gad, struct Screen *scr, struct Window *win, struct Requester *req);

/* gadget box in screen coords */
void GetScrGadgetIBox(struct Gadget *gad, struct Screen *scr, struct Window *win,
                      struct Requester *req, struct IBox *box);

/* gadget box relative to upper left window edge */
void GetWinGadgetIBox(struct Gadget *gad, struct Screen *scr, struct Window *win,
                      struct Requester *req, struct IBox *box);

/* gadget box in domain coords */
void GetDomGadgetIBox(struct Gadget *gad, struct Screen *scr, struct Window *win,
                      struct Requester *req, struct IBox *box);

/* gadget bounds (or box if not GMORE_BOUNDS) in screen coords */
void GetScrGadgetBounds(struct Gadget *gad, struct Screen *scr, struct Window *win,
                        struct Requester *req, struct IBox *box);

/* gadget bounds (or box if not GMORE_BOUNDS) relative to upper left window edge */
void GetWinGadgetBounds(struct Gadget *gad, struct Screen *scr, struct Window *win,
                        struct Requester *req, struct IBox *box);

/* gadget bounds (or box if not GMORE_BOUNDS)in domain coords */
void GetDomGadgetBounds(struct Gadget *gad, struct Screen *scr, struct Window *win,
                        struct Requester *req, struct IBox *box);

void EraseRelGadgetArea(struct Window *win, struct Rectangle *clipto,
    	    	    	BOOL onlydamagelist, struct IntuitionBase *IntuitionBase);
			
void RenderDisabledPattern(struct RastPort *rp, struct DrawInfo *dri, WORD x1, WORD y1,
                           WORD x2, WORD y2, struct IntuitionBase *IntuitionBase);

/* GetGadgetState returns IDS_? */
ULONG GetGadgetState(struct Window *window, struct Gadget *gadget);

#endif /* _GADGETS_H_ */

