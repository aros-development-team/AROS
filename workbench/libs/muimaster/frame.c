/* 
    Copyright © 1999, David Le Corfec.
    Copyright © 2002, The AROS Development Team.
    All rights reserved.

    $Id$
*/

#include <exec/types.h>

#include <proto/graphics.h>
#include <stdio.h>

#include "muimaster_intern.h"
#include "mui.h"
#include "frame.h"

extern struct Library *MUIMasterBase;

/**************************************************************************
 no frame
**************************************************************************/
static void  frame_none_draw (struct MUI_RenderInfo *mri,
		 int left, int top, int width, int height)
{
}

/**************************************************************************
 1 : FST_RECT
**************************************************************************/
static void rect_draw(struct MUI_RenderInfo *mri,
	  int left, int top, int width, int height,
	  MPen preset_color)
{ 
    struct RastPort *rp = mri->mri_RastPort;

    SetAPen(rp, mri->mri_Pens[preset_color]);

    /* FIXME: usually RectFill() is faster */
    Move(rp, left,             top);
    Draw(rp, left + width - 1, top);
    Draw(rp, left + width - 1, top + height - 1);
    Draw(rp, left,             top + height - 1);
    Draw(rp, left,             top);
}


/**************************************************************************
 simple white border
**************************************************************************/
static void  frame_white_rect_draw (struct MUI_RenderInfo *mri,
		       int left, int top, int width, int height)
{
    rect_draw(mri, left, top, width, height, MPEN_SHINE);
}

/**************************************************************************
 simple black border
**************************************************************************/
static void  frame_black_rect_draw (struct MUI_RenderInfo *mri,
		       int left, int top, int width, int height)
{
    rect_draw(mri, left, top, width, height, MPEN_SHADOW);
}

/**************************************************************************
 2 : FST_BEVEL

 Draw a bicolor rectangle
**************************************************************************/
static void button_draw (struct MUI_RenderInfo *mri,
	     int left, int top, int width, int height,
	     MPen ul_preset, MPen lr_preset)
{
    struct RastPort *rp = mri->mri_RastPort;

    SetAPen(rp, mri->mri_Pens[ul_preset]);
    Move(rp, left,             top + height - 2);
    Draw(rp, left,             top);
    Draw(rp, left + width - 2, top);

    SetAPen(rp, mri->mri_Pens[lr_preset]);
    Move(rp, left + width - 1, top);
    Draw(rp, left + width - 1, top + height - 1);
    Draw(rp, left,             top + height - 1);

    SetAPen(rp, mri->mri_Pens[MPEN_BACKGROUND]);
    WritePixel(rp, left,             top + height - 1);
    WritePixel(rp, left + width - 1, top);
}

/**************************************************************************
 classic button
**************************************************************************/
static void  frame_bevelled_draw (struct MUI_RenderInfo *mri,
		     int left, int top, int width, int height)
{
    button_draw(mri, left, top, width, height, MPEN_SHINE, MPEN_SHADOW);
}

/**************************************************************************
 classic pressed button
**************************************************************************/
static void frame_recessed_draw (struct MUI_RenderInfo *mri,
		     int left, int top, int width, int height)
{
    button_draw(mri, left, top, width, height, MPEN_SHADOW, MPEN_SHINE);
}

/**************************************************************************
 3 : FST_THIN_BORDER
 Draw a thin relief border
**************************************************************************/
static void thinborder_draw (struct MUI_RenderInfo *mri,
		 int left, int top, int width, int height,
		 MPen ul_preset, MPen lr_preset)
{
    struct RastPort *rp = mri->mri_RastPort;

    SetAPen(rp, mri->mri_Pens[ul_preset]);
    Move(rp, left,             top + height - 1);
    Draw(rp, left,             top);
    Draw(rp, left + width - 1, top);

    Move(rp, left + width - 2, top + 2);
    Draw(rp, left + width - 2, top + height - 2);
    Draw(rp, left + 2,         top + height - 2);

    rect_draw(mri, left+1, top+1, width-1, height-1, lr_preset);
}

/**************************************************************************
 draw border up
**************************************************************************/
static void frame_thin_border_up_draw (struct MUI_RenderInfo *mri,
			   int left, int top, int width, int height)
{
    thinborder_draw(mri, left, top, width, height, MPEN_SHINE, MPEN_SHADOW);
}

