/*
    Copyright � 2002, The AROS Development Team. 
    All rights reserved.
    
    $Id$
*/

#include <string.h>
#include <exec/memory.h>
#include <intuition/icclass.h>
#include <intuition/gadgetclass.h>
#include <intuition/imageclass.h>
#include <clib/alib_protos.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/graphics.h>

#ifdef _AROS
#include <proto/muimaster.h>
#endif

#include "mui.h"
#include "muimaster_intern.h"
#include "support.h"
#include "prefs.h"

/*  #define MYDEBUG 1 */
#include "debug.h"

extern struct Library *MUIMasterBase;

#define INTERTAB 4
#define TEXTSPACING 4
#define REGISTERTAB_EXTRA_HEIGHT 7
#define REGISTER_FRAMEX 4
#define REGISTER_FRAMEY 4

#define MAX(a,b) (((a) > (b)) ? (a) : (b))
#define MIN(a,b) (((a) < (b)) ? (a) : (b))

struct RegisterTabItem
{
    STRPTR  text;    /* strlen(text) - valide between new/delete */
    WORD    textlen; /* strlen(text) - valide between setup/cleanup */
    WORD    x1, x2;  /* tab x input sensitive interval, relative to object's origin - valid between show/hide */
    WORD    y1, y2;  /* tab y input sensitive interval - valid between setup/cleanup */
};

struct MUI_RegisterData
{
    struct MUI_EventHandlerNode ehn;
    struct RegisterTabItem     *items;
    char    	    	      **labels;
    WORD    	    	        numitems;
    WORD    	    	     	active;
    WORD    	    	    	oldactive;
    WORD    	    	     	left;
    WORD    	    	     	top;
    WORD    	    	     	min_width;       /* object min width required */
    WORD    	    	     	def_width;       /* object def width required */
    WORD    	    	     	tab_height;      /* title height */
    WORD    	    	     	framewidth;
    WORD    	    	     	frameheight;
    WORD    	    	     	fontw;
    WORD    	    	     	fonth;
    WORD    	    	     	fontb;
    WORD                        total_hspacing;
    WORD                        ty; /* text y origin - valid between setup/cleanup */
};

