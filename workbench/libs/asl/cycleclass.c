/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/


#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/utility.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/cybergraphics.h>
#include <exec/memory.h>
#include <intuition/screens.h>
#include <intuition/icclass.h>
#include <intuition/cghooks.h>
#include <intuition/imageclass.h>
#include <intuition/gadgetclass.h>
#include <graphics/gfx.h>
#include <cybergraphx/cybergraphics.h>

#include <string.h>

#include "asl_intern.h"
#include "layout.h"

#define SDEBUG 0
#define DEBUG 0

#include <aros/debug.h>

#define G(x) ((struct Gadget *)(x))
#define EG(x) ((struct ExtGadget *)(x))

#define CLASS_ASLBASE ((struct AslBase_intern *)cl->cl_UserData)
#define HOOK_ASLBASE  ((struct AslBase_intern *)hook->h_Data)

#define AslBase CLASS_ASLBASE

/********************** ASL CYCLE CLASS **************************************************/

#define CYCLEIMAGEWIDTH 19
#define ARROWWIDTH 7
#define ARROWHEIGHT 4

#define ARROW_DOWN 1
#define ARROW_UP   2

/* it is possible to scroll faster through the
** popup after TURBOCOUNTDOWN intuiticks
** (= 1/10 sec) by moving around the mouse
** (to force mousemove events to be sent to
** the gadget)
*/

#define TURBOCOUNTDOWN 5

/***********************************************************************************/

struct CycleItem
{
    char	*string;
    WORD	charlen;
    WORD	pixellen;
};

struct AslCycleData
{
    Object			*frame;
    struct CycleItem		*itemmemory;
    struct RastPort		*rp;
    struct RastPort		clonerp;
    struct DrawInfo		*dri;
    struct TextFont		*font;
    struct Window		*popupwindow;
    char			**labels;
    WORD			itemheight;
    WORD			itemwidth;
    WORD			menuleft;
    WORD			menutop;
    WORD			menuwidth;
    WORD			menuheight;
    WORD			numitems;
    WORD			active;
    WORD			visible;
    WORD			top;
    WORD			selected;
    WORD			menux1;
    WORD			menuy1;
    WORD			menux2;
    WORD			menuy2;
    WORD			layerx1;
    WORD			layery1;
    WORD			layerx2;
    WORD			layery2;
    BYTE			borderleft;
    BYTE			borderright;
    BYTE			bordertop;
    BYTE			borderbottom;
    BYTE			maypopup;
    BYTE			popup;
    BYTE			popupwindowtype;
    BYTE			uparrowblack;
    BYTE			downarrowblack;
    BYTE			sentgadgetup;
    BYTE			turbocountdown;
};


/***********************************************************************************/

static IPTR aslcycle_set(Class * cl, Object * o, struct opSet *msg);

/***********************************************************************************/

static void RenderObject_Update(Class *cl, Object *o, struct GadgetInfo *gi)
{
    struct RastPort *rp;
    
    if ((rp = ObtainGIRPort(gi)))
    {
        DoMethod(o, GM_RENDER,
		    (IPTR) gi,
		    (IPTR) rp,
		    GREDRAW_UPDATE);
		    
        ReleaseGIRPort(rp);
    }
}

/***********************************************************************************/

static void DrawArrow(Class *cl, struct RastPort *rp, WORD x1, WORD y1, WORD type)
{
    WORD dy = 1;
    
    if (type == ARROW_UP)
    {
        y1 += 3; dy = -1;
    }
    
    RectFill(rp, x1, y1, x1 + 6, y1); y1 += dy;
    RectFill(rp, x1 + 1, y1, x1 + 5, y1); y1 += dy;
    RectFill(rp, x1 + 2, y1, x1 + 4, y1); y1 += dy;
    WritePixel(rp, x1 + 3, y1);
}

/***********************************************************************************/

