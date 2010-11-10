#ifndef _NOUVEAU_COMPOSING_H
#define _NOUVEAU_COMPOSING_H
/*
    Copyright (C) 2010, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <proto/oop.h>

BOOL Composing_TopBitMapChanged(OOP_Object * newTopBitMap);
BOOL Composing_BitMapPositionChanged(OOP_Object * bm);

#endif /* _NOUVEAU_INTERN_H */
