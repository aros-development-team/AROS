/*
    (C) 1997-98 AROS - The Amiga Research OS
    $Id$

    Desc: Implementation of SYSICLASS
    Lang: english
*/

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

/****************************************************************************/

/* Some handy transparent base class object casting defines.
 */
#define IM(o) ((struct Image *)o)

static UWORD getbgpen(ULONG state, UWORD *pens);

#undef IntuitionBase
#define IntuitionBase	((struct IntuitionBase *)(cl->cl_UserData))


/****************************************************************************/

struct SysIData
{
    ULONG type;
    struct DrawInfo *dri;
    struct Image *frame;
};

/****************************************************************************/

/* Some handy drawing functions */

#warning FIXME: Draw lines that are broader than 1 pixel
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

/****************************************************************************/


/****************************************************************************/
BOOL sysi_setnew(Class *cl, Object *obj, struct opSet *msg)
{
    struct SysIData *data = INST_DATA(cl, obj);
    struct TagItem *taglist, *tag;
    BOOL unsupported = FALSE;

    taglist = msg->ops_AttrList;
    while ((tag = NextTagItem(&taglist)))
    {
	switch(tag->ti_Tag)
	{
	case SYSIA_DrawInfo:
	    data->dri = (struct DrawInfo *)tag->ti_Data;
	    break;
	case SYSIA_Which:
	    data->type = tag->ti_Data;
D(bug("SYSIA_Which type: %d\n", data->type));
            switch (tag->ti_Data)
            {
            /* The following images are not scalable, yet! */
            case LEFTIMAGE:
                IM(obj)->ImageData = ArrowLeft0Data;
                IM(obj)->Width     = ARROWLEFT_WIDTH;
                IM(obj)->Height    = ARROWLEFT_HEIGHT;
                break;

            case UPIMAGE:
                IM(obj)->ImageData = ArrowUp0Data;
                IM(obj)->Width     = ARROWUP_WIDTH;
                IM(obj)->Height    = ARROWUP_HEIGHT;
                break;

            case RIGHTIMAGE:
                IM(obj)->ImageData = ArrowRight0Data;
                IM(obj)->Width     = ARROWRIGHT_WIDTH;
                IM(obj)->Height    = ARROWRIGHT_HEIGHT;
                break;

            case DOWNIMAGE:
                IM(obj)->ImageData = ArrowDown0Data;
                IM(obj)->Width     = ARROWDOWN_WIDTH;
                IM(obj)->Height    = ARROWDOWN_HEIGHT;
                break;

            case CHECKIMAGE:
            case MXIMAGE:
                break;

            case DEPTHIMAGE:
            case ZOOMIMAGE:
            case CLOSEIMAGE:
            case SIZEIMAGE:
	        break;
		
            case SDEPTHIMAGE:
            case MENUCHECK:
            case AMIGAKEY:
#warning FIXME: Missing Tags
            default:
                unsupported = TRUE;
                break;
            }
	    break;
	case SYSIA_ReferenceFont:
#warning FIXME: Missing Tag
	    break;
	case SYSIA_Size:
#warning FIXME: Missing Tag
	    break;
	}
    }

D(bug("dri: %p, unsupported: %d\n", data->dri, unsupported));
    if ((!data->dri) || (unsupported))
	return FALSE;
    return TRUE;
}


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
    if (!sysi_setnew(cl, obj, (struct opSet *)msg))
    {
        CoerceMethod(cl, obj, OM_DISPOSE);
        return NULL;
    }

D(bug("sysi_setnew called successfully\n"));

    switch (data->type)
    {
    case CHECKIMAGE:
    case MXIMAGE:
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
    case LEFTIMAGE:
    case UPIMAGE:
    case RIGHTIMAGE:
    case DOWNIMAGE:
    
    case DEPTHIMAGE:
    case ZOOMIMAGE:
    case CLOSEIMAGE:
    case SIZEIMAGE:
        break;
    
    default:
        CoerceMethod(cl, obj, OM_DISPOSE);
        return NULL;
    }

    return obj;
}


