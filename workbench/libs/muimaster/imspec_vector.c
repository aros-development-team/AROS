/*
    Copyright © 2003, The AROS Development Team. 
    All rights reserved.
    
    $Id$
*/

#include <intuition/imageclass.h>
#include <proto/graphics.h>
#include <proto/exec.h>
#include <aros/debug.h>

#include "mui.h"
#include "imspec_intern.h"
#include "support.h"

extern struct Library *MUIMasterBase;

static void draw_thick_line(struct RastPort *rp,int x1, int y1, int x2, int y2)
{
    Move(rp,x1,y1);
    Draw(rp,x2,y2);
    Move(rp,x1+1,y1);
    Draw(rp,x2+1,y2);
}

//#define SPACING 1
#define HSPACING 0
#define VSPACING 0

#define ARROW_SPACING 1
#define ARROW_VSPACING ARROW_SPACING
#define ARROW_HSPACING ARROW_SPACING
#define VERTARROW_VSPACING ARROW_VSPACING
#define HORIZARROW_HSPACING ARROW_HSPACING

void arrowup_draw(struct MUI_RenderInfo *mri, LONG left, LONG top, LONG width, LONG height, LONG state)
{
    int cx;
    struct RastPort *rport = mri->mri_RastPort;

    SetAPen(rport, mri->mri_Pens[MPEN_TEXT]);

    cx = width / 2;

    if ((width > 8 + ARROW_SPACING) && (height > 8 + ARROW_SPACING))
    {
	Move(rport, left + ARROW_HSPACING + 1, top + height - 1 - VERTARROW_VSPACING);
	Draw(rport, left + width - cx, top + VERTARROW_VSPACING);
    }
    Move(rport, left + ARROW_HSPACING, top + height - 1 - VERTARROW_VSPACING);
    Draw(rport, left + width - cx - 1, top + VERTARROW_VSPACING);
    if ((width > 8 + ARROW_SPACING) && (height > 8 + ARROW_SPACING))
    {
	Move(rport, left + width - 1 - ARROW_HSPACING - 1, top + height - 1 - VERTARROW_VSPACING);
	Draw(rport, left + cx - 1, top + VERTARROW_VSPACING);
    }
    Move(rport, left + width - 1 - ARROW_HSPACING, top + height - 1 - VERTARROW_VSPACING);
    Draw(rport, left + cx, top + VERTARROW_VSPACING);
}

void arrowdown_draw(struct MUI_RenderInfo *mri, LONG left, LONG top, LONG width, LONG height, LONG state)
{
    int cx;
    struct RastPort *rport = mri->mri_RastPort;

    SetAPen(rport, mri->mri_Pens[MPEN_TEXT]);

    cx = width / 2;

    if ((width > 8 + ARROW_SPACING) && (height > 8 + ARROW_SPACING))
    {
	Move(rport, left + ARROW_HSPACING + 1, top + VERTARROW_VSPACING);
	Draw(rport, left + width - cx, top + height - 1 - VERTARROW_VSPACING);
    }
    Move(rport, left + ARROW_HSPACING, top + VERTARROW_VSPACING);
    Draw(rport, left + width - cx - 1, top + height - 1 - VERTARROW_VSPACING);
    if ((width > 8 + ARROW_SPACING) && (height > 8 + ARROW_SPACING))
    {
	Move(rport, left + width - 1 - ARROW_HSPACING - 1, top + VERTARROW_VSPACING);
	Draw(rport, left + cx - 1, top + height - 1 - VERTARROW_VSPACING);
    }
    Move(rport, left + width - 1 - ARROW_HSPACING, top + VERTARROW_VSPACING);
    Draw(rport, left + cx, top + height - 1 - VERTARROW_VSPACING);
}