static void DrawUpArrow(Class *cl, struct AslCycleData *data, struct DrawInfo *dri, BOOL force)
{
    WORD x;
    BOOL black;

    if (data->visible == data->numitems) return;
    
    black = data->top > 0 ? TRUE : FALSE;
    if (force || (black != data->uparrowblack))
    {
	data->uparrowblack = black;

	SetAPen(data->rp, dri->dri_Pens[black ? SHADOWPEN : BACKGROUNDPEN]);

	x = data->borderleft + (data->itemwidth - ARROWWIDTH) / 2;

	DrawArrow(cl, data->rp, data->menuleft + x, data->menutop + data->menuy1 - ARROWHEIGHT - 1, ARROW_UP);
    }
}

/***********************************************************************************/

static void DrawDownArrow(Class *cl, struct AslCycleData *data, struct DrawInfo *dri, BOOL force)
{
    WORD x;
    BOOL black;

    if (data->visible == data->numitems) return;

    black = data->top + data->visible < data->numitems ? TRUE : FALSE;
    if (force || (black != data->downarrowblack))
    {
	data->downarrowblack = black;

	SetAPen(data->rp, dri->dri_Pens[black ? SHADOWPEN : BACKGROUNDPEN]);

	x = data->borderleft + (data->itemwidth - ARROWWIDTH) / 2;

	DrawArrow(cl, data->rp, data->menuleft + x, data->menutop + data->menuy2 + 2, ARROW_DOWN);
    }
}

/***********************************************************************************/

static void DrawItem(Class *cl, Object *o, struct DrawInfo *dri, WORD num, BOOL nowhitefill)
{
    struct AslCycleData *data = INST_DATA(cl, o);
    struct CycleItem	*item;
    WORD		off,x,y;

    off = num - data->top;

    if ((off >= 0) && (off < data->visible))
    {
	item = &data->itemmemory[num];

	x = data->menuleft + data->menux1;
	y = data->menutop  + data->menuy1 + off * data->itemheight;

	if ((num == data->selected) || !nowhitefill)
	{
	    SetAPen(data->rp, dri->dri_Pens[(num == data->selected) ? SHADOWPEN : SHINEPEN]);
	    RectFill(data->rp, x, y,
		     x + data->itemwidth - 1,
		     y + data->itemheight - 1);
	}

	x = (x + x + data->itemwidth - item->pixellen) / 2;
	y += data->font->tf_Baseline + 1;

	SetAPen(data->rp, dri->dri_Pens[(num == data->selected) ? SHINEPEN : TEXTPEN]);
	Move(data->rp, x, y);
	Text(data->rp, item->string, item->charlen);

    } /* if ((off >= 0) && (off < data->visible)) */
}

/***********************************************************************************/

static void DrawMenu(Class *cl, Object *o, struct DrawInfo *dri)
{
    struct AslCycleData *data = INST_DATA(cl, o);
    WORD		 i, x1, y1, x2, y2;

    x1 = data->menuleft;
    y1 = data->menutop;

    x2 = x1 + data->menuwidth - 1;
    y2 = y1 + data->menuheight - 1;

    SetAPen(data->rp, dri->dri_Pens[SHADOWPEN]);
    RectFill(data->rp, x1, y1, x2, y1);
    RectFill(data->rp, x2, y1 + 1, x2, y2);
    RectFill(data->rp, x1, y2, x2 - 1, y2);
    RectFill(data->rp, x1, y1 + 1, x1, y2 - 1);
    
    SetAPen (data->rp, dri->dri_Pens[SHINEPEN]);
    RectFill(data->rp, x1 + 1, y1  + 1, x2  - 1, y2 - 1);

    for(i = data->top;i < data->top + data->visible;i++)
    {
	DrawItem(cl, o, dri, i,TRUE);
    }

    DrawUpArrow(cl, data, dri, TRUE);
    DrawDownArrow(cl, data, dri,TRUE);
}

/***********************************************************************************/

static void UnselectActiveItem(Class *cl, Object *o, struct DrawInfo *dri)
{
    struct AslCycleData *data = INST_DATA(cl, o);
    WORD 		old;

    old = data->selected;
    data->selected = -1;
    DrawItem(cl, o, dri, old, FALSE);
}

/***********************************************************************************/