/**************************************************************************
   Render one item
**************************************************************************/
static void RenderRegisterTabItem(struct IClass *cl, Object *obj,  WORD item)
{
    struct MUI_RegisterData *data = INST_DATA(cl, obj);
    struct RegisterTabItem *ri = &data->items[item];
    struct TextExtent extent;
    WORD fitlen;  /* text len fitting in alloted space */
    WORD fitpix;  /* text pixels fitting in alloted space */
    WORD x, y;
    WORD top_item_bar_y;
    WORD bottom_item_bar_y;
    WORD left_item_bar_x;
    WORD right_item_bar_x;
    WORD item_bar_width;
    WORD text_y;
    WORD item_bg_height;
    WORD fitwidth;

    if ((item < 0) || (item >= data->numitems)) return;
    
    y = data->top + ri->y1;

    if (data->active == item)
    {
	top_item_bar_y = _top(obj) + ri->y1;
	bottom_item_bar_y = _top(obj) + ri->y2 - 2;
	left_item_bar_x = _left(obj) + ri->x1 - 1;
	right_item_bar_x = _left(obj) + ri->x2 + 1;
	item_bg_height = data->tab_height;
	text_y = y + data->ty;
	item_bar_width = right_item_bar_x - left_item_bar_x + 1;
	/* fill tab with register background */
	DoMethod(obj,MUIM_DrawBackground, left_item_bar_x, top_item_bar_y + 4,
		 item_bar_width, item_bg_height - 4,
		 left_item_bar_x, top_item_bar_y + 4, 0);
	DoMethod(obj,MUIM_DrawBackground, left_item_bar_x + 2, top_item_bar_y + 2,
		 item_bar_width - (2 * 2), 2,
		 left_item_bar_x + 2, top_item_bar_y + 2, 0);
	DoMethod(obj,MUIM_DrawBackground, left_item_bar_x + 4, top_item_bar_y + 1,
		 item_bar_width - (2 * 4), 1,
		 left_item_bar_x + 4, top_item_bar_y + 1, 0);
    }
    else
    {
	top_item_bar_y = _top(obj) + ri->y1 + 2;
	bottom_item_bar_y = _top(obj) + ri->y2 - 1;
	left_item_bar_x = _left(obj) + ri->x1;
	right_item_bar_x = _left(obj) + ri->x2;
	item_bg_height = data->tab_height - 3;
	text_y = y + data->ty + 1;
	item_bar_width = right_item_bar_x - left_item_bar_x + 1;
	SetAPen(_rp(obj), _pens(obj)[MPEN_BACKGROUND]);
	RectFill(_rp(obj), left_item_bar_x, top_item_bar_y + 4,
		 right_item_bar_x, bottom_item_bar_y);
	RectFill(_rp(obj), left_item_bar_x + 2, top_item_bar_y + 2,
		 right_item_bar_x - 2, top_item_bar_y + 3);
	RectFill(_rp(obj), left_item_bar_x + 4, top_item_bar_y + 1,
		 right_item_bar_x - 4, top_item_bar_y + 1);
    }

    /* top horiz bar */
    SetAPen(_rp(obj), _pens(obj)[MPEN_SHINE]);
    RectFill(_rp(obj), left_item_bar_x + 4, top_item_bar_y, right_item_bar_x - 4, top_item_bar_y);
    /* left vert bar */
    RectFill(_rp(obj), left_item_bar_x, top_item_bar_y + 4, left_item_bar_x, bottom_item_bar_y);
    WritePixel(_rp(obj), left_item_bar_x + 1, top_item_bar_y + 3);
    WritePixel(_rp(obj), left_item_bar_x + 1, top_item_bar_y + 2);
    WritePixel(_rp(obj), left_item_bar_x + 2, top_item_bar_y + 1);
    WritePixel(_rp(obj), left_item_bar_x + 3, top_item_bar_y + 1);
    SetAPen(_rp(obj), _pens(obj)[MPEN_HALFSHINE]);
    WritePixel(_rp(obj), left_item_bar_x + 3, top_item_bar_y);
    WritePixel(_rp(obj), left_item_bar_x + 4, top_item_bar_y + 1);
    WritePixel(_rp(obj), left_item_bar_x + 2, top_item_bar_y + 2);
    WritePixel(_rp(obj), left_item_bar_x + 3, top_item_bar_y + 2);
    WritePixel(_rp(obj), left_item_bar_x + 2, top_item_bar_y + 3);
    WritePixel(_rp(obj), left_item_bar_x, top_item_bar_y + 3);
    WritePixel(_rp(obj), left_item_bar_x + 1, top_item_bar_y + 4);

    if (data->active == item)
    {
	/* bottom horiz bar */
	SetAPen(_rp(obj), _pens(obj)[MPEN_SHINE]);
	WritePixel(_rp(obj), left_item_bar_x - 1, bottom_item_bar_y + 1);
	SetAPen(_rp(obj), _pens(obj)[MPEN_SHADOW]);
	WritePixel(_rp(obj), right_item_bar_x + 1, bottom_item_bar_y + 1);
	DoMethod(obj,MUIM_DrawBackground, left_item_bar_x - 1, bottom_item_bar_y + 2,
		 item_bar_width + (2 * 1), 1,
		 left_item_bar_x - 1, top_item_bar_y + 2, 0);
	
    }
    /* right vert bar */
    SetAPen(_rp(obj), _pens(obj)[MPEN_SHADOW]);
    WritePixel(_rp(obj), right_item_bar_x - 1, top_item_bar_y + 2);
    RectFill(_rp(obj), right_item_bar_x, top_item_bar_y + 4, right_item_bar_x, bottom_item_bar_y);
    SetAPen(_rp(obj), _pens(obj)[MPEN_HALFSHADOW]);
    WritePixel(_rp(obj), right_item_bar_x - 2, top_item_bar_y + 1);
    WritePixel(_rp(obj), right_item_bar_x - 1, top_item_bar_y + 3);
    WritePixel(_rp(obj), right_item_bar_x, top_item_bar_y + 3);
    SetAPen(_rp(obj), _pens(obj)[MPEN_BACKGROUND]);
    WritePixel(_rp(obj), right_item_bar_x - 3, top_item_bar_y + 1);

    /* text */ 
    fitwidth = item_bar_width - TEXTSPACING;
    fitlen = TextFit(_rp(obj), ri->text, ri->textlen, &extent, NULL, 1, fitwidth, data->tab_height);
    fitpix = extent.te_Width;
/*      D(bug("extent for %s (len=%d) in %d pix = %d chars, %d pix [%x,%x,%x]\n", */
/*  	  ri->text, ri->textlen, fitwidth, fitlen, fitpix, _rp(obj), _rp(obj)->Font, _font(obj))); */
    x = left_item_bar_x + (item_bar_width - fitpix) / 2;
    SetDrMd(_rp(obj), JAM1);
    SetAPen(_rp(obj), _pens(obj)[MPEN_TEXT]);
    Move(_rp(obj), x, text_y);
    Text(_rp(obj), ri->text, fitlen);
}

