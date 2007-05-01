
/*
Copyright  2002-2007, The AROS Development Team. All rights reserved.
$Id$
*/

//#define MYDEBUG
#include "debug.h"

//#define DEBUG_ILC_EVENTS
//#define DEBUG_ILC_KEYEVENTS
//#define DEBUG_ILC_ICONRENDERING
//#define DEBUG_ILC_ICONSORTING
//#define DEBUG_ILC_ICONSORTING_DUMP
//#define DEBUG_ILC_ICONPOSITIONING
//#define DEBUG_ILC_LASSO

//#define CREATE_FULL_DRAGIMAGE

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include <dos/dos.h>
#include <dos/datetime.h>

#include <exec/memory.h>
#include <graphics/gfx.h>
#include <graphics/view.h>
#include <graphics/rpattr.h>
#include <workbench/icon.h>
#include <workbench/workbench.h>
#include <devices/rawkeycodes.h>
#include <clib/alib_protos.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/utility.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/icon.h>
#include <proto/layers.h>
#include <proto/muimaster.h>
#include <proto/dos.h>
#include <proto/iffparse.h>
#include <prefs/prefhdr.h>
#include <prefs/wanderer.h>
#include <proto/cybergraphics.h>

#include "mui.h"
#include "muimaster_intern.h"
#include "support.h"
#include "imspec.h"
#include "iconlist_attributes.h"
#include "iconlist.h"

extern struct Library *MUIMasterBase;

#ifndef NO_ICON_POSITION
#define NO_ICON_POSITION                               (0x8000000) /* belongs to workbench/workbench.h */
#endif

#define UPDATE_SINGLEICON                              1
#define UPDATE_SCROLL                                  2
#define UPDATE_SORT                                    3
#define UPDATE_RESCALE                                 4

#define LEFT_BUTTON	                                   1
#define RIGHT_BUTTON                                   2
#define MIDDLE_BUTTON                                  4

#define ICON_LISTMODE_GRID                             0
#define ICON_LISTMODE_ROUGH                            1

#define ICON_TEXTMODE_OUTLINE                          0
#define ICON_TEXTMODE_PLAIN                            1
#define ICON_TEXTMODE_DROPSHADOW                       2

#define ICONLIST_DRAWMODE_NORMAL                       1
#define ICONLIST_DRAWMODE_FAST                         2

/* Default Icon rendering settings */
// Spacing between icons ..
#define ILC_ICON_HORIZONTALMARGIN_DEFAULT              5
#define ILC_ICON_VERTICALMARGIN_DEFAULT                5

/* Default Icon label rendering settings */
// Max no of characters to display on a single line
#define ILC_ICONLABEL_MAXLINELEN_DEFAULT               15

// Space between icon image and icons label bounding box
#define ILC_ICONLABEL_IMAGEMARGIN_DEFAULT              1
// Spacing between label text lines
#define ILC_ICONLABEL_HORIZONTALTEXTMARGIN_DEFAULT     4
#define ILC_ICONLABEL_VERTICALTEXTMARGIN_DEFAULT       4

// Icon Label's border dimensions
#define ILC_ICONLABEL_BORDERWIDTH_DEFAULT              1
#define ILC_ICONLABEL_BORDERHEIGHT_DEFAULT             1

struct MUI_IconData
{
    APTR                          icld_Pool;                          /* Pool to allocate data from */

	struct RastPort               *icld_BufferRastPort;
    struct TextFont               *icld_IconLabelFont;
    struct TextFont               *icld_IconInfoFont;

    struct List                   icld_IconList;                      /* IconEntry */

    LONG                          icld_ViewX,                         /* the leftmost/upper coordinates of the view */
	                              icld_ViewY;
    ULONG                         icld_ViewWidth,                     /* dimensions of the view (_mwidth(obj) and _mheight(obj)) */
	                              icld_ViewHeight,
                                  icld_AreaWidth,                              /* The whole width/height */
	                              icld_AreaHeight;


	/* Selection & Drag/Drop Info .. */
    struct IconEntry              *icld_SelectionFirst;               /* the icon which has been selected first or NULL */
    struct IconEntry              *icld_SelectionLast;
    struct IconEntry              *icld_FocusIcon;

    struct IconList_Drop          drop_entry;                         /* the icon where the icons have been dropped */
    struct IconList_Click         icon_click;

    /* Input / Event Information */
    struct MUI_EventHandlerNode   ehn;

    LONG                          touch_x;
    LONG                          touch_y;

    LONG                          click_x;
    LONG                          click_y;

    ULONG                         last_secs;                          /* DoubleClick stuff */
    ULONG                         last_mics;

    /* RENDERING DATA! ###### */
    ULONG                         icld_DisplayFlags;                  /* Internal Sorting related stuff */
    ULONG                         icld_SortFlags;
    ULONG                         icld_IconLargestWidth;
    ULONG                         icld_IconLargestHeight;

    /* values for icld_UpdateMode - :

       UPDATE_SINGLEICON = draw the given single icon only
       UPDATE_SCROLL     = scroll the view by update_scrolldx/update_scrolldy
       UPDATE_RESCALE    = rescaling window                                   */

    ULONG                         icld_UpdateMode;
    WORD                          update_scrolldx;
    WORD                          update_scrolldy;
    WORD                          update_oldwidth;
    WORD                          update_oldheight;
    
    struct IconEntry              *update_icon;
    struct Rectangle              *update_rect1;
    struct Rectangle              *update_rect2;
    struct Rectangle              view_rect;
    
    struct Rectangle              icld_LassoRectangle;                /* lasso data */
    BOOL                          icld_LassoActive;

#warning "TODO: move config options to a seperate struct"
	/* IconList configuration settings ... */
	ULONG                         icld_LabelPen;		
	ULONG                         icld_LabelShadowPen;
	ULONG                         icld_InfoPen;
	ULONG                         icld_InfoShadowPen;

    ULONG                         icld__Option_IconTextMaxLen;                   /* max no. of chars to display in a line */
    UBYTE                         icld__Option_IconListMode;                     /* */
    UBYTE                         icld__Option_IconTextMode;                     /* */
	BOOL                          icld__Option_IconListFixedBackground;          /* */
	BOOL                          icld__Option_IconListScaledBackground;         /* */
    ULONG                         icld__Option_LabelTextMultiLine;               /* */
    BOOL                          icld__Option_LabelTextMultiLineOnFocus;        /* */
    UBYTE                         icld__Option_IconHorizontalSpacing;            /* */
    UBYTE                         icld__Option_IconVerticalSpacing;              /* */
    UBYTE                         icld__Option_IconImageSpacing;                 /* */
    UBYTE                         icld__Option_LabelTextHorizontalSpacing;       /* */
    UBYTE                         icld__Option_LabelTextVerticalSpacing;         /* */
    UBYTE                         icld__Option_LabelTextBorderWidth;             /* */
    UBYTE                         icld__Option_LabelTextBorderHeight;            /* */
	
    UBYTE                         mouse_pressed;
};

/**************************************************************************

**************************************************************************/

#define ForeachNodeReversed(list, node)                    \
for                                                        \
(                                                          \
    node = (void *)(((struct List *)(list))->lh_TailPred); \
    ((struct Node *)(node))->ln_Pred;                      \
    node = (void *)(((struct Node *)(node))->ln_Pred)      \
)

#ifdef AndRectRect
/* Fine */
#else
#error "Implement AndRectRect (rom/graphics/andrectrect.c)"
#endif

int RectAndRect(struct Rectangle *a, struct Rectangle *b)
{
    if ((a->MinX > b->MaxX) || (a->MinY > b->MaxY) || (a->MaxX < b->MinX) || (a->MaxY < b->MinY))
        return 0;
    return 1;
}

struct IconEntry *Node_NextVisible(struct IconEntry *current_Node)
{
	current_Node = (struct IconEntry *)GetSucc(current_Node);
	while ((current_Node != NULL) && (!(current_Node->ile_Flags & ICONENTRY_FLAG_VISIBLE)))
	{
		current_Node = (struct IconEntry *)GetSucc(current_Node);
	}
	return current_Node;
}

struct IconEntry *Node_FirstVisible(struct List *icon_list)
{
	struct IconEntry *current_Node = (struct IconEntry *)GetHead(icon_list);

	if ((current_Node != NULL) && !(current_Node->ile_Flags & ICONENTRY_FLAG_VISIBLE))
		current_Node = Node_NextVisible(current_Node);

	return current_Node;
}

struct IconEntry *Node_PreviousVisible(struct IconEntry *current_Node)
{
	current_Node = (struct IconEntry *)GetPred(current_Node);
	while ((current_Node != NULL) && (!(current_Node->ile_Flags & ICONENTRY_FLAG_VISIBLE)))
	{
		current_Node = (struct IconEntry *)GetPred(current_Node);
	}
	return current_Node;
}

struct IconEntry *Node_LastVisible(struct List *icon_list)
{
	struct IconEntry *current_Node = (struct IconEntry *)GetTail(icon_list);

	if ((current_Node != NULL) && !(current_Node->ile_Flags & ICONENTRY_FLAG_VISIBLE))
		current_Node = Node_PreviousVisible(current_Node);

	return current_Node;
}
/**************************************************************************
 get positive lasso coords
**************************************************************************/
static void GetAbsoluteLassoRect(struct MUI_IconData *data, struct Rectangle *LassoRectangle)
{
    WORD minx = data->icld_LassoRectangle.MinX;
    WORD miny = data->icld_LassoRectangle.MinY;
    WORD maxx = data->icld_LassoRectangle.MaxX;
    WORD maxy = data->icld_LassoRectangle.MaxY;

#if defined(DEBUG_ILC_LASSO)
D(bug("[IconList] GetAbsoluteLassoRect()\n"));
#endif

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
    
    LassoRectangle->MinX = data->view_rect.MinX - data->icld_ViewX + minx;
    LassoRectangle->MinY = data->view_rect.MinY - data->icld_ViewY + miny;
    LassoRectangle->MaxX = data->view_rect.MinX - data->icld_ViewX + maxx;
    LassoRectangle->MaxY = data->view_rect.MinY - data->icld_ViewY + maxy;
}

static void IconList_InvertPixelRect(struct RastPort *rp, WORD minx, WORD miny, WORD maxx, WORD maxy, struct Rectangle *clip)
{
	struct Rectangle r, clipped_r;
	
#if defined(DEBUG_ILC_RENDERING)
D(bug("[IconList] IconList_InvertPixelRect()\n"));
#endif

    if (maxx < minx)
    {
    	/* Swap minx, maxx */
    	minx ^= maxx;
	    maxx ^= minx;
	    minx ^= maxx;
    }
    
    if (maxy < miny)
    {
    	/* Swap miny, maxy */
    	miny ^= maxy;
	    maxy ^= miny;
	    miny ^= maxy;
    }
    
	r.MinX = minx;
	r.MinY = miny;
	r.MaxX = maxx;
	r.MaxY = maxy;
	
	if (AndRectRect(&r, clip, &clipped_r))
	{	
    	InvertPixelArray(rp, clipped_r.MinX, clipped_r.MinY,
						 clipped_r.MaxX - clipped_r.MinX + 1, clipped_r.MaxY - clipped_r.MinY + 1);
	}
}

/**************************************************************************
 Simple lasso drawing by inverting area outlines
**************************************************************************/
void IconList_InvertLassoOutlines(Object *obj, struct Rectangle *rect)
{
    struct Rectangle lasso;
	struct Rectangle clip;
	
#if defined(DEBUG_ILC_LASSO)
D(bug("[IconList] IconList_InvertLassoOutlines()\n"));
#endif

    /* get abolute iconlist coords */
    lasso.MinX = rect->MinX + _mleft(obj);
    lasso.MaxX = rect->MaxX + _mleft(obj);
    lasso.MinY = rect->MinY + _mtop(obj);
    lasso.MaxY = rect->MaxY + _mtop(obj);
  
    clip.MinX = _mleft(obj);
	clip.MinY = _mtop(obj);
	clip.MaxX = _mright(obj);
	clip.MaxY = _mbottom(obj);
	
    /* horizontal lasso lines */
    IconList_InvertPixelRect(_rp(obj), lasso.MinX, lasso.MinY, lasso.MaxX-1, lasso.MinY + 1, &clip);
	IconList_InvertPixelRect(_rp(obj), lasso.MinX, lasso.MaxY, lasso.MaxX-1, lasso.MaxY + 1, &clip);

    /* vertical lasso lines */
    IconList_InvertPixelRect(_rp(obj), lasso.MinX, lasso.MinY, lasso.MinX + 1, lasso.MaxY - 1, &clip);
    IconList_InvertPixelRect(_rp(obj), lasso.MaxX, lasso.MinY, lasso.MaxX + 1, lasso.MaxY - 1, &clip);
} 

/**************************************************************************
As we don't use the label drawing of icon.library we also have to do
this by hand
**************************************************************************/
static void IconList_GetIconImageRectangle(Object *obj, struct MUI_IconData *data, struct IconEntry *icon, struct Rectangle *rect)
{
#if defined(DEBUG_ILC_ICONPOSITIONING)
D(bug("[IconList] IconList_GetIconImageRectangle()\n"));
#endif
    /* Get basic width/height */    
    GetIconRectangleA(NULL, icon->ile_DiskObj, NULL, rect, NULL);
}

static void IconList_GetIconLabelRectangle(Object *obj, struct MUI_IconData *data, struct IconEntry *icon, struct Rectangle *rect)
{
	ULONG      outline_offset = 0;
	ULONG 	   textwidth = 0;

#if defined(DEBUG_ILC_ICONPOSITIONING)
D(bug("[IconList] IconList_GetIconLabelRectangle()\n"));
#endif

	switch ( data->icld__Option_IconTextMode )
	{
		case ICON_TEXTMODE_DROPSHADOW:
			outline_offset = 1;
			break;

		case ICON_TEXTMODE_PLAIN:
			break;

		default:
			outline_offset = 2;
			break;
	}
	
    /* Get icon box width including text width */
    if (icon->ile_IconListEntry.label && icon->ile_TxtBuf_DisplayedLabel)
    {
		SetFont(data->icld_BufferRastPort, data->icld_IconLabelFont);
		
		ULONG textlength = strlen(icon->ile_TxtBuf_DisplayedLabel);

		if ( !data->icld__Option_IconTextMaxLen ) data->icld__Option_IconTextMaxLen = ILC_ICONLABEL_MAXLINELEN_DEFAULT;
		if ( textlength > data->icld__Option_IconTextMaxLen ) textlength = data->icld__Option_IconTextMaxLen;
		
		rect->MinX = 0;
		icon->ile_TxtBuf_DisplayedLabelWidth = TextLength(data->icld_BufferRastPort, icon->ile_TxtBuf_DisplayedLabel, textlength) + outline_offset;
		rect->MaxX = icon->ile_TxtBuf_DisplayedLabelWidth + ( data->icld__Option_LabelTextHorizontalSpacing * 2 ) + (data->icld__Option_LabelTextBorderWidth * 2);
	
		rect->MinY = 0;
		
		rect->MaxY = rect->MinY + data->icld_IconLabelFont->tf_YSize + outline_offset + (data->icld__Option_LabelTextVerticalSpacing * 2) + (data->icld__Option_LabelTextBorderHeight * 2);

		/*  Date/size sorting has the date/size appended under the icon label
			only list regular files like this (drawers have no size/date output) */
		if(
			icon->ile_IconListEntry.type != WBDRAWER && 
			((data->icld_SortFlags & ICONLIST_SORT_BY_SIZE) || (data->icld_SortFlags & ICONLIST_SORT_BY_DATE))
		)
		{
			SetFont(data->icld_BufferRastPort, data->icld_IconInfoFont);

			if( (data->icld_SortFlags & ICONLIST_SORT_BY_SIZE) && !(data->icld_SortFlags & ICONLIST_SORT_BY_DATE) )
			{
				icon->ile_TxtBuf_SIZEWidth = TextLength(data->icld_BufferRastPort, icon->ile_TxtBuf_SIZE, strlen(icon->ile_TxtBuf_SIZE));
				textwidth = icon->ile_TxtBuf_SIZEWidth;
			}
			else
			{
				if( !(data->icld_SortFlags & ICONLIST_SORT_BY_SIZE) && (data->icld_SortFlags & ICONLIST_SORT_BY_DATE) )
				{
					if( icon->ile_Flags & ICONENTRY_FLAG_TODAY )
					{
						icon->ile_TxtBuf_TIMEWidth = TextLength(data->icld_BufferRastPort, icon->ile_TxtBuf_TIME, strlen(icon->ile_TxtBuf_TIME));
						textwidth = icon->ile_TxtBuf_TIMEWidth;
					}
					else
					{
						icon->ile_TxtBuf_DATEWidth = TextLength(data->icld_BufferRastPort, icon->ile_TxtBuf_DATE, strlen(icon->ile_TxtBuf_DATE));
						textwidth = icon->ile_TxtBuf_DATEWidth;
					}
				}
			}

			if (textwidth > 0) rect->MaxY += data->icld_IconInfoFont->tf_YSize + outline_offset + data->icld__Option_LabelTextVerticalSpacing;
			
			if ((textwidth + outline_offset + (data->icld__Option_LabelTextHorizontalSpacing * 2) + (data->icld__Option_LabelTextBorderWidth * 2)) > (rect->MaxX - rect->MinX)) rect->MaxX = textwidth + outline_offset + (data->icld__Option_LabelTextVerticalSpacing * 2) + (data->icld__Option_LabelTextBorderWidth * 2);
		}
	}
}

static void IconList_GetIconRectangle(Object *obj, struct MUI_IconData *data, struct IconEntry *icon, struct Rectangle *rect)
{
#if defined(DEBUG_ILC_ICONPOSITIONING)
D(bug("[IconList] IconList_GetIconRectangle()\n"));
#endif
    struct Rectangle labelrect;

	IconList_GetIconImageRectangle(obj, data, icon, rect);
	
    icon->ile_IconWidth  = (rect->MaxX - rect->MinX) + 1;
    icon->ile_IconHeight = (rect->MaxY - rect->MinY) + 1;

    /* Store */
    icon->ile_AreaWidth = icon->ile_IconWidth;
    icon->ile_AreaHeight = icon->ile_IconHeight;
	
    rect->MaxX = (rect->MinX + icon->ile_IconWidth) - 1;
    rect->MaxY = (rect->MinY + icon->ile_IconHeight) - 1;
	
    /* Get icon box width including text width */
    IconList_GetIconLabelRectangle(obj, data, icon, &labelrect);

	if (icon->ile_AreaWidth < ((labelrect.MaxX - labelrect.MinX) + 1))
		icon->ile_AreaWidth = (labelrect.MaxX - labelrect.MinX) + 1;

    icon->ile_AreaHeight += data->icld__Option_IconImageSpacing + ((labelrect.MaxY - labelrect.MinY) + 1);
	
	/* Store */
	rect->MaxX = (rect->MinX + icon->ile_AreaWidth) - 1;
	rect->MaxY = (rect->MinY + icon->ile_AreaHeight) - 1;
}

/**************************************************************************
Draw the icon at its position
**************************************************************************/