static IPTR aslcycle_new(Class * cl, Object * o, struct opSet *msg)
{
    struct AslCycleData *data;
    struct TagItem 	fitags[] =
    {
	{IA_FrameType, FRAME_BUTTON},
	{IA_EdgesOnly, FALSE	   },
	{TAG_DONE    , 0UL	   }
    };
    IPTR 		rc = 0;
    
    if ((o = (Object *)DoSuperMethodA(cl, o, (Msg)msg)))
    {
	data = INST_DATA(cl, o);

	/* We want to get a GM_LAYOUT message, no matter if gadget is GFLG_RELRIGHT/RELBOTTOM/
	   RELWIDTH/RELHEIGHT or not */	   
	G(o)->Flags |= GFLG_RELSPECIAL;

	data->frame = NewObjectA(NULL, FRAMEICLASS, fitags);	
	if (data->frame)
	{
	    aslcycle_set(cl, o, msg);
	    
	    rc = (ULONG)o;
	} else {

	    CoerceMethod(cl, o, OM_DISPOSE);

	}
    };
    
    return rc;
}

/***********************************************************************************/

static IPTR aslcycle_dispose(Class * cl, Object * o, Msg msg)
{
    struct AslCycleData *data = INST_DATA(cl, o);

    if (data->itemmemory) FreeVec(data->itemmemory);
    if (data->frame) DisposeObject(data->frame);
    
    return DoSuperMethodA(cl, o, msg);
}

/***********************************************************************************/

static IPTR aslcycle_set(Class * cl, Object * o, struct opSet *msg)
{
    struct AslCycleData *data = INST_DATA(cl, o);
    struct TagItem 	*tag, *tstate = msg->ops_AttrList;
    IPTR		retval, tidata;
    
    retval = DoSuperMethod(cl, o, OM_SET, (IPTR) msg->ops_AttrList, (IPTR) msg->ops_GInfo);
    
    while((tag = NextTagItem(&tstate)))
    {
        tidata = tag->ti_Data;
	
        switch(tag->ti_Tag)
	{
            case ASLCY_Active:
	        data->active = tidata;
	        retval += 1;
		break;
	
	    case ASLCY_Labels:
	        data->labels = (char **)tidata;

	        if (data->itemmemory)
		{
		    FreeVec(data->itemmemory);
		    data->itemmemory = 0;
		}
		
		data->active   = 0;
		data->numitems = 0;
		
		if (data->labels)
		{
		    WORD i;
		    
		    for(i = 0; data->labels[i]; i++)
		    {
		    }
		    
		    if (i) data->itemmemory = AllocVec(i * sizeof(struct CycleItem), MEMF_PUBLIC | MEMF_CLEAR);
		    
		    if (!data->itemmemory)
		    {
		        data->labels = 0;
		    } else {
		        struct CycleItem *item = data->itemmemory;
			
			data->numitems = i;
			
			for(i = 0; i < data->numitems;i++,item++)
			{
			    item->string  = data->labels[i];
			    item->charlen =  strlen(item->string);
			}
			
			if (msg->ops_GInfo)
			{
			    struct gpLayout gpl;
			    
			    gpl.MethodID    = GM_LAYOUT;
			    gpl.gpl_GInfo   = msg->ops_GInfo;
			    gpl.gpl_Initial = 0;
			    
			    DoMethodA(o, (Msg)&gpl);
			}			
			
		    } /* if (data->itemmemory) */
		    			    
		} /* if (data->labels) */
		
		retval += 1;
		break;
    	    
	    case ASLCY_Font:
	    	data->font = (struct TextFont *)tidata;
	    	retval += 1;
		break;
		
	} /* switch(tag->ti_Tag) */
	 
    } /* while((tag = NextTagItem(&tstate))) */ 
    
    if (retval)
    {
        RenderObject_Update(cl, o, msg->ops_GInfo);
    }
    
    return retval;    
}


/***********************************************************************************/

static IPTR aslcycle_get(Class * cl, Object * o, struct opGet *msg)
{
    struct AslCycleData *data = INST_DATA(cl, o);

    IPTR retval = 1;
    
    switch(msg->opg_AttrID)
    {
        case ASLCY_Active:
	    *msg->opg_Storage = data->active;
	    break;
	    
	default:
	    retval = DoSuperMethodA(cl, o, (Msg)msg);
	    break;
    }
    
    return retval;
}