/**************************************************************************
   Render tab bar
**************************************************************************/
static void RenderRegisterTab(struct IClass *cl, Object *obj, ULONG flags)
{
    struct MUI_RegisterData *data = INST_DATA(cl, obj);
    WORD i;
    WORD tabx;

/*
 * Erase / prepare for drawing
 */
    if (flags & MADF_DRAWOBJECT)
    {
	DoMethod(_parent(obj),MUIM_DrawBackground, data->left, data->top,
		 data->framewidth, data->tab_height - 1, data->left, data->top, 0);
    }
    else
    {
	/* draw parent bg over oldactive */
	WORD old_left, old_top, old_width, old_height;
	struct RegisterTabItem *ri = &data->items[data->oldactive];

	old_left = _left(obj) + ri->x1 - 2;
	old_top = _top(obj) + ri->y1;
	old_width = ri->x2 - ri->x1 + 5;
	old_height = data->tab_height - 1;
	DoMethod(_parent(obj),MUIM_DrawBackground, old_left, old_top,
		 old_width, old_height, old_left, old_top, 0);
	SetDrMd(_rp(obj), JAM1);
	SetAPen(_rp(obj), _pens(obj)[MPEN_SHINE]);
	RectFill(_rp(obj), old_left, old_top + old_height, old_left + old_width, old_top + old_height);
    }

    SetDrMd(_rp(obj), JAM1);
    SetAPen(_rp(obj), _pens(obj)[MPEN_SHINE]);   
    SetFont(_rp(obj), _font(obj));

/*
 * Draw new graphics
 */
    /* register frame */
    if (flags & MADF_DRAWOBJECT)
    {
    	SetAPen(_rp(obj), _pens(obj)[MPEN_SHINE]);
	
	RectFill(_rp(obj), data->left,
	    	     data->top + data->tab_height - 1,
		     data->left,
		     data->top + data->tab_height + data->frameheight - 1);
		     		     
	RectFill(_rp(obj), data->left + 1,
	    	     data->top + data->tab_height - 1,
		     data->left + data->framewidth - 2,
		     data->top + data->tab_height - 1);
		     
	SetAPen(_rp(obj), _pens(obj)[MPEN_SHADOW]);
	
	RectFill(_rp(obj), data->left + data->framewidth - 1,
	    	     data->top + data->tab_height - 1,
		     data->left + data->framewidth - 1,
		     data->top + data->tab_height + data->frameheight - 1);
		     
	RectFill(_rp(obj), data->left + 1,
	    	     data->top + data->tab_height + data->frameheight - 1,
		     data->left + data->framewidth - 2,
		     data->top + data->tab_height + data->frameheight - 1);
	for(i = 0, tabx = 0; i < data->numitems; i++)
	{
	    RenderRegisterTabItem(cl, obj, i); 
	}
    }
    else
    {
	RenderRegisterTabItem(cl, obj, data->active);
	RenderRegisterTabItem(cl, obj, data->oldactive);
    }

}

