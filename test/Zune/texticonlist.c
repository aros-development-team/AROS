/*
    Copyright © 2002-2004, The AROS Development Team. All rights reserved.
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
#include <proto/cybergraphics.h>
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

#define AUTOSCROLL_MILLIS  	    	20

struct TextIconEntry
{
    struct MinNode  	    node;
    struct MinNode  	    selection_node;
    struct FileInfoBlock    fib;
    LONG    	    	    field_width[NUM_COLUMNS];
    UBYTE   	    	    datebuf[LEN_DATSTRING];
    UBYTE   	    	    timebuf[LEN_DATSTRING];
    UBYTE   	    	    sizebuf[30];
    UBYTE   	    	    protbuf[8];
    UBYTE     	    	    selected;
    UBYTE   	    	    dirty;
};

struct MyColor
{
    ULONG   pixel;
    ULONG   rgbpixel;
    UBYTE   alloced;
};

#define COLOR_COLUMN_BACKGROUND     	    	0
#define COLOR_COLUMN_BACKGROUND_SORTED	    	1
#define COLOR_COLUMN_BACKGROUND_LASSO	    	2
#define COLOR_COLUMN_BACKGROUND_LASSO_SORTED	3

#define COLOR_SELECTED_BACKGROUND   	    	4
#define COLOR_SELECTED_BACKGROUND_SORTED    	5
#define COLOR_SELECTED_BACKGROUND_LASSO     	6
#define COLOR_SELECTED_BACKGROUND_LASSO_SORTED	7

#define NUM_COLORS  8

static const ULONG rgb_colors[NUM_COLORS] =
{
    0xFFFFFF,
    0xF0F0F0,
    0xE4E7Ef,
    0xDEE1E9,
    0x0A246A,
    0x0A246A,
    0x324882,
    0x324882    
};

static const ULONG pen_colors[NUM_COLORS] =
{
    MPEN_SHINE,
    MPEN_SHINE,
    MPEN_BACKGROUND,
    MPEN_BACKGROUND,
    MPEN_FILL,
    MPEN_FILL,
    MPEN_FILL,
    MPEN_FILL
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
    struct MinList  	    	selection_list;
    struct RastPort 	    	temprp;
    struct Rectangle	    	view_rect;
    struct Rectangle	    	header_rect;
    struct Rectangle	    	lasso_rect, old_lasso_rect;
    struct Rectangle	    	*update_rect1, *update_rect2;
    struct MyColor  	    	colors[NUM_COLORS];
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
    BYTE    	    	    	column_sortable[NUM_COLUMNS];
    STRPTR  	    	    	column_title[NUM_COLUMNS];
    BYTE    	    	    	sort_column;
    BYTE    	    	    	sort_direction;
    BYTE    	    	    	sort_dirs;
    LONG    	    	    	inputstate;
    BOOL    	    	    	show_header;
    BOOL    	    	    	is_setup;
    BOOL    	    	    	is_shown;
    BOOL    	    	    	lasso_active;
    BOOL    	    	    	lasso_paint;
    BOOL    	    	    	truecolor;
    
    struct MUI_EventHandlerNode ehn;    
    struct MUI_InputHandlerNode thn;
};

#define UPDATE_SCROLL	    	2
#define UPDATE_DIRTY_ENTRIES  	3
#define UPDATE_ALL  	    	4
#define UPDATE_HEADER	    	5

#define INPUTSTATE_NONE     	    0
#define INPUTSTATE_PAN	    	    1
#define INPUTSTATE_COL_RESIZE 	    2
#define INPUTSTATE_COL_HEADER_CLICK 3
#define INPUTSTATE_LASSO    	    4

#define MUIB_TextIconList   	    (MUIB_AROS | 0x00000700)

#define MUIA_TextIconList_Left      (MUIB_TextIconList | 0x00000000)
#define MUIA_TextIconList_Top       (MUIB_TextIconList | 0x00000001)
#define MUIA_TextIconList_Width     (MUIB_TextIconList | 0x00000002)
#define MUIA_TextIconList_Height    (MUIB_TextIconList | 0x00000003)
#define MUIA_TextIconList_VisWidth  (MUIB_TextIconList | 0x00000004)
#define MUIA_TextIconList_VisHeight (MUIB_TextIconList | 0x00000005)

#define MUIM_TextIconList_Clear     (MUIB_TextIconList | 0x00000000)
#define MUIM_TextIconList_Add	    (MUIB_TextIconList | 0x00000001)
#define MUIM_TextIconList_AutoScroll (MUIB_TextIconList | 0x00000002)

struct MUIP_TextIconList_Clear      {STACKULONG MethodID;};
struct MUIP_TextIconList_Add        {STACKULONG MethodID; struct FileInfoBlock *fib;};

#define TextIconListObject  	    BOOPSIOBJMACRO_START(CL_TextIconList->mcc_Class)


#define INDEX_NAME  	    0
#define INDEX_SIZE  	    1
#define INDEX_PROTECTION    2
#define INDEX_DATE  	    3
#define INDEX_TIME  	    4
#define INDEX_COMMENT 	    5

#define SORT_DRAWERS_FIRST  0
#define SORT_DRAWERS_MIXED  1
#define SORT_DRAWERS_LAST   2

#define SORT_DIRECTION_UP   0
#define SORT_DIRECTION_DOWN 1

#define SORT_BY_NAME	    0
#define SORT_BY_DATE	    1
#define SORT_BY_SIZE	    2

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

static LONG ColumnHeaderUnderMouse(struct TextIconList_DATA *data, LONG mx, LONG my)
{
    LONG col = -1;

    if (data->show_header &&
    	(my >= data->header_rect.MinY) &&
	(my <= data->header_rect.MaxY))
    {
    	col = ColumnUnderMouse(data, mx, my);
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
    LONG firstvis, lastvis;
    
    firstvis = FirstVisibleColumnNumber(data);
    lastvis = LastVisibleColumnNumber(data);
    
    for(i = 0; i < NUM_COLUMNS; i++)
    {
    	LONG idx = data->column_pos[i];
	LONG w;
	
    	if (!data->column_visible[idx]) continue;
	
	w = data->column_width[idx];
	
	if (idx == index)
	{
	    retval = TRUE;
	    *x1 = x - ((i == firstvis) ? LINE_SPACING_LEFT : 0);
	    *x2 = x + w - 1 + ((i == lastvis) ? LINE_SPACING_RIGHT : 0);
	    break;	    
	}
	
	x += w;
    }
    
    return retval;
}

static LONG CompareNodes(struct TextIconList_DATA *data, struct TextIconEntry *node1, struct TextIconEntry *node2)
{
    LONG pri1 = (node1->fib.fib_DirEntryType > 0) ? 1 : 0;
    LONG pri2 = (node2->fib.fib_DirEntryType > 0) ? 1 : 0;
    LONG diff = (pri2 - pri1) * -(data->sort_dirs - 1);
    
    if (!diff)
    {
    	switch(data->sort_column)
	{
	    case INDEX_DATE:
	    case INDEX_TIME:
	    	diff = CompareDates((const struct DateStamp *)&node2->fib.fib_Date,
		    	    	    (const struct DateStamp *)&node1->fib.fib_Date);

    	    	break;
		
	    case INDEX_SIZE:
	    	if (node1->fib.fib_Size < node2->fib.fib_Size)
		{
		    diff = -1;
		}
		else if (node1->fib.fib_Size > node2->fib.fib_Size)
		{
		    diff = 1;
		}
		break;
		
    	    default:
	    case INDEX_NAME:
	    	diff = Stricmp(node1->fib.fib_FileName, node2->fib.fib_FileName);
		break;
		
	}
	
	if (data->sort_direction == SORT_DIRECTION_DOWN) diff = -diff;
    }
    
    return diff;
}

static void SortInNode(struct TextIconList_DATA *data, struct List *list, struct Node *node,
		       LONG (*compare)(APTR data, APTR node1, APTR node2))
{
    struct Node *prevnode = NULL;
    struct Node *checknode;
    
    ForeachNode(list, checknode)
    {
        if (compare(data, node, checknode) < 0) break;

	prevnode = checknode;
    }
    
    Insert(list, node, prevnode);
}

static void ReSortEntries(struct TextIconList_DATA *data)
{
    struct List templist;
    struct Node *node, *succ;
    
    NEWLIST(&templist);
    
    ForeachNodeSafe(&data->entries_list, node, succ)
    {
    	Remove(node);
    	AddTail(&templist, node);
    }
    
    ForeachNodeSafe(&templist, node, succ)
    {
    	SortInNode(data, (struct List *)&data->entries_list, node, (APTR)CompareNodes);
    }
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

static void GetAbsoluteLassoRect(struct TextIconList_DATA *data, struct Rectangle *lasso_rect)
{
    WORD minx = data->lasso_rect.MinX;
    WORD miny = data->lasso_rect.MinY;
    WORD maxx = data->lasso_rect.MaxX;
    WORD maxy = data->lasso_rect.MaxY;
    
    if (minx > maxx)
    {
    	/* Swap minx, maxx */
    	minx ^= maxx;
	maxx ^= minx;
	minx ^= maxx;
    }
    
    if (miny > maxy)
    {
    	/* Swap miny, maxy */
    	miny ^= maxy;
	maxy ^= miny;
	miny ^= maxy;
    }
    
    lasso_rect->MinX = data->view_rect.MinX - data->view_x + minx;
    lasso_rect->MinY = data->view_rect.MinY - data->view_y + miny;
    lasso_rect->MaxX = data->view_rect.MinX - data->view_x + maxx;
    lasso_rect->MaxY = data->view_rect.MinY - data->view_y + maxy;
}

