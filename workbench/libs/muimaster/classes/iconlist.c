/*
    Copyright © 2002, The AROS Development Team.
    All rights reserved.

    $Id$
*/

#include <string.h>
#include <stdlib.h>
#include <math.h>

#include <exec/memory.h>
#include <graphics/gfx.h>
#include <graphics/view.h>
#include <workbench/icon.h>
#include <workbench/workbench.h>
#include <clib/alib_protos.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/utility.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/icon.h>
#include <proto/layers.h>
#include <proto/muimaster.h>

#define MYDEBUG
#include "debug.h"
#include "mui.h"
#include "muimaster_intern.h"
#include "support.h"
#include "imspec.h"

extern struct Library *MUIMasterBase;

#ifndef NO_ICON_POSITION
#define NO_ICON_POSITION (0x8000000) /* belongs to workbench/workbench.h */
#endif

#ifdef __AROS__
#define dol_Name dol_OldName /* This doesn't work really */
#endif

#define UPDATE_SINGLEICON   1
#define UPDATE_SCROLL	    2

#define LEFT_BUTTON 	    1
#define RIGHT_BUTTON 	    2
#define MIDDLE_BUTTON	    4

struct IconEntry
{
    struct MinNode node;
    struct IconList_Entry entry;

    struct DiskObject *dob; /* The icons disk objects */

    int x,y;
    int width,height;
    int selected;
};

struct MUI_IconData
{
    APTR pool; /* Pool to allocate data from */

    struct MinList icon_list; /* IconEntry */
    int view_x,view_y; /* the leftmost/upper coordinates of the view */
    int view_width,view_height; /* dimensions of the view (_mwidth(obj) and _mheight(obj)) */
    int width,height; /* The whole width/height */
    int mouse_pressed;

    struct MUI_EventHandlerNode ehn;

    LONG touch_x;
    LONG touch_y;

    LONG click_x;
    LONG click_y;

    struct IconEntry *first_selected; /* the icon which has been selected first or NULL */

    /* DoubleClick stuff */
    ULONG last_secs;
    ULONG last_mics;
    struct IconEntry *last_selected;

    /* Notify stuff */
    struct IconList_Entry *drop_entry; /* the icon where the icons have been dropped */
    struct IconList_Click icon_click;

    /* Render stuff */

    /* values for update */
    /* UPDATE_SINGLEICON = draw the given single icon only */
    /* UPDATE_SCROLL = scroll the view by update_scrolldx/update_scrolldy */
    
    ULONG update;
    WORD update_scrolldx;
    WORD update_scrolldy;
    struct IconEntry *update_icon;
    struct Rectangle *update_rect1;
    struct Rectangle *update_rect2;
};

/**************************************************************************

**************************************************************************/
int RectAndRect(struct Rectangle *a, struct Rectangle *b)
{
    if ((a->MinX > b->MaxX) || (a->MinY > b->MaxY) || (a->MaxX < b->MinX) || (a->MaxY < b->MinY))
	return 0;
    return 1;
}

/**************************************************************************
 As we don't use the label drawing of icon.library we also have to do
 this by hand
**************************************************************************/
static void IconList_GetIconRectangle(Object *obj, struct IconEntry *icon, struct Rectangle *rect)
{
    int tx,txwidth;

    GetIconRectangleA(NULL,icon->dob,NULL,rect,NULL);

    if (icon->entry.label)
    {
    	SetFont(_rp(obj), _font(obj));
	txwidth = TextLength(_rp(obj),icon->entry.label,strlen(icon->entry.label));
	tx = (icon->width - txwidth)/2;
	if (tx < rect->MinX) rect->MinX = tx;
	if (tx + txwidth - 1 > rect->MaxX) rect->MaxX = tx + txwidth - 1;
    }

    rect->MaxY += _font(obj)->tf_YSize + 2;
}

/**************************************************************************
 Draw the icon at its position
**************************************************************************/
static void IconList_DrawIcon(Object *obj, struct MUI_IconData *data, struct IconEntry *icon)
{
    struct Rectangle iconrect;
    struct Rectangle objrect;
    
    LONG tx,ty;
    LONG txwidth; // txheight;

    IconList_GetIconRectangle(obj, icon, &iconrect);
    iconrect.MinX += _mleft(obj) - data->view_x + icon->x;
    iconrect.MaxX += _mleft(obj) - data->view_x + icon->x;
    iconrect.MinY += _mtop(obj) - data->view_y + icon->y;
    iconrect.MaxY += _mtop(obj) - data->view_y + icon->y;

    objrect.MinX = _mleft(obj);
    objrect.MinY = _mtop(obj);
    objrect.MaxX = _mright(obj);
    objrect.MaxY = _mbottom(obj);
    
    if (!RectAndRect(&iconrect, &objrect)) return;
 
    /* data->update_rect1 and data->update_rect2 may
       point to rectangles to indicate that only icons
       in any of this rectangles need to be drawn */
       
    if (data->update_rect1 && data->update_rect2)
    {
    	if (!RectAndRect(&iconrect, data->update_rect1) &&
	    !RectAndRect(&iconrect, data->update_rect2)) return;
    }
    else if (data->update_rect1)
    {
    	if (!RectAndRect(&iconrect, data->update_rect1)) return;
    }
    else if (data->update_rect2)
    {
    	if (!RectAndRect(&iconrect, data->update_rect2)) return;
    }
    
    SetABPenDrMd(_rp(obj),_pens(obj)[MPEN_TEXT],0,JAM1);

#ifndef __AROS__
    DrawIconState
    (
        _rp(obj), icon->dob, NULL,
        _mleft(obj) - data->view_x + icon->x, 
        _mtop(obj) - data->view_y + icon->y, 
        icon->selected ? IDS_SELECTED : IDS_NORMAL, 
        ICONDRAWA_EraseBackground, FALSE, 
        TAG_DONE
    );
#else
    DrawIconStateA
    (
        _rp(obj), icon->dob, NULL,
        _mleft(obj) - data->view_x + icon->x, 
        _mtop(obj) - data->view_y + icon->y, 
        icon->selected ? IDS_SELECTED : IDS_NORMAL, 
        TAG_DONE
    );
#endif

    if (icon->entry.label)
    {
    	ULONG nameLength = strlen(icon->entry.label);
        
        SetFont(_rp(obj), _font(obj));
	txwidth = TextLength(_rp(obj), icon->entry.label, nameLength);
	tx = _mleft(obj) - data->view_x + icon->x + (icon->width - txwidth)/2;
	ty = _mtop(obj)  - data->view_y + icon->y + icon->height + _font(obj)->tf_Baseline + 1;
	
        // FIXME: this isn't a very optimal to make an outline...
        SetSoftStyle(_rp(obj), FSF_BOLD, FSF_BOLD);
        SetAPen(_rp(obj), _pens(obj)[MPEN_SHADOW]);
        Move(_rp(obj), tx - 1, ty - 1); Text(_rp(obj), icon->entry.label, nameLength);
        Move(_rp(obj), tx - 1, ty    ); Text(_rp(obj), icon->entry.label, nameLength);
        Move(_rp(obj), tx - 1, ty + 1); Text(_rp(obj), icon->entry.label, nameLength);
        Move(_rp(obj), tx,     ty - 1); Text(_rp(obj), icon->entry.label, nameLength);
        Move(_rp(obj), tx,     ty + 1); Text(_rp(obj), icon->entry.label, nameLength);
        Move(_rp(obj), tx + 1, ty - 1); Text(_rp(obj), icon->entry.label, nameLength);
        Move(_rp(obj), tx + 1, ty    ); Text(_rp(obj), icon->entry.label, nameLength);
        Move(_rp(obj), tx + 1, ty + 1); Text(_rp(obj), icon->entry.label, nameLength);

        SetAPen(_rp(obj), _pens(obj)[MPEN_SHINE]);
        Move(_rp(obj), tx, ty);
	Text(_rp(obj), icon->entry.label, nameLength);
    }
}

