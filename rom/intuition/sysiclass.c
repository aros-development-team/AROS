/*
    (C) 1997-2001 AROS - The Amiga Research OS
    $Id$

    Desc: Implementation of SYSICLASS
    Lang: english
*/

/**************************************************************************************************/

#include <exec/types.h>

#include <proto/intuition.h>
#include <proto/boopsi.h>
#include <intuition/classes.h>
#include <intuition/classusr.h>
#include <intuition/imageclass.h>
#include <intuition/intuition.h>
#include <intuition/intuitionbase.h>
#include <intuition/screens.h>

#include <proto/graphics.h>
#include <graphics/rastport.h>

#include <proto/utility.h>
#include <utility/tagitem.h>

#include <proto/alib.h>

#include <aros/asmcall.h>
#include "intuition_intern.h"


#include "gadgets.h" /* Some handy rendering funtions */

#undef DEBUG
#define DEBUG 0
#include <aros/debug.h>

/**************************************************************************************************/

#define DEFSIZE_WIDTH  14
#define DEFSIZE_HEIGHT 14

#if 0 /* stegerg: ???? */

/* Image data */
#define ARROWDOWN_WIDTH    18
#define ARROWDOWN_HEIGHT   11

UWORD ArrowDown0Data[] =
{
    0x0000, 0x4000, 0x0000, 0x4000, 0x0000, 0x4000, 0x0C0C, 0x4000,
    0x0738, 0x4000, 0x03F0, 0x4000, 0x01E0, 0x4000, 0x00C0, 0x4000,
    0x0000, 0x4000, 0x0000, 0x4000, 0x7FFF, 0xC000,

    0xFFFF, 0x8000, 0x8000, 0x0000, 0x8000, 0x0000, 0x8000, 0x0000,
    0x8000, 0x0000, 0x8000, 0x0000, 0x8000, 0x0000, 0x8000, 0x0000,
    0x8000, 0x0000, 0x8000, 0x0000, 0x8000, 0x0000,
};

UWORD ArrowDown1Data[] =
{
    0xFFFF, 0x8000, 0x8000, 0x0000, 0x8000, 0x0000, 0x8C0C, 0x0000,
    0x8738, 0x0000, 0x83F0, 0x0000, 0x81E0, 0x0000, 0x80C0, 0x0000,
    0x8000, 0x0000, 0x8000, 0x0000, 0x8000, 0x0000,

    0x0000, 0x4000, 0x0000, 0x4000, 0x0000, 0x4000, 0x0000, 0x4000,
    0x0000, 0x4000, 0x0000, 0x4000, 0x0000, 0x4000, 0x0000, 0x4000,
    0x0000, 0x4000, 0x0000, 0x4000, 0x7FFF, 0xC000,
};

#define ARROWUP_WIDTH	 18
#define ARROWUP_HEIGHT	 11

UWORD ArrowUp0Data[] =
{
    0x0000, 0x4000, 0x0000, 0x4000, 0x0000, 0x4000, 0x00C0, 0x4000,
    0x01E0, 0x4000, 0x03F0, 0x4000, 0x0738, 0x4000, 0x0C0C, 0x4000,
    0x0000, 0x4000, 0x0000, 0x4000, 0x7FFF, 0xC000,

    0xFFFF, 0x8000, 0x8000, 0x0000, 0x8000, 0x0000, 0x8000, 0x0000,
    0x8000, 0x0000, 0x8000, 0x0000, 0x8000, 0x0000, 0x8000, 0x0000,
    0x8000, 0x0000, 0x8000, 0x0000, 0x8000, 0x0000,
};

UWORD ArrowUp1Data[] =
{
    0xFFFF, 0x8000, 0x8000, 0x0000, 0x8000, 0x0000, 0x80C0, 0x0000,
    0x81E0, 0x0000, 0x83F0, 0x0000, 0x8738, 0x0000, 0x8C0C, 0x0000,
    0x8000, 0x0000, 0x8000, 0x0000, 0x8000, 0x0000,

    0x0000, 0x4000, 0x0000, 0x4000, 0x0000, 0x4000, 0x0000, 0x4000,
    0x0000, 0x4000, 0x0000, 0x4000, 0x0000, 0x4000, 0x0000, 0x4000,
    0x0000, 0x4000, 0x0000, 0x4000, 0x7FFF, 0xC000,
};

#define ARROWLEFT_WIDTH    11
#define ARROWLEFT_HEIGHT   16

UWORD ArrowLeft0Data[] =
{
    0x0000, 0x0020, 0x0020, 0x0120, 0x0320, 0x0620, 0x0E20, 0x1C20,
    0x1C20, 0x0E20, 0x0620, 0x0320, 0x0120, 0x0020, 0x0020, 0xFFE0,

    0xFFE0, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000,
    0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x0000,
};

UWORD ArrowLeft1Data[] =
{
    0xFFE0, 0x8000, 0x8000, 0x8100, 0x8300, 0x8600, 0x8E00, 0x9C00,
    0x9C00, 0x8E00, 0x8600, 0x8300, 0x8100, 0x8000, 0x8000, 0x0000,

    0x0000, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020,
    0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0xFFE0,
};