static void EnableMouseMoveEvents(Object *obj, struct TextIconList_DATA *data)
{
    if (!(data->ehn.ehn_Events & IDCMP_MOUSEMOVE))
    {
	DoMethod(_win(obj),MUIM_Window_RemEventHandler, (IPTR)&data->ehn);
	data->ehn.ehn_Events |= IDCMP_MOUSEMOVE;
	DoMethod(_win(obj),MUIM_Window_AddEventHandler, (IPTR)&data->ehn);			    
    }
}

static void DisableMouseMoveEvents(Object *obj, struct TextIconList_DATA *data)
{
    if (data->ehn.ehn_Events & IDCMP_MOUSEMOVE)
    {
	DoMethod(_win(obj),MUIM_Window_RemEventHandler, (IPTR)&data->ehn);
	data->ehn.ehn_Events &= ~IDCMP_MOUSEMOVE;
	DoMethod(_win(obj),MUIM_Window_AddEventHandler, (IPTR)&data->ehn);			    
    }
}

static void EnableAutoScrollTimer(Object *obj, struct TextIconList_DATA *data)
{
    if (!data->thn.ihn_Millis)
    {
    	data->thn.ihn_Millis = AUTOSCROLL_MILLIS;
	DoMethod(_app(obj), MUIM_Application_AddInputHandler, (IPTR)&data->thn);
    }    
}

