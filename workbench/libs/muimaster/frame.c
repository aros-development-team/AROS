/* Zune -- a free Magic User Interface implementation
 * Copyright (C) 1999 David Le Corfec
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <exec/types.h>

#include <proto/graphics.h>

#include "muimaster_intern.h"
#include "mui.h"

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

    /* FIXME: missing SetAPen() here? */
    Move(rp, left + width - 2, top + 2);
    Draw(rp, left + width - 2, top + height - 2);
    Draw(rp, left + 2,         top + height - 2);

    rect_draw(mri, left+1, top+1, width-2, height-2, lr_preset);
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
 5 : FST_WIN_BEVEL
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
 6 : FST_GRAY_BORDER
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
 7 : FST_SEMIROUND_BEVEL
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
 8 : FST_SEMIROUND_THIN_BORDER
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
 9 : FST_ROUND_THICK_BORDER
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
 10 : FST_ROUND_BEVEL
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
 hold builtin frames.
**************************************************************************/
static struct ZuneFrameGfx __builtinFrameGfx[] = {
    { /* type 0 : FST_NONE */
	{frame_none_draw,
	frame_none_draw},
	0, 0
    },
    /* monochrome border */
    { /* 1 : FST_RECT */
	{frame_white_rect_draw,
	frame_black_rect_draw},
	1, 1
    },
    /* clean 3D look */
    { /* 2 : FST_BEVEL */
	{frame_bevelled_draw,
	frame_recessed_draw},
	1, 1
    },
    /* thin relief border */
    { /* 3 : FST_THIN_BORDER */
	{frame_thin_border_up_draw,
	frame_thin_border_down_draw},
	2, 2
    },
    /* thick relief border */
    { /* 4 : FST_THICK_BORDER */
	{frame_thick_border_up_draw,
	frame_thick_border_down_draw},
	3, 3
    },
    /* zin31 ugly look */
    { /* 5 : FST_WIN_BEVEL */
	{frame_border_button_up_draw,
	frame_border_button_down_draw},
	3, 3
    },
    /* strange gray border */
    { /* 6 : FST_GRAY_BORDER */
	{frame_gray_border_up_draw,
	frame_gray_border_down_draw},
	3, 3
    },
    /* semi rounded bevel */
    { /* 7 : FST_SEMIROUND_BEVEL */
	{frame_semiround_bevel_up_draw,
	frame_semiround_bevel_down_draw},
	2, 2
    },
    /* rounded thin border */
    { /* 8 : FST_ROUND_THIN_BORDER */
	{frame_round_thin_border_up_draw,
	frame_round_thin_border_down_draw},
	4, 4
    },
    /* rounded thick border */
    { /* 9 : FST_ROUND_THICK_BORDER */
	{frame_round_thick_border_up_draw,
	frame_round_thick_border_down_draw},
	4, 4
    },
    /* rounded bevel */
    { /* 10 : FST_ROUND_BEVEL */
	{frame_round_bevel_up_draw,
	frame_round_bevel_down_draw},
	4, 1
    },
};


/**************************************************************************
 Get a frame given its index
**************************************************************************/
struct ZuneFrameGfx *zune_zframe_get_index (int i)
{
    if (i >= FST_COUNT) return &__builtinFrameGfx[FST_RECT];
    return &__builtinFrameGfx[i];
}

/**************************************************************************

**************************************************************************/
struct ZuneFrameGfx *zune_zframe_get (struct MUI_FrameSpec *frameSpec)
{
    if (frameSpec->type >= FST_COUNT) return &__builtinFrameGfx[FST_RECT];
    return &__builtinFrameGfx[frameSpec->type];
}


/**************************************************************************

**************************************************************************/
STRPTR zune_framespec_to_string (struct MUI_FrameSpec *frame)
{
#if 0
    static char buf[300];

    g_snprintf(buf, 300, "%hu,%hu,%hu,%hu,%hu,%hu",
	       frame->type, frame->state,
	       frame->innerLeft, frame->innerRight,
	       frame->innerTop, frame->innerBottom);
    return buf;
#endif
    return NULL;
}

/**************************************************************************

**************************************************************************/
BOOL zune_string_to_framespec (STRPTR str, struct MUI_FrameSpec *frame)
{
#if 0
    if (str && frame && sscanf(str, "%hu,%hu,%hu,%hu,%hu,%hu",
			       &frame->type, &frame->state,
			       &frame->innerLeft, &frame->innerRight,
			       &frame->innerTop, &frame->innerBottom) == 6)
	return TRUE;
#endif
    return FALSE;
}
