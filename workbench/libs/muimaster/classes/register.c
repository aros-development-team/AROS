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

extern struct Library *MUIMasterBase;

#define REGISTERTAB_EXTRA_HEIGHT     4
#define REGISTERTAB_IMEXTRA_HEIGHT   2
#define REGISTERTABITEM_EXTRA_WIDTH  8
#define REGISTERTAB_SPACE_LEFT 	     8
#define REGISTERTAB_SPACE_RIGHT      8
#define REGISTERTAB_IMAGE_TEXT_SPACE 4
#define REGISTER_FRAMEX 4
#define REGISTER_FRAMEY 4
#define SAMEWIDTH 1

#define IMWIDTH(x)  (((struct Image *)(x))->Width)
#define IMHEIGHT(x) (((struct Image *)(x))->Height)

struct RegisterTabItem
{
    STRPTR  text;
    Object *image;
    WORD    textlen;
    WORD    x1, y1, x2, y2, w, h;
    WORD    tx, ty, ix, iy;
};

struct MUI_RegisterData
{
    struct MUI_EventHandlerNode   ehn;
    struct RegisterTabItem  	 *items;
    char    	    	    	**labels;
    WORD    	    	     	  numitems;
    WORD    	    	     	  active;
    WORD    	    	    	  oldactive;
    WORD    	    	     	  left;
    WORD    	    	     	  top;
    WORD    	    	     	  width;
    WORD    	    	     	  height;
    WORD    	    	     	  framewidth;
    WORD    	    	     	  frameheight;
    WORD    	    	     	  fontw;
    WORD    	    	     	  fonth;
    WORD    	    	     	  fontb;
    WORD    	    	     	  slopew;    
};