static void DisableAutoScrollTimer(Object *obj, struct TextIconList_DATA *data)
{
    if (data->thn.ihn_Millis)
    {
    	data->thn.ihn_Millis = 0;
	DoMethod(_app(obj), MUIM_Application_RemInputHandler, (IPTR)&data->thn);
    }
}

static BOOL OrRectOutlineRegion(struct Region *reg, struct Rectangle *rect)
{
    struct Rectangle r;
    BOOL result;
    
    r.MinX = rect->MinX;
    r.MinY = rect->MinY;
    r.MaxX = rect->MaxX;
    r.MaxY = rect->MinY;
    result = OrRectRegion(reg, &r);

    r.MinX = rect->MaxX;
    r.MinY = rect->MinY;
    r.MaxX = rect->MaxX;
    r.MaxY = rect->MaxY;
    result = result && OrRectRegion(reg, &r);

    r.MinX = rect->MinX;
    r.MinY = rect->MaxY;
    r.MaxX = rect->MaxX;
    r.MaxY = rect->MaxY;
    result = result && OrRectRegion(reg, &r);

    r.MinX = rect->MinX;
    r.MinY = rect->MinY;
    r.MaxX = rect->MinX;
    r.MaxY = rect->MaxY;
    result = result && OrRectRegion(reg, &r);

    return result;
}

static void MyRectFill(struct TextIconList_DATA *data, struct RastPort *rp,
    	    	       LONG x1, LONG y1, LONG x2, LONG y2, LONG mycol)
{
    if (x1 > x2)
    {
    	x1 ^= x2;
	x2 ^= x1;
	x1 ^= x2;
    }

    if (y1 > y2)
    {
    	y1 ^= y2;
	y2 ^= y1;
	y1 ^= y2;
    }
    
    SetDrMd(rp, JAM1);
    
    if (data->truecolor && CyberGfxBase)
    {
    	FillPixelArray(rp, x1, y1, x2 - x1 + 1, y2 - y1 + 1, data->colors[mycol].rgbpixel);
    }
    else
    {
    	SetAPen(rp, data->colors[mycol].pixel);
	RectFill(rp, x1, y1, x2, y2);
    }
}

static void RenderHeaderField(Object *obj, struct TextIconList_DATA *data,
    	    	    	    struct Rectangle *rect, LONG index)
{
    STRPTR text;
    struct TextExtent te;
    ULONG fit;
    BOOL  sel = FALSE;
    
    if ((data->inputstate == INPUTSTATE_COL_HEADER_CLICK) &&
    	(data->click_column == index))
    	
    {
    	if (ColumnHeaderUnderMouse(data, data->click_x, data->click_y) == index)
	{
	    sel = TRUE;
	}
    }
    
    text = GetTextIconHeaderText(data, index);

    SetAPen(_rp(obj), _pens(obj)[sel ? MPEN_HALFSHADOW : MPEN_HALFSHINE]);
    RectFill(_rp(obj), rect->MinX + 1, rect->MinY + 1,
    	    	       rect->MaxX - 1, rect->MaxY - 1);
    SetAPen(_rp(obj), _pens(obj)[sel ? MPEN_SHADOW : MPEN_SHINE]);
    RectFill(_rp(obj), rect->MinX, rect->MinY, rect->MinX, rect->MaxY);
    RectFill(_rp(obj), rect->MinX + 1, rect->MinY, rect->MaxX - 1, rect->MinY);
    SetAPen(_rp(obj), _pens(obj)[sel ? MPEN_HALFSHINE : MPEN_HALFSHADOW]);
    RectFill(_rp(obj), rect->MaxX, rect->MinY, rect->MaxX, rect->MaxY);
    RectFill(_rp(obj), rect->MinX + 1, rect->MaxY, rect->MaxX - 1, rect->MaxY);
    		       
    if (index == data->sort_column)
    {
    	LONG x = rect->MaxX - 4 - 6;
	LONG y = (rect->MinY + rect->MaxY + 1) / 2 - 3;
	
	if (x > rect->MinX)
	{
	    SetAPen(_rp(obj), _pens(obj)[sel ? MPEN_SHADOW : MPEN_HALFSHADOW]);
	    if (data->sort_direction == SORT_DIRECTION_UP)
	    {
		RectFill(_rp(obj), x, y, x + 5, y + 1);
		RectFill(_rp(obj), x + 1, y + 2, x + 4, y + 3);
		RectFill(_rp(obj), x + 2, y + 4, x + 3, y + 5);
	    }
	    else
	    {
		RectFill(_rp(obj), x, y + 4, x + 5, y + 5);
		RectFill(_rp(obj), x + 1, y + 2, x + 4, y + 3);
		RectFill(_rp(obj), x + 2, y, x + 3, y + 1);
	    }
	}
    }
    		       
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