void arrowleft_draw(struct MUI_RenderInfo *mri, LONG left, LONG top, LONG width, LONG height, LONG state)
{
    int cy;
    struct RastPort *rport = mri->mri_RastPort;

    SetAPen(rport, mri->mri_Pens[MPEN_TEXT]);

    cy = height / 2;

    if ((height > 8 + ARROW_SPACING) && (width > 8 + ARROW_SPACING))
    {
	Move(rport, left + width - 1 - HORIZARROW_HSPACING, top + ARROW_VSPACING + 1);
	Draw(rport, left + HORIZARROW_HSPACING, top + height - cy);
    }
    Move(rport, left + width - 1 - HORIZARROW_HSPACING, top + ARROW_VSPACING);
    Draw(rport, left + HORIZARROW_HSPACING, top + height - cy - 1);
    if ((height > 8 + ARROW_SPACING) && (width > 8 + ARROW_SPACING))
    {
	Move(rport, left + width - 1 - HORIZARROW_HSPACING, top + height - 1 - ARROW_VSPACING - 1);
	Draw(rport, left + HORIZARROW_HSPACING, top + cy - 1);
    }
    Move(rport, left + width - 1 - HORIZARROW_HSPACING, top + height - 1 - ARROW_VSPACING);
    Draw(rport, left + HORIZARROW_HSPACING, top + cy);
}

void arrowright_draw(struct MUI_RenderInfo *mri, LONG left, LONG top, LONG width, LONG height, LONG state)
{
    int cy;
    struct RastPort *rport = mri->mri_RastPort;

    SetAPen(rport, mri->mri_Pens[MPEN_TEXT]);

    cy = height / 2;

    if ((height > 8 + ARROW_SPACING) && (width > 8 + ARROW_SPACING))
    {
	Move(rport, left + HORIZARROW_HSPACING, top + ARROW_VSPACING + 1);
	Draw(rport, left + width - 1 - HORIZARROW_HSPACING, top + height - cy);
    }
    Move(rport, left + HORIZARROW_HSPACING, top + ARROW_VSPACING);
    Draw(rport, left + width - 1 - HORIZARROW_HSPACING, top + height - cy - 1);
    if ((height > 8 + ARROW_SPACING) && (width > 8 + ARROW_SPACING))
    {
	Move(rport, left + HORIZARROW_HSPACING, top + height - 1- ARROW_VSPACING - 1);
	Draw(rport, left + width - 1 - HORIZARROW_HSPACING, top + cy - 1);
    }
    Move(rport, left + HORIZARROW_HSPACING, top + height - 1 - ARROW_VSPACING);
    Draw(rport, left + width - 1 - HORIZARROW_HSPACING, top + cy);
}

void checkbox_draw(struct MUI_RenderInfo *mri, LONG left, LONG top, LONG width, LONG height, LONG state)
{
    int h_spacing = width / 4;
    int v_spacing = height / 4;
    int bottom = top + height - 1;
    int right = left + width - 1;

    /* Draw checkmark (only if image is in selected state) */

    if (state == IDS_SELECTED)
    {
	left += h_spacing;right -= h_spacing;width -= h_spacing * 2;
	top += v_spacing;bottom -= v_spacing;height -= v_spacing * 2;

        SetAPen(mri->mri_RastPort, mri->mri_Pens[MPEN_TEXT]);

	draw_thick_line(mri->mri_RastPort, left, top + height / 3 , left, bottom);
	draw_thick_line(mri->mri_RastPort, left + 1, bottom, right - 1, top);
    }
}