static void RenderRegisterTabItem(struct IClass *cl, Object *obj,  WORD item)
{
    struct MUI_RegisterData *data = INST_DATA(cl, obj);
    struct RegisterTabItem *ri = &data->items[item];
    WORD x, y;
    
    if ((item < 0) || (item >= data->numitems)) return;
    
    SetDrMd(_rp(obj), JAM1);
    SetFont(_rp(obj), _font(obj));
    
    x = data->left + ri->x1;
    y = data->top + ri->y1;
 
    SetAPen(_rp(obj), _pens(obj)[(data->active == item) ? MPEN_TEXT : MPEN_BACKGROUND]);       
    Move(_rp(obj), x + ri->tx + 1, y + ri->ty);
    Text(_rp(obj), ri->text, ri->textlen);
    SetAPen(_rp(obj), _pens(obj)[MPEN_TEXT]);       
    Move(_rp(obj), x + ri->tx, y + ri->ty);
    Text(_rp(obj), ri->text, ri->textlen);
    
    if (ri->image)
    {
    	DrawImageState(_rp(obj), (struct Image*)ri->image, x + ri->ix, y + ri->iy, IDS_NORMAL, muiRenderInfo(obj)->mri_DrawInfo);
    }
    
    /* upper / at left side */
    
    SetAPen(_rp(obj), _pens(obj)[MPEN_SHINE]);
    WritePixel(_rp(obj), x + data->slopew, y + 2);
    Move(_rp(obj), x + data->slopew / 2, y + 3 + data->slopew - 1);
    Draw(_rp(obj), x + data->slopew - 1, y + 3);
    
    /* --- at top side */
    
    RectFill(_rp(obj), x + data->slopew + 1, y + 1, x + data->slopew + 2, y + 1);
    RectFill(_rp(obj), x + data->slopew + 3, y, x + ri->w - 1 - data->slopew - 3, y);

    SetAPen(_rp(obj), _pens(obj)[MPEN_SHADOW]);
    RectFill(_rp(obj), x + ri->w - 1 - data->slopew - 2, y + 1, x + ri->w - 1 - data->slopew - 1, y + 1);
    
    /* upper \ at right side */
    
    WritePixel(_rp(obj), x + ri->w - 1 - data->slopew, y + 2);
    Move(_rp(obj), x + ri->w - 1 - data->slopew + 1, y + 3);
    Draw(_rp(obj), x + ri->w - 1 - data->slopew / 2, y + 3 + data->slopew - 1);
    
    /* lower / at left side. */
    
    if ((item == 0) || (data->active == item))
    {
    	SetAPen(_rp(obj), _pens(obj)[MPEN_SHINE]);
    }
    else
    {
    	SetAPen(_rp(obj), _pens(obj)[MPEN_BACKGROUND]);
    }
    Move(_rp(obj), x, y + ri->h - 2);
    Draw(_rp(obj), x + data->slopew / 2 - 1, y + ri->h - 2 - data->slopew + 1);
    
    /* lower \ at the lefst side from the previous item */
    
    if (item > 0)
    {
    	if (data->active == item)
	{
    	    SetAPen(_rp(obj), _pens(obj)[MPEN_BACKGROUND]);
	}
	else
	{
    	    SetAPen(_rp(obj), _pens(obj)[MPEN_SHADOW]);
	}
	Move(_rp(obj), x + data->slopew / 2, y + ri->h - 2 - data->slopew + 1);
	Draw(_rp(obj), x + data->slopew - 1, y + ri->h - 2);
    }
    
    /* lower \ at right side. */
    
    if (data->active == item + 1)
    {
    	SetAPen(_rp(obj), _pens(obj)[MPEN_BACKGROUND]);
    }
    else
    {
    	SetAPen(_rp(obj), _pens(obj)[MPEN_SHADOW]);
    }
    Move(_rp(obj), x + ri->w - 1 - data->slopew / 2 + 1, y + ri->h - 2 - data->slopew + 1);
    Draw(_rp(obj), x + ri->w - 1, y + ri->h - 2);
    
    if (data->active == item)
    {
    	SetAPen(_rp(obj), _pens(obj)[MPEN_BACKGROUND]);
    	RectFill(_rp(obj), x, y + ri->h - 1, x + ri->w - 1, y + ri->h - 1);
    }
    else
    {
    	WORD x1, x2;
	
	x1 = x;
	x2 = x + ri->w - 1;
	
	if (data->active == item - 1)
	    x1 += data->slopew;
	else if (data->active == item + 1)
	    x2 -= data->slopew;
	    
    	SetAPen(_rp(obj), _pens(obj)[MPEN_SHINE]);
    	RectFill(_rp(obj), x1, y + ri->h - 1, x2, y + ri->h - 1);
    }

}

static void RenderRegisterTab(struct IClass *cl, Object *obj, BOOL alsoframe)
{
    struct MUI_RegisterData *data = INST_DATA(cl, obj);
    WORD i;

    SetDrMd(_rp(obj), JAM1);
    
    SetAPen(_rp(obj), _pens(obj)[MPEN_SHINE]);   
     
    RectFill(_rp(obj), data->left,
    	    	 data->top + data->height - 1,
		 data->left + REGISTERTAB_SPACE_LEFT - 1,
		 data->top + data->height - 1);
		 
    RectFill(_rp(obj), data->left + data->width - REGISTERTAB_SPACE_RIGHT,
    	    	 data->top + data->height - 1,
		 data->left + data->width - 1,
		 data->top + data->height - 1); 
		 
    for(i = 0; i < data->numitems; i++)
    {
    	RenderRegisterTabItem(cl, obj, i); 
    }
    
    if (alsoframe)
    {
    	SetAPen(_rp(obj), _pens(obj)[MPEN_SHINE]);
	
	RectFill(_rp(obj), data->left,
	    	     data->top + data->height - 1,
		     data->left,
		     data->top + data->height + data->frameheight - 1);
		     		     
	RectFill(_rp(obj), data->left + data->width,
	    	     data->top + data->height - 1,
		     data->left + data->framewidth - 2,
		     data->top + data->height - 1);
		     
	SetAPen(_rp(obj), _pens(obj)[MPEN_SHADOW]);
	
	RectFill(_rp(obj), data->left + data->framewidth - 1,
	    	     data->top + data->height - 1,
		     data->left + data->framewidth - 1,
		     data->top + data->height + data->frameheight - 1);
		     
	RectFill(_rp(obj), data->left + 1,
	    	     data->top + data->height + data->frameheight - 1,
		     data->left + data->framewidth - 2,
		     data->top + data->height + data->frameheight - 1);
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
    	    	    	    	    	TAG_MORE, msg->ops_AttrList);
    if (!obj) return NULL;

    data = INST_DATA(cl, obj);
    
    data->labels = (char**)GetTagData(MUIA_Register_Titles, NULL, msg->ops_AttrList);
    data->active = (WORD)GetTagData(MUIA_Group_ActivePage, 0, msg->ops_AttrList);
    
    if (!data->labels)
    {
	CoerceMethod(cl, obj, OM_DISPOSE);
	return NULL;
    }

    for(data->numitems = 0; data->labels[data->numitems]; data->numitems++)
    {
    }
    
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
    
    data->active = (WORD)GetTagData(MUIA_Group_ActivePage, data->active, msg->ops_AttrList);
    
    retval = DoSuperMethodA(cl, obj, (Msg)msg);
    
    return retval;
}