/***********************************************************************************/

static IPTR aslcycle_layout(Class * cl, Object * o, struct gpLayout *msg)
{
    struct AslCycleData *data = INST_DATA(cl, o);
    struct CycleItem 	*item;
    WORD 		len, i;
    WORD 		x, y, w, h;

    getgadgetcoords(G(o), msg->gpl_GInfo, &x, &y, &w, &h);
    
    if (!data->font) data->font = msg->gpl_GInfo->gi_DrInfo->dri_Font;
    
    item = data->itemmemory;
    len = 0;

    InitRastPort(&data->clonerp);
    SetFont(&data->clonerp, data->font);
    
    for (i = 0; i < data->numitems; i++, item++)
    {
        item->pixellen = TextLength(&data->clonerp, item->string, item->charlen);
	if (item->pixellen > len) len = item->pixellen;	
    }

    data->itemwidth = len + 2;
    i = w - CYCLEIMAGEWIDTH - BORDERCYCLESPACINGX;
    if (data->itemwidth < i) data->itemwidth = i;

    data->itemheight = data->font->tf_YSize + 2;

    data->borderleft   = 4;
    data->borderright  = 4;
    data->bordertop    = 4;
    data->borderbottom = 4;

    data->menuwidth = data->itemwidth + data->borderleft + data->borderright;

    data->menuheight = data->numitems * data->itemheight +
		       data->bordertop +
		       data->borderbottom;

    data->menux1 = data->borderleft;

    data->maypopup = TRUE;

    if (data->menuheight > msg->gpl_GInfo->gi_Screen->Height)
    {
	data->visible = (msg->gpl_GInfo->gi_Screen->Height -
			 data->bordertop -
		         data->borderbottom -
			 (ARROWHEIGHT + 2) * 2) / data->itemheight;

	if (data->visible < 2)
	{
	    data->maypopup = FALSE;
	} else {
	    data->menuheight = data->bordertop +
			       data->visible * data->itemheight +
			       data->borderbottom +
			       (ARROWHEIGHT + 2) * 2;

	    data->menuy1 = data->bordertop + ARROWHEIGHT + 2;
	}
    } else {
	data->visible = data->numitems;
	data->menuy1 = data->bordertop;
    }

    data->menux2 = data->menux1 + data->itemwidth - 1;
    data->menuy2 = data->menuy1 + data->visible * data->itemheight - 1;
    
    return 0;
}

/***********************************************************************************/

static IPTR aslcycle_render(Class * cl, Object * o, struct gpRender *msg)
{
    struct AslCycleData *data = INST_DATA(cl, o);
    struct RastPort	*rp   = msg->gpr_RPort;
    struct DrawInfo 	*dri  = msg->gpr_GInfo->gi_DrInfo;
    struct CycleItem 	*item;
    WORD 		gadx, gady, gadw, gadh, x, y, y2, a1, a2;
    
    BOOL 		selected;
    
    if (rp)
    {
	struct TagItem im_tags[] =
	{
	    {IA_Width	, 0	},
	    {IA_Height	, 0	},
	    {TAG_DONE		}
	};
 
        getgadgetcoords(G(o), msg->gpr_GInfo, &gadx, &gady, &gadw, &gadh);

	im_tags[0].ti_Data = gadw;
	im_tags[1].ti_Data = gadh;

	SetAttrsA(data->frame, im_tags);

	DrawImageState(msg->gpr_RPort,
		       (struct Image *)data->frame,
		       gadx,
		       gady,
		       (G(o)->Flags & GFLG_SELECTED) ? IDS_SELECTED : IDS_NORMAL,
		       msg->gpr_GInfo->gi_DrInfo);
		       
	
	if (data->labels)
	{
	    selected = (G(o)->Flags & GFLG_SELECTED) ? TRUE: FALSE;
	    SetABPenDrMd(rp, dri->dri_Pens[selected ? FILLTEXTPEN : TEXTPEN], 0, JAM1);

	    item = &data->itemmemory[data->active];

	    a1 = gadx + BORDERCYCLESPACINGX;
	    a2 = gadx + gadw - CYCLEIMAGEWIDTH - BORDERCYCLESPACINGX;
	    x  = (a1 + a2 - item->pixellen) / 2;

	    y  = gady + (gadh - data->font->tf_YSize) / 2 + data->font->tf_Baseline;

    	    SetFont(rp, data->font);
	    
	    Move(rp, x, y);
	    Text(rp, item->string, item->charlen);
	}

	x = gadx + gadw - CYCLEIMAGEWIDTH;
	y = gady + 2;
	y2 = gady + gadh - 1 - 2;

	/* separator bar */
	
	SetAPen(rp, dri->dri_Pens[SHINEPEN]);
	RectFill(rp, x + 1, y , x + 1, y2);
	SetAPen(rp, dri->dri_Pens[SHADOWPEN]);
	RectFill(rp, x, y, x, y2);

	y = gady + (gadh - 4) / 2;
	x += 6;

	DrawArrow(cl, rp, x, y, ARROW_DOWN);

    } /* if (rp) */
    
    return 0;
}

