#ifndef _GADGETS_H_
#define _GADGETS_H_
/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc: Macros and stuff for Gadgets
    Lang: english
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

struct BBox
{
    WORD Left, Top, Width, Height;
};

void printgadgetlabel(Class *cl, Object *o, struct gpRender *msg);

/* Calculate the size of the Bounding Box of the gadget */
void CalcBBox (struct Window *, struct Gadget *, struct BBox *);
void GetGadgetIBox(Object *o, struct GadgetInfo *gi, struct IBox *ibox);

/* Render a label */
ULONG LabelWidth (struct RastPort *, STRPTR label, ULONG len,
		struct IntuitionBase *);
void RenderLabel (struct RastPort *, STRPTR label, ULONG len,
		struct IntuitionBase *);

#endif /* _GADGETS_H_ */

