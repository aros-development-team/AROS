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
    ULONG  *data; /* Buffer of RAW ARGB data */
    UWORD   w;
    UWORD   h;

    ULONG   subimagescols;  /* Number of columns of subimages in image, ie. states of gadget*/
    ULONG   subimagesrows;  /* Number of rows of subimages in image, ie. states of gadget*/


    BOOL    istiled;
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

struct NewImage *NewImageContainer(UWORD w, UWORD h);
void DisposeImageContainer(struct NewImage *ni);

struct NewImage *GetImageFromFile(STRPTR path, STRPTR name,
    ULONG expectedsubimagescols, ULONG expectedsubimagesrows);
void RemoveLUTImage(struct NewImage *ni);
void SetImage(struct NewImage *in, struct NewImage *out, BOOL truecolor, struct Screen* scr);

struct NewLUT8Image *NewLUT8ImageContainer(UWORD w, UWORD h);
void DisposeLUT8ImageContainer(struct NewLUT8Image *ni);
struct Region *RegionFromLUT8Image(int w, int h, struct NewLUT8Image *s);

#endif