/***********************************************************************************/

static IPTR aslcycle_goactive(Class * cl, Object * o, struct gpInput *msg)
{
    struct AslCycleData *data = INST_DATA(cl, o);
    WORD 		x, y, x2, y2, gadx, gady, gadw, gadh;
    
    IPTR 		rc = GMR_MEACTIVE;

    if (!data->labels || !msg->gpi_IEvent) return GMR_NOREUSE;
        
    data->popup = FALSE;
    data->sentgadgetup = FALSE;
    data->turbocountdown = TURBOCOUNTDOWN;

    getgadgetcoords(G(o), msg->gpi_GInfo, &gadx, &gady, &gadw, &gadh);

    x = msg->gpi_GInfo->gi_Window->MouseX;

    if (data->maypopup && (data->numitems > 2) && (x < gadx + gadw - CYCLEIMAGEWIDTH))
    {
	x = msg->gpi_GInfo->gi_Window->LeftEdge + gadx;
	y = msg->gpi_GInfo->gi_Window->TopEdge + gady + gadh;

	if (x < 0) x = 0;
	if ((x + data->menuwidth) > msg->gpi_GInfo->gi_Screen->Width)
	{
	    x = msg->gpi_GInfo->gi_Screen->Width - data->menuwidth;
	}

	if (y < 0) y = 0;
	if ((y + data->menuheight) > msg->gpi_GInfo->gi_Screen->Height)
	{
	    y = msg->gpi_GInfo->gi_Screen->Height - data->menuheight;
	}

	x2 = x + data->menuwidth - 1;
	y2 = y + data->menuheight - 1;

	if ((data->popupwindow = OpenWindowTags(0,WA_CustomScreen, (IPTR) msg->gpi_GInfo->gi_Screen, 
						  WA_Left, x,
						  WA_Top, y,
						  WA_Width, data->menuwidth,
						  WA_Height, data->menuheight,
						  WA_Flags, WFLG_BORDERLESS,
						  WA_BackFill, (IPTR) LAYERS_NOBACKFILL,
						  TAG_DONE)))
	{
	    data->menuleft = 0;
	    data->menutop = 0;
	    data->rp = data->popupwindow->RPort;

	}

	if (data->popupwindow)
	{
	    data->layerx1 = x;  data->layery1 = y;
	    data->layerx2 = x2; data->layery2 = y2;

	    data->top = 0;
	    data->selected = -1;
	    SetDrMd(data->rp, JAM1);
	    SetFont(data->rp, data->font);

	    data->popup = TRUE;
	    DrawMenu(cl, o, msg->gpi_GInfo->gi_DrInfo);
	}

    } /* if (data->maypopup && (data->numitems > 2) && (x < gadx + gadw - CYCLEIMAGEWIDTH)) */
    else
    {
         G(o)->Flags |= GFLG_SELECTED;
	 RenderObject_Update(cl, o, msg->gpi_GInfo);
    }
	
    return rc;
}

/***********************************************************************************/

