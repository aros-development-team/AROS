#ifndef __ZUNE_FRAME__
#define __ZUNE_FRAME__

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

struct MUI_FrameSpec
{
    UWORD type;
    UWORD state;           /* 0 = up, 1 = down */
    UWORD innerLeft;
    UWORD innerRight;
    UWORD innerTop;
    UWORD innerBottom;
};

struct MUI_RenderInfo;

typedef void (*ZFDrawFunc)(struct MUI_RenderInfo *mri,
			   int left, int top, int width, int height);


struct ZuneFrameGfx {
    ZFDrawFunc draw[2];
    UWORD  xthickness;
    UWORD  ythickness;
};

struct ZuneFrameGfx *zune_zframe_get_index (int i);
struct ZuneFrameGfx *zune_zframe_get (struct MUI_FrameSpec *frameSpec);

typedef enum {
    FST_NONE,
    FST_RECT,
    FST_BEVEL,
    FST_THIN_BORDER,
    FST_THICK_BORDER,
    FST_WIN_BEVEL,
    FST_GRAY_BORDER,
    FST_SEMIROUND_BEVEL,
    FST_ROUND_THIN_BORDER,
    FST_ROUND_THICK_BORDER,
    FST_ROUND_BEVEL,
    FST_COUNT,
} FrameSpecType;

STRPTR zune_framespec_to_string (struct MUI_FrameSpec *frameSpec);
BOOL zune_string_to_framespec(STRPTR str, struct MUI_FrameSpec *frameSpec);

#endif

