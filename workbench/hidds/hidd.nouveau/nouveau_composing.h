#ifndef _NOUVEAU_COMPOSING_H
#define _NOUVEAU_COMPOSING_H
/*
    Copyright © 2010, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <proto/oop.h>

#define ENABLE_COMPOSING 0

BOOL Composing_TopBitMapChanged(OOP_Object * bm);
BOOL Composing_BitMapPositionChanged(OOP_Object * bm);
VOID Composing_BitMapRectChanged(OOP_Object * bm, WORD x, WORD y, WORD width, WORD height);
VOID Composing_BitMapStackChanged(struct HIDD_ViewPortData * vpdata);

#endif /* _NOUVEAU_INTERN_H */