/**************************************************************************
 draw border down
**************************************************************************/
static void frame_thin_border_down_draw (struct MUI_RenderInfo *mri,
			     int left, int top, int width, int height)
{
    thinborder_draw(mri, left, top, width, height, MPEN_SHADOW, MPEN_SHINE);
}

/**************************************************************************
 4 : FST_THICK_BORDER
 Draw a thick relief border
**************************************************************************/
static void thickborder_draw (struct MUI_RenderInfo *mri,
		  int left, int top, int width, int height,
		  BOOL bevelled)

{
    if (bevelled)
    {
	button_draw(mri, left, top, width, height, MPEN_SHINE, MPEN_SHADOW);
	button_draw(mri, left+2, top+2, width-4, height-4, MPEN_SHADOW, MPEN_SHINE);
    }
    else
    {
	button_draw(mri, left, top, width, height, MPEN_SHADOW, MPEN_SHINE);
	button_draw(mri, left+2, top+2, width-4, height-4, MPEN_SHINE, MPEN_SHADOW);
    }

    rect_draw(mri, left+1, top+1, width-2, height-2, MPEN_BACKGROUND);
}

/**************************************************************************
 draw thick border up
**************************************************************************/
static void frame_thick_border_up_draw (struct MUI_RenderInfo *mri,
			    int left, int top, int width, int height)
{
    thickborder_draw(mri, left, top, width, height, TRUE);
}

/**************************************************************************
 draw thick border down
**************************************************************************/
static void frame_thick_border_down_draw (struct MUI_RenderInfo *mri,
			      int left, int top, int width, int height)
{
    thickborder_draw(mri, left, top, width, height, FALSE);
}

/**************************************************************************
 5 : FST_ROUND_BEVEL
**************************************************************************/
static void round_bevel_draw (struct MUI_RenderInfo *mri,
		  int left, int top, int width, int height,
		  MPen ul, MPen lr)
{
    SetAPen(mri->mri_RastPort, mri->mri_Pens[MPEN_BACKGROUND]);
    RectFill(mri->mri_RastPort,
             left, top, left + 3, top + height - 1);
    RectFill(mri->mri_RastPort,
             left + width - 4, top, left + width - 1, top + height - 1);
    rect_draw(mri, left, top+2, 2, height-4, ul);
    rect_draw(mri, left+1, top+1, 2, 1, ul);
    rect_draw(mri, left+1, top + height - 2, 2, 1, ul);
    rect_draw(mri, left+2, top + height - 1, 1, 1, ul);
    rect_draw(mri, left+2, top, width - 5, 1, ul);

    rect_draw(mri, left + width - 2, top+2, 2, height-4, lr);
    rect_draw(mri, left + width - 3, top+1, 2, 1, lr);
    rect_draw(mri, left + width - 3, top + height - 2, 2, 1, lr);
    rect_draw(mri, left+3, top + height - 1, width - 5, 1, lr);
    rect_draw(mri, left + width - 3, top, 1, 1, lr);
}

/**************************************************************************

**************************************************************************/
static void frame_round_bevel_up_draw (struct MUI_RenderInfo *mri,
			   int left, int top, int width, int height)
{
    round_bevel_draw (mri, left, top, width, height,
		      MPEN_SHINE, MPEN_SHADOW);
}

/**************************************************************************

**************************************************************************/
static void frame_round_bevel_down_draw (struct MUI_RenderInfo *mri,
			     int left, int top, int width, int height)
{
    round_bevel_draw (mri, left, top, width, height,
		      MPEN_SHADOW, MPEN_SHINE);
}

/**************************************************************************
 6 : FST_WIN_BEVEL
**************************************************************************/
static void frame_border_button_up_draw (struct MUI_RenderInfo *mri,
			    int left, int top, int width, int height)
{
    rect_draw(mri, left, top, width, height, MPEN_SHADOW);
    button_draw(mri, left+1, top+1, width-2, height-2,
		MPEN_SHINE, MPEN_HALFSHADOW);
}

/**************************************************************************

**************************************************************************/
static void frame_border_button_down_draw (struct MUI_RenderInfo *mri,
			      int left, int top, int width, int height)
{
    button_draw(mri, left+2, top+2, width-2, height-2,
		MPEN_BACKGROUND, MPEN_SHADOW);
    button_draw(mri, left+1, top+1, width-1, height-1,
		MPEN_HALFSHADOW, MPEN_SHADOW);
    rect_draw(mri, left, top, width, height, MPEN_SHADOW);
}

