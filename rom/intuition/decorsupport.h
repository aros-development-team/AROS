#ifndef DECORSUPPORT_H
#define DECORSUPPORT_H

/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Drawing helpers shared by the decoration and sysimage classes
*/

UWORD getbgpen(ULONG state, UWORD *pens);
VOID getintframesizes(struct IntuitionBase *IntuitionBase, WORD *xsize, WORD *ysize);
void drawimageframe(struct RastPort *rp, ULONG which, ULONG state, UWORD *pens,
                    WORD left, WORD top, WORD width, WORD height,
                    UWORD frameh, UWORD framev, struct IntuitionBase *IntuitionBase);
void renderimageframe(struct RastPort *rp, ULONG which, ULONG state, UWORD *pens,
                      WORD left, WORD top, WORD width, WORD height,
                      struct IntuitionBase *IntuitionBase);
void draw_thick_line(Class *cl, struct RastPort *rport,
                     LONG x1, LONG y1, LONG x2, LONG y2,
                     UWORD thickness);

#endif /* DECORSUPPORT_H */