#define ARROWRIGHT_WIDTH    11
#define ARROWRIGHT_HEIGHT   16

UWORD ArrowRight0Data[] =
{
    0x0000, 0x0020, 0x0020, 0x1020, 0x1820, 0x0C20, 0x0E20, 0x0720,
    0x0720, 0x0E20, 0x0C20, 0x1820, 0x1020, 0x0020, 0x0020, 0xFFE0,

    0xFFE0, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000,
    0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x0000,
};

UWORD ArrowRight1Data[] =
{
    0xFFE0, 0x8000, 0x8000, 0x9000, 0x9800, 0x8C00, 0x8E00, 0x8700,
    0x8700, 0x8E00, 0x8C00, 0x9800, 0x9000, 0x8000, 0x8000, 0x0000,

    0x0000, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020,
    0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0xFFE0,
};

#endif

/**************************************************************************************************/

/* Some handy transparent base class object casting defines.
 */
#define IM(o) ((struct Image *)o)

static UWORD getbgpen(ULONG state, UWORD *pens);
static void renderimageframe(struct RastPort *rp, ULONG which, ULONG state, UWORD *pens,
			     WORD left, WORD top, WORD width, WORD height,
			     struct IntuitionBase *IntuitionBase);

#undef IntuitionBase
#define IntuitionBase	((struct IntuitionBase *)(cl->cl_UserData))


/**************************************************************************************************/

struct SysIData
{
    ULONG 		type;
    struct DrawInfo 	*dri;
    struct Image 	*frame;
    UWORD 		flags;
};

#define SYSIFLG_GADTOOLS 1
#define SYSIFLG_NOBORDER 2

/**************************************************************************************************/

/* Some handy drawing functions */

void draw_thick_line(Class *cl, struct RastPort *rport,
                     LONG x1, LONG y1, LONG x2, LONG y2,
                     UWORD thickness)
{
    Move(rport, x1, y1);
    Draw(rport, x2, y2);
    /* Georg Steger */
    Move(rport, x1 + 1, y1);
    Draw(rport, x2 + 1, y2);
}

/**************************************************************************************************/

BOOL sysi_setnew(Class *cl, Object *obj, struct opSet *msg)
{
    struct SysIData *data = INST_DATA(cl, obj);
    struct TagItem *taglist, *tag;
    struct TextFont *reffont = NULL;
    
    BOOL unsupported = FALSE;

    taglist = msg->ops_AttrList;
    while ((tag = NextTagItem((const struct TagItem **)&taglist)))
    {
	switch(tag->ti_Tag)
	{
	case SYSIA_DrawInfo:
	    data->dri = (struct DrawInfo *)tag->ti_Data;
	    reffont = data->dri->dri_Font;
	    break;
	    
	case SYSIA_Which:
	    data->type = tag->ti_Data;
	    
	    D(bug("SYSIA_Which type: %d\n", data->type));
	    
            switch (tag->ti_Data)
            {

#warning if IA_Width, IA_Height was not specified sysiclass should choose size depending on drawinfo (screen resolution)

            case LEFTIMAGE:
            case UPIMAGE:
            case RIGHTIMAGE:
            case DOWNIMAGE:
            case CHECKIMAGE:
            case MXIMAGE:
            case DEPTHIMAGE:
	    case SDEPTHIMAGE:
            case ZOOMIMAGE:
            case CLOSEIMAGE:
            case SIZEIMAGE:
            case MENUCHECK:
            case AMIGAKEY:
	        break;
		
            default:
                unsupported = TRUE;
                break;
            }
	    break;
	    
	case SYSIA_ReferenceFont:
	    if (tag->ti_Data) reffont = (struct TextFont *)tag->ti_Data;
	    break;
	    
	case SYSIA_Size:
#warning FIXME: Missing Tag
	    break;
	
	/* private tags */
	
	case SYSIA_WithBorder:
	    if (tag->ti_Data == FALSE)
	    {
	    	data->flags |= SYSIFLG_NOBORDER;
	    }
	    break;
	
	case SYSIA_Style:
	    if (tag->ti_Data == SYSISTYLE_GADTOOLS)
	    {
	    	data->flags |= SYSIFLG_GADTOOLS;
	    }
	    break;
	    
	} /* switch(tag->ti_Tag) */
	
    } /* while ((tag = NextTagItem(&taglist))) */

    D(bug("dri: %p, unsupported: %d\n", data->dri, unsupported));

    if ((!data->dri) || (unsupported))
	return FALSE;

    switch(data->type)
    {
        case LEFTIMAGE:
        case UPIMAGE:
        case RIGHTIMAGE:
        case DOWNIMAGE:
	    if (IM(obj)->Width  == 0) IM(obj)->Width  = DEFSIZE_WIDTH;
	    if (IM(obj)->Height == 0) IM(obj)->Height = DEFSIZE_HEIGHT; 
            break;

        case DEPTHIMAGE:
	case SDEPTHIMAGE:
        case ZOOMIMAGE:
        case CLOSEIMAGE:
        case SIZEIMAGE:
	    if (IM(obj)->Width  == 0) IM(obj)->Width  = DEFSIZE_WIDTH;
	    if (IM(obj)->Height == 0) IM(obj)->Height = DEFSIZE_HEIGHT;
	    break;

        case MENUCHECK:
	    if (IM(obj)->Width  == 0) IM(obj)->Width  = reffont->tf_XSize * 3 / 2;
	    if (IM(obj)->Height == 0) IM(obj)->Height = reffont->tf_YSize;
	    break;
	    
        case AMIGAKEY:
	    #if MENUS_AMIGALOOK
	        if (IM(obj)->Width  == 0) IM(obj)->Width  = reffont->tf_XSize * 2;
	        if (IM(obj)->Height == 0) IM(obj)->Height = reffont->tf_YSize;
	    #else
	        if (IM(obj)->Width  == 0) IM(obj)->Width  = reffont->tf_XSize * 2;
	        if (IM(obj)->Height == 0) IM(obj)->Height = reffont->tf_YSize + 1;
	    #endif
	    break;
    
    } /* switch(data->type) */
    
    return TRUE;
}

