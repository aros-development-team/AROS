/*
    Copyright © 2003, The AROS Development Team. 
    All rights reserved.
    
    $Id$
*/

#include <intuition/imageclass.h>
#include <proto/graphics.h>

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

#define SPACING 1
#define HSPACING 1
#define VSPACING 1

void arrowup_draw(struct MUI_RenderInfo *mri, LONG left, LONG top, LONG width, LONG height, LONG state)
{
    int cx;
    struct RastPort *rport = mri->mri_RastPort;

    SetAPen(rport, mri->mri_Pens[MPEN_TEXT]);

    cx = width / 2;

    Move(rport, left + HSPACING + 1, top + height - 1 - VSPACING);
    Draw(rport, left + width - cx, top + VSPACING);
    Move(rport, left + HSPACING, top + height - 1 - VSPACING);
    Draw(rport, left + width - cx - 1, top + VSPACING);

    Move(rport, left + width - 1 - HSPACING - 1, top + height - 1 - VSPACING);
    Draw(rport, left + cx - 1, top + VSPACING);
    Move(rport, left + width - 1 - HSPACING, top + height - 1 - VSPACING);
    Draw(rport, left + cx, top + VSPACING);
}

void arrowdown_draw(struct MUI_RenderInfo *mri, LONG left, LONG top, LONG width, LONG height, LONG state)
{
    int cx;
    struct RastPort *rport = mri->mri_RastPort;

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
}

void arrowleft_draw(struct MUI_RenderInfo *mri, LONG left, LONG top, LONG width, LONG height, LONG state)
{
    int cy;
    struct RastPort *rport = mri->mri_RastPort;

    SetAPen(rport, mri->mri_Pens[MPEN_TEXT]);

    cy = height / 2;

    Move(rport, left + width - 1 - HSPACING, top + VSPACING + 1);
    Draw(rport, left + HSPACING, top + height - cy);
    Move(rport, left + width - 1 - HSPACING, top + VSPACING);
    Draw(rport, left + HSPACING, top + height - cy - 1);

    Move(rport, left + width - 1 - HSPACING, top + height - 1- VSPACING - 1);
    Draw(rport, left + HSPACING, top + cy - 1);
    Move(rport, left + width - 1 - HSPACING, top + height - 1 - VSPACING);
    Draw(rport, left + HSPACING, top + cy);
}

void arrowright_draw(struct MUI_RenderInfo *mri, LONG left, LONG top, LONG width, LONG height, LONG state)
{
    int cy;
    struct RastPort *rport = mri->mri_RastPort;

    SetAPen(rport, mri->mri_Pens[MPEN_TEXT]);

    cy = height / 2;

    Move(rport, left + HSPACING, top + VSPACING + 1);
    Draw(rport, left + width - 1 - HSPACING, top + height - cy);
    Move(rport, left + HSPACING, top + VSPACING);
    Draw(rport, left + width - 1 - HSPACING, top + height - cy - 1);

    Move(rport, left + HSPACING, top + height - 1- VSPACING - 1);
    Draw(rport, left + width - 1 - HSPACING, top + cy - 1);
    Move(rport, left + HSPACING, top + height - 1 - VSPACING);
    Draw(rport, left + width - 1 - HSPACING, top + cy);
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
    int arrow_top;

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

    arrow_top = top + (3 * height / 4) - 3;
    /* prevent arrow touching bottom */
    if (arrow_top + 4 >= bottom-1)
	arrow_top = bottom - 5;

    RectFill(rport,right-7,top+1,right-6,arrow_top-1);

    /* The small arrow */
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
	Draw(rport,right - 7 + 1, arrow_top+2);
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

struct vector_image
{
    int minwidth;
    int minheight;
    VECTOR_DRAW_FUNC draw_func;
};

static struct vector_image vector_table[] =
{
    {10,8,arrowup_draw},
    {10,8,arrowdown_draw},
    {8,10,arrowleft_draw},
    {8,10,arrowright_draw},
    {16,10,checkbox_draw},
    {10,10,mx_draw},
    {15,8,cycle_draw},
    {10,11,popup_draw},
    {10,11,popfile_draw},
    {10,11,popdrawer_draw},
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