/**************************************************************************
 OM_NEW
**************************************************************************/
static ULONG Register_New(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct MUI_RegisterData *data;
    int i;

    obj = (Object *)DoSuperNew(cl, obj, MUIA_Group_PageMode, TRUE,
					MUIA_Background, MUII_RegisterBack,
    	    	    	    	    	TAG_MORE, msg->ops_AttrList);
    if (!obj) return NULL;

    data = INST_DATA(cl, obj);
    
    data->labels = (char**)GetTagData(MUIA_Register_Titles, NULL, msg->ops_AttrList);

    if (!data->labels)
    {
	CoerceMethod(cl, obj, OM_DISPOSE);
	return NULL;
    }

    for(data->numitems = 0; data->labels[data->numitems]; data->numitems++)
	;

    data->active = (WORD)GetTagData(MUIA_Group_ActivePage, 0, msg->ops_AttrList);
    if (data->active < 0 || data->active >= data->numitems)
    {
	data->active = 0;
    }
    data->oldactive = data->active;

    data->items = (struct RegisterTabItem *)AllocVec(data->numitems * sizeof(struct RegisterTabItem),
    	    	    	    	    	    	     MEMF_PUBLIC | MEMF_CLEAR);
    
    if (!data->items)
    {
	CoerceMethod(cl, obj, OM_DISPOSE);
	return NULL;
    }
    
    for(i = 0; i < data->numitems; i++)
    {
    	data->items[i].text = data->labels[i];
    }
        
    data->ehn.ehn_Events   = IDCMP_MOUSEBUTTONS;
    data->ehn.ehn_Priority = 0;
    data->ehn.ehn_Flags    = 0;
    data->ehn.ehn_Object   = obj;
    data->ehn.ehn_Class    = cl;
    
    return (ULONG)obj;
}

/**************************************************************************
 OM_DISPOSE
**************************************************************************/
static ULONG Register_Dispose(struct IClass *cl, Object *obj, Msg msg)
{
    struct MUI_RegisterData *data = INST_DATA(cl, obj);

    if (data->items) FreeVec(data->items);
    
    return DoSuperMethodA(cl, obj, msg);
}

/**************************************************************************
 OM_SET
**************************************************************************/
static IPTR Register_Set(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct MUI_RegisterData *data = INST_DATA(cl, obj);
    IPTR    	    	     retval;
    WORD active;

    active = (WORD)GetTagData(MUIA_Group_ActivePage, data->active, msg->ops_AttrList);

    if (active != data->active)
    {
	data->oldactive = data->active;
	data->active = active;
    }    
    retval = DoSuperMethodA(cl, obj, (Msg)msg);
    
    return retval;
}

/**************************************************************************
 MUIM_Setup
**************************************************************************/
static ULONG Register_Setup(struct IClass *cl, Object *obj, struct MUIP_Setup *msg)
{
    struct MUI_RegisterData *data = INST_DATA(cl, obj);   
    WORD    	    i, h = 0;
    
    if (!DoSuperMethodA(cl, obj, (Msg)msg))
    {
    	return FALSE;
    }
    
    data->fonth = _font(obj)->tf_YSize;
    data->fontb = _font(obj)->tf_Baseline;
    
    h = 0;

    i = data->fonth + REGISTERTAB_EXTRA_HEIGHT;
    h = (i > h) ? i : h;
    
    data->tab_height = h;
    data->ty = data->fontb + 1 + (data->tab_height - data->fonth) / 2;

/*      D(bug("Register_Setup : data->height=%d\n", data->tab_height)); */
    
    for(i = 0; i < data->numitems; i++)
    {
    	data->items[i].textlen = strlen(data->items[i].text);
	data->items[i].y1 = 0;
	data->items[i].y2 = data->tab_height - 1;
    }

    data->total_hspacing = (data->numitems + 1) * INTERTAB - 2;
/*      D(bug("Register_AskMinMax : data->total_hspacing = %d\n", data->total_hspacing)); */

    data->min_width = data->total_hspacing * 3;
    data->def_width = data->total_hspacing;

    if (!(muiGlobalInfo(obj)->mgi_Prefs->register_truncate_titles))
    {
	struct RastPort temprp;
	int i;
	WORD textpixmax;

	InitRastPort(&temprp);
	SetFont(&temprp, _font(obj));

	textpixmax = 0;
	for(i = 0; i < data->numitems; i++)
	{
	    WORD textpix = TextLength(&temprp, data->items[i].text, data->items[i].textlen);
	    textpixmax = MAX(textpix, textpixmax);
	}
	data->def_width += (textpixmax + TEXTSPACING + 1) * data->numitems;
	data->def_width = MAX(data->min_width, data->def_width);
    }

    muiAreaData(obj)->mad_HardILeft  	= REGISTER_FRAMEX;
    muiAreaData(obj)->mad_HardITop   	= data->tab_height + REGISTER_FRAMEY;
    muiAreaData(obj)->mad_HardIRight  	= REGISTER_FRAMEX;
    muiAreaData(obj)->mad_HardIBottom 	= REGISTER_FRAMEY;
    muiAreaData(obj)->mad_Flags     	|= (MADF_INNERLEFT | MADF_INNERTOP | MADF_INNERRIGHT | MADF_INNERBOTTOM);
        
    DoMethod(_win(obj), MUIM_Window_AddEventHandler, (IPTR)&data->ehn);
    
    return TRUE;
}

