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

struct BBox
{
    WORD Left, Top, Width, Height;
};

/* Calculate the size of the Bounding Box of the gadget */
void CalcBBox (struct Window *, struct Gadget *, struct BBox *);

#endif /* _GADGETS_H_ */

