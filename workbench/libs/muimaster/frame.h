/* 
    Copyright © 1999, David Le Corfec.
    Copyright © 2002, The AROS Development Team.
    All rights reserved.

    $Id$
*/

#ifndef _MUI_FRAME_H
#define _MUI_FRAME_H

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

struct MUI_FrameSpec;

/* MUI_*Spec really are ASCII strings.
 */
#if 0
struct MUI_FrameSpec
{
    UBYTE spec[7]; /* eg. "504444", "A13333" */
};
#endif

typedef enum {
    FST_NONE,
    FST_RECT,
    FST_BEVEL,
    FST_THIN_BORDER,
    FST_THICK_BORDER,
    FST_ROUND_BEVEL, /* 5 */
    FST_WIN_BEVEL,
    FST_ROUND_THICK_BORDER,
    FST_ROUND_THIN_BORDER,
    FST_GRAY_BORDER,
    FST_SEMIROUND_BEVEL,
    FST_COUNT,
} FrameSpecType;

/* here values are converted from their ASCII counterparts
 */
struct MUI_FrameSpec_intern
{
    FrameSpecType type;
    UBYTE state;           /* 0 = up, 1 = down */
    UBYTE innerLeft;
    UBYTE innerRight;
    UBYTE innerTop;
    UBYTE innerBottom;
};


struct MUI_RenderInfo;

typedef void (*ZFDrawFunc)(struct MUI_RenderInfo *mri,
			   int left, int top, int width, int height);


struct ZuneFrameGfx {
    ZFDrawFunc draw;
    UWORD  ileft;
    UWORD  iright;
    UWORD  itop;
    UWORD  ibottom;
};

const struct ZuneFrameGfx *zune_zframe_get (const struct MUI_FrameSpec_intern *frameSpec);
const struct ZuneFrameGfx *zune_zframe_get_with_state (const struct MUI_FrameSpec_intern *frameSpec,
			  			       UWORD state);

BOOL zune_frame_intern_to_spec (const struct MUI_FrameSpec_intern *intern,
				STRPTR spec);
BOOL zune_frame_spec_to_intern(CONST_STRPTR spec,
			       struct MUI_FrameSpec_intern *intern);

#endif