/**************************************************************************
 
**************************************************************************/
static void IconList_RethinkDimensions(Object *obj, struct MUI_IconData *data, struct IconEntry *singleicon)
{
    struct IconEntry *icon;
    WORD    	      maxx = data->width - 1, maxy = data->height - 1;

    if (!(_flags(obj)&MADF_SETUP)) return;
    
    icon = singleicon ? singleicon : List_First(&data->icon_list);
    while (icon)
    {
	if (icon->dob && icon->x != NO_ICON_POSITION && icon->y != NO_ICON_POSITION)
	{
	    struct Rectangle icon_rect;
	    
	    IconList_GetIconRectangle(obj, icon, &icon_rect);
	    icon_rect.MinX += icon->x;
	    icon_rect.MaxX += icon->x;
	    icon_rect.MinY += icon->y;
	    icon_rect.MaxY += icon->y;

	    if (icon_rect.MaxX > maxx) maxx = icon_rect.MaxX;
	    if (icon_rect.MaxY > maxy) maxy = icon_rect.MaxY;
	    
	}
	
	if (singleicon) break;
	
	icon = Node_Next(icon);
    }
    
    /* update our view */
    if (maxx + 1 > data->width)
    {
    	data->width = maxx + 1;
	set(obj, MUIA_IconList_Width, data->width);
    }
    
    if (maxy + 1 > data->height)
    {
    	data->height = maxy + 1;
	set(obj, MUIA_IconList_Height, data->height);
    }
    
}

/**************************************************************************
 Checks weather we can place a icon with the given dimesions at the
 suggested positions.

 atx and aty are absolute positions
**************************************************************************/
static int IconList_CouldPlaceIcon(Object *obj, struct MUI_IconData *data, struct IconEntry *toplace, int atx, int aty)
{
    struct IconEntry *icon;
    struct Rectangle toplace_rect;

    IconList_GetIconRectangle(obj, toplace, &toplace_rect);
    toplace_rect.MinX += atx;
    toplace_rect.MaxX += atx;
    toplace_rect.MinY += aty;
    toplace_rect.MaxY += aty;

    icon = List_First(&data->icon_list);
    while (icon)
    {
	if (icon->dob && icon->x != NO_ICON_POSITION && icon->y != NO_ICON_POSITION)
	{
	    struct Rectangle icon_rect;
	    IconList_GetIconRectangle(obj, icon, &icon_rect);
	    icon_rect.MinX += icon->x;
	    icon_rect.MaxX += icon->x;
	    icon_rect.MinY += icon->y;
	    icon_rect.MaxY += icon->y;

	    if (RectAndRect(&icon_rect, &toplace_rect))
		return FALSE; /* There is already an icon on this place */
	}
	icon = Node_Next(icon);
    }
    return 1;
}

/**************************************************************************
 Place the icon at atx and aty.

 atx and aty are absolute positions
**************************************************************************/
static void IconList_PlaceIcon(Object *obj, struct MUI_IconData *data, struct IconEntry *toplace, int atx, int aty)
{
#if 0
    struct Rectangle toplace_rect;

    IconList_GetIconRectangle(obj, toplace, &toplace_rect);
    toplace_rect.MinX += atx + data->view_x;
    toplace_rect.MaxX += atx + data->view_x;
    toplace_rect.MinY += aty + data->view_y;
    toplace_rect.MaxY += aty + data->view_y;
#endif
    toplace->x = atx;
    toplace->y = aty;
#if 0
    /* update our view */
    if (toplace_rect.MaxX - data->view_x > data->width)
    {
    	data->width = toplace_rect.MaxX - data->view_x;
	set(obj, MUIA_IconList_Width, data->width);
    }

    if (toplace_rect.MaxY - data->view_y > data->height)
    {
    	data->height = toplace_rect.MaxY - data->view_y;
	set(obj, MUIA_IconList_Height, data->height);
    }
#endif
}

/**************************************************************************
 Place icons with NO_ICON_POSITION coords somewhere
**************************************************************************/

static void IconList_FixNoPositionIcons(Object *obj, struct MUI_IconData *data)
{
    struct IconEntry *icon;
    int cur_x = data->view_x + 36;
    int cur_y = data->view_y + 4;

    icon = List_First(&data->icon_list);
    while (icon)
    {
	if (icon->dob && icon->x == NO_ICON_POSITION && icon->y == NO_ICON_POSITION)
	{
	    int loops = 0;
	    int cur_x_save = cur_x;
	    int cur_y_save = cur_y;
	    struct Rectangle icon_rect;

	    IconList_GetIconRectangle(obj, icon, &icon_rect);
	    icon_rect.MinX += cur_x - icon->width/2 + data->view_x;
	    if (icon_rect.MinX < 0)
		cur_x -= icon_rect.MinX;

	    while (!IconList_CouldPlaceIcon(obj, data, icon, cur_x - icon->width/2, cur_y) && loops < 5000)
	    {
		cur_y++;

		if (cur_y + icon->height > data->view_x + data->view_height) /* on both sides -1 */
		{
		    cur_x += 72;
		    cur_y = data->view_y + 4;
		}
	    }

	    IconList_PlaceIcon(obj, data, icon, cur_x - icon->width/2, cur_y);

	    if (icon_rect.MinX < 0)
	    {
		cur_x = cur_x_save;
		cur_y = cur_y_save;
	    }
	}
	icon = Node_Next(icon);
    }
    
    IconList_RethinkDimensions(obj, data, NULL);
}

/**************************************************************************
 OM_NEW
**************************************************************************/
static IPTR IconList_New(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct MUI_IconData   *data;
    struct TagItem  	    *tag, *tags;

    obj = (Object *)DoSuperNewTags(cl, obj, NULL,
	MUIA_Dropable, TRUE,
	MUIA_Font, MUIV_Font_Tiny,
    	TAG_MORE, (IPTR) msg->ops_AttrList);
    if (!obj) return FALSE;

    data = INST_DATA(cl, obj);
    NewList((struct List*)&data->icon_list);

    set(obj,MUIA_FillArea,TRUE);

    /* parse initial taglist */
    for (tags = msg->ops_AttrList; (tag = NextTagItem(&tags)); )
    {
	switch (tag->ti_Tag)
	{
    	}
    }

    data->pool =  CreatePool(0,4096,4096);
    if (!data->pool)
    {
	CoerceMethod(cl,obj,OM_DISPOSE);
	return NULL;
    }

    data->ehn.ehn_Events   = IDCMP_MOUSEBUTTONS;
    data->ehn.ehn_Priority = 0;
    data->ehn.ehn_Flags    = 0;
    data->ehn.ehn_Object   = obj;
    data->ehn.ehn_Class    = cl;

    return (IPTR)obj;
}

/**************************************************************************
 OM_DISPOSE
**************************************************************************/
static IPTR IconList_Dispose(struct IClass *cl, Object *obj, Msg msg)
{
    struct MUI_IconData *data = INST_DATA(cl, obj);
    struct IconEntry *node;

    node = List_First(&data->icon_list);
    while (node)
    {
	if (node->dob) FreeDiskObject(node->dob);
	node = Node_Next(node);
    }

    if (data->pool) DeletePool(data->pool);

    DoSuperMethodA(cl,obj,msg);
    return 0;
}


