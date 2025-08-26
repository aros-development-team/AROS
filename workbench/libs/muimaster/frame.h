/*
    Copyright  1999, David Le Corfec.
    Copyright  2025, The AROS Development Team.
    All rights reserved.

*/

#ifndef _MUI_FRAME_H
#define _MUI_FRAME_H

#include "intuition/classusr.h"
#include <proto/graphics.h>

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

struct MUI_FrameSpec;
struct MUI_FrameCharacteristics;

/* MUI_*Spec really are ASCII strings.
 */
#if 0
struct MUI_FrameSpec
{
    UBYTE spec[7];              /* eg. "504444", "A13333" */
};
#endif

typedef enum
{
    FST_NONE,
    FST_RECT,
    FST_BEVEL,
    FST_THIN_BORDER,
    FST_THICK_BORDER,
    FST_ROUND_BEVEL,            /* 5 */
    FST_WIN_BEVEL,
    FST_ROUND_THICK_BORDER,
    FST_ROUND_THIN_BORDER,
    FST_GRAY_BORDER,
    FST_SEMIROUND_BEVEL,
    FST_ROUNDED,                /* 11 - rounded corners */
    FST_CUSTOM1,
    FST_CUSTOM2,
    FST_CUSTOM3,
    FST_CUSTOM4,
    FST_CUSTOM5,
    FST_CUSTOM6,
    FST_CUSTOM7,
    FST_CUSTOM8,
    FST_CUSTOM9,
    FST_CUSTOM10,
    FST_CUSTOM11,
    FST_CUSTOM12,
    FST_CUSTOM13,
    FST_CUSTOM14,
    FST_CUSTOM15,
    FST_CUSTOM16,
    FST_COUNT,
} FrameSpecType;

/* here values are converted from their ASCII counterparts */
struct MUI_FrameSpec_intern
{
    FrameSpecType type;
    UBYTE state;                /* 0 = up, 1 = down */
    UBYTE innerLeft;
    UBYTE innerRight;
    UBYTE innerTop;
    UBYTE innerBottom;
    UBYTE border_radius;        /* 0-15, optional */
    UBYTE border_width;         /* 0-15, optional */
};

struct MUI_RenderInfo;
struct dt_frame_image;
typedef void (*ZFDrawFunc) (struct dt_frame_image *fi,
    struct MUI_RenderInfo *mri, int globleft, int globtop, int globwidth,
    int globheight, int left, int top, int width, int height);

struct ZuneFrameGfx
{
    ZFDrawFunc draw;
    UWORD type;
    UWORD ileft;
    UWORD iright;
    UWORD itop;
    UWORD ibottom;
    struct dt_frame_image *customframe;
    BOOL noalpha;
    UWORD border_width;        /* Border width in pixels */
    UWORD border_radius;       /* Corner radius in pixels (0 = no rounded corners) */
};

/* Structure for frame information */
struct MUI_FrameCharacteristics
{
    UWORD border_width;        /* Border width in pixels */
    UWORD border_radius;       /* Corner radius in pixels (0 = no rounded corners) */
    BOOL has_rounded_corners;  /* TRUE if frame has rounded corners */
};

const struct ZuneFrameGfx *zune_zframe_get(Object *obj,
    const struct MUI_FrameSpec_intern *frameSpec);
const struct ZuneFrameGfx *zune_zframe_get_with_state(Object *obj,
    const struct MUI_FrameSpec_intern *frameSpec, UWORD state);

BOOL zune_frame_intern_to_spec(const struct MUI_FrameSpec_intern *intern,
    STRPTR spec);

BOOL zune_frame_spec_to_intern(CONST_STRPTR spec,
    struct MUI_FrameSpec_intern *intern);

/* Function to query frame characteristics */
BOOL zune_frame_get_characteristics(Object *obj, const struct MUI_FrameSpec_intern *frameSpec,
    struct MUI_FrameCharacteristics *characteristics);

/* Function to create clipping region for rounded corners */
struct Region *zune_frame_create_clip_region(int left, int top, int width, int height,
    UWORD border_radius);

/* Helper function to prepare dt_frame_image for drawing */
struct dt_frame_image *zune_frame_prepare_for_drawing(
    const struct ZuneFrameGfx *zframe,
    const struct MUI_FrameSpec_intern *framespec,
    struct dt_frame_image *temp_storage);

/* Frame parameters are now stored in dt_frame_image structure */

#endif