/**************************************************************************
 7 : FST_ROUND_THICK_BORDER
**************************************************************************/
static void round_thick_border_draw (struct MUI_RenderInfo *mri,
			int left, int top, int width, int height,
			MPen pen1, MPen pen2, MPen pen3, MPen pen4, MPen pen5)
{
    struct RastPort *rp = mri->mri_RastPort;

    rect_draw(mri, left, top, width - 2, height - 2, pen1);
    rect_draw(mri, left + 2, top + 2, width - 2, height - 2, pen5);

    rect_draw(mri, left, top, 2, 2, pen2);
    rect_draw(mri, left, top + height - 3, 2, 2, pen2);
    rect_draw(mri, left + width - 3, top, 2, 2, pen2);

    rect_draw(mri, left + width - 2, top + height - 2, 2, 2, pen4);
    rect_draw(mri, left + 1, top + height - 2, 2, 2, pen4);
    rect_draw(mri, left + width - 2, top + 1, 2, 2, pen4);

    rect_draw(mri, left + 1, top + 1, width - 2, height - 2, pen3);

    rect_draw(mri, left + 2, top + 2, 2, 2, pen3);
    rect_draw(mri, left + 2, top + height - 4, 2, 2, pen3);
    rect_draw(mri, left + width - 4, top + height - 4, 2, 2, pen3);
    rect_draw(mri, left + width - 4, top + 2, 2, 2, pen3);

    /* these points were not in the original frame.  -dlc */
    SetAPen(rp, mri->mri_Pens[pen5]);
    WritePixel(rp, left + 3, top + 3);

    SetAPen(rp, mri->mri_Pens[pen1]);
    WritePixel(rp, left + width - 4, top + height - 4);
}

/**************************************************************************

**************************************************************************/
static void frame_round_thick_border_up_draw (struct MUI_RenderInfo *mri,
				 int left, int top, int width, int height)
{
    round_thick_border_draw(mri, left, top, width, height,
			    MPEN_SHINE, MPEN_HALFSHINE, MPEN_BACKGROUND,
			    MPEN_HALFSHADOW, MPEN_SHADOW);
}

/**************************************************************************

**************************************************************************/
static void frame_round_thick_border_down_draw (struct MUI_RenderInfo *mri,
				   int left, int top, int width, int height)
{
    round_thick_border_draw(mri, left, top, width, height,
			    MPEN_SHADOW, MPEN_HALFSHADOW, MPEN_BACKGROUND,
			    MPEN_HALFSHINE, MPEN_SHINE);
}

/**************************************************************************
 8 : FST_ROUND_THIN_BORDER
**************************************************************************/
static void round_thin_border_draw (struct MUI_RenderInfo *mri,
			int left, int top, int width, int height,
			MPen pen1, MPen pen2, MPen pen3, MPen pen4, MPen pen5)
{
    struct RastPort *rp = mri->mri_RastPort;

    rect_draw(mri, left, top, width - 1, height - 1, pen1);
    rect_draw(mri, left + 1, top + 1, width - 1, height - 1, pen5);

    rect_draw(mri, left, top, 2, 2, pen2);
    rect_draw(mri, left + width - 4, top + height - 4, 2, 2, pen2);
    rect_draw(mri, left + 2, top + 2, 2, 2, pen4);
    rect_draw(mri, left + width - 2, top + height - 2, 2, 2, pen4);
    rect_draw(mri, left + 1, top + 1, 2, 2, pen3);
    rect_draw(mri, left + width - 3, top + height - 3, 2, 2, pen3);

    rect_draw(mri, left + 1, top + height - 3, 1, 3, pen4);
    rect_draw(mri, left + width - 3, top + 1, 3, 1, pen4);

    WritePixel(rp, left + 2,         top + height - 3);
    WritePixel(rp, left + width - 3, top + 2);

    SetAPen(rp, mri->mri_Pens[pen2]);
    WritePixel(rp, left,             top + height - 2);
    WritePixel(rp, left + 2,         top + height - 2);
    WritePixel(rp, left + width - 2, top);
    WritePixel(rp, left + width - 2, top + 2);
}

/**************************************************************************

**************************************************************************/
static void frame_round_thin_border_up_draw (struct MUI_RenderInfo *mri,
				 int left, int top, int width, int height)
{
    round_thin_border_draw(mri, left, top, width, height,
			   MPEN_SHINE, MPEN_HALFSHINE, MPEN_BACKGROUND,
			   MPEN_HALFSHADOW, MPEN_SHADOW);
}