void sysi_draw(Class *cl, Object *obj, struct impDraw *msg)
{
    struct SysIData *data = INST_DATA(cl, obj);
    struct RastPort *rport = msg->imp_RPort;
    WORD left = IM(obj)->LeftEdge + msg->imp_Offset.X;
    WORD top = IM(obj)->TopEdge + msg->imp_Offset.Y;
    UWORD width = IM(obj)->Width;
    UWORD height = IM(obj)->Height;

    EraseRect(rport, left, top, left + width - 1, top + height - 1);

    switch(data->type)
    {
    case CHECKIMAGE:
    {
        /* Draw frame */
        DrawImageState(rport, data->frame,
                       msg->imp_Offset.X, msg->imp_Offset.Y,
                       IDS_NORMAL, data->dri);

        /* Draw checkmark (only if image is in selected state) */
        if (msg->imp_State == IDS_SELECTED)
        {
            SetAPen(rport, data->dri->dri_Pens[TEXTPEN]);
       	    SetDrMd(rport, JAM1);

            draw_thick_line(cl, rport, left + width/4, top + height/2, left + width/2, top + width*3/4, 2);
            draw_thick_line(cl, rport, left + width/2, top + height/4*3, left + width/4*3, top + height/4, 3);
            Move(rport, left + width/4*3, top + height/4);
            Draw(rport, left + width/8*7, top + height/4);

        }
        break;
    }
    case MXIMAGE:
    {
        struct TagItem tags[] = {
          { IA_Recessed, FALSE },
          { TAG_DONE, 0UL }
        };

        /* Draw frame */
        if (msg->imp_State == IDS_SELECTED)
            tags[0].ti_Data = TRUE;
        SetAttrsA(data->frame, tags);
        DrawImageState(rport, data->frame,
                       msg->imp_Offset.X, msg->imp_Offset.Y,
                       IDS_NORMAL, data->dri);

        if (msg->imp_State == IDS_SELECTED)
        {
            SetABPenDrMd(rport, data->dri->dri_Pens[FILLPEN], 0, JAM1);
            RectFill(rport, left + 4, top + 2, left + width - 5, top + height - 3);
        }
        break;
    }
    
    #define SPACING 3
    /* Georg Steger */
    #define HSPACING 4
    #define VSPACING 2
    
    case LEFTIMAGE:
    {
    	WORD cy;
	
    	SetAPen(rport, data->dri->dri_Pens[TEXTPEN]);
    	SetDrMd(rport, JAM1);

#if 0
    	Move(rport, left + width - SPACING - 1, top + SPACING); /* Move to upper right */
    	Draw(rport, left + SPACING, top + ((height - 2 * SPACING) / 2));     /* Render '/' */
    	Move(rport, rport->cp_x, rport->cp_y + 1);
    	Draw(rport, left + width - SPACING - 1, top + height - SPACING - 1); /* Render '\' */
#endif
	/* Georg Steger */
	cy = height / 2;

	Move(rport, left + width - 1 - HSPACING, top + VSPACING + 1);
	Draw(rport, left + HSPACING, top + height - cy);
	Move(rport, left + width - 1 - HSPACING, top + VSPACING);
	Draw(rport, left + HSPACING, top + height - cy - 1);
	
	Move(rport, left + width - 1 - HSPACING, top + height - 1- VSPACING - 1);
	Draw(rport, left + HSPACING, top + cy - 1);
	Move(rport, left + width - 1 - HSPACING, top + height - 1 - VSPACING);
	Draw(rport, left + HSPACING, top + cy);
	
    	break;
    }

    case UPIMAGE:
    {
    	WORD cx;
	
    	SetAPen(rport, data->dri->dri_Pens[TEXTPEN]);
    	SetDrMd(rport, JAM1);
#if 0
    	Move(rport, left + SPACING, top + height - SPACING - 1); /* Move to lower left */
    	Draw(rport, left + ((width - SPACING * 2) / 2), top + SPACING);	 /* Render '/' */
    	Move(rport, rport->cp_x + 1, rport->cp_y);
    	Draw(rport, left + width - SPACING - 1, top + height - SPACING - 1); /* Render '\' */
#endif
	/* Georg Steger */
	cx = width / 2;
	
	Move(rport, left + HSPACING + 1, top + height - 1 - VSPACING);
	Draw(rport, left + width - cx, top + VSPACING);
	Move(rport, left + HSPACING, top + height - 1 - VSPACING);
	Draw(rport, left + width - cx - 1, top + VSPACING);
	
	Move(rport, left + width - 1 - HSPACING - 1, top + height - 1 - VSPACING);
	Draw(rport, left + cx - 1, top + VSPACING);
	Move(rport, left + width - 1 - HSPACING, top + height - 1 - VSPACING);
	Draw(rport, left + cx, top + VSPACING);
    	break;
    }
    
    case RIGHTIMAGE:
    {
    	WORD cy;
	
    	SetAPen(rport, data->dri->dri_Pens[TEXTPEN]);
    	SetDrMd(rport, JAM1);
#if 0
    	Move(rport, left + SPACING, top + SPACING); /* Move to upper left */
    	Draw(rport, left + width - SPACING - 1, top + ((height - 2 * SPACING) / 2)); /* Render '\' */
    	Move(rport, rport->cp_x, rport->cp_y + 1);
    	Draw(rport, left + SPACING, top + height - SPACING - 1);		 /* Render '/' */
#endif
	/* Georg Steger */
	cy = height / 2;

	Move(rport, left + HSPACING, top + VSPACING + 1);
	Draw(rport, left + width - 1 - HSPACING, top + height - cy);
	Move(rport, left + HSPACING, top + VSPACING);
	Draw(rport, left + width - 1 - HSPACING, top + height - cy - 1);
	
	Move(rport, left + HSPACING, top + height - 1- VSPACING - 1);
	Draw(rport, left + width - 1 - HSPACING, top + cy - 1);
	Move(rport, left + HSPACING, top + height - 1 - VSPACING);
	Draw(rport, left + width - 1 - HSPACING, top + cy);
	
    	break;
    }

    case DOWNIMAGE:
    {
    	WORD cx;
	
    	SetAPen(rport, data->dri->dri_Pens[TEXTPEN]);
    	SetDrMd(rport, JAM1);
#if 0    	
	Move(rport, left + SPACING, top + SPACING);	/* Move to upper left */
    	Draw(rport, left + ((width - SPACING * 2) / 2), top + height - SPACING - 1);
    	Move(rport, rport->cp_x + 1, rport->cp_y);
    	Draw(rport, left + width - SPACING - 1, top + SPACING);
#endif
	/* Georg Steger */
	cx = width / 2;
	
	Move(rport, left + HSPACING + 1, top + VSPACING);
	Draw(rport, left + width - cx, top + height - 1 - VSPACING);
	Move(rport, left + HSPACING, top + VSPACING);
	Draw(rport, left + width - cx - 1, top + height - 1 - VSPACING);
	
	Move(rport, left + width - 1 - HSPACING - 1, top + VSPACING);
	Draw(rport, left + cx - 1, top + height - 1 - VSPACING);
	Move(rport, left + width - 1 - HSPACING, top + VSPACING);
	Draw(rport, left + cx, top + height - 1 - VSPACING);
    	break;
    }

    case DEPTHIMAGE: {
        UWORD *pens = data->dri->dri_Pens;
	UWORD bg;

        WORD h_spacing = width  / 3;
	WORD v_spacing = height / 3;
	
	WORD right, bottom;
	
	
	
	bg = getbgpen(msg->imp_State, pens);
	
	/* Clear background into correct color */
	SetAPen(rport, bg);
	RectFill(rport, left, top, left + width - 1, top + height - 1);
	
	/* Draw a image of two partly overlapped tiny windows, 
	*/
	
	left += h_spacing / 2;
	top  += v_spacing / 2;
	
	width  -= h_spacing;
	height -= v_spacing;
	
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
	
	if (msg->imp_State != IDS_INACTIVENORMAL)
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
	SetAPen(rport, pens[(msg->imp_State == IDS_INACTIVENORMAL) ? BACKGROUNDPEN : SHINEPEN]);
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
	
	SetAPen(rport, getbgpen(msg->imp_State, pens));
	RectFill(rport,left, top, left + width - 1, top + height - 1);
	SetAPen(rport, pens[SHADOWPEN]);
	RectFill(rport, left + 5, top + 4, left + width - 1 - 5, top + height - 1 - 4);
	SetAPen(rport, pens[(msg->imp_State == IDS_NORMAL) ? SHINEPEN : BACKGROUNDPEN]);
	RectFill(rport, left + 6, top + 5, left + width - 1 - 6, top + height - 1 - 5); 

        break; }
	
    case ZOOMIMAGE: {
        UWORD *pens = data->dri->dri_Pens;
	UWORD bg;

	WORD bottom = top + height -1 ;

	bg = getbgpen(msg->imp_State, pens);
	
	/* Clear background into correct color */
	SetAPen(rport, bg);
	RectFill(rport, left, top, left + width - 1, top + height - 1);
	
	SetAPen(rport, pens[SHADOWPEN]);
	RectFill(rport, left + 2, top + 2, left + width - 1 - 2, top + height - 1 - 2);

	SetAPen(rport, pens[(msg->imp_State == IDS_SELECTED) ? SHINEPEN :
						 	       (msg->imp_State == IDS_NORMAL) ? FILLPEN : BACKGROUNDPEN]);
	RectFill(rport, left + 3, top + 3, left + width - 1 - 3, top + height - 1 - 3);
	
	SetAPen(rport, pens[SHADOWPEN]);
	RectFill(rport, left + 3, top + 3, left + 4 + 4, top + 3 + 4);
	
	SetAPen(rport, pens[(msg->imp_State == IDS_SELECTED) ? FILLPEN :
							       (msg->imp_State == IDS_NORMAL) ? SHINEPEN : BACKGROUNDPEN]);
	RectFill(rport,left + 4, top + 3, left + 5 + 1, top + 3 + 3);
        break; }
	
    
    case SIZEIMAGE: {
        UWORD *pens = data->dri->dri_Pens;
	UWORD bg;

        WORD h_spacing = width  / 4;
	WORD v_spacing = height / 4;
	
	WORD right, bottom;
	
	
	
	bg = getbgpen(msg->imp_State, pens);
	
	/* Clear background into correct color */
	SetAPen(rport, bg);
	RectFill(rport, left, top, left + width - 1, top + height - 1);
	
	/* A triangle image 	*/
	
	left += h_spacing;
	top  += v_spacing;
	
	right  = left + width  - 1 - h_spacing;
	bottom = top  + height - 1 - v_spacing;
	
	/* Draw triangle border */
	Move(rport, left, bottom);
	Draw(rport, right, top);
	Draw(rport, right, bottom);
	Draw(rport, left, bottom);
	
	break; }
	
	/* Fill rectangle with white color */
	
	/* !! need AreaFill() for this */
    
    } /* switch (image type) */
    return;
}

/****************************************************************************/

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

#undef IntuitionBase

/****************************************************************************/

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

