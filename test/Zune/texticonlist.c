/*
    Copyright © 2002, The AROS Development Team.
    All rights reserved.

    $Id$
*/

#define MUIMASTER_YES_INLINE_STDARG

#include <exec/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <dos/dos.h>
#include <dos/datetime.h>
#include <intuition/gadgetclass.h>
#include <intuition/icclass.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <proto/utility.h>
#include <proto/graphics.h>
#include <clib/alib_protos.h>

#include <aros/debug.h>

/* the following should go in a single include file which then only
** constits of the public constants and members. Actually this is easiey
*/

#include <libraries/mui.h>

struct Library *MUIMasterBase;

Object *app;
struct MUI_CustomClass *CL_TextIconList, *CL_TextIconListview;

/*================== TextIconList class =====================*/

#define NUM_COLUMNS 6

#define MIN_COLUMN_WIDTH 10

#define COLUMN_ALIGN_LEFT   0
#define COLUMN_ALIGN_CENTER 1
#define COLUMN_ALIGN_RIGHT  2

#define LINE_SPACING_TOP    2
#define LINE_SPACING_BOTTOM 2
#define LINE_EXTRAHEIGHT    (LINE_SPACING_TOP + LINE_SPACING_BOTTOM)

#define LINE_SPACING_LEFT   1
#define LINE_SPACING_RIGHT  1
#define LINE_EXTRAWIDTH     (LINE_SPACING_LEFT + LINE_SPACING_RIGHT)

#define ENTRY_SPACING_LEFT  1
#define ENTRY_SPACING_RIGHT 1
#define ENTRY_EXTRAWIDTH    (ENTRY_SPACING_LEFT + ENTRY_SPACING_RIGHT)

#define HEADERLINE_SPACING_TOP	    	3
#define HEADERLINE_SPACING_BOTTOM   	3
#define HEADERLINE_EXTRAHEIGHT	    	(HEADERLINE_SPACING_TOP + HEADERLINE_SPACING_BOTTOM)

#define HEADERLINE_SPACING_LEFT 	1
#define HEADERLINE_SPACING_RIGHT	1
#define HEADERLINE_EXTRAWIDTH 	    	(HEADERLINE_SPACING_LEFT + HEADERLINE_SPACING_RIGHT)

#define HEADERENTRY_SPACING_LEFT	4
#define HEADERENTRY_SPACING_RIGHT   	4
#define HEADERENTRY_EXTRAWIDTH	    	(HEADERENTRY_SPACING_LEFT + HEADERENTRY_SPACING_RIGHT)


struct TextIconEntry
{
    struct MinNode  	    node;
    struct FileInfoBlock    fib;
    LONG    	    	    field_width[NUM_COLUMNS];
    UBYTE   	    	    datebuf[LEN_DATSTRING];
    UBYTE   	    	    timebuf[LEN_DATSTRING];
    UBYTE   	    	    sizebuf[30];
    UBYTE   	    	    protbuf[8];
    UBYTE     	    	    selected;
    UBYTE   	    	    dirty;
};

struct TextIconList_DATA
{
    APTR    	    	    	pool;
    LONG    	    	    	view_x, view_y;
    LONG    	    	    	view_width, view_height;
    LONG    	    	    	width, height, lineheight, headerheight;
    LONG    	    	    	update;
    LONG    	    	    	update_scrolldx;
    LONG    	    	    	update_scrolldy;
    LONG    	    	    	update_entry;
    struct MinList  	    	entries_list;
    struct RastPort 	    	temprp;
    struct Rectangle	    	view_rect;
    struct Rectangle	    	header_rect;
    struct Rectangle	    	*update_rect1, *update_rect2;
    LONG    	    	    	num_entries;
    LONG    	    	    	num_selected;
    LONG    	    	    	active_entry;
    LONG    	    	    	click_x, click_y, click_column;
    LONG    	    	    	column_pos[NUM_COLUMNS];
    LONG    	    	    	column_maxwidth[NUM_COLUMNS];
    LONG    	    	    	column_width[NUM_COLUMNS];
    BYTE    	    	    	column_visible[NUM_COLUMNS];
    BYTE    	    	    	column_align[NUM_COLUMNS];
    BYTE    	    	    	column_clickable[NUM_COLUMNS];
    STRPTR  	    	    	column_title[NUM_COLUMNS];
    LONG    	    	    	inputstate;
    BOOL    	    	    	show_header;
    BOOL    	    	    	is_setup;
    BOOL    	    	    	is_shown;
    
    struct MUI_EventHandlerNode ehn;    
};

#define UPDATE_SCROLL	    	2
#define UPDATE_DIRTY_ENTRIES  	3
#define UPDATE_ALL  	    	4

#define INPUTSTATE_NONE     	0
#define INPUTSTATE_PAN	    	1
#define INPUTSTATE_COL_RESIZE 	2

#define MUIB_TextIconList   	    (MUIB_AROS | 0x00000700)

#define MUIA_TextIconList_Left      (MUIB_TextIconList | 0x00000000)
#define MUIA_TextIconList_Top       (MUIB_TextIconList | 0x00000001)
#define MUIA_TextIconList_Width     (MUIB_TextIconList | 0x00000002)
#define MUIA_TextIconList_Height    (MUIB_TextIconList | 0x00000003)
#define MUIA_TextIconList_VisWidth  (MUIB_TextIconList | 0x00000004)
#define MUIA_TextIconList_VisHeight (MUIB_TextIconList | 0x00000005)

#define MUIM_TextIconList_Clear     (MUIB_TextIconList | 0x00000000)
#define MUIM_TextIconList_Add	    (MUIB_TextIconList | 0x00000001)

struct MUIP_TextIconList_Clear      {STACKULONG MethodID;};
struct MUIP_TextIconList_Add        {STACKULONG MethodID; struct FileInfoBlock *fib;};

#define TextIconListObject  	    BOOPSIOBJMACRO_START(CL_TextIconList->mcc_Class)


#define INDEX_NAME  	    0
#define INDEX_SIZE  	    1
#define INDEX_PROTECTION    2
#define INDEX_DATE  	    3
#define INDEX_TIME  	    4
#define INDEX_COMMENT 	    5

static STRPTR GetTextIconEntryText(struct TextIconList_DATA *data, struct TextIconEntry *entry,
    	    	    	           LONG index)
{
    STRPTR ret = 0;

    switch(index)
    {
	case INDEX_NAME:
	    ret = entry->fib.fib_FileName;
	    break;

	case INDEX_SIZE:
	    ret = entry->sizebuf;
	    break;

	case INDEX_DATE:
	    ret = entry->datebuf;
	    break;

	case INDEX_TIME:
	    ret = entry->timebuf;
	    break;

	case INDEX_COMMENT:
	    ret = entry->fib.fib_Comment;
	    break;

	case INDEX_PROTECTION:
	    ret = entry->protbuf;
	    break;			    
    }
    
    return ret;
}

static STRPTR GetTextIconHeaderText(struct TextIconList_DATA *data, LONG index)
{
    STRPTR ret = 0;

    ret = data->column_title[index];
    if (ret) return ret;
    
    switch(index)
    {
	case INDEX_NAME:
	    ret = "Name";
	    break;

	case INDEX_SIZE:
	    ret = "Size";
	    break;

	case INDEX_DATE:
	    ret = "Date";
	    break;

	case INDEX_TIME:
	    ret = "Time";
	    break;

	case INDEX_COMMENT:
	    ret = "Comment";
	    break;

	case INDEX_PROTECTION:
	    ret = "Protection";
	    break;			    
    }
    
    return ret;
}

static void CalcWidth(struct TextIconList_DATA *data)
{
    LONG i, width = LINE_EXTRAWIDTH;
    
    for(i = 0; i < NUM_COLUMNS; i++)
    {
	if (data->column_visible[i])
	{
	    width += data->column_width[i];
	}
    }
    
    data->width = width;
}

static void CalcEntryDimension(struct TextIconList_DATA *data, struct TextIconEntry *entry)
{
    LONG i;
    STRPTR text;
    LONG len;
    
    for (i = 0; i < NUM_COLUMNS; i++)
    {
    	text = GetTextIconEntryText(data, entry, i);
	len = TextLength(&data->temprp, text, strlen(text));
	
	entry->field_width[i] = len + ENTRY_EXTRAWIDTH;
	
	if (entry->field_width[i] > data->column_maxwidth[i])
	{
	    data->column_maxwidth[i] = entry->field_width[i];
	}
	
    }
}