/**************************************************************************
 OM_SET
**************************************************************************/
static IPTR IconList_Set(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct MUI_IconData *data = INST_DATA(cl, obj);
    struct TagItem  	*tag, *tags;
    WORD    	    	 oldleft = data->view_x, oldtop = data->view_y;
    
    /* parse initial taglist */
    for (tags = msg->ops_AttrList; (tag = NextTagItem(&tags)); )
    {
	switch (tag->ti_Tag)
	{
	    case    MUIA_IconList_Left:
		    if (data->view_x != tag->ti_Data)
		    {
			data->view_x = tag->ti_Data;
		    }
		    break;

	    case    MUIA_IconList_Top:
		    if (data->view_y != tag->ti_Data)
		    {
			data->view_y = tag->ti_Data;
		    }
		    break;
    	}
    }

    if ((oldleft != data->view_x) || (oldtop != data->view_y))
    {
    	data->update = UPDATE_SCROLL;
	data->update_scrolldx = data->view_x - oldleft;
	data->update_scrolldy = data->view_y - oldtop;
	
	MUI_Redraw(obj, MADF_DRAWUPDATE);
    }
    
    return DoSuperMethodA(cl, obj, (Msg)msg);
}

/**************************************************************************
 OM_GET
**************************************************************************/
static ULONG IconList_Get(struct IClass *cl, Object *obj, struct opGet *msg)
{
/* small macro to simplify return value storage */
#define STORE *(msg->opg_Storage)
    struct MUI_IconData *data = INST_DATA(cl, obj);

    switch (msg->opg_AttrID)
    {
	case MUIA_IconList_Left: STORE = data->view_x; return 1;
	case MUIA_IconList_Top: STORE = data->view_y; return 1;
	case MUIA_IconList_Width: STORE = data->width; return 1;
	case MUIA_IconList_Height: STORE = data->height; return 1;
	case MUIA_IconList_IconsDropped: STORE = (ULONG)data->drop_entry; return 1;
	case MUIA_IconList_Clicked: STORE = (ULONG)&data->icon_click; return 1;
    }

    if (DoSuperMethodA(cl, obj, (Msg) msg)) return 1;
    return 0;
#undef STORE
}

/**************************************************************************
 MUIM_Setup
**************************************************************************/
static ULONG IconList_Setup(struct IClass *cl, Object *obj, struct MUIP_Setup *msg)
{
    struct MUI_IconData *data = INST_DATA(cl, obj);
    struct IconEntry *node;

    if (!DoSuperMethodA(cl, obj, (Msg) msg)) return 0;

    DoMethod(_win(obj),MUIM_Window_AddEventHandler, (IPTR)&data->ehn);

    node = List_First(&data->icon_list);
    while (node)
    {
	if (!node->dob)
	{
	    node->dob = GetIconTags
            (
                node->entry.filename, 
                ICONGETA_FailIfUnavailable,        FALSE, 
                ICONGETA_Label,             (IPTR) node->entry.label,
                TAG_DONE
            );
	}
	node = Node_Next(node);
    }

    return 1;
}

/**************************************************************************
 MUIM_Show
**************************************************************************/
static IPTR IconList_Show(struct IClass *cl, Object *obj, struct MUIP_Show *msg)
{
    struct MUI_IconData *data = INST_DATA(cl, obj);
    WORD newleft, newtop;
    IPTR rc;
    
    rc = DoSuperMethodA(cl, obj, (Msg)msg);

    newleft = data->view_x;
    newtop = data->view_y;
    
    if (newleft + _mwidth(obj) > data->width) newleft = data->width - _mwidth(obj);
    if (newleft < 0) newleft = 0;
    
    if (newtop + _mheight(obj) > data->height) newtop = data->height - _mheight(obj);
    if (newtop < 0) newtop = 0;

    if ((newleft != data->view_x) || (newtop != data->view_y))
    {    
	SetAttrs(obj, MUIA_IconList_Left, newleft,
    	    	      MUIA_IconList_Top, newtop,
		      TAG_DONE);
    }
    
    SetFont(_rp(obj), _font(obj));
    return rc;
}


/**************************************************************************
 MUIM_Cleanup
**************************************************************************/
static ULONG IconList_Cleanup(struct IClass *cl, Object *obj, struct MUIP_Cleanup *msg)
{
    struct MUI_IconData *data = INST_DATA(cl, obj);

    struct IconEntry *node;

    node = List_First(&data->icon_list);
    while (node)
    {
	if (node->dob)
	{
	    FreeDiskObject(node->dob);
	    node->dob = NULL;
	}
	node = Node_Next(node);
    }


    DoMethod(_win(obj),MUIM_Window_RemEventHandler, (IPTR)&data->ehn);
    return DoSuperMethodA(cl, obj, (Msg) msg);
}

/**************************************************************************
 MUIM_AskMinMax
**************************************************************************/
static ULONG IconList_AskMinMax(struct IClass *cl, Object *obj, struct MUIP_AskMinMax *msg)
{
    ULONG rc = DoSuperMethodA(cl, obj, (Msg) msg);

//    msg->MinMaxInfo->MinWidth  += 0;
//    msg->MinMaxInfo->MinHeight += 0;

    msg->MinMaxInfo->DefWidth  += 200;
    msg->MinMaxInfo->DefHeight += 180;

    msg->MinMaxInfo->MaxWidth = MUI_MAXMAX;
    msg->MinMaxInfo->MaxHeight = MUI_MAXMAX;

    return rc;
}

/**************************************************************************
 MUIM_Layout
**************************************************************************/
static ULONG IconList_Layout(struct IClass *cl, Object *obj,struct MUIP_Layout *msg)
{
    struct MUI_IconData *data = INST_DATA(cl, obj);
    ULONG rc = DoSuperMethodA(cl,obj,(Msg)msg);
    data->view_width = _mwidth(obj);
    data->view_height = _mheight(obj);
    return rc;
}