	SetABPenDrMd(_rp(obj), _pens(obj)[MPEN_TEXT], 0, JAM1);
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
    linerect.MaxX = data->header_rect.MaxX; //linerect.MinX + data->width - 1;
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
    
    x += HEADERLINE_SPACING_RIGHT;
    
    if (x < linerect.MaxX)
    {
    	linerect.MinX = x;
	
	if (MustRenderRect(data, &linerect))
	{
    	    SetABPenDrMd(_rp(obj), _pens(obj)[MPEN_HALFSHINE], 0, JAM1);
	    RectFill(_rp(obj), linerect.MinX, linerect.MinY, linerect.MaxX, linerect.MaxY);
	}
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

    fit = selected ? COLOR_SELECTED_BACKGROUND : COLOR_COLUMN_BACKGROUND;
    if (index == data->sort_column) fit++;
    if (data->lasso_paint) fit += 2;
    
    MyRectFill(data, _rp(obj), rect->MinX, rect->MinY, rect->MaxX, rect->MaxY, fit);
    
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
    linerect.MaxX = data->view_rect.MaxX; //linerect.MinX + data->width - 1;
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

    x += LINE_SPACING_RIGHT;
    
    if (x < linerect.MaxX)
    {
    	linerect.MinX = x;
	
	if (MustRenderRect(data, &linerect))
	{
    	    SetABPenDrMd(_rp(obj), _pens(obj)[data->lasso_paint ? MPEN_BACKGROUND : MPEN_SHINE], 0, JAM1);
	    RectFill(_rp(obj), linerect.MinX, linerect.MinY, linerect.MaxX, linerect.MaxY);
	}
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


static void RethinkLasso(Object *obj, struct TextIconList_DATA *data)
{
    struct TextIconEntry *entry;
    
    LONG ny1 = data->lasso_rect.MinY;
    LONG ny2 = data->lasso_rect.MaxY;
    LONG oy1 = data->old_lasso_rect.MinY;
    LONG oy2 = data->old_lasso_rect.MaxY;
    LONG y1, y2;
    LONG x1, x2;
    LONG numdirty = 0;
    BOOL lasso_hot;
    
    if (!data->num_entries) return;
    
    if (ny1 > ny2)
    {
    	ny1 ^= ny2;
	ny2 ^= ny1;
	ny1 ^= ny2;
    }
    
    if (oy1 > oy2)
    {
    	oy1 ^= oy2;
	oy2 ^= oy1;
	oy1 ^= oy2;    
    }
    
    ny1 /= data->lineheight;
    ny2 /= data->lineheight;
    oy1 /= data->lineheight;
    oy2 /= data->lineheight;
    
    y1 = (ny1 < oy1) ? ny1 : oy1;
    y2 = (ny2 > oy2) ? ny2 : oy2;
    
    if (y1 < 0)
    {
    	y1 = 0;
    }
    else if (y1 >= data->num_entries)
    {
    	y1 = data->num_entries - 1;
    }
    
    if (y2 < 0)
    {
    	y2 = 0;
    }
    else if (y2 >= data->num_entries)
    {
    	y2 = data->num_entries - 1;
    }
    
    GetColumnCoords(data, INDEX_NAME, &x1, &x2);
    x1 += data->view_x - data->view_rect.MinX;
    x2 += data->view_x - data->view_rect.MinX;
    
    lasso_hot = ((data->lasso_rect.MinX >= x1) && (data->lasso_rect.MinX <= x2)) ||
    	    	((data->lasso_rect.MaxX >= x1) && (data->lasso_rect.MaxX <= x2)) ||
		((data->lasso_rect.MinX < x1) && (data->lasso_rect.MaxX > x2)) ||
		((data->lasso_rect.MaxX < x1) && (data->lasso_rect.MinX > x2));
    
    entry = GetEntryFromIndex(data, y1);
    while(entry && entry->node.mln_Succ && (y1 <= y2))
    {
    	BOOL select;
	
    	select = (y1 >= ny1) && (y1 <= ny2) && lasso_hot;
	if (select != entry->selected)
	{
	    if (select)
	    {
	    	AddTail((struct List *)&data->selection_list, (struct Node *)&entry->selection_node);
		data->num_selected++;
	    }
	    else
	    {
	    	Remove((struct Node *)&entry->selection_node);
		data->num_selected--;
	    }
	    entry->selected = select;
	    entry->dirty = TRUE;
	    numdirty++;
	}
	
    	entry = (struct TextIconEntry *)entry->node.mln_Succ;
	y1++;
    }
    
    if (numdirty)
    {
    	data->update = UPDATE_DIRTY_ENTRIES;
	MUI_Redraw(obj, MADF_DRAWUPDATE);
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
    NewList((struct List*)&data->selection_list);
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

    data->column_sortable[INDEX_NAME] = TRUE;
    data->column_sortable[INDEX_SIZE] = TRUE;
    data->column_sortable[INDEX_DATE] = TRUE;
    data->column_sortable[INDEX_TIME] = TRUE;

    data->sort_column = INDEX_NAME;
    data->sort_direction = SORT_DIRECTION_UP;
    data->sort_dirs = SORT_DRAWERS_FIRST;
        
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

    data->thn.ihn_Flags = MUIIHNF_TIMER;
    data->thn.ihn_Method = MUIM_TextIconList_AutoScroll;
    data->thn.ihn_Object = obj;

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
    LONG    	    	      i;
    
    if (!DoSuperMethodA(cl, obj, (Msg) msg)) return 0;
    
    DoMethod(_win(obj),MUIM_Window_AddEventHandler, (IPTR)&data->ehn);

    InitRastPort(&data->temprp);
    SetFont(&data->temprp, _font(obj));
    data->truecolor = GetBitMapAttr(_screen(obj)->RastPort.BitMap, BMA_DEPTH) >= 15;
    
    data->lineheight = LINE_EXTRAHEIGHT + _font(obj)->tf_YSize;
    data->is_setup = TRUE;
    
    for(i = 0; i < NUM_COLORS; i++)
    {
 	data->colors[i].rgbpixel = rgb_colors[i];
	data->colors[i].pixel = data->colors[i].rgbpixel;
	
	if (!data->truecolor || !CyberGfxBase)
	{
	    ULONG r = (rgb_colors[i] & 0x00FF0000) >> 16;
	    ULONG g = (rgb_colors[i] & 0x0000FF00) >> 8;
	    ULONG b = (rgb_colors[i] & 0x000000FF);
	    
	    LONG pen = ObtainBestPen(_screen(obj)->ViewPort.ColorMap,
	    	    	    	     r * 0x01010101,
				     g * 0x01010101,
				     b * 0x01010101,
				     OBP_FailIfBad, FALSE,
				     OBP_Precision, PRECISION_GUI,
				     TAG_DONE);

    	    if (pen >= 0)
	    {
	    	data->colors[i].pixel = pen;
		data->colors[i].alloced = TRUE;
	    }
	    else
	    {
	    	data->colors[i].pixel = _pens(obj)[pen_colors[i]];
	    	data->colors[i].alloced = FALSE;
	    }			     
	}
    }
    
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
    LONG    	    	      i;
    
    DoMethod(_win(obj),MUIM_Window_RemEventHandler, (IPTR)&data->ehn);

    DeinitRastPort(&data->temprp);
        
    for(i = 0; i < NUM_COLORS; i++)
    {
    	if (data->colors[i].alloced)
	{
	    ReleasePen(_screen(obj)->ViewPort.ColorMap, data->colors[i].pixel);
	    data->colors[i].alloced = FALSE;
	}
    }
    
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

static void DrawHeaderLine(Object *obj, struct TextIconList_DATA *data)
{
    APTR clip;
    
    if (data->show_header && MustRenderRect(data, &data->header_rect))
    {
	clip = MUI_AddClipping(muiRenderInfo(obj), data->header_rect.MinX,
    	    	    	    	    		   data->header_rect.MinY,
						   data->header_rect.MaxX - data->header_rect.MinX + 1,
						   data->header_rect.MaxY - data->header_rect.MinY + 1);

	RenderHeaderline(obj, data);

	MUI_RemoveClipping(muiRenderInfo(obj),clip);
    }
}

static void DrawLassoOutline(Object *obj, struct TextIconList_DATA *data)
{
    struct Rectangle lasso;
    
    GetAbsoluteLassoRect(data, &lasso);

#if 1
    MyRectFill(data, _rp(obj), lasso.MinX, lasso.MinY, lasso.MaxX, lasso.MinY, COLOR_SELECTED_BACKGROUND);
    MyRectFill(data, _rp(obj), lasso.MaxX, lasso.MinY, lasso.MaxX, lasso.MaxY, COLOR_SELECTED_BACKGROUND);
    MyRectFill(data, _rp(obj), lasso.MinX, lasso.MaxY, lasso.MaxX, lasso.MaxY, COLOR_SELECTED_BACKGROUND);
    MyRectFill(data, _rp(obj), lasso.MinX, lasso.MinY, lasso.MinX, lasso.MaxY, COLOR_SELECTED_BACKGROUND);    
#else    
    SetABPenDrMd(_rp(obj), _pens(obj)[MPEN_SHADOW], 0, JAM1);
    Move(_rp(obj), lasso.MinX, lasso.MinY);
    Draw(_rp(obj), lasso.MaxX, lasso.MinY);
    Draw(_rp(obj), lasso.MaxX, lasso.MaxY);
    Draw(_rp(obj), lasso.MinX, lasso.MaxY);
    Draw(_rp(obj), lasso.MinX, lasso.MinY);
#endif
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
	    struct Region *clipregion;
	    LONG first, numvisible, index = 0;

	    data->update = 0;

	    clipregion = NewRectRegion(data->view_rect.MinX,
	    	    	    	       data->view_rect.MinY,
				       data->view_rect.MaxX,
				       data->view_rect.MaxY);
	    
	    if (clipregion)
	    {
		if (data->lasso_active)
		{
	    	    struct Rectangle lasso_rect;

		    GetAbsoluteLassoRect(data, &lasso_rect);
	    	    ClearRectRegion(clipregion, &lasso_rect);
    		}		

		clip = MUI_AddClipRegion(muiRenderInfo(obj), clipregion);

		first = FirstVisibleLine(data);
		numvisible = NumVisibleLines(data);

		ForeachNode(&data->entries_list, entry)
		{
	    	    if (entry->dirty)
		    {
			if ((index >= first) && (index < first + numvisible))
			{
		    	    RenderEntry(obj, data, index);
			}
		    }
		    index++;
		}

    	    	if (data->lasso_active)
		{
	    	    struct Rectangle lasso_rect;
	    	    struct Rectangle vis_lasso_rect;

		    GetAbsoluteLassoRect(data, &lasso_rect);

		    if (AndRectRect(&data->view_rect, &lasso_rect, &vis_lasso_rect))
		    {
	    		MUI_RemoveClipRegion(muiRenderInfo(obj),clip);

			clipregion = NewRectRegion(vis_lasso_rect.MinX,
	    	    	    			   vis_lasso_rect.MinY,
						   vis_lasso_rect.MaxX,
						   vis_lasso_rect.MaxY);

    	    		data->lasso_paint = TRUE;

			clip = MUI_AddClipRegion(muiRenderInfo(obj), clipregion);

    	    	    	index = 0;
			ForeachNode(&data->entries_list, entry)
			{
	    		    if (entry->dirty)
			    {
				if ((index >= first) && (index < first + numvisible))
				{
		    		    RenderEntry(obj, data, index);
				}
			    }
			    index++;
			}
			
			data->lasso_paint = FALSE;
			
			DrawLassoOutline(obj, data);

		    } /* if (AndRectRect(&data->view_rect, &lasso_rect, &vis_lasso_rect)) */
		    
		} /* if (data->lasso_active) */
		
		MUI_RemoveClipRegion(muiRenderInfo(obj), clip);

		ForeachNode(&data->entries_list, entry)
    	    	{
		    if (entry->dirty) entry->dirty = FALSE;
		}
		return 0;
	    }
	    
	} /* else if (data->update == UPDATE_DIRTY_ENTRIES) */
	else if (data->update == UPDATE_HEADER)
	{
	    data->update = 0;
	    
	    DrawHeaderLine(obj, data);
	    
	    return 0;
	}
	
    } /* if (msg->flags & MADF_DRAWUPDATE) */

    if (MustRenderRect(data, &data->view_rect))
    {
    	struct Region *clipregion;
	
	clipregion = NewRectRegion(data->view_rect.MinX,
	    	    	    	   data->view_rect.MinY,
				   data->view_rect.MaxX,
				   data->view_rect.MaxY);
					    
    	if (clipregion)
	{
	    if (data->lasso_active)
	    {
	    	struct Rectangle lasso_rect;

		GetAbsoluteLassoRect(data, &lasso_rect);
	    	ClearRectRegion(clipregion, &lasso_rect);
    	    }		
		
	    clip = MUI_AddClipRegion(muiRenderInfo(obj), clipregion);

	    RenderAllEntries(obj, data);

    	    if (data->lasso_active)
	    {
	    	struct Rectangle lasso_rect;
	    	struct Rectangle vis_lasso_rect;
		
		GetAbsoluteLassoRect(data, &lasso_rect);
		
		if (AndRectRect(&data->view_rect, &lasso_rect, &vis_lasso_rect))
		{
	    	    MUI_RemoveClipRegion(muiRenderInfo(obj),clip);

		    clipregion = NewRectRegion(vis_lasso_rect.MinX,
	    	    	    		       vis_lasso_rect.MinY,
					       vis_lasso_rect.MaxX,
					       vis_lasso_rect.MaxY);

    	    	    data->lasso_paint = TRUE;
		    
		    clip = MUI_AddClipRegion(muiRenderInfo(obj), clipregion);

		    RenderAllEntries(obj, data);
		    
		    data->lasso_paint = FALSE;

		    DrawLassoOutline(obj, data);
		    	    	
		}
	    }
	    
	    MUI_RemoveClipRegion(muiRenderInfo(obj),clip);
	    
	} /* if (clipregion) */
	
    } /* if (MustRenderRect(data, &data->view_rect)) */
    
    DrawHeaderLine(obj, data);
    
    data->update = 0;

    return 0;
}

/**************************************************************************
 MUIM_Draw
**************************************************************************/
static IPTR TextIconList_AutoScroll(struct IClass *cl, Object *obj, Msg msg)
{
    struct TextIconList_DATA *data = INST_DATA(cl, obj);

    if (data->lasso_active)
    {
	LONG new_view_x, new_view_y;
		    
	new_view_x = data->view_x;
	new_view_y  = data->view_y;
		    
	if (data->click_x < data->view_rect.MinX)
	{
	    new_view_x -= (data->view_rect.MinX - data->click_x) / 4;
	}
	else if (data->click_x > data->view_rect.MaxX)
	{
	    new_view_x += (data->click_x - data->view_rect.MaxX) / 4;
	}
	
	if (data->click_y < data->view_rect.MinY)
	{
	    new_view_y -= (data->view_rect.MinY - data->click_y) / 4;
	}
	else if (data->click_y > data->view_rect.MaxY)
	{
	    new_view_y += (data->click_y - data->view_rect.MaxY) / 4;
	}
	
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
	    data->old_lasso_rect = data->lasso_rect;
	    	    
	    data->lasso_rect.MaxX += new_view_x - data->view_x;
	    data->lasso_rect.MaxY += new_view_y - data->view_y;

	    RethinkLasso(obj, data);
	    
	    SetAttrs(obj, MUIA_TextIconList_Left, new_view_x,
			  MUIA_TextIconList_Top, new_view_y,
			  TAG_DONE);
	}

    }
    else
    {
    	DisableAutoScrollTimer(obj, data);
    }
    
    return 0;
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
					    Remove((struct Node *)&old->selection_node);
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
					    	    Remove((struct Node *)&entry->selection_node);
					    	    entry->selected = FALSE;
						    entry->dirty = TRUE;
						}
					    }