static void CalcAllEntryDimensions(struct TextIconList_DATA *data)
{
    struct TextIconEntry *entry;
    
    ForeachNode(&data->entries_list, entry)
    {
    	CalcEntryDimension(data, entry);
    }    
}

static void RecalcColumnMaxWidths(struct TextIconList_DATA *data)
{
    struct TextIconEntry *entry;
    LONG i;
    
    for(i = 0; i < NUM_COLUMNS; i++)
    {
    	data->column_maxwidth[i] = 0;
    }
    
    ForeachNode(&data->entries_list, entry)
    {
	for(i = 0; i < NUM_COLUMNS; i++)
	{
    	    if (entry->field_width[i] > data->column_maxwidth[i])
	    {
	    	data->column_maxwidth[i] = entry->field_width[i];
	    }
	}    	
    }        
}

static LONG FirstVisibleColumnNumber(struct TextIconList_DATA *data)
{
    LONG i;
    LONG retval = -1;
    
    for(i = 0; i < NUM_COLUMNS; i++)
    {
    	LONG index = data->column_pos[i];
	
	if (data->column_visible[index])
	{
	    retval = i;
	    break;
	}
    }
    
    return retval;
}

static LONG LastVisibleColumnNumber(struct TextIconList_DATA *data)
{
    LONG i;
    LONG retval = -1;
    
    for(i = 0; i < NUM_COLUMNS; i++)
    {
    	LONG index = data->column_pos[i];
	
	if (data->column_visible[index])
	{
	    retval = i;
	}
    }
    
    return retval;
    
}

static struct TextIconEntry *GetEntryFromIndex(struct TextIconList_DATA *data, LONG index)
{
    struct TextIconEntry *node;
    struct TextIconEntry *retval = 0;
    ULONG i = 0;
    
    if (index >= 0 && index < data->num_entries)
    {
	ForeachNode(&data->entries_list, node)
	{
    	    if (i == index)
	    {
		retval = node;
		break;
	    }
	    i++;
	}
    }
    
    return retval;    
}

static BOOL MustRenderRect(struct TextIconList_DATA *data, struct Rectangle *rect)
{
    if (data->update_rect1 && data->update_rect2)
    {
    	if (!AndRectRect(rect, data->update_rect1, NULL) &&
	    !AndRectRect(rect, data->update_rect2, NULL)) return FALSE;	    
    }
    else if (data->update_rect1)
    {
    	if (!AndRectRect(rect, data->update_rect1, NULL)) return FALSE;
    }
    else if (data->update_rect2)
    {
    	if (!AndRectRect(rect, data->update_rect2, NULL)) return FALSE;
    }
    
    return TRUE;
}

static void RenderHeaderField(Object *obj, struct TextIconList_DATA *data,
    	    	    	    struct Rectangle *rect, LONG index)
{
    STRPTR text;
    struct TextExtent te;
    ULONG fit;
    
    text = GetTextIconHeaderText(data, index);

    SetAPen(_rp(obj), _pens(obj)[MPEN_HALFSHINE]);
    RectFill(_rp(obj), rect->MinX + 1, rect->MinY + 1,
    	    	       rect->MaxX - 1, rect->MaxY - 1);
    SetAPen(_rp(obj), _pens(obj)[MPEN_SHINE]);
    RectFill(_rp(obj), rect->MinX, rect->MinY, rect->MinX, rect->MaxY);
    RectFill(_rp(obj), rect->MinX + 1, rect->MinY, rect->MaxX - 1, rect->MinY);
    SetAPen(_rp(obj), _pens(obj)[MPEN_HALFSHADOW]);
    RectFill(_rp(obj), rect->MaxX, rect->MinY, rect->MaxX, rect->MaxY);
    RectFill(_rp(obj), rect->MinX + 1, rect->MaxY, rect->MaxX - 1, rect->MaxY);
    		       
    rect->MinX += HEADERENTRY_SPACING_LEFT;
    rect->MinY += HEADERLINE_SPACING_TOP;
    rect->MaxX -= HEADERENTRY_SPACING_RIGHT;
    rect->MaxY -= HEADERLINE_SPACING_BOTTOM;
    		  
    if (text && text[0])
    {		       
	fit = TextFit(_rp(obj), text, strlen(text), &te, NULL, 1,
    	    	       rect->MaxX - rect->MinX + 1,
		       rect->MaxY - rect->MinY + 1);

	if (!fit) return;

	SetABPenDrMd(_rp(obj), _pens(obj)[MPEN_TEXT], _pens(obj)[MPEN_HALFSHINE], JAM2);
	Move(_rp(obj), rect->MinX, rect->MinY + _rp(obj)->TxBaseline);
	Text(_rp(obj), text, fit);
    }
}


static void RenderHeaderline(Object *obj, struct TextIconList_DATA *data)
{
    struct Rectangle linerect;
    LONG    	     x, i;
    LONG    	     firstvis, lastvis;
    linerect = data->header_rect;
    linerect.MinX -= data->view_x;
    linerect.MaxX -= data->view_x;

    linerect.MinX = data->header_rect.MinX - data->view_x;
    linerect.MaxX = linerect.MinX + data->width - 1;
    linerect.MinY = data->header_rect.MinY;
    linerect.MaxY = data->header_rect.MaxY;
    
    if (!MustRenderRect(data, &linerect)) return;
    
    SetFont(_rp(obj), _font(obj));
    
    x = linerect.MinX + HEADERLINE_SPACING_LEFT;
    
    firstvis = FirstVisibleColumnNumber(data);
    lastvis = LastVisibleColumnNumber(data);
    
    for(i = 0; i < NUM_COLUMNS; i++)
    {
	struct Rectangle field_rect;
    	LONG 	    	 index = data->column_pos[i];
	
	if (!data->column_visible[index]) continue;
	
	field_rect.MinX = (i == firstvis) ? linerect.MinX : x;
	field_rect.MinY = linerect.MinY;
    	field_rect.MaxX = x + data->column_width[index] - 1 + ((i == lastvis) ? HEADERLINE_SPACING_RIGHT : 0);
	field_rect.MaxY = linerect.MaxY;
	
	if (MustRenderRect(data, &field_rect))
	{
	    RenderHeaderField(obj, data, &field_rect, index);
	}
	x += data->column_width[index];
    }
        
}

#define ENTRYPOS_FIRST -1
#define ENTRYPOS_MIDDLE 0
#define ENTRYPOS_LAST   1

static void RenderEntryField(Object *obj, struct TextIconList_DATA *data,
    	    	    	     struct TextIconEntry *entry, struct Rectangle *rect,
			     LONG index, BOOL firstvis, BOOL lastvis)
{
    STRPTR text;
    struct TextExtent te;
    ULONG fit;
    BOOL selected;
    
    selected = (entry && data->column_clickable[index]) ? entry->selected : FALSE;

    SetABPenDrMd(_rp(obj), _pens(obj)[selected ? MPEN_FILL : MPEN_SHINE], 0, JAM1);
    RectFill(_rp(obj), rect->MinX, rect->MinY, rect->MaxX, rect->MaxY);
    
    rect->MinX += ENTRY_SPACING_LEFT;
    rect->MaxX -= ENTRY_SPACING_RIGHT;
    rect->MinY += LINE_SPACING_TOP;
    rect->MaxY -= LINE_SPACING_BOTTOM;
    
    if (firstvis) rect->MinX += LINE_SPACING_LEFT;
    if (lastvis)  rect->MaxX -= LINE_SPACING_RIGHT;
    
    if (!entry) return;
    
    text = GetTextIconEntryText(data, entry, index);
    if (!text) return;
    if (!text[0]) return;
    
    fit = TextFit(_rp(obj), text, strlen(text), &te, NULL, 1,
    	    	   rect->MaxX - rect->MinX + 1,
		   rect->MaxY - rect->MinY + 1);
    
    if (!fit) return;
    
    SetABPenDrMd(_rp(obj), _pens(obj)[selected ? MPEN_SHINE : MPEN_TEXT], 0, JAM1);
    
    switch(data->column_align[index])
    {
    	case COLUMN_ALIGN_LEFT:
    	    Move(_rp(obj), rect->MinX, rect->MinY + _rp(obj)->TxBaseline);
	    break;
	    
	case COLUMN_ALIGN_RIGHT:
    	    Move(_rp(obj), rect->MaxX - te.te_Width, rect->MinY + _rp(obj)->TxBaseline);
	    break;
	    
	case COLUMN_ALIGN_CENTER:
    	    Move(_rp(obj), rect->MinX + (rect->MaxX - rect->MinX + 1 + 1 - te.te_Width) / 2,
	    	    	   rect->MinY + _rp(obj)->TxBaseline);
	    break;
	    
    }
    Text(_rp(obj), text, fit);
}