void mx_draw(struct MUI_RenderInfo *mri, LONG left, LONG top, LONG width, LONG height, LONG state)
{
    struct RastPort *rport = mri->mri_RastPort;
    int bottom = top + height - 1;
    int right = left + width - 1;
    int col1;
    int col2;

    if (state == IDS_SELECTED)
    {
	col1 = MPEN_SHADOW;
	col2 = MPEN_SHINE;
    } else
    {
	col1 = MPEN_SHINE;
	col2 = MPEN_SHADOW;
    }

    SetAPen(rport, mri->mri_Pens[col1]);
    RectFill(rport, left + 3, top, right - 3, top);
    WritePixel(rport, left + 2, top + 1);
    WritePixel(rport, left + 1, top + 2);
    RectFill(rport, left, top + 3, left, bottom - 3);
    WritePixel(rport, left + 1, bottom - 2);
    WritePixel(rport, left + 2, bottom - 1);
	
    SetAPen(rport, mri->mri_Pens[col2]);
    WritePixel(rport, right - 2, top + 1);
    WritePixel(rport, right - 1, top + 2);
    RectFill(rport, right, top + 3, right, bottom - 3);
    WritePixel(rport, right - 1, bottom - 2);
    WritePixel(rport, right - 2, bottom - 1);
    RectFill(rport, left + 3, bottom, right - 3, bottom);
	
    SetAPen(rport, mri->mri_DrawInfo->dri_Pens[BACKGROUNDPEN]);
    RectFill(rport, left + 3, top + 1, right - 3, bottom - 1);
    RectFill(rport, left + 2, top + 2, right - 2, bottom - 2);
    RectFill(rport, left + 1, top + 3, right - 1, bottom - 3);

    if (state == IDS_SELECTED)
    {
	left += 3;right -= 3;width -= 6;
	top += 3;bottom -= 3;height -= 6;
	    
        SetAPen(rport, mri->mri_Pens[MPEN_FILL]);
	if ((width >= 5) && (height >= 5))
	{
	    RectFill(rport, left, top + 2, left, bottom - 2);
	    RectFill(rport, left + 1, top + 1, left + 1, bottom - 1);
	    RectFill(rport, left + 2, top, right - 2, bottom);
	    RectFill(rport, right - 1, top + 1, right - 1, bottom - 1);
	    RectFill(rport, right, top + 2, right, bottom - 2);
	} else {
	    RectFill(rport, left, top, right, bottom);
	    RectFill(rport, left - 1, top + 1, right + 1, bottom - 1);
	    RectFill(rport, left + 1, top - 1, right - 1, bottom + 1);
	}
    }
}

void cycle_draw(struct MUI_RenderInfo *mri, LONG left, LONG top, LONG width, LONG height, LONG state)
{
    struct RastPort *rport = mri->mri_RastPort;
    int bottom = top + height - 1;
    int right = left + width - 1;
#if BIGGER_ARROW
    int arrow_top;
#endif

    SetAPen(rport, mri->mri_Pens[MPEN_TEXT]);

    Move(rport,left,top+1);
    Draw(rport,left,bottom-1);
    Move(rport,left+1,top);
    Draw(rport,left+1,bottom);
    Draw(rport,right-7,bottom);
    Move(rport,right-7,bottom-1);
    Draw(rport,right-6,bottom-1);
    Move(rport,left+2,top);
    Draw(rport,right-7,top);
    Move(rport,right-7,top+1);
    Draw(rport,right-6,top+1);

#if BIGGER_ARROW
    arrow_top = top + (3 * height / 4) - 3;
    /* prevent arrow touching bottom */
    if (arrow_top + 4 >= bottom-1)
	arrow_top = bottom - 5;

    RectFill(rport,right-7,top+1,right-6,arrow_top-1);
#endif
    /* The small arrow */
    Move(rport,right - 6 - 3, top+2);
    Draw(rport,right - 7 + 3, top+2);
    Move(rport,right - 6 - 2, top+3);
    Draw(rport,right - 7 + 2, top+3);
    Move(rport,right - 6 - 1, top+4);
    Draw(rport,right - 7 + 1, top+4);
#if BIGGER_ARROW
    /* makes arrow bigger */
    if (arrow_top - top - 2 >= 4)
    {
	Move(rport,right - 6 - 3, arrow_top-1);
	Draw(rport,right - 7 + 3, arrow_top-1);
	Move(rport,right - 6 - 2, arrow_top);
	Draw(rport,right - 7 + 2, arrow_top);
	Move(rport,right - 6 - 2, arrow_top+1);
	Draw(rport,right - 7 + 2, arrow_top+1);
	Move(rport,right - 6 - 1, arrow_top+2);
	Draw(rport,right -7 7 + 1, arrow_top+2);
    }
    else
    {
	Move(rport,right - 6 - 3, arrow_top);
	Draw(rport,right - 7 + 3, arrow_top);
	Move(rport,right - 6 - 2, arrow_top+1);
	Draw(rport,right - 7 + 2, arrow_top+1);
	Move(rport,right - 6 - 1, arrow_top+2);
	Draw(rport,right - 7 + 1, arrow_top+2);
    }
#endif

    /* The right bar */
    SetAPen(rport, mri->mri_Pens[MPEN_SHADOW]);
    Move(rport,right - 1, top);
    Draw(rport,right - 1, bottom);
    SetAPen(rport, mri->mri_Pens[MPEN_SHINE]);
    Move(rport,right, top);
    Draw(rport,right, bottom);
}