static IPTR aslcycle_handleinput(Class * cl, Object * o, struct gpInput *msg)
{
    struct AslCycleData *data = INST_DATA(cl, o);
    WORD 		gadx, gady, gadw, gadh, x, y, i;
    IPTR 		rc = GMR_MEACTIVE;

    getgadgetcoords(G(o), msg->gpi_GInfo, &gadx, &gady, &gadw, &gadh);
    
    if (data->popup)
    {
	if ((msg->gpi_IEvent->ie_Class == IECLASS_RAWMOUSE) &&
            (msg->gpi_IEvent->ie_Code == (IECODE_LBUTTON | IECODE_UP_PREFIX)))
	{
	    rc = GMR_NOREUSE;
	    if (data->selected != -1)
	    {
		data->active = data->selected;
		data->sentgadgetup = TRUE;

		rc |= GMR_VERIFY;
		*msg->gpi_Termination = data->active;
	    }
	    goto done;

	}

	x = msg->gpi_GInfo->gi_Screen->MouseX - data->layerx1;
	y = msg->gpi_GInfo->gi_Screen->MouseY - data->layery1;
	
	if ((x < data->menux1) || (x > data->menux2))
	{
	    UnselectActiveItem(cl, o, msg->gpi_GInfo->gi_DrInfo);
	} else {
	    i = (y - data->menuy1) / data->itemheight;

	    if ((i >= data->visible) || (y < data->menuy1))
	    {
		UnselectActiveItem(cl, o, msg->gpi_GInfo->gi_DrInfo);

		if (data->turbocountdown && (msg->gpi_IEvent->ie_Class == IECLASS_TIMER))
		{
			data->turbocountdown--;
		}

		if (!data->turbocountdown || (msg->gpi_IEvent->ie_Class == IECLASS_TIMER))
		{
		    if (i >= data->visible)
		    {
			// scroll down 
			if (data->top + data->visible < data->numitems)
			{
			    ClipBlit(data->rp, data->menux1 + data->menuleft, data->menuy1 + data->menutop + data->itemheight,
			             data->rp, data->menux1 + data->menuleft, data->menuy1 + data->menutop, data->itemwidth, data->itemheight * (data->visible - 1), 192);
			    data->top++;
			    DrawItem(cl, o, msg->gpi_GInfo->gi_DrInfo, data->top + data->visible - 1, FALSE);
			    DrawUpArrow(cl, data, msg->gpi_GInfo->gi_DrInfo, FALSE);
			    DrawDownArrow(cl, data, msg->gpi_GInfo->gi_DrInfo, FALSE);
			}
		    } else {
			// scroll up
			if (data->top > 0)
			{
			    ClipBlit(data->rp, data->menux1 + data->menuleft, data->menuy1 + data->menutop,
				     data->rp, data->menux1 + data->menuleft, data->menuy1 + data->menutop + data->itemheight, data->itemwidth, data->itemheight * (data->visible - 1), 192);
			    data->top--;
			    DrawItem(cl, o, msg->gpi_GInfo->gi_DrInfo, data->top, FALSE);
			    DrawUpArrow(cl, data, msg->gpi_GInfo->gi_DrInfo, FALSE);
			    DrawDownArrow(cl, data, msg->gpi_GInfo->gi_DrInfo, FALSE);
			}
		    }
		} /* if (!data->turbocountdown || (msg->gpi_IEvent->ie_Class == IECLASS_TIMER)) */

	    } else {

		data->turbocountdown = TURBOCOUNTDOWN;
		x = data->selected;
		data->selected = data->top + i;
		if (data->selected != x)
		{
		    DrawItem(cl, o, msg->gpi_GInfo->gi_DrInfo, x, FALSE);
		    DrawItem(cl, o, msg->gpi_GInfo->gi_DrInfo, data->selected, FALSE);
		}
	    }
	}

    } else {	/* if (data->popup) */

	/* like a GadTools cycle gadget */

	switch(msg->gpi_IEvent->ie_Class)
	{
	    case IECLASS_RAWMOUSE:
		switch(msg->gpi_IEvent->ie_Code)
		{
		    case IECODE_LBUTTON | IECODE_UP_PREFIX:
			if (G(o)->Flags & GFLG_SELECTED)
			{
			    data->active++;
			    if (data->active >= data->numitems) data->active = 0;

			    G(o)->Flags &= (~GFLG_SELECTED);
			    RenderObject_Update(cl, o, msg->gpi_GInfo);
			    rc |= GMR_VERIFY;
			    *msg->gpi_Termination = G(o)->GadgetID;
			}
			rc |= GMR_NOREUSE;
			break;

		    default:
			if ((msg->gpi_Mouse.X >= 0   ) &&
			    (msg->gpi_Mouse.Y >= 0   ) &&
			    (msg->gpi_Mouse.X <  gadw) &&
			    (msg->gpi_Mouse.Y <  gadh))
			{
			    if (!(G(o)->Flags & GFLG_SELECTED))
			    {
				G(o)->Flags |= GFLG_SELECTED;
				RenderObject_Update(cl, o, msg->gpi_GInfo);
			    }
			} else {
			    if (G(o)->Flags & GFLG_SELECTED)
			    {
				G(o)->Flags &= (~GFLG_SELECTED);
				RenderObject_Update(cl, o, msg->gpi_GInfo);
			    }
			}
			rc = GMR_MEACTIVE;

			break;

		} /* switch(msg->gpi_IEvent->ie_Code) */

 	} /* switch(msg->gpi_IEvent->ie_Class) */

    } /* if (data->popup) {...} else { */

done:

    return rc;
    
}