/**************************************************************************
 MUIM_Draw
**************************************************************************/
static ULONG IconList_Draw(struct IClass *cl, Object *obj, struct MUIP_Draw *msg)
{
    struct MUI_IconData *data = INST_DATA(cl, obj);
    APTR clip;
    struct IconEntry *icon;

    DoSuperMethodA(cl, obj, (Msg) msg);

    if (msg->flags & MADF_DRAWUPDATE)
    {
	if (data->update == UPDATE_SINGLEICON) /* draw only a single icon at update_icon */
	{
	    struct Rectangle rect;

	    IconList_GetIconRectangle(obj, data->update_icon, &rect);
	    rect.MinX += _mleft(obj) - data->view_x + data->update_icon->x;
	    rect.MaxX += _mleft(obj) - data->view_x + data->update_icon->x;
	    rect.MinY += _mtop(obj) - data->view_y + data->update_icon->y;
	    rect.MaxY += _mtop(obj) - data->view_y + data->update_icon->y;

	    clip = MUI_AddClipping(muiRenderInfo(obj), _mleft(obj), _mtop(obj), _mwidth(obj), _mheight(obj));

    	#if 1
            DoMethod(obj, MUIM_DrawBackground, rect.MinX, rect.MinY,
		     rect.MaxX - rect.MinX + 1, rect.MaxY - rect.MinY + 1,
		     data->view_x + (rect.MinX - _mleft(obj)),
		     data->view_y + (rect.MinY - _mtop(obj)), 0);
	#else
            DoMethod(obj, MUIM_DrawBackground, _mleft(obj), _mtop(obj),
		     _mwidth(obj), _mheight(obj),
		     data->view_x, data->view_y, 0);
    	#endif
	
	    /* We could have deleted also other icons so they must be redrawn */
	    icon = List_First(&data->icon_list);
	    while (icon)
	    {
		if (icon != data->update_icon)
		{
		    struct Rectangle rect2;
		    IconList_GetIconRectangle(obj, icon, &rect2);

		    rect2.MinX += _mleft(obj) - data->view_x + icon->x;
		    rect2.MaxX += _mleft(obj) - data->view_x + icon->x;
		    rect2.MinY += _mtop(obj) - data->view_y + icon->y;
		    rect2.MaxY += _mtop(obj) - data->view_y + icon->y;

		    if (RectAndRect(&rect,&rect2))
		    {
			IconList_DrawIcon(obj, data, icon);
		    }
		}
		icon = Node_Next(icon);
	    }

	    IconList_DrawIcon(obj, data, data->update_icon);
	    data->update = 0;
	    MUI_RemoveClipping(muiRenderInfo(obj),clip);
	    return 0;
	}
	else if (data->update == UPDATE_SCROLL)
	{
	    struct Region   	*region;
	    struct Rectangle 	 xrect, yrect;
	    BOOL    	    	 scroll_caused_damage;

	    scroll_caused_damage = (_rp(obj)->Layer->Flags & LAYERREFRESH) ? FALSE : TRUE;

	    data->update = 0;

	    if ((abs(data->update_scrolldx) >= _mwidth(obj)) ||
	    	(abs(data->update_scrolldy) >= _mheight(obj)))
	    {
		MUI_Redraw(obj, MADF_DRAWOBJECT);
		return 0;
	    }

	    region = NewRegion();
	    if (!region)
	    {
		MUI_Redraw(obj, MADF_DRAWOBJECT);
		return 0;
	    }

	    if (data->update_scrolldx > 0)
	    {
		xrect.MinX = _mright(obj) - data->update_scrolldx;
		xrect.MinY = _mtop(obj);
		xrect.MaxX = _mright(obj);
		xrect.MaxY = _mbottom(obj);

		OrRectRegion(region, &xrect);
		
		data->update_rect1 = &xrect;
	    }
	    else if (data->update_scrolldx < 0)
	    {
		xrect.MinX = _mleft(obj);
		xrect.MinY = _mtop(obj);
		xrect.MaxX = _mleft(obj) - data->update_scrolldx;
		xrect.MaxY = _mbottom(obj);

		OrRectRegion(region, &xrect);

		data->update_rect1 = &xrect;
	    }

	    if (data->update_scrolldy > 0)
	    {
		yrect.MinX = _mleft(obj);
		yrect.MinY = _mbottom(obj) - data->update_scrolldy;
		yrect.MaxX = _mright(obj);
		yrect.MaxY = _mbottom(obj);

		OrRectRegion(region, &yrect);

		data->update_rect2 = &yrect;
	    }
	    else if (data->update_scrolldy < 0)
	    {
		yrect.MinX = _mleft(obj);
		yrect.MinY = _mtop(obj);
		yrect.MaxX = _mright(obj);
		yrect.MaxY = _mtop(obj) - data->update_scrolldy;

		OrRectRegion(region, &yrect);

		data->update_rect2 = &yrect;
	    }

	    ScrollRasterBF(_rp(obj),
	    	    	   data->update_scrolldx,
			   data->update_scrolldy,
			   _mleft(obj),
			   _mtop(obj),
			   _mright(obj),
			   _mbottom(obj));

    	    scroll_caused_damage = scroll_caused_damage && (_rp(obj)->Layer->Flags & LAYERREFRESH) ? TRUE : FALSE;

	    clip = MUI_AddClipRegion(muiRenderInfo(obj), region);

	    MUI_Redraw(obj, MADF_DRAWOBJECT);

    	    data->update_rect1 = data->update_rect2 = NULL;
	    
	    MUI_RemoveClipRegion(muiRenderInfo(obj), clip);

//	    DisposeRegion(region);

	    if (scroll_caused_damage)
	    {
    		if (MUI_BeginRefresh(muiRenderInfo(obj), 0))
		{
		    /* Theoretically it might happen that more damage is caused
		       after ScrollRaster. By something else, like window movement
		       in front of our window. Therefore refresh root object of
		       window, not just this object */

		    Object *o;

		    get(_win(obj),MUIA_Window_RootObject, &o);
		    MUI_Redraw(o, MADF_DRAWOBJECT);

		    MUI_EndRefresh(muiRenderInfo(obj), 0);
		}
	    }

	    return 0;
	}

    } else
    {
	DoMethod(obj, MUIM_DrawBackground, _mleft(obj), _mtop(obj), _mwidth(obj), _mheight(obj),
		data->view_x, data->view_y, 0);
    }


    /* At we see if there any Icons without proper position, this is the wrong place here,
     * it should be done after all icons have been loaded */

    IconList_FixNoPositionIcons(obj, data);

    clip = MUI_AddClipping(muiRenderInfo(obj), _mleft(obj), _mtop(obj), _mwidth(obj), _mheight(obj));

    icon = List_First(&data->icon_list);
    while (icon)
    {
	if (icon->dob && icon->x != NO_ICON_POSITION && icon->y != NO_ICON_POSITION)
	{
	    IconList_DrawIcon(obj, data, icon);
	}
	icon = Node_Next(icon);
    }

    MUI_RemoveClipping(muiRenderInfo(obj),clip);

    data->update = 0;

    return 0;
}

/**************************************************************************
 MUIM_IconList_Refresh
 Implemented by subclasses
**************************************************************************/
static ULONG IconList_Update(struct IClass *cl, Object *obj, struct MUIP_IconList_Update *msg)
{
    return 1;
}

/**************************************************************************
 MUIM_IconList_Clear
**************************************************************************/
static ULONG IconList_Clear(struct IClass *cl, Object *obj, struct MUIP_IconList_Clear *msg)
{
    struct MUI_IconData *data = INST_DATA(cl, obj);
    struct IconEntry *node;

    while ((node = (struct IconEntry*)RemTail((struct List*)&data->icon_list)))
    {
	if (node->dob) FreeDiskObject(node->dob);
	if (node->entry.label) FreePooled(data->pool,node->entry.label,strlen(node->entry.label)+1);
	if (node->entry.filename) FreePooled(data->pool,node->entry.filename,strlen(node->entry.filename)+1);
        FreePooled(data->pool,node,sizeof(struct IconEntry));
    }

    data->first_selected = NULL;
    data->view_x = data->view_y = data->width = data->height = 0;

    SetAttrs(obj, MUIA_IconList_Left, data->view_x,
    	    	  MUIA_IconList_Top, data->view_y,
		  MUIA_IconList_Width, data->width,
		  MUIA_IconList_Height, data->height,
		  TAG_DONE);
		  
    MUI_Redraw(obj,MADF_DRAWOBJECT);
    return 1;
}

