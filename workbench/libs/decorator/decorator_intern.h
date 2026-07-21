/*
    Copyright (C) 2024-2026, The AROS Development Team. All rights reserved.

    Desc: decorator.library internal header
*/

#ifndef DECORATOR_INTERN_H
#define DECORATOR_INTERN_H

#include <exec/types.h>
#include <exec/libraries.h>
#include <libraries/decorator.h>

struct DecoratorBase
{
    struct Library          db_Lib;
};

/* ========== ARGB pixel access ========== */
/* DecorImage pixel data is stored as big-endian ARGB, read as native ULONGs */

#if AROS_BIG_ENDIAN
#define GET_ARGB_A(rgb) ((rgb >> 24) & 0xff)
#define GET_ARGB_R(rgb) ((rgb >> 16) & 0xff)
#define GET_ARGB_G(rgb) ((rgb >> 8) & 0xff)
#define GET_ARGB_B(rgb) (rgb & 0xff)
#define SET_ARGB(a, r, g, b) (a << 24 | r << 16 | g << 8 | b)
#define GET_ARCH_A GET_ARGB_A
#define GET_ARCH_R GET_ARGB_R
#define GET_ARCH_G GET_ARGB_G
#define GET_ARCH_B GET_ARGB_B
#else
#define GET_ARGB_A(rgb) (rgb & 0xff)
#define GET_ARGB_R(rgb) ((rgb >> 8) & 0xff)
#define GET_ARGB_G(rgb) ((rgb >> 16) & 0xff)
#define GET_ARGB_B(rgb) ((rgb >> 24) & 0xff)
#define GET_ARCH_A GET_ARGB_B
#define GET_ARCH_R GET_ARGB_G
#define GET_ARCH_G GET_ARGB_R
#define GET_ARCH_B GET_ARGB_A
#define SET_ARGB(a, r, g, b) (b << 24 | g << 16 | r << 8 | a)
#endif

/* ========== Row-level pixel kernels (pixops.c) ========== */
/* These are the hot inner loops of the rendering support code, operating
   on rows of 32bit ARGB pixels. Generic C implementations live in
   pixops.c; architectures may provide optimized (e.g. SIMD) replacements
   by building an arch specific version of that file (see
   arch/x86_64-all/decorator/). Replacements must produce identical
   output to the generic implementations. */

/* Fill n pixels with px */
void pixop_splat_row(ULONG *d, LONG n, ULONG px);
/* Extract the alpha channel of n pixels into a LUT8 buffer */
void pixop_extract_alpha_row(UBYTE *d, const ULONG *s, LONG n);
/* Blend n pixels towards the RGB colour in argb by ratio (0-255), forcing alpha opaque */
void pixop_tint_row(ULONG *d, LONG n, ULONG argb, UWORD ratio);
/* Blend n source pixels over destination by ratio (0-255); when tiled is
   set additionally blend by source alpha. Result alpha = source alpha */
void pixop_mix_row(ULONG *d, const ULONG *s, LONG n, UWORD ratio, BOOL tiled);
/* d[i] = s[i] with all four channels multiplied by fact/256, saturated */
void pixop_shade_row(ULONG *d, const ULONG *s, LONG n, UWORD fact);
/* 14-tap neighbourhood blur of row r0 using the two rows above (rm2, rm1)
   and below (rp1, rp2); when tex is non-NULL the blurred result is mixed
   with the original pixel by the inverted texture alpha and the result
   alpha is set to that inverted alpha, otherwise alpha is forced opaque */
void pixop_blur14mix_row(ULONG *d, const ULONG *rm2, const ULONG *rm1, const ULONG *r0,
                         const ULONG *rp1, const ULONG *rp2, const ULONG *tex, LONG w);

/* ========== Internal function declarations ========== */
/* These are the non-D-prefixed internal implementations called
   within the library. The D-prefixed AROS_LH wrappers delegate to these. */

/* newimagefuncs.c */
struct DecorImage *NewImageContainer(UWORD w, UWORD h);
struct DecorImage *ScaleNewImage(struct DecorImage *oni, UWORD neww, UWORD newh);
void DisposeImageContainer(struct DecorImage *ni);
struct DecorImage *GetImageFromFile(STRPTR path, STRPTR name, ULONG expectedsubimagescols, ULONG expectedsubimagesrows);
struct DecorImage *CreateNewImageContainerMatchingScreen(struct DecorImage *in, BOOL truecolor, struct Screen *scr);
struct DecorImageLUT8 *NewLUT8ImageContainer(UWORD w, UWORD h);
void DisposeLUT8ImageContainer(struct DecorImageLUT8 *ni);
struct Region *RegionFromLUT8Image(int w, int h, struct DecorImageLUT8 *s);
ULONG *ScaleBuffer(ULONG *srcdata, LONG widthBuffer, LONG widthSrc, LONG heightSrc, LONG widthDest, LONG heightDest);