/**************************************************************************************************/

Object *sysi_new(Class *cl, Class *rootcl, struct opSet *msg)
{
    struct SysIData *data;
    Object *obj;

    D(bug("sysi_new()\n"));
    obj = (Object *)DoSuperMethodA(cl, (Object *)rootcl, (Msg)msg);
    if (!obj)
        return NULL;
	
    D(bug("sysi_new,: obj=%p\n", obj));

    data = INST_DATA(cl, obj);
    data->type = 0L;
    data->dri = NULL;
    data->frame = NULL;
    data->flags = 0;
    if (!sysi_setnew(cl, obj, (struct opSet *)msg))
    {
        CoerceMethod(cl, obj, OM_DISPOSE);
        return NULL;
    }

    D(bug("sysi_setnew called successfully\n"));

    switch (data->type)
    {
    case CHECKIMAGE:
    {
        struct TagItem tags[] = {
          {IA_FrameType, FRAME_BUTTON},
          {IA_EdgesOnly, FALSE},
          {TAG_MORE, 0L}
        };

        tags[2].ti_Data = (IPTR)msg->ops_AttrList;
        data->frame = NewObjectA(NULL, FRAMEICLASS, tags);
        if (!data->frame)
        {
            CoerceMethod(cl, obj, OM_DISPOSE);
            return NULL;
        }
        break;
    }

    /* Just to prevent it from reaching default: */
    case MXIMAGE:
    case LEFTIMAGE:
    case UPIMAGE:
    case RIGHTIMAGE:
    case DOWNIMAGE:
    
    case SDEPTHIMAGE:
    case DEPTHIMAGE:
    case ZOOMIMAGE:
    case CLOSEIMAGE:
    case SIZEIMAGE:
    
    case MENUCHECK:
    case AMIGAKEY:
        break;
    
    default:
        CoerceMethod(cl, obj, OM_DISPOSE);
        return NULL;
    }

    return obj;
}

/**************************************************************************************************/

