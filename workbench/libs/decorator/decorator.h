/*
    Copyright (C) 2024-2026, The AROS Development Team. All rights reserved.

    Desc: decorator.library public header

    decorator.library is the generic decoration render engine: image
    containers, rendering primitives and element descriptors with
    compositing support. It knows nothing about themes - see
    decortheme.library for building element sets from a theme
    configuration.
*/

#ifndef LIBRARIES_DECORATOR_H
#define LIBRARIES_DECORATOR_H

#include <exec/types.h>
#include <intuition/intuition.h>
#include <intuition/classes.h>
#include <graphics/gfx.h>
#include <graphics/rastport.h>

/* ========== Gradient Specification ========== */

struct GradientSpec
{
    LONG    gs_xt, gs_yt, gs_xb, gs_yb;    /* Gradient bounds (top-left to bottom-right) */
    LONG    gs_xp, gs_yp;                   /* Rendering position */
    LONG    gs_w, gs_h;                     /* Rendering size */
    ULONG   gs_StartRGB;                    /* Start color */
    ULONG   gs_EndRGB;                      /* End color */
    LONG    gs_Angle;                       /* Gradient angle */
    LONG    gs_dx, gs_dy;                   /* Position offset */
};

/* ========== Image Containers ========== */

struct TileInfo
{
    UWORD TileLeft;
    UWORD TileRight;
    UWORD TileTop;
    UWORD TileBottom;
};

struct DecorImage
{
    ULONG  *data;
    UWORD   w;
    UWORD   h;
    BOOL    ok;

    ULONG   subimagescols;
    ULONG   subimagesrows;
    BOOL   *subimageinbm;

    Object  *o;
    APTR    mask;
    struct  BitMap  *bitmap;
    STRPTR  filename;

    struct  BitMap  *bitmap2;
};

struct DecorImageLUT8
{
    UWORD   w;
    UWORD   h;
    UBYTE  *data;
};

/* ========== DecoratorElement ========== */

/* Element rendering types */
#define DE_TYPE_NONE                0   /* Uninitialized/unused */
#define DE_TYPE_STATEFUL_GADGET     1   /* Multi-state gadget image (normal/selected/disabled/inactive) */
#define DE_TYPE_TILED_HORIZONTAL    2   /* Tiled horizontally using offset+size from composite */
#define DE_TYPE_TILED_VERTICAL      3   /* Tiled vertically using offset+size from composite */
#define DE_TYPE_SCALED_TILED_H      4   /* Vertically scaled, horizontally tiled */
#define DE_TYPE_TILED_TITLE         5   /* Title-bar style tiling */
#define DE_TYPE_IMAGE               6   /* Plain image blit */
#define DE_TYPE_TILED_BOTH          7   /* Tiled in both directions */

/* Element flags */
#define DEF_THREE_STATE     (1 << 0)   /* 3-state gadget (normal/selected/inactive) vs 4-state */
#define DEF_TITLE_GADGET    (1 << 1)   /* This is a title bar gadget */
#define DEF_SCALABLE        (1 << 2)   /* Can be scaled to fit */
#define DEF_HAS_GRIPPER     (1 << 3)   /* Has a central gripper section (scrollbar knobs) */
#define DEF_FILL            (1 << 4)   /* In an element chain, shares the leftover space */

struct DecoratorElement
{
    struct DecorImage *de_Image;          /* Source image */
    UWORD   de_Type;                    /* DE_TYPE_xxx rendering type */
    UWORD   de_Flags;                   /* DEF_xxx flags */
    UWORD   de_SubImageCols;            /* Subimage columns in source */
    UWORD   de_SubImageRows;            /* Subimage rows in source */
    LONG    de_SrcOffset;               /* Offset in composite image (_o config value) */
    LONG    de_SrcSize;                 /* Size of element section (_s config value) */
    LONG    de_SrcHeight;               /* Source height for vertically scaled tiling (0 = subimage height) */
    WORD    de_PadX;                    /* Extra X offset when rendering */
    WORD    de_PadY;                    /* Extra Y offset when rendering */
};

#endif /* LIBRARIES_DECORATOR_H */