void popup_draw(struct MUI_RenderInfo *mri, LONG left, LONG top, LONG width, LONG height, LONG state)
{
    int right,bottom,cx;
    struct RastPort *rport = mri->mri_RastPort;

    height -= 3;

    SetAPen(rport, mri->mri_Pens[MPEN_TEXT]);

    cx = width / 2;

    Move(rport, left + HSPACING + 1, top + VSPACING);
    Draw(rport, left + width - cx, top + height - 1 - VSPACING);
    Move(rport, left + HSPACING, top + VSPACING);
    Draw(rport, left + width - cx - 1, top + height - 1 - VSPACING);

    Move(rport, left + width - 1 - HSPACING - 1, top + VSPACING);
    Draw(rport, left + cx - 1, top + height - 1 - VSPACING);
    Move(rport, left + width - 1 - HSPACING, top + VSPACING);
    Draw(rport, left + cx, top + height - 1 - VSPACING);

    bottom = top + height - 1 + 3;
    right = left + width - 1;
    Move(rport, left, bottom-2);
    Draw(rport, right, bottom-2);
    Move(rport, left, bottom-1);
    Draw(rport, right, bottom-1);
}

void popfile_draw(struct MUI_RenderInfo *mri, LONG left, LONG top, LONG width, LONG height, LONG state)
{
    int right,bottom;
    int edgex,edgey;
    struct RastPort *rport = mri->mri_RastPort;

    right = left + width - 1;
    bottom = top + height - 1;

    edgex = left + width * 5 / 8;
    edgey = top + height * 5 / 8;

    SetAPen(rport, mri->mri_Pens[MPEN_TEXT]);
    Move(rport, left, top);
    Draw(rport, left, bottom);
    Move(rport, left+1, top);
    Draw(rport, left+1, bottom);

    Move(rport, left, bottom);
    Draw(rport, right, bottom);
    Move(rport, left, bottom-1);
    Draw(rport, right, bottom-1);

    Move(rport, right, bottom-1);
    Draw(rport, right, edgey);
    Move(rport, right-1, bottom-1);
    Draw(rport, right-1, edgey);

    Move(rport, right, edgey-1);
    Draw(rport, edgex, edgey-1);
    Draw(rport, edgex, top);
    Draw(rport, left+2,top);
    Move(rport, left+2,top+1);
    Draw(rport, edgex-1,top+1);

    Move(rport, edgex+1, top);
    Draw(rport, right, edgey-1);

}

void popdrawer_draw(struct MUI_RenderInfo *mri, LONG left, LONG top, LONG width, LONG height, LONG state)
{
    int right,bottom;
    int halfx,halfy,quartery;
    struct RastPort *rport = mri->mri_RastPort;

    right = left + width - 1;
    bottom = top + height - 1;

    halfx = (left + right) / 2;
    halfy = (top + bottom) / 2;
    quartery = top + height / 4;

    SetAPen(rport, mri->mri_Pens[MPEN_TEXT]);
    Move(rport, left, quartery);
    Draw(rport, left, bottom);
    Move(rport, left+1, quartery);
    Draw(rport, left+1, bottom);
    Draw(rport, right, bottom);
    Draw(rport, right, halfy);
    Draw(rport, halfx, halfy);
    Draw(rport, halfx, quartery);
    Draw(rport, left+1,quartery);

    Move(rport, halfx, quartery-1);
    Draw(rport, halfx + 2, top);
    Draw(rport, right - 2, top);
    Draw(rport, right, quartery-1);
    Draw(rport, right, halfy);
}