					    data->num_selected = 0;
					}

					if (new && !new->selected)
					{
					    AddTail((struct List *)&data->selection_list, (struct Node *)&new->selection_node);
				    	    new->selected = TRUE;
					    new->dirty = TRUE;

					    data->num_selected++;
					}

					data->update = UPDATE_DIRTY_ENTRIES;
					MUI_Redraw(obj, MADF_DRAWUPDATE);
					
				    } /* if (data->active_entry != line) */
				    
				} /* if (data->column_clickable[col]) */
				else
				{
				    if (!shift_qual && data->num_selected)
				    {
				    	struct TextIconEntry *entry;

					ForeachNode(&data->entries_list, entry)
					{
					    if (entry->selected)
					    {
					    	Remove((struct Node *)&entry->selection_node);
					    	entry->selected = FALSE;
						entry->dirty = TRUE;
					    }
					}

					data->num_selected = 0;
					data->update = UPDATE_DIRTY_ENTRIES;
					MUI_Redraw(obj, MADF_DRAWUPDATE);
				    }

				    data->lasso_rect.MinX = mx - data->view_rect.MinX + data->view_x;
				    data->lasso_rect.MinY = my - data->view_rect.MinY + data->view_y;
				    data->lasso_rect.MaxX = mx - data->view_rect.MinX + data->view_x;
				    data->lasso_rect.MaxY = my - data->view_rect.MinY + data->view_y;
				    
				    data->inputstate = INPUTSTATE_LASSO;
				    data->lasso_active = TRUE;	

				    data->click_x = mx;
				    data->click_y = my;
				    
				    EnableMouseMoveEvents(obj, data);			    
				}
				
			    } /* if click on entry */
			    else if ((col = ColumnResizeHandleUnderMouse(data, mx, my)) >= 0)
			    {
				data->inputstate = INPUTSTATE_COL_RESIZE;
				data->click_column = col;
				data->click_x = mx - data->column_width[col];

    	    	    	    	EnableMouseMoveEvents(obj, data);
				
			    } /* else if click on column header entry resize handle */
			    else if ((col = ColumnHeaderUnderMouse(data, mx, my)) >= 0)
			    {
				data->inputstate = INPUTSTATE_COL_HEADER_CLICK;
				data->click_column = col;
				data->click_x = mx;
				data->click_y = my;

    	    	    	    	EnableMouseMoveEvents(obj, data);

    	    	    	    	data->update = UPDATE_HEADER;
				MUI_Redraw(obj, MADF_DRAWUPDATE);
			    }
			    
			} /* if (data->inputstate == INPUTSTATE_NONE) */
		    	break;
			
		    case SELECTUP:
		    	if (data->inputstate == INPUTSTATE_COL_RESIZE)
			{
			    DisableMouseMoveEvents(obj, data);
			    
			    data->inputstate = INPUTSTATE_NONE;
			}
			else if (data->inputstate == INPUTSTATE_COL_HEADER_CLICK)
			{
			    DisableMouseMoveEvents(obj, data);
			
			    data->inputstate = INPUTSTATE_NONE;
			    
			    if (ColumnHeaderUnderMouse(data, data->click_x, data->click_y) == data->click_column)
			    {
				
				if (data->column_sortable[data->click_column])
				{
				    if (data->sort_column == data->click_column)
				    {
				    	data->sort_direction = 1 - data->sort_direction;
				    }
				    else
				    {
				    	data->sort_direction = SORT_DIRECTION_UP;
					data->sort_column = data->click_column;
				    }
				
				    ReSortEntries(data);
				
				    data->update = UPDATE_ALL;
				    MUI_Redraw(obj, MADF_DRAWUPDATE);

				}
				else
				{
			    	    data->update = UPDATE_HEADER;
				    MUI_Redraw(obj, MADF_DRAWUPDATE);
				}
				
			    } /* mouse still over column header */
			    
			} /* else if (data->inputstate == INPUTSTATE_COL_HEADER_CLICK) */
			else if (data->inputstate == INPUTSTATE_LASSO)
			{
			    DisableMouseMoveEvents(obj, data);
			    DisableAutoScrollTimer(obj, data);
			    
			    data->inputstate = INPUTSTATE_NONE;
			    data->lasso_active = FALSE;
			    
			    data->update = UPDATE_ALL;
			    MUI_Redraw(obj, MADF_DRAWUPDATE);
			}			
			break;
		    	
		    case MIDDLEDOWN:
		    	if (data->inputstate == INPUTSTATE_NONE)
			{
			    data->inputstate = INPUTSTATE_PAN;
			    
			    data->click_x = mx - data->view_rect.MinX + data->view_x;
			    data->click_y = my - data->view_rect.MinY + data->view_y;

    	    	    	    EnableMouseMoveEvents(obj, data);

			}
		    	break;
			
		    case MIDDLEUP:
		    	if (data->inputstate == INPUTSTATE_PAN)
			{
			    DisableMouseMoveEvents(obj, data);
			    
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
		else if (data->inputstate == INPUTSTATE_COL_HEADER_CLICK)
		{
		    BOOL old = ColumnHeaderUnderMouse(data, data->click_x, data->click_y);
		    BOOL new = ColumnHeaderUnderMouse(data, mx, my);
		    
		    if (new != old)
		    {
		    	data->click_x = mx;
			data->click_y = my;
			data->update = UPDATE_HEADER;
			MUI_Redraw(obj, MADF_DRAWUPDATE);
		    }
		}
		else if (data->inputstate == INPUTSTATE_LASSO)
		{
		    struct Rectangle 	 old_lasso, new_lasso;
		    struct Region   	*region;
		    APTR    	    	 clip;
		    
		    data->click_x = mx;
		    data->click_y = my;
		    
		    data->old_lasso_rect = data->lasso_rect;
		    GetAbsoluteLassoRect(data, &old_lasso);
		    data->lasso_rect.MaxX = mx - data->view_rect.MinX + data->view_x;
		    data->lasso_rect.MaxY = my - data->view_rect.MinY + data->view_y;
		    GetAbsoluteLassoRect(data, &new_lasso);
		    
		    region = NewRectRegion(new_lasso.MinX, new_lasso.MinY, new_lasso.MaxX, new_lasso.MaxY);
    	    	    if (region)
		    {
		    	struct Rectangle render_range;
			
		    	XorRectRegion(region, &old_lasso);
			OrRectOutlineRegion(region, &old_lasso);
			OrRectOutlineRegion(region, &new_lasso);
			
			render_range = region->bounds;
			
		    	clip = MUI_AddClipRegion(muiRenderInfo(obj), region);
			
		    	data->update = UPDATE_ALL;
			data->update_rect1 = &render_range;
		    	MUI_Redraw(obj, MADF_DRAWUPDATE);
			data->update_rect1 = 0;
			
			MUI_RemoveClipRegion(muiRenderInfo(obj), clip);
		    }
		    
		    RethinkLasso(obj, data);

		    if ((mx >= data->view_rect.MinX) &&
		        (my >= data->view_rect.MinY) &&
			(mx <= data->view_rect.MaxX) &&
			(my <= data->view_rect.MaxY))
		    {
		    	DisableAutoScrollTimer(obj, data);
		    }
		    else
		    {
		    	EnableAutoScrollTimer(obj, data);
		    }
		    
		}
		
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
    NewList((struct List *)&data->selection_list);

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
    
    SortInNode(data, (struct List *)&data->entries_list, (struct Node *)entry, (APTR)CompareNodes);
    
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

    	case MUIM_TextIconList_AutoScroll: return TextIconList_AutoScroll(cl,obj,(APTR)msg);
    }
    
    return DoSuperMethodA(cl, obj, msg);
}
BOOPSI_DISPATCHER_END

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
	    Child, (IPTR) texticonlist,
	    Child, (IPTR) (vert = ScrollbarObject,
		usewinborder ? MUIA_Prop_UseWinBorder : TAG_IGNORE, MUIV_Prop_UseWinBorder_Right,
		MUIA_Prop_DeltaFactor, 20,
		MUIA_Group_Horiz,      FALSE,
		End),
	    Child, (IPTR) (horiz = ScrollbarObject,
	    	usewinborder?MUIA_Prop_UseWinBorder:TAG_IGNORE, MUIV_Prop_UseWinBorder_Bottom,
		MUIA_Prop_DeltaFactor, 20,
	    	MUIA_Group_Horiz,      TRUE,
	    	End),
	    usewinborder ? TAG_IGNORE : Child, (IPTR) button,
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
	    	    	MUIA_Group_LayoutHook, (IPTR) &data->layout_hook,
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
BOOPSI_DISPATCHER_END