void sysi_draw(Class *cl, Object *obj, struct impDraw *msg)
{
    struct SysIData *data = INST_DATA(cl, obj);
    struct RastPort *rport = msg->imp_RPort;
    WORD left = IM(obj)->LeftEdge + msg->imp_Offset.X;
    WORD top = IM(obj)->TopEdge + msg->imp_Offset.Y;
    UWORD width = IM(obj)->Width;
    UWORD height = IM(obj)->Height;
    WORD right = left + width - 1;
    WORD bottom = top + height - 1;

    SetDrMd(rport, JAM1);
    
    switch(data->type)
    {
    case CHECKIMAGE:
    {
	WORD h_spacing = width / 4;
	WORD v_spacing = height / 4;
	
        /* Draw frame */
        DrawImageState(rport, data->frame,
                       msg->imp_Offset.X, msg->imp_Offset.Y,
                       IDS_NORMAL, data->dri);

        /* Draw checkmark (only if image is in selected state) */
        if (msg->imp_State == IDS_SELECTED)
        {
	    left += h_spacing;right -= h_spacing;width -= h_spacing * 2;
	    top += v_spacing;bottom -= v_spacing;height -= v_spacing * 2;
	    
            SetAPen(rport, data->dri->dri_Pens[SHADOWPEN]);

	    #if 0
            draw_thick_line(cl, rport, left, top + height/2, left + width/3, bottom, 0);
            draw_thick_line(cl, rport, left + width/3, bottom, right - 2, top, 0);
            Move(rport, right -1 , top);
            Draw(rport, right, top);
	    #else
	    draw_thick_line(cl, rport, left, top + height / 3 , left, bottom, 0);
	    draw_thick_line(cl, rport, left + 1, bottom, right - 1, top, 0);
	    #endif

        }
        break;
    }
    case MXIMAGE:
    {
        BOOL selected = FALSE;
	WORD col1 = SHINEPEN;
	WORD col2 = SHADOWPEN;

	if ((msg->imp_State == IDS_SELECTED) || (msg->imp_State == IDS_INACTIVESELECTED))
	{
	    col1 = SHADOWPEN;
	    col2 = SHINEPEN;
	    selected = TRUE;
	}
	
	SetAPen(rport, data->dri->dri_Pens[BACKGROUNDPEN]);
	RectFill(rport, left, top, right, bottom);

#if 0
    	/* THICK MX IMAGE */
		
	SetAPen(rport, data->dri->dri_Pens[col1]);
	RectFill(rport, left + 2, top, right - 3, top + 1);
	RectFill(rport, left + 1, top + 2, left + 2, top + 3);
	RectFill(rport, left, top + 4, left + 1, bottom - 4);
	RectFill(rport, left + 1, bottom - 3, left + 2, bottom - 2);
	RectFill(rport, left + 2, bottom - 1, left + 2, bottom);
	
	SetAPen(rport, data->dri->dri_Pens[col2]);
	RectFill(rport, right - 2, top, right - 2, top + 1);
	RectFill(rport, right - 2, top + 2, right - 1, top + 3);
	RectFill(rport, right - 1, top + 4, right, bottom - 4);
	RectFill(rport, right - 2, bottom - 3, right - 1, bottom - 2);
	RectFill(rport, left + 3, bottom - 1, right - 2, bottom);
	
        if (selected)
        {
	    left += 4;right -= 4;width -= 8;
	    top += 4;bottom -= 4;height -= 8;
	    
            SetAPen(rport, data->dri->dri_Pens[FILLPEN]);
	    if ((width >= 3) && (height >= 3))
	    {
        	RectFill(rport, left + 1, top, right - 1, top);
		RectFill(rport, left, top + 1, right, bottom - 1);
		RectFill(rport, left + 1, bottom, right - 1, bottom);
	    } else {
	        RectFill(rport, left, top, right, bottom);
	    }
        }
#else
    	/* THIN MX IMAGE */
	
	SetAPen(rport, data->dri->dri_Pens[col1]);
	RectFill(rport, left + 3, top, right - 3, top);
	WritePixel(rport, left + 2, top + 1);
	RectFill(rport, left + 1, top + 2, left + 1, top + 3);
	RectFill(rport, left, top + 4, left, bottom - 4);
	RectFill(rport, left + 1, bottom - 3, left + 1, bottom - 2);
	WritePixel(rport, left + 2, bottom - 1);
	
	SetAPen(rport, data->dri->dri_Pens[col2]);
	WritePixel(rport, right - 2, top + 1);
	RectFill(rport, right - 1, top + 2, right - 1, top + 3);
	RectFill(rport, right, top + 4, right, bottom - 4);
	RectFill(rport, right - 1, bottom - 3, right - 1, bottom - 2);
	WritePixel(rport, right - 2, bottom - 1);
	RectFill(rport, left + 3, bottom, right - 3, bottom);
	
        if (selected)
        {
	    left += 3;right -= 3;width -= 6;
	    top += 3;bottom -= 3;height -= 6;
	    
            SetAPen(rport, data->dri->dri_Pens[FILLPEN]);
	    if ((width >= 5) && (height >= 5))
	    {
	    	RectFill(rport, left, top + 2, left, bottom - 2);
		RectFill(rport, left + 1, top + 1, left + 1, bottom - 1);
		RectFill(rport, left + 2, top, right - 2, bottom);
		RectFill(rport, right - 1, top + 1, right - 1, bottom - 1);
		RectFill(rport, right, top + 2, right, bottom - 2);
 	    } else {
	        RectFill(rport, left, top, right, bottom);
	    }
        }
	
#endif
        break;
    }
    
    #define SPACING 3
    /* Georg Steger */
    #define HSPACING 3
    #define VSPACING 3
    
    case LEFTIMAGE:
    {
    	WORD cy;
	
	if (!(data->flags & SYSIFLG_NOBORDER))
	{
	    renderimageframe(rport, LEFTIMAGE, msg->imp_State, data->dri->dri_Pens,
	    		     left, top, width, height, IntuitionBase);
	    left++;top++;right--;bottom--;
	    width -= 2;height -= 2; 
	}

	if (data->flags & SYSIFLG_GADTOOLS)
	{
	    SetAPen(rport, data->dri->dri_Pens[SHADOWPEN]);

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
	else
	{
	    WORD i;
	    
	    SetAPen(rport, getbgpen(msg->imp_State, data->dri->dri_Pens));
	    
	    RectFill(rport, left, top, right, bottom);
	    
	    left += HSPACING; top += VSPACING;
	    width -= HSPACING * 2;
	    height -= VSPACING * 2;
	    
	    right = left + width - 1;
	    bottom = top + height - 1;
	    
	    cy = (height + 1) / 2;
	    
	    SetAPen(rport, data->dri->dri_Pens[SHADOWPEN]);
	    
	    for(i = 0; i < cy; i++)
	    {
	    	RectFill(rport, left + (cy - i - 1) * width / cy,
				top + i,
				right - i * width / cy / 2,
				top + i);
		RectFill(rport, left + (cy - i - 1) * width / cy,
				bottom - i,
				right - i * width / cy / 2,
				bottom - i);
	    }
	    
	}
    	break;
    }

    case UPIMAGE:
    {
    	WORD cx;
	
	if (!(data->flags & SYSIFLG_NOBORDER))
	{
	    renderimageframe(rport, UPIMAGE, msg->imp_State, data->dri->dri_Pens,
	    		     left, top, width, height, IntuitionBase);
	    left++;top++;right--;bottom--;
	    width -= 2;height -= 2; 
	}

	if (data->flags & SYSIFLG_GADTOOLS)
	{
    	    SetAPen(rport, data->dri->dri_Pens[SHADOWPEN]);

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
	else
	{
	    WORD i;
	    
	    SetAPen(rport, getbgpen(msg->imp_State, data->dri->dri_Pens));
	    
	    RectFill(rport, left, top, right, bottom);
	    	    
	    left += HSPACING; top += VSPACING;
	    width -= HSPACING * 2;
	    height -= VSPACING * 2;
	    
	    right = left + width - 1;
	    bottom = top + height - 1;

	    cx = (width + 1) / 2;
	    	    
	    SetAPen(rport, data->dri->dri_Pens[SHADOWPEN]);
	    
	    for(i = 0; i < cx; i++)
	    {
	    	RectFill(rport, left + i,
				top + (cx - i - 1) * height / cx,
				left + i,
				bottom - i * height / cx / 2);
	    	RectFill(rport, right - i,
				top + (cx - i - 1) * height / cx,
				right - i,
				bottom - i * height / cx / 2);
	    }
	    
	}
    	break;
    }
    
    case RIGHTIMAGE:
    {
    	WORD cy;
	
	if (!(data->flags & SYSIFLG_NOBORDER))
	{
	    renderimageframe(rport, RIGHTIMAGE, msg->imp_State, data->dri->dri_Pens,
	    		     left, top, width, height, IntuitionBase);
	    left++;top++;right--;bottom--;
	    width -= 2;height -= 2; 
	}

	if (data->flags & SYSIFLG_GADTOOLS)
	{
    	    SetAPen(rport, data->dri->dri_Pens[SHADOWPEN]);

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
	else
	{
	    WORD i;
	    
	    SetAPen(rport, getbgpen(msg->imp_State, data->dri->dri_Pens));
	    
	    RectFill(rport, left, top, right, bottom);
	    
	    left += HSPACING; top += VSPACING;
	    width -= HSPACING * 2;
	    height -= VSPACING * 2;
	    
	    right = left + width - 1;
	    bottom = top + height - 1;
	    
	    cy = (height + 1) / 2;
	    
	    SetAPen(rport, data->dri->dri_Pens[SHADOWPEN]);
	    
	    for(i = 0; i < cy; i++)
	    {
	    	RectFill(rport, left + i * width / cy / 2,
				top + i,
				right - (cy - i - 1) * width / cy,
				top + i);
		RectFill(rport, left + i * width / cy / 2,
				bottom - i,
				right - (cy - i - 1) * width / cy,
				bottom - i);
	    }
	    
	}	
    	break;
    }

    case DOWNIMAGE:
    {
    	WORD cx;
	
	if (!(data->flags & SYSIFLG_NOBORDER))
	{
	    renderimageframe(rport, DOWNIMAGE, msg->imp_State, data->dri->dri_Pens,
	    		     left, top, width, height, IntuitionBase);
	    left++;top++;right--;bottom--;
	    width -= 2;height -= 2; 
	}

	if (data->flags & SYSIFLG_GADTOOLS)
	{
    	    SetAPen(rport, data->dri->dri_Pens[SHADOWPEN]);

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
	else
	{
	    WORD i;
	    
	    SetAPen(rport, getbgpen(msg->imp_State, data->dri->dri_Pens));
	    
	    RectFill(rport, left, top, right, bottom);
	    	    
	    left += HSPACING; top += VSPACING;
	    width -= HSPACING * 2;
	    height -= VSPACING * 2;
	    
	    right = left + width - 1;
	    bottom = top + height - 1;

	    cx = (width + 1) / 2;
	    	    
	    SetAPen(rport, data->dri->dri_Pens[SHADOWPEN]);
	    
	    for(i = 0; i < cx; i++)
	    {
	    	RectFill(rport, left + i,
				top + i * height / cx / 2,
				left + i,
				bottom - (cx - i - 1) * height / cx);
	    	RectFill(rport, right - i,
				top + i * height / cx / 2,
				right -  i,
				bottom - (cx - i - 1) * height / cx);
	    	
	    }
	    
	}
    	break;
    }

    case DEPTHIMAGE:
    case SDEPTHIMAGE: {
        UWORD *pens = data->dri->dri_Pens;
	UWORD bg;

        WORD h_spacing; 
	WORD v_spacing;
		
	if (!(data->flags & SYSIFLG_NOBORDER))
	{
	    renderimageframe(rport, DEPTHIMAGE, msg->imp_State, pens,
	    		     left, top, width, height, IntuitionBase);
	    left += 2;top++;
	    width -= 3;height -= 2; 
	}
	
	h_spacing = width / 6;
	v_spacing = height / 6;
	
	if (data->type == DEPTHIMAGE)
	{
	    bg = getbgpen(msg->imp_State, pens);
	} else {
	    bg = pens[BACKGROUNDPEN];
	}
	
	/* Clear background into correct color */
	SetAPen(rport, bg);
	RectFill(rport, left, top, left + width - 1, top + height - 1);
	
	/* Draw a image of two partly overlapped tiny windows, 
	*/
	
	left += h_spacing;
	top  += v_spacing;
	
	width  -= h_spacing * 2;
	height -= v_spacing * 2;
	
	right  = left + width  - 1;
	bottom = top  + height - 1;
	
	/* Render top left window  */
	
	SetAPen(rport, pens[SHADOWPEN]);
        drawrect(rport
		, left
		, top
		, right - (width / 3 )
		, bottom - (height / 3)
		, IntuitionBase);
	
	
	/* Fill top left window (inside of the frame above) */
	
	if ((msg->imp_State != IDS_INACTIVENORMAL) &&
	    (data->type != SDEPTHIMAGE))
	{
	    SetAPen(rport,pens[BACKGROUNDPEN]);
	    RectFill(rport
	            , left		    + 1
		    , top		    + 1
		    , right - (width / 3)   - 1
		    , bottom - (height / 3) - 1);
		 
	}
	
	/* Render bottom right window  */
	SetAPen(rport, pens[SHADOWPEN]);
	drawrect(rport
		, left + (width / 3)
		, top + (height / 3)
		, right
		, bottom
		, IntuitionBase);
		
	/* Fill bottom right window (inside of the frame above) */
	SetAPen(rport, pens[((msg->imp_State == IDS_INACTIVENORMAL) &&
			     (data->type != SDEPTHIMAGE)) ? BACKGROUNDPEN : SHINEPEN]);
	RectFill(rport
		, left + (width / 3) 	+ 1
		, top + (height / 3)	+ 1
		, right 		- 1
		, bottom 		- 1);

        
	if (msg->imp_State == IDS_SELECTED)
	{
	    /* Re-Render top left window  */

	    SetAPen(rport, pens[SHADOWPEN]);
            drawrect(rport
		    , left
		    , top
		    , right - (width / 3 )
		    , bottom - (height / 3)
		    , IntuitionBase);
	}
        break; }
	
    	
         
	
    case CLOSEIMAGE: {
	UWORD *pens = data->dri->dri_Pens;
	WORD h_spacing;
	WORD v_spacing;

	if (!(data->flags & SYSIFLG_NOBORDER))
	{
	    renderimageframe(rport, CLOSEIMAGE, msg->imp_State, pens,
	    		     left, top, width, height, IntuitionBase);
	    left++ ;top++;
	    width -= 3; /* 3 is no bug! */
	    height -= 2; 
	}

	right = left + width - 1;
	bottom = top + height - 1;
	h_spacing = width * 4 / 10;
	v_spacing = height * 3 / 10;
	
	SetAPen(rport, getbgpen(msg->imp_State, pens));
	RectFill(rport,left, top, right, bottom);
	
	left += h_spacing;right -= h_spacing;
	top += v_spacing;bottom -= v_spacing;
	
	SetAPen(rport, pens[SHADOWPEN]);
	RectFill(rport, left, top, right, bottom);
	
	left++;top++;
	right--;bottom--;
	
	SetAPen(rport, pens[(msg->imp_State == IDS_NORMAL) ? SHINEPEN : BACKGROUNDPEN]);
	RectFill(rport, left, top, right, bottom);

        break; }
	
    case ZOOMIMAGE: {
        UWORD *pens = data->dri->dri_Pens;
	UWORD bg;
	WORD h_spacing;
	WORD v_spacing;

	if (!(data->flags & SYSIFLG_NOBORDER))
	{
	    renderimageframe(rport, ZOOMIMAGE, msg->imp_State, pens,
	    		     left, top, width, height, IntuitionBase);
	    left += 2;top++;
	    width -= 3;height -= 2; 
	}

	right = left + width - 1;
	bottom = top + height - 1 ;
	h_spacing = width / 6;
	v_spacing = height / 6;
	
	bg = getbgpen(msg->imp_State, pens);
	
	/* Clear background into correct color */
	SetAPen(rport, bg);
	RectFill(rport, left, top, right, bottom);
	
	left += h_spacing;right -= h_spacing;
	top += v_spacing;bottom -= v_spacing;
	
	SetAPen(rport, pens[SHADOWPEN]);
	RectFill(rport, left, top, right, bottom);

	SetAPen(rport, pens[(msg->imp_State == IDS_SELECTED) ? SHINEPEN :
						 	       (msg->imp_State == IDS_NORMAL) ? FILLPEN : BACKGROUNDPEN]);
	RectFill(rport, left + 1, top + 1, right - 1, bottom - 1);

	right = left + (right - left + 1) / 2;
	bottom = top + (bottom - top + 1) / 2;
	
	if (right - left <  4) right = left + 4;
	
	SetAPen(rport, pens[SHADOWPEN]);
	RectFill(rport, left, top, right, bottom);
	
	left += 2;right -= 2;
	top += 1; bottom -= 1;
	
	SetAPen(rport, pens[(msg->imp_State == IDS_SELECTED) ? FILLPEN :
							       (msg->imp_State == IDS_NORMAL) ? SHINEPEN : BACKGROUNDPEN]);
	RectFill(rport,left, top, right, bottom);
        break; }
	
    
    case SIZEIMAGE: {
        UWORD *pens = data->dri->dri_Pens;
	UWORD bg;
        WORD h_spacing;
	WORD v_spacing;
	
	WORD right, bottom, x, y;

	if (!(data->flags & SYSIFLG_NOBORDER))
	{
	    renderimageframe(rport, SIZEIMAGE, msg->imp_State, pens,
	    		     left, top, width, height, IntuitionBase);
	    left++;top++;
	    width -= 2;height -= 2; 
	}

        h_spacing = width  / 5;
	v_spacing = height / 5;
	
	bg = getbgpen(msg->imp_State, pens);
	
	/* Clear background into correct color */
	SetAPen(rport, bg);
	RectFill(rport, left, top, left + width - 1, top + height - 1);
	
	/* A triangle image 	*/
	
	left += h_spacing;
	top  += v_spacing;
	
	right  = left + width  - 1 - (h_spacing * 2);
	bottom = top  + height - 1 - (v_spacing * 2);
	
	width  = right  - left + 1;
	height = bottom - top  + 1;
	
	if (msg->imp_State != IDS_INACTIVENORMAL)
	{
	    SetAPen(rport, pens[SHINEPEN]);
	    for(y = top; y <= bottom; y++)
	    {
	    	x = left + (bottom - y) * width / height;
		RectFill(rport, x, y, right, y);
	    }
	}
	
	SetAPen(rport, pens[SHADOWPEN]);
	/* Draw triangle border */
	Move(rport, left, bottom);
	Draw(rport, right, top);
	Draw(rport, right, bottom);
	Draw(rport, left, bottom);
	
	break; }
	
    case MENUCHECK: {
        UWORD *pens = data->dri->dri_Pens;
	
    #if MENUS_AMIGALOOK
    	SetAPen(rport, pens[BARBLOCKPEN]);
    #else
	SetAPen(rport, pens[(msg->imp_State == IDS_SELECTED) ? FILLPEN : BACKGROUNDPEN]);
    #endif
	RectFill(rport, left, top, right, bottom);

	SetAPen(rport, pens[BARDETAILPEN]);
	draw_thick_line(cl, rport, left + 1, top + height / 3 , left + 1, bottom, 0);
	draw_thick_line(cl, rport, left + 2, bottom, right - 2, top, 0);
	
    	break; }

    case AMIGAKEY: {
        UWORD *pens = data->dri->dri_Pens;

    #if MENUS_AMIGALOOK
        struct TextFont *oldfont;
	UBYTE oldstyle;
	
    	SetAPen(rport, pens[BARDETAILPEN]);
    #else
	SetAPen(rport, pens[SHINEPEN]);
    #endif

	RectFill(rport, left, top, right, bottom);

    #if MENUS_AMIGALOOK
	SetAPen(rport, pens[BARBLOCKPEN]);

	oldfont = rport->Font;
	oldstyle = rport->AlgoStyle;
	
	SetFont(rport, GfxBase->DefaultFont);
	SetSoftStyle(rport, FSF_ITALIC, AskSoftStyle(rport));

	Move(rport, left + (width - rport->TxWidth) / 2,
		    top  + (height - rport->TxHeight) / 2 + rport->TxBaseline);
	Text(rport, "A", 1);
	
	SetSoftStyle(rport, oldstyle, AskSoftStyle(rport));
	SetFont(rport, oldfont);

	SetAPen(rport, pens[BARBLOCKPEN]);
    #else
	SetAPen(rport, pens[SHADOWPEN]);

	RectFill(rport, left + 1, top, right - 1, top);
	RectFill(rport, right, top + 1, right, bottom - 1);
	RectFill(rport, left + 1, bottom, right - 1, bottom);
	RectFill(rport, left, top + 1, left, bottom - 1);

        SetAPen(rport, pens[BACKGROUNDPEN]);	
	RectFill(rport, left + 1, bottom - 1, right - 1, bottom - 1);
	RectFill(rport, right - 1, top + 1, right - 1, bottom - 2);
	
	RectFill(rport, left + 2, top + 2, left + 4, top + 2);
	RectFill(rport, left + 2, top + 3, left + 2, top + 4);
	
	SetAPen(rport, pens[SHADOWPEN]);
	RectFill(rport, left + 2, bottom - 2, right - 2, bottom - 2);
	RectFill(rport, right - 2, top + 2, right - 2, bottom - 4);
	
	{
	    WORD a_size   = height - 7;	    
	    WORD a_left   = left + 5;
	    WORD a_top    = top + 2;
	    WORD a_right  = a_left + a_size;
	    WORD a_bottom = a_top + a_size;
	    
	    Move(rport, a_left, a_bottom);
	    Draw(rport, a_right, a_top);
	    Draw(rport, a_right, a_bottom);
	    Move(rport, a_right - 1, a_top + 1);
	    Draw(rport, a_right - 1, a_bottom);
	}	 

        SetAPen(rport, pens[(msg->imp_State == IDS_SELECTED) ? FILLPEN : BACKGROUNDPEN]);
    #endif
	WritePixel(rport, left, top);
	WritePixel(rport, right, top);
	WritePixel(rport, right, bottom);
	WritePixel(rport, left, bottom);

   	break; }

    } /* switch (image type) */
    return;
}

/**************************************************************************************************/

AROS_UFH3S(IPTR, dispatch_sysiclass,
    AROS_UFHA(Class *,  cl,  A0),
    AROS_UFHA(Object *, obj, A2),
    AROS_UFHA(Msg,      msg, A1)
)
{
    IPTR retval = 0UL;
    struct SysIData *data;

    switch (msg->MethodID)
    {
    case OM_NEW:
        retval = (IPTR)sysi_new(cl, (Class *)obj, (struct opSet *)msg);
        break;

    case OM_DISPOSE:
        data = INST_DATA(cl, obj);
        DisposeObject(data->frame);
        retval = DoSuperMethodA(cl, obj, msg);
        break;

    case OM_SET:
        data = INST_DATA(cl, obj);
        if (data->frame)
            DoMethodA((Object *)data->frame, msg);
        retval = DoSuperMethodA(cl, obj, msg);
        break;

    case IM_DRAW:
        sysi_draw(cl, obj, (struct impDraw *)msg);
        break;

    default:
        retval = DoSuperMethodA(cl, obj, msg);
        break;
    }

    return retval;
}

/**************************************************************************************************/

#undef IntuitionBase

/**************************************************************************************************/

/* Initialize our class. */
struct IClass *InitSysIClass (struct IntuitionBase * IntuitionBase)
{
    struct IClass *cl = NULL;

    cl = MakeClass(SYSICLASS, IMAGECLASS, NULL, sizeof(struct SysIData), 0);
    if (cl == NULL)
        return NULL;

    cl->cl_Dispatcher.h_Entry    = (APTR)AROS_ASMSYMNAME(dispatch_sysiclass);
    cl->cl_Dispatcher.h_SubEntry = NULL;
    cl->cl_UserData              = (IPTR)IntuitionBase;

    AddClass (cl);

    return (cl);
}

/**************************************************************************************************/

static UWORD getbgpen(ULONG state, UWORD *pens)
{
    UWORD bg;
    
    switch (state)
    {
	
	case IDS_NORMAL:
	case IDS_SELECTED:
	    bg = pens[FILLPEN];
	    break;
	    
	case IDS_INACTIVENORMAL:
	    bg = pens[BACKGROUNDPEN];
	    break;
	default:
	    bg = pens[BACKGROUNDPEN];
	    break;
    }
    return bg;
}

/**************************************************************************************************/

static void renderimageframe(struct RastPort *rp, ULONG which, ULONG state, UWORD *pens,
			     WORD left, WORD top, WORD width, WORD height,
			     struct IntuitionBase *IntuitionBase)
{
    WORD right = left + width - 1;
    WORD bottom = top + height - 1;
    BOOL leftedgegodown = FALSE;
    BOOL topedgegoright = FALSE;

    switch(which)
    {
    	case CLOSEIMAGE:
	    /* draw separator line at the right side */
	    SetAPen(rp, pens[SHINEPEN]);
	    RectFill(rp, right, top, right, bottom - 1);
	    SetAPen(rp, pens[SHADOWPEN]);
	    WritePixel(rp, right, bottom);
	    
	    right--;
	    break;
	
	case ZOOMIMAGE:
	case DEPTHIMAGE:
	case SDEPTHIMAGE:
	    /* draw separator line at the left side */
	    SetAPen(rp, pens[SHINEPEN]);
	    WritePixel(rp, left, top);
	    SetAPen(rp, pens[SHADOWPEN]);
	    RectFill(rp, left, top + 1, left, bottom);

	    left++;
	    break;
	
	case UPIMAGE:
	case DOWNIMAGE:
	    leftedgegodown = TRUE;
	    break;
	
	case LEFTIMAGE:
	case RIGHTIMAGE:
	    topedgegoright = TRUE;
	    break;
    }

    if (left == 0) leftedgegodown = TRUE;
    if (top == 0) topedgegoright = TRUE;
    
    SetAPen(rp, pens[((state == IDS_SELECTED) || (state == IDS_INACTIVESELECTED)) ? SHADOWPEN : SHINEPEN]);

    /* left edge */
    RectFill(rp, left,
    		 top,
		 left,
		 bottom - (leftedgegodown ? 0 : 1));

    /* top edge */
    RectFill(rp, left + 1,
    		 top,
		 right - (topedgegoright ? 0 : 1),
		 top);
    
    SetAPen(rp, pens[((state == IDS_SELECTED) || (state == IDS_INACTIVESELECTED)) ? SHINEPEN : SHADOWPEN]);

    /* right edge */
    RectFill(rp, right,
    		 top + (topedgegoright ? 1 : 0),
		 right,
		 bottom);

    /* bottom edge */
    RectFill(rp, left + (leftedgegodown ? 1 : 0),
    		 bottom,
		 right - 1,
		 bottom);
}

/**************************************************************************************************/