static void stddrawer_draw(struct MUI_RenderInfo *mri, LONG left, LONG top, LONG width, LONG height)
{
    struct RastPort *rport = mri->mri_RastPort;
    int right = left + width - 1;
    int bottom = top + height - 1;
    int x, y;
    
    #define XX(x)   (x * (width - 1) / 100)
    #define YY(y)   (y * (height - 1) / 100)
    #define YOFF    45
    #define XOFF    24
    
    for(y = 0; y < YY(YOFF); y++)
    {
    	x = (YY(XOFF) > 1) ? XX(XOFF) * y / (YY(YOFF) - 1) : 1;
        SetAPen(rport, mri->mri_Pens[MPEN_SHINE]);
	RectFill(rport, left + XX(XOFF) - x, top + y, right - XX(XOFF) + x, top + y);
    	SetAPen(rport, mri->mri_Pens[MPEN_SHADOW]);
    	WritePixel(rport, left + XX(XOFF) - x, top + y);
	WritePixel(rport, right - XX(XOFF) + x, top + y);
    }    


    SetAPen(rport, mri->mri_Pens[MPEN_SHINE]);
    RectFill(rport, left, top + YY(YOFF), right, bottom);

    SetAPen(rport, mri->mri_Pens[MPEN_SHADOW]);
    Move(rport, left + XX(XOFF), top);
    Draw(rport, right - XX(XOFF), top);
    
    Move(rport, left, top + YY(YOFF));
    Draw(rport, left, bottom);
    Draw(rport, right, bottom);
    Draw(rport, right, top + YY(YOFF));
    Draw(rport, left, top + YY(YOFF));
    
    #undef XX
    #undef YY
    #undef YOFF
    #undef XOFF
}


void drawer_draw(struct MUI_RenderInfo *mri, LONG left, LONG top, LONG width, LONG height, LONG state)
{
    struct RastPort *rport = mri->mri_RastPort;
    int right = left + width - 1;

    stddrawer_draw(mri, left, top, width, height);
    
    #define XX(x) x * (width - 1) / 100
    #define YY(y) y * (height - 1) / 100

    SetAPen(rport, mri->mri_Pens[MPEN_SHADOW]);
    Move(rport, left + XX(40), top + YY(75));
    Draw(rport, right - XX(40), top + YY(75));
    
    #undef XX
    #undef YY

}

void harddisk_draw(struct MUI_RenderInfo *mri, LONG left, LONG top, LONG width, LONG height, LONG state)
{
    struct RastPort *rport = mri->mri_RastPort;

    stddrawer_draw(mri, left, top, width, height);
    
    #define XX(x) x * (width - 1) / 100
    #define YY(y) y * (height - 1) / 100

    SetAPen(rport, mri->mri_Pens[MPEN_SHADOW]);
    Move(rport, left + XX(20), top + YY(75));
    Draw(rport, left + XX(30), top + YY(75));
    
    #undef XX
    #undef YY
}

void disk_draw(struct MUI_RenderInfo *mri, LONG left, LONG top, LONG width, LONG height, LONG state)
{
    struct RastPort *rport = mri->mri_RastPort;
    int right = left + width - 1;
    int bottom = top + height - 1;
    int x, y;
    
    #define XX(x)   (x * (width - 1) / 100)
    #define YY(y)   (y * (height - 1) / 100)
    #define YOFF    29
    #define XOFF    25
    
    SetAPen(rport, mri->mri_Pens[MPEN_SHADOW]);
    for(y = 0; y < YY(YOFF); y++)
    {
    	x = (YY(YOFF) > 1) ? XX(XOFF) * y / (YY(YOFF) - 1) : 1;
	RectFill(rport, left, top + y, right - XX(XOFF) + x, top + y);
    }    
    RectFill(rport, left, top + YY(YOFF), right, bottom);

    SetAPen(rport, mri->mri_Pens[MPEN_SHINE]);
    RectFill(rport, left + XX(33), top, right - XX(33), top + YY(YOFF));
    RectFill(rport, left + XX(25), top + YY(60), right - XX(25), bottom);
       
    #undef XX
    #undef YY
    #undef YOFF
    #undef XOFF
}