/**************************************************************************
 MUIM_IconList_Add.
 Returns 0 on failure otherwise 1
**************************************************************************/
static IPTR IconList_Add(struct IClass *cl, Object *obj, struct MUIP_IconList_Add *msg)
{
    struct MUI_IconData *data = INST_DATA(cl, obj);
    struct IconEntry *entry;
    struct DiskObject *dob;
    struct Rectangle rect;
    
    dob = GetIconTags
    (
        msg->filename, 
        ICONGETA_FailIfUnavailable,        FALSE,
        ICONGETA_Label,             (IPTR) msg->label,
        TAG_DONE
    );
    
    if (!dob)
    {
	return 0;
    }

    if (!(entry = AllocPooled(data->pool,sizeof(struct IconEntry))))
    {
	FreeDiskObject(dob);
	return 0;
    }

    memset(entry,0,sizeof(struct IconEntry));

    if (!(entry->entry.filename = AllocPooled(data->pool,strlen(msg->filename)+1)))
    {
	FreePooled(data->pool,entry,sizeof(struct IconEntry));
	FreeDiskObject(dob);
	return 0;
    }

    strcpy(entry->entry.filename,msg->filename);

    if (!(entry->entry.label = AllocPooled(data->pool,strlen(msg->label)+1)))
    {
    	FreePooled(data->pool,entry->entry.filename,strlen(entry->entry.filename)+1);
	FreePooled(data->pool,entry,sizeof(struct IconEntry));
	FreeDiskObject(dob);
	return 0;
    }

    strcpy(entry->entry.label,msg->label);

    GetIconRectangleA(NULL,dob,NULL,&rect,NULL);

    entry->dob = dob;
    entry->entry.udata = msg->udata;
    entry->entry.type = msg->type;

    entry->x = dob->do_CurrentX;
    entry->y = dob->do_CurrentY;
    entry->width = rect.MaxX - rect.MinX + 1;
    entry->height = rect.MaxY - rect.MinY + 1;

#if 0
    if (entry->x != NO_ICON_POSITION)
    {
	if (entry->x < data->view_x) data->view_x = entry->x;
	if (entry->x + entry->width - data->view_x > data->width) data->width = entry->x + entry->width - data->view_x;
    }

    if (entry->y != NO_ICON_POSITION)
    {
	if (entry->y < data->view_y) data->view_y = entry->y;
	if (entry->y + entry->height - data->view_y > data->height) data->height = entry->y + entry->height - data->view_y;
    }
#endif

    strcpy(entry->entry.filename,msg->filename);

    {
    	struct IconEntry *icon1, *icon2=NULL;
	icon1 = List_First(&data->icon_list);
	while (icon1)
	{
	    if (Stricmp(entry->entry.label,icon1->entry.label)<0) break;
	    icon2 = icon1;
	    icon1 = Node_Next(icon1);
	}
	Insert((struct List*)&data->icon_list,(struct Node*)entry,(struct Node*)icon2);
    }

#if 0
    set(obj, MUIA_IconList_Width, data->width);
    set(obj, MUIA_IconList_Height, data->height);
#endif
    
    return 1;
}

/**************************************************************************
 MUIM_HandleEvent
**************************************************************************/
static ULONG IconList_HandleEvent(struct IClass *cl, Object *obj, struct MUIP_HandleEvent *msg)
{
    struct MUI_IconData *data = INST_DATA(cl, obj);

    if (msg->imsg)
    {
    	LONG mx = msg->imsg->MouseX - _mleft(obj);
    	LONG my = msg->imsg->MouseY - _mtop(obj);
	switch (msg->imsg->Class)
	{
	    case    IDCMP_MOUSEBUTTONS:
		    if (msg->imsg->Code == SELECTDOWN)
		    {
			if (mx >= 0 && mx < _width(obj) && my >= 0 && my < _height(obj))
			{
			    struct IconEntry *node;
			    struct IconEntry *new_selected = NULL;

			    data->first_selected = NULL;

			    node = List_First(&data->icon_list);
			    while (node)
			    {
			    	if (mx >= node->x - data->view_x && mx < node->x - data->view_x + node->width &&
			    	    my >= node->y - data->view_y && my < node->y - data->view_y + node->height && !new_selected)
			    	{
				    new_selected = node;

				    if (!node->selected)
				    {
					node->selected = 1;
					data->update = UPDATE_SINGLEICON;
					data->update_icon = node;
					MUI_Redraw(obj,MADF_DRAWUPDATE);
				    }

				    data->first_selected = node;
				} else
				{
				    if (node->selected)
				    {
					node->selected = 0;
					data->update = UPDATE_SINGLEICON;
					data->update_icon = node;
					MUI_Redraw(obj,MADF_DRAWUPDATE);
				    }
				}
				node = Node_Next(node);
			    }

			    data->icon_click.shift = !!(msg->imsg->Qualifier & (IEQUALIFIER_LSHIFT | IEQUALIFIER_RSHIFT));
			    data->icon_click.entry = new_selected?&new_selected->entry:NULL;
			    set(obj,MUIA_IconList_Clicked,(IPTR)&data->icon_click);

			    if (DoubleClick(data->last_secs, data->last_mics, msg->imsg->Seconds, msg->imsg->Micros) && data->last_selected == new_selected)
			    {
				set(obj,MUIA_IconList_DoubleClick, TRUE);
			    } else if (!data->mouse_pressed)
			    {
				data->last_selected = new_selected;
				data->last_secs = msg->imsg->Seconds;
				data->last_mics = msg->imsg->Micros;

				/* After a double click you often open a new window
				 * and since Zune doesn't not support the faking
				 * of SELECTUP events only change the Events
				 * if not doubleclicked */
				 
				data->mouse_pressed |= LEFT_BUTTON;

				if (!(data->ehn.ehn_Events & IDCMP_MOUSEMOVE))
				{
				    DoMethod(_win(obj),MUIM_Window_RemEventHandler, (IPTR)&data->ehn);
				    data->ehn.ehn_Events |= IDCMP_MOUSEMOVE;
				    DoMethod(_win(obj),MUIM_Window_AddEventHandler, (IPTR)&data->ehn);
				}
				
			    }

			    data->click_x = mx;
			    data->click_y = my;

			    return MUI_EventHandlerRC_Eat;
			}
		    }
		    else if (msg->imsg->Code == MIDDLEDOWN)
		    {
		    	if (!data->mouse_pressed)
			{
		    	    data->mouse_pressed |= MIDDLE_BUTTON;

    	    	    	    data->click_x = data->view_x + mx;
			    data->click_y = data->view_y + my;

			    if (!(data->ehn.ehn_Events & IDCMP_MOUSEMOVE))
			    {
				DoMethod(_win(obj),MUIM_Window_RemEventHandler, (IPTR)&data->ehn);
				data->ehn.ehn_Events |= IDCMP_MOUSEMOVE;
				DoMethod(_win(obj),MUIM_Window_AddEventHandler, (IPTR)&data->ehn);
			    }
			}			
		    }
		    else
		    {
		    	BOOL eat = FALSE;

		    	if (msg->imsg->Code == SELECTUP)
			{
			    data->mouse_pressed &= ~LEFT_BUTTON;
			    eat = TRUE;
			}
			
			if (msg->imsg->Code == MIDDLEUP)
			{
			    data->mouse_pressed &= ~MIDDLE_BUTTON;
			    eat = TRUE;
			}
			
			if ((data->ehn.ehn_Events & IDCMP_MOUSEMOVE) && !data->mouse_pressed)
			{
			    DoMethod(_win(obj),MUIM_Window_RemEventHandler, (IPTR)&data->ehn);
			    data->ehn.ehn_Events &= ~IDCMP_MOUSEMOVE;
			    DoMethod(_win(obj),MUIM_Window_AddEventHandler, (IPTR)&data->ehn);
			}
			
    	    	    	if (eat) return MUI_EventHandlerRC_Eat;			
		    
		    }
		    break;

	    case    IDCMP_MOUSEMOVE:
		    if (data->mouse_pressed & LEFT_BUTTON)
		    {
		    	int move_x = mx;
		    	int move_y = my;

			if (data->first_selected && (abs(move_x - data->click_x) >= 2 || abs(move_y - data->click_y) >= 2))
			{
			    DoMethod(_win(obj),MUIM_Window_RemEventHandler, (IPTR)&data->ehn);
			    data->ehn.ehn_Events &= ~IDCMP_MOUSEMOVE;
			    DoMethod(_win(obj),MUIM_Window_AddEventHandler, (IPTR)&data->ehn);

    	    	    	    data->mouse_pressed &= ~LEFT_BUTTON;
			    
			    data->touch_x = move_x + data->view_x - data->first_selected->x;
			    data->touch_y = move_y + data->view_y - data->first_selected->y;
			    DoMethod(obj,MUIM_DoDrag, data->touch_x, data->touch_y, 0);
			}

			return MUI_EventHandlerRC_Eat;
		    }
		    else if (data->mouse_pressed & MIDDLE_BUTTON)
		    {
		    	WORD newleft, newtop;
			
			newleft = data->click_x - mx;
			newtop = data->click_y - my;
			
			if (newleft + _mwidth(obj) > data->width) newleft = data->width - _mwidth(obj);
			if (newleft < 0) newleft = 0;
			
			if (newtop + _mheight(obj) > data->height) newtop = data->height - _mheight(obj);
			if (newtop < 0) newtop = 0;
			
			if ((newleft != data->view_x) || (newtop != data->view_y))
			{
			    SetAttrs(obj, MUIA_IconList_Left, newleft,
			    	    	  MUIA_IconList_Top, newtop,
					  TAG_DONE);
			}
			
		    	return MUI_EventHandlerRC_Eat;
		    }
		    
		    break;
	}
    }

    return 0;
}