IPTR IconList__MUIM_IconList_DrawEntry(struct IClass *CLASS, Object *obj, struct MUIP_IconList_DrawEntry *message)
{
    struct MUI_IconData *data = INST_DATA(CLASS, obj);

    struct Rectangle iconrect;
    struct Rectangle objrect;

    //LONG tx,ty;
	LONG offsetx,offsety;
    //LONG txwidth, txheight;

#if defined(DEBUG_ILC_ICONRENDERING)
D(bug("[IconList] IconList__MUIM_IconList_DrawEntry(message->icon @ %x)\n", message->icon));
#endif

	if ((!(message->icon->ile_Flags & ICONENTRY_FLAG_VISIBLE)) ||
		(!(message->icon->ile_DiskObj)))
	{
#if defined(DEBUG_ILC_ICONRENDERING)
D(bug("[IconList] IconList__MUIM_IconList_DrawEntry: Not visible or missing DOB\n"));
#endif
		return FALSE;
	}
	
    /* Get the dimensions and affected area of message->icon */
    IconList_GetIconImageRectangle(obj, data, message->icon, &iconrect);

    /* Add the relative position offset of the message->icon */
    offsetx = _mleft(obj) - data->icld_ViewX + message->icon->ile_IconX;
	/* Centre our image with our text */
	if (message->icon->ile_IconWidth < message->icon->ile_AreaWidth)
		offsetx += (message->icon->ile_AreaWidth - message->icon->ile_IconWidth)/2;

    iconrect.MinX += offsetx;
    iconrect.MaxX += offsetx;

    offsety = _mtop(obj) - data->icld_ViewY + message->icon->ile_IconY;
    iconrect.MinY += offsety;
    iconrect.MaxY += offsety;

    /* Add the relative position of the window */
    objrect.MinX = _mleft(obj);
    objrect.MinY = _mtop(obj);
    objrect.MaxX = _mright(obj);
    objrect.MaxY = _mbottom(obj);

    if (!RectAndRect(&iconrect, &objrect)) return FALSE;

    /* data->update_rect1 and data->update_rect2 may
       point to rectangles to indicate that only icons
       in any of this rectangles need to be drawn      */
    
    if (data->update_rect1 && data->update_rect2)
    {
        if (!RectAndRect(&iconrect, data->update_rect1) &&
        !RectAndRect(&iconrect, data->update_rect2)) return FALSE;
    }
    else if (data->update_rect1)
    {
        if (!RectAndRect(&iconrect, data->update_rect1)) return FALSE;
    }
    else if (data->update_rect2)
    {
        if (!RectAndRect(&iconrect, data->update_rect2)) return FALSE;
    }
    
    if (message->drawmode == ICONENTRY_DRAWMODE_NONE) return TRUE;
    
    if (data->icld_BufferRastPort == NULL)
    {
    	data->icld_BufferRastPort = _rp(obj);
    }
    
	// Center icon image
    ULONG iconX = iconrect.MinX;
    ULONG iconY = iconrect.MinY;

#if !defined(__AROS__)
    DrawIconState(
#else
    DrawIconStateA(
#endif
        data->icld_BufferRastPort ? data->icld_BufferRastPort : data->icld_BufferRastPort, message->icon->ile_DiskObj, NULL,
        iconX, 
        iconY, 
        (message->icon->ile_Flags & ICONENTRY_FLAG_SELECTED) ? IDS_SELECTED : IDS_NORMAL, 
#if !defined(__AROS__)
        ICONDRAWA_EraseBackground, FALSE, 
#endif
        TAG_DONE
    );

    return TRUE;
}

IPTR IconList__MUIM_IconList_DrawEntryLabel(struct IClass *CLASS, Object *obj, struct MUIP_IconList_DrawEntry *message)
{
    struct MUI_IconData *data = INST_DATA(CLASS, obj);

    STRPTR     			buf = NULL;

    struct Rectangle 	iconlabelrect;
    struct Rectangle 	objrect;

	ULONG               txtbox_width = 0;
    LONG 				tx,ty,offsetx,offsety;
    LONG 				txwidth; // txheight;

#if defined(DEBUG_ILC_ICONRENDERING)
D(bug("[IconList] IconList__MUIM_IconList_DrawEntryLabel(message->icon @ %x)\n", message->icon));
#endif

	if ((!(message->icon->ile_Flags & ICONENTRY_FLAG_VISIBLE)) ||
		(!(message->icon->ile_DiskObj)))
	{
#if defined(DEBUG_ILC_ICONRENDERING)
D(bug("[IconList] IconList__MUIM_IconList_DrawEntryLabel: Not visible or missing DOB\n"));
#endif
		return FALSE;
	}
	
    /* Get the dimensions and affected area of message->icon's label */
    IconList_GetIconLabelRectangle(obj, data, message->icon, &iconlabelrect);

    /* Add the relative position offset of the message->icon's label */
    offsetx = (_mleft(obj) - data->icld_ViewX) + message->icon->ile_IconX;
	txtbox_width = (iconlabelrect.MaxX - iconlabelrect.MinX) + 1;

	if (txtbox_width < message->icon->ile_AreaWidth)
		offsetx += ((message->icon->ile_AreaWidth - txtbox_width)/2);

    iconlabelrect.MinX += offsetx;
    iconlabelrect.MaxX += offsetx;

    offsety = (_mtop(obj) - data->icld_ViewY) + message->icon->ile_IconY + message->icon->ile_IconHeight + data->icld__Option_IconImageSpacing;
    iconlabelrect.MinY += offsety;
    iconlabelrect.MaxY += offsety;

    /* Add the relative position of the window */
    objrect.MinX = _mleft(obj);
    objrect.MinY = _mtop(obj);
    objrect.MaxX = _mright(obj);
    objrect.MaxY = _mbottom(obj);

    if (!RectAndRect(&iconlabelrect, &objrect)) return FALSE;

    /* data->update_rect1 and data->update_rect2 may
       point to rectangles to indicate that only icons
       in any of this rectangles need to be drawn      */
    
    if (data->update_rect1 && data->update_rect2)
    {
        if (!RectAndRect(&iconlabelrect, data->update_rect1) &&
        !RectAndRect(&iconlabelrect, data->update_rect2)) return FALSE;
    }
    else if (data->update_rect1)
    {
        if (!RectAndRect(&iconlabelrect, data->update_rect1)) return FALSE;
    }
    else if (data->update_rect2)
    {
        if (!RectAndRect(&iconlabelrect, data->update_rect2)) return FALSE;
    }
    
    if (message->drawmode == ICONENTRY_DRAWMODE_NONE) return TRUE;
    
    if (data->icld_BufferRastPort == NULL)
    {
    	data->icld_BufferRastPort = _rp(obj);
    }
    
    SetABPenDrMd(data->icld_BufferRastPort, _pens(obj)[MPEN_TEXT], 0, JAM1);

    // Center message->icon's label
	ULONG labelX = iconlabelrect.MinX + data->icld__Option_LabelTextBorderWidth + data->icld__Option_LabelTextHorizontalSpacing;
    ULONG labelY = iconlabelrect.MinY + data->icld__Option_LabelTextBorderHeight + data->icld__Option_LabelTextVerticalSpacing;

	ULONG txtarea_width = txtbox_width - ((data->icld__Option_LabelTextBorderWidth + data->icld__Option_LabelTextHorizontalSpacing) * 2);

    if (message->icon->ile_IconListEntry.label && message->icon->ile_TxtBuf_DisplayedLabel)
    {
        ULONG nameLength = strlen(message->icon->ile_TxtBuf_DisplayedLabel);

        tx = labelX;

		if (message->icon->ile_TxtBuf_DisplayedLabelWidth < txtarea_width)
			tx += ((txtarea_width - message->icon->ile_TxtBuf_DisplayedLabelWidth)/2);

        ty = labelY + data->icld_IconLabelFont->tf_Baseline;

		if ((message->icon->ile_Flags & ICONENTRY_FLAG_FOCUS) && ((BOOL)XGET(_win(obj), MUIA_Window_Activate)))
		{
			//Draw the focus box around the selected label ..
			InvertPixelArray(data->icld_BufferRastPort,
						iconlabelrect.MinX, iconlabelrect.MinY,
						(iconlabelrect.MaxX - iconlabelrect.MinX) + 1, data->icld__Option_LabelTextBorderHeight);
			InvertPixelArray(data->icld_BufferRastPort,
						iconlabelrect.MaxX, iconlabelrect.MinY,
						data->icld__Option_LabelTextBorderWidth, (iconlabelrect.MaxY - iconlabelrect.MinY) + 1);
			InvertPixelArray(data->icld_BufferRastPort,
						iconlabelrect.MinX, iconlabelrect.MaxY,
						(iconlabelrect.MaxX - iconlabelrect.MinX) + 1, data->icld__Option_LabelTextBorderHeight);
			InvertPixelArray(data->icld_BufferRastPort,
						iconlabelrect.MinX, iconlabelrect.MinY,
						data->icld__Option_LabelTextBorderWidth, (iconlabelrect.MaxY - iconlabelrect.MinY) + 1);
		}

		SetFont(data->icld_BufferRastPort, data->icld_IconLabelFont);

        switch ( data->icld__Option_IconTextMode )
        {
            case ICON_TEXTMODE_DROPSHADOW:
                SetAPen(data->icld_BufferRastPort, data->icld_LabelShadowPen);
                Move(data->icld_BufferRastPort, tx + 1, ty + 1); 
                Text(data->icld_BufferRastPort, message->icon->ile_TxtBuf_DisplayedLabel, nameLength);
            case ICON_TEXTMODE_PLAIN:
                SetAPen(data->icld_BufferRastPort, data->icld_LabelPen);
                Move(data->icld_BufferRastPort, tx, ty); 
                Text(data->icld_BufferRastPort, message->icon->ile_TxtBuf_DisplayedLabel, nameLength);
                break;
                
            default:
                // Outline mode:
                
                SetSoftStyle(data->icld_BufferRastPort, FSF_BOLD, AskSoftStyle(data->icld_BufferRastPort));
                SetAPen(data->icld_BufferRastPort, data->icld_LabelShadowPen);

				Move(data->icld_BufferRastPort, tx + 1, ty ); Text(data->icld_BufferRastPort, message->icon->ile_TxtBuf_DisplayedLabel, nameLength);
				Move(data->icld_BufferRastPort, tx + 2, ty ); Text(data->icld_BufferRastPort, message->icon->ile_TxtBuf_DisplayedLabel, nameLength);
				Move(data->icld_BufferRastPort, tx , ty ); Text(data->icld_BufferRastPort, message->icon->ile_TxtBuf_DisplayedLabel, nameLength);
				Move(data->icld_BufferRastPort, tx, ty + 1);  Text(data->icld_BufferRastPort, message->icon->ile_TxtBuf_DisplayedLabel, nameLength);
				Move(data->icld_BufferRastPort, tx, ty + 2);  Text(data->icld_BufferRastPort, message->icon->ile_TxtBuf_DisplayedLabel, nameLength);
				Move(data->icld_BufferRastPort, tx + 1, ty + 2);  Text(data->icld_BufferRastPort, message->icon->ile_TxtBuf_DisplayedLabel, nameLength);
				Move(data->icld_BufferRastPort, tx + 2, ty + 1);  Text(data->icld_BufferRastPort, message->icon->ile_TxtBuf_DisplayedLabel, nameLength);
				Move(data->icld_BufferRastPort, tx + 2, ty + 2);  Text(data->icld_BufferRastPort, message->icon->ile_TxtBuf_DisplayedLabel, nameLength);

                SetAPen(data->icld_BufferRastPort, data->icld_LabelPen);
                Move(data->icld_BufferRastPort, tx + 1, ty + 1);
                Text(data->icld_BufferRastPort, message->icon->ile_TxtBuf_DisplayedLabel, nameLength);
                SetSoftStyle(data->icld_BufferRastPort, FS_NORMAL, AskSoftStyle(data->icld_BufferRastPort));
                break;
        }

        /*date/size sorting has the date/size appended under the message->icon label*/

        if( message->icon->ile_IconListEntry.type != WBDRAWER && ((data->icld_SortFlags & ICONLIST_SORT_BY_SIZE) || (data->icld_SortFlags & ICONLIST_SORT_BY_DATE)) )
        {
			buf = NULL;
			SetFont(data->icld_BufferRastPort, data->icld_IconInfoFont);

			if( (data->icld_SortFlags & ICONLIST_SORT_BY_SIZE) && !(data->icld_SortFlags & ICONLIST_SORT_BY_DATE) )
			{
				buf = message->icon->ile_TxtBuf_SIZE;
				txwidth = message->icon->ile_TxtBuf_SIZEWidth;
			}
			else
			{
				if( !(data->icld_SortFlags & ICONLIST_SORT_BY_SIZE) && (data->icld_SortFlags & ICONLIST_SORT_BY_DATE) )
				{
					if( message->icon->ile_Flags & ICONENTRY_FLAG_TODAY )
					{
						buf  = message->icon->ile_TxtBuf_TIME;
						txwidth = message->icon->ile_TxtBuf_TIMEWidth;
					}
					else
					{
						buf = message->icon->ile_TxtBuf_DATE;
						txwidth = message->icon->ile_TxtBuf_DATEWidth;
					}
				}
			}

			if (buf)
			{
				nameLength = strlen(buf);
				tx = labelX;

				if (txwidth < txtarea_width)
					tx += ((txtarea_width - txwidth)/2);

				ty = labelY + ((data->icld__Option_LabelTextVerticalSpacing + data->icld_IconLabelFont->tf_YSize ) * 1) + data->icld_IconInfoFont->tf_Baseline;

				switch ( data->icld__Option_IconTextMode )
				{
					case ICON_TEXTMODE_DROPSHADOW:
						SetAPen(data->icld_BufferRastPort, data->icld_InfoShadowPen);
						Move(data->icld_BufferRastPort, tx + 1, ty + 1); Text(data->icld_BufferRastPort, buf, nameLength);
					case ICON_TEXTMODE_PLAIN:
						SetAPen(data->icld_BufferRastPort, data->icld_InfoPen);
						Move(data->icld_BufferRastPort, tx, ty); Text(data->icld_BufferRastPort, buf, nameLength);
						break;
						
					default:
						// Outline mode..
						SetSoftStyle(data->icld_BufferRastPort, FSF_BOLD, AskSoftStyle(data->icld_BufferRastPort));
						SetAPen(data->icld_BufferRastPort, data->icld_InfoShadowPen);

						Move(data->icld_BufferRastPort, tx + 1, ty ); Text(data->icld_BufferRastPort, buf, nameLength);
						Move(data->icld_BufferRastPort, tx + 2, ty ); Text(data->icld_BufferRastPort, buf, nameLength);
						Move(data->icld_BufferRastPort, tx , ty ); Text(data->icld_BufferRastPort, buf, nameLength);
						Move(data->icld_BufferRastPort, tx, ty + 1);  Text(data->icld_BufferRastPort, buf, nameLength);
						Move(data->icld_BufferRastPort, tx, ty + 2);  Text(data->icld_BufferRastPort, buf, nameLength);
						Move(data->icld_BufferRastPort, tx + 1, ty + 2);  Text(data->icld_BufferRastPort, buf, nameLength);
						Move(data->icld_BufferRastPort, tx + 2, ty + 1);  Text(data->icld_BufferRastPort, buf, nameLength);
						Move(data->icld_BufferRastPort, tx + 2, ty + 2);  Text(data->icld_BufferRastPort, buf, nameLength);
			
						SetAPen(data->icld_BufferRastPort, data->icld_InfoPen);
						Move(data->icld_BufferRastPort, tx + 1, ty + 1);
						Text(data->icld_BufferRastPort, buf, nameLength);
						SetSoftStyle(data->icld_BufferRastPort, FS_NORMAL, AskSoftStyle(data->icld_BufferRastPort));
						break;
				}
			}
        }
    }
    
    return TRUE;
}

/**************************************************************************

**************************************************************************/
IPTR IconList__MUIM_IconList_RethinkDimensions(struct IClass *CLASS, Object *obj, struct MUIP_IconList_RethinkDimensions *message)
{
    struct MUI_IconData *data = INST_DATA(CLASS, obj);

    struct IconEntry  *icon = NULL;
    LONG              maxx = 0,
	                  maxy = 0;

#if defined(DEBUG_ILC_ICONPOSITIONING)
D(bug("[IconList] IconList_RethinkDimensions()\n"));
#endif

    if (!(_flags(obj) & MADF_SETUP)) return FALSE;

    if (message->singleicon)
	{
		icon = message->singleicon;
        maxx = data->icld_AreaWidth - 1,
        maxy = data->icld_AreaHeight - 1;
	}
	else icon = (struct IconEntry *)GetHead(&data->icld_IconList);
    
    while (icon)
    {
        if (icon->ile_DiskObj &&
			(icon->ile_Flags & ICONENTRY_FLAG_VISIBLE) &&
            (icon->ile_IconX != NO_ICON_POSITION) &&
            (icon->ile_IconY != NO_ICON_POSITION))
        {
            struct Rectangle icon_rect;

            IconList_GetIconRectangle(obj, data, icon, &icon_rect);
    
            icon_rect.MinX += icon->ile_IconX;
            icon_rect.MaxX += icon->ile_IconX;
            icon_rect.MinY += icon->ile_IconY;
            icon_rect.MaxY += icon->ile_IconY;
    
            if (icon_rect.MaxX > maxx) maxx = icon_rect.MaxX;
            if (icon_rect.MaxY > maxy) maxy = icon_rect.MaxY;
        }

        if (message->singleicon) break;
    
        icon = (struct IconEntry *)GetSucc(icon);
    }

    /* update our view when max x/y have changed */
    if (maxx + 1 != data->icld_AreaWidth)
    {
        data->icld_AreaWidth = maxx + 1;
        SET(obj, MUIA_IconList_Width, data->icld_AreaWidth);
    }
    if (maxy + 1 != data->icld_AreaHeight)
    {
        data->icld_AreaHeight = maxy + 1;
        SET(obj, MUIA_IconList_Height, data->icld_AreaHeight);
    }

	return TRUE;
}

/**************************************************************************
Checks weather we can place a icon with the given dimesions at the
suggested positions.

atx and aty are absolute positions
**************************************************************************/
/*
static int IconList_CouldPlaceIcon(Object *obj, struct MUI_IconData *data, struct IconEntry *toplace, int atx, int aty)
{
	struct IconEntry *icon;
	struct Rectangle toplace_rect;

	IconList_GetIconRectangle(obj, toplace, &toplace_rect);
	toplace_rect.MinX += atx;
	toplace_rect.MaxX += atx;
	toplace_rect.MinY += aty;
	toplace_rect.MaxY += aty;

	icon = GetHead(&data->icld_IconList);
	while (icon)
	{
		if (icon->ile_DiskObj && icon->ile_IconX != NO_ICON_POSITION && icon->ile_IconY != NO_ICON_POSITION)
		{
			struct Rectangle icon_rect;
			IconList_GetIconRectangle(obj, icon, &icon_rect);
			icon_rect.MinX += icon->ile_IconX;
			icon_rect.MaxX += icon->ile_IconX;
			icon_rect.MinY += icon->ile_IconY;
			icon_rect.MaxY += icon->ile_IconY;

			if (RectAndRect(&icon_rect, &toplace_rect))
			return FALSE; *//* There is already an icon on this place *//*
		}
		icon = GetSucc(icon);
	}
	return 1;
}
*/

/**************************************************************************
Place the icon at atx and aty.

atx and aty are absolute positions
**************************************************************************/
/*
static void IconList_PlaceIcon(Object *obj, struct MUI_IconData *data, struct IconEntry *toplace, int atx, int aty)
{
#if 0
	struct Rectangle toplace_rect;

	IconList_GetIconRectangle(obj, toplace, &toplace_rect);
	toplace_rect.MinX += atx + data->icld_ViewX;
	toplace_rect.MaxX += atx + data->icld_ViewX;
	toplace_rect.MinY += aty + data->icld_ViewY;
	toplace_rect.MaxY += aty + data->icld_ViewY;
#endif
	toplace->x = atx;
	toplace->y = aty;
#if 0
	*//* update our view *//*
	if (toplace_rect.MaxX - data->icld_ViewX > data->icld_AreaWidth)
	{
		data->icld_AreaWidth = toplace_rect.MaxX - data->icld_ViewX;
		SET(obj, MUIA_IconList_Width, data->icld_AreaWidth);
	}

	if (toplace_rect.MaxY - data->icld_ViewY > data->icld_AreaHeight)
	{
		data->icld_AreaHeight = toplace_rect.MaxY - data->icld_ViewY;
		SET(obj, MUIA_IconList_Height, data->icld_AreaHeight);
	}
#endif
}
*/
/**************************************************************************
MUIM_PositionIcons - Place icons with NO_ICON_POSITION coords somewhere
**************************************************************************/

IPTR IconList__MUIM_IconList_PositionIcons(struct IClass *CLASS, Object *obj, struct MUIP_IconList_PositionIcons *message)
{
    struct MUI_IconData *data = INST_DATA(CLASS, obj);
    struct IconEntry    *icon = NULL, *pass_first = NULL;
    
    int left = data->icld__Option_IconHorizontalSpacing;
    int top = data->icld__Option_IconVerticalSpacing;
    int cur_x = left;
    int cur_y = top;
    int gridx = 0;
    int gridy = 0;
    int maxw = 0;        // Widest & Talest icon in a column or row.
    int maxh = 0;

#if defined(DEBUG_ILC_ICONPOSITIONING)
D(bug("[IconList] IconList__MUIM_IconList_PositionIcons()\n"));
#endif

	BOOL  next = TRUE;
    
    // Now go to the actual positioning
    icon = (struct IconEntry *)GetHead(&data->icld_IconList);
    while (icon != NULL)
    {
        if ((icon->ile_DiskObj != NULL) && (icon->ile_Flags & ICONENTRY_FLAG_VISIBLE))
        {
            icon->ile_IconX = cur_x;
            icon->ile_IconY = cur_y;
    
            if (data->icld__Option_IconListMode == ICON_LISTMODE_GRID)
            {
				maxw = data->icld_IconLargestWidth + data->icld__Option_IconHorizontalSpacing;
				maxh = data->icld_IconLargestHeight + data->icld__Option_IconVerticalSpacing;
                gridx = maxw;
                gridy = maxh;

                // center icons on grid
				if (icon->ile_AreaWidth < data->icld_IconLargestWidth)
					icon->ile_IconX += ( data->icld_IconLargestWidth - icon->ile_AreaWidth ) / 2;

/*  Dont try to vertically centre yet -:

	# we should work out the LARGEST_ICON LARGEST_ LABEL (seperately)
	# the LARGEST_AREA should be LARGEST_ICON + LARGEST_LABEL
	# with all labels offset at LARGEST_ICON + 1 and the icon's image
    # then centred within LARGEST_ICON */

/*
				if (icon->ile_AreaHeight < maxHeight)
					icon->ile_IconY += ( maxHeight - icon->ile_AreaHeight ) / 2;
*/
            }
            else
            {
                struct Rectangle iconrect;

				if (!(pass_first)) pass_first = icon;

                IconList_GetIconRectangle(obj, data, icon, &iconrect);

				if (icon->ile_AreaWidth < maxw)
					icon->ile_IconX += ( maxw - icon->ile_AreaWidth ) / 2;

                if ((maxw < icon->ile_AreaWidth) || (maxh < icon->ile_AreaHeight))
				{
					if (maxw < icon->ile_AreaWidth) maxw = icon->ile_AreaWidth;
					if (maxh < icon->ile_AreaHeight) maxh = icon->ile_AreaHeight;
					if (pass_first != icon)
					{
						icon = pass_first;
						cur_x = icon->ile_IconX;
						cur_y = icon->ile_IconY;
						continue;
					}
				}

				if (data->icld_DisplayFlags & ICONLIST_DISP_VERTICAL)
				{
					gridx = maxw;
					gridy = icon->ile_AreaHeight + data->icld__Option_IconHorizontalSpacing;
				}
				else
				{
					gridx = icon->ile_AreaWidth + data->icld__Option_IconVerticalSpacing;
					gridy = maxh;
				}
            }
    
            if (data->icld_DisplayFlags & ICONLIST_DISP_VERTICAL)
            {
                cur_y += gridy;
    
                if (cur_y >= data->icld_ViewHeight )
                {
                    next = FALSE;
                    cur_x += maxw;
                    cur_y =  top;
					pass_first = NULL;
					maxw = 0;
                }
            }
            else
            {
                cur_x += gridx;
        
                if (cur_x >= data->icld_ViewWidth )
                {
                    next = FALSE;
                    cur_x =  left;
                    cur_y += maxh;
					pass_first = NULL;
					maxh = 0;
                }
            }
        }
        if ( next ) 
            icon = (struct IconEntry *)GetSucc(icon);

        next = TRUE;
    }
    DoMethod(obj, MUIM_IconList_RethinkDimensions, NULL);
    return 0;
}

/*static void IconList_FixNoPositionIcons(Object *obj, struct MUI_IconData *data)
{
struct IconEntry *icon;
int cur_x = data->icld_ViewX + 36;
int cur_y = data->icld_ViewY + 4;

icon = GetHead(&data->icld_IconList);
while (icon)
{
if (icon->ile_DiskObj && icon->ile_IconX == NO_ICON_POSITION && icon->ile_IconY == NO_ICON_POSITION)
{
int loops = 0;
int cur_x_save = cur_x;
int cur_y_save = cur_y;
struct Rectangle icon_rect;

IconList_GetIconRectangle(obj, icon, &icon_rect);
icon_rect.MinX += cur_x - icon->ile_IconWidth/2 + data->icld_ViewX;
if (icon_rect.MinX < 0)
cur_x -= icon_rect.MinX;

while (!IconList_CouldPlaceIcon(obj, data, icon, cur_x - icon->ile_IconWidth/2, cur_y) && loops < 5000)
{
cur_y++;

if (cur_y + icon->ile_IconHeight > data->icld_ViewX + data->icld_ViewHeight) *//* on both sides -1 *//*
{
    cur_x += 72;
    cur_y = data->icld_ViewY + 4;
}
}

IconList_PlaceIcon(obj, data, icon, cur_x - icon->ile_IconWidth/2, cur_y);

if (icon_rect.MinX < 0)
{
    cur_x = cur_x_save;
    cur_y = cur_y_save;
}
}
icon = GetSucc(icon);
}

DoMethod(obj, MUIM_IconList_RethinkDimensions, NULL);
}
*/
/**************************************************************************
OM_NEW
**************************************************************************/
IPTR IconList__OM_NEW(struct IClass *CLASS, Object *obj, struct opSet *message)
{
    struct MUI_IconData  *data = NULL;
    struct TextFont      *icl_WindowFont = NULL;
	struct RastPort      *icl_RastPort = NULL;

    icl_WindowFont = (struct TextFont *) GetTagData(MUIA_Font, (IPTR) NULL, message->ops_AttrList);
    icl_RastPort = (struct RastPort *) GetTagData(MUIA_IconList_Rastport, (IPTR) NULL, message->ops_AttrList);

    obj = (Object *)DoSuperNewTags(CLASS, obj, NULL,
	    MUIA_FillArea, FALSE,
        MUIA_Dropable, TRUE,
        MUIA_Font, MUIV_Font_Tiny,
        TAG_MORE, (IPTR) message->ops_AttrList);

    if (!obj) return FALSE;

    data = INST_DATA(CLASS, obj);

	data->icld_Pool =  CreatePool(0,4096,4096);
    if (!data->icld_Pool)
    {
        CoerceMethod(CLASS,obj,OM_DISPOSE);
        return (IPTR)NULL;
    }
	
D(bug("[IconList] IconList__OM_NEW: SELF = %x\n", obj));

    data->icld_SelectionLast = NULL;
    
    NewList((struct List*)&data->icld_IconList);

	data->icld_BufferRastPort = icl_RastPort;
	data->icld_IconLabelFont = icl_WindowFont;	

    // Get some initial values
    data->icld__Option_IconListMode   = (UBYTE)GetTagData(MUIA_IconList_IconListMode, 0, message->ops_AttrList);
    data->icld__Option_IconTextMode   = (UBYTE)GetTagData(MUIA_IconList_LabelText_Mode, 0, message->ops_AttrList);
    data->icld__Option_IconTextMaxLen = GetTagData(MUIA_IconList_LabelText_MaxLineLen, 0, message->ops_AttrList);

    if ( data->icld__Option_IconTextMaxLen <= 0 )
        data->icld__Option_IconTextMaxLen = ILC_ICONLABEL_MAXLINELEN_DEFAULT;

    data->ehn.ehn_Events   = IDCMP_MOUSEBUTTONS | IDCMP_RAWKEY;
    data->ehn.ehn_Priority = 0;
    data->ehn.ehn_Flags    = 0;
    data->ehn.ehn_Object   = obj;
    data->ehn.ehn_Class    = CLASS;

    data->icld_SortFlags    = 0;
    data->icld_DisplayFlags = ICONLIST_DISP_SHOWINFO;

    return (IPTR)obj;
}

/**************************************************************************
OM_DISPOSE
**************************************************************************/
IPTR IconList__OM_DISPOSE(struct IClass *CLASS, Object *obj, Msg message)
{
    struct MUI_IconData *data = INST_DATA(CLASS, obj);
    struct IconEntry    *node = NULL;

    ForeachNode(&data->icld_IconList, node)
    {
        if (node->ile_DiskObj) FreeDiskObject(node->ile_DiskObj);
    }

    if (data->icld_Pool) DeletePool(data->icld_Pool);

    DoSuperMethodA(CLASS,obj,message);
    return 0;
}


/**************************************************************************
OM_SET
**************************************************************************/
IPTR IconList__OM_SET(struct IClass *CLASS, Object *obj, struct opSet *message)
{
    struct MUI_IconData *data = INST_DATA(CLASS, obj);
    struct TagItem  	*tag = NULL,
                        *tags = NULL;

    WORD    	    	 oldleft = data->icld_ViewX,
                         oldtop = data->icld_ViewY;
                         //oldwidth = data->icld_ViewWidth,
                         //oldheight = data->icld_ViewHeight;
    
    /* parse initial taglist */
    for (tags = message->ops_AttrList; (tag = NextTagItem((const struct TagItem **)&tags)); )
    {
        switch (tag->ti_Tag)
        {
            case MUIA_IconList_Left:
                if (data->icld_ViewX != tag->ti_Data)
                    data->icld_ViewX = tag->ti_Data;
                break;
    
            case MUIA_IconList_Top:
                if (data->icld_ViewY != tag->ti_Data)
                    data->icld_ViewY = tag->ti_Data;
                break;

            case MUIA_IconList_Rastport:
                data->icld_BufferRastPort = (struct RastPort*)tag->ti_Data;
                break;

            case MUIA_Font:
                data->icld_IconLabelFont = (struct TextFont*)tag->ti_Data;
                break;

			case MUIA_IconList_DisplayFlags:
				data->icld_DisplayFlags = tag->ti_Data;
                break;

			case MUIA_IconList_SortFlags:
				data->icld_SortFlags = tag->ti_Data;
                break;
				
            case MUIA_IconList_IconListMode:
                data->icld__Option_IconListMode = (UBYTE)tag->ti_Data;
                break;
            
            case MUIA_IconList_LabelText_Mode:
                data->icld__Option_IconTextMode = (UBYTE)tag->ti_Data;
                break;
            
            case MUIA_IconList_LabelText_MaxLineLen:
                data->icld__Option_IconTextMaxLen = tag->ti_Data;
                break;

			/* Settings defined by the view class */
			case MUIA_IconListview_FixedBackground:
				data->icld__Option_IconListFixedBackground = (BOOL)tag->ti_Data;
                break;

			case MUIA_IconListview_ScaledBackground:
				data->icld__Option_IconListScaledBackground = (BOOL)tag->ti_Data;
                break;

			/* We also listen for this and manually adjust for known stuff */
			case MUIA_Background:
#if defined(DEBUG_ILC_ICONRENDERING)
D(bug("[IconList] IconList__OM_SET: MUIA_Background\n"));
#endif
			{
				char *bgmode_string = (char *)tag->ti_Data;
				BYTE this_mode = bgmode_string[0] - 48;

#if defined(DEBUG_ILC_ICONRENDERING)
D(bug("[IconList] IconList__OM_SET: MUIA_Background | MUI BG Mode = %d\n", this_mode));
#endif
				switch (this_mode)
				{
				case 0:
					//MUI Pattern
					data->icld__Option_IconListFixedBackground = FALSE;
					data->icld__Option_IconListScaledBackground = FALSE;
					break;
				case 2:
					//MUI RGB color
					data->icld__Option_IconListFixedBackground = FALSE;
					data->icld__Option_IconListScaledBackground = FALSE;
					break;
				case 7:
					//Zune Gradient
					data->icld__Option_IconListFixedBackground = TRUE;
					data->icld__Option_IconListScaledBackground = TRUE;
					break;
				case 5:
					//Image
					data->icld__Option_IconListFixedBackground = FALSE;
					data->icld__Option_IconListScaledBackground = FALSE;
					break;
				}
				return 0;
			}
        }
    }

    if ((oldleft != data->icld_ViewX) || (oldtop != data->icld_ViewY))
    {
        data->icld_UpdateMode = UPDATE_SCROLL;
        data->update_scrolldx = data->icld_ViewX - oldleft;
        data->update_scrolldy = data->icld_ViewY - oldtop;
        MUI_Redraw(obj, MADF_DRAWUPDATE);
    }

    return DoSuperMethodA(CLASS, obj, (Msg)message);
}

/**************************************************************************
OM_GET
**************************************************************************/
IPTR IconList__OM_GET(struct IClass *CLASS, Object *obj, struct opGet *message)
{
    /* small macro to simplify return value storage */
#define STORE *(message->opg_Storage)
    struct MUI_IconData *data = INST_DATA(CLASS, obj);

    switch (message->opg_AttrID)
    {
		case MUIA_IconList_Rastport:                 STORE = (IPTR)data->icld_BufferRastPort; return 1;
        case MUIA_IconList_Left:                     STORE = data->icld_ViewX; return 1;
        case MUIA_IconList_Top:                      STORE = data->icld_ViewY; return 1;
        case MUIA_IconList_Width:                    STORE = data->icld_AreaWidth; return 1;
        case MUIA_IconList_Height:                   STORE = data->icld_AreaHeight; return 1;
        case MUIA_IconList_IconsDropped:             STORE = (IPTR)&data->drop_entry; return 1;
        case MUIA_IconList_Clicked:                  STORE = (ULONG)&data->icon_click; return 1;
        case MUIA_IconList_IconListMode:             STORE = (ULONG)data->icld__Option_IconListMode; return 1;
        case MUIA_IconList_LabelText_Mode:           STORE = (ULONG)data->icld__Option_IconTextMode; return 1;
        case MUIA_IconList_LabelText_MaxLineLen:     STORE = (ULONG)data->icld__Option_IconTextMaxLen; return 1;
        case MUIA_IconList_DisplayFlags:             STORE = data->icld_DisplayFlags; return 1;
        case MUIA_IconList_SortFlags:                STORE = data->icld_SortFlags; return 1;

		case MUIA_IconList_FocusIcon:                STORE = (IPTR)data->icld_FocusIcon; return 1;
		
		/* Settings defined by the view class */
		case MUIA_IconListview_FixedBackground:      STORE = (IPTR)data->icld__Option_IconListFixedBackground; return 1;
		case MUIA_IconListview_ScaledBackground:     STORE = (IPTR)data->icld__Option_IconListScaledBackground; return 1;
    }

    if (DoSuperMethodA(CLASS, obj, (Msg) message)) 
        return 1;
    return 0;
#undef STORE
}

/**************************************************************************
MUIM_Setup
**************************************************************************/
IPTR IconList__MUIM_Setup(struct IClass *CLASS, Object *obj, struct MUIP_Setup *message)
{
    struct MUI_IconData *data = INST_DATA(CLASS, obj);
    struct IconEntry    *node = NULL;

    if (!DoSuperMethodA(CLASS, obj, (Msg) message)) return 0;

    DoMethod(_win(obj), MUIM_Window_AddEventHandler, (IPTR)&data->ehn);

	/* Get Internal Objects to use if not set .. */
	if (data->icld_BufferRastPort == NULL) data->icld_BufferRastPort = _rp(obj);
	if (data->icld_IconLabelFont == NULL)  data->icld_IconLabelFont = _font(obj);
	if (data->icld_IconInfoFont == NULL)   data->icld_IconInfoFont = _font(obj);
D(bug("[IconList] IconList__MUIM_Setup: Use Font @ %x, RastPort @ %x\n", data->icld_IconLabelFont, data->icld_BufferRastPort ));

	/* Set our base options .. */
    data->icld_LabelPen                           = _pens(obj)[MPEN_SHINE];
    data->icld_LabelShadowPen                     = _pens(obj)[MPEN_SHADOW];
    data->icld_InfoPen                            = _pens(obj)[MPEN_SHINE];
	data->icld_InfoShadowPen                      = _pens(obj)[MPEN_SHADOW];

//	data->icld__Option_LabelTextMultiLine;
//	data->icld__Option_LabelTextOnlySelectedMultiLine;

	data->icld__Option_IconHorizontalSpacing      = ILC_ICON_HORIZONTALMARGIN_DEFAULT;
	data->icld__Option_IconVerticalSpacing        = ILC_ICON_VERTICALMARGIN_DEFAULT;
	data->icld__Option_IconImageSpacing           = ILC_ICONLABEL_IMAGEMARGIN_DEFAULT;
	data->icld__Option_LabelTextHorizontalSpacing = ILC_ICONLABEL_HORIZONTALTEXTMARGIN_DEFAULT;
	data->icld__Option_LabelTextVerticalSpacing   = ILC_ICONLABEL_VERTICALTEXTMARGIN_DEFAULT;
	data->icld__Option_LabelTextBorderWidth       = ILC_ICONLABEL_BORDERWIDTH_DEFAULT;
	data->icld__Option_LabelTextBorderHeight      = ILC_ICONLABEL_BORDERHEIGHT_DEFAULT;
	
    ForeachNode(&data->icld_IconList, node)
    {
        if (!node->ile_DiskObj)
        {
            if (!(node->ile_DiskObj = GetIconTags(node->ile_IconListEntry.filename, ICONGETA_FailIfUnavailable, FALSE, ICONGETA_Label, (IPTR) node->ile_IconListEntry.label, TAG_DONE)))
			{
D(bug("[IconList] IconList__MUIM_Setup: Failed to obtain Icon '%s's diskobj!!\n", node->ile_IconListEntry.filename));
				/*	We should proabbly remove this node if the icon cant be obtained ? */
			}
        }
    }
    return 1;
}

/**************************************************************************
MUIM_Show
**************************************************************************/
IPTR IconList__MUIM_Show(struct IClass *CLASS, Object *obj, struct MUIP_Show *message)
{
    struct MUI_IconData *data = INST_DATA(CLASS, obj);
    LONG                newleft,
                        newtop;
    IPTR                rc;

    if ((rc = DoSuperMethodA(CLASS, obj, (Msg)message)))
	{

		newleft = data->icld_ViewX;
		newtop = data->icld_ViewY;

		if (newleft + _mwidth(obj) > data->icld_AreaWidth) newleft = data->icld_AreaWidth - _mwidth(obj);
		if (newleft < 0) newleft = 0;

		if (newtop + _mheight(obj) > data->icld_AreaHeight) newtop = data->icld_AreaHeight - _mheight(obj);
		if (newtop < 0) newtop = 0;

		if ((newleft != data->icld_ViewX) || (newtop != data->icld_ViewY))
		{    
			SetAttrs(obj, MUIA_IconList_Left, newleft,
				MUIA_IconList_Top, newtop,
				TAG_DONE);
		}

		/* Get Internal Objects to use if not set .. */
		if (data->icld_BufferRastPort == NULL) data->icld_BufferRastPort = _rp(obj);
		if (data->icld_IconLabelFont == NULL)       data->icld_IconLabelFont = _font(obj);
		if (data->icld_IconInfoFont == NULL)        data->icld_IconInfoFont = _font(obj);
D(bug("[IconList] IconList__MUIM_Show: Use Font @ %x, RastPort @ %x\n", data->icld_IconLabelFont, data->icld_BufferRastPort ));

		if ((data->icld_BufferRastPort) && (data->icld_IconLabelFont))
			SetFont(data->icld_BufferRastPort, data->icld_IconLabelFont);
	}
    return rc;
}


/**************************************************************************
MUIM_Cleanup
**************************************************************************/
IPTR IconList__MUIM_Cleanup(struct IClass *CLASS, Object *obj, struct MUIP_Cleanup *message)
{
    struct MUI_IconData *data = INST_DATA(CLASS, obj);
    struct IconEntry    *node = NULL;

    ForeachNode(&data->icld_IconList, node)
    {
        if (node->ile_DiskObj)
        {
            FreeDiskObject(node->ile_DiskObj);
            node->ile_DiskObj = NULL;
        }
    }

    DoMethod(_win(obj), MUIM_Window_RemEventHandler, (IPTR)&data->ehn);

    return DoSuperMethodA(CLASS, obj, (Msg)message);
}

/**************************************************************************
MUIM_AskMinMax
**************************************************************************/
IPTR IconList__MUIM_AskMinMax(struct IClass *CLASS, Object *obj, struct MUIP_AskMinMax *message)
{
    ULONG rc = DoSuperMethodA(CLASS, obj, (Msg) message);

    message->MinMaxInfo->MinWidth  += 96;
    message->MinMaxInfo->MinHeight += 64;

    message->MinMaxInfo->DefWidth  += 200;
    message->MinMaxInfo->DefHeight += 180;

    message->MinMaxInfo->MaxWidth = MUI_MAXMAX;
    message->MinMaxInfo->MaxHeight = MUI_MAXMAX;

    return rc;
}

/**************************************************************************
MUIM_Layout
**************************************************************************/
IPTR IconList__MUIM_Layout(struct IClass *CLASS, Object *obj,struct MUIP_Layout *message)
{
    struct MUI_IconData *data = INST_DATA(CLASS, obj);

    ULONG rc = DoSuperMethodA(CLASS, obj, (Msg)message);

    data->icld_ViewWidth = _mwidth(obj);
    data->icld_ViewHeight = _mheight(obj);

    return rc;
}

/**************************************************************************
MUIM_Draw - draw the IconList
**************************************************************************/
IPTR IconList__MUIM_Draw(struct IClass *CLASS, Object *obj, struct MUIP_Draw *message)
{   
    struct MUI_IconData    *data = INST_DATA(CLASS, obj);
    struct IconEntry       *icon = NULL;

    APTR                   clip;

    ULONG                  update_oldwidth = 0,
                           update_oldheight = 0;

	LONG                  clear_xoffset = 0,
                           clear_yoffset = 0;

    DoSuperMethodA(CLASS, obj, (Msg)message);

	if (!data->icld__Option_IconListFixedBackground)
	{
		clear_xoffset = data->icld_ViewX;
		clear_yoffset = data->icld_ViewY;
	}

    // If window size changes, only update needed areas
    if (data->update_oldwidth == 0) data->update_oldwidth = data->icld_ViewWidth;
    if (data->update_oldheight == 0) data->update_oldheight = data->icld_ViewHeight;
    if (data->update_oldwidth != data->icld_ViewWidth || data->update_oldheight != data->icld_ViewHeight)
    {
        if ( data->icld_UpdateMode != UPDATE_SCROLL )
        { 
            data->icld_UpdateMode = UPDATE_RESCALE;
            update_oldwidth = data->update_oldwidth;
            update_oldheight = data->update_oldheight;
            data->update_oldwidth = data->icld_ViewWidth;
            data->update_oldheight = data->icld_ViewHeight;
        }
    }

    if (message->flags & MADF_DRAWUPDATE || data->icld_UpdateMode == UPDATE_RESCALE)
    {
        if (data->icld_UpdateMode == UPDATE_SINGLEICON) /* draw only a single icon at update_icon */
        {
            struct Rectangle rect;
    
            IconList_GetIconRectangle(obj, data, data->update_icon, &rect);
    
            rect.MinX += _mleft(obj) + (data->update_icon->ile_IconX - data->icld_ViewX);
            rect.MaxX += _mleft(obj) + (data->update_icon->ile_IconX - data->icld_ViewX);
            rect.MinY += _mtop(obj) + (data->update_icon->ile_IconY - data->icld_ViewY);
            rect.MaxY += _mtop(obj) + (data->update_icon->ile_IconY - data->icld_ViewY);
    
            clip = MUI_AddClipping(muiRenderInfo(obj), _mleft(obj), _mtop(obj), _mwidth(obj), _mheight(obj));

#if defined(DEBUG_ILC_ICONRENDERING)
D(bug("[IconList] IconList__MUIM_Draw: Calling MUIM_DrawBackground (A)\n"));
#endif
			DoMethod(obj, MUIM_DrawBackground, 
				rect.MinX, rect.MinY,
				rect.MaxX - rect.MinX + 1, rect.MaxY - rect.MinY + 1,
				clear_xoffset, clear_yoffset, 
				0);

            /* We could have deleted also other icons so they must be redrawn */
            ForeachNode(&data->icld_IconList, icon)
            {
                if ((icon != data->update_icon) && (icon->ile_Flags & ICONENTRY_FLAG_VISIBLE))
                {
                    struct Rectangle rect2;
                    IconList_GetIconRectangle(obj, data, icon, &rect2);

                    rect2.MinX += _mleft(obj) - data->icld_ViewX + icon->ile_IconX;
                    rect2.MaxX += _mleft(obj) - data->icld_ViewX + icon->ile_IconX;
                    rect2.MinY += _mtop(obj) - data->icld_ViewY + icon->ile_IconY;
                    rect2.MaxY += _mtop(obj) - data->icld_ViewY + icon->ile_IconY;

                    if (RectAndRect(&rect, &rect2))
                    {  
                        // Update icon here
                        DoMethod(obj, MUIM_IconList_DrawEntry, icon, ICONENTRY_DRAWMODE_PLAIN);
                        DoMethod(obj, MUIM_IconList_DrawEntryLabel, icon, ICONENTRY_DRAWMODE_PLAIN);
                    }
                }
            }

            DoMethod(obj, MUIM_IconList_DrawEntry, data->update_icon, ICONENTRY_DRAWMODE_PLAIN);
			DoMethod(obj, MUIM_IconList_DrawEntryLabel, data->update_icon, ICONENTRY_DRAWMODE_PLAIN);
            data->icld_UpdateMode = 0;
            MUI_RemoveClipping(muiRenderInfo(obj),clip);
            return 0;
        }
        else if (data->icld_UpdateMode == UPDATE_SCROLL)
        {
            struct Region   	*region = NULL;
            struct Rectangle 	xrect,
								yrect;
            BOOL    	    	scroll_caused_damage;

			if (!data->icld__Option_IconListFixedBackground)
			{
				scroll_caused_damage = (_rp(obj)->Layer->Flags & LAYERREFRESH) ? FALSE : TRUE;
		
				data->icld_UpdateMode = 0;

				if ((abs(data->update_scrolldx) >= _mwidth(obj)) ||
					(abs(data->update_scrolldy) >= _mheight(obj)))
				{
					MUI_Redraw(obj, MADF_DRAWOBJECT);
					return 0;
				}

				if (!(region = NewRegion()))
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

				ScrollRasterBF(data->icld_BufferRastPort,
					data->update_scrolldx,
					data->update_scrolldy,
					_mleft(obj),
					_mtop(obj),
					_mright(obj),
					_mbottom(obj));

				scroll_caused_damage = scroll_caused_damage && (_rp(obj)->Layer->Flags & LAYERREFRESH) ? TRUE : FALSE;

				clip = MUI_AddClipRegion(muiRenderInfo(obj), region);
            }

            MUI_Redraw(obj, MADF_DRAWOBJECT);

            data->update_rect1 = data->update_rect2 = NULL;

			if (!data->icld__Option_IconListFixedBackground)
			{
				MUI_RemoveClipRegion(muiRenderInfo(obj), clip);
		
				if (scroll_caused_damage)
				{
					if (MUI_BeginRefresh(muiRenderInfo(obj), 0))
					{
						/* Theoretically it might happen that more damage is caused
						after ScrollRaster. By something else, like window movement
						in front of our window. Therefore refresh root object of
						window, not just this object */
			
						Object *o = NULL;
			
						GET(_win(obj),MUIA_Window_RootObject, &o);
						MUI_Redraw(o, MADF_DRAWOBJECT);
			
						MUI_EndRefresh(muiRenderInfo(obj), 0);
					}
				}
			}
            return 0;
        }
        else if (data->icld_UpdateMode == UPDATE_RESCALE)
        {
            struct Region       *region = NULL;
            struct Rectangle    wrect,
                                hrect;
            ULONG               diffw = 0,
                                diffh = 0;

            data->icld_UpdateMode = 0;

            if (!data->icld__Option_IconListScaledBackground)
			{
				if (!(region = NewRegion()))
				{
					MUI_Redraw(obj, MADF_DRAWOBJECT);
					return 0;
				}

				if ( data->icld_ViewWidth > update_oldwidth )
					diffw = data->icld_ViewWidth - update_oldwidth;
				if ( data->icld_ViewHeight > update_oldheight )
					diffh = data->icld_ViewHeight - update_oldheight;            

				if (diffw)
				{
					wrect.MinX = _mright(obj) - diffw;
					wrect.MinY = _mtop(obj);
					wrect.MaxX = _mright(obj);
					wrect.MaxY = _mbottom(obj);
					OrRectRegion(region, &wrect);
					data->update_rect1 = &wrect;
				}

				if (diffh)
				{
					hrect.MinX = _mleft(obj);
					hrect.MinY = _mbottom(obj) - diffh;
					hrect.MaxX = _mright(obj);
					hrect.MaxX = _mright(obj);
					hrect.MaxY = _mbottom(obj);
					OrRectRegion(region, &hrect);
					data->update_rect2 = &hrect;
				}
				if (diffh||diffw)
				{
					clip = MUI_AddClipRegion(muiRenderInfo(obj), region);
				}
				else
				{
					/* View became smaller both in horizontal and vertical direction.
					   Nothing to do */
					   
					DisposeRegion(region);
					return 0;
				}
			}

			MUI_Redraw(obj, MADF_DRAWOBJECT);

			if (!data->icld__Option_IconListScaledBackground)
			{
				if (diffh||diffw)
				{
						data->update_rect1 = data->update_rect2 = NULL;
						MUI_RemoveClipRegion(muiRenderInfo(obj), clip);
				} else DisposeRegion(region);
			}

            return 0;
        }
    }

    /* At we see if there any Icons without proper position, this is the wrong place here,
    * it should be done after all icons have been loaded */

    /*ric - no need!: IconList_FixNoPositionIcons(obj, data);*/
	if (message->flags & MADF_DRAWOBJECT)
	{
		clip = MUI_AddClipping(muiRenderInfo(obj), _mleft(obj), _mtop(obj), _mwidth(obj), _mheight(obj));

#if defined(DEBUG_ILC_ICONRENDERING)
D(bug("[IconList] IconList__MUIM_Draw: Calling MUIM_DrawBackground (B)\n"));
#endif
		DoMethod(
			obj, MUIM_DrawBackground, _mleft(obj), _mtop(obj), _mwidth(obj), _mheight(obj),
			clear_xoffset, clear_yoffset, 0
		);

		ForeachNode(&data->icld_IconList, icon)
		{
			if ((icon->ile_Flags & ICONENTRY_FLAG_VISIBLE) && (icon->ile_DiskObj && icon->ile_IconX != NO_ICON_POSITION && icon->ile_IconY != NO_ICON_POSITION))
			{
				DoMethod(obj, MUIM_IconList_DrawEntry, icon, ICONENTRY_DRAWMODE_PLAIN);
				DoMethod(obj, MUIM_IconList_DrawEntryLabel, icon, ICONENTRY_DRAWMODE_PLAIN);
			}
		}

		MUI_RemoveClipping(muiRenderInfo(obj), clip);
	}
    data->icld_UpdateMode = 0;

    return 0;
}

/**************************************************************************
MUIM_IconList_Refresh
Implemented by subclasses
**************************************************************************/
IPTR IconList__MUIM_IconList_Update(struct IClass *CLASS, Object *obj, struct MUIP_IconList_Update *message)
{
    struct MUI_IconData *data = INST_DATA(CLASS, obj);

D(bug("[IconList]: IconList__MUIM_IconList_Update()\n"));

	data->icld_FocusIcon = NULL;
	data->icld_SelectionFirst = NULL;
	data->icld_SelectionLast = NULL;

    return 1;
}

/**************************************************************************
MUIM_IconList_Clear
**************************************************************************/
IPTR IconList__MUIM_IconList_Clear(struct IClass *CLASS, Object *obj, struct MUIP_IconList_Clear *message)
{
    struct MUI_IconData *data = INST_DATA(CLASS, obj);
    struct IconEntry    *node = NULL;

    while ((node = (struct IconEntry*)RemTail((struct List*)&data->icld_IconList)))
    {
        DoMethod(obj, MUIM_IconList_DestroyEntry, node);
    }

    data->icld_SelectionFirst = NULL;
    data->icld_ViewX = data->icld_ViewY = data->icld_AreaWidth = data->icld_AreaHeight = 0;
    /*data->icld_SortFlags = 0;*/

    data->icld_IconLargestWidth = data->icld_IconLargestHeight = 32;	/*default icon size*/

    SetAttrs(obj, MUIA_IconList_Left, data->icld_ViewX,
        MUIA_IconList_Top, data->icld_ViewY,
        MUIA_IconList_Width, data->icld_AreaWidth,
        MUIA_IconList_Height, data->icld_AreaHeight,
        TAG_DONE);

    MUI_Redraw(obj,MADF_DRAWOBJECT);
    return 1;
}

IPTR IconList__MUIM_IconList_DestroyEntry(struct IClass *CLASS, Object *obj, struct MUIP_IconList_DestroyEntry *message)
{
    struct MUI_IconData *data = INST_DATA(CLASS, obj);

	if (message->icon)
	{
		if (message->icon->ile_TxtBuf_DisplayedLabel) FreePooled(data->icld_Pool, message->icon->ile_TxtBuf_DisplayedLabel, 255);
		if (message->icon->ile_TxtBuf_PROT) FreePooled(data->icld_Pool, message->icon->ile_TxtBuf_PROT, 8);
		if (message->icon->ile_TxtBuf_SIZE) FreePooled(data->icld_Pool, message->icon->ile_TxtBuf_SIZE, 30);
		if (message->icon->ile_TxtBuf_TIME) FreePooled(data->icld_Pool, message->icon->ile_TxtBuf_TIME, LEN_DATSTRING);
		if (message->icon->ile_TxtBuf_DATE) FreePooled(data->icld_Pool, message->icon->ile_TxtBuf_DATE, LEN_DATSTRING);
        if (message->icon->ile_DiskObj) FreeDiskObject(message->icon->ile_DiskObj);
        if (message->icon->ile_IconListEntry.label) FreePooled(data->icld_Pool, message->icon->ile_IconListEntry.label, strlen(message->icon->ile_IconListEntry.label)+1);
        if (message->icon->ile_IconListEntry.filename) FreePooled(data->icld_Pool, message->icon->ile_IconListEntry.filename, strlen(message->icon->ile_IconListEntry.filename)+1);
        FreePooled(data->icld_Pool, message->icon, sizeof(struct IconEntry));
	}
	return TRUE;
}

/**************************************************************************
MUIM_IconList_CreateEntry.
Returns 0 on failure otherwise it returns the icons entry ..
**************************************************************************/
IPTR IconList__MUIM_IconList_CreateEntry(struct IClass *CLASS, Object *obj, struct MUIP_IconList_CreateEntry *message)
{
    struct MUI_IconData  *data = INST_DATA(CLASS, obj);
    struct IconEntry     *entry = NULL;
    struct DateTime 	 dt;
	struct DateStamp     now;
    UBYTE   	    	 *sp = NULL;

    struct DiskObject    *dob = NULL;
    struct Rectangle     rect;

    /*disk object (icon)*/
	if (!(message->icon_dob))
	{
		dob = GetIconTags
		(
			message->filename, 
			ICONGETA_FailIfUnavailable,        FALSE,
			ICONGETA_Label,             (IPTR) message->label,
			TAG_DONE
		);
	}
	else
	{
		dob = message->icon_dob;
	}

    if (!dob) return 0;

    if (!(entry = AllocPooled(data->icld_Pool,sizeof(struct IconEntry))))
    {
        FreeDiskObject(dob);
        return 0;
    }
    memset(entry,0,sizeof(struct IconEntry));

	/* Allocate Text Buffers */
    if (!(entry->ile_TxtBuf_DisplayedLabel = AllocPooled(data->icld_Pool, 255)))
	{
        DoMethod(obj, MUIM_IconList_DestroyEntry, entry);
        return 0;
	}
	memset(entry->ile_TxtBuf_DisplayedLabel, 0, 255);
	
    if (!(entry->ile_TxtBuf_DATE = AllocPooled(data->icld_Pool, LEN_DATSTRING)))
    {
        DoMethod(obj, MUIM_IconList_DestroyEntry, entry);
        return 0;
    }
	memset(entry->ile_TxtBuf_DATE, 0, LEN_DATSTRING);

    if (!(entry->ile_TxtBuf_TIME = AllocPooled(data->icld_Pool, LEN_DATSTRING)))
    {
        DoMethod(obj, MUIM_IconList_DestroyEntry, entry);
        return 0;
    }
	memset(entry->ile_TxtBuf_TIME, 0, LEN_DATSTRING);

    if (!(entry->ile_TxtBuf_SIZE = AllocPooled(data->icld_Pool, 30)))
    {
        DoMethod(obj, MUIM_IconList_DestroyEntry, entry);
        return 0;
    }
	memset(entry->ile_TxtBuf_SIZE, 0, 30);

    if (!(entry->ile_TxtBuf_PROT = AllocPooled(data->icld_Pool, 8)))
    {
        DoMethod(obj, MUIM_IconList_DestroyEntry, entry);
        return 0;
    }
	memset(entry->ile_TxtBuf_PROT, 0, 8);
	
    /*alloc filename*/
    if (!(entry->ile_IconListEntry.filename = AllocPooled(data->icld_Pool, strlen(message->filename) + 1)))
    {
        DoMethod(obj, MUIM_IconList_DestroyEntry, entry);
        return 0;
    }

    /*alloc icon label*/
    if (!(entry->ile_IconListEntry.label = AllocPooled(data->icld_Pool, strlen(message->label) + 1)))
    {
        DoMethod(obj, MUIM_IconList_DestroyEntry, entry);
        return 0;
    }

    /*file info block*/
    if( message->fib )
    {
        entry->ile_FileInfoBlock = *message->fib;

        if (entry->ile_FileInfoBlock.fib_DirEntryType > 0)
        {
            strcpy(entry->ile_TxtBuf_SIZE, "Drawer");
        }
        else
        {
			int i = entry->ile_FileInfoBlock.fib_Size;

			/*show byte size for small files*/
			if( i > 9999 )
				sprintf(entry->ile_TxtBuf_SIZE, "%ld KB", (LONG)(i/1024));
			else
				sprintf(entry->ile_TxtBuf_SIZE, "%ld B", (LONG)i);
        }

        dt.dat_Stamp    = entry->ile_FileInfoBlock.fib_Date;
        dt.dat_Format   = FORMAT_DEF;
        dt.dat_Flags    = 0;
        dt.dat_StrDay   = NULL;
        dt.dat_StrDate  = entry->ile_TxtBuf_DATE;
        dt.dat_StrTime  = entry->ile_TxtBuf_TIME;
    
        DateToStr(&dt);
		DateStamp(&now);

		/*if modified today show time, otherwise just show date*/
		if( now.ds_Days == entry->ile_FileInfoBlock.fib_Date.ds_Days )
			entry->ile_Flags |= ICONENTRY_FLAG_TODAY;
		else
			entry->ile_Flags &= ~ICONENTRY_FLAG_TODAY;
		
        sp = entry->ile_TxtBuf_PROT;
        *sp++ = (entry->ile_FileInfoBlock.fib_Protection & FIBF_SCRIPT)  ? 's' : '-';
        *sp++ = (entry->ile_FileInfoBlock.fib_Protection & FIBF_PURE)    ? 'p' : '-';
        *sp++ = (entry->ile_FileInfoBlock.fib_Protection & FIBF_ARCHIVE) ? 'a' : '-';
        *sp++ = (entry->ile_FileInfoBlock.fib_Protection & FIBF_READ)    ? '-' : 'r';
        *sp++ = (entry->ile_FileInfoBlock.fib_Protection & FIBF_WRITE)   ? '-' : 'w';
        *sp++ = (entry->ile_FileInfoBlock.fib_Protection & FIBF_EXECUTE) ? '-' : 'e';
        *sp++ = (entry->ile_FileInfoBlock.fib_Protection & FIBF_DELETE)  ? '-' : 'd';
        *sp++ = '\0';
    
        entry->ile_IconListEntry.type = entry->ile_FileInfoBlock.fib_DirEntryType;
    }
    else
    {
        entry->ile_IconListEntry.type = ST_USERDIR;
    }

    strcpy(entry->ile_IconListEntry.filename, message->filename);
    strcpy(entry->ile_IconListEntry.label, message->label);

	ULONG nameLength = strlen(entry->ile_IconListEntry.label);

	ULONG len = data->icld__Option_IconTextMaxLen;
	// Make sure the maxlen is at least the length of ".."
	if ( len < 3 ) len = 3;
        
	if(nameLength > len)
	{
		strncpy(entry->ile_TxtBuf_DisplayedLabel, entry->ile_IconListEntry.label, len - 3);
		strcat(entry->ile_TxtBuf_DisplayedLabel , " ..");
		nameLength = len;
	}
	else 
	{
		strncpy(entry->ile_TxtBuf_DisplayedLabel, entry->ile_IconListEntry.label, nameLength );
	}
	
    entry->ile_DiskObj = dob;
    entry->ile_IconListEntry.udata = NULL;
    entry->ile_IconX = dob->do_CurrentX;
    entry->ile_IconY = dob->do_CurrentY;

    /* Use a geticonrectangle routine that gets textwidth! */
    IconList_GetIconRectangle(obj, data, entry, &rect);

    AddHead((struct List*)&data->icld_IconList, (struct Node*)entry);

    return (IPTR)entry;
}
/* fib_DirEntryType,ST_USERDIR; LONG type */

static void DoWheelMove(struct IClass *CLASS, Object *obj, LONG wheelx, LONG wheely, UWORD qual)
{
    struct MUI_IconData *data = INST_DATA(CLASS, obj);

    LONG    newleft = data->icld_ViewX,
			newtop = data->icld_ViewY;

    /* Use horizontal scrolling if any of the following cases are true ...

		#  vertical wheel is used but there's nothing to scroll
           (everything is visible already) ..

        #  vertical wheel is used and one of the ALT keys is down.      */  

    if ((wheely && !wheelx) &&
       ((data->icld_AreaHeight <= _mheight(obj)) || (qual & (IEQUALIFIER_LALT | IEQUALIFIER_RALT))))
    {
        wheelx = wheely; wheely = 0;
    }

    if (qual & (IEQUALIFIER_CONTROL))
    {
        if (wheelx < 0) newleft = 0;
        if (wheelx > 0) newleft = data->icld_AreaWidth;
        if (wheely < 0) newtop = 0;
        if (wheely > 0) newtop = data->icld_AreaHeight;
    }
    else if (qual & (IEQUALIFIER_LSHIFT | IEQUALIFIER_RSHIFT))
    {
        newleft += (wheelx * _mwidth(obj));
        newtop += (wheely * _mheight(obj));
    }
    else
    {
        newleft += wheelx * 30;
        newtop += wheely * 30;
    }

    if (newleft + _mwidth(obj) > data->icld_AreaWidth) newleft = data->icld_AreaWidth - _mwidth(obj);
    if (newleft < 0) newleft = 0;

    if (newtop + _mheight(obj) > data->icld_AreaHeight) newtop = data->icld_AreaHeight - _mheight(obj);
    if (newtop < 0) newtop = 0;

    if ((newleft != data->icld_ViewX) || (newtop != data->icld_ViewY))
    {
        SetAttrs(obj, MUIA_IconList_Left, newleft,
            MUIA_IconList_Top, newtop,
            TAG_DONE);
    }

}

/**************************************************************************
MUIM_HandleEvent
**************************************************************************/
IPTR IconList__MUIM_HandleEvent(struct IClass *CLASS, Object *obj, struct MUIP_HandleEvent *message)
{
    struct MUI_IconData *data = INST_DATA(CLASS, obj);

    if (message->imsg)
    {
        LONG mx = message->imsg->MouseX - _mleft(obj);
        LONG my = message->imsg->MouseY - _mtop(obj);

        LONG wheelx = 0;
        LONG wheely = 0;
    
        switch (message->imsg->Class)
        {
            case IDCMP_RAWKEY:
            {
				BOOL rawkey_handled = FALSE;

				switch(message->imsg->Code)
				{
					case RAWKEY_NM_WHEEL_UP:
						wheely = -1;
						rawkey_handled = TRUE;
						break;

					case RAWKEY_NM_WHEEL_DOWN:
						wheely = 1;
						rawkey_handled = TRUE;
						break;

					case RAWKEY_NM_WHEEL_LEFT:
						wheelx = -1;
						rawkey_handled = TRUE;
						break;

					case RAWKEY_NM_WHEEL_RIGHT:
						wheelx = 1;
						rawkey_handled = TRUE;
						break;
				}

				if (rawkey_handled)
				{
#if defined(DEBUG_ILC_KEYEVENTS)
D(bug("[IconList] IconList__MUIM_HandleEvent: Processing mouse wheel event\n"));
#endif
					if (_isinobject(message->imsg->MouseX, message->imsg->MouseY) &&
						(wheelx || wheely))
					{
						DoWheelMove(CLASS, obj, wheelx, wheely, message->imsg->Qualifier);
					}
				}
				else if (!(message->imsg->Code & IECODE_UP_PREFIX))
				{
					LONG new_ViewY = data->icld_ViewY;
					struct IconEntry *start_entry = NULL, *active_entry = NULL, *entry_next = NULL;
					IPTR              start_X = 0, start_Y = 0, active_X = 0, active_Y = 0, next_X = 0, next_Y = 0;
					IPTR              x_diff = 0;

#if defined(DEBUG_ILC_KEYEVENTS)
D(bug("[IconList] IconList__MUIM_HandleEvent: Processing key up event\n"));
#endif

					switch(message->imsg->Code)
					{
						case RAWKEY_RETURN:
							rawkey_handled = TRUE;

#if defined(DEBUG_ILC_KEYEVENTS)
D(bug("[IconList] IconList__MUIM_HandleEvent: RAWKEY_RETURN\n"));
#endif

							if (data->icld_SelectionFirst) active_entry = data->icld_SelectionFirst;
							else if (data->icld_FocusIcon) active_entry = data->icld_FocusIcon;

							if (active_entry)
							{
								if (!(active_entry->ile_Flags & ICONENTRY_FLAG_SELECTED))
								{
									active_entry->ile_Flags |= ICONENTRY_FLAG_SELECTED;
									data->icld_UpdateMode = UPDATE_SINGLEICON;
									data->update_icon = active_entry;
									MUI_Redraw(obj, MADF_DRAWUPDATE);
								}
								data->icld_SelectionFirst = active_entry;
								data->icld_SelectionLast = active_entry;
								data->icld_FocusIcon = active_entry;

								SET(obj, MUIA_IconList_DoubleClick, TRUE);
							}
							break;

						case RAWKEY_SPACE:
							rawkey_handled = TRUE;

#if defined(DEBUG_ILC_KEYEVENTS)
D(bug("[IconList] IconList__MUIM_HandleEvent: RAWKEY_SPACE\n"));
#endif

							if (data->icld_FocusIcon) active_entry = data->icld_FocusIcon;
							else if (data->icld_SelectionFirst) active_entry = data->icld_SelectionFirst;

						    if (!(message->imsg->Qualifier & IEQUALIFIER_LSHIFT) && ((data->icld_SelectionFirst)||(data->icld_SelectionLast)))
							{
#if defined(DEBUG_ILC_KEYEVENTS)
D(bug("[IconList] IconList__MUIM_HandleEvent: SPACE: Clearing selected icons ..\n"));
#endif
								DoMethod(obj, MUIM_IconList_UnselectAll);
							}

							if (active_entry)
							{
								if (!(active_entry->ile_Flags & ICONENTRY_FLAG_SELECTED))
								{
									active_entry->ile_Flags |= ICONENTRY_FLAG_SELECTED;
									data->icld_SelectionFirst = active_entry;
								}
								else
									active_entry->ile_Flags &= ~ICONENTRY_FLAG_SELECTED;

								data->icld_FocusIcon = active_entry;

								data->icld_UpdateMode = UPDATE_SINGLEICON;
								data->update_icon = active_entry;
								MUI_Redraw(obj, MADF_DRAWUPDATE);
							}
							break;

						case RAWKEY_PAGEUP:
							rawkey_handled = TRUE;

#if defined(DEBUG_ILC_KEYEVENTS)
D(bug("[IconList] IconList__MUIM_HandleEvent: RAWKEY_PAGEUP\n"));
#endif

							if (data->icld_AreaHeight > data->icld_ViewHeight)
							{
								new_ViewY -= data->icld_ViewHeight;
								if (new_ViewY< 0)
									new_ViewY = 0;
							}

							if (new_ViewY != data->icld_ViewY)
							{
								SET(obj, MUIA_IconList_Top, new_ViewY);
							}
							break;

						case RAWKEY_PAGEDOWN:
							rawkey_handled = TRUE;

#if defined(DEBUG_ILC_KEYEVENTS)
D(bug("[IconList] IconList__MUIM_HandleEvent: RAWKEY_PAGEDOWN\n"));
#endif

							if (data->icld_AreaHeight > data->icld_ViewHeight)
							{
								new_ViewY += data->icld_ViewHeight;
								if (new_ViewY > (data->icld_AreaHeight - data->icld_ViewHeight))
									new_ViewY = data->icld_AreaHeight - data->icld_ViewHeight;
							}

							if (new_ViewY != data->icld_ViewY)
							{
								SET(obj, MUIA_IconList_Top, new_ViewY);
							}
							break;

						case RAWKEY_UP:
							rawkey_handled = TRUE;

#if defined(DEBUG_ILC_KEYEVENTS)
D(bug("[IconList] IconList__MUIM_HandleEvent: RAWKEY_UP\n"));
#endif

							if (data->icld_FocusIcon)
							{
								start_entry = data->icld_FocusIcon;
#if defined(DEBUG_ILC_KEYEVENTS)
D(bug("[IconList] IconList__MUIM_HandleEvent: UP: Clearing existing focused icon @ %x\n", start_entry));
#endif

								start_entry->ile_Flags &= ~ICONENTRY_FLAG_FOCUS;
								data->icld_UpdateMode = UPDATE_SINGLEICON;
								data->update_icon = start_entry;
								MUI_Redraw(obj, MADF_DRAWUPDATE);

								start_X = start_entry->ile_IconX;
								start_Y = start_entry->ile_IconY;
#if defined(DEBUG_ILC_KEYEVENTS)
D(bug("[IconList] IconList__MUIM_HandleEvent: UP: start_icon @ %x '%s' - start_X %d, start_Y %d\n", start_entry, start_entry->ile_IconListEntry.label, start_X, start_Y));
#endif
								if (data->icld__Option_IconListMode == ICON_LISTMODE_GRID)
								{
									if (start_entry->ile_AreaWidth < data->icld_IconLargestWidth)
									{
										start_X = start_X - ((data->icld_IconLargestWidth - start_entry->ile_AreaWidth)/2);
#if defined(DEBUG_ILC_KEYEVENTS)
D(bug("[IconList] IconList__MUIM_HandleEvent: UP: adjusted start_X for grid = %d\n", start_X));
#endif
									}
								}

								if ((active_entry = Node_PreviousVisible(start_entry)) && !(data->icld_DisplayFlags & ICONLIST_DISP_VERTICAL))
								{
									//Check if we are at the edge of the icon area ..
#if defined(DEBUG_ILC_KEYEVENTS)
D(bug("[IconList] IconList__MUIM_HandleEvent: UP: active_entry @ %x '%s' , X %d, Y %d\n", active_entry, active_entry->ile_IconListEntry.label, active_entry->ile_IconX, active_entry->ile_IconY));
#endif
									active_Y = active_entry->ile_IconY;

									if (active_Y == start_Y)
									{

#if defined(DEBUG_ILC_KEYEVENTS)
D(bug("[IconList] IconList__MUIM_HandleEvent: UP: active_entry is on our row (not at the edge)\n"));
#endif
										entry_next = active_entry;
										next_X = entry_next->ile_IconX;
										if (data->icld__Option_IconListMode == ICON_LISTMODE_GRID)
										{
											if (entry_next->ile_AreaWidth < data->icld_IconLargestWidth)
												next_X = next_X - ((data->icld_IconLargestWidth - entry_next->ile_AreaWidth)/2);
										}
									}
								}
#if defined(DEBUG_ILC_KEYEVENTS)
D(bug("[IconList] IconList__MUIM_HandleEvent: UP: using start_X %d, start_Y %d\n", start_X, start_Y));
#endif
							}

						    if (!(message->imsg->Qualifier & IEQUALIFIER_LSHIFT) && ((data->icld_SelectionFirst)||(data->icld_SelectionLast)))
							{
#if defined(DEBUG_ILC_KEYEVENTS)
D(bug("[IconList] IconList__MUIM_HandleEvent: UP: Clearing selected icons ..\n"));
#endif
								DoMethod(obj, MUIM_IconList_UnselectAll);
							}

#if defined(DEBUG_ILC_KEYEVENTS)
D(bug("[IconList] IconList__MUIM_HandleEvent: UP: active = %x, next = %x\n", active_entry, entry_next));
#endif
							if (!(active_entry))
							{
								// If nothing is selected we will use the last visible icon ..
								active_entry = Node_LastVisible(&data->icld_IconList);
								start_X = active_entry->ile_IconX;
								if (data->icld__Option_IconListMode == ICON_LISTMODE_GRID)
								{
									if (active_entry->ile_AreaWidth < data->icld_IconLargestWidth)
										start_X = start_X - ((data->icld_IconLargestWidth - active_entry->ile_AreaWidth)/2);
								}
								start_Y = active_entry->ile_IconY;
							}

							while (active_entry != NULL)
							{
#if defined(DEBUG_ILC_KEYEVENTS)
D(bug("[IconList] IconList__MUIM_HandleEvent: UP: Checking active @ %x\n", active_entry));
#endif
								if (data->icld_DisplayFlags & ICONLIST_DISP_VERTICAL)
								{
									// Return the first visible since the list flow direction matches
									// our cursor direction
									if (active_entry->ile_Flags & ICONENTRY_FLAG_VISIBLE)
										break;
								}
								else
								{
									active_X = active_entry->ile_IconX;

									if (data->icld__Option_IconListMode == ICON_LISTMODE_GRID)
									{
										if (active_entry->ile_AreaWidth < data->icld_IconLargestWidth)
											x_diff = ((data->icld_IconLargestWidth - active_entry->ile_AreaWidth)/2);
									}
									active_Y = active_entry->ile_IconY;
									
									if (start_entry)
									{
										if (entry_next)
										{
											if ((active_entry->ile_Flags & ICONENTRY_FLAG_VISIBLE) &&
												(active_Y < start_Y) &&
												(((active_X - x_diff) >= start_X ) &&
												((active_X - x_diff) <= (start_X + start_entry->ile_AreaWidth + (x_diff*2)))))
											{
#if defined(DEBUG_ILC_KEYEVENTS)
D(bug("[IconList] IconList__MUIM_HandleEvent: UP: (A) entry %x matches\n", active_entry));
#endif
												break;
											}
											else if (active_entry == (struct IconEntry *)GetHead(&data->icld_IconList))
											{
#if defined(DEBUG_ILC_KEYEVENTS)
D(bug("[IconList] IconList__MUIM_HandleEvent: UP: (A) reached list start .. restarting from the end ..\n"));
#endif
												start_X = next_X;

												if ((entry_next = Node_PreviousVisible(entry_next)))
												{
													if (entry_next->ile_IconX > start_X)
														entry_next = NULL;
													else
													{
														next_X = entry_next->ile_IconX;
														if (data->icld__Option_IconListMode == ICON_LISTMODE_GRID)
														{
															if (entry_next->ile_AreaWidth < data->icld_IconLargestWidth)
																next_X = next_X - ((data->icld_IconLargestWidth - entry_next->ile_AreaWidth)/2);
														}
													}
												}
												start_Y = 0;
#if defined(DEBUG_ILC_KEYEVENTS)
D(bug("[IconList] IconList__MUIM_HandleEvent: UP: (A) startx = %d, start_Y = %d, next_X = %d, entry_next @ %x\n", start_X, start_Y, next_X, entry_next));
#endif
												active_entry = Node_LastVisible(&data->icld_IconList);
											}
										}
										else
										{
											if ((active_entry->ile_Flags & ICONENTRY_FLAG_VISIBLE) &&
												(active_Y < start_Y) &&
												((active_X + x_diff) < (start_X + start_entry->ile_AreaWidth + x_diff)))
											{
#if defined(DEBUG_ILC_KEYEVENTS)
D(bug("[IconList] IconList__MUIM_HandleEvent: UP: (B) entry %x matches\n", active_entry));
#endif
												break;
											}
										}
									}
									else
									{
										if (active_entry->ile_Flags & ICONENTRY_FLAG_VISIBLE)
										{
#if defined(DEBUG_ILC_KEYEVENTS)
D(bug("[IconList] IconList__MUIM_HandleEvent: UP: (C) entry %x matches\n", active_entry));
#endif
											break;
										}
									}
								}
								active_entry = (struct IconEntry *)(((struct Node *)active_entry)->ln_Pred);
							}

							if (!(active_entry))
							{
#if defined(DEBUG_ILC_KEYEVENTS)
D(bug("[IconList] IconList__MUIM_HandleEvent: UP: No Next UP Node - Getting Last visible icon ..\n"));
#endif
								/* We didnt find a "next UP" icon so just use the last visible */
								active_entry = Node_LastVisible(&data->icld_IconList);
							}
							
							if (active_entry)
							{
								if (!(active_entry->ile_Flags & ICONENTRY_FLAG_FOCUS))
								{
									active_entry->ile_Flags |= ICONENTRY_FLAG_FOCUS;
									data->icld_UpdateMode = UPDATE_SINGLEICON;
									data->update_icon = active_entry;
									MUI_Redraw(obj, MADF_DRAWUPDATE);
								}
								
							}
							data->icld_FocusIcon = active_entry;
							break;

						case RAWKEY_DOWN:
							rawkey_handled = TRUE;

#if defined(DEBUG_ILC_KEYEVENTS)
D(bug("[IconList] IconList__MUIM_HandleEvent: RAWKEY_DOWN\n"));
#endif

							if (data->icld_FocusIcon)
							{
								start_entry = data->icld_FocusIcon;
#if defined(DEBUG_ILC_KEYEVENTS)
D(bug("[IconList] IconList__MUIM_HandleEvent: DOWN: Clearing existing focused icon @ %x\n", start_entry));
#endif

								start_entry->ile_Flags &= ~ICONENTRY_FLAG_FOCUS;
								data->icld_UpdateMode = UPDATE_SINGLEICON;
								data->update_icon = start_entry;
								MUI_Redraw(obj, MADF_DRAWUPDATE);

								start_X = start_entry->ile_IconX;
								start_Y = start_entry->ile_IconY;
#if defined(DEBUG_ILC_KEYEVENTS)
D(bug("[IconList] IconList__MUIM_HandleEvent: DOWN: start_icon @ %x '%s' - start_X %d, start_Y %d\n", start_entry, start_entry->ile_IconListEntry.label, start_X, start_Y));
#endif
								if (data->icld__Option_IconListMode == ICON_LISTMODE_GRID)
								{
									if (start_entry->ile_AreaWidth < data->icld_IconLargestWidth)
									{
										start_X = start_X - ((data->icld_IconLargestWidth - start_entry->ile_AreaWidth)/2);
#if defined(DEBUG_ILC_KEYEVENTS)
D(bug("[IconList] IconList__MUIM_HandleEvent: DOWN: adjusted start_X for grid = %d\n", start_X));
#endif
									}
								}

								if ((active_entry = Node_NextVisible(start_entry)) &&
									(!(data->icld_DisplayFlags & ICONLIST_DISP_VERTICAL)))
								{
#if defined(DEBUG_ILC_KEYEVENTS)
D(bug("[IconList] IconList__MUIM_HandleEvent: DOWN: active_entry @ %x '%s' , X %d, Y %d\n", active_entry, active_entry->ile_IconListEntry.label, active_entry->ile_IconX, active_entry->ile_IconY));
#endif
									active_Y = active_entry->ile_IconY;

									if (active_Y == start_Y)
									{

#if defined(DEBUG_ILC_KEYEVENTS)
D(bug("[IconList] IconList__MUIM_HandleEvent: DOWN: active_entry is on our row (not at the edge)\n"));
#endif
										entry_next = active_entry;
										next_X = entry_next->ile_IconX;
										if (data->icld__Option_IconListMode == ICON_LISTMODE_GRID)
										{
											if (entry_next->ile_AreaWidth < data->icld_IconLargestWidth)
												next_X = next_X - ((data->icld_IconLargestWidth - entry_next->ile_AreaWidth)/2);
										}
									}
								}
#if defined(DEBUG_ILC_KEYEVENTS)
D(bug("[IconList] IconList__MUIM_HandleEvent: DOWN: using start_X %d, start_Y %d\n", start_X, start_Y));
#endif
							}

						    if (!(message->imsg->Qualifier & IEQUALIFIER_LSHIFT) && ((data->icld_SelectionFirst)||(data->icld_SelectionLast)))
							{
#if defined(DEBUG_ILC_KEYEVENTS)
D(bug("[IconList] IconList__MUIM_HandleEvent: DOWN: Clearing selected icons ..\n"));
#endif
								DoMethod(obj, MUIM_IconList_UnselectAll);
							}

#if defined(DEBUG_ILC_KEYEVENTS)
D(bug("[IconList] IconList__MUIM_HandleEvent: DOWN: active = %x, next = %x\n", active_entry, entry_next));
#endif
							if (!(active_entry))
							{
								// If nothing is selected we will use the First visible icon ..
								active_entry = Node_FirstVisible(&data->icld_IconList);
								start_X = active_entry->ile_IconX;
								if (data->icld__Option_IconListMode == ICON_LISTMODE_GRID)
								{
									if (active_entry->ile_AreaWidth < data->icld_IconLargestWidth)
										start_X = start_X - ((data->icld_IconLargestWidth - active_entry->ile_AreaWidth)/2);
								}
								start_Y = active_entry->ile_IconY;
							}

							while (active_entry != NULL)
							{
#if defined(DEBUG_ILC_KEYEVENTS)
D(bug("[IconList] IconList__MUIM_HandleEvent: DOWN: Checking active @ %x\n", active_entry));
#endif
								if (data->icld_DisplayFlags & ICONLIST_DISP_VERTICAL)
								{
									if (active_entry->ile_Flags & ICONENTRY_FLAG_VISIBLE)
										break;
								}
								else
								{
									active_X = active_entry->ile_IconX;

									if (data->icld__Option_IconListMode == ICON_LISTMODE_GRID)
									{
										if (active_entry->ile_AreaWidth < data->icld_IconLargestWidth)
											x_diff = ((data->icld_IconLargestWidth - active_entry->ile_AreaWidth)/2);
									}
									active_Y = active_entry->ile_IconY;

									if (start_entry)
									{
										if (entry_next)
										{
											if ((active_entry->ile_Flags & ICONENTRY_FLAG_VISIBLE) &&
												(active_Y > start_Y) &&
												(((active_X - x_diff) >= start_X ) &&
												((active_X - x_diff) <= (start_X + start_entry->ile_AreaWidth + (x_diff*2)))))
											{
#if defined(DEBUG_ILC_KEYEVENTS)
D(bug("[IconList] IconList__MUIM_HandleEvent: DOWN: (A) entry %x matches\n", active_entry));
#endif
												break;
											}
											else if (active_entry == (struct IconEntry *)GetTail(&data->icld_IconList))
											{
#if defined(DEBUG_ILC_KEYEVENTS)
D(bug("[IconList] IconList__MUIM_HandleEvent: DOWN: (A) reached list end .. starting at the beginng ..\n"));
#endif
												start_X = entry_next->ile_IconX;
												if (data->icld__Option_IconListMode == ICON_LISTMODE_GRID)
												{
													if (entry_next->ile_AreaWidth < data->icld_IconLargestWidth)
														start_X = start_X - ((data->icld_IconLargestWidth - entry_next->ile_AreaWidth)/2);
												}

												if ((entry_next = (struct IconEntry *)Node_NextVisible(entry_next)))
												{
													if (entry_next->ile_IconX < start_X)
														entry_next = NULL;
													else
													{
														next_X = entry_next->ile_IconX;
														if (data->icld__Option_IconListMode == ICON_LISTMODE_GRID)
														{
															if (entry_next->ile_AreaWidth < data->icld_IconLargestWidth)
																next_X = next_X + ((data->icld_IconLargestWidth - entry_next->ile_AreaWidth)/2);
														}
													}
												}
												start_Y = 0;
#if defined(DEBUG_ILC_KEYEVENTS)
D(bug("[IconList] IconList__MUIM_HandleEvent: DOWN: (A) startx = %d, start_Y = %d, next_X = %d, entry_next @ %x\n", start_X, start_Y, next_X, entry_next));
#endif
												active_entry = Node_FirstVisible(&data->icld_IconList);
											}
										}
										else
										{
											if ((active_entry->ile_Flags & ICONENTRY_FLAG_VISIBLE) &&
												(active_Y > start_Y) &&
												(active_X > start_X - 1))
											{
#if defined(DEBUG_ILC_KEYEVENTS)
D(bug("[IconList] IconList__MUIM_HandleEvent: DOWN: (B) entry %x matches\n", active_entry));
#endif
												break;
											}
										}
									}
									else
									{
										if (active_entry->ile_Flags & ICONENTRY_FLAG_VISIBLE)
										{
#if defined(DEBUG_ILC_KEYEVENTS)
D(bug("[IconList] IconList__MUIM_HandleEvent: DOWN: (C) entry %x matches\n", active_entry));
#endif
											break;
										}
									}
								}
								active_entry = (struct IconEntry *)GetSucc(active_entry);
							}

							if (!(active_entry))
							{
#if defined(DEBUG_ILC_KEYEVENTS)
D(bug("[IconList] IconList__MUIM_HandleEvent: DOWN: No Next DOWN Node - Getting first visable icon ..\n"));
#endif
								/* We didnt find a "next DOWN" icon so just use the first visible */
								active_entry =  Node_FirstVisible(&data->icld_IconList);
							}
							
							if (active_entry)
							{
								if (!(active_entry->ile_Flags & ICONENTRY_FLAG_FOCUS))
								{
									active_entry->ile_Flags |= ICONENTRY_FLAG_FOCUS;
									data->icld_UpdateMode = UPDATE_SINGLEICON;
									data->update_icon = active_entry;
									MUI_Redraw(obj, MADF_DRAWUPDATE);
								}
								
							}
							data->icld_FocusIcon = active_entry;
							break;

						case RAWKEY_LEFT:
							rawkey_handled = TRUE;

#if defined(DEBUG_ILC_KEYEVENTS)
D(bug("[IconList] IconList__MUIM_HandleEvent: RAWKEY_LEFT\n"));
#endif

							if (data->icld_FocusIcon)
							{
								start_entry = data->icld_FocusIcon;
#if defined(DEBUG_ILC_KEYEVENTS)
D(bug("[IconList] IconList__MUIM_HandleEvent: LEFT: Clearing existing focused icon @ %x\n", start_entry));
#endif

								start_entry->ile_Flags &= ~ICONENTRY_FLAG_FOCUS;
								data->icld_UpdateMode = UPDATE_SINGLEICON;
								data->update_icon = start_entry;
								MUI_Redraw(obj, MADF_DRAWUPDATE);

								start_X = start_entry->ile_IconX;
								if (data->icld__Option_IconListMode == ICON_LISTMODE_GRID)
								{
									if (start_entry->ile_AreaWidth < data->icld_IconLargestWidth)
										start_X = start_X - ((data->icld_IconLargestWidth - start_entry->ile_AreaWidth)/2);
								}
								start_Y = start_entry->ile_IconY;

#if defined(DEBUG_ILC_KEYEVENTS)
D(bug("[IconList] IconList__MUIM_HandleEvent: LEFT: start_X %d, start_Y %d\n", start_X, start_Y));
#endif

								if (!(active_entry = Node_NextVisible(start_entry)) && (!(data->icld_DisplayFlags & ICONLIST_DISP_VERTICAL)))
								{
									active_entry = (struct IconEntry *)GetHead(&data->icld_IconList);
#if defined(DEBUG_ILC_KEYEVENTS)
D(bug("[IconList] IconList__MUIM_HandleEvent: LEFT: Start at the beginning (Active @ %x) using icon X + Width\n", active_entry));
#endif
									start_X = start_X + start_entry->ile_AreaWidth;
									if (data->icld__Option_IconListMode == ICON_LISTMODE_GRID)
									{
										if (start_entry->ile_AreaWidth < data->icld_IconLargestWidth)
											start_X = start_X + ((data->icld_IconLargestWidth - start_entry->ile_AreaWidth)/2);
									}

									start_Y = 0;
									entry_next = NULL;
								}
								else if (active_entry && (!(data->icld_DisplayFlags & ICONLIST_DISP_VERTICAL)))
								{
#if defined(DEBUG_ILC_KEYEVENTS)
D(bug("[IconList] IconList__MUIM_HandleEvent: LEFT: Active @ %x, X %d\n", active_entry, active_entry->ile_IconX));
#endif
									if ((entry_next = Node_NextVisible(start_entry)))
									{
#if defined(DEBUG_ILC_KEYEVENTS)
D(bug("[IconList] IconList__MUIM_HandleEvent: LEFT: Next @ %x, X %d\n", entry_next, entry_next->ile_IconX));
#endif

										if (entry_next->ile_IconX < start_X)
											entry_next = NULL;
										else
										{
											next_X = entry_next->ile_IconX;
											if (data->icld__Option_IconListMode == ICON_LISTMODE_GRID)
											{
												if (entry_next->ile_AreaWidth < data->icld_IconLargestWidth)
													next_X = next_X - ((data->icld_IconLargestWidth - entry_next->ile_AreaWidth)/2);
											}
										}
									}
								}
#if defined(DEBUG_ILC_KEYEVENTS)
D(bug("[IconList] IconList__MUIM_HandleEvent: LEFT: using start_X %d, start_Y %d\n", start_X, start_Y));
#endif
							}

						    if (!(message->imsg->Qualifier & IEQUALIFIER_LSHIFT) && ((data->icld_SelectionFirst)||(data->icld_SelectionLast)))
							{
#if defined(DEBUG_ILC_KEYEVENTS)
D(bug("[IconList] IconList__MUIM_HandleEvent: LEFT: Clearing selected icons ..\n"));
#endif
								DoMethod(obj, MUIM_IconList_UnselectAll);
							}

#if defined(DEBUG_ILC_KEYEVENTS)
D(bug("[IconList] IconList__MUIM_HandleEvent: LEFT: active = %x, next = %x\n", active_entry, entry_next));
#endif

							if (!(active_entry))
							{
								active_entry = (struct IconEntry *)GetHead(&data->icld_IconList);
							}

							while (active_entry != NULL)
							{
#if defined(DEBUG_ILC_KEYEVENTS)
D(bug("[IconList] IconList__MUIM_HandleEvent: LEFT: Checking active @ %x\n", active_entry));
#endif
								if (data->icld_DisplayFlags & ICONLIST_DISP_VERTICAL)
								{
									if (active_entry->ile_Flags & ICONENTRY_FLAG_VISIBLE)
										break;
								}
								else
								{
									LONG active_entry_X = active_entry->ile_IconX;
									if (data->icld__Option_IconListMode == ICON_LISTMODE_GRID)
									{
										if (active_entry->ile_AreaWidth < data->icld_IconLargestWidth)
											active_entry_X = active_entry_X - ((data->icld_IconLargestWidth - active_entry->ile_AreaWidth)/2);
									}
									LONG active_entry_Y = active_entry->ile_IconY;
									
									if (start_entry)
									{
										if (entry_next)
										{
											if ((active_entry->ile_Flags & ICONENTRY_FLAG_VISIBLE) &&
												(active_entry_Y > start_Y) &&
												((active_entry_X > start_X - 1) &&
												(active_entry_X < next_X)))
											{
#if defined(DEBUG_ILC_KEYEVENTS)
D(bug("[IconList] IconList__MUIM_HandleEvent: LEFT: (A) entry %x matches\n", active_entry));
#endif
												break;
											}
											else if (active_entry == (struct IconEntry *)GetTail(&data->icld_IconList))
											{
#if defined(DEBUG_ILC_KEYEVENTS)
D(bug("[IconList] IconList__MUIM_HandleEvent: LEFT: (A) reached list end .. starting at the beginng ..\n"));
#endif
												start_X = entry_next->ile_IconX;
												if (data->icld__Option_IconListMode == ICON_LISTMODE_GRID)
												{
													if (entry_next->ile_AreaWidth < data->icld_IconLargestWidth)
														start_X = start_X - ((data->icld_IconLargestWidth - entry_next->ile_AreaWidth)/2);
												}

												if ((entry_next = Node_NextVisible(entry_next)))
												{
													if (entry_next->ile_IconX < start_X)
														entry_next = NULL;
													else
													{
														next_X = entry_next->ile_IconX;
														if (data->icld__Option_IconListMode == ICON_LISTMODE_GRID)
														{
															if (entry_next->ile_AreaWidth < data->icld_IconLargestWidth)
																next_X = next_X + ((data->icld_IconLargestWidth - entry_next->ile_AreaWidth)/2);
														}
													}
												}
												start_Y = 0;
#if defined(DEBUG_ILC_KEYEVENTS)
D(bug("[IconList] IconList__MUIM_HandleEvent: LEFT: (A) startx = %d, start_Y = %d, next_X = %d, entry_next @ %x\n", start_X, start_Y, next_X, entry_next));
#endif
												active_entry = (struct IconEntry *)GetHead(&data->icld_IconList);
											}
										}
										else
										{
											if ((active_entry->ile_Flags & ICONENTRY_FLAG_VISIBLE) &&
												(active_entry_Y > start_Y) &&
												(active_entry_X > start_X - 1))
											{
#if defined(DEBUG_ILC_KEYEVENTS)
D(bug("[IconList] IconList__MUIM_HandleEvent: LEFT: (B) entry %x matches\n", active_entry));
#endif
												break;
											}
										}
									}
									else
									{
										if (active_entry->ile_Flags & ICONENTRY_FLAG_VISIBLE)
										{
#if defined(DEBUG_ILC_KEYEVENTS)
D(bug("[IconList] IconList__MUIM_HandleEvent: LEFT: (C) entry %x matches\n", active_entry));
#endif
											break;
										}
									}
								}
								active_entry = (struct IconEntry *)GetSucc(active_entry);
							}

							if (!(active_entry))
							{
#if defined(DEBUG_ILC_KEYEVENTS)
D(bug("[IconList] IconList__MUIM_HandleEvent: LEFT: No Next LEFT Node - Getting first visable icon ..\n"));
#endif
								/* We didnt find a "next LEFT" icon so just use the last visible */
								active_entry =  (struct IconEntry *)GetHead(&data->icld_IconList);
								while ((active_entry != NULL) &&(!(active_entry->ile_Flags & ICONENTRY_FLAG_VISIBLE)))
								{
									active_entry = (struct IconEntry *)GetSucc(active_entry);
								}
							}
							
							if (active_entry)
							{
								if (!(active_entry->ile_Flags & ICONENTRY_FLAG_FOCUS))
								{
									active_entry->ile_Flags |= ICONENTRY_FLAG_FOCUS;
									data->icld_UpdateMode = UPDATE_SINGLEICON;
									data->update_icon = active_entry;
									MUI_Redraw(obj, MADF_DRAWUPDATE);
								}
								
							}
							data->icld_FocusIcon = active_entry;
							break;

						case RAWKEY_RIGHT:
							rawkey_handled = TRUE;

#if defined(DEBUG_ILC_KEYEVENTS)
D(bug("[IconList] IconList__MUIM_HandleEvent: RAWKEY_RIGHT\n"));
#endif

							if (data->icld_FocusIcon)
							{
								start_entry = data->icld_FocusIcon;
#if defined(DEBUG_ILC_KEYEVENTS)
D(bug("[IconList] IconList__MUIM_HandleEvent: RIGHT: Clearing existing focused icon @ %x\n", start_entry));
#endif
								start_entry->ile_Flags &= ~ICONENTRY_FLAG_FOCUS;
								data->icld_UpdateMode = UPDATE_SINGLEICON;
								data->update_icon = start_entry;
								MUI_Redraw(obj, MADF_DRAWUPDATE);

								start_X = start_entry->ile_IconX;
								if (data->icld__Option_IconListMode == ICON_LISTMODE_GRID)
								{
									if (start_entry->ile_AreaWidth < data->icld_IconLargestWidth)
										start_X = start_X - ((data->icld_IconLargestWidth - start_entry->ile_AreaWidth)/2);
								}
								start_Y = start_entry->ile_IconY;

#if defined(DEBUG_ILC_KEYEVENTS)
D(bug("[IconList] IconList__MUIM_HandleEvent: RIGHT: start_X %d, start_Y %d\n", start_X, start_Y));
#endif
								if (!(active_entry = Node_NextVisible(start_entry)) && (!(data->icld_DisplayFlags & ICONLIST_DISP_VERTICAL)))
								{
									active_entry = (struct IconEntry *)GetHead(&data->icld_IconList);
#if defined(DEBUG_ILC_KEYEVENTS)
D(bug("[IconList] IconList__MUIM_HandleEvent: RIGHT: Start at the beginning (Active @ %x) using icon X + Width\n", active_entry));
#endif
									start_X = 0;
									start_Y = start_Y + start_entry->ile_AreaHeight;
									entry_next = NULL;
								}
								else if (active_entry && (data->icld_DisplayFlags & ICONLIST_DISP_VERTICAL))
								{
#if defined(DEBUG_ILC_KEYEVENTS)
D(bug("[IconList] IconList__MUIM_HandleEvent: RIGHT: Active @ %x, X %d\n", active_entry, active_entry->ile_IconX));
#endif
									if ((entry_next = Node_NextVisible(start_entry)))
									{
#if defined(DEBUG_ILC_KEYEVENTS)
D(bug("[IconList] IconList__MUIM_HandleEvent: RIGHT: Next @ %x, X %d\n", entry_next, entry_next->ile_IconX));
#endif

										if (entry_next->ile_IconY < start_Y)
											entry_next = NULL;
										else
											next_Y = entry_next->ile_IconY;
									}
								}
#if defined(DEBUG_ILC_KEYEVENTS)
D(bug("[IconList] IconList__MUIM_HandleEvent: RIGHT: using start_X %d, start_Y %d\n", start_X, start_Y));
#endif
							}

						    if (!(message->imsg->Qualifier & IEQUALIFIER_LSHIFT) && ((data->icld_SelectionFirst)||(data->icld_SelectionLast)))
							{
#if defined(DEBUG_ILC_KEYEVENTS)
D(bug("[IconList] IconList__MUIM_HandleEvent: RIGHT: Clearing selected icons ..\n"));
#endif
								DoMethod(obj, MUIM_IconList_UnselectAll);
							}

#if defined(DEBUG_ILC_KEYEVENTS)
D(bug("[IconList] IconList__MUIM_HandleEvent: RIGHT: active = %x, next = %x\n", active_entry, entry_next));
#endif

							if (!(active_entry))
							{
								active_entry = (struct IconEntry *)GetHead(&data->icld_IconList);
							}

							while (active_entry != NULL)
							{
#if defined(DEBUG_ILC_KEYEVENTS)
D(bug("[IconList] IconList__MUIM_HandleEvent: RIGHT: Checking active @ %x\n", active_entry));
#endif
								if (!(data->icld_DisplayFlags & ICONLIST_DISP_VERTICAL))
								{
									if (active_entry->ile_Flags & ICONENTRY_FLAG_VISIBLE)
										break;
								}
								else
								{
									LONG active_entry_X = active_entry->ile_IconX;
									if (data->icld__Option_IconListMode == ICON_LISTMODE_GRID)
									{
										if (active_entry->ile_AreaWidth < data->icld_IconLargestWidth)
											active_entry_X = active_entry_X - ((data->icld_IconLargestWidth - active_entry->ile_AreaWidth)/2);
									}
									LONG active_entry_Y = active_entry->ile_IconY;
									
									if (start_entry)
									{
										if (entry_next)
										{
											if ((active_entry->ile_Flags & ICONENTRY_FLAG_VISIBLE) &&
												(active_entry_X > start_X) &&
												((active_entry_Y > start_Y - 1) &&
												(active_entry_Y < next_Y)))
											{
#if defined(DEBUG_ILC_KEYEVENTS)
D(bug("[IconList] IconList__MUIM_HandleEvent: RIGHT: (A) entry %x matches\n", active_entry));
#endif
												break;
											}
											else if (active_entry == (struct IconEntry *)GetTail(&data->icld_IconList))
											{
#if defined(DEBUG_ILC_KEYEVENTS)
D(bug("[IconList] IconList__MUIM_HandleEvent: RIGHT: (A) reached list end .. starting at the beginng ..\n"));
#endif
												start_Y = entry_next->ile_IconY;

												if ((entry_next = Node_NextVisible(entry_next)))
												{
													if (entry_next->ile_IconY < start_Y)
														entry_next = NULL;
													else
													{
														next_Y = entry_next->ile_IconY;
													}
												}
												start_Y = 0;
#if defined(DEBUG_ILC_KEYEVENTS)
D(bug("[IconList] IconList__MUIM_HandleEvent: RIGHT: (A) startx = %d, start_Y = %d, next_X = %d, entry_next @ %x\n", start_X, start_Y, next_X, entry_next));
#endif
												active_entry = (struct IconEntry *)GetHead(&data->icld_IconList);
											}
										}
										else
										{
											if ((active_entry->ile_Flags & ICONENTRY_FLAG_VISIBLE) &&
												(active_entry_X > start_X) &&
												(active_entry_Y > start_Y - 1))
											{
#if defined(DEBUG_ILC_KEYEVENTS)
D(bug("[IconList] IconList__MUIM_HandleEvent: RIGHT: (B) entry %x matches\n", active_entry));
#endif
												break;
											}
										}
									}
									else
									{
										if (active_entry->ile_Flags & ICONENTRY_FLAG_VISIBLE)
										{
#if defined(DEBUG_ILC_KEYEVENTS)
D(bug("[IconList] IconList__MUIM_HandleEvent: RIGHT: (C) entry %x matches\n", active_entry));
#endif
											break;
										}
									}
								}
								active_entry = (struct IconEntry *)GetSucc(active_entry);
							}

							if (!(active_entry))
							{
#if defined(DEBUG_ILC_KEYEVENTS)
D(bug("[IconList] IconList__MUIM_HandleEvent: RIGHT: No Next RIGHT Node - Getting first visable icon ..\n"));
#endif
								/* We didnt find a "next RIGHT" icon so just use the first visible */
								active_entry =  (struct IconEntry *)GetHead(&data->icld_IconList);
								while ((active_entry != NULL) &&(!(active_entry->ile_Flags & ICONENTRY_FLAG_VISIBLE)))
								{
									active_entry = (struct IconEntry *)GetSucc(active_entry);
								}
							}
							
							if (active_entry)
							{
								if (!(active_entry->ile_Flags & ICONENTRY_FLAG_FOCUS))
								{
									active_entry->ile_Flags |= ICONENTRY_FLAG_FOCUS;
									data->icld_UpdateMode = UPDATE_SINGLEICON;
									data->update_icon = active_entry;
									MUI_Redraw(obj, MADF_DRAWUPDATE);
								}
								
							}
							data->icld_FocusIcon = active_entry;
							break;

						case RAWKEY_HOME:
							rawkey_handled = TRUE;

#if defined(DEBUG_ILC_KEYEVENTS)
D(bug("[IconList] IconList__MUIM_HandleEvent: RAWKEY_HOME\n"));
#endif

							if (data->icld_FocusIcon)
							{
								data->icld_FocusIcon->ile_Flags &= ~ICONENTRY_FLAG_FOCUS;
								data->icld_UpdateMode = UPDATE_SINGLEICON;
								data->update_icon = data->icld_FocusIcon;
								MUI_Redraw(obj, MADF_DRAWUPDATE);
							}

							active_entry =  Node_FirstVisible(&data->icld_IconList);
							
							if ((active_entry) && (!(active_entry->ile_Flags & ICONENTRY_FLAG_FOCUS)))
							{
								active_entry->ile_Flags |= ICONENTRY_FLAG_FOCUS;
								data->icld_UpdateMode = UPDATE_SINGLEICON;
								data->update_icon = active_entry;
								MUI_Redraw(obj, MADF_DRAWUPDATE);
							}
							data->icld_FocusIcon = active_entry;
							break;

						case RAWKEY_END:
							rawkey_handled = TRUE;

#if defined(DEBUG_ILC_KEYEVENTS)
D(bug("[IconList] IconList__MUIM_HandleEvent: RAWKEY_END\n"));
#endif

							if (data->icld_FocusIcon)
							{
								data->icld_FocusIcon->ile_Flags &= ~ICONENTRY_FLAG_FOCUS;
								data->icld_UpdateMode = UPDATE_SINGLEICON;
								data->update_icon = data->icld_FocusIcon;
								MUI_Redraw(obj, MADF_DRAWUPDATE);
							}

							active_entry = Node_LastVisible(&data->icld_IconList);
							
							if ((active_entry) && (!(active_entry->ile_Flags & ICONENTRY_FLAG_FOCUS)))
							{
								active_entry->ile_Flags |= ICONENTRY_FLAG_FOCUS;
								data->icld_UpdateMode = UPDATE_SINGLEICON;
								data->update_icon = active_entry;
								MUI_Redraw(obj, MADF_DRAWUPDATE);
							}
							data->icld_FocusIcon = active_entry;
							break;
					}
				}
				if (rawkey_handled) return MUI_EventHandlerRC_Eat;
			}
            break;
    
            case IDCMP_MOUSEBUTTONS:

                if (message->imsg->Code == SELECTDOWN)
                {
                    /* check if mouse pressed on iconlist area */
                    if (mx >= 0 && mx < _width(obj) && my >= 0 && my < _height(obj))
                    {
                        struct IconEntry *node = NULL;
                        struct IconEntry *new_selected = NULL;

                        int selections = 0;
          
                        data->icld_SelectionFirst = NULL;

                        /* check if clicked on icon */
                        ForeachNode(&data->icld_IconList, node)
                        {
							if (node->ile_Flags & ICONENTRY_FLAG_VISIBLE)
							{
								/* count all OLD selections */
								if (node->ile_Flags & ICONENTRY_FLAG_SELECTED) selections++;

								if (((mx + data->icld_ViewX) >= node->ile_IconX  && (mx + data->icld_ViewX) < node->ile_IconX + node->ile_AreaWidth) &&
									((my + data->icld_ViewY) >= node->ile_IconY  && (my + data->icld_ViewY) < node->ile_IconY + node->ile_AreaHeight) &&
									!new_selected)
								{
									new_selected = node;
#if defined(DEBUG_ILC_EVENTS)
D(bug("[IconList] IconList__MUIM_HandleEvent: New Icon clicked on ..\n"));
#endif

								}
							}
                        }

						if (data->icld_FocusIcon)
						{
							if (!(new_selected) || (new_selected && (new_selected != data->icld_FocusIcon)))
							{
								data->icld_FocusIcon->ile_Flags &= ~(ICONENTRY_FLAG_FOCUS|ICONENTRY_FLAG_SELECTED);
								if ((data->icld_SelectionLast == data->icld_FocusIcon) && selections < 2) data->icld_SelectionLast = NULL;
								data->icld_UpdateMode = UPDATE_SINGLEICON;
								data->update_icon = data->icld_FocusIcon;
								MUI_Redraw(obj, MADF_DRAWUPDATE);
								data->icld_FocusIcon = NULL;
#if defined(DEBUG_ILC_EVENTS)
D(bug("[IconList] IconList__MUIM_HandleEvent: Cleared currently focused icon..\n"));
#endif
							}
						}

						if (data->icld_SelectionLast)
						{
							if (!(new_selected) || (new_selected && (new_selected != data->icld_SelectionLast) && selections < 2))
							{
								data->icld_SelectionLast->ile_Flags &= ~ICONENTRY_FLAG_SELECTED;
								data->icld_UpdateMode = UPDATE_SINGLEICON;
								data->update_icon = data->icld_SelectionLast;
								MUI_Redraw(obj,MADF_DRAWUPDATE);
#if defined(DEBUG_ILC_EVENTS)
D(bug("[IconList] IconList__MUIM_HandleEvent: ReDrawn last selected..\n"));
#endif
							}
						}
						
                        /* if not cliked on icon set lasso as active */
                        if (!new_selected)
                        {
                            data->icld_LassoActive = TRUE;

                            data->icld_LassoRectangle.MinX = mx - data->view_rect.MinX + data->icld_ViewX;  
                            data->icld_LassoRectangle.MinY = my - data->view_rect.MinY + data->icld_ViewY;
                            data->icld_LassoRectangle.MaxX = mx - data->view_rect.MinX + data->icld_ViewX;
                            data->icld_LassoRectangle.MaxY = my - data->view_rect.MinY + data->icld_ViewY; 
 
                            /* deselect old selection */
                            DoMethod(obj, MUIM_IconList_UnselectAll);

                            /* draw initial lasso */
                            IconList_InvertLassoOutlines(obj, &data->icld_LassoRectangle);
                        }
                        else
                        {
							BOOL update_icon = FALSE;
                            data->icld_LassoActive = FALSE;

							if (!(new_selected->ile_Flags & ICONENTRY_FLAG_SELECTED))
							{
								new_selected->ile_Flags |= ICONENTRY_FLAG_SELECTED;
								update_icon = TRUE;
							}
							if (!(new_selected->ile_Flags & ICONENTRY_FLAG_FOCUS))
							{
								new_selected->ile_Flags |= ICONENTRY_FLAG_FOCUS;
								update_icon = TRUE;
							}

							if (update_icon)
							{
								data->icld_UpdateMode = UPDATE_SINGLEICON;
								data->update_icon = new_selected;
								MUI_Redraw(obj, MADF_DRAWUPDATE);
#if defined(DEBUG_ILC_EVENTS)
D(bug("[IconList] IconList__MUIM_HandleEvent: Rendered selected icon..\n"));
#endif
							}
							data->icld_SelectionFirst = new_selected;
							data->icld_FocusIcon = new_selected;
                        }                       

                        data->icon_click.shift = !!(message->imsg->Qualifier & (IEQUALIFIER_LSHIFT | IEQUALIFIER_RSHIFT));
                        data->icon_click.entry = new_selected ? &new_selected->ile_IconListEntry : NULL;
                        SET(obj, MUIA_IconList_Clicked, (IPTR)&data->icon_click);

                        if (DoubleClick(data->last_secs, data->last_mics, message->imsg->Seconds, message->imsg->Micros) && data->icld_SelectionLast == new_selected)
                        {
                            SET(obj, MUIA_IconList_DoubleClick, TRUE);
                            data->icld_SelectionLast = NULL;
                        }
                        else if (!data->mouse_pressed)
                        {
                            data->icld_SelectionLast = new_selected;
                            data->last_secs = message->imsg->Seconds;
                            data->last_mics = message->imsg->Micros;
            
                            /* After a double click you often open a new window
                            * and since Zune doesn't not support the faking
                            * of SELECTUP events only change the Events
                            * if not doubleclicked */

                            data->mouse_pressed |= LEFT_BUTTON;

                            if (!(data->ehn.ehn_Events & IDCMP_MOUSEMOVE))
                            {
                                DoMethod(_win(obj), MUIM_Window_RemEventHandler, (IPTR)&data->ehn);
                                data->ehn.ehn_Events |= IDCMP_MOUSEMOVE;
                                DoMethod(_win(obj), MUIM_Window_AddEventHandler, (IPTR)&data->ehn);
                            }
                        }

                        data->click_x = mx;
                        data->click_y = my;

                        return MUI_EventHandlerRC_Eat;
                    }
                }
                else if (message->imsg->Code == MIDDLEDOWN)
                {
                    if (!data->mouse_pressed)
                    {
                        data->mouse_pressed |= MIDDLE_BUTTON;

                        data->click_x = data->icld_ViewX + mx;
                        data->click_y = data->icld_ViewY + my;

                        if (!(data->ehn.ehn_Events & IDCMP_MOUSEMOVE))
                        {
                            DoMethod(_win(obj), MUIM_Window_RemEventHandler, (IPTR)&data->ehn);
                            data->ehn.ehn_Events |= IDCMP_MOUSEMOVE;
                            DoMethod(_win(obj), MUIM_Window_AddEventHandler, (IPTR)&data->ehn);
                        }
                    }			
                }
                else
                {
                    if (message->imsg->Code == SELECTUP)
                    {
                        /* check if mose released on iconlist aswell */
                        if (mx >= 0 && mx < _width(obj) && my >= 0 && my < _height(obj) && data->icld_LassoActive == FALSE)
                        {
                            struct IconEntry *node = NULL;
                            //struct IconEntry *new_selected = NULL;
  
                            data->icld_SelectionFirst = NULL;

                            ForeachNode(&data->icld_IconList, node)
                            {
								if (node->ile_Flags & ICONENTRY_FLAG_VISIBLE)
								{
									/* unselect all other nodes if mouse released on icon and lasso was not selected during mouse press */
									if ( ( (node->ile_Flags & ICONENTRY_FLAG_SELECTED) && data->icld_LassoActive == FALSE) &&
										 ( mx < node->ile_IconX - data->icld_ViewX || mx > node->ile_IconX - data->icld_ViewX + node->ile_AreaWidth ||
										   my < node->ile_IconY - data->icld_ViewY || my > node->ile_IconY - data->icld_ViewY + node->ile_AreaHeight ) )
										   
									{
										  node->ile_Flags &= ~ICONENTRY_FLAG_SELECTED;
										  data->icld_UpdateMode = UPDATE_SINGLEICON;
										  data->update_icon = node;
										  MUI_Redraw(obj,MADF_DRAWUPDATE);
									}
								}
                            }
                        }                                           

                        /* stop lasso selection/drawing now */
                        if (data->icld_LassoActive == TRUE)
                        {
                            /* make lasso disappear again */
                            struct Rectangle old_lasso;
                            GetAbsoluteLassoRect(data, &old_lasso); 
                            IconList_InvertLassoOutlines(obj, &old_lasso);

                            data->icld_LassoActive = FALSE;
                        }

                        data->mouse_pressed &= ~LEFT_BUTTON;
                    }

                    if (message->imsg->Code == MIDDLEUP)
                    {
                        data->mouse_pressed &= ~MIDDLE_BUTTON;
                    }

                    if ((data->ehn.ehn_Events & IDCMP_MOUSEMOVE) && !data->mouse_pressed)
                    {
                        DoMethod(_win(obj),MUIM_Window_RemEventHandler, (IPTR)&data->ehn);
                        data->ehn.ehn_Events &= ~IDCMP_MOUSEMOVE;
                        DoMethod(_win(obj),MUIM_Window_AddEventHandler, (IPTR)&data->ehn);
                    }
                }
                break;
    
            case IDCMP_MOUSEMOVE:

                if (data->mouse_pressed & LEFT_BUTTON)
                {
                    LONG    move_x = mx;
                    LONG    move_y = my;

                    /* check if clicked on icon, or update lasso coords if lasso activated */
                    if (
                        data->icld_SelectionFirst && data->icld_LassoActive == FALSE && 
                        (abs(move_x - data->click_x) >= 2 || abs(move_y - data->click_y) >= 2)
                    )
                    {
                        DoMethod(_win(obj),MUIM_Window_RemEventHandler, (IPTR)&data->ehn);
                        data->ehn.ehn_Events &= ~IDCMP_MOUSEMOVE;
                        DoMethod(_win(obj),MUIM_Window_AddEventHandler, (IPTR)&data->ehn);
            
                        data->mouse_pressed &= ~LEFT_BUTTON;
            
                        data->touch_x = move_x + data->icld_ViewX - data->icld_SelectionFirst->ile_IconX;
                        data->touch_y = move_y + data->icld_ViewY - data->icld_SelectionFirst->ile_IconY;
                        DoMethod(obj,MUIM_DoDrag, data->touch_x, data->touch_y, 0);
                    }
                    else if (data->icld_LassoActive == TRUE) /* if no icon selected start lasso */
                    {
                        struct Rectangle    new_lasso,
											old_lasso;
                        struct IconEntry    *node = NULL;
                        struct IconEntry    *new_selected = NULL; 

                        /* unmark old lasso area */
                        GetAbsoluteLassoRect(data, &old_lasso);                          
                        IconList_InvertLassoOutlines(obj, &old_lasso);

                        /* if mouse leaves iconlist area during lasso mode, scroll view */
                        if (mx < 0 || mx >= _mwidth(obj) || my < 0 || my >= _mheight(obj))
                        {
                            LONG newleft = data->icld_ViewX;
                            LONG newtop = data->icld_ViewY;

                            if (mx >= _mwidth(obj)) newleft += 5;
                               else if (mx < 0) newleft -= 5;
                            if (my >= _mheight(obj)) newtop +=  5;
                               else if (my < 0) newtop -= 5;

                            if (newleft + _mwidth(obj) > data->icld_AreaWidth) newleft = data->icld_AreaWidth - _mwidth(obj);
                            if (newleft < 0) newleft = 0;

                            if (newtop + _mheight(obj) > data->icld_AreaHeight) newtop = data->icld_AreaHeight - _mheight(obj);
                            if (newtop < 0) newtop = 0;

                            if ((newleft != data->icld_ViewX) || (newtop != data->icld_ViewY))
                            {
                                SetAttrs(obj, MUIA_IconList_Left, newleft, MUIA_IconList_Top, newtop, TAG_DONE);
                            }
                        } 

                        /* update lasso coordinates */
                        data->icld_LassoRectangle.MaxX = mx - data->view_rect.MinX + data->icld_ViewX;
                        data->icld_LassoRectangle.MaxY = my - data->view_rect.MinY + data->icld_ViewY;

                        /* get absolute lasso coordinates */
                        GetAbsoluteLassoRect(data, &new_lasso);

                        data->icld_SelectionFirst = NULL;

                        ForeachNode(&data->icld_IconList, node)
                        {
							if (node->ile_Flags & ICONENTRY_FLAG_VISIBLE)
							{
								 /* check if clicked on icon */
								if (new_lasso.MaxX >= node->ile_IconX - data->icld_ViewX 
									 && new_lasso.MinX < node->ile_IconX - data->icld_ViewX + (LONG)node->ile_AreaWidth &&
									 new_lasso.MaxY >= node->ile_IconY - data->icld_ViewY 
									 && new_lasso.MinY < node->ile_IconY - data->icld_ViewY + (LONG)node->ile_AreaHeight) 
								{
									 new_selected = node;
			 
									 /* check if icon was already selected before */
									 if (!(node->ile_Flags & ICONENTRY_FLAG_SELECTED))
									 {
										 node->ile_Flags |= ICONENTRY_FLAG_SELECTED;
										 data->icld_UpdateMode = UPDATE_SINGLEICON;
										 data->update_icon = node;
										 data->icld_SelectionLast = node; // <- I'm the latest addition to your flock! =)
										 MUI_Redraw(obj, MADF_DRAWUPDATE);
									 }
					
									 data->icld_SelectionFirst = node;
								} 
								else
								{
									if (node->ile_Flags & ICONENTRY_FLAG_SELECTED)  /* if not catched by lasso and selected before -> unselect */
									{
										node->ile_Flags &= ~ICONENTRY_FLAG_SELECTED;
										data->icld_UpdateMode = UPDATE_SINGLEICON;
										data->update_icon = node;
										MUI_Redraw(obj, MADF_DRAWUPDATE);
									}
								} 
							}
                        }
                        /* set lasso borders */                         
			            IconList_InvertLassoOutlines(obj, &new_lasso);                        
                    }
                            
                    return MUI_EventHandlerRC_Eat;
                }
                else if (data->mouse_pressed & MIDDLE_BUTTON)
                {
                    LONG     newleft,
					         newtop;
        
                    newleft = data->click_x - mx;
                    newtop = data->click_y - my;
        
                    if (newleft + _mwidth(obj) > data->icld_AreaWidth) newleft = data->icld_AreaWidth - _mwidth(obj);
                    if (newleft < 0) newleft = 0;
        
                    if (newtop + _mheight(obj) > data->icld_AreaHeight) newtop = data->icld_AreaHeight - _mheight(obj);
                    if (newtop < 0) newtop = 0;
        
                    if ((newleft != data->icld_ViewX) || (newtop != data->icld_ViewY))
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
IPTR IconList__MUIM_IconList_NextSelected(struct IClass *CLASS, Object *obj, struct MUIP_IconList_NextSelected *message)
{
    struct MUI_IconData    *data = INST_DATA(CLASS, obj);
    struct IconEntry       *node = NULL;
    struct IconList_Entry  *ent = NULL;

    if (!message->entry) return (IPTR)NULL;
    ent = *message->entry;

    if (((IPTR)ent) == MUIV_IconList_NextSelected_Start)
    {
        if (!(node = data->icld_SelectionLast))
        {
D(bug("[IconList] IconList__MUIM_IconList_NextSelected: No selected entries!!!\n"));
            *message->entry = (struct IconList_Entry*)MUIV_IconList_NextSelected_End;
        } 
        else
        {
            /* get the first selected entry in list */
            node = (struct IconEntry *)GetHead(&data->icld_IconList);
            while (!(node->ile_Flags & ICONENTRY_FLAG_SELECTED))
            {
                node = (struct IconEntry *)GetSucc(node);
            }

            *message->entry = &node->ile_IconListEntry;
        }
        return 0;
    }

    node = (struct IconEntry *)GetHead(&data->icld_IconList); /* not really necessary but it avoids compiler warnings */

    node = (struct IconEntry*)(((char*)ent) - ((char*)(&node->ile_IconListEntry) - (char*)node));
    node = (struct IconEntry *)GetSucc(node);

    while (node)
    {
        if (node->ile_Flags & ICONENTRY_FLAG_SELECTED)
        {
            *message->entry = &node->ile_IconListEntry;
            return 0;
        }
        node = (struct IconEntry *)GetSucc(node);
    }

    *message->entry = (struct IconList_Entry*)MUIV_IconList_NextSelected_End;

    return (IPTR)NULL;
}

/**************************************************************************
MUIM_CreateDragImage
**************************************************************************/
IPTR IconList__MUIM_CreateDragImage(struct IClass *CLASS, Object *obj, struct MUIP_CreateDragImage *message)
{
    struct MUI_IconData     *data = INST_DATA(CLASS, obj);
    struct MUI_DragImage    *img = NULL;    
//	LONG                    first_x = -1,
//							first_y = -1;

    if (!data->icld_SelectionFirst) DoSuperMethodA(CLASS, obj, (Msg)message);

    if ((img = (struct MUI_DragImage *)AllocVec(sizeof(struct MUIP_CreateDragImage), MEMF_CLEAR)))
    {
        struct IconEntry *node = NULL;
        LONG depth = GetBitMapAttr(_screen(obj)->RastPort.BitMap, BMA_DEPTH);
    
        node = data->icld_SelectionFirst;

#if defined(CREATE_FULL_DRAGIMAGE)
		ForeachNode(&data->icld_IconList, node)
		{
			if ((node->ile_Flags & ICONENTRY_FLAG_VISIBLE) && (node->ile_Flags & ICONENTRY_FLAG_SELECTED))
			{
				if ((first_x == -1) || ((first_x != -1) && (node->ile_IconX < first_x))) first_x = node->ile_IconX;
				if ((first_y == -1) || ((first_y != -1) && (node->ile_IconY < first_y))) first_y = node->ile_IconY;
				if ((node->ile_IconX + node->ile_IconWidth) > img->width)   img->width = node->ile_IconX + node->ile_IconWidth;
				if ((node->ile_IconY + node->ile_IconHeight) > img->height) img->height = node->ile_IconY + node->ile_IconHeight;
			}
		}
        img->width = (img->width - first_x) + 2;
        img->height = (img->height - first_y) + 2;
#else
        img->width = node->ile_IconWidth;
        img->height = node->ile_IconHeight;
#endif
    
        if ((img->bm = AllocBitMap(img->width, img->height, depth, BMF_MINPLANES|BMF_CLEAR, _screen(obj)->RastPort.BitMap)))
        {
            struct RastPort temprp;
            InitRastPort(&temprp);
            temprp.BitMap = img->bm;
    
#if defined(CREATE_FULL_DRAGIMAGE)
			ForeachNode(&data->icld_IconList, node)
			{
				if ((node->ile_Flags & ICONENTRY_FLAG_VISIBLE) && (node->ile_Flags & ICONENTRY_FLAG_SELECTED))
				{
#if !defined(__AROS__)
					DrawIconState(
#else
					DrawIconStateA(
#endif
							&temprp, node->ile_DiskObj, NULL,
							(node->ile_IconX + 1) - first_x, (node->ile_IconY + 1) - first_y,
                            IDS_SELECTED,
#if !defined(__AROS__)
							ICONDRAWA_EraseBackground, TRUE,
#endif
							TAG_DONE);
				}
			}
#else
#if !defined(__AROS__)
			DrawIconState(
#else
			DrawIconStateA(
#endif
					&temprp, node->ile_DiskObj, NULL,
					0, 0,
					IDS_SELECTED,
#if !defined(__AROS__)
					ICONDRAWA_EraseBackground, TRUE,
#endif
					TAG_DONE);
#endif

            DeinitRastPort(&temprp);
        }
    
        img->touchx = message->touchx;
        img->touchy = message->touchy;
        img->flags = 0;
    }
    return (ULONG)img;
}

/**************************************************************************
MUIM_DeleteDragImage
**************************************************************************/
IPTR IconList__MUIM_DeleteDragImage(struct IClass *CLASS, Object *obj, struct MUIP_DeleteDragImage *message)
{
    struct MUI_IconData *data = INST_DATA(CLASS, obj);

    if (!data->icld_SelectionFirst) return DoSuperMethodA(CLASS,obj,(Msg)message);

    if (message->di)
    {
        if (message->di->bm)
            FreeBitMap(message->di->bm);
        FreeVec(message->di);
    }
    return (IPTR)NULL;
}

/**************************************************************************
MUIM_DragQuery
**************************************************************************/
IPTR IconList__MUIM_DragQuery(struct IClass *CLASS, Object *obj, struct MUIP_DragQuery *message)
{
    if (message->obj == obj) return MUIV_DragQuery_Accept;
    else
    {
        BOOL           is_iconlist = FALSE;
        struct IClass  *msg_cl = OCLASS(message->obj);
    
        while (msg_cl)
        {
            if (msg_cl == CLASS)
            {
                is_iconlist = TRUE;
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
IPTR IconList__MUIM_DragDrop(struct IClass *CLASS, Object *obj, struct MUIP_DragDrop *message)
{
    struct MUI_IconData *data = INST_DATA(CLASS, obj);

    /* check if dropped on same iconlist object */
    if (message->obj == obj)
    {
        struct IconEntry *icon = data->icld_SelectionFirst;

        if (icon)
        {
            struct Region       *region = NULL;
            struct Rectangle    rect_old,
                                rect_new;
            APTR    	        clip = NULL;

            /* icon moved, dropped in the same window */
            SET(obj, MUIA_IconList_IconsMoved, (IPTR) &(data->icld_SelectionFirst->ile_IconListEntry)); /* Now notify */
D(bug("[IconList] IconList__MUIM_DragDrop: move entry: %s dropped in same window\n", data->icld_SelectionFirst->ile_IconListEntry.filename); )
                
            IconList_GetIconRectangle(obj, data, icon, &rect_old);

            rect_old.MinX += _mleft(obj) - data->icld_ViewX + icon->ile_IconX;
            rect_old.MaxX += _mleft(obj) - data->icld_ViewX + icon->ile_IconX;
            rect_old.MinY += _mtop(obj) - data->icld_ViewY + icon->ile_IconY;
            rect_old.MaxY += _mtop(obj) - data->icld_ViewY + icon->ile_IconY;

            icon->ile_IconX = message->x - _mleft(obj) + data->icld_ViewX - data->touch_x;
            icon->ile_IconY = message->y - _mtop(obj) + data->icld_ViewY - data->touch_y;

            DoMethod(obj, MUIM_IconList_RethinkDimensions, data->icld_SelectionFirst);

            IconList_GetIconRectangle(obj, data, data->icld_SelectionFirst, &rect_new);
    
            rect_new.MinX += _mleft(obj) - data->icld_ViewX + icon->ile_IconX;
            rect_new.MaxX += _mleft(obj) - data->icld_ViewX + icon->ile_IconX;
            rect_new.MaxX += _mleft(obj) - data->icld_ViewX + icon->ile_IconX;
            rect_new.MinY += _mtop(obj) - data->icld_ViewY + icon->ile_IconY;
            rect_new.MaxY += _mtop(obj) - data->icld_ViewY + icon->ile_IconY;
    
            region = NewRegion();
            if (region)
            {
                OrRectRegion(region, &rect_old);
                OrRectRegion(region, &rect_new);
                clip = MUI_AddClipRegion(muiRenderInfo(obj), region);
            }

            MUI_Redraw(obj,MADF_DRAWOBJECT);

            if (region)
            {
                MUI_RemoveClipRegion(muiRenderInfo(obj), clip);
            }
			DoMethod(obj, MUIM_IconList_CoordsSort);
        }
    } 
    else
    {
        // struct IconEntry      *icon     = NULL;
        struct IconList_Entry *entry    = (APTR) MUIV_IconList_NextSelected_Start;

        /* get selected entries from SOURCE iconlist */
        DoMethod(message->obj, MUIM_IconList_NextSelected, (IPTR) &entry);

        /* check if dropped on an icon on the iconlist area */
        if (entry)
        {
           /* check if dropped on a drawer */
           struct IconEntry *node = NULL;
           struct IconEntry *drop_target_node = NULL;

           /* go through list and check if dropped on icon */
           ForeachNode(&data->icld_IconList, node)
           {
               if ((node->ile_Flags & ICONENTRY_FLAG_VISIBLE) &&
				   message->x >= node->ile_IconX - data->icld_ViewX && 
                   message->x <  node->ile_IconX - data->icld_ViewX + node->ile_AreaWidth  &&
                   message->y >= node->ile_IconY - data->icld_ViewY + _mtop(obj)  && 
                   message->y <  node->ile_IconY - data->icld_ViewY + node->ile_AreaHeight + _mtop(obj) && !drop_target_node)
               {
                   drop_target_node = node;
               } 
           }

           /* get path to destination directory */
           STRPTR directory_path = NULL;
		   GET(obj, MUIA_IconDrawerList_Drawer, &directory_path);

           /* check if dropped on a root drive - 
              last condition is a hack, based upon another hack (adding ":Disk" to rootdrive name) 
              since ST_ROOT seems not to be set properly right now (?) */
           if (drop_target_node && ((drop_target_node->ile_IconListEntry.type == ST_ROOT)
                                || (!strcmp(drop_target_node->ile_IconListEntry.filename + strlen(drop_target_node->ile_IconListEntry.filename) - 5, ":Disk"))))
           {
               int tmplen = 0;

               /* avoid copying "Disk" (hack anyway?!) from root drive name, eg. "Ram Disk:Disk"*/
               tmplen = strlen(drop_target_node->ile_IconListEntry.filename) - 4;

               /* copy path of dir icon dropped on */
               strncpy(data->drop_entry.destination_string, drop_target_node->ile_IconListEntry.filename, tmplen);

               /* mark the drive the icon was dropped on*/
               drop_target_node->ile_Flags |= ICONENTRY_FLAG_SELECTED;
               data->icld_UpdateMode = UPDATE_SINGLEICON;
               data->update_icon = drop_target_node;
               MUI_Redraw(obj,MADF_DRAWUPDATE);

D(bug("[IconList] IconList__MUIM_DragDrop: drop entry: %s dropped on disk icon %s\n", entry->filename, drop_target_node->ile_IconListEntry.filename); )
           }
           /* check if dropped on a drawer icon in iconlist */
           else if (drop_target_node && (drop_target_node->ile_IconListEntry.type == ST_USERDIR))
           {
               /* copy path of dir icon dropped on */
               strcpy(data->drop_entry.destination_string, drop_target_node->ile_IconListEntry.filename);

               /* mark the directory the icon was dropped on*/
               drop_target_node->ile_Flags |= ICONENTRY_FLAG_SELECTED;
               data->icld_UpdateMode = UPDATE_SINGLEICON;
               data->update_icon = drop_target_node;
               MUI_Redraw(obj,MADF_DRAWUPDATE);

D(bug("[IconList] IconList__MUIM_DragDrop: drop entry: %s dropped on dir %s icon in window %s\n", entry->filename, drop_target_node->ile_IconListEntry.filename,  directory_path); )
           }
           else
           {
               /* not dropped on icon -> get path of DESTINATION iconlist */
D(bug("[IconList] IconList__MUIM_DragDrop: drop entry: %s dropped in window %s\n", entry->filename, directory_path); )
               /* copy path */
               strcpy(data->drop_entry.destination_string, directory_path);
           }

           /* copy relevant data to drop entry */
           data->drop_entry.source_iconlistobj = message->obj;
           data->drop_entry.destination_iconlistobj = obj;
           
           /* return drop entry */
           SET(obj, MUIA_IconList_IconsDropped, (IPTR)&data->drop_entry); /* Now notify */
		   DoMethod(obj, MUIM_IconList_CoordsSort);
        }
        else
        {
           /* no drop entry */
           SET(obj, MUIA_IconList_IconsDropped, (IPTR)NULL); /* Now notify */
        }
        
    }
    return DoSuperMethodA(CLASS, obj, (Msg)message);
}

/**************************************************************************
MUIM_UnselectAll
**************************************************************************/
IPTR IconList__MUIM_IconList_UnselectAll(struct IClass *CLASS, Object *obj, Msg message)
{
    struct MUI_IconData *data = INST_DATA(CLASS, obj);
    struct IconEntry    *node = NULL;

    ForeachNode(&data->icld_IconList, node)
    {
		BOOL update_icon = FALSE;

        if (node->ile_Flags & ICONENTRY_FLAG_SELECTED)
        {
            node->ile_Flags &= ~ICONENTRY_FLAG_SELECTED;
			update_icon = TRUE;
		}
        if (node->ile_Flags & ICONENTRY_FLAG_FOCUS)
        {
            node->ile_Flags &= ~ICONENTRY_FLAG_FOCUS;
			update_icon = TRUE;
		}

		if (update_icon)
		{
            data->icld_UpdateMode = UPDATE_SINGLEICON;
            data->update_icon = node;
            MUI_Redraw(obj, MADF_DRAWUPDATE);
        }
    }
    data->icld_FocusIcon = NULL;
    data->icld_SelectionFirst = NULL;
    data->icld_SelectionLast = NULL;
    return 1;
}

/**************************************************************************
MUIM_SelectAll
**************************************************************************/
IPTR IconList__MUIM_IconList_SelectAll(struct IClass *CLASS, Object *obj, Msg message)
{
    struct MUI_IconData *data = INST_DATA(CLASS, obj);
    struct IconEntry    *node = NULL;

    node = (struct IconEntry *)GetHead(&data->icld_IconList);

	if (node)
	{
		data->icld_FocusIcon = node;
		node->ile_Flags &= ~(ICONENTRY_FLAG_SELECTED|ICONENTRY_FLAG_FOCUS);
		node->ile_Flags |= ICONENTRY_FLAG_FOCUS;

		while (node)
		{
			BOOL update_icon = FALSE;

			if (!(node->ile_Flags & ICONENTRY_FLAG_SELECTED))
			{
				node->ile_Flags |= ICONENTRY_FLAG_SELECTED;
				update_icon = TRUE;
			}

			if (update_icon)
			{
				data->icld_UpdateMode = UPDATE_SINGLEICON;
				data->update_icon = node;
				data->icld_SelectionLast = node;
				MUI_Redraw(obj, MADF_DRAWUPDATE);
			}
			node = (struct IconEntry *)GetSucc(node);
		}
	}

    return 1;
}

struct MUI_IconDrawerData
{
    char *drawer;
};

/**************************************************************************
Read icons in
**************************************************************************/
static int IconDrawerList__ParseContents(struct IClass *CLASS, Object *obj)
{
    struct MUI_IconDrawerData *data = INST_DATA(CLASS, obj);
    BPTR                      lock = NULL, tmplock = NULL;
    char                      filename[256];
	char                      namebuffer[512];
    ULONG                     list_DisplayFlags = 0;
	
    if (!data->drawer) return 1;

    lock = Lock(data->drawer, SHARED_LOCK);

    if (lock)
    {
        struct FileInfoBlock *fib = AllocDosObject(DOS_FIB, NULL);
        if (fib)
        {
            if (Examine(lock, fib))
            {
				GET(obj, MUIA_IconList_DisplayFlags, &list_DisplayFlags);
D(bug("[IconList] IconDrawerList__ParseContents: DisplayFlags = %x\n", list_DisplayFlags));

                while(ExNext(lock, fib))
                {
                    int len = strlen(fib->fib_FileName);
					memset(namebuffer, 0, 512);
                    strcpy(filename, fib->fib_FileName);

D(bug("[IconList] IconDrawerList__ParseContents: '%s', len = %d\n", filename, len));

                    if (len >= 5)
                    {
                        if (!Stricmp(&filename[len-5],".info"))
						{
							/* Its a .info file .. skip "disk.info" and just ".info" files*/
							if ((len == 5) || (!Stricmp(filename,"Disk")))
							{
D(bug("[IconList] IconDrawerList__ParseContents: Skiping file named disk.info or just .info ('%s')\n", filename));
								continue;
							}

							strcpy(namebuffer, data->drawer);
							memset((filename + len - 5), 0, 1); //Remove the .info section
							AddPart(namebuffer, filename, sizeof(namebuffer));
D(bug("[IconList] IconDrawerList__ParseContents: Checking for .info files real file '%s'\n", namebuffer));
							
							if ((tmplock = Lock(namebuffer, SHARED_LOCK)))
							{
								/* We have a real file so skip it for now and let it be found seperately */
D(bug("[IconList] IconDrawerList__ParseContents: File found .. skipping\n"));
								UnLock(tmplock); 
								continue;
							}
						}
                    }

D(bug("[IconList] IconDrawerList__ParseContents: Registering file '%s'\n", filename));
					strcpy(namebuffer, data->drawer);
					AddPart(namebuffer, filename, sizeof(namebuffer));

					struct IconEntry *this_Icon = NULL;
						
					if ((this_Icon = (struct IconEntry *)DoMethod(obj, MUIM_IconList_CreateEntry, (IPTR)namebuffer, (IPTR)filename, (IPTR)fib, (IPTR)NULL)))
					{
						sprintf(namebuffer + strlen(namebuffer), ".info");
						if ((tmplock = Lock(namebuffer, SHARED_LOCK)))
						{
D(bug("[IconList] IconDrawerList__ParseContents: File has a .info file .. updating info\n"));
							UnLock(tmplock); 
							if (!(this_Icon->ile_Flags & ICONENTRY_FLAG_HASICON)) 
								this_Icon->ile_Flags |= ICONENTRY_FLAG_HASICON;
						}

						if (list_DisplayFlags & ICONLIST_DISP_SHOWINFO)
						{
							if ((this_Icon->ile_Flags & ICONENTRY_FLAG_HASICON) && !(this_Icon->ile_Flags & ICONENTRY_FLAG_VISIBLE))
								this_Icon->ile_Flags |= ICONENTRY_FLAG_VISIBLE;
						}
						else if (!(this_Icon->ile_Flags & ICONENTRY_FLAG_VISIBLE))
						{
							this_Icon->ile_Flags |= ICONENTRY_FLAG_VISIBLE;
						}
						this_Icon->ile_Node.ln_Pri = 0;
					}
					else
					{
D(bug("[IconList] IconDrawerList__ParseContents: Failed to Register file!!!\n"));
					}
                }
            }
    
            FreeDosObject(DOS_FIB, fib);
        }
    
        UnLock(lock);
    }

    return 1;
}

/*
static int OLDReadIcons(struct IClass *CLASS, Object *obj)
{
struct MUI_IconDrawerData *data = INST_DATA(CLASS, obj);
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

*//*
ParsePatternNoCase("#?.info",pattern,sizeof(pattern));
eac->eac_MatchString = pattern;
*//*
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

    strcpy(filename,entry->ile_ed_Name);

    *//*
        // if we only display icons

        filename[strlen(filename)-5]=0;
    *//*
        len = strlen(filename);
    if (len >= 5)
    {
        *//* reject all .info files, so we have a Show All mode *//*
        if (!Stricmp(&filename[len-5],".info"))
            continue;
    }

    if (Stricmp(filename,"Disk")) *//* skip disk.info *//*
    {
        char buf[512];
        strcpy(buf,data->drawer);
        AddPart(buf,filename,sizeof(buf));

        if (!(DoMethod(obj,MUIM_IconList_CreateEntry,(IPTR)buf,(IPTR)filename,entry->ile_ed_Type,NULL *//* udata *//*)))
        {
        }
    }
    }   while ((entry = entry->ile_ed_Next));
} while (more);

CurrentDir(olddir);

FreeVec(ead);
FreeDosObject(DOS_EXALLCONTROL,eac);
UnLock(lock);
return 1;
}

*/
IPTR IconList__MUIM_IconList_CoordsSort(struct IClass *CLASS, Object *obj, struct MUIP_IconList_Sort *message)
{
    struct MUI_IconData *data = INST_DATA(CLASS, obj);

    struct IconEntry    *entry = NULL,
		                *test_icon = NULL;

    struct List         list_VisibleIcons;
    struct List         list_HiddenIcons;

	/*
		perform a quick sort of the iconlist based on icon coords
	    this method DOESNT cause any visual output.
	*/
#if defined(DEBUG_ILC_ICONSORTING)
D(bug("[IconList] IconList__MUIM_IconList_CoordsSort()\n"));
#endif

    NewList((struct List*)&list_VisibleIcons);
    NewList((struct List*)&list_HiddenIcons);

    /*move list into our local list struct(s)*/
    while ((entry = (struct IconEntry *)RemTail((struct List*)&data->icld_IconList)))
	{
        if (entry->ile_Flags & ICONENTRY_FLAG_VISIBLE)
			AddHead((struct List*)&list_VisibleIcons, (struct Node *)entry);
        else
			AddHead((struct List*)&list_HiddenIcons, (struct Node *)entry);
	}

    while ((entry = (struct IconEntry *)RemTail((struct List*)&list_VisibleIcons)))
    {
        test_icon = (struct IconEntry *)GetTail(&data->icld_IconList);

		while (test_icon != NULL)
		{
			if (((data->icld_DisplayFlags & ICONLIST_DISP_VERTICAL) && (test_icon->ile_IconX > entry->ile_IconX)) ||
				(!(data->icld_DisplayFlags & ICONLIST_DISP_VERTICAL) && (test_icon->ile_IconY > entry->ile_IconY)))
			{
				test_icon = (struct IconEntry *)GetPred(test_icon);
				continue;
			}
			else break;
		}

		while (test_icon != NULL)
		{
			if (((data->icld_DisplayFlags & ICONLIST_DISP_VERTICAL) && (test_icon->ile_IconY > entry->ile_IconY)) ||
				(!(data->icld_DisplayFlags & ICONLIST_DISP_VERTICAL) && (test_icon->ile_IconX > entry->ile_IconX)))
			{
				test_icon = (struct IconEntry *)GetPred(test_icon);
				continue;
			}
			else break;
		}
		Insert((struct List*)&data->icld_IconList, (struct Node *)entry, (struct Node *)test_icon);
	}
#if defined(DEBUG_ILC_ICONSORTING)
D(bug("[IconList] IconList__MUIM_IconList_CoordsSort: Done\n"));
#endif

    while ((entry = (struct IconEntry *)RemTail((struct List*)&list_HiddenIcons)))
	{
		AddTail((struct List*)&data->icld_IconList, (struct Node *)entry);
	}

#if defined(DEBUG_ILC_ICONSORTING_DUMP)
	ForeachNode(&data->icld_IconList, entry)
	{
D(bug("[IconList] IconList__MUIM_IconList_CoordsSort: %d   %d   '%s'\n", entry->ile_IconX, entry->ile_IconY, entry->ile_IconListEntry.label));
	}
#endif

	return TRUE;
}

/**************************************************************************
MUIM_Sort - sortsort
**************************************************************************/
IPTR IconList__MUIM_IconList_Sort(struct IClass *CLASS, Object *obj, struct MUIP_IconList_Sort *message)
{
    struct MUI_IconData *data = INST_DATA(CLASS, obj);
    struct IconEntry    *entry = NULL,
		                *icon1 = NULL,
	                    *icon2 = NULL;
    struct List         list_VisibleIcons,
		                list_SortedIcons,
		                list_HiddenIcons;

    BOOL                sortme;
    int                 i, visible_count = 0;

#if defined(DEBUG_ILC_ICONSORTING)
D(bug("[IconList] IconList__MUIM_IconList_Sort()\n"));
#endif

    NewList((struct List*)&list_VisibleIcons);
    NewList((struct List*)&list_SortedIcons);
    NewList((struct List*)&list_HiddenIcons);

    /*move list into our local list struct(s)*/
    while ((entry = (struct IconEntry *)RemTail((struct List*)&data->icld_IconList)))
	{
		if (!(entry->ile_Flags & ICONENTRY_FLAG_HASICON))
		{

			if (data->icld_DisplayFlags & ICONLIST_DISP_SHOWINFO)
			{
				if (entry->ile_Flags & ICONENTRY_FLAG_VISIBLE)
					entry->ile_Flags &= ~ICONENTRY_FLAG_VISIBLE;
			}
			else if (!(entry->ile_Flags & ICONENTRY_FLAG_VISIBLE))
				entry->ile_Flags |= ICONENTRY_FLAG_VISIBLE;
		}
		else
		{
			if (!(entry->ile_Flags & ICONENTRY_FLAG_VISIBLE))
				entry->ile_Flags |= ICONENTRY_FLAG_VISIBLE;
		}
		
		/* Now we have fixed visibility lets dump them into the correct list for sorting */
        if (entry->ile_Flags & ICONENTRY_FLAG_VISIBLE)
		{
			AddHead((struct List*)&list_VisibleIcons, (struct Node *)entry);
			visible_count++;
		}
        else AddHead((struct List*)&list_HiddenIcons, (struct Node *)entry);
	}

    /* Reset incase view options have changed .. */
    data->icld_IconLargestWidth = 0;
    data->icld_IconLargestHeight = 0;

    /* Copy each visible icon entry back to the main list, sorting as we go*/

    while ((entry = (struct IconEntry *)RemTail((struct List*)&list_VisibleIcons)))
    {
        icon1 = (struct IconEntry *)GetHead(&list_SortedIcons);
        icon2 = NULL;

		if (data->icld__Option_IconListMode == ICON_LISTMODE_GRID)
		{
			/* Force grid placement to recognise largest icon */
			if( entry->ile_AreaWidth > data->icld_IconLargestWidth ) data->icld_IconLargestWidth = entry->ile_AreaWidth;
			if( entry->ile_AreaHeight > data->icld_IconLargestHeight ) data->icld_IconLargestHeight = entry->ile_AreaHeight;
		}

        sortme = FALSE;

		if (visible_count > 1)
		{
//D(bug(" - %s %s %s %i\n",entry->ile_IconListEntry.label,entry->ile_TxtBuf_DATE,entry->ile_TxtBuf_TIME,entry->ile_FileInfoBlock.fib_Size));

			while (icon1)
			{
				if(data->icld_SortFlags & ICONLIST_SORT_DRAWERS_MIXED)
				{
					/*drawers mixed*/

					sortme = TRUE;
				}
				else
				{
					/*drawers first*/

					if ((icon1->ile_IconListEntry.type == WBDRAWER) && (entry->ile_IconListEntry.type == WBDRAWER))
					{
						sortme = TRUE;
					}
					else
					{
						if ((icon1->ile_IconListEntry.type != WBDRAWER) && (entry->ile_IconListEntry.type != WBDRAWER))
							sortme = TRUE;
						else
						{
							/* we are the first drawer to arrive or we need to insert ourselves
							   due to being sorted to the end of the drawers*/

							if ((!icon2 || icon2->ile_IconListEntry.type == WBDRAWER) &&
								(entry->ile_IconListEntry.type == WBDRAWER) &&
								(icon1->ile_IconListEntry.type != WBDRAWER))
							{
//D(bug("force %s\n",entry->ile_IconListEntry.label));
								break;
							}
						}
					}
				}
		
				if (sortme)
				{
					i = 0;
			
					if( (data->icld_SortFlags & ICONLIST_SORT_BY_DATE) && !(data->icld_SortFlags & ICONLIST_SORT_BY_SIZE) )
					{
						/* Sort by Date */
						i = CompareDates((const struct DateStamp *)&entry->ile_FileInfoBlock.fib_Date,(const struct DateStamp *)&icon1->ile_FileInfoBlock.fib_Date);
//D(bug("     -  %i\n",i));
					}
					else
					{
						if( (data->icld_SortFlags & ICONLIST_SORT_BY_SIZE) && !(data->icld_SortFlags & ICONLIST_SORT_BY_DATE) )
						{
							/* Sort by Size .. */
							i = entry->ile_FileInfoBlock.fib_Size - icon1->ile_FileInfoBlock.fib_Size;
//D(bug("     -  %i\n",i));
						}
						else if( data->icld_SortFlags & (ICONLIST_SORT_BY_DATE | ICONLIST_SORT_BY_SIZE) )
						{
						   /* Sort by Type .. */
						}
						else
						{
							/* Sort by Name .. */
							i = Stricmp(entry->ile_IconListEntry.label, icon1->ile_IconListEntry.label);
						}
					}

					if (!(data->icld_SortFlags & ICONLIST_SORT_REVERSE) && (i < 0))
						break;

					if ((data->icld_SortFlags & ICONLIST_SORT_REVERSE) && (i > 0))
						break;
				}
				icon2 = icon1;
				icon1 = (struct IconEntry *)GetSucc( icon1 );
			}
		}
        Insert((struct List*)&list_SortedIcons, (struct Node *)entry, (struct Node *)icon2);
    }

	/* Quickly resort based on node priorities .. */
    while ((entry = (struct IconEntry *)RemHead((struct List*)&list_SortedIcons)))
	{
		Enqueue((struct List*)&data->icld_IconList, (struct Node *)entry);
	}

    DoMethod(obj, MUIM_IconList_PositionIcons);
    MUI_Redraw(obj, MADF_DRAWOBJECT);

#warning "TODO: leave hidden icons on a seperate list to speed up normal list parsing"
    while ((entry = (struct IconEntry *)RemTail((struct List*)&list_HiddenIcons)))
	{
		AddTail((struct List*)&data->icld_IconList, (struct Node *)entry);
	}

    return 1;
}

/**************************************************************************
MUIM_DragReport. Since MUI doesn't change the drop object if the dragged
object is moved above another window (while still in the bounds of the
orginal drop object) we must do it here manually to be compatible with
MUI. Maybe Zune should fix this bug somewhen.
**************************************************************************/
IPTR IconList__MUIM_DragReport(struct IClass *CLASS, Object *obj, struct MUIP_DragReport *message)
{
    struct Window   *wnd = _window(obj);
    struct Layer    *l = NULL;

    l = WhichLayer(&wnd->WScreen->LayerInfo, wnd->LeftEdge + message->x, wnd->TopEdge + message->y);

    if (l != wnd->WLayer) return MUIV_DragReport_Abort;

    return MUIV_DragReport_Continue;
}

/**************************************************************************
 MUIM_IconList_UnknownDropDestination
**************************************************************************/

IPTR IconList__MUIM_UnknownDropDestination(struct IClass *CLASS, Object *obj, struct MUIP_UnknownDropDestination *message)
{
D(bug("[IconList] IconList__MUIM_UnknownDropDestination: icons dropped on custom window \n"));

    SET(obj, MUIA_IconList_AppWindowDrop, (IPTR)message); /* Now notify */

    return 0;
}

/*************************************************************************/

BOOPSI_DISPATCHER(IPTR,IconList_Dispatcher, CLASS, obj, message)
{
    switch (message->MethodID)
    {
        case OM_NEW:                            return IconList__OM_NEW(CLASS, obj, (struct opSet *)message);
        case OM_DISPOSE:                        return IconList__OM_DISPOSE(CLASS, obj, message);
        case OM_SET:                            return IconList__OM_SET(CLASS, obj, (struct opSet *)message);
        case OM_GET:                            return IconList__OM_GET(CLASS, obj, (struct opGet *)message);
        
        case MUIM_Setup:                        return IconList__MUIM_Setup(CLASS, obj, (struct MUIP_Setup *)message);
        
        case MUIM_Show:                         return IconList__MUIM_Show(CLASS,obj, (struct MUIP_Show *)message);
        case MUIM_Cleanup:                      return IconList__MUIM_Cleanup(CLASS, obj, (struct MUIP_Cleanup *)message);
        case MUIM_AskMinMax:                    return IconList__MUIM_AskMinMax(CLASS, obj, (struct MUIP_AskMinMax *)message);
        case MUIM_Draw:                         return IconList__MUIM_Draw(CLASS, obj, (struct MUIP_Draw *)message);
        case MUIM_Layout:                       return IconList__MUIM_Layout(CLASS, obj, (struct MUIP_Layout *)message);
        case MUIM_HandleEvent:                  return IconList__MUIM_HandleEvent(CLASS, obj, (struct MUIP_HandleEvent *)message);
        case MUIM_CreateDragImage:              return IconList__MUIM_CreateDragImage(CLASS, obj, (APTR)message);
        case MUIM_DeleteDragImage:              return IconList__MUIM_DeleteDragImage(CLASS, obj, (APTR)message);
        case MUIM_DragQuery:                    return IconList__MUIM_DragQuery(CLASS, obj, (APTR)message);
        case MUIM_DragReport:                   return IconList__MUIM_DragReport(CLASS, obj, (APTR)message);
        case MUIM_DragDrop:                     return IconList__MUIM_DragDrop(CLASS, obj, (APTR)message);
        case MUIM_UnknownDropDestination:       return IconList__MUIM_UnknownDropDestination(CLASS, obj, (APTR)message);       
        case MUIM_IconList_Update:              return IconList__MUIM_IconList_Update(CLASS, obj, (APTR)message);
        case MUIM_IconList_Clear:               return IconList__MUIM_IconList_Clear(CLASS, obj, (APTR)message);
        case MUIM_IconList_RethinkDimensions:   return IconList__MUIM_IconList_RethinkDimensions(CLASS, obj, (APTR)message);
        case MUIM_IconList_CreateEntry:         return IconList__MUIM_IconList_CreateEntry(CLASS, obj, (APTR)message);
        case MUIM_IconList_DestroyEntry:        return IconList__MUIM_IconList_DestroyEntry(CLASS, obj, (APTR)message);
		case MUIM_IconList_DrawEntry:           return IconList__MUIM_IconList_DrawEntry(CLASS, obj, (APTR)message);
		case MUIM_IconList_DrawEntryLabel:      return IconList__MUIM_IconList_DrawEntryLabel(CLASS, obj, (APTR)message);
        case MUIM_IconList_NextSelected:        return IconList__MUIM_IconList_NextSelected(CLASS, obj, (APTR)message);
        case MUIM_IconList_UnselectAll:         return IconList__MUIM_IconList_UnselectAll(CLASS, obj, (APTR)message);
        case MUIM_IconList_Sort:                return IconList__MUIM_IconList_Sort(CLASS, obj, (APTR)message);
        case MUIM_IconList_CoordsSort:          return IconList__MUIM_IconList_CoordsSort(CLASS, obj, (APTR)message);
        case MUIM_IconList_PositionIcons:       return IconList__MUIM_IconList_PositionIcons(CLASS, obj, (APTR)message);
        case MUIM_IconList_SelectAll:           return IconList__MUIM_IconList_SelectAll(CLASS, obj, (APTR)message);
//      case MUIM_IconList_ViewIcon:            return IconList__MUIM_IconList_ViewIcon(CLASS, obj, (APTR)message);
    }

    return DoSuperMethodA(CLASS, obj, message);
}
BOOPSI_DISPATCHER_END


/**************************************************************************
OM_NEW
**************************************************************************/
IPTR IconDrawerList__OM_NEW(struct IClass *CLASS, Object *obj, struct opSet *message)
{
    struct MUI_IconDrawerData   *data = NULL;
    struct TagItem  	        *tag = NULL,
                                *tags = NULL;

    obj = (Object *)DoSuperNewTags(CLASS, obj, NULL,
        TAG_MORE, (IPTR) message->ops_AttrList);

    if (!obj) return FALSE;

    data = INST_DATA(CLASS, obj);

    /* parse initial taglist */
    for (tags = message->ops_AttrList; (tag = NextTagItem((const struct TagItem **)&tags)); )
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
IPTR IconDrawerList__OM_DISPOSE(struct IClass *CLASS, Object *obj, Msg message)
{
    struct MUI_IconDrawerData *data = INST_DATA(CLASS, obj);

    if (data->drawer) FreeVec(data->drawer);

    return DoSuperMethodA(CLASS, obj, message);
}

/**************************************************************************
OM_SET
**************************************************************************/
IPTR IconDrawerList__OM_SET(struct IClass *CLASS, Object *obj, struct opSet *message)
{
    struct MUI_IconDrawerData   *data = INST_DATA(CLASS, obj);
    struct TagItem  	        *tag = NULL,
                                *tags = NULL;

    /* parse initial taglist */
    for (tags = message->ops_AttrList; (tag = NextTagItem((const struct TagItem **)&tags)); )
    {
        switch (tag->ti_Tag)
        {
            case    MUIA_IconDrawerList_Drawer:
                if (data->drawer) FreeVec(data->drawer);

                data->drawer = StrDup((char*)tag->ti_Data);
                DoMethod(obj, MUIM_IconList_Update);

                break;
        }
    }

    return DoSuperMethodA(CLASS, obj, (Msg)message);
}

/**************************************************************************
OM_GET
**************************************************************************/
IPTR IconDrawerList__OM_GET(struct IClass *CLASS, Object *obj, struct opGet *message)
{
    /* small macro to simplify return value storage */
#define STORE *(message->opg_Storage)
    struct MUI_IconDrawerData *data = INST_DATA(CLASS, obj);

    switch (message->opg_AttrID)
    {
        case MUIA_IconDrawerList_Drawer: STORE = (unsigned long)data->drawer; return 1;
    }

    if (DoSuperMethodA(CLASS, obj, (Msg) message)) return 1;
    return 0;
#undef STORE
}

/**************************************************************************
MUIM_IconList_Update
**************************************************************************/
IPTR IconDrawerList__MUIM_Update(struct IClass *CLASS, Object *obj, struct MUIP_IconList_Update *message)
{
    struct MUI_IconDrawerData *data = INST_DATA(CLASS, obj);
    //struct IconEntry *node;

D(bug("[IconList]: IconDrawerList__MUIM_Update()\n"));

	DoSuperMethodA(CLASS, obj, (Msg) message);

    DoMethod(obj, MUIM_IconList_Clear);

    /* If not in setup do nothing */
    if (!(_flags(obj) & MADF_SETUP)) return 1;

    IconDrawerList__ParseContents(CLASS, obj);

    /*_Sort takes care of icon placement and redrawing for us*/
    DoMethod(obj, MUIM_IconList_Sort);

    return 1;
}


BOOPSI_DISPATCHER(IPTR, IconDrawerList_Dispatcher, CLASS, obj, message)
{
    switch (message->MethodID)
    {
        case OM_NEW: return IconDrawerList__OM_NEW(CLASS, obj, (struct opSet *)message);
        case OM_DISPOSE: return IconDrawerList__OM_DISPOSE(CLASS, obj, message);
        case OM_SET: return IconDrawerList__OM_SET(CLASS, obj, (struct opSet *)message);
        case OM_GET: return IconDrawerList__OM_GET(CLASS, obj, (struct opGet *)message);
        case MUIM_IconList_Update: return IconDrawerList__MUIM_Update(CLASS, obj, (APTR)message);
    }
    return DoSuperMethodA(CLASS, obj, message);
}
BOOPSI_DISPATCHER_END

/* sba: taken from SimpleFind3 */

struct NewDosList
{
    struct List       list;
    APTR              pool;
};

struct NewDosNode
{
    struct Node       node;
    STRPTR            name;
    struct Device     *device;
	struct Unit       *unit;
    struct MsgPort    *port;
};

static struct NewDosList *IconVolumeList__CreateDOSList(void)
{
    APTR pool = CreatePool(MEMF_PUBLIC,4096,4096);
    if (pool)
    {
        struct NewDosList *ndl = (struct NewDosList*)AllocPooled(pool, sizeof(struct NewDosList));
        if (ndl)
        {
            struct DosList *dl = NULL;
        
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
                UBYTE *dosname = dl->dol_Ext.dol_AROS.dol_DevName;
                LONG len = strlen(dosname);
#endif

                if ((name = (STRPTR)AllocPooled(pool, len + 1)))
                {
                    struct NewDosNode *ndn = NULL;
    
                    name[len] = 0;
                    strncpy(name, dosname, len);
    
                    if ((ndn = (struct NewDosNode*)AllocPooled(pool, sizeof(*ndn))))
                    {
                        ndn->name = name;
                        ndn->device = dl->dol_Ext.dol_AROS.dol_Device;
                        ndn->unit = dl->dol_Ext.dol_AROS.dol_Unit;
D(bug("[IconList]: IconVolumeList__CreateDOSList: adding node for '%s' (Device @ %x, Unit @ %x)\n", ndn->name, ndn->device, ndn->unit));
D(bug("[IconList]: IconVolumeList__CreateDOSList: Device '%s'\n", ndn->device->dd_Library.lib_Node.ln_Name));
#ifndef __AROS__
                        ndn->port = dl->dol_Task;
#else
                        ndn->port = NULL;
#endif
                        AddTail((struct List*)ndl, (struct Node*)ndn);
                    }
                }
            }
            UnLockDosList(LDF_VOLUMES|LDF_READ);

#ifndef __AROS__
            dl = LockDosList(LDF_DEVICES|LDF_READ);
            while(( dl = NextDosEntry(dl, LDF_DEVICES)))
            {
                struct NewDosNode *ndn = NULL;
    
                if (!dl->dol_Task) continue;
    
                ndn = (struct NewDosNode*)GetHead(ndl);
                while ((ndn))
                {
                    if (dl->dol_Task == ndn->port)
                    {
                        STRPTR name;
                        UBYTE  *dosname = (UBYTE*)BADDR(dl->dol_Name);
                        LONG   len = dosname[0];
    
                        if ((name = (STRPTR)AllocPooled(pool, len + 1)))
                        {
                            name[len] = 0;
                            strncpy(name, &dosname[1], len);
                        }
    
                        ndn->device = name;
                        break;
                    }
    
                    ndn = (struct NewDosNode*)GetSucc(ndn);
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

static void IconVolumeList__DestroyDOSList(struct NewDosList *ndl)
{
    if (ndl && ndl->pool) DeletePool(ndl->pool);
}
/* sba: End SimpleFind3 */


struct MUI_IconVolumeData
{
    int dummy;
};

/**************************************************************************
OM_NEW
**************************************************************************/
IPTR IconVolumeList__OM_NEW(struct IClass *CLASS, Object *obj, struct opSet *message)
{
    struct MUI_IconDrawerData   *data = NULL;
//    struct TagItem  	        *tag = NULL,
//                                *tags = NULL;

    obj = (Object *)DoSuperNewTags(CLASS, obj, NULL,
        TAG_MORE, (IPTR) message->ops_AttrList);

    if (!obj) return FALSE;

    data = INST_DATA(CLASS, obj);

	SET(obj, MUIA_IconList_DisplayFlags, ICONLIST_DISP_VERTICAL);
    SET(obj, MUIA_IconList_SortFlags, 0);

    return (IPTR)obj;
}

/**************************************************************************
MUIM_IconList_Update
**************************************************************************/
IPTR IconVolumeList__MUIM_IconList_Update(struct IClass *CLASS, Object *obj, struct MUIP_IconList_Update *message)
{
    //struct MUI_IconVolumeData *data = INST_DATA(CLASS, obj);
    struct IconEntry  *this_Icon = NULL;
    struct NewDosList *ndl = NULL;

D(bug("[IconList]: IconVolumeList__MUIM_IconList_Update()\n"));

	DoSuperMethodA(CLASS, obj, (Msg) message);
	
    DoMethod(obj, MUIM_IconList_Clear);

    /* If not in setup do nothing */
    if (!(_flags(obj) & MADF_SETUP)) return 1;

    if ((ndl = IconVolumeList__CreateDOSList()))
    {
        struct NewDosNode *nd = NULL;
		struct	MsgPort 			*mp=NULL;
			
		mp = CreateMsgPort();
		if (mp)
		{
			ForeachNode(ndl, nd)
			{
				char buf[300];
				if (nd->name)
				{
					strcpy(buf, nd->name);
					strcat(buf, ":Disk");
			
					if (!(this_Icon = (struct IconEntry *)DoMethod(obj, MUIM_IconList_CreateEntry, (IPTR)buf, (IPTR)nd->name, (IPTR)NULL, (IPTR)NULL)))
					{
	D(bug("[IconList]: IconVolumeList__MUIM_IconList_Update: Failed to Add IconEntry for '%s'\n", nd->name));
					}
					else
					{
						if (!(this_Icon->ile_Flags & ICONENTRY_FLAG_HASICON))
							this_Icon->ile_Flags |= ICONENTRY_FLAG_HASICON;

						if (!(strcmp(nd->name, "Ram Disk")))
						{
	D(bug("[IconList]: IconVolumeList__MUIM_IconList_Update: Setting Ram Disks icon node priority to 5\n"));
							this_Icon->ile_Node.ln_Pri = 5;   // Special dirs get Priority 5
						}
						else
						{
							this_Icon->ile_Node.ln_Pri = 1;   // Fixed Media get Priority 1

/*							struct IOExtTD *ioreq = NULL;
							struct DriveGeometry dg;

							if (ioreq = (struct IOStdReq *)CreateIORequest(mp, sizeof(struct IOStdReq)))
							{
								if (OpenDevice(boot_Device, boot_Unit, (struct IORequest *)ioreq, 0) == 0)
								{
									ioreq->iotd_Req.io_Command = TD_GETGEOMETRY;
									ioreq->iotd_Req.io_Data = &dg;
									ioreq->iotd_Req.io_Length = sizeof(struct DriveGeometry);
									DoIO((struct IORequest *)ioreq);
									if (dg.dg_Flags & DGF_REMOVABLE)
									{
										this_Icon->ile_Node.ln_Pri = 0;   // Removable Media get Priority 0
									}
									CloseDevice((struct IORequest *)ioreq);
								}
								DeleteIORequest((struct IORequest *)ioreq);
							}*/
						}
					}
				}
			}
			IconVolumeList__DestroyDOSList(ndl);
		}
    }

    /* default display/sorting flags */
    DoMethod(obj, MUIM_IconList_Sort);

    return 1;
}


BOOPSI_DISPATCHER(IPTR, IconVolumeList_Dispatcher, CLASS, obj, message)
{
    switch (message->MethodID)
    {
        case OM_NEW: return IconVolumeList__OM_NEW(CLASS, obj, (struct opSet *)message);
        case MUIM_IconList_Update: return IconVolumeList__MUIM_IconList_Update(CLASS,obj,(APTR)message);
    }

    return DoSuperMethodA(CLASS, obj, message);
}
BOOPSI_DISPATCHER_END

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
    sizeof(struct MUI_IconVolumeData), 
    (void*)IconVolumeList_Dispatcher
};