void ram_draw(struct MUI_RenderInfo *mri, LONG left, LONG top, LONG width, LONG height, LONG state)
{
    struct RastPort *rport = mri->mri_RastPort;
    int right, bottom;
    
    #define XX(x) left + x * (width - 1) / 100
    #define YY(y) top  + y * (height - 1) / 100
     
    right = left + width - 1;
    bottom = top + height - 1;

    SetAPen(rport, mri->mri_Pens[MPEN_SHADOW]);
    RectFill(rport, left, top, right, bottom);

    SetAPen(rport, mri->mri_Pens[MPEN_SHINE]);
    Move(rport, XX(33), YY(20));
    Draw(rport, XX(33), YY(80));
    
    Move(rport, XX(71), YY(20));
    Draw(rport, XX(61), YY(20));
    Draw(rport, XX(61), YY(80));
    Draw(rport, XX(71), YY(80));
            
    #undef XX
    #undef YY
}

void volume_draw(struct MUI_RenderInfo *mri, LONG left, LONG top, LONG width, LONG height, LONG state)
{
    struct RastPort *rport = mri->mri_RastPort;
    int right, bottom;
    
    #define XX(x) left + x * (width - 1) / 100
    #define YY(y) top  + y * (height - 1) / 100
     
    right = left + width - 1;
    bottom = top + height - 1;

    SetAPen(rport, mri->mri_Pens[MPEN_SHADOW]);
    RectFill(rport, left, top, right, bottom);

    SetAPen(rport, mri->mri_Pens[MPEN_SHINE]);
    Move(rport, XX(8), YY(20));
    Draw(rport, XX(8), YY(60));
    Draw(rport, XX(16), YY(80));
    Draw(rport, XX(24), YY(60));
    Draw(rport, XX(24), YY(20));
    
    Move(rport, XX(40), YY(20));
    Draw(rport, XX(60), YY(20));
    Draw(rport, XX(60), YY(80));
    Draw(rport, XX(40), YY(80));
    Draw(rport, XX(40), YY(20));
    
    Move(rport, XX(80), YY(20));
    Draw(rport, XX(80), YY(80));
    Draw(rport, XX(92), YY(80));
            
    #undef XX
    #undef YY
}

void network_draw(struct MUI_RenderInfo *mri, LONG left, LONG top, LONG width, LONG height, LONG state)
{
    struct RastPort *rport = mri->mri_RastPort;
    int right = left + width - 1;

    stddrawer_draw(mri, left, top, width, height);
    
    #define XX(x) x * (width - 1) / 100
    #define YY(y) y * (height - 1) / 100

    SetAPen(rport, mri->mri_Pens[MPEN_SHADOW]);
    Move(rport, left + XX(40), top + YY(75));
    Draw(rport, right - XX(40), top + YY(75));
    
    #undef XX
    #undef YY
}

void assign_draw(struct MUI_RenderInfo *mri, LONG left, LONG top, LONG width, LONG height, LONG state)
{
    struct RastPort *rport = mri->mri_RastPort;
    int right = left + width - 1;

    stddrawer_draw(mri, left, top, width, height);
    
    #define XX(x) x * (width - 1) / 100
    #define YY(y) y * (height - 1) / 100

    SetAPen(rport, mri->mri_Pens[MPEN_SHADOW]);
    Move(rport, left + XX(40), top + YY(75));
    Draw(rport, right - XX(40), top + YY(75));
    
    #undef XX
    #undef YY
}