/**************************************************************************
 MUIM_Cleanup
**************************************************************************/
static ULONG Register_Cleanup(struct IClass *cl, Object *obj, struct MUIP_Cleanup *msg)
{
    struct MUI_RegisterData *data = INST_DATA(cl, obj);

    DoMethod(_win(obj), MUIM_Window_RemEventHandler, (IPTR)&data->ehn);
    
    return DoSuperMethodA(cl,obj,(Msg)msg);
}

/**************************************************************************
 MUIM_AskMinMax
**************************************************************************/
static ULONG Register_AskMinMax(struct IClass *cl, Object *obj, struct MUIP_AskMinMax *msg)
{
    struct MUI_RegisterData *data = INST_DATA(cl, obj);

    DoSuperMethodA(cl, obj, (Msg)msg);
    D(bug("Register_AskMinMax1 : %ld, %ld, %ld\n",
	  msg->MinMaxInfo->MinWidth, msg->MinMaxInfo->DefWidth, msg->MinMaxInfo->MaxWidth));
   

    D(bug("Register_AskMinMax : spacings = %d, minw=%d, defw=%d, mymin=%d, mydef=%d\n",
	  data->total_hspacing, msg->MinMaxInfo->MinWidth, msg->MinMaxInfo->DefWidth,
	  data->min_width, data->def_width));

    msg->MinMaxInfo->MinWidth = MAX(msg->MinMaxInfo->MinWidth, data->min_width);
    msg->MinMaxInfo->MaxWidth = MAX(msg->MinMaxInfo->MaxWidth, data->def_width);
    msg->MinMaxInfo->DefWidth = MAX(msg->MinMaxInfo->DefWidth, data->def_width);

    msg->MinMaxInfo->MinHeight += data->tab_height;
    msg->MinMaxInfo->MaxHeight += data->tab_height;
    msg->MinMaxInfo->DefHeight += data->tab_height;

    D(bug("Register_AskMinMax2 : %ld, %ld, %ld\n",
	  msg->MinMaxInfo->MinWidth, msg->MinMaxInfo->DefWidth, msg->MinMaxInfo->MaxWidth));
    return TRUE;
}

/**************************************************************************
 MUIM_Layout
**************************************************************************/
static ULONG Register_Layout(struct IClass *cl, Object *obj, struct MUIP_Layout *msg)
{
    struct MUI_RegisterData *data = INST_DATA(cl, obj);
    ULONG retval = 1;
    
    DoSuperMethodA(cl, obj, (Msg)msg);
    
    data->left        = _left(obj);
    data->top         = _top(obj);
    data->framewidth  = _width(obj);
    data->frameheight = _height(obj) - data->tab_height;
/*      D(bug("Register_Layout : left=%d, top=%d / framewidth=%d, frameheight=%d\n", */
/*  	  data->left, data->top, data->framewidth, data->frameheight)); */

    return retval;
}