/**************************************************************************
 MUIM_Setup
**************************************************************************/
static ULONG Register_Setup(struct IClass *cl, Object *obj, struct MUIP_Setup *msg)
{
    struct MUI_RegisterData *data = INST_DATA(cl, obj);   
    struct RastPort temprp;
    WORD    	    i, x, h, biggest_w = 0;
    
    if (!DoSuperMethodA(cl, obj, (Msg)msg))
    {
    	return FALSE;
    }
    
    InitRastPort(&temprp);
    SetFont(&temprp, _font(obj));
    
    data->fonth = _font(obj)->tf_YSize;
    data->fontb = _font(obj)->tf_Baseline;
    
    h = 0;
    for(i = 0; i < data->numitems; i++)
    {
    	if (data->items[i].image)
	{
	    if (IMHEIGHT(data->items[i].image) > h) h = IMHEIGHT(data->items[i].image);
	}
    }
    
    if (h) h += REGISTERTAB_IMEXTRA_HEIGHT;
        
    i = data->fonth + REGISTERTAB_EXTRA_HEIGHT;
    h = (i > h) ? i : h;
    
    data->height = (h + 3) & ~3; /* Multiple of 4 */
    
    data->height += 4;
    
    data->slopew = (data->height - 4) / 2;
    
    for(i = 0; i < data->numitems; i++)
    {
    	data->items[i].textlen = strlen(data->items[i].text);
    	data->items[i].w = TextLength(&temprp, data->items[i].text, data->items[i].textlen);
	data->items[i].w += REGISTERTABITEM_EXTRA_WIDTH + data->slopew * 2;
	
	if (data->items[i].image)
	{
	    data->items[i].w += REGISTERTAB_IMAGE_TEXT_SPACE +
	    	    	       ((struct Image *)data->items[i].image)->Width;
	}
	
	if (data->items[i].w > biggest_w) biggest_w = data->items[i].w;
    }
    
    if (SAMEWIDTH)
    {
	for(i = 0; i < data->numitems; i++)
	{
    	    data->items[i].w = biggest_w;
	}
    }

    x = REGISTERTAB_SPACE_LEFT;
    for(i = 0; i < data->numitems; i++)
    {
	WORD itemwidth;
    	WORD to = 0;
	
	itemwidth = TextLength(&temprp, data->items[i].text, data->items[i].textlen);

	if (data->items[i].image)
	{
	    to = IMWIDTH(data->items[i].image) + REGISTERTAB_IMAGE_TEXT_SPACE;
            itemwidth += to;
    	}
	
	data->items[i].x1 = x;
	data->items[i].y1 = 0;
	data->items[i].x2 = x + data->items[i].w - 1;
	data->items[i].y2 = data->height - 1;
	data->items[i].h  = data->items[i].y2 - data->items[i].y1 + 1;
    	data->items[i].ix = (data->items[i].w - itemwidth) / 2;
	if (data->items[i].image)
	{
	    data->items[i].iy = (data->items[i].h - IMHEIGHT(data->items[i].image)) / 2;
	}
	
	data->items[i].tx = data->items[i].ix + to;
	data->items[i].ty = data->fontb + (data->items[i].h - data->fonth) / 2;
	
	
	x += data->items[i].w - data->slopew;
    }

    data->width = x + data->slopew + REGISTERTAB_SPACE_RIGHT;

#ifdef AROS
    DeinitRastPort(&temprp);
#endif

    _addleft(obj)   = REGISTER_FRAMEX;
    _subwidth(obj)  = _addleft(obj) + REGISTER_FRAMEX;
    _addtop(obj)    = data->height + REGISTER_FRAMEY;
    _subheight(obj) = REGISTER_FRAMEY + _addtop(obj);
    
    DoMethod(_win(obj), MUIM_Window_AddEventHandler, &data->ehn);
    
    return TRUE;
}