static void RenderEntry(Object *obj, struct TextIconList_DATA *data, LONG index)
{
    struct TextIconEntry *entry = GetEntryFromIndex(data, index);
    struct Rectangle linerect;
    LONG    	     x, i;
    LONG    	     firstvis, lastvis;
    
    linerect.MinX = data->view_rect.MinX - data->view_x;
    linerect.MaxX = linerect.MinX + data->width - 1;
    linerect.MinY = data->view_rect.MinY + index * data->lineheight - data->view_y;
    linerect.MaxY = linerect.MinY + data->lineheight - 1;
    
    if (!AndRectRect(&linerect, &data->view_rect, NULL)) return;
    if (!MustRenderRect(data, &linerect)) return;
    
    SetFont(_rp(obj), _font(obj));
    
    x = linerect.MinX + LINE_SPACING_LEFT;

    firstvis = FirstVisibleColumnNumber(data);
    lastvis = LastVisibleColumnNumber(data);
    
    for(i = 0; i < NUM_COLUMNS; i++)
    {
	struct Rectangle field_rect;
    	LONG 	    	 index = data->column_pos[i];
	
	if (!data->column_visible[i]) continue;
	
	field_rect.MinX = (i == firstvis) ? linerect.MinX : x;
	field_rect.MinY = linerect.MinY;
    	field_rect.MaxX = x + data->column_width[index] - 1 + ((i == lastvis) ? LINE_SPACING_RIGHT : 0);
	field_rect.MaxY = linerect.MaxY;
	
	if (MustRenderRect(data, &field_rect))
	{
	    if (AndRectRect(&field_rect, &data->view_rect, NULL))
	    {
		RenderEntryField(obj, data, entry, &field_rect, index,
		    	    	 (i == firstvis), (i == lastvis));
	    }
	}
	x += data->column_width[index];
    }
        
}

static LONG FirstVisibleLine(struct TextIconList_DATA *data)
{
    return data->view_y / data->lineheight;
}

static LONG NumVisibleLines(struct TextIconList_DATA *data)
{
    LONG visible = data->view_height + data->lineheight - 1 +
    	    	   (data->view_y % data->lineheight);
		   
    visible /= data->lineheight;
    
    return visible;		   
}

static void RenderAllEntries(Object *obj, struct TextIconList_DATA *data)
{
    LONG first   = FirstVisibleLine(data);
    LONG visible = NumVisibleLines(data);
    LONG i;
    
    for(i = 0; i < visible; i++)
    {
    	RenderEntry(obj, data, first + i);
    }
    
}