/**************************************************************************

**************************************************************************/
static void frame_round_thin_border_down_draw (struct MUI_RenderInfo *mri,
				   int left, int top, int width, int height)
{
    round_thin_border_draw(mri, left, top, width, height,
			   MPEN_SHADOW, MPEN_HALFSHADOW, MPEN_BACKGROUND,
			   MPEN_HALFSHINE, MPEN_SHINE);
}

/**************************************************************************
 9 : FST_GRAY_BORDER
**************************************************************************/
static void frame_gray_border_up_draw (struct MUI_RenderInfo *mri,
			   int left, int top, int width, int height)
{
    thinborder_draw(mri, left, top, width, height,
		    MPEN_HALFSHINE, MPEN_HALFSHADOW);
}

/**************************************************************************

**************************************************************************/
static void frame_gray_border_down_draw (struct MUI_RenderInfo *mri,
			     int left, int top, int width, int height)
{
    thinborder_draw(mri, left, top, width, height,
		    MPEN_HALFSHADOW, MPEN_HALFSHINE);
}

/**************************************************************************
 A : FST_SEMIROUND_BEVEL
**************************************************************************/
static void semiround_bevel_draw (struct MUI_RenderInfo *mri,
		      int left, int top, int width, int height,
		      MPen pen1, MPen pen2, MPen pen3, MPen pen4, MPen pen5)
{
    struct RastPort *rp = mri->mri_RastPort;

    button_draw(mri, left, top, width, height, pen1, pen5);
    button_draw(mri, left+1, top+1, width-2, height-2, pen2, pen4);

    SetAPen(rp, mri->mri_Pens[pen2]);
    WritePixel(rp, left, top);

    SetAPen(rp, mri->mri_Pens[pen1]);
    WritePixel(rp, left + 1, top + 1);

    SetAPen(mri->mri_RastPort, mri->mri_Pens[pen5]);
    WritePixel(rp, left + width - 2, top + height - 2);

    SetAPen(mri->mri_RastPort, mri->mri_Pens[pen4]);
    WritePixel(rp, left + width - 1, top + height - 1);

    SetAPen(mri->mri_RastPort, mri->mri_Pens[pen3]);
    WritePixel(rp, left,             top + height - 2);
    WritePixel(rp, left + 1,         top + height - 1);
    WritePixel(rp, left + width - 2, top);
    WritePixel(rp, left + width - 1, top + 1);
}

/**************************************************************************

**************************************************************************/
static void frame_semiround_bevel_up_draw (struct MUI_RenderInfo *mri,
			       int left, int top, int width, int height)
{
    semiround_bevel_draw (mri, left, top, width, height,
			  MPEN_SHINE, MPEN_HALFSHINE, MPEN_BACKGROUND,
			  MPEN_HALFSHADOW, MPEN_SHADOW);
}

/**************************************************************************

**************************************************************************/
static void frame_semiround_bevel_down_draw (struct MUI_RenderInfo *mri,
				 int left, int top, int width, int height)
{
    semiround_bevel_draw (mri, left, top, width, height,
			  MPEN_SHADOW, MPEN_HALFSHADOW, MPEN_BACKGROUND,
			  MPEN_HALFSHINE, MPEN_SHINE);
}

