#ifndef BITMAP_H_
#define BITMAP_H_

/*
    Copyright © 2004-2007, The AROS Development Team. All rights reserved.
    $Id$
*/


#include "ati.h"

void BitmapInit(struct ati_staticdata *sd);
ULONG BitmapAlloc(struct ati_staticdata *sd, ULONG size);
void BitmapFree(struct ati_staticdata *sd, ULONG ptr, ULONG size);

#endif /*BITMAP_H_*/