/* drawfuncs.c */
void DrawPartImageToRP(struct RastPort *rp, struct DecorImage *ni, UWORD x, UWORD y, UWORD sx, UWORD sy, UWORD sw, UWORD sh);
void DrawPartToImage(struct DecorImage *src, struct DecorImage *dest, UWORD sx, UWORD sy, UWORD sw, UWORD sh, UWORD dx, UWORD dy);
void DrawStatefulGadgetImageToRP(struct RastPort *rp, struct DecorImage *ni, ULONG state, UWORD xp, UWORD yp);
void DrawScaledStatefulGadgetImageToRP(struct RastPort *rp, struct DecorImage *ni, ULONG state, UWORD xp, UWORD yp, WORD scaledwidth, WORD scaledheight);
void HorizVertRepeatNewImage(struct DecorImage *ni, ULONG color, UWORD offx, UWORD offy, struct RastPort *rp, UWORD x, UWORD y, WORD w, WORD h);
void HorizRepeatBuffer(UBYTE *buf, LONG offy, LONG pen, BOOL tc, struct RastPort *rp, LONG x, LONG y, LONG w, LONG h);
void FillPixelArrayGradient(LONG pen, BOOL tc, struct RastPort *rp, LONG xt, LONG yt, LONG xb, LONG yb, LONG xp, LONG yp, LONG w, LONG h, ULONG start_rgb, ULONG end_rgb, LONG angle, LONG dx, LONG dy);
void FillMemoryBufferRGBGradient(UBYTE *buf, LONG pen, LONG xt, LONG yt, LONG xb, LONG yb, LONG xp, LONG yp, LONG w, LONG h, ULONG start_rgb, ULONG end_rgb, LONG angle);
void ShadeLine(LONG pen, BOOL tc, BOOL usegradients, struct RastPort *rp, struct DecorImage *ni, ULONG basecolor, UWORD fact, UWORD offy, UWORD x0, UWORD y0, UWORD x1, UWORD y1);
void SetImageTint(struct DecorImage *dst, UWORD ratio, ULONG argb);
void TileMapToBitmap(struct DecorImage *src, struct TileInfo *srcti, struct BitMap *map, UWORD dw, UWORD dh);
void WriteAlphaPixelArray(struct DecorImage *src, struct DecorImageLUT8 *dst, LONG sx, LONG sy, LONG dx, LONG dy, LONG w, LONG h);
LONG WriteTiledImageTitle(BOOL fill, LONG clipw, struct RastPort *rp, struct DecorImage *ni, LONG sx, LONG sy, LONG sw, LONG sh, LONG xp, LONG yp, LONG dw, LONG dh);
LONG WriteTiledImageVertical(struct RastPort *rp, struct DecorImage *ni, ULONG subimage, LONG sy, LONG sh, LONG xp, LONG yp, LONG dh);
LONG WriteTiledImageHorizontal(struct RastPort *rp, struct DecorImage *ni, ULONG subimage, LONG sx, LONG sw, LONG xp, LONG yp, LONG dw);
LONG WriteVerticalScaledTiledImageHorizontal(struct RastPort *rp, struct DecorImage *ni, ULONG subimage, LONG sx, LONG sw, LONG xp, LONG yp, LONG sh, LONG dw, LONG dh);
void PutImageToRP(struct RastPort *rp, struct DecorImage *ni, UWORD x, UWORD y);
struct DecorImage *GetImageFromRP(struct RastPort *rp, UWORD x, UWORD y, UWORD w, UWORD h);
void RenderMenuBackground(struct DecorImage *pic, struct DecorImage *texture, struct TileInfo *textureti, UWORD ratio);
void RenderMenuBarBackground(struct DecorImage *pic, struct DecorImage *texture, struct TileInfo *textureti, UWORD ratio);

/* decoratorelement.c */
LONG RenderElement(struct DecoratorElement *element, struct RastPort *rp, ULONG subimage, LONG xp, LONG yp, LONG dw, LONG dh, LONG clipw);
LONG RenderElementChain(struct DecoratorElement **elements, ULONG count, BOOL vertical, struct RastPort *rp, ULONG subimage, LONG xp, LONG yp, LONG dw, LONG dh);

#endif /* DECORATOR_INTERN_H */