/**************************************************************************
 hold builtin frames.
**************************************************************************/
static const struct ZuneFrameGfx __builtinFrameGfx[] = {
    /* type 0 : FST_NONE */
    {frame_none_draw, 0, 0, 0, 0},
    {frame_none_draw, 0, 0, 0, 0},
    
    /* monochrome border */
    /* 1 : FST_RECT */
    {frame_white_rect_draw, 1, 1, 1, 1},
    {frame_black_rect_draw, 1, 1, 1, 1},
    
    /* clean 3D look */
    /* 2 : FST_BEVEL */
    {frame_bevelled_draw, 1, 1, 1, 1},
    {frame_recessed_draw, 1, 1, 1, 1},
    
    /* thin relief border */
    /* 3 : FST_THIN_BORDER */
    {frame_thin_border_up_draw, 2, 2, 2, 2},
    {frame_thin_border_down_draw, 2, 2, 2, 2},
    
    /* thick relief border */
    /* 4 : FST_THICK_BORDER */
    {frame_thick_border_up_draw, 3, 3, 3, 3},
    {frame_thick_border_down_draw, 3, 3, 3, 3},
    
    /* rounded bevel */
    /* 5 : FST_ROUND_BEVEL */
    {frame_round_bevel_up_draw, 4, 4, 1, 1},
    {frame_round_bevel_down_draw, 4, 4, 1, 1},
    
    /* zin31/xen look */
    /* 6 : FST_WIN_BEVEL */
    {frame_border_button_up_draw, 2, 2, 2, 2},
    {frame_border_button_down_draw, 3, 1, 3, 1},
    
    /* rounded thick border */
    /* 7 : FST_ROUND_THICK_BORDER */
    {frame_round_thick_border_up_draw, 4, 4, 4, 4},
    {frame_round_thick_border_down_draw, 4, 4, 4, 4},
    
    /* rounded thin border */
    /* 8 : FST_ROUND_THIN_BORDER */
    {frame_round_thin_border_up_draw, 4, 4, 4, 4},
    {frame_round_thin_border_down_draw, 4, 4, 4, 4},
    
    /* strange gray border */
    /* 9 : FST_GRAY_BORDER */
    {frame_gray_border_up_draw, 2, 2, 2, 2},
    {frame_gray_border_down_draw, 2, 2, 2, 2},
    
    /* semi rounded bevel */
    /* A : FST_SEMIROUND_BEVEL */
    {frame_semiround_bevel_up_draw, 2, 2, 2, 2},
    {frame_semiround_bevel_down_draw, 2, 2, 2, 2},
};


/**************************************************************************

**************************************************************************/
const struct ZuneFrameGfx *zune_zframe_get (const struct MUI_FrameSpec_intern *frameSpec)
{
    if (frameSpec->type >= FST_COUNT) return &__builtinFrameGfx[2 * FST_RECT];
    return &__builtinFrameGfx[2 * frameSpec->type + frameSpec->state];
}

const struct ZuneFrameGfx *zune_zframe_get_with_state (const struct MUI_FrameSpec_intern *frameSpec,
						 UWORD state)
{
    if (frameSpec->type >= FST_COUNT) return &__builtinFrameGfx[2 * FST_RECT];
    return &__builtinFrameGfx[2 * frameSpec->type + state];
}

/*------------------------------------------------------------------------*/

BOOL zune_frame_intern_to_spec (const struct MUI_FrameSpec_intern *intern,
				STRPTR spec)
{
    if (!intern || !spec)
	return FALSE;

    /* Must cast to LONG because on AmigaOS SNPrintf() is used which is like
     * RawDoFmt() 16 bit */
    snprintf(&spec[0], 1, "%lx", (LONG)intern->type);
    snprintf(&spec[1], 1, "%lx", (LONG)intern->state);
    snprintf(&spec[2], 1, "%lx", (LONG)intern->innerLeft);
    snprintf(&spec[3], 1, "%lx", (LONG)intern->innerRight);
    snprintf(&spec[4], 1, "%lx", (LONG)intern->innerTop);
    snprintf(&spec[5], 1, "%lx", (LONG)intern->innerBottom);
    spec[6] = 0;
    return TRUE;
}

/*------------------------------------------------------------------------*/

static int hexasciichar_to_int(char x)
{
    if (x >= '0' && x <= '9') return x - '0';
    if (x >= 'a' && x <= 'f') return x - 'a';
    if (x >= 'A' && x <= 'F') return x - 'A';
    return -1;
}

BOOL zune_frame_spec_to_intern(CONST_STRPTR spec,
			       struct MUI_FrameSpec_intern *intern)
{    
    int val;
    
    if (!intern || !spec)
	return FALSE;

    val = hexasciichar_to_int(spec[0]);
    if (val == -1) return FALSE;
    intern->type = val;

    val = hexasciichar_to_int(spec[1]);
    if (val == -1) return FALSE;
    intern->state = val;

    val = hexasciichar_to_int(spec[2]);
    if (val == -1) return FALSE;
    intern->innerLeft = val;

    val = hexasciichar_to_int(spec[3]);
    if (val == -1) return FALSE;
    intern->innerRight = val;

    val = hexasciichar_to_int(spec[4]);
    if (val == -1) return FALSE;
    intern->innerTop = val;

    val = hexasciichar_to_int(spec[5]);
    if (val == -1) return FALSE;
    intern->innerBottom = val;
    return TRUE;
}