void tape_play_draw(struct MUI_RenderInfo *mri, LONG left, LONG top, LONG width, LONG height, LONG state)
{
    struct RastPort *rport = mri->mri_RastPort;
    int x, half = height / 2;
    
    SetAPen(rport, mri->mri_Pens[MPEN_SHADOW]);

    for(x = 0; x < width; x++)
    {
    	int y = (width > 1) ?  half * x / (width - 1) : 1;
	
	RectFill(rport, left + width - 1 - x, top + half - y, left + width - 1 - x, top + half + y);
    }
}

void tape_playback_draw(struct MUI_RenderInfo *mri, LONG left, LONG top, LONG width, LONG height, LONG state)
{
    struct RastPort *rport = mri->mri_RastPort;
    int x, half = height / 2;
    
    SetAPen(rport, mri->mri_Pens[MPEN_SHADOW]);

    for(x = 0; x < width; x++)
    {
    	int y = (width > 1) ? half * x / (width - 1) : 1;
	
	RectFill(rport, left + x, top + half - y, left + x, top + half + y);
    }
}

void tape_pause_draw(struct MUI_RenderInfo *mri, LONG left, LONG top, LONG width, LONG height, LONG state)
{
    struct RastPort *rport = mri->mri_RastPort;

    SetAPen(rport, mri->mri_Pens[MPEN_SHADOW]);   
    RectFill(rport, left, top, left + width / 4, top + height - 1);
    RectFill(rport, left + width - 1 - width / 4, top, left + width - 1, top + height - 1);
}

void tape_stop_draw(struct MUI_RenderInfo *mri, LONG left, LONG top, LONG width, LONG height, LONG state)
{
    struct RastPort *rport = mri->mri_RastPort;

    SetAPen(rport, mri->mri_Pens[MPEN_TEXT]);
    RectFill(rport, left, top, left + width - 1, top + height - 1);
}

void tape_record_draw(struct MUI_RenderInfo *mri, LONG left, LONG top, LONG width, LONG height, LONG state)
{
    struct RastPort *rport = mri->mri_RastPort;
    int rx = (width - 1) / 2;
    int ry = (height - 1) / 2;    
    int x = rx, y = 0;     /* ellipse points */

    /* intermediate terms to speed up loop */
    int    	t1 = rx * rx, t2 = t1 << 1, t3 = t2 << 1;
    int    	t4 = ry * ry, t5 = t4 << 1, t6 = t5 << 1;
    int    	t7 = rx * t5, t8 = t7 << 1, t9 = 0L;
    int    	d1 = t2 - t7 + (t4 >> 1);    /* error terms */
    int    	d2 = (t1 >> 1) - t8 + t5;

    SetAPen(rport, mri->mri_Pens[MPEN_SHADOW]);

    while (d2 < 0)                  /* til slope = -1 */
    {
    	
        /* draw 4 points using symmetry */
	RectFill(rport, left + rx - x, top + ry + y, left + rx + x, top + ry + y);
	RectFill(rport, left + rx - x, top + ry - y, left + rx + x, top + ry - y);
	
        y++;            /* always move up here */
        t9 = t9 + t3;
        if (d1 < 0)     /* move straight up */
        {
            d1 = d1 + t9 + t2;
            d2 = d2 + t9;
        }
        else            /* move up and left */
        {
            x--;
            t8 = t8 - t6;
            d1 = d1 + t9 + t2 - t8;
            d2 = d2 + t9 + t5 - t8;
        }
    }

    do                              /* rest of top right quadrant */
    {
        /* draw 4 points using symmetry */
	RectFill(rport, left + rx - x, top + ry + y, left + rx + x, top + ry + y);
	RectFill(rport, left + rx - x, top + ry - y, left + rx + x, top + ry - y);

        x--;            /* always move left here */
        t8 = t8 - t6;
        if (d2 < 0)     /* move up and left */
        {
            y++;
            t9 = t9 + t3;
            d2 = d2 + t9 + t5 - t8;
        }
        else            /* move straight left */
        {
            d2 = d2 + t5 - t8;
        }

    } while (x >= 0);

}

