#ifndef _NOUVEAU_COMPOSITING_H
#define _NOUVEAU_COMPOSITING_H
/*
    Copyright © 2010, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <proto/oop.h>

#define ENABLE_COMPOSITING 0

BOOL Compositing_TopBitMapChanged(OOP_Object * bm);
BOOL Compositing_BitMapPositionChanged(OOP_Object * bm);
VOID Compositing_BitMapRectChanged(OOP_Object * bm, WORD x, WORD y, WORD width, WORD height);
VOID Compositing_BitMapStackChanged(struct HIDD_ViewPortData * vpdata);

#endif /* _NOUVEAU_COMPOSITING_H */
