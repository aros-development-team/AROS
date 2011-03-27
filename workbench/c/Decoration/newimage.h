/*
    Copyright  2011, The AROS Development Team.
    $Id$
*/

#ifndef NEWIMAGE_H
#define NEWIMAGE_H

#include <exec/types.h>
#include <intuition/intuition.h>
#include <graphics/gfx.h>

struct NewImage
{
    UWORD   w;
    UWORD   h;
    BOOL    istiled;
    ULONG  *data;
    UWORD   tile_left, tile_top, tile_bottom, tile_right;
    UWORD   inner_left, inner_top, inner_bottom, inner_right;
    APTR    mask;
    Object  *o;
    struct  BitMap  *bitmap;
    BOOL    ok;
    STRPTR  filename;
};

struct  NewLUT8Image
{
    UWORD   w;
    UWORD   h;
    UBYTE  *data;
};

struct NewLUT8Image *NewLUT8ImageContainer(UWORD w, UWORD h);
struct Region *RegionFromLUT8Image(int w, int h, struct NewLUT8Image *s);
void DisposeLUT8ImageContainer(struct NewLUT8Image *ni);

#endif
