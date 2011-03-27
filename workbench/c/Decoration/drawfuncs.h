/*
    Copyright  2011, The AROS Development Team.
    $Id$
*/

#ifndef DRAWFUNCS_H
#define DRAWFUNCS_H

#include <graphics/rastport.h>

#include "newimage.h"

void DrawPartImageToRP(struct RastPort *rp, struct NewImage *ni, UWORD x, UWORD y, UWORD sx, UWORD sy, UWORD sw, UWORD sh);
void DrawPartToImage(struct NewImage *src, struct NewImage *dest, UWORD sx, UWORD sy, UWORD sw, UWORD sh, UWORD dx, UWORD dy);
void DrawTileToRP(struct RastPort *rp, struct NewImage *ni, ULONG color, UWORD offx, UWORD offy, UWORD x, UWORD y, WORD w, WORD h);
void DrawTileToRPRoot(struct RastPort *rp, struct NewImage *ni, ULONG color, UWORD offx, UWORD offy, UWORD x, UWORD y, WORD w, WORD h);
void DrawStatefulGadgetImageToRP(struct RastPort *rp, struct NewImage *ni, ULONG state, UWORD xp, UWORD yp);

void FillPixelArrayGradient(LONG pen, BOOL tc, struct RastPort *rp, LONG xt, LONG yt, LONG xb, LONG yb, LONG xp, LONG yp, LONG w, LONG h, ULONG start_rgb, ULONG end_rgb, LONG angle);
void FillPixelArrayGradientDelta(LONG pen, BOOL tc, struct RastPort *rp, LONG xt, LONG yt, LONG xb, LONG yb, LONG xp, LONG yp, LONG w, LONG h, ULONG start_rgb, ULONG end_rgb, LONG angle, LONG dx, LONG dy);

void RenderBackground(struct NewImage *pic, struct NewImage *texture, UWORD ratio);
void ShadeLine(LONG pen, BOOL tc, BOOL usegradients, struct RastPort *rp, struct NewImage *ni, ULONG basecolor, UWORD fact, UWORD _offy, UWORD x0, UWORD y0, UWORD x1, UWORD y1);
void SetImageTint(struct NewImage *dst, UWORD ratio, ULONG argb);
void TileMapToBitmap(struct NewImage *src, struct BitMap *map, UWORD dw, UWORD dh);

void WriteAlphaPixelArray(struct NewImage *src, struct NewLUT8Image *dst, LONG sx, LONG sy, LONG dx, LONG dy, LONG w, LONG h);
LONG WriteTiledImageTitle(BOOL fill, struct Window *win, struct RastPort *rp, struct NewImage *ni, LONG sx, LONG sy, LONG sw, LONG sh, LONG xp, LONG yp, LONG dw, LONG dh);
LONG WriteTiledImageVertical(struct RastPort *rp, struct NewImage *ni, ULONG subimage, LONG sy, LONG sh, LONG xp, LONG yp, LONG dh);
LONG WriteTiledImageHorizontal(struct RastPort *rp, struct NewImage *ni, ULONG subimage, LONG sx, LONG sw, LONG xp, LONG yp, LONG dw);
LONG WriteTiledImage(struct Window *win, struct RastPort *rp, struct NewImage *ni, LONG sx, LONG sy, LONG sw, LONG sh, LONG xp, LONG yp, LONG dw, LONG dh);


void PutImageToRP(struct RastPort *rp, struct NewImage *ni, UWORD x, UWORD y);
struct NewImage *GetImageFromRP(struct RastPort *rp, UWORD x, UWORD y, UWORD w, UWORD h);

#endif