/**************************************************************************
 OM_NEW
**************************************************************************/
static IPTR TextIconList_New(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct TextIconList_DATA    *data;
    struct TagItem  	    	*tag, *tags;
    LONG    	    	    	 i;
    
    obj = (Object *)DoSuperNewTags(cl, obj, NULL,
	MUIA_FillArea, FALSE,
   	TAG_MORE, (IPTR) msg->ops_AttrList);
    if (!obj) return FALSE;

    data = INST_DATA(cl, obj);
    NewList((struct List*)&data->entries_list);
    data->show_header = TRUE;
    data->active_entry = -1;
    
    for(i = 0; i < NUM_COLUMNS; i++)
    {
    	data->column_pos[i] = i;
	data->column_visible[i] = TRUE;
	data->column_width[i] = 100;
    }
    data->column_align[INDEX_SIZE] = COLUMN_ALIGN_RIGHT;
    data->column_clickable[INDEX_NAME] = TRUE;
    
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
	return 0;
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
static IPTR TextIconList_Dispose(struct IClass *cl, Object *obj, Msg msg)
{
    struct TextIconList_DATA *data = INST_DATA(cl, obj);
    struct TextIconEntry *node;

    ForeachNode(&data->entries_list, node)
    {
    }

    if (data->pool) DeletePool(data->pool);

    DoSuperMethodA(cl,obj,msg);
    return 0;
}

/**************************************************************************
 OM_SET
**************************************************************************/
static IPTR TextIconList_Set(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct TextIconList_DATA *data = INST_DATA(cl, obj);
    struct TagItem  	     *tag, *tags;
    LONG    	    	      oldleft = data->view_x, oldtop = data->view_y;
    
    /* parse initial taglist */
    for (tags = msg->ops_AttrList; (tag = NextTagItem(&tags)); )
    {
	switch (tag->ti_Tag)
	{
	    case    MUIA_TextIconList_Left:
		    if (data->view_x != (LONG)tag->ti_Data)
		    {
		    	LONG new_view_x = (LONG)tag->ti_Data;
			
			if (new_view_x + data->view_width > data->width)
			{
			    new_view_x = data->width - data->view_width;
			}
			if (new_view_x < 0) new_view_x = 0;
			
			data->view_x = new_view_x;
			tag->ti_Data = new_view_x;
		    }
		    break;

	    case    MUIA_TextIconList_Top:
		    if (data->view_y != (LONG)tag->ti_Data)
		    {
		    	LONG new_view_y = (LONG)tag->ti_Data;
			
			if (new_view_y + data->view_height > data->height)
			{
			    new_view_y = data->height - data->view_height;
			}
			if (new_view_y < 0) new_view_y = 0;
			
			data->view_y = new_view_y;
			tag->ti_Data = new_view_y;		
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
static IPTR TextIconList_Get(struct IClass *cl, Object *obj, struct opGet *msg)
{
/* small macro to simplify return value storage */
#define STORE *(msg->opg_Storage)
    struct TextIconList_DATA *data = INST_DATA(cl, obj);

    switch (msg->opg_AttrID)
    {
	case MUIA_TextIconList_Left: STORE = data->view_x; return 1;
	case MUIA_TextIconList_Top: STORE = data->view_y; return 1;
	case MUIA_TextIconList_Width: STORE = data->width; return 1;
	case MUIA_TextIconList_Height: STORE = data->height; return 1;
    }

    return DoSuperMethodA(cl, obj, (Msg) msg);
#undef STORE
}

/**************************************************************************
 MUIM_Setup
**************************************************************************/
static IPTR TextIconList_Setup(struct IClass *cl, Object *obj, struct MUIP_Setup *msg)
{
    struct TextIconList_DATA *data = INST_DATA(cl, obj);
 
    if (!DoSuperMethodA(cl, obj, (Msg) msg)) return 0;

    DoMethod(_win(obj),MUIM_Window_AddEventHandler, (IPTR)&data->ehn);

    InitRastPort(&data->temprp);
    SetFont(&data->temprp, _font(obj));
    
    data->lineheight = LINE_EXTRAHEIGHT + _font(obj)->tf_YSize;
    data->is_setup = TRUE;
    
    if (data->show_header)
    {
    	data->headerheight = HEADERLINE_EXTRAHEIGHT + _font(obj)->tf_YSize;
    }
    else
    {
    	data->headerheight = 0;
    }
    
    if (data->num_entries)
    {
    	CalcAllEntryDimensions(data);
	CalcWidth(data);
	data->height = data->num_entries * data->lineheight;
	RecalcColumnMaxWidths(data);
	
	SetAttrs(obj, MUIA_TextIconList_Width, data->width,
	    	      MUIA_TextIconList_Height, data->height,
		      TAG_DONE);
    }
    
    return 1;
}

/**************************************************************************
 MUIM_Show
**************************************************************************/
static IPTR TextIconList_Show(struct IClass *cl, Object *obj, struct MUIP_Show *msg)
{
    struct TextIconList_DATA *data = INST_DATA(cl, obj);
    LONG newleft, newtop;
    IPTR rc;
    
    rc = DoSuperMethodA(cl, obj, (Msg)msg);

    newleft = data->view_x;
    newtop = data->view_y;

    data->view_width = _mwidth(obj);
    data->view_height = _mheight(obj) - data->headerheight;

    SetAttrs(obj, MUIA_TextIconList_VisWidth, data->view_width,
    	    	  MUIA_TextIconList_VisHeight, data->view_height,
		  TAG_DONE);
    
    data->view_rect.MinX = _mleft(obj);
    data->view_rect.MinY = _mtop(obj) + data->headerheight;
    data->view_rect.MaxX = _mright(obj);
    data->view_rect.MaxY = _mbottom(obj);
    
    data->header_rect.MinX = _mleft(obj);
    data->header_rect.MinY = _mtop(obj);
    data->header_rect.MaxX = _mright(obj);
    data->header_rect.MaxY = _mtop(obj) + data->headerheight - 1;
    
    if (newleft + data->view_width > data->width) newleft = data->width - data->view_width;
    if (newleft < 0) newleft = 0;
    
    if (newtop + data->view_height > data->height) newtop = data->height -  data->view_height;
    if (newtop < 0) newtop = 0;

    if ((newleft != data->view_x) || (newtop != data->view_y))
    {    
	SetAttrs(obj, MUIA_TextIconList_Left, newleft,
    	    	      MUIA_TextIconList_Top, newtop,
		      TAG_DONE);
    }
    
    SetFont(_rp(obj), _font(obj));
    
    data->is_shown = TRUE;
    
    return rc;
}

/**************************************************************************
 MUIM_Hide
**************************************************************************/
static IPTR TextIconList_Hide(struct IClass *cl, Object *obj, struct MUIP_Hide *msg)
{
    struct TextIconList_DATA *data = INST_DATA(cl, obj);
    IPTR rc;
    
    data->is_shown = FALSE;
    
    rc = DoSuperMethodA(cl, obj, (Msg)msg);

    return rc;
}

/**************************************************************************
 MUIM_Cleanup
**************************************************************************/
static IPTR TextIconList_Cleanup(struct IClass *cl, Object *obj, struct MUIP_Cleanup *msg)
{
    struct TextIconList_DATA *data = INST_DATA(cl, obj);

    DoMethod(_win(obj),MUIM_Window_RemEventHandler, (IPTR)&data->ehn);

    DeinitRastPort(&data->temprp);
        
    data->is_setup = FALSE;
    
    return DoSuperMethodA(cl, obj, (Msg) msg);
}

/**************************************************************************
 MUIM_AskMinMax
**************************************************************************/
static IPTR TextIconList_AskMinMax(struct IClass *cl, Object *obj, struct MUIP_AskMinMax *msg)
{
    ULONG rc = DoSuperMethodA(cl, obj, (Msg) msg);

    msg->MinMaxInfo->MinWidth  += 10;
    msg->MinMaxInfo->MinHeight += 10;

    msg->MinMaxInfo->DefWidth  += 100;
    msg->MinMaxInfo->DefHeight += 100;

    msg->MinMaxInfo->MaxWidth = MUI_MAXMAX;
    msg->MinMaxInfo->MaxHeight = MUI_MAXMAX;

    return rc;
}

/**************************************************************************
 MUIM_Draw
**************************************************************************/
static IPTR TextIconList_Draw(struct IClass *cl, Object *obj, struct MUIP_Draw *msg)
{
    struct TextIconList_DATA *data = INST_DATA(cl, obj);
    APTR    	    	      clip;
    struct TextIconEntry     *entry;

    DoSuperMethodA(cl, obj, (Msg) msg);

    if (msg->flags & MADF_DRAWUPDATE)
    {
	if (data->update == UPDATE_SCROLL)
	{
	    struct Region   	*region;
	    struct Rectangle 	 xrect, yrect;
	    BOOL    	    	 scroll_caused_damage;

	    scroll_caused_damage = (_rp(obj)->Layer->Flags & LAYERREFRESH) ? FALSE : TRUE;

	    data->update = 0;

	    if ((abs(data->update_scrolldx) >= data->view_width) ||
	    	(abs(data->update_scrolldy) >= data->view_height))
	    {
	    	data->update = UPDATE_ALL;
		MUI_Redraw(obj, MADF_DRAWUPDATE);
		return 0;
	    }

	    region = NewRegion();
	    if (!region)
	    {
	    	data->update = UPDATE_ALL;
		MUI_Redraw(obj, MADF_DRAWUPDATE);
		return 0;
	    }

	    if (data->update_scrolldx > 0)
	    {
		xrect.MinX = data->view_rect.MaxX - data->update_scrolldx;
		xrect.MinY = data->view_rect.MinY;
		xrect.MaxX = data->view_rect.MaxX;
		xrect.MaxY = data->view_rect.MaxY;

		OrRectRegion(region, &xrect);
		
		data->update_rect1 = &xrect;
	    }
	    else if (data->update_scrolldx < 0)
	    {
		xrect.MinX = data->view_rect.MinX;
		xrect.MinY = data->view_rect.MinY;
		xrect.MaxX = data->view_rect.MinX - data->update_scrolldx;
		xrect.MaxY = data->view_rect.MaxY;

		OrRectRegion(region, &xrect);

		data->update_rect1 = &xrect;
	    }

	    if (data->update_scrolldy > 0)
	    {
		yrect.MinX = data->view_rect.MinX;
		yrect.MinY = data->view_rect.MaxY - data->update_scrolldy;
		yrect.MaxX = data->view_rect.MaxX;
		yrect.MaxY = data->view_rect.MaxY;

		OrRectRegion(region, &yrect);

		data->update_rect2 = &yrect;
	    }
	    else if (data->update_scrolldy < 0)
	    {
		yrect.MinX = data->view_rect.MinX;
		yrect.MinY = data->view_rect.MinY;
		yrect.MaxX = data->view_rect.MaxX;
		yrect.MaxY = data->view_rect.MinY - data->update_scrolldy;

		OrRectRegion(region, &yrect);

		data->update_rect2 = &yrect;
	    }

	    ScrollRasterBF(_rp(obj),
	    	    	   data->update_scrolldx,
			   data->update_scrolldy,
			   data->view_rect.MinX,
			   data->view_rect.MinY,
			   data->view_rect.MaxX,
			   data->view_rect.MaxY);

    	    if (data->show_header && data->update_scrolldx)
	    {
		ScrollRasterBF(_rp(obj),
	    	    	       data->update_scrolldx,
			       0,
			       data->header_rect.MinX,
			       data->header_rect.MinY,
			       data->header_rect.MaxX,
			       data->header_rect.MaxY);	    	
	    }
	    
    	    scroll_caused_damage = scroll_caused_damage && (_rp(obj)->Layer->Flags & LAYERREFRESH) ? TRUE : FALSE;

	    clip = MUI_AddClipRegion(muiRenderInfo(obj), region);
	    data->update = UPDATE_ALL;
	    MUI_Redraw(obj, MADF_DRAWUPDATE);
	    MUI_RemoveClipRegion(muiRenderInfo(obj), clip);

    	    if (data->show_header && data->update_scrolldx)
	    {
	    	xrect.MinY = data->header_rect.MinY;
		xrect.MaxY = data->header_rect.MaxY;
		
		data->update_rect1 = &xrect;
		data->update_rect2 = NULL;
		
		clip = MUI_AddClipping(muiRenderInfo(obj), xrect.MinX,
		    	    	    	    	    	   xrect.MinY, 
							   xrect.MaxX - xrect.MinX + 1,
							   xrect.MaxY - xrect.MinY + 1);
		
		data->update = UPDATE_ALL;
		MUI_Redraw(obj, MADF_DRAWUPDATE);
		MUI_RemoveClipRegion(muiRenderInfo(obj), clip);

	    	data->update_rect2 = NULL;
		
    	    }
	    
	    data->update_rect1 = data->update_rect2 = NULL;
	    data->update_scrolldx = data->update_scrolldy = 0;

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
	    
	} /* if (data->update == UPDATE_SCROLL) */
	else if (data->update == UPDATE_DIRTY_ENTRIES)
	{
	    LONG first, numvisible, index = 0;

	    data->update = 0;
	    
	    clip = MUI_AddClipping(muiRenderInfo(obj), data->view_rect.MinX,
    	    	    	    	    		       data->view_rect.MinY,
						       data->view_width,
						       data->view_height);

	    first = FirstVisibleLine(data);
	    numvisible = NumVisibleLines(data);

	    ForeachNode(&data->entries_list, entry)
	    {
	    	if (entry->dirty)
		{
		    entry->dirty = FALSE;
		    
		    if ((index >= first) && (index < first + numvisible))
		    {
		    	RenderEntry(obj, data, index);
		    }
		}
		index++;
	    }
	    
	    MUI_RemoveClipping(muiRenderInfo(obj),clip);
	    
	    return 0;
	    
	} /* else if (data->update == UPDATE_DIRTY_ENTRIES) */
	
    } /* if (msg->flags & MADF_DRAWUPDATE) */

    if (MustRenderRect(data, &data->view_rect))
    {
	clip = MUI_AddClipping(muiRenderInfo(obj), data->view_rect.MinX,
    	    	    	    	    		   data->view_rect.MinY,
						   data->view_width,
						   data->view_height);

	RenderAllEntries(obj, data);

	MUI_RemoveClipping(muiRenderInfo(obj),clip);
    }
    
    if (data->show_header && MustRenderRect(data, &data->header_rect))
    {
	clip = MUI_AddClipping(muiRenderInfo(obj), data->header_rect.MinX,
    	    	    	    	    		   data->header_rect.MinY,
						   data->header_rect.MaxX - data->header_rect.MinX + 1,
						   data->header_rect.MaxY - data->header_rect.MinY + 1);

	RenderHeaderline(obj, data);

	MUI_RemoveClipping(muiRenderInfo(obj),clip);
    }
    

    data->update = 0;

    return 0;
}

static LONG LineUnderMouse(struct TextIconList_DATA *data, LONG mx, LONG my)
{
    LONG index = -1;
    
    if ((mx >= data->view_rect.MinX) &&
	(my >= data->view_rect.MinY) &&
    	(mx <= data->view_rect.MaxX) &&
	(my <= data->view_rect.MaxY))
    {
    	index = (my - data->view_rect.MinY + data->view_y) / data->lineheight;
	
	if ((index < 0) || (index >= data->num_entries)) index = -1;
    }
    
    return index;
}

static LONG ColumnUnderMouse(struct TextIconList_DATA *data, LONG mx, LONG my)
{
    LONG col = -1;

    if ((mx >= data->view_rect.MinX) &&
	(my >= data->view_rect.MinY - data->headerheight) &&
    	(mx <= data->view_rect.MaxX) &&
	(my <= data->view_rect.MaxY))
    {
    	LONG x = data->view_rect.MinX - data->view_x + LINE_SPACING_LEFT;
	LONG w, i;
	
    	for(i = 0; i < NUM_COLUMNS; i++)
	{
	    LONG index = data->column_pos[i];
	    
	    if (!data->column_visible[index]) continue;
	    
	    w = data->column_width[index];
	    
	    if ((mx >= x) && (mx < x + w))
	    {
	    	col = index;
		break;
	    }
	    x += w;
	}
	
    }
    
    return col;    
}

static LONG ColumnResizeHandleUnderMouse(struct TextIconList_DATA *data, LONG mx, LONG my)
{
    LONG col = -1;

    if ((mx >= data->view_rect.MinX) &&
	(my >= data->view_rect.MinY - data->headerheight) &&
    	(mx <= data->view_rect.MaxX) &&
	(my <= data->view_rect.MaxY))
    {
    	LONG x = data->view_rect.MinX - data->view_x + LINE_SPACING_LEFT;
	LONG w, i;
	
    	for(i = 0; i < NUM_COLUMNS; i++)
	{
	    LONG index = data->column_pos[i];
	    
	    if (!data->column_visible[index]) continue;
	    
	    w = data->column_width[index];
	    
	    if (abs(mx - (x + w - 1)) <= 4)
	    {
	    	col = index;
		break;
	    }
	    x += w;
	}
	
    }
    
    return col;    
}

static BOOL GetColumnCoords(struct TextIconList_DATA *data, LONG index, LONG *x1, LONG *x2)
{
    LONG i;
    BOOL retval = FALSE;
    LONG x = data->view_rect.MinX - data->view_x + LINE_SPACING_LEFT;
    
    for(i = 0; i < NUM_COLUMNS; i++)
    {
    	LONG idx = data->column_pos[i];
	LONG w;
	
    	if (!data->column_visible[idx]) continue;
	
	w = data->column_width[idx];
	
	if (idx == index)
	{
	    retval = TRUE;
	    *x1 = x - ((i == 0) ? LINE_SPACING_LEFT : 0);
	    *x2 = x + w - ((i == NUM_COLUMNS - 1) ? 0 : 1);
	    break;	    
	}
	
	x += w;
    }
    
    return retval;
}


/**************************************************************************
 MUIM_HandleEvent
**************************************************************************/
static IPTR TextIconList_HandleEvent(struct IClass *cl, Object *obj, struct MUIP_HandleEvent *msg)
{
    struct TextIconList_DATA *data = INST_DATA(cl, obj);
    IPTR retval = 0;
    
    if (msg->imsg)
    {
    	LONG mx = msg->imsg->MouseX;
    	LONG my = msg->imsg->MouseY;
	LONG line, col;
	BOOL shift_qual;
	
	shift_qual = (msg->imsg->Qualifier & (IEQUALIFIER_LSHIFT | IEQUALIFIER_RSHIFT)) ? TRUE : FALSE;
	
	switch (msg->imsg->Class)
	{
	    case IDCMP_MOUSEBUTTONS:
	    	switch(msg->imsg->Code)
		{
		    case SELECTDOWN:
		    	//kprintf("SELECTDOWN\n");
			if (data->inputstate == INPUTSTATE_NONE)
			{
		    	    if ( ((line = LineUnderMouse(data, mx, my)) >= 0) &&
				 ((col = ColumnUnderMouse(data, mx, my)) >= 0) )
			    {
				//kprintf("click on line %d col %d\n", line, col);

				if (data->column_clickable[col])
				{
			    	    if (data->active_entry != line)
				    {
					struct TextIconEntry *old, *new;

					old = GetEntryFromIndex(data, data->active_entry);
					new = GetEntryFromIndex(data, line);

					data->active_entry = line;
					if (old && old->selected && !shift_qual)
					{
				    	    old->selected = FALSE;
					    old->dirty = TRUE;

					    data->num_selected--;
					}

					if (!shift_qual && data->num_selected)
					{
				    	    struct TextIconEntry *entry;

					    ForeachNode(&data->entries_list, entry)
					    {
						if (entry->selected)
						{
					    	    entry->selected = FALSE;
						    entry->dirty = TRUE;
						}
					    }

					    data->num_selected = 0;
					}

					if (new && !new->selected)
					{
				    	    new->selected = TRUE;
					    new->dirty = TRUE;

					    data->num_selected++;
					}

					data->update = UPDATE_DIRTY_ENTRIES;
					MUI_Redraw(obj, MADF_DRAWUPDATE);
					
				    } /* if (data->active_entry != line) */
				    
				} /* if (data->column_clickable[col]) */
				
			    } /* if click on entry */
			    else if ((col = ColumnResizeHandleUnderMouse(data, mx, my)) >= 0)
			    {
				data->inputstate = INPUTSTATE_COL_RESIZE;
				data->click_column = col;
				data->click_x = mx - data->column_width[col];

    	    	    		if (!(data->ehn.ehn_Events & IDCMP_MOUSEMOVE))
				{
			    	    DoMethod(_win(obj),MUIM_Window_RemEventHandler, (IPTR)&data->ehn);
			    	    data->ehn.ehn_Events |= IDCMP_MOUSEMOVE;
			    	    DoMethod(_win(obj),MUIM_Window_AddEventHandler, (IPTR)&data->ehn);			    
				}
				
			    } /* else if click on column header entry */
			    
			} /* if (data->inputstate == INPUTSTATE_NONE) */
		    	break;
			
		    case SELECTUP:
		    	if (data->inputstate == INPUTSTATE_COL_RESIZE)
			{
		    	    if (data->ehn.ehn_Events & IDCMP_MOUSEMOVE)
			    {
				DoMethod(_win(obj),MUIM_Window_RemEventHandler, (IPTR)&data->ehn);
				data->ehn.ehn_Events &= ~IDCMP_MOUSEMOVE;
				DoMethod(_win(obj),MUIM_Window_AddEventHandler, (IPTR)&data->ehn);			    
			    }
			    
			    data->inputstate = INPUTSTATE_NONE;
			}
			break;
		    	
		    case MIDDLEDOWN:
		    	if (data->inputstate == INPUTSTATE_NONE)
			{
			    data->inputstate = INPUTSTATE_PAN;
			    
			    data->click_x = mx - data->view_rect.MinX + data->view_x;
			    data->click_y = my - data->view_rect.MinY + data->view_y;

    	    	    	    if (!(data->ehn.ehn_Events & IDCMP_MOUSEMOVE))
			    {
			    	DoMethod(_win(obj),MUIM_Window_RemEventHandler, (IPTR)&data->ehn);
			    	data->ehn.ehn_Events |= IDCMP_MOUSEMOVE;
			    	DoMethod(_win(obj),MUIM_Window_AddEventHandler, (IPTR)&data->ehn);			    
			    }

			}
		    	break;
			
		    case MIDDLEUP:
		    	if (data->inputstate == INPUTSTATE_PAN)
			{
		    	    if (data->ehn.ehn_Events & IDCMP_MOUSEMOVE)
			    {
				DoMethod(_win(obj),MUIM_Window_RemEventHandler, (IPTR)&data->ehn);
				data->ehn.ehn_Events &= ~IDCMP_MOUSEMOVE;
				DoMethod(_win(obj),MUIM_Window_AddEventHandler, (IPTR)&data->ehn);			    
			    }
			    
			    data->inputstate = INPUTSTATE_NONE;
			}
			break;
						
		} /* switch(msg->imsg->Code) */
	    	break;
		
	    case IDCMP_MOUSEMOVE:
	    	if (data->inputstate == INPUTSTATE_PAN)
		{
		    LONG new_view_x, new_view_y;
		    
		    new_view_x = data->click_x - (mx - data->view_rect.MinX);
		    new_view_y  = data->click_y - (my - data->view_rect.MinY);
		    
		    if (new_view_x + data->view_width > data->width)
		    {
			    new_view_x = data->width - data->view_width;
		    }
		    if (new_view_x < 0) new_view_x = 0;
			
		    if (new_view_y + data->view_height > data->height)
		    {
			    new_view_y = data->height - data->view_height;
		    }
		    if (new_view_y < 0) new_view_y = 0;

		    if ((new_view_x != data->view_x) || (new_view_y != data->view_y))
		    {
		    	SetAttrs(obj, MUIA_TextIconList_Left, new_view_x,
			    	      MUIA_TextIconList_Top, new_view_y,
				      TAG_DONE);
		    }
		} /* if (data->inputstate == INPUTSTATE_PAN) */
		else if (data->inputstate == INPUTSTATE_COL_RESIZE)
		{
		    LONG act_colwidth = data->column_width[data->click_column];
		    LONG new_colwidth = mx - data->click_x;
		    
		    if (new_colwidth < MIN_COLUMN_WIDTH) new_colwidth = MIN_COLUMN_WIDTH;
		    
		    if (new_colwidth > act_colwidth)
		    {
		    	data->column_width[data->click_column] = new_colwidth;
			data->width += new_colwidth - act_colwidth;
			data->update = UPDATE_ALL;
			MUI_Redraw(obj, MADF_DRAWUPDATE);
			
			set(obj, MUIA_TextIconList_Width, data->width);
		    }
		    else if (new_colwidth < act_colwidth)
		    {
		    	BOOL scroll_left = FALSE;
			
		    	data->column_width[data->click_column] = new_colwidth;
			data->width += new_colwidth - act_colwidth;

    	    	    	if (data->view_x + data->view_width > data->width)
			{
			    LONG new_view_x = data->width - data->view_width;
			   
			    if (new_view_x < 0) new_view_x = 0;
			    if (new_view_x != data->view_x)
			    {
			    	scroll_left = TRUE;
				data->view_x = new_view_x;
			    }
			}
			
			data->update = UPDATE_ALL;
			MUI_Redraw(obj, MADF_DRAWUPDATE);
    	    	    			
			SetAttrs(obj, scroll_left ? MUIA_TextIconList_Left : TAG_IGNORE, data->view_x,
			    	      MUIA_TextIconList_Width, data->width,
				      TAG_DONE);
				      
		    } /* else if (new_colwidth < act_colwidth) */
		    
		} /* else if (data->inputstate == INPUTSTATE_COL_RESIZE) */
		break;
		
    	} /* switch (msg->imsg->Class) */
	
    } /* if (msg->imsg) */
    
    return retval;
    
}

/**************************************************************************
 MUIM_TextIconList_Clear
**************************************************************************/
static IPTR TextIconList_Clear(struct IClass *cl, Object *obj, struct MUIP_TextIconList_Clear *msg)
{
    struct TextIconList_DATA *data = INST_DATA(cl, obj);
    struct TextIconEntry *node;
    LONG    	    	  i;
    
    while ((node = (struct TextIconEntry *)RemTail((struct List *)&data->entries_list)))
    {
        FreePooled(data->pool,node,sizeof(*node));
    }

    data->view_x = data->view_y = data->width = data->height = 0;
    data->num_entries = 0;
    data->active_entry = -1;
    data->num_selected = 0;
    
    for(i = 0; i < NUM_COLUMNS; i++)
    {
    	data->column_maxwidth[i] = 0;
    }
    
    SetAttrs(obj, MUIA_TextIconList_Left, data->view_x,
    	    	  MUIA_TextIconList_Top, data->view_y,
		  MUIA_TextIconList_Width, data->width,
		  MUIA_TextIconList_Height, data->height,
		  TAG_DONE);

    data->update = UPDATE_ALL;		  
    MUI_Redraw(obj,MADF_DRAWUPDATE);
    return 1;
}

/**************************************************************************
 MUIM_TextIconList_Add.
 Returns 0 on failure otherwise 1
**************************************************************************/
static IPTR TextIconList_Add(struct IClass *cl, Object *obj, struct MUIP_TextIconList_Add *msg)
{
    struct TextIconList_DATA 	*data = INST_DATA(cl, obj);
    struct TextIconEntry    	*entry;
    struct DateTime 	    	 dt;
    UBYTE   	    	    	*sp;
    
    if (!(entry = AllocPooled(data->pool,sizeof(struct TextIconEntry))))
    {
	return 0;
    }

    memset(entry, 0, sizeof(struct TextIconEntry));

    entry->fib = *msg->fib;
    
    if (entry->fib.fib_DirEntryType > 0)
    {
    	strcpy(GetTextIconEntryText(data, entry, INDEX_SIZE), "Drawer");
    }
    else
    {
    	sprintf(GetTextIconEntryText(data, entry, INDEX_SIZE), "%ld", entry->fib.fib_Size);
    }
    
    dt.dat_Stamp    = entry->fib.fib_Date;
    dt.dat_Format   = FORMAT_DOS;
    dt.dat_Flags    = 0;
    dt.dat_StrDay   = NULL;
    dt.dat_StrDate  = GetTextIconEntryText(data, entry, INDEX_DATE);
    dt.dat_StrTime  = GetTextIconEntryText(data, entry, INDEX_TIME);;
    
    DateToStr(&dt);
    
    sp = GetTextIconEntryText(data, entry, INDEX_PROTECTION);
    *sp++ = (entry->fib.fib_Protection & FIBF_SCRIPT)  ? 's' : '-';
    *sp++ = (entry->fib.fib_Protection & FIBF_PURE)    ? 'p' : '-';
    *sp++ = (entry->fib.fib_Protection & FIBF_ARCHIVE) ? 'a' : '-';
    *sp++ = (entry->fib.fib_Protection & FIBF_READ)    ? '-' : 'r';
    *sp++ = (entry->fib.fib_Protection & FIBF_WRITE)   ? '-' : 'w';
    *sp++ = (entry->fib.fib_Protection & FIBF_EXECUTE) ? '-' : 'e';
    *sp++ = (entry->fib.fib_Protection & FIBF_DELETE)  ? '-' : 'd';
    *sp++ = '\0';
    
    data->num_entries++;
    
    AddTail((struct List *)&data->entries_list, (struct Node *)entry);
    
    if (data->is_setup)
    {
   	CalcEntryDimension(data, entry);
	data->height += data->lineheight;
	
	CalcWidth(data);
		
	SetAttrs(obj, MUIA_TextIconList_Width, data->width,
	    	      MUIA_TextIconList_Height, data->height,
		      TAG_DONE);
    }
    
    return 1;
}

BOOPSI_DISPATCHER(IPTR,TextIconList_Dispatcher, cl, obj, msg)
{
    switch (msg->MethodID)
    {

	case OM_NEW: return TextIconList_New(cl, obj, (struct opSet *)msg);
	case OM_DISPOSE: return TextIconList_Dispose(cl,obj, msg);
	case OM_SET: return TextIconList_Set(cl,obj,(struct opSet *)msg);
	case OM_GET: return TextIconList_Get(cl,obj,(struct opGet *)msg);
	case MUIM_Setup: return TextIconList_Setup(cl,obj,(struct MUIP_Setup *)msg);
	case MUIM_Show: return TextIconList_Show(cl,obj,(struct MUIP_Show *)msg);
	case MUIM_Hide: return TextIconList_Hide(cl,obj,(struct MUIP_Hide *)msg);
	case MUIM_Cleanup: return TextIconList_Cleanup(cl,obj,(struct MUIP_Cleanup *)msg);
	case MUIM_AskMinMax: return TextIconList_AskMinMax(cl,obj,(struct MUIP_AskMinMax *)msg);
	case MUIM_Draw: return TextIconList_Draw(cl,obj,(struct MUIP_Draw *)msg);
//	case MUIM_Layout: return TextIconList_Layout(cl,obj,(struct MUIP_Layout *)msg);
	case MUIM_HandleEvent: return TextIconList_HandleEvent(cl,obj,(struct MUIP_HandleEvent *)msg);
//	case MUIM_CreateDragImage: return TextIconList_CreateDragImage(cl,obj,(APTR)msg);
//	case MUIM_DeleteDragImage: return TextIconList_DeleteDragImage(cl,obj,(APTR)msg);
//	case MUIM_DragQuery: return TextIconList_DragQuery(cl,obj,(APTR)msg);
//	case MUIM_DragReport: return TextIconList_DragReport(cl,obj,(APTR)msg);
//	case MUIM_DragDrop: return TextIconList_DragDrop(cl,obj,(APTR)msg);

//	case MUIM_TextIconList_Update: return TextIconList_Update(cl,obj,(APTR)msg);
	case MUIM_TextIconList_Clear: return TextIconList_Clear(cl,obj,(APTR)msg);
	case MUIM_TextIconList_Add: return TextIconList_Add(cl,obj,(APTR)msg);
//	case MUIM_TextIconList_NextSelected: return TextIconList_NextSelected(cl,obj,(APTR)msg);
//	case MUIM_TextIconList_UnselectAll: return TextIconList_UnselectAll(cl,obj,(APTR)msg);

    }
    
    return DoSuperMethodA(cl, obj, msg);
}

/*================ TextIconListview class ===================*/

#define MUIB_TextIconListview	    	    (MUIB_AROS | 0x00000800)

#define MUIA_TextIconListview_TextIconList  (MUIB_TextIconListview | 0x00000000)
#define MUIA_TextIconListview_UseWinBorder  (MUIB_TextIconListview | 0x00000001)

#define TextIconListviewObject  	    BOOPSIOBJMACRO_START(CL_TextIconListview->mcc_Class)

struct TextIconListview_DATA
{
    Object  	    	*texticonlist;
    Object  	    	*vert, *horiz, *button;
    struct Hook     	 hook;
    struct Hook     	 layout_hook;
};


IPTR TextIconListview_Layout_Function(struct Hook *hook, Object *obj, struct MUI_LayoutMsg *lm)
{
    struct TextIconListview_DATA *data = (struct TextIconListview_DATA *)hook->h_Data;
    
    switch (lm->lm_Type)
    {
	case    MUILM_MINMAX:
		{
		    /* Calulate the minmax dimension of the group,
		    ** We only have a fixed number of children, so we need no NextObject()
		    */
		    WORD maxxxxwidth = 0;
		    WORD maxxxxheight = 0;

		    maxxxxwidth = _minwidth(data->texticonlist) + _minwidth(data->vert);
		    if (_minwidth(data->horiz) > maxxxxwidth) maxxxxwidth = _minwidth(data->horiz);
		    lm->lm_MinMax.MinWidth = maxxxxwidth;

		    maxxxxheight = _minheight(data->texticonlist) + _minheight(data->horiz);
		    if (_minheight(data->vert) > maxxxxheight) maxxxxheight = _minheight(data->vert);
		    lm->lm_MinMax.MinHeight = maxxxxheight;

		    maxxxxwidth = _defwidth(data->texticonlist) + _defwidth(data->vert);
		    if (_defwidth(data->horiz) > maxxxxwidth) maxxxxwidth = _defwidth(data->horiz);
		    lm->lm_MinMax.DefWidth = maxxxxwidth;

		    maxxxxheight = _defheight(data->texticonlist) + _defheight(data->horiz);
		    if (_defheight(data->vert) > maxxxxheight) maxxxxheight = _defheight(data->vert);
		    lm->lm_MinMax.DefHeight = maxxxxheight;

		    lm->lm_MinMax.MaxWidth  = MUI_MAXMAX;
		    lm->lm_MinMax.MaxHeight = MUI_MAXMAX;
		    
		    return 0;
		}

		case MUILM_LAYOUT:
		{
		    /* Now place the objects between (0,0,lm->lm_Layout.Width-1,lm->lm_Layout.Height-1)
		    */

		    LONG virt_width;
		    LONG virt_height;
		    LONG vert_width = _minwidth(data->vert);
		    LONG horiz_height = _minheight(data->horiz);
		    LONG lay_width = lm->lm_Layout.Width;
		    LONG lay_height = lm->lm_Layout.Height;
		    LONG cont_width;
		    LONG cont_height;

		    /* layout the virtual group a first time, to determine the virtual width/height */
		    MUI_Layout(data->texticonlist,0,0,lay_width,lay_height,0);

		    get(data->texticonlist, MUIA_TextIconList_Width, &virt_width);
		    get(data->texticonlist, MUIA_TextIconList_Height, &virt_height);

		    virt_width += _subwidth(data->texticonlist);
		    virt_height += _subheight(data->texticonlist);

		    if (virt_width > lay_width && virt_height > lay_height)
		    {
		    	/* We need all scrollbars and the button */
			set(data->vert, MUIA_ShowMe, TRUE); /* We could also overload MUIM_Show... */
			set(data->horiz, MUIA_ShowMe, TRUE);
			set(data->button, MUIA_ShowMe, TRUE);
			cont_width = lay_width - vert_width;
			cont_height = lay_height - horiz_height;
			MUI_Layout(data->vert, cont_width, 0, vert_width, cont_height,0);
			MUI_Layout(data->horiz, 0, cont_height, cont_width, horiz_height, 0);
			MUI_Layout(data->button, cont_width, cont_height, vert_width, horiz_height, 0);
		    } else
		    {
		    	if (virt_height > lay_height)
		    	{
			    set(data->vert, MUIA_ShowMe, TRUE);
			    set(data->horiz, MUIA_ShowMe, FALSE);
			    set(data->button, MUIA_ShowMe, FALSE);

			    cont_width = lay_width - vert_width;
			    cont_height = lay_height;
			    MUI_Layout(data->vert, cont_width, 0, vert_width, cont_height,0);
		    	} else
		    	{
			    if (virt_width > lay_width)
			    {
				set(data->vert, MUIA_ShowMe, FALSE);
				set(data->horiz, MUIA_ShowMe, TRUE);
				set(data->button, MUIA_ShowMe, FALSE);

				cont_width = lay_width;
				cont_height = lay_height - horiz_height;
				MUI_Layout(data->horiz, 0, cont_height, cont_width, horiz_height, 0);
			    } else
			    {
				set(data->vert, MUIA_ShowMe, FALSE);
				set(data->horiz, MUIA_ShowMe, FALSE);
				set(data->button, MUIA_ShowMe, FALSE);

			    	cont_width = lay_width;
			    	cont_height = lay_height;
			    }
		    	}
		    }

		    /* Layout the group a second time, note that setting _mwidth() and _mheight() should be enough, or we invent a new flag */
		    MUI_Layout(data->texticonlist,0,0,cont_width,cont_height,0);
		    return 1;
		}
    }
    return 0;
}


IPTR TextIconListview_Function(struct Hook *hook, APTR dummyobj, void **msg)
{
    struct TextIconListview_DATA *data = (struct TextIconListview_DATA *)hook->h_Data;
    int type = (int)msg[0];
    LONG val = (LONG)msg[1];

    switch (type)
    {
	case	1:
		{
		    get(data->vert,MUIA_Prop_First,&val);
		    SetAttrs(data->texticonlist,MUIA_TextIconList_Top, val, MUIA_NoNotify, TRUE, TAG_DONE);
		    break;
		}

	case	2:
		{
		    get(data->horiz,MUIA_Prop_First,&val);
		    SetAttrs(data->texticonlist,MUIA_TextIconList_Left, val, MUIA_NoNotify, TRUE, TAG_DONE);
		    break;
		}
	case	3: nnset(data->horiz, MUIA_Prop_First, val); break;
	case	4: nnset(data->vert, MUIA_Prop_First, val); break;
    }
    return 0;
}


IPTR TextIconListview__OM_NEW(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct TextIconListview_DATA *data;
    //struct TagItem *tags,*tag;
    Object *texticonlist = (Object*)GetTagData(MUIA_TextIconListview_TextIconList, 0, msg->ops_AttrList);
    Object *vert,*horiz,*button,*group;

    int usewinborder;

    usewinborder = GetTagData(MUIA_TextIconListview_UseWinBorder, FALSE, msg->ops_AttrList);

    if (!usewinborder) button = ScrollbuttonObject, End;
    else button = NULL;

    obj = (Object *)DoSuperNewTags(cl, obj, NULL,
    	MUIA_Group_Horiz, FALSE,
    	Child, (IPTR) (group = GroupObject,
	    Child, texticonlist,
	    Child, vert = ScrollbarObject,
		usewinborder?MUIA_Prop_UseWinBorder:TAG_IGNORE, MUIV_Prop_UseWinBorder_Right,
		MUIA_Prop_DeltaFactor, 20,
		MUIA_Group_Horiz, FALSE,
		End,
	    Child, horiz = ScrollbarObject,
	    	usewinborder?MUIA_Prop_UseWinBorder:TAG_IGNORE, MUIV_Prop_UseWinBorder_Bottom,
		MUIA_Prop_DeltaFactor, 20,
	    	MUIA_Group_Horiz, TRUE,
	    	End,
	    usewinborder?TAG_IGNORE:Child, button,
	    End),
       TAG_DONE);

    if (!obj)
    {
 	return 0;
    }

    data = INST_DATA(cl, obj);
    data->vert = vert;
    data->horiz = horiz;
    data->button = button;
    data->texticonlist = texticonlist;

    if (!usewinborder)
    {
    	data->layout_hook.h_Entry = HookEntry;
	data->layout_hook.h_SubEntry = (HOOKFUNC)TextIconListview_Layout_Function;
	data->layout_hook.h_Data = data;

	SetAttrs(group, MUIA_Group_Forward, FALSE,
	    	    	MUIA_Group_LayoutHook, &data->layout_hook,
			TAG_DONE);
    }
    
    data->hook.h_Entry = HookEntry;
    data->hook.h_SubEntry = (HOOKFUNC)TextIconListview_Function;
    data->hook.h_Data = data;

    DoMethod(vert, MUIM_Notify, MUIA_Prop_First, MUIV_EveryTime, (IPTR)obj, 4, MUIM_CallHook, (IPTR)&data->hook, 1, MUIV_TriggerValue);
    DoMethod(horiz, MUIM_Notify, MUIA_Prop_First, MUIV_EveryTime, (IPTR)obj, 4, MUIM_CallHook, (IPTR)&data->hook, 2, MUIV_TriggerValue);
    DoMethod(texticonlist, MUIM_Notify, MUIA_TextIconList_Left, MUIV_EveryTime, (IPTR)obj, 4, MUIM_CallHook, (IPTR)&data->hook, 3, MUIV_TriggerValue);
    DoMethod(texticonlist, MUIM_Notify, MUIA_TextIconList_Top, MUIV_EveryTime, (IPTR)obj, 4, MUIM_CallHook, (IPTR)&data->hook, 4, MUIV_TriggerValue);
    DoMethod(texticonlist, MUIM_Notify, MUIA_TextIconList_Width, MUIV_EveryTime, (IPTR)horiz, 3, MUIM_NoNotifySet, MUIA_Prop_Entries, MUIV_TriggerValue);
    DoMethod(texticonlist, MUIM_Notify, MUIA_TextIconList_Height, MUIV_EveryTime, (IPTR)vert, 3, MUIM_NoNotifySet, MUIA_Prop_Entries, MUIV_TriggerValue);
    DoMethod(texticonlist, MUIM_Notify, MUIA_TextIconList_VisWidth, MUIV_EveryTime, (IPTR)horiz, 3, MUIM_NoNotifySet, MUIA_Prop_Visible, MUIV_TriggerValue);
    DoMethod(texticonlist, MUIM_Notify, MUIA_TextIconList_VisHeight, MUIV_EveryTime, (IPTR)vert, 3, MUIM_NoNotifySet, MUIA_Prop_Visible, MUIV_TriggerValue);

    return (IPTR)obj;
}


IPTR TextIconListview__OM_DISPOSE(struct IClass *cl, Object *obj, Msg msg)
{
    //struct TextIconListview_DATA *data = INST_DATA(cl, obj);

    return DoSuperMethodA(cl,obj,msg);
}

IPTR TextIconListview__MUIM_Show(struct IClass *cl, Object *obj, struct MUIP_Show *msg)
{
    struct TextIconListview_DATA *data = INST_DATA(cl, obj);
    IPTR top,left,width,height,viswidth,visheight;

    get(data->texticonlist, MUIA_TextIconList_Left, &left);
    get(data->texticonlist, MUIA_TextIconList_Top, &top);
    get(data->texticonlist, MUIA_TextIconList_Width, &width);
    get(data->texticonlist, MUIA_TextIconList_Height, &height);
    get(data->texticonlist, MUIA_TextIconList_VisWidth, &viswidth);
    get(data->texticonlist, MUIA_TextIconList_VisHeight, &visheight);

    SetAttrs(data->horiz, MUIA_Prop_First, left,
			  MUIA_Prop_Entries, width,
			  MUIA_Prop_Visible, viswidth,
			  TAG_DONE);


    SetAttrs(data->vert,  MUIA_Prop_First, top,
			  MUIA_Prop_Entries, height,
			  MUIA_Prop_Visible, visheight,
			  TAG_DONE);

    return DoSuperMethodA(cl,obj,(Msg)msg);
}

BOOPSI_DISPATCHER(IPTR,TextIconListview_Dispatcher, cl, obj, msg)
{
    switch (msg->MethodID)
    {
	case OM_NEW: return TextIconListview__OM_NEW(cl, obj, (struct opSet *) msg);
	case OM_DISPOSE: return TextIconListview__OM_DISPOSE(cl, obj, msg);
	case MUIM_Show: return TextIconListview__MUIM_Show(cl, obj, (struct MUIP_Show*)msg);
    }
    return DoSuperMethodA(cl, obj, msg);
}


int main(void)
{
    Object *wnd, *iconlist, *texticonlist;
    
    MUIMasterBase = (struct Library*)OpenLibrary("muimaster.library",0);

    CL_TextIconListview = MUI_CreateCustomClass(NULL,MUIC_Group,NULL,sizeof(struct TextIconListview_DATA), TextIconListview_Dispatcher);
    CL_TextIconList = MUI_CreateCustomClass(NULL,MUIC_Area,NULL,sizeof(struct TextIconList_DATA), TextIconList_Dispatcher);

    app = ApplicationObject,
   	SubWindow, wnd = WindowObject,
    	    MUIA_Window_Title, "texticonlist",
	    MUIA_Window_Activate, TRUE,

    	    WindowContents, VGroup,
	    	MUIA_Background, MUII_GroupBack,
#if 0
		Child, IconListviewObject,
		    MUIA_IconListview_IconList, iconlist = IconDrawerListObject,
    	    	    	InputListFrame,
			MUIA_IconDrawerList_Drawer,"C:",
		    	End,
		    End,
#endif
		Child, TextIconListviewObject,
    	    	    // MUIA_TextIconListview_UseWinBorder, TRUE,
		    MUIA_TextIconListview_TextIconList, texticonlist = TextIconListObject,
    	    	    	InputListFrame,
			InnerSpacing(0,0),
		    	End,
		    End,

/*
    	    	Child, texticonlist = TextIconListObject, InputListFrame, End,
*/
		End,
	    End,
	End;

    if (app)
    {
	ULONG sigs = 0;

	DoMethod
        (
            wnd, MUIM_Notify, MUIA_Window_CloseRequest, TRUE, (IPTR) app, 
            2, MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit
        );
	
	{
	    BPTR lock = Lock("C:", SHARED_LOCK);
	    
	    if (lock)
	    {
	    	struct FileInfoBlock *fib = AllocDosObject(DOS_FIB, NULL);
		if (fib)
		{
		    if (Examine(lock, fib))
		    {
		    	while(ExNext(lock, fib))
			{
			    DoMethod(texticonlist, MUIM_TextIconList_Add, fib);
			}
		    }
		    
		    FreeDosObject(DOS_FIB, fib);
		}
		
	    	UnLock(lock);
	    }
	}

	set(wnd,MUIA_Window_Open,TRUE);
    #if 0
    	set(iconlist, MUIA_IconDrawerList_Drawer, "C:");
    #endif
	
	while (DoMethod(app, MUIM_Application_NewInput, (IPTR) &sigs) != MUIV_Application_ReturnID_Quit)
	{
	    if (sigs)
	    {
		sigs = Wait(sigs | SIGBREAKF_CTRL_C | SIGBREAKF_CTRL_D);
		if (sigs & SIGBREAKF_CTRL_C) break;
		if (sigs & SIGBREAKF_CTRL_D) break;
	    }
	}

	MUI_DisposeObject(app);
    }

    MUI_DeleteCustomClass(CL_TextIconList);
    MUI_DeleteCustomClass(CL_TextIconListview);
    
    CloseLibrary(MUIMasterBase);
    
    return 0;
}