/**************************************************************************
 MUIM_IconList_NextSelected
**************************************************************************/
static ULONG IconList_NextSelected(struct IClass *cl, Object *obj, struct MUIP_IconList_NextSelected *msg)
{
    struct MUI_IconData *data = INST_DATA(cl, obj);
    struct IconEntry *node;
    struct IconList_Entry *ent;

    if (!msg->entry) return NULL;
    ent = *msg->entry;

    if (((IPTR)ent) == MUIV_IconList_NextSelected_Start)
    {
	if (!(node = data->last_selected))
	{
	    *msg->entry = (struct IconList_Entry*)MUIV_IconList_NextSelected_End;
	} else
	{
	    *msg->entry = &node->entry;
	}
	return 0;
    }

    node = List_First(&data->icon_list); /* not really necessary but it avoids compiler warnings */

    node = (struct IconEntry*)(((char*)ent) - ((char*)(&node->entry) - (char*)node));
    node = Node_Next(node);

    while (node)
    {
	if (node->selected)
	{
	    *msg->entry = &node->entry;
	    return 0;
	}
	node = Node_Next(node);
    }

    *msg->entry = (struct IconList_Entry*)MUIV_IconList_NextSelected_End;

    return NULL;
}

/**************************************************************************
 MUIM_CreateDragImage
**************************************************************************/
static ULONG IconList_CreateDragImage(struct IClass *cl, Object *obj, struct MUIP_CreateDragImage *msg)
{
    struct MUI_IconData *data = INST_DATA(cl, obj);
    struct MUI_DragImage *img;

    if (!data->first_selected) DoSuperMethodA(cl,obj,(Msg)msg);

    if ((img = (struct MUI_DragImage *)AllocVec(sizeof(struct MUIP_CreateDragImage),MEMF_CLEAR)))
    {
    	struct IconEntry *node;
	LONG depth = GetBitMapAttr(_screen(obj)->RastPort.BitMap,BMA_DEPTH);

	node = data->first_selected;

    	img->width = node->width;
    	img->height = node->height;

    	if ((img->bm = AllocBitMap(img->width,img->height,depth,BMF_MINPLANES | BMF_CLEAR,_screen(obj)->RastPort.BitMap)))
    	{
    	    struct RastPort temprp;
    	    InitRastPort(&temprp);
    	    temprp.BitMap = img->bm;

#ifndef __AROS__
	    DrawIconState(&temprp,node->dob,NULL,0,0, node->selected?IDS_SELECTED:IDS_NORMAL, ICONDRAWA_EraseBackground, TRUE, TAG_DONE);
#else
	    DrawIconStateA(&temprp,node->dob,NULL,0,0, node->selected?IDS_SELECTED:IDS_NORMAL, NULL);
#endif
	    DeinitRastPort(&temprp);
    	}

    	img->touchx = msg->touchx;
    	img->touchy = msg->touchy;
    	img->flags = 0;
    }
    return (ULONG)img;
}

/**************************************************************************
 MUIM_DeleteDragImage
**************************************************************************/
static ULONG IconList_DeleteDragImage(struct IClass *cl, Object *obj, struct MUIP_DeleteDragImage *msg)
{
    struct MUI_IconData *data = INST_DATA(cl, obj);

    if (!data->first_selected) return DoSuperMethodA(cl,obj,(Msg)msg);

    if (msg->di)
    {
	if (msg->di->bm) FreeBitMap(msg->di->bm);
	FreeVec(msg->di);
    }
    return NULL;
}

/**************************************************************************
 MUIM_DragQuery
**************************************************************************/
static ULONG IconList_DragQuery(struct IClass *cl, Object *obj, struct MUIP_DragQuery *msg)
{
    if (msg->obj == obj) return MUIV_DragQuery_Accept;
    else
    {
	int is_iconlist = 0;
	struct IClass *msg_cl = OCLASS(msg->obj);

	while (msg_cl)
	{
	    if (msg_cl == cl)
	    {
	    	is_iconlist = 1;
	    	break;
	    }
	    msg_cl = msg_cl->cl_Super;
	}
	if (is_iconlist) return MUIV_DragQuery_Accept;
    }

    return MUIV_DragQuery_Refuse;
}

/**************************************************************************
 MUIM_DragDrop
**************************************************************************/
static ULONG IconList_DragDrop(struct IClass *cl, Object *obj, struct MUIP_DragDrop *msg)
{
    struct MUI_IconData *data = INST_DATA(cl, obj);

    if (msg->obj == obj)
    {
	struct IconEntry *icon = data->first_selected;
	
	if (icon)
	{
	    struct Rectangle old, new;
	    struct Region    *region;
	    APTR    	     clip = NULL;
	    
	    IconList_GetIconRectangle(obj, icon, &old);
	    old.MinX += _mleft(obj) - data->view_x + icon->x;
	    old.MaxX += _mleft(obj) - data->view_x + icon->x;
	    old.MinY += _mtop(obj) - data->view_y + icon->y;
	    old.MaxY += _mtop(obj) - data->view_y + icon->y;
	    
	    icon->x = msg->x - _mleft(obj) + data->view_x - data->touch_x;
	    icon->y = msg->y - _mtop(obj) + data->view_y - data->touch_y;
	    	    
	    IconList_RethinkDimensions(obj, data, data->first_selected);

	    IconList_GetIconRectangle(obj, data->first_selected, &new);
	    new.MinX += _mleft(obj) - data->view_x + icon->x;
	    new.MaxX += _mleft(obj) - data->view_x + icon->x;
	    new.MinY += _mtop(obj) - data->view_y + icon->y;
	    new.MaxY += _mtop(obj) - data->view_y + icon->y;
	    
	    region = NewRegion();
	    if (region)
	    {
	    	OrRectRegion(region, &old);
	    	OrRectRegion(region, &new);
		clip = MUI_AddClipRegion(muiRenderInfo(obj), region);
	    }
	    
	    MUI_Redraw(obj,MADF_DRAWOBJECT);
	    
	    if (region)
	    {
	    	MUI_RemoveClipRegion(muiRenderInfo(obj), clip);
	    }
	    
	}
    } else
    {
    	data->drop_entry = NULL;
    	set(obj, MUIA_IconList_IconsDropped, (IPTR)data->drop_entry); /* Now notify */
    }
    return DoSuperMethodA(cl,obj,(Msg)msg);
}

/**************************************************************************
 MUIM_UnselectAll
**************************************************************************/
static ULONG IconList_UnselectAll(struct IClass *cl, Object *obj, Msg msg)
{
    struct MUI_IconData *data = INST_DATA(cl, obj);
    struct IconEntry *node;

    node = List_First(&data->icon_list);
    while (node)
    {
	if (node->selected)
	{
	    node->selected = 0;
	    data->update = UPDATE_SINGLEICON;
	    data->update_icon = node;
	    MUI_Redraw(obj,MADF_DRAWUPDATE);
	}
	node = Node_Next(node);
    }
    data->first_selected = NULL;
    data->last_selected = NULL;
    return 1;
}