void tape_up_draw(struct MUI_RenderInfo *mri, LONG left, LONG top, LONG width, LONG height, LONG state)
{
    struct RastPort *rport = mri->mri_RastPort;
    int y, half = width / 2;
    
    SetAPen(rport, mri->mri_Pens[MPEN_SHADOW]);

    for(y = 0; y < height; y++)
    {
    	int x = (height > 1) ? half * y / (height - 1) : 1;
	
	RectFill(rport, left + half - x, top + y, left + half + x, top + y);
    }
}

void tape_down_draw(struct MUI_RenderInfo *mri, LONG left, LONG top, LONG width, LONG height, LONG state)
{
    struct RastPort *rport = mri->mri_RastPort;
    int y, half = width / 2;
    
    SetAPen(rport, mri->mri_Pens[MPEN_SHADOW]);

    for(y = 0; y < height; y++)
    {
    	int x = (height > 1) ? half * y / (height - 1) : 1;
	
	RectFill(rport, left + half - x, top + height - 1 - y, left + half + x, top + height - 1 - y);
    }
}

struct vector_image
{
    int minwidth;
    int minheight;
    VECTOR_DRAW_FUNC draw_func;
};

static const struct vector_image vector_table[] =
{
    { 8 + 2 * ARROW_SPACING, 8 + 2 * ARROW_SPACING, arrowup_draw },
    { 8 + 2 * ARROW_SPACING, 8 + 2 * ARROW_SPACING, arrowdown_draw },
    { 8 + 2 * ARROW_SPACING, 8 + 2 * ARROW_SPACING, arrowleft_draw },
    { 8 + 2 * ARROW_SPACING, 8 + 2 * ARROW_SPACING, arrowright_draw },
    { 16, 10, checkbox_draw },
    { 10, 10, mx_draw },
    { 15,  8, cycle_draw },
    { 10, 11, popup_draw },
    { 10, 11, popfile_draw },
    { 10, 11, popdrawer_draw },

    { 13, 8, drawer_draw },
    { 13, 8, harddisk_draw },
    { 12, 8, disk_draw },
    { 14, 6, ram_draw },
    { 14, 6, volume_draw },
    { 13, 8, network_draw },
    { 13, 8, assign_draw },

    { 4, 7, tape_play_draw },
    { 4, 7, tape_playback_draw },
    { 6, 7, tape_pause_draw },
    { 6, 7, tape_stop_draw },
    { 7, 7, tape_record_draw },
    { 7, 4, tape_up_draw },
    { 7, 4, tape_down_draw },
};

#define VECTOR_TABLE_ENTRIES (sizeof(vector_table)/sizeof(vector_table[0]))

struct MUI_ImageSpec_intern *zune_imspec_create_vector(LONG vect)
{
    struct MUI_ImageSpec_intern *spec;

    if (!(vect >= 0 && vect < VECTOR_TABLE_ENTRIES))
	return NULL;

    if ((spec = mui_alloc_struct(struct MUI_ImageSpec_intern)))
    {
	spec->type = IST_VECTOR;
	spec->u.vect.type = vect;
	spec->u.vect.draw = vector_table[vect].draw_func;
    }
    return spec;
}

BOOL zune_imspec_vector_get_minmax(struct MUI_ImageSpec_intern *spec, struct MUI_MinMax *minmax)
{
    if (!spec || spec->type != IST_VECTOR)
        return FALSE;
    if ((spec->u.vect.type >= 0) && (spec->u.vect.type < VECTOR_TABLE_ENTRIES))
    {
        minmax->MinWidth = vector_table[spec->u.vect.type].minwidth;
        minmax->MinHeight = vector_table[spec->u.vect.type].minheight;
        minmax->DefWidth = minmax->MinWidth;
        minmax->DefHeight = minmax->MinHeight;
        minmax->MaxWidth = MUI_MAXMAX;
        minmax->MaxHeight = MUI_MAXMAX;
    }
    else
    {
        return FALSE;
    }
    return TRUE;
}