/**************************************************************************
 MUIM_Draw
**************************************************************************/
static ULONG Register_Show(struct IClass *cl, Object *obj, struct MUIP_Show *msg)
{
    struct MUI_RegisterData *data = INST_DATA(cl, obj);
    WORD i;
    //WORD minwidth;
    WORD extra_space;
    WORD fitwidth;
    WORD x;
    WORD item_width; /* from vertical line left to v l right */

    DoSuperMethodA(cl,obj,(Msg)msg);

    item_width = (_width(obj) - data->total_hspacing) / data->numitems;
    extra_space = (_width(obj) - data->total_hspacing) % data->numitems;
    D(bug("Register_Show(%lx) : width = %d, mwidth = %d, max item width = %d, remainder = %d\n",
	  obj, _width(obj), _mwidth(obj), item_width, extra_space));

/*      D(bug("Register_Show : left = %d, _left = %d, mleft = %d, \n", data->left, _left(obj), _mleft(obj))); */
    x = INTERTAB - 1;

    for(i = 0; i < data->numitems; i++)
    {
	struct RegisterTabItem *ri = &data->items[i];
    	ri->x1 = x;
    	ri->x2 = ri->x1 + item_width - 1;
	if (extra_space > 0)
	{
	    ri->x2++;
	    extra_space--;
	}
	fitwidth = ri->x2 - ri->x1 + 1 - TEXTSPACING;
	x += fitwidth + TEXTSPACING + INTERTAB;
    }
    
    return TRUE;
}

/**************************************************************************
 MUIM_Draw
**************************************************************************/
static ULONG Register_Draw(struct IClass *cl, Object *obj, struct MUIP_Draw *msg)
{
    //struct MUI_RegisterData *data = INST_DATA(cl, obj);
    
    DoSuperMethodA(cl,obj,(Msg)msg);

/*      D(bug("Register_Draw : flags = %d\n", msg->flags)); */
    if (!(msg->flags & (MADF_DRAWOBJECT | MADF_DRAWUPDATE)))
	return(0);
    RenderRegisterTab(cl, obj, msg->flags);
    
    return TRUE;
}

/**************************************************************************
 MUIM_HandleEvent
**************************************************************************/
static ULONG Register_HandleEvent(struct IClass *cl, Object *obj, struct MUIP_HandleEvent *msg)
{
    struct MUI_RegisterData *data = INST_DATA(cl, obj);
    WORD i, x, y;
    
    if (msg->imsg)
    {
	if ((msg->imsg->Class == IDCMP_MOUSEBUTTONS) &&
	    (msg->imsg->Code == SELECTDOWN))
	{
	    x = msg->imsg->MouseX - data->left;
	    y = msg->imsg->MouseY - data->top;

/*  	    D(bug("Register_HandleEvent : %d,%d,%d -- %d,%d,%d\n", 0, x, _width(obj), 0, y, data->tab_height)); */
	    if (_between(0, x, _width(obj)) &&
		_between(0, y, data->tab_height))
	    {
/*  		D(bug("Register_HandleEvent : in tab, %d,%d\n", x, y)); */
		for(i = 0; i < data->numitems; i++)
		{
		    if (_between(data->items[i].x1, x, data->items[i].x2) &&
			_between(data->items[i].y1, y, data->items[i].y2))
		    {
			if (data->active != i)
			{
			    set(obj, MUIA_Group_ActivePage, i);
			}
			break;
		    }

		}
	    }
	    	
	}
    }

    return 0;
}


BOOPSI_DISPATCHER(IPTR, Register_Dispatcher, cl, obj, msg)
{
    switch (msg->MethodID)
    {
	case OM_NEW: return Register_New(cl, obj, (struct opSet *)msg);
	case OM_DISPOSE: return Register_Dispose(cl, obj, msg);
	case OM_SET: return Register_Set(cl, obj, (struct opSet *)msg);
	case MUIM_Setup: return Register_Setup(cl, obj, (struct MUIP_Setup *)msg);
    	case MUIM_Cleanup: return Register_Cleanup(cl, obj, (struct MUIP_Cleanup *)msg);
	case MUIM_AskMinMax: return Register_AskMinMax(cl, obj, (struct MUIP_AskMinMax *)msg);
    	case MUIM_Layout: return Register_Layout(cl, obj, (struct MUIP_Layout *)msg);
    	case MUIM_Show: return Register_Show(cl, obj, (struct MUIP_Show *)msg);
	case MUIM_Draw: return Register_Draw(cl, obj, (struct MUIP_Draw *)msg);
	case MUIM_HandleEvent: return Register_HandleEvent(cl, obj, (struct MUIP_HandleEvent *)msg);
    }
    return DoSuperMethodA(cl, obj, msg);
}

/*
 * Class descriptor.
 */
const struct __MUIBuiltinClass _MUI_Register_desc = { 
    MUIC_Register, 
    MUIC_Group, 
    sizeof(struct MUI_RegisterData), 
    (void*)Register_Dispatcher 
};