/***********************************************************************************/

static IPTR aslcycle_goinactive(Class * cl, Object * o, struct gpGoInactive *msg)
{
    struct AslCycleData *data = INST_DATA(cl, o);
    
    if (data->popupwindow)
    {
        CloseWindow(data->popupwindow);
        data->popupwindow = 0;
    }
    
    if (data->sentgadgetup)
    {
        RenderObject_Update(cl, o, msg->gpgi_GInfo);
    }
    
    return 0;
}

/***********************************************************************************/

AROS_UFH3S(IPTR, dispatch_aslcycleclass,
	  AROS_UFHA(Class *, cl, A0),
	  AROS_UFHA(Object *, o, A2),
	  AROS_UFHA(Msg, msg, A1)
)
{
    AROS_USERFUNC_INIT

    IPTR retval = 0UL;

    switch (msg->MethodID)
    {
        case OM_NEW:
	    retval = aslcycle_new(cl, o, (struct opSet *)msg);
	    break;
	
	case OM_DISPOSE:
	    retval = aslcycle_dispose(cl, o, msg);
	    break;

	case OM_SET:
	    retval = aslcycle_set(cl, o, (struct opSet *)msg);
	    break;
	    
	case OM_GET:
	    retval = aslcycle_get(cl, o, (struct opGet *)msg);
	    break;
	
	case GM_LAYOUT:
	    retval = aslcycle_layout(cl, o, (struct gpLayout *)msg);
	    break;

	case GM_RENDER:
	    retval = aslcycle_render(cl, o, (struct gpRender *)msg);
	    break;
	
	case GM_GOACTIVE:
	    retval = aslcycle_goactive(cl, o, (struct gpInput *)msg);
	    break;
	
	case GM_HANDLEINPUT:
	    retval = aslcycle_handleinput(cl, o, (struct gpInput *)msg);
	    break;
	
	case GM_GOINACTIVE:
	    retval = aslcycle_goinactive(cl, o, (struct gpGoInactive *)msg);
	    break;
	    
	default:
	    retval = DoSuperMethodA(cl, o, msg);
	    break;

    } /* switch (msg->MethodID) */
    
    return retval;

    AROS_USERFUNC_EXIT
}

/***********************************************************************************/

#undef AslBase

Class *makeaslcycleclass(struct AslBase_intern * AslBase)
{
    Class *cl = NULL;

    if (AslBase->aslcycleclass)
	return AslBase->aslcycleclass;

    cl = MakeClass(NULL, GADGETCLASS, NULL, sizeof(struct AslCycleData), 0UL);
    if (!cl)
	return NULL;
	
    cl->cl_Dispatcher.h_Entry = (APTR) AROS_ASMSYMNAME(dispatch_aslcycleclass);
    cl->cl_Dispatcher.h_SubEntry = NULL;
    cl->cl_UserData = (IPTR) AslBase;

    AslBase->aslcycleclass = cl;

    return cl;
}

/***********************************************************************************/