/**************************************************************************
 MUIM_DragReport. Since MUI doesn't change the drop object if the dragged
 object is moved above another window (while still in the bounds of the
 orginal drop object) we must do it here manually to be compatible with
 MUI. Maybe Zune should fix this bug somewhen.
**************************************************************************/
static ULONG IconList_DragReport(struct IClass *cl, Object *obj, struct MUIP_DragReport *msg)
{
    struct Window *wnd = _window(obj);
    struct Layer *l;

    l = WhichLayer(&wnd->WScreen->LayerInfo, wnd->LeftEdge + msg->x, wnd->TopEdge + msg->y);
    if (l != wnd->WLayer) return MUIV_DragReport_Abort;
    return MUIV_DragReport_Continue;
}

BOOPSI_DISPATCHER(IPTR,IconList_Dispatcher, cl, obj, msg)
{
    switch (msg->MethodID)
    {
	case OM_NEW: return IconList_New(cl, obj, (struct opSet *)msg);
	case OM_DISPOSE: return IconList_Dispose(cl,obj, msg);
	case OM_SET: return IconList_Set(cl,obj,(struct opSet *)msg);
	case OM_GET: return IconList_Get(cl,obj,(struct opGet *)msg);
	case MUIM_Setup: return IconList_Setup(cl,obj,(struct MUIP_Setup *)msg);
	case MUIM_Show: return IconList_Show(cl,obj,(struct MUIP_Show *)msg);
	case MUIM_Cleanup: return IconList_Cleanup(cl,obj,(struct MUIP_Cleanup *)msg);
	case MUIM_AskMinMax: return IconList_AskMinMax(cl,obj,(struct MUIP_AskMinMax *)msg);
	case MUIM_Draw: return IconList_Draw(cl,obj,(struct MUIP_Draw *)msg);
	case MUIM_Layout: return IconList_Layout(cl,obj,(struct MUIP_Layout *)msg);
	case MUIM_HandleEvent: return IconList_HandleEvent(cl,obj,(struct MUIP_HandleEvent *)msg);
	case MUIM_CreateDragImage: return IconList_CreateDragImage(cl,obj,(APTR)msg);
	case MUIM_DeleteDragImage: return IconList_DeleteDragImage(cl,obj,(APTR)msg);
	case MUIM_DragQuery: return IconList_DragQuery(cl,obj,(APTR)msg);
	case MUIM_DragReport: return IconList_DragReport(cl,obj,(APTR)msg);
	case MUIM_DragDrop: return IconList_DragDrop(cl,obj,(APTR)msg);

	case MUIM_IconList_Update: return IconList_Update(cl,obj,(APTR)msg);
	case MUIM_IconList_Clear: return IconList_Clear(cl,obj,(APTR)msg);
	case MUIM_IconList_Add: return IconList_Add(cl,obj,(APTR)msg);
	case MUIM_IconList_NextSelected: return IconList_NextSelected(cl,obj,(APTR)msg);
	case MUIM_IconList_UnselectAll: return IconList_UnselectAll(cl,obj,(APTR)msg);
    }
    
    return DoSuperMethodA(cl, obj, msg);
}

struct MUI_IconDrawerData
{
    char *drawer;
};

/**************************************************************************
 Read icons in
**************************************************************************/
static int ReadIcons(struct IClass *cl, Object *obj)
{
    struct MUI_IconDrawerData *data = INST_DATA(cl, obj);
    BPTR lock;
    struct ExAllControl *eac;
    struct ExAllData *entry;
    void *ead;
    LONG more;
    BPTR olddir;
    //char pattern[40];
    char filename[256];

    if (!data->drawer) return 1;
    lock = Lock(data->drawer,ACCESS_READ);
    if (!lock) return 0;

    eac = (struct ExAllControl*)AllocDosObject(DOS_EXALLCONTROL,NULL);
    if (!eac)
    {
	UnLock(lock);
	return 0;
    }

    ead = AllocVec(1024,0);
    if (!ead)
    {
	FreeDosObject(DOS_EXALLCONTROL,eac);
	UnLock(lock);
	return 0;
    }

/*
    ParsePatternNoCase("#?.info",pattern,sizeof(pattern));
    eac->eac_MatchString = pattern;
*/
#ifdef __AROS__
#warning AROS ExAll() doesnt support eac_MatchString
#endif
    eac->eac_MatchString = NULL;
    eac->eac_LastKey = 0;

    olddir = CurrentDir(lock);

    do
    {
	more = ExAll(lock,ead,1024,ED_TYPE,eac);
	if ((!more) && (IoErr() != ERROR_NO_MORE_ENTRIES)) break;
	if (eac->eac_Entries == 0) continue;

	entry = (struct ExAllData*)ead;
	do
	{
	    int len;

	    strcpy(filename,entry->ed_Name);

/*
	    // if we only display icons

	    filename[strlen(filename)-5]=0;
*/
            len = strlen(filename);
	    if (len >= 5)
	    {
		/* reject all .info files, so we have a Show All mode */
		if (!Stricmp(&filename[len-5],".info"))
		    continue;
	    }

	    if (Stricmp(filename,"Disk")) /* skip disk.info */
	    {
		char buf[512];
		strcpy(buf,data->drawer);
		AddPart(buf,filename,sizeof(buf));

		if (!(DoMethod(obj,MUIM_IconList_Add,(IPTR)buf,(IPTR)filename,entry->ed_Type,NULL /* udata */)))
		{
		}
	    }
	}   while ((entry = entry->ed_Next));
    } while (more);

    CurrentDir(olddir);

    FreeVec(ead);
    FreeDosObject(DOS_EXALLCONTROL,eac);
    UnLock(lock);
    return 1;
}

/**************************************************************************
 OM_NEW
**************************************************************************/
static IPTR IconDrawerList_New(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct MUI_IconDrawerData   *data;
    struct TagItem  	    *tag, *tags;

    obj = (Object *)DoSuperNewTags(cl, obj, NULL,
    	TAG_MORE, (IPTR) msg->ops_AttrList);
    if (!obj) return FALSE;

    data = INST_DATA(cl, obj);

    /* parse initial taglist */
    for (tags = msg->ops_AttrList; (tag = NextTagItem(&tags)); )
    {
	switch (tag->ti_Tag)
	{
	    case    MUIA_IconDrawerList_Drawer:
		    data->drawer = StrDup((char*)tag->ti_Data);
		    break;
    	}
    }

    return (IPTR)obj;
}

/**************************************************************************
 OM_DISPOSE
**************************************************************************/
static IPTR IconDrawerList_Dispose(struct IClass *cl, Object *obj, Msg msg)
{
    struct MUI_IconDrawerData *data = INST_DATA(cl, obj);

    if (data->drawer) FreeVec(data->drawer);
    return DoSuperMethodA(cl,obj,msg);
}

/**************************************************************************
 OM_SET
**************************************************************************/
static IPTR IconDrawerList_Set(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct MUI_IconDrawerData   *data = INST_DATA(cl, obj);
    struct TagItem  	    *tag, *tags;

    /* parse initial taglist */
    for (tags = msg->ops_AttrList; (tag = NextTagItem(&tags)); )
    {
	switch (tag->ti_Tag)
	{
	    case    MUIA_IconDrawerList_Drawer:
		    if (data->drawer) FreeVec(data->drawer);
		    data->drawer = StrDup((char*)tag->ti_Data);
		    DoMethod(obj,MUIM_IconList_Update);
		    break;
    	}
    }

    return DoSuperMethodA(cl, obj, (Msg)msg);
}