int main(void)
{
    Object *wnd, *texticonlist;
#if 0
    Object *iconlist;
#endif 

    MUIMasterBase = (struct Library*)OpenLibrary("muimaster.library",0);

    CL_TextIconListview = MUI_CreateCustomClass(NULL,MUIC_Group,NULL,sizeof(struct TextIconListview_DATA), TextIconListview_Dispatcher);
    CL_TextIconList = MUI_CreateCustomClass(NULL,MUIC_Area,NULL,sizeof(struct TextIconList_DATA), TextIconList_Dispatcher);

    app = ApplicationObject,
   	SubWindow, (IPTR) (wnd = WindowObject,
    	    MUIA_Window_Title,    (IPTR) "texticonlist",
	    MUIA_Window_Activate,        TRUE,

    	    WindowContents, (IPTR) VGroup,
	    	MUIA_Background, MUII_GroupBack,
#if 0
		Child, (IPTR) IconListviewObject,
		    MUIA_IconListview_IconList, (IPTR) (iconlist = IconDrawerListObject,
    	    	    	InputListFrame,
			MUIA_IconDrawerList_Drawer, (IPTR) "C:",
		    	End),
		    End,
#endif
		Child, (IPTR) TextIconListviewObject,
    	    	    // MUIA_TextIconListview_UseWinBorder, TRUE,
		    MUIA_TextIconListview_TextIconList, (IPTR) (texticonlist = TextIconListObject,
    	    	    	InputListFrame,
			InnerSpacing(0,0),
		    	End),
		    End,

/*
    	    	Child, (IPTR) (texticonlist = TextIconListObject, InputListFrame, End),
*/
		End,
	    End),
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
			    DoMethod(texticonlist, MUIM_TextIconList_Add, (IPTR) fib);
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