/**************************************************************************
 MUIM_Cleanup
**************************************************************************/
static ULONG Register_Cleanup(struct IClass *cl, Object *obj, struct MUIP_Cleanup *msg)
{
    struct MUI_RegisterData *data = INST_DATA(cl, obj);

    DoMethod(_win(obj), MUIM_Window_RemEventHandler, &data->ehn);
    
    return DoSuperMethodA(cl,obj,(Msg)msg);
}

/**************************************************************************
 MUIM_AskMinMax
**************************************************************************/
static ULONG Register_AskMinMax(struct IClass *cl, Object *obj, struct MUIP_AskMinMax *msg)
{
    struct MUI_RegisterData *data = INST_DATA(cl, obj);

    DoSuperMethodA(cl, obj, (Msg)msg);
    
    if (data->width > msg->MinMaxInfo->MinWidth)
    	msg->MinMaxInfo->MinWidth = data->width;
	
    if (data->width > msg->MinMaxInfo->MaxWidth)
    	msg->MinMaxInfo->MaxWidth = data->width;
	
    if (data->width > msg->MinMaxInfo->DefWidth)
    	msg->MinMaxInfo->DefWidth = data->width;
	
    msg->MinMaxInfo->MinHeight += data->height;
    msg->MinMaxInfo->MaxHeight += data->height;
    msg->MinMaxInfo->DefHeight += data->height;

    return TRUE;
}

/**************************************************************************
 MUIM_Layout
**************************************************************************/
static ULONG Register_Layout(struct IClass *cl, Object *obj, struct MUIP_Layout *msg)
{
    struct MUI_RegisterData *data = INST_DATA(cl, obj);
    ULONG retval;
    
    DoSuperMethodA(cl, obj, (Msg)msg);
    
    data->left        = _left(obj);
    data->top         = _top(obj);
    data->framewidth  = _width(obj);
    data->frameheight = _height(obj) - data->height;

    return retval;
}

/**************************************************************************
 MUIM_Draw
**************************************************************************/
static ULONG Register_Draw(struct IClass *cl, Object *obj, struct MUIP_Draw *msg)
{
    struct MUI_RegisterData *data = INST_DATA(cl, obj);
    
    DoSuperMethodA(cl,obj,(Msg)msg);

    RenderRegisterTab(cl, obj, TRUE);
    
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

	    if (_between(0, x, data->width) &&
		_between(0, y, data->height))
	    {
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

#ifndef _AROS
__asm IPTR Register_Dispatcher( register __a0 struct IClass *cl, register __a2 Object *obj, register __a1 Msg msg)
#else
AROS_UFH3S(IPTR,Register_Dispatcher,
	AROS_UFHA(Class  *, cl,  A0),
	AROS_UFHA(Object *, obj, A2),
	AROS_UFHA(Msg     , msg, A1))
#endif
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