/**************************************************************************
 OM_GET
**************************************************************************/
static ULONG IconDrawerList_Get(struct IClass *cl, Object *obj, struct opGet *msg)
{
/* small macro to simplify return value storage */
#define STORE *(msg->opg_Storage)
    struct MUI_IconDrawerData *data = INST_DATA(cl, obj);

    switch (msg->opg_AttrID)
    {
	case MUIA_IconDrawerList_Drawer: STORE = (unsigned long)data->drawer; return 1;
    }

    if (DoSuperMethodA(cl, obj, (Msg) msg)) return 1;
    return 0;
#undef STORE
}

/**************************************************************************
 MUIM_IconList_Update
**************************************************************************/
ULONG IconDrawerList_Update(struct IClass *cl, Object *obj, struct MUIP_IconList_Update *msg)
{
    //struct MUI_IconDrawerData *data = INST_DATA(cl, obj);
    //struct IconEntry *node;
    DoMethod(obj,MUIM_IconList_Clear);

    /* If not in setup do nothing */
    if (!(_flags(obj)&MADF_SETUP)) return 1;
    ReadIcons(cl,obj);

    MUI_Redraw(obj,MADF_DRAWOBJECT);
    return 1;
}


BOOPSI_DISPATCHER(IPTR, IconDrawerList_Dispatcher, cl, obj, msg)
{
    switch (msg->MethodID)
    {
	case OM_NEW: return IconDrawerList_New(cl, obj, (struct opSet *)msg);
	case OM_DISPOSE: return IconDrawerList_Dispose(cl,obj,msg);
	case OM_SET: return IconDrawerList_Set(cl, obj, (struct opSet *)msg);
	case OM_GET: return IconDrawerList_Get(cl, obj, (struct opGet *)msg);
	case MUIM_IconList_Update: return IconDrawerList_Update(cl,obj,(APTR)msg);
    }

    return DoSuperMethodA(cl, obj, msg);
}

/* sba: taken from SimpleFind3 */

struct NewDosList
{
	struct MinList list;
	APTR pool;
};

struct NewDosNode
{
    struct MinNode node;
    STRPTR name;
    STRPTR device;
    struct MsgPort *port;
};

static struct NewDosList *DosList_Create(void)
{
	APTR pool = CreatePool(MEMF_PUBLIC,4096,4096);
	if (pool)
	{
		struct NewDosList *ndl = (struct NewDosList*)AllocPooled(pool,sizeof(struct NewDosList));
		if (ndl)
		{
			struct DosList *dl;

			NewList((struct List*)ndl);
			ndl->pool = pool;

			dl = LockDosList(LDF_VOLUMES|LDF_READ);
			while(( dl = NextDosEntry(dl, LDF_VOLUMES)))
			{
				STRPTR name;
#ifndef __AROS__
				UBYTE *dosname = (UBYTE*)BADDR(dl->dol_Name);
				LONG len = dosname[0];
				dosname++;
#else
				UBYTE *dosname = dl->dol_DevName;
				LONG len = strlen(dosname);
#endif

				if ((name = (STRPTR)AllocPooled(pool, len+1)))
				{
					struct NewDosNode *ndn;

					name[len] = 0;
					strncpy(name,dosname,len);

					if ((ndn = (struct NewDosNode*)AllocPooled(pool, sizeof(*ndn))))
					{
						ndn->name = name;
						ndn->device = NULL;
#ifndef __AROS__
						ndn->port = dl->dol_Task;
#else
						ndn->port = NULL;
#endif
						AddTail((struct List*)ndl,(struct Node*)ndn);
					}
				}
			}
			UnLockDosList(LDF_VOLUMES|LDF_READ);

#ifndef __AROS__
			dl = LockDosList(LDF_DEVICES|LDF_READ);
			while(( dl = NextDosEntry(dl, LDF_DEVICES)))
			{
				struct NewDosNode *ndn;

				if (!dl->dol_Task) continue;

				ndn = (struct NewDosNode*)List_First(ndl);
				while ((ndn))
				{
					if (dl->dol_Task == ndn->port)
					{
						STRPTR name;
						UBYTE *dosname = (UBYTE*)BADDR(dl->dol_Name);
						LONG len = dosname[0];

						if ((name = (STRPTR)AllocPooled(pool, len+1)))
						{
							name[len] = 0;
							strncpy(name,&dosname[1],len);
						}

						ndn->device = name;
						break;
					}

					ndn = (struct NewDosNode*)Node_Next(ndn);
				}
			}
			UnLockDosList(LDF_DEVICES|LDF_READ);
#endif

			return ndl;
		}
		DeletePool(pool);
	}
	return NULL;
}

static void DosList_Dispose(struct NewDosList *ndl)
{
	if (ndl && ndl->pool) DeletePool(ndl->pool);
}
/* sba: End SimpleFind3 */


struct MUI_IconVolumneData
{
    int dummy;
};

/**************************************************************************
 OM_NEW
**************************************************************************/
static IPTR IconVolumeList_New(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct MUI_IconDrawerData   *data;
    struct TagItem  	    *tag, *tags;
    
    obj = (Object *)DoSuperNewTags(cl, obj, NULL,
    	TAG_MORE, (IPTR) msg->ops_AttrList);
    if (!obj) return FALSE;

    data = INST_DATA(cl, obj);

    /* parse initial taglist */
    for (tags = msg->ops_AttrList; (tag = NextTagItem(&tags)); )
    {
	switch (tag->ti_Tag)
	{
    	}
    }

    return (IPTR)obj;
}

/**************************************************************************
 MUIM_IconList_Update
**************************************************************************/
ULONG IconVolumeList_Update(struct IClass *cl, Object *obj, struct MUIP_IconList_Update *msg)
{
    //struct MUI_IconVolumeData *data = INST_DATA(cl, obj);
    //struct IconEntry *node;
    struct NewDosList *ndl;
    DoMethod(obj,MUIM_IconList_Clear);

    /* If not in setup do nothing */
    if (!(_flags(obj)&MADF_SETUP)) return 1;

    if ((ndl = DosList_Create()))
    {
	struct NewDosNode *nd = List_First(ndl);
	while (nd)
	{
	    char buf[300];
	    if (nd->name)
	    {
		strcpy(buf,nd->name);
		strcat(buf,":Disk");

		if (!(DoMethod(obj,MUIM_IconList_Add,(IPTR)buf,(IPTR)nd->name,ST_USERDIR, NULL/* udata */)))
		{
		}

	    }
	    nd = Node_Next(nd);
	}
	DosList_Dispose(ndl);
    }

    MUI_Redraw(obj,MADF_DRAWOBJECT);
    return 1;
}


BOOPSI_DISPATCHER(IPTR, IconVolumeList_Dispatcher, cl, obj, msg)
{
    switch (msg->MethodID)
    {
	case OM_NEW: return IconVolumeList_New(cl, obj, (struct opSet *)msg);
	case MUIM_IconList_Update: return IconVolumeList_Update(cl,obj,(APTR)msg);
    }
    
    return DoSuperMethodA(cl, obj, msg);
}

/*
 * Class descriptor.
 */
const struct __MUIBuiltinClass _MUI_IconList_desc = { 
    MUIC_IconList, 
    MUIC_Area, 
    sizeof(struct MUI_IconData), 
    (void*)IconList_Dispatcher
};

const struct __MUIBuiltinClass _MUI_IconDrawerList_desc = { 
    MUIC_IconDrawerList, 
    MUIC_IconList, 
    sizeof(struct MUI_IconDrawerData), 
    (void*)IconDrawerList_Dispatcher 
};

const struct __MUIBuiltinClass _MUI_IconVolumeList_desc = { 
    MUIC_IconVolumeList, 
    MUIC_IconList, 
    sizeof(struct MUI_IconVolumneData), 
    (void*)IconVolumeList_Dispatcher
};

