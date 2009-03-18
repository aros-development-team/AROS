/*
Copyright  2002-2009, The AROS Development Team. All rights reserved.
$Id$
*/

#include "../portable_macros.h"
#ifndef __AROS__
#define WANDERER_BUILTIN_ICONLIST 1
#else
#define DEBUG 0
#include <aros/debug.h>
#endif

//#define DEBUG_ILC_FUNCS
//#define DEBUG_ILC_ATTRIBS
//#define DEBUG_ILC_EVENTS
//#define DEBUG_ILC_KEYEVENTS
//#define DEBUG_ILC_ICONDRAGDROP
//#define DEBUG_ILC_ICONRENDERING
//#define DEBUG_ILC_ICONSORTING
//#define DEBUG_ILC_ICONSORTING_DUMP
//#define DEBUG_ILC_ICONPOSITIONING
//#define DEBUG_ILC_LASSO
//#define DEBUG_ILC_MEMALLOC

//#define CREATE_FULL_DRAGIMAGE

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include <dos/dos.h>
#include <dos/datetime.h>
#include <dos/filehandler.h>

#include <exec/memory.h>
#include <graphics/gfx.h>
#include <graphics/view.h>
#include <graphics/rpattr.h>
#include <workbench/icon.h>
#include <workbench/workbench.h>

#ifdef __AROS__
#include <devices/rawkeycodes.h>
#include <clib/alib_protos.h>
#else
#include <devices_AROS/rawkeycodes.h>
#endif


#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/utility.h>
#include <proto/dos.h>
#include <proto/icon.h>
#include <proto/layers.h>
#include <proto/dos.h>
#include <proto/iffparse.h>

#ifdef __AROS__
#include <prefs/prefhdr.h>
#include <prefs/wanderer.h>
#else
#include <prefs_AROS/prefhdr.h>
#include <prefs_AROS/wanderer.h>
#endif

#include <proto/cybergraphics.h>

#ifdef __AROS__
#include <cybergraphx/cybergraphics.h>
#else
#include <cybergraphx_AROS/cybergraphics.h>
#endif


#if defined(__AMIGA__) && !defined(__PPC__)
#define NO_INLINE_STDARG
#endif
#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <libraries/mui.h>
#include "iconlist_attributes.h"
#include "icon_attributes.h"
#include "iconlist.h"
#include "iconlist_private.h"
#include "iconlistview.h"

#ifndef __AROS__
#define DEBUG 1

#ifdef DEBUG
  #define D(x) if (DEBUG) x
  #ifdef __amigaos4__
  #define bug DebugPrintF
  #else
  #define bug kprintf
  #endif
#else
  #define  D(...)
#endif
#endif

#define _between(a,x,b) ((x)>=(a) && (x)<=(b))
#define _isinobject(x,y) (_between(_mleft(obj),(x),_mright (obj)) \
                          && _between(_mtop(obj) ,(y),_mbottom(obj)))

extern struct Library   *MUIMasterBase;

static struct Hook             __iconlist_UpdateLabels_hook;

// N.B: We Handle frame/background rendering so make sure icon.library doesnt do it ..
struct TagItem          __iconList_DrawIconStateTags[] = {
    { ICONDRAWA_Frameless, TRUE},
    { ICONDRAWA_Borderless, TRUE},
    { ICONDRAWA_EraseBackground, FALSE},
    { TAG_DONE, }
};

#ifndef NO_ICON_POSITION
#define NO_ICON_POSITION                               (0x8000000) /* belongs to workbench/workbench.h */
#endif

#define UPDATE_SINGLEICON                              1
#define UPDATE_SCROLL                                  2
#define UPDATE_SORT                                    3
#define UPDATE_RESIZE                                  4

#define LEFT_BUTTON                                    1
#define RIGHT_BUTTON                                   2
#define MIDDLE_BUTTON                                  4

#define ICONLIST_DRAWMODE_NORMAL                       1
#define ICONLIST_DRAWMODE_FAST                         2

/**************************************************************************
                         Support Functions
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
#ifdef __AROS__
#error "Implement AndRectRect (rom/graphics/andrectrect.c)"
#else
#warning "Implement AndRectRect (rom/graphics/andrectrect.c)"
#endif
#endif

#define RPALPHAFLAT (1 << 0)
#define RPALPHARADIAL (1 << 1)

static void RastPortSetAlpha(struct RastPort *arport, ULONG ax, ULONG ay, ULONG width, ULONG height, UBYTE val, UBYTE alphamode)
{
	ULONG x, y;
	ULONG alphaval, pixelval;

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			if ((pixelval = ReadRGBPixel(arport, x, y)))
			{
                if (alphamode == RPALPHARADIAL){
                    //Set the alpha value based on distance from ax,ay
                } else {
                    alphaval = val;
                }
				WriteRGBPixel(arport, x, y, ((pixelval & 0xffffff)|(alphaval << 24)));
			}
		}
	}
}

///RectAndRect()
// Icon/Label Area support functions
static int RectAndRect(struct Rectangle *a, struct Rectangle *b)
{
    if ((a->MinX > b->MaxX) || (a->MinY > b->MaxY) || (a->MaxX < b->MinX) || (a->MaxY < b->MinY))
        return 0;
    return 1;
}
///

///Node_NextVisible()
// IconEntry List navigation functions ..
static struct IconEntry *Node_NextVisible(struct IconEntry *current_Node)
{
    current_Node = (struct IconEntry *)GetSucc(&current_Node->ie_IconNode);
    while ((current_Node != NULL) && (!(current_Node->ie_Flags & ICONENTRY_FLAG_VISIBLE)))
    {
        current_Node = (struct IconEntry *)GetSucc(&current_Node->ie_IconNode);
    }
    return current_Node;
}
///

///Node_FirstVisible()
static struct IconEntry *Node_FirstVisible(struct List *icon_list)
{
    struct IconEntry *current_Node = (struct IconEntry *)GetHead(icon_list);

    if ((current_Node != NULL) && !(current_Node->ie_Flags & ICONENTRY_FLAG_VISIBLE))
        current_Node = Node_NextVisible(current_Node);

    return current_Node;
}
///

///Node_PreviousVisible()
static struct IconEntry *Node_PreviousVisible(struct IconEntry *current_Node)
{
    current_Node = (struct IconEntry *)GetPred(&current_Node->ie_IconNode);
    while ((current_Node != NULL) && (!(current_Node->ie_Flags & ICONENTRY_FLAG_VISIBLE)))
    {
        current_Node = (struct IconEntry *)GetPred(&current_Node->ie_IconNode);
    }
    return current_Node;
}
///

///Node_LastVisible()
static struct IconEntry *Node_LastVisible(struct List *icon_list)
{
    struct IconEntry *current_Node = (struct IconEntry *)GetTail(icon_list);

    if ((current_Node != NULL) && !(current_Node->ie_Flags & ICONENTRY_FLAG_VISIBLE))
        current_Node = Node_PreviousVisible(current_Node);

    return current_Node;
}
///


const UBYTE MSG_MEM_G[] = "GB";
const UBYTE MSG_MEM_M[] = "MB";
const UBYTE MSG_MEM_K[] = "KB";
const UBYTE MSG_MEM_B[] = "Bytes";

///FmtSizeToString()
static void FmtSizeToString(UBYTE *buf, ULONG num)
{
  UQUAD d;
  UBYTE *ch;
  struct
  {
    IPTR val;
    IPTR  dec;
  } array =
  {
    num,
    0
  };

  if (num >= 1073741824)
  {
    //Gigabytes
    array.val = num >> 30;
    d = ((UQUAD)num * 10 + 536870912) / 1073741824;
    array.dec = d % 10;
    ch = MSG_MEM_G;
  }
  else if (num >= 1048576)
  {
    //Megabytes
    array.val = num >> 20;
    d = ((UQUAD)num * 10 + 524288) / 1048576;
    array.dec = d % 10;
    ch = MSG_MEM_M;
  }
  else if (num >= 1024)
  {
    //Kilobytes
    array.val = num >> 10;
    d = (num * 10 + 512) / 1024;
    array.dec = d % 10;
    ch = MSG_MEM_K;
  }
  else
  {
    //Bytes
    array.val = num;
    array.dec = 0;
    d = 0;
    ch = MSG_MEM_B;
  }

  if (!array.dec && (d > array.val * 10))
  {
    array.val++;
  }

  RawDoFmt(array.dec ? "%lu.%lu" : "%lu", &array, NULL, buf);

  while (*buf)
  {
    buf++;
  }

  sprintf(buf, " %s", ch);
}
///

///GetAbsoluteLassoRect()
// get positive lasso coords
static void GetAbsoluteLassoRect(struct IconList_DATA *data, struct Rectangle *LassoRectangle)
{
    WORD minx = data->icld_LassoRectangle.MinX;
    WORD miny = data->icld_LassoRectangle.MinY;
    WORD maxx = data->icld_LassoRectangle.MaxX;
    WORD maxy = data->icld_LassoRectangle.MaxY;

#if defined(DEBUG_ILC_LASSO) && defined(DEBUG_ILC_FUNCS)
D(bug("[IconList]: %s()\n", __PRETTY_FUNCTION__));
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
///

///IconList_InvertPixelRect()
static void IconList_InvertPixelRect(struct RastPort *rp, WORD minx, WORD miny, WORD maxx, WORD maxy, struct Rectangle *clip)
{
    struct Rectangle r, clipped_r;
    
#if defined(DEBUG_ILC_RENDERING) && defined(DEBUG_ILC_FUNCS)
D(bug("[IconList]: %s()\n", __PRETTY_FUNCTION__));
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
///

///IconList_InvertLassoOutlines()
// Simple lasso drawing by inverting area outlines
static void IconList_InvertLassoOutlines(Object *obj, struct Rectangle *rect)
{
    struct Rectangle lasso;
    struct Rectangle clip;
    
#if defined(DEBUG_ILC_LASSO) && defined(DEBUG_ILC_FUNCS)
D(bug("[IconList]: %s()\n", __PRETTY_FUNCTION__));
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
///

///IconList_GetIconImageRectangle()
//We don't use icon.library's label drawing so we do this by hand
static void IconList_GetIconImageRectangle(Object *obj, struct IconList_DATA *data, struct IconEntry *icon, struct Rectangle *rect)
{
#if defined(DEBUG_ILC_ICONPOSITIONING) && defined(DEBUG_ILC_FUNCS)
D(bug("[IconList]: %s(icon @ %p)\n", __PRETTY_FUNCTION__, icon));
#endif

    /* Get basic width/height */    
    GetIconRectangleA(NULL, icon->ie_DiskObj, NULL, rect, NULL);
#if defined(DEBUG_ILC_ICONPOSITIONING)
D(bug("[IconList] %s: MinX %d, MinY %d      MaxX %d, MaxY %d\n", __PRETTY_FUNCTION__, rect->MinX, rect->MinY, rect->MaxX, rect->MaxY));
#endif
    icon->ie_IconWidth  = (rect->MaxX - rect->MinX) + 1;
    icon->ie_IconHeight = (rect->MaxY - rect->MinY) + 1;
    
    if (icon->ie_IconHeight > data->icld_IconLargestHeight)
        data->icld_IconLargestHeight = icon->ie_IconHeight;
}
///

///IconList_GetIconLabelRectangle()
static void IconList_GetIconLabelRectangle(Object *obj, struct IconList_DATA *data, struct IconEntry *icon, struct Rectangle *rect)
{
    ULONG      outline_offset = 0;
    ULONG      textwidth = 0;

#if defined(DEBUG_ILC_ICONPOSITIONING) && defined(DEBUG_ILC_FUNCS)
D(bug("[IconList]: %s()\n", __PRETTY_FUNCTION__));
#endif

    switch ( data->icld__Option_LabelTextMode )
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
    if ((icon->ie_IconListEntry.label != NULL) && (icon->ie_TxtBuf_DisplayedLabel != NULL))
    {
        ULONG curlabel_TotalLines;
        SetFont(data->icld_BufferRastPort, data->icld_IconLabelFont);
        
        rect->MinX = 0;
        rect->MaxX = (((data->icld__Option_LabelTextHorizontalPadding + data->icld__Option_LabelTextBorderWidth) * 2) + icon->ie_TxtBuf_DisplayedLabelWidth + outline_offset) - 1;
    
        rect->MinY = 0;

        curlabel_TotalLines = icon->ie_SplitParts;
        if (curlabel_TotalLines == 0)
            curlabel_TotalLines = 1;
        if (curlabel_TotalLines > data->icld__Option_LabelTextMultiLine)
            curlabel_TotalLines = data->icld__Option_LabelTextMultiLine;
        
        rect->MaxY = (((data->icld__Option_LabelTextBorderHeight + data->icld__Option_LabelTextVerticalPadding) * 2) + 
                     ((data->icld_IconLabelFont->tf_YSize + outline_offset) * curlabel_TotalLines)) - 1;

        /*  Date/size sorting has the date/size appended under the icon label
            only list regular files like this (drawers have no size/date output) */
        if(
            icon->ie_IconListEntry.type != ST_USERDIR && 
            ((data->icld_SortFlags & ICONLIST_SORT_BY_SIZE) || (data->icld_SortFlags & ICONLIST_SORT_BY_DATE))
        )
        {
            SetFont(data->icld_BufferRastPort, data->icld_IconInfoFont);

            if( (data->icld_SortFlags & ICONLIST_SORT_BY_SIZE) && !(data->icld_SortFlags & ICONLIST_SORT_BY_DATE) )
            {
                icon->ie_TxtBuf_SIZEWidth = TextLength(data->icld_BufferRastPort, icon->ie_TxtBuf_SIZE, strlen(icon->ie_TxtBuf_SIZE));
                textwidth = icon->ie_TxtBuf_SIZEWidth;
            }
            else
            {
                if( !(data->icld_SortFlags & ICONLIST_SORT_BY_SIZE) && (data->icld_SortFlags & ICONLIST_SORT_BY_DATE) )
                {
                    if( icon->ie_Flags & ICONENTRY_FLAG_TODAY )
                    {
                        icon->ie_TxtBuf_TIMEWidth = TextLength(data->icld_BufferRastPort, icon->ie_TxtBuf_TIME, strlen(icon->ie_TxtBuf_TIME));
                        textwidth = icon->ie_TxtBuf_TIMEWidth;
                    }
                    else
                    {
                        icon->ie_TxtBuf_DATEWidth = TextLength(data->icld_BufferRastPort, icon->ie_TxtBuf_DATE, strlen(icon->ie_TxtBuf_DATE));
                        textwidth = icon->ie_TxtBuf_DATEWidth;
                    }
                }
            }

            if (textwidth > 0)
            {
                rect->MaxY = rect->MaxY + data->icld_IconInfoFont->tf_YSize + outline_offset;
                if ((textwidth + outline_offset + ((data->icld__Option_LabelTextHorizontalPadding + data->icld__Option_LabelTextBorderWidth) * 2)) > ((rect->MaxX - rect->MinX) + 1))
                    rect->MaxX = (textwidth + outline_offset + ((data->icld__Option_LabelTextVerticalPadding + data->icld__Option_LabelTextBorderWidth) * 2)) - 1;
            }
        }
    }
    if (((rect->MaxY - rect->MinY) + 1) > data->icld_LabelLargestHeight) data->icld_LabelLargestHeight = ((rect->MaxY - rect->MinY) + 1);
}
///

///IconList_GetIconAreaRectangle()
static void IconList_GetIconAreaRectangle(Object *obj, struct IconList_DATA *data, struct IconEntry *icon, struct Rectangle *rect)
{
    struct Rectangle labelrect;
    ULONG iconlabel_Width;
    ULONG iconlabel_Height;

#if defined(DEBUG_ILC_ICONPOSITIONING) && defined(DEBUG_ILC_FUNCS)
D(bug("[IconList]: %s()\n", __PRETTY_FUNCTION__));
#endif

    /* Get icon box width including text width */
    memset(rect, 0, sizeof(struct Rectangle));

    IconList_GetIconImageRectangle(obj, data, icon, rect);

    icon->ie_AreaWidth = icon->ie_IconWidth;
    if (data->icld__Option_IconListMode == ICON_LISTMODE_GRID)
    {
        icon->ie_AreaHeight = data->icld_IconLargestHeight;
    }
    else
    {
        icon->ie_AreaHeight = icon->ie_IconHeight;
    }

    IconList_GetIconLabelRectangle(obj, data, icon, &labelrect);

    iconlabel_Width = ((labelrect.MaxX - labelrect.MinX) + 1);
    iconlabel_Height = ((labelrect.MaxY - labelrect.MinY) + 1);
    
    if (iconlabel_Width > icon->ie_AreaWidth)
        icon->ie_AreaWidth = iconlabel_Width;

    icon->ie_AreaHeight = icon->ie_AreaHeight + data->icld__Option_IconImageSpacing + iconlabel_Height;
    
    /* Store */
    rect->MaxX = (rect->MinX + icon->ie_AreaWidth) - 1;
    rect->MaxY = (rect->MinY + icon->ie_AreaHeight) - 1;
    
    if (icon->ie_AreaWidth > data->icld_IconAreaLargestWidth) data->icld_IconAreaLargestWidth = icon->ie_AreaWidth;
    if (icon->ie_AreaHeight > data->icld_IconAreaLargestHeight) data->icld_IconAreaLargestHeight = icon->ie_AreaHeight;
}
///
/**************************************************************************
Draw the icon at its position
**************************************************************************/
///IconList__MUIM_IconList_DrawEntry()
IPTR IconList__MUIM_IconList_DrawEntry(struct IClass *CLASS, Object *obj, struct MUIP_IconList_DrawEntry *message)
{
    struct IconList_DATA *data = INST_DATA(CLASS, obj);

    BOOL outside = FALSE;

    struct Rectangle iconrect;
    struct Rectangle objrect;

    LONG offsetx,offsety;

    ULONG objX, objY, objW, objH;
    LONG iconX, iconY;
    ULONG iconW, iconH;

    if (data->icld_BufferRastPort == data->icld_DisplayRastPort)
    {
        objX = _mleft(obj);
        objY = _mtop(obj);
    }
    else
    {
        objX = objY = 0;
    }
    objW = _mright(obj) - _mleft(obj) + 1;
    objH = _mbottom(obj) - _mtop(obj) + 1;

#if defined(DEBUG_ILC_ICONRENDERING)
D(bug("[IconList]: %s(message->icon = 0x%p)\n", __PRETTY_FUNCTION__, message->icon));
#endif

    if ((!(message->icon->ie_Flags & ICONENTRY_FLAG_VISIBLE)) ||
        (data->icld_BufferRastPort == NULL) ||
        (!(message->icon->ie_DiskObj)))
    {
#if defined(DEBUG_ILC_ICONRENDERING)
D(bug("[IconList] %s: Not visible or missing DOB\n", __PRETTY_FUNCTION__));
#endif
        return FALSE;
    }
    
    /* Get the dimensions and affected area of message->icon */
    IconList_GetIconImageRectangle(obj, data, message->icon, &iconrect);
    iconW = iconrect.MaxX - iconrect.MinX + 1;
    iconH = iconrect.MaxY - iconrect.MinY + 1;
    
    /* Add the relative position offset of the message->icon */
    offsetx = objX - data->icld_ViewX + message->icon->ie_IconX;
    /* Centre our image with our text */
    if (message->icon->ie_IconWidth < message->icon->ie_AreaWidth)
        offsetx += (message->icon->ie_AreaWidth - message->icon->ie_IconWidth)/2;

    if ((data->icld__Option_IconListMode == ICON_LISTMODE_GRID) &&
        (message->icon->ie_AreaWidth < data->icld_IconAreaLargestWidth))
        offsetx += ((data->icld_IconAreaLargestWidth - message->icon->ie_AreaWidth)/2);

    iconrect.MinX += offsetx;
    iconrect.MaxX += offsetx;

    offsety = objY - data->icld_ViewY + message->icon->ie_IconY;
    iconrect.MinY += offsety;
    iconrect.MaxY += offsety;

    /* Add the relative position of the window */
    objrect.MinX = objX;
    objrect.MinY = objY;
    objrect.MaxX = objX + objW;
    objrect.MaxY = objY + objH;

    if (!RectAndRect(&iconrect, &objrect))
    {
#if defined(DEBUG_ILC_ICONRENDERING)
D(bug("[IconList] %s: Icon '%s' image outside of visible area .. skipping\n", __PRETTY_FUNCTION__, message->icon->ie_IconListEntry.label));
#endif
        return FALSE;
    }

    /* data->update_rect1 and data->update_rect2 may
       point to rectangles to indicate that only icons
       in any of this rectangles need to be drawn      */
    if (data->update_rect1)
    {
        if (!RectAndRect(&iconrect, data->update_rect1)) outside = TRUE;
    }

    if (data->update_rect2)
    {
        if (data->update_rect1)
        {
            if ((outside == TRUE) && RectAndRect(&iconrect, data->update_rect2)) outside = FALSE;
        }
        else
        {
            if (!RectAndRect(&iconrect, data->update_rect2)) outside = TRUE;
        }
    }

    if (outside == TRUE)
    {
#if defined(DEBUG_ILC_ICONRENDERING)
D(bug("[IconList] %s: Icon '%s' image outside of update area .. skipping\n", __PRETTY_FUNCTION__, message->icon->ie_IconListEntry.label));
#endif
        return FALSE;
    }

    if (message->drawmode == ICONENTRY_DRAWMODE_NONE) return TRUE;

    // Center icon image
    iconX = iconrect.MinX - objX + data->icld_DrawOffsetX;
    iconY = iconrect.MinY - objY + data->icld_DrawOffsetY;

    if ((data->icld_BufferRastPort == data->icld_DisplayRastPort) || 
         ((data->icld_BufferRastPort != data->icld_DisplayRastPort) && 
          ((iconX > objX) && (iconX < (objX + objW)) && (iconY > objY) && (iconY < (objY + objH))) &&
          (((iconX + iconW) > objX) && ((iconX + iconW)< (objX + objW)) && ((iconY + iconH) > objY) && ((iconY + iconH) < (objY + objH)))
         ))
    {
#if defined(DEBUG_ILC_ICONRENDERING)
D(bug("[IconList] %s: DrawIconState('%s') .. %d, %d\n", __PRETTY_FUNCTION__, message->icon->ie_IconListEntry.label, iconX, iconY));
#endif
        DrawIconStateA
            (
                data->icld_BufferRastPort, message->icon->ie_DiskObj, NULL,
                iconX, 
                iconY, 
                (message->icon->ie_Flags & ICONENTRY_FLAG_SELECTED) ? IDS_SELECTED : IDS_NORMAL,
                __iconList_DrawIconStateTags
            );
#if defined(DEBUG_ILC_ICONRENDERING)
D(bug("[IconList] %s: DrawIconState Done\n", __PRETTY_FUNCTION__));
#endif
    }
    else
    {
#if defined(DEBUG_ILC_ICONRENDERING)
D(bug("[IconList] %s: DrawIconState('%s') NEEDS CLIPPED!\n", __PRETTY_FUNCTION__, message->icon->ie_IconListEntry.label));
#endif
    }

    return TRUE;
}
///

///IconList__LabelFunc_SplitLabel()
void IconList__LabelFunc_SplitLabel(Object *obj, struct IconList_DATA *data, struct IconEntry *icon)
{
    ULONG   	labelSplit_MaxLabelLineLength = data->icld__Option_LabelTextMaxLen;
    ULONG       labelSplit_LabelLength = strlen(icon->ie_IconListEntry.label);
    ULONG       txwidth;
//    ULONG       labelSplit_FontY = data->icld_IconLabelFont->tf_YSize;
     int        labelSplit_CharsDone,   labelSplit_CharsSplit;
    ULONG 		labelSplit_CurSplitWidth;

	if ((data->icld__Option_TrimVolumeNames) && 
		((icon->ie_IconListEntry.type == ST_ROOT) && (icon->ie_IconListEntry.label[labelSplit_LabelLength - 1] == ':')))
		labelSplit_LabelLength--;

    if (labelSplit_MaxLabelLineLength >= labelSplit_LabelLength)
    {
#if defined(DEBUG_ILC_ICONRENDERING)
D(bug("[IconList]: %s: Label'%s' doesnt need split (onyl %d chars)\n", __PRETTY_FUNCTION__, icon->ie_IconListEntry.label, labelSplit_LabelLength));
#endif
        return;
    }

    SetFont(data->icld_BufferRastPort, data->icld_IconLabelFont);
    txwidth = TextLength(data->icld_BufferRastPort, icon->ie_IconListEntry.label, labelSplit_MaxLabelLineLength);
#if defined(DEBUG_ILC_ICONRENDERING)
D(bug("[IconList]: %s: txwidth = %d\n", __PRETTY_FUNCTION__, txwidth));
#endif
    icon->ie_TxtBuf_DisplayedLabel = AllocVecPooled(data->icld_Pool, 256);
    memset(icon->ie_TxtBuf_DisplayedLabel, 0, 256);
    icon->ie_SplitParts = 0;
    
    labelSplit_CharsDone = 0;
    labelSplit_CharsSplit = 0;

    while (labelSplit_CharsDone < labelSplit_LabelLength)
    {
        ULONG labelSplit_CurSplitLength = labelSplit_LabelLength - labelSplit_CharsDone;
        IPTR  labelSplit_SplitStart = (IPTR)(icon->ie_IconListEntry.label + labelSplit_CharsDone);
        int   tmp_checkoffs = 0;
        IPTR  labelSplit_RemainingCharsAfterSplit;
        IPTR  labelSplit_CurSplitDest;

        while (*(char *)(labelSplit_SplitStart) == ' ')
        {
            //Skip preceding spaces..
            labelSplit_SplitStart = labelSplit_SplitStart + 1;
            labelSplit_CurSplitLength = labelSplit_CurSplitLength - 1;
            labelSplit_CharsDone = labelSplit_CharsDone + 1;
        }

        while(TextLength(data->icld_BufferRastPort, (char *)labelSplit_SplitStart, labelSplit_CurSplitLength) < txwidth) labelSplit_CurSplitLength++;
        while(TextLength(data->icld_BufferRastPort, (char *)labelSplit_SplitStart, labelSplit_CurSplitLength) > txwidth) labelSplit_CurSplitLength--;
#if defined(DEBUG_ILC_ICONRENDERING)
D(bug("[IconList]: %s: labelSplit_CurSplitLength = %d\n", __PRETTY_FUNCTION__, labelSplit_CurSplitLength));
#endif

#if defined(DEBUG_ILC_ICONRENDERING)
            D(bug("[IconList]: %s: Attempting to find neat split ", __PRETTY_FUNCTION__));
#endif
        while(tmp_checkoffs < (labelSplit_CurSplitLength - ILC_ICONLABEL_SHORTEST))
        {
#if defined(DEBUG_ILC_ICONRENDERING)
            D(bug("%d", tmp_checkoffs));
#endif
            labelSplit_RemainingCharsAfterSplit = labelSplit_LabelLength - (labelSplit_CharsDone + labelSplit_CurSplitLength);

            if ((labelSplit_CurSplitLength - tmp_checkoffs) > ILC_ICONLABEL_SHORTEST)
            {
#if defined(DEBUG_ILC_ICONRENDERING)
            D(bug("<"));
#endif
                if ((*(char *)(labelSplit_SplitStart + labelSplit_CurSplitLength - tmp_checkoffs) == ' ') ||
                    (*(char *)(labelSplit_SplitStart + labelSplit_CurSplitLength - tmp_checkoffs) == '.') ||
                    (*(char *)(labelSplit_SplitStart + labelSplit_CurSplitLength - tmp_checkoffs) == '-'))
                {
#if defined(DEBUG_ILC_ICONRENDERING)
            D(bug("!"));
#endif
                    labelSplit_CurSplitLength = labelSplit_CurSplitLength - tmp_checkoffs;
                    labelSplit_RemainingCharsAfterSplit = labelSplit_RemainingCharsAfterSplit - tmp_checkoffs;
                    tmp_checkoffs = 0;
                    break;
                }
            }

            if ((labelSplit_RemainingCharsAfterSplit - tmp_checkoffs) < 0)
            {
#if defined(DEBUG_ILC_ICONRENDERING)
            D(bug("="));
#endif
                    labelSplit_CurSplitLength = labelSplit_CurSplitLength + tmp_checkoffs;
                    labelSplit_RemainingCharsAfterSplit = labelSplit_RemainingCharsAfterSplit + tmp_checkoffs;
                    tmp_checkoffs = 0;
                    break;
            }

            if ((labelSplit_RemainingCharsAfterSplit - tmp_checkoffs) >= ILC_ICONLABEL_SHORTEST)
            {
#if defined(DEBUG_ILC_ICONRENDERING)
            D(bug(">"));
#endif
                if ((*(char *)(labelSplit_SplitStart + labelSplit_CurSplitLength + tmp_checkoffs) == ' ') ||
                    (*(char *)(labelSplit_SplitStart + labelSplit_CurSplitLength + tmp_checkoffs) == '.') ||
                    (*(char *)(labelSplit_SplitStart + labelSplit_CurSplitLength + tmp_checkoffs) == '-'))
                {
#if defined(DEBUG_ILC_ICONRENDERING)
            D(bug("!"));
#endif
                    labelSplit_CurSplitLength = labelSplit_CurSplitLength + tmp_checkoffs;
                    labelSplit_RemainingCharsAfterSplit = labelSplit_RemainingCharsAfterSplit + tmp_checkoffs;
                    tmp_checkoffs = 0;
                    break;
                }
            }

            tmp_checkoffs = tmp_checkoffs + 1;
        }
#if defined(DEBUG_ILC_ICONRENDERING)
        D(bug("\n"));
#endif
        if (tmp_checkoffs != 0)
        {
#if defined(DEBUG_ILC_ICONRENDERING)
            D(bug("[IconList]: %s: Couldnt find neat split : Still %d chars\n", __PRETTY_FUNCTION__, labelSplit_RemainingCharsAfterSplit));
#endif
            if (labelSplit_RemainingCharsAfterSplit <= ILC_ICONLABEL_SHORTEST)
            {
                labelSplit_CurSplitLength = labelSplit_CurSplitLength + (labelSplit_RemainingCharsAfterSplit - ILC_ICONLABEL_SHORTEST);
            }
        }
        if ((labelSplit_CharsDone + labelSplit_CurSplitLength) > labelSplit_LabelLength) labelSplit_CurSplitLength = labelSplit_LabelLength - labelSplit_CharsDone;

        labelSplit_CurSplitDest = (IPTR)(icon->ie_TxtBuf_DisplayedLabel + labelSplit_CharsSplit + icon->ie_SplitParts);
        
        strncpy((char *)labelSplit_CurSplitDest, (char *)labelSplit_SplitStart, labelSplit_CurSplitLength);
        
        labelSplit_CurSplitWidth = TextLength(data->icld_BufferRastPort, (char *)labelSplit_CurSplitDest, labelSplit_CurSplitLength);
        
        icon->ie_SplitParts = icon->ie_SplitParts + 1;
        
        labelSplit_CharsDone = labelSplit_CharsDone + labelSplit_CurSplitLength;
        labelSplit_CharsSplit = labelSplit_CharsSplit + labelSplit_CurSplitLength;
        
        if (labelSplit_CurSplitWidth > icon->ie_TxtBuf_DisplayedLabelWidth) icon->ie_TxtBuf_DisplayedLabelWidth = labelSplit_CurSplitWidth;
    }
    if ((icon->ie_SplitParts <= 1) && icon->ie_TxtBuf_DisplayedLabel)
    {
        FreeVecPooled(data->icld_Pool, icon->ie_TxtBuf_DisplayedLabel);
        icon->ie_TxtBuf_DisplayedLabel = NULL;
        icon->ie_SplitParts = 0;
    }
//  if ((labelSplit_FontY * icon->ie_SplitParts) > data->icld_LabelLargestHeight) data->icld_LabelLargestHeight = (labelSplit_FontY * icon->ie_SplitParts);
}
///

///IconList__LabelFunc_CreateLabel()
IPTR IconList__LabelFunc_CreateLabel(Object *obj, struct IconList_DATA *data, struct IconEntry *icon)
{
#if defined(DEBUG_ILC_ICONRENDERING) && defined(DEBUG_ILC_FUNCS)
D(bug("[IconList]: %s('%s')\n", __PRETTY_FUNCTION__, icon->ie_IconListEntry.label));
#endif
    if (icon->ie_TxtBuf_DisplayedLabel)
    {
        FreeVecPooled(data->icld_Pool, icon->ie_TxtBuf_DisplayedLabel);
        icon->ie_TxtBuf_DisplayedLabel = NULL;
        icon->ie_SplitParts = 0;
    }
    
    if (data->icld__Option_LabelTextMultiLine > 1)
    {
#if defined(DEBUG_ILC_ICONRENDERING)
D(bug("[IconList]: %s: Attempting to split label ..\n", __PRETTY_FUNCTION__));
#endif
        IconList__LabelFunc_SplitLabel(obj, data, icon);
    }
    
    if (icon->ie_TxtBuf_DisplayedLabel == NULL)
    { 
        ULONG ie_LabelLength = strlen(icon->ie_IconListEntry.label);
        icon->ie_SplitParts = 1;

#if defined(DEBUG_ILC_ICONRENDERING)
D(bug("[IconList]: %s: Building unsplit label (len = %d) ..\n", __PRETTY_FUNCTION__, ie_LabelLength));
#endif

		if ((data->icld__Option_TrimVolumeNames) && 
			((icon->ie_IconListEntry.type == ST_ROOT) && (icon->ie_IconListEntry.label[ie_LabelLength - 1] == ':')))
			ie_LabelLength--;

        if(ie_LabelLength > data->icld__Option_LabelTextMaxLen)
        {
            if (!(icon->ie_TxtBuf_DisplayedLabel = AllocVecPooled(data->icld_Pool, data->icld__Option_LabelTextMaxLen + 1)))
            {
                return (IPTR)NULL;
            }
            memset(icon->ie_TxtBuf_DisplayedLabel, 0, data->icld__Option_LabelTextMaxLen + 1);
            strncpy(icon->ie_TxtBuf_DisplayedLabel, icon->ie_IconListEntry.label, data->icld__Option_LabelTextMaxLen - 3);
            strcat(icon->ie_TxtBuf_DisplayedLabel , " ..");
        }
        else 
        {
            if (!(icon->ie_TxtBuf_DisplayedLabel = AllocVecPooled(data->icld_Pool, ie_LabelLength + 1)))
            {
                return (IPTR)NULL;
            }
            memset(icon->ie_TxtBuf_DisplayedLabel, 0, ie_LabelLength + 1);
            strncpy(icon->ie_TxtBuf_DisplayedLabel, icon->ie_IconListEntry.label, ie_LabelLength );
        }
        icon->ie_TxtBuf_DisplayedLabelWidth = TextLength(data->icld_BufferRastPort, icon->ie_TxtBuf_DisplayedLabel, strlen(icon->ie_TxtBuf_DisplayedLabel));
//    if ((data->icld_IconLabelFont->tf_YSize) > data->icld_LabelLargestHeight) data->icld_LabelLargestHeight = data->icld_IconLabelFont->tf_YSize;
    }

//  if (icon->ie_TxtBuf_DisplayedLabelWidth > data->icld_LabelLargestWidth) data->icld_LabelLargestWidth = icon->ie_TxtBuf_DisplayedLabelWidth;

    return (IPTR)icon->ie_TxtBuf_DisplayedLabel;
}
///

///IconList__HookFunc_UpdateLabelsFunc()
AROS_UFH3(
  void, IconList__HookFunc_UpdateLabelsFunc,
  AROS_UFHA(struct Hook *,    hook,   A0),
  AROS_UFHA(APTR *,           obj,    A2),
  AROS_UFHA(APTR,             param,  A1)
)
{
  AROS_USERFUNC_INIT
  
  /* Get our private data */
  Class *CLASS = *( Class **)param;

    struct IconList_DATA *data = INST_DATA(CLASS, obj);

#if defined(DEBUG_ILC_LASSO) && defined(DEBUG_ILC_FUNCS)
D(bug("[IconList]: %s()\n", __PRETTY_FUNCTION__));
#endif

    if (((data->icld__Option_LabelTextMaxLen != data->icld__Option_LastLabelTextMaxLen) &&
        (data->icld__Option_LabelTextMultiLine > 1)) ||
        (data->icld__Option_LabelTextMultiLine != data->icld__Option_LastLabelTextMultiLine));
    {
        struct IconEntry *iconentry_Current = NULL;
  #ifdef __AROS__
        ForeachNode(&data->icld_IconList, iconentry_Current)
  #else
  Foreach_Node(&data->icld_IconList, iconentry_Current);
  #endif
        {
            IconList__LabelFunc_CreateLabel((Object *)obj, data, iconentry_Current);
        }
    }

    data->icld__Option_LastLabelTextMaxLen = data->icld__Option_LabelTextMaxLen;
    data->icld__Option_LastLabelTextMultiLine = data->icld__Option_LabelTextMultiLine;

  AROS_USERFUNC_EXIT
}
///

///IconList__MUIM_IconList_DrawEntryLabel()
IPTR IconList__MUIM_IconList_DrawEntryLabel(struct IClass *CLASS, Object *obj, struct MUIP_IconList_DrawEntry *message)
{
    struct IconList_DATA *data = INST_DATA(CLASS, obj);

    STRPTR          buf = NULL;
    BOOL outside = FALSE;

    struct Rectangle  iconlabelrect;
    struct Rectangle  objrect;

    ULONG               txtbox_width = 0;
    LONG        tx,ty,offsetx,offsety;
    LONG        txwidth; // txheight;

    ULONG objX, objY, objW, objH;
    LONG labelX, labelY;
    ULONG labelW, labelH;

    if (data->icld_BufferRastPort == data->icld_DisplayRastPort)
    {
        objX = _mleft(obj);
        objY = _mtop(obj);
    }
    else
    {
        objX = objY = 0;
    }
    objW = _mright(obj) - _mleft(obj) + 1;
    objH = _mbottom(obj) - _mtop(obj) + 1;

    ULONG txtarea_width;
    ULONG curlabel_TotalLines, curlabel_CurrentLine, offset_y;

#if defined(DEBUG_ILC_ICONRENDERING) && defined(DEBUG_ILC_FUNCS)
D(bug("[IconList]: %s(message->icon = 0x%p), '%s'\n", __PRETTY_FUNCTION__, message->icon, message->icon->ie_IconListEntry.label));
#endif

    if ((!(message->icon->ie_Flags & ICONENTRY_FLAG_VISIBLE)) ||
        (data->icld_BufferRastPort == NULL) ||
        (!(message->icon->ie_DiskObj)))
    {
#if defined(DEBUG_ILC_ICONRENDERING)
D(bug("[IconList] %s: Not visible or missing DOB\n", __PRETTY_FUNCTION__));
#endif
        return FALSE;
    }

    /* Get the dimensions and affected area of message->icon's label */
    IconList_GetIconLabelRectangle(obj, data, message->icon, &iconlabelrect);
    labelW = iconlabelrect.MaxX - iconlabelrect.MinX + 1;
    labelH = iconlabelrect.MaxY - iconlabelrect.MinY + 1;

    /* Add the relative position offset of the message->icon's label */
    offsetx = (objX - data->icld_ViewX) + message->icon->ie_IconX;
    txtbox_width = (iconlabelrect.MaxX - iconlabelrect.MinX) + 1;

    if (txtbox_width < message->icon->ie_AreaWidth)
        offsetx += ((message->icon->ie_AreaWidth - txtbox_width)/2);

    if ((data->icld__Option_IconListMode == ICON_LISTMODE_GRID) &&
        (message->icon->ie_AreaWidth < data->icld_IconAreaLargestWidth))
        offsetx += ((data->icld_IconAreaLargestWidth - message->icon->ie_AreaWidth)/2);
    
    iconlabelrect.MinX += offsetx;
    iconlabelrect.MaxX += offsetx;

    offsety = (objY - data->icld_ViewY) + message->icon->ie_IconY + data->icld__Option_IconImageSpacing;
    if (data->icld__Option_IconListMode == ICON_LISTMODE_GRID)
    {
        offsety = offsety + data->icld_IconLargestHeight;
    }
    else
    {
        offsety = offsety + message->icon->ie_IconHeight;
    }
    iconlabelrect.MinY += offsety;
    iconlabelrect.MaxY += offsety;

    /* Add the relative position of the window */
    objrect.MinX = objX;
    objrect.MinY = objX;
    objrect.MaxX = objX + objW;
    objrect.MaxY = objY + objH;

    if (!RectAndRect(&iconlabelrect, &objrect))
    {
#if defined(DEBUG_ILC_ICONRENDERING)
D(bug("[IconList] %s: Icon '%s' label outside of visible area .. skipping\n", __PRETTY_FUNCTION__, message->icon->ie_IconListEntry.label));
#endif
        return FALSE;
    }

    /* data->update_rect1 and data->update_rect2 may
       point to rectangles to indicate that only icons
       in any of this rectangles need to be drawn      */
    if (data->update_rect1)
    {
        if (!RectAndRect(&iconlabelrect, data->update_rect1)) outside = TRUE;
    }

    if (data->update_rect2)
    {
        if (data->update_rect1)
        {
            if ((outside == TRUE) && RectAndRect(&iconlabelrect, data->update_rect2)) outside = FALSE;
        }
        else
        {
            if (!RectAndRect(&iconlabelrect, data->update_rect2)) outside = TRUE;
        }
    }

    if (outside == TRUE)
    {
#if defined(DEBUG_ILC_ICONRENDERING)
D(bug("[IconList] %s: Icon '%s' label outside of update area .. skipping\n", __PRETTY_FUNCTION__, message->icon->ie_IconListEntry.label));
#endif
        return FALSE;
    }
    
    if (message->drawmode == ICONENTRY_DRAWMODE_NONE) return TRUE;
    
    SetABPenDrMd(data->icld_BufferRastPort, _pens(obj)[MPEN_TEXT], 0, JAM1);

    iconlabelrect.MinX = (iconlabelrect.MinX - objX) + data->icld_DrawOffsetX;
    iconlabelrect.MinY = (iconlabelrect.MinY - objY) + data->icld_DrawOffsetY;
    iconlabelrect.MaxX = (iconlabelrect.MaxX - objX) + data->icld_DrawOffsetX;
    iconlabelrect.MaxY = (iconlabelrect.MaxY - objY) + data->icld_DrawOffsetY;

    labelX = iconlabelrect.MinX + data->icld__Option_LabelTextBorderWidth + data->icld__Option_LabelTextHorizontalPadding;
    labelY = iconlabelrect.MinY + data->icld__Option_LabelTextBorderHeight + data->icld__Option_LabelTextVerticalPadding;

    txtarea_width = txtbox_width - ((data->icld__Option_LabelTextBorderWidth + data->icld__Option_LabelTextHorizontalPadding) * 2);

    if ((data->icld_BufferRastPort == data->icld_DisplayRastPort) || 
         ((data->icld_BufferRastPort != data->icld_DisplayRastPort) && 
          ((iconlabelrect.MinX > objX) && (iconlabelrect.MinX < (objX + objW)) && (iconlabelrect.MinY > objY) && (iconlabelrect.MinY < (objY + objH))) &&
          ((iconlabelrect.MaxX > objX) && (iconlabelrect.MaxX < (objX + objW)) && (iconlabelrect.MaxY > objY) && (iconlabelrect.MaxY < (objY + objH)))
         ))
    {
#if defined(DEBUG_ILC_ICONRENDERING)
D(bug("[IconList] %s: Drawing Label '%s' .. %d, %d\n", __PRETTY_FUNCTION__, message->icon->ie_IconListEntry.label, labelX, labelY));
#endif
        if (message->icon->ie_IconListEntry.label && message->icon->ie_TxtBuf_DisplayedLabel)
        {
      char *curlabel_StrPtr;

            if ((message->icon->ie_Flags & ICONENTRY_FLAG_FOCUS) && ((BOOL)XGET(_win(obj), MUIA_Window_Activate)))
            {
                //Draw the focus box around the selected label ..
                if (data->icld__Option_LabelTextBorderHeight > 0)
                {
                    InvertPixelArray(data->icld_BufferRastPort,
                                iconlabelrect.MinX, iconlabelrect.MinY,
                                (iconlabelrect.MaxX - iconlabelrect.MinX) + 1, data->icld__Option_LabelTextBorderHeight);

                    InvertPixelArray(data->icld_BufferRastPort,
                                iconlabelrect.MinX, iconlabelrect.MaxY - (data->icld__Option_LabelTextBorderHeight - 1),
                                (iconlabelrect.MaxX - iconlabelrect.MinX) + 1, data->icld__Option_LabelTextBorderHeight);
                }
                if (data->icld__Option_LabelTextBorderWidth > 0)
                {
                    InvertPixelArray(data->icld_BufferRastPort,
                                iconlabelrect.MinX, iconlabelrect.MinY + data->icld__Option_LabelTextBorderHeight,
                                data->icld__Option_LabelTextBorderWidth, (((iconlabelrect.MaxY - iconlabelrect.MinY) + 1) - (data->icld__Option_LabelTextBorderHeight *  2)));

                    InvertPixelArray(data->icld_BufferRastPort,
                                iconlabelrect.MaxX - (data->icld__Option_LabelTextBorderWidth - 1), iconlabelrect.MinY  + data->icld__Option_LabelTextBorderHeight,
                                data->icld__Option_LabelTextBorderWidth, (((iconlabelrect.MaxY - iconlabelrect.MinY) + 1) - (data->icld__Option_LabelTextBorderHeight * 2)));
                }
            }

            SetFont(data->icld_BufferRastPort, data->icld_IconLabelFont);

            curlabel_TotalLines = message->icon->ie_SplitParts;
                  curlabel_CurrentLine = 0;

            if (curlabel_TotalLines == 0)
                curlabel_TotalLines = 1;

            if (!(data->icld__Option_LabelTextMultiLineOnFocus) || (data->icld__Option_LabelTextMultiLineOnFocus && (message->icon->ie_Flags & ICONENTRY_FLAG_FOCUS)))
            {
                if (curlabel_TotalLines > data->icld__Option_LabelTextMultiLine)
                    curlabel_TotalLines = data->icld__Option_LabelTextMultiLine;
            }
            else
                curlabel_TotalLines = 1;

            curlabel_StrPtr = message->icon->ie_TxtBuf_DisplayedLabel;
            
            ty = labelY - 1;

#if defined(DEBUG_ILC_ICONRENDERING)
D(bug("[IconList] %s: Font YSize %d Baseline %d\n", __PRETTY_FUNCTION__,data->icld_IconLabelFont->tf_YSize, data->icld_IconLabelFont->tf_Baseline));
#endif
            for (curlabel_CurrentLine = 0; curlabel_CurrentLine < curlabel_TotalLines; curlabel_CurrentLine++)
            {
          ULONG ie_LabelLength;

                if (curlabel_CurrentLine > 0) curlabel_StrPtr = curlabel_StrPtr + strlen(curlabel_StrPtr) + 1;
                if ((curlabel_CurrentLine >= (curlabel_TotalLines -1)) && (curlabel_TotalLines < message->icon->ie_SplitParts))
                {
                    char *tmpLine = curlabel_StrPtr;
                    ULONG tmpLen = strlen(tmpLine);

                    if ((curlabel_StrPtr = AllocVecPooled(data->icld_Pool, tmpLen + 1)) != NULL)
                    {
                        memset(curlabel_StrPtr, 0, tmpLen + 1);
                        strncpy(curlabel_StrPtr, tmpLine, tmpLen - 3);
                        strcat(curlabel_StrPtr , " ..");
                    }
                    else return FALSE;
                    
                }
                
                ie_LabelLength = strlen(curlabel_StrPtr);
                offset_y = 0;

                // Center message->icon's label
                tx = (labelX + (message->icon->ie_TxtBuf_DisplayedLabelWidth / 2) - (TextLength(data->icld_BufferRastPort, curlabel_StrPtr, strlen(curlabel_StrPtr)) / 2));

                if (message->icon->ie_TxtBuf_DisplayedLabelWidth < txtarea_width)
                    tx += ((txtarea_width - message->icon->ie_TxtBuf_DisplayedLabelWidth)/2);

                ty = ty + data->icld_IconLabelFont->tf_YSize;

                switch ( data->icld__Option_LabelTextMode )
                {
                    case ICON_TEXTMODE_DROPSHADOW:
                        SetAPen(data->icld_BufferRastPort, data->icld_LabelShadowPen);
                        Move(data->icld_BufferRastPort, tx + 1, ty + 1); 
                        Text(data->icld_BufferRastPort, curlabel_StrPtr, ie_LabelLength);
                        offset_y = 1;
                    case ICON_TEXTMODE_PLAIN:
                        SetAPen(data->icld_BufferRastPort, data->icld_LabelPen);
                        Move(data->icld_BufferRastPort, tx, ty); 
                        Text(data->icld_BufferRastPort, curlabel_StrPtr, ie_LabelLength);
                        break;
                        
                    default:
                        // Outline mode:
                        
                        SetSoftStyle(data->icld_BufferRastPort, FSF_BOLD, AskSoftStyle(data->icld_BufferRastPort));

                        SetAPen(data->icld_BufferRastPort, data->icld_LabelShadowPen);
                        Move(data->icld_BufferRastPort, tx + 1, ty ); 
                        Text(data->icld_BufferRastPort, curlabel_StrPtr, ie_LabelLength);
                        Move(data->icld_BufferRastPort, tx - 1, ty ); 
                        Text(data->icld_BufferRastPort, curlabel_StrPtr, ie_LabelLength);
                        Move(data->icld_BufferRastPort, tx, ty + 1);  
                        Text(data->icld_BufferRastPort, curlabel_StrPtr, ie_LabelLength);
                        Move(data->icld_BufferRastPort, tx, ty - 1);
                        Text(data->icld_BufferRastPort, curlabel_StrPtr, ie_LabelLength);
                        
                        SetAPen(data->icld_BufferRastPort, data->icld_LabelPen);
                        Move(data->icld_BufferRastPort, tx , ty ); 
                        Text(data->icld_BufferRastPort, curlabel_StrPtr, ie_LabelLength);
                        
                        SetSoftStyle(data->icld_BufferRastPort, FS_NORMAL, AskSoftStyle(data->icld_BufferRastPort));
                        offset_y = 2;
                        break;
                }
                if ((curlabel_CurrentLine >= (curlabel_TotalLines -1)) && (curlabel_TotalLines < message->icon->ie_SplitParts))
                {
                    FreeVecPooled(data->icld_Pool, curlabel_StrPtr);
                }
                ty = ty + offset_y;
            }

            /*date/size sorting has the date/size appended under the message->icon label*/

            if ((message->icon->ie_IconListEntry.type != ST_USERDIR) && ((data->icld_SortFlags & (ICONLIST_SORT_BY_SIZE|ICONLIST_SORT_BY_DATE)) != 0))
            {
                buf = NULL;
                SetFont(data->icld_BufferRastPort, data->icld_IconInfoFont);

                if ((data->icld_SortFlags & ICONLIST_SORT_MASK) == ICONLIST_SORT_BY_SIZE)
                {
                    buf = message->icon->ie_TxtBuf_SIZE;
                    txwidth = message->icon->ie_TxtBuf_SIZEWidth;
                }
                else if ((data->icld_SortFlags & ICONLIST_SORT_MASK) == ICONLIST_SORT_BY_DATE)
                {
                    if (message->icon->ie_Flags & ICONENTRY_FLAG_TODAY)
                    {
                        buf  = message->icon->ie_TxtBuf_TIME;
                        txwidth = message->icon->ie_TxtBuf_TIMEWidth;
                    }
                    else
                    {
                        buf = message->icon->ie_TxtBuf_DATE;
                        txwidth = message->icon->ie_TxtBuf_DATEWidth;
                    }
                }

                if (buf)
                {
                    ULONG ie_LabelLength = strlen(buf);
                    tx = labelX;

                    if (txwidth < txtarea_width)
                        tx += ((txtarea_width - txwidth)/2);

                    ty = labelY + ((data->icld__Option_LabelTextVerticalPadding + data->icld_IconLabelFont->tf_YSize ) * curlabel_TotalLines) + data->icld_IconInfoFont->tf_YSize;

                    switch ( data->icld__Option_LabelTextMode )
                    {
                        case ICON_TEXTMODE_DROPSHADOW:
                            SetAPen(data->icld_BufferRastPort, data->icld_InfoShadowPen);
                            Move(data->icld_BufferRastPort, tx + 1, ty + 1); Text(data->icld_BufferRastPort, buf, ie_LabelLength);
                        case ICON_TEXTMODE_PLAIN:
                            SetAPen(data->icld_BufferRastPort, data->icld_InfoPen);
                            Move(data->icld_BufferRastPort, tx, ty); Text(data->icld_BufferRastPort, buf, ie_LabelLength);
                            break;
                            
                        default:
                            // Outline mode..
                            SetSoftStyle(data->icld_BufferRastPort, FSF_BOLD, AskSoftStyle(data->icld_BufferRastPort));
                            SetAPen(data->icld_BufferRastPort, data->icld_InfoShadowPen);
                            
                            Move(data->icld_BufferRastPort, tx + 1, ty ); 
                            Text(data->icld_BufferRastPort, buf, ie_LabelLength);
                            Move(data->icld_BufferRastPort, tx - 1, ty );  
                            Text(data->icld_BufferRastPort, buf, ie_LabelLength);
                            Move(data->icld_BufferRastPort, tx, ty - 1 );  
                            Text(data->icld_BufferRastPort, buf, ie_LabelLength);
                            Move(data->icld_BufferRastPort, tx, ty + 1 );  
                            Text(data->icld_BufferRastPort, buf, ie_LabelLength);
                            
                            SetAPen(data->icld_BufferRastPort, data->icld_InfoPen);
                            
                            Move(data->icld_BufferRastPort, tx, ty );
                            Text(data->icld_BufferRastPort, buf, ie_LabelLength);
                            
                            SetSoftStyle(data->icld_BufferRastPort, FS_NORMAL, AskSoftStyle(data->icld_BufferRastPort));
                            break;
                    }
                }
            }
        }
    }
    
    return TRUE;
}
///
/**************************************************************************

**************************************************************************/
///IconList__MUIM_IconList_RethinkDimensions()
IPTR IconList__MUIM_IconList_RethinkDimensions(struct IClass *CLASS, Object *obj, struct MUIP_IconList_RethinkDimensions *message)
{
    struct IconList_DATA *data = INST_DATA(CLASS, obj);

    struct IconEntry  *icon = NULL;
    LONG              maxx = 0,
                      maxy = 0;
    struct Rectangle icon_rect;

#if defined(DEBUG_ILC_ICONPOSITIONING) && defined(DEBUG_ILC_FUNCS)
D(bug("[IconList]: %s()\n", __PRETTY_FUNCTION__));
#endif

#warning "TODO: Handle MADF_SETUP"
//  if (!(_flags(obj) & MADF_SETUP)) return FALSE;

    if (message->singleicon != NULL)
    {
        icon = message->singleicon;
        maxx = data->icld_AreaWidth - 1,
        maxy = data->icld_AreaHeight - 1;
#if defined(DEBUG_ILC_ICONPOSITIONING)
D(bug("[IconList] %s: SingleIcon - maxx = %d, maxy = %d\n", __PRETTY_FUNCTION__, maxx, maxy));
#endif
    }
    else icon = (struct IconEntry *)GetHead(&data->icld_IconList);

    while (icon != NULL)
    {
        if (icon->ie_DiskObj &&
            (icon->ie_Flags & ICONENTRY_FLAG_VISIBLE) &&
            (icon->ie_IconX != NO_ICON_POSITION) &&
            (icon->ie_IconY != NO_ICON_POSITION))
        {
            IconList_GetIconAreaRectangle(obj, data, icon, &icon_rect);

            icon_rect.MaxX += icon->ie_IconX + data->icld__Option_IconHorizontalSpacing;
            if ((data->icld__Option_IconListMode == ICON_LISTMODE_GRID) &&
                (icon->ie_AreaWidth < data->icld_IconAreaLargestWidth))
                icon_rect.MaxX += (data->icld_IconAreaLargestWidth - icon->ie_AreaWidth);

            icon_rect.MaxY += icon->ie_IconY + data->icld__Option_IconVerticalSpacing;

            if (icon_rect.MaxX > maxx) maxx = icon_rect.MaxX;
            if (icon_rect.MaxY > maxy) maxy = icon_rect.MaxY;
        }

        if (message->singleicon) break;

        icon = (struct IconEntry *)GetSucc(&icon->ie_IconNode);
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
///
/**************************************************************************
Checks weather we can place an icon with the given dimesions at the
suggested positions.

atx and aty are absolute positions
**************************************************************************/
/*
///IconList_CouldPlaceIcon()
static int IconList_CouldPlaceIcon(Object *obj, struct IconList_DATA *data, struct IconEntry *toplace, int atx, int aty)
{
    struct IconEntry *icon;
    struct Rectangle toplace_rect;

    IconList_GetIconAreaRectangle(obj, toplace, &toplace_rect);
    toplace_rect.MinX += atx;
    toplace_rect.MaxX += atx;
    toplace_rect.MinY += aty;
    toplace_rect.MaxY += aty;

    icon = GetHead(&data->icld_IconList);
    while (icon)
    {
        if (icon->ie_DiskObj && icon->ie_IconX != NO_ICON_POSITION && icon->ie_IconY != NO_ICON_POSITION)
        {
            struct Rectangle icon_rect;
            IconList_GetIconAreaRectangle(obj, icon, &icon_rect);
            icon_rect.MinX += icon->ie_IconX;
            icon_rect.MaxX += icon->ie_IconX;
            icon_rect.MinY += icon->ie_IconY;
            icon_rect.MaxY += icon->ie_IconY;

            if (RectAndRect(&icon_rect, &toplace_rect))
            return FALSE; *//* There is already an icon on this place *//*
        }
        icon = GetSucc(icon);
    }
    return 1;
}
///
*/

/**************************************************************************
Place the icon at atx and aty.

atx and aty are absolute positions
**************************************************************************/
/*
///IconList_PlaceIcon
static void IconList_PlaceIcon(Object *obj, struct IconList_DATA *data, struct IconEntry *toplace, int atx, int aty)
{
#if 0
    struct Rectangle toplace_rect;

    IconList_GetIconAreaRectangle(obj, toplace, &toplace_rect);
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
///
*/

///IconList__MUIM_IconList_PositionIcons()
/**************************************************************************
MUIM_PositionIcons - Place icons with NO_ICON_POSITION coords somewhere
**************************************************************************/

IPTR IconList__MUIM_IconList_PositionIcons(struct IClass *CLASS, Object *obj, struct MUIP_IconList_PositionIcons *message)
{
    struct IconList_DATA *data = INST_DATA(CLASS, obj);
    struct IconEntry    *icon = NULL, *pass_first = NULL;
    
    int left = data->icld__Option_IconHorizontalSpacing;
    int top = data->icld__Option_IconVerticalSpacing;
    int cur_x = left;
    int cur_y = top;
    int gridx = 0;
    int gridy = 0;
    int maxw = 0;        // Widest & Talest icon in a column or row.
    int maxh = 0;

    BOOL  next;
    struct Rectangle iconrect;

#if defined(DEBUG_ILC_ICONPOSITIONING) && defined(DEBUG_ILC_FUNCS)
D(bug("[IconList]: %s()\n", __PRETTY_FUNCTION__));
#endif

    // Now go to the actual positioning
    icon = (struct IconEntry *)GetHead(&data->icld_IconList);
    while (icon != NULL)
    {
        next = FALSE;
        if ((icon->ie_DiskObj != NULL) && (icon->ie_Flags & ICONENTRY_FLAG_VISIBLE))
        {
            next = TRUE;
            icon->ie_IconX = cur_x;
            icon->ie_IconY = cur_y;
    
            if (icon->ie_Flags & ICONENTRY_FLAG_SELECTED)
            {
                if (data->icld_SelectionLastClicked == NULL) data->icld_SelectionLastClicked = icon;
                if (data->icld_FocusIcon == NULL) data->icld_FocusIcon = icon;
            }

            if (data->icld__Option_IconListMode == ICON_LISTMODE_GRID)
            {
                maxw = data->icld_IconAreaLargestWidth + data->icld__Option_IconHorizontalSpacing;
                maxh = data->icld_IconLargestHeight + data->icld__Option_IconImageSpacing + data->icld_LabelLargestHeight + data->icld__Option_IconVerticalSpacing;
                gridx = maxw;
                gridy = maxh;
            }
            else
            {
                if (!(pass_first)) pass_first = icon;

                IconList_GetIconAreaRectangle(obj, data, icon, &iconrect);

                if (icon->ie_AreaWidth < maxw)
                    icon->ie_IconX += ( maxw - icon->ie_AreaWidth ) / 2;

                if ((maxw < icon->ie_AreaWidth) || (maxh < icon->ie_AreaHeight))
                {
                    if (maxw < icon->ie_AreaWidth) maxw = icon->ie_AreaWidth;
                    if (maxh < icon->ie_AreaHeight) maxh = icon->ie_AreaHeight;
                    if (pass_first != icon)
                    {
                        icon = pass_first;
                        cur_x = icon->ie_IconX;
                        cur_y = icon->ie_IconY;
                        continue;
                    }
                }

                if (data->icld_DisplayFlags & ICONLIST_DISP_VERTICAL)
                {
                    gridx = maxw;
                    gridy = icon->ie_AreaHeight + data->icld__Option_IconHorizontalSpacing;
                }
                else
                {
                    gridx = icon->ie_AreaWidth + data->icld__Option_IconVerticalSpacing;
                    gridy = maxh;
                }
            }
        }
        if ((icon = (struct IconEntry *)GetSucc(&icon->ie_IconNode)) != NULL)
        {
            if (next == TRUE)
            {
                if (data->icld_DisplayFlags & ICONLIST_DISP_VERTICAL)
                {
                    cur_y += gridy;
        
                    if ((cur_y >= data->icld_ViewHeight) ||
                        ((data->icld__Option_IconListMode == ICON_LISTMODE_ROUGH) && ((cur_y + icon->ie_AreaHeight - data->icld__Option_IconBorderOverlap) >= data->icld_ViewHeight)) ||
                        ((data->icld__Option_IconListMode == ICON_LISTMODE_GRID) && ((cur_y + data->icld_IconAreaLargestHeight - data->icld__Option_IconBorderOverlap) >= data->icld_ViewHeight)))
                    {
                        cur_x += maxw;
                        cur_y =  top;
                        pass_first = NULL;
                        maxw = 0;
                    }
                }
                else
                {
                    cur_x += gridx;
            
                    if ((cur_x >= data->icld_ViewWidth) ||
                        ((data->icld__Option_IconListMode == ICON_LISTMODE_ROUGH) && ((cur_x + icon->ie_AreaWidth - data->icld__Option_IconBorderOverlap) >= data->icld_ViewWidth)) ||
                        ((data->icld__Option_IconListMode == ICON_LISTMODE_GRID) && ((cur_x + data->icld_IconAreaLargestWidth - data->icld__Option_IconBorderOverlap) >= data->icld_ViewWidth)))
                    {
                        cur_x =  left;
                        cur_y += maxh;
                        pass_first = NULL;
                        maxh = 0;
                    }
                    else if (data->icld__Option_IconListMode == ICON_LISTMODE_GRID)
                    {
                        
                    }
                }
            }
        }
    }
    DoMethod(obj, MUIM_IconList_RethinkDimensions, NULL);
    return 0;
}
///

/*
///IconList_FixNoPositionIcons
static void IconList_FixNoPositionIcons(Object *obj, struct IconList_DATA *data)
{
struct IconEntry *icon;
int cur_x = data->icld_ViewX + 36;
int cur_y = data->icld_ViewY + 4;

icon = GetHead(&data->icld_IconList);
while (icon)
{
if (icon->ie_DiskObj && icon->ie_IconX == NO_ICON_POSITION && icon->ie_IconY == NO_ICON_POSITION)
{
int loops = 0;
int cur_x_save = cur_x;
int cur_y_save = cur_y;
struct Rectangle icon_rect;

IconList_GetIconAreaRectangle(obj, icon, &icon_rect);
icon_rect.MinX += cur_x - icon->ie_IconWidth/2 + data->icld_ViewX;
if (icon_rect.MinX < 0)
cur_x -= icon_rect.MinX;

while (!IconList_CouldPlaceIcon(obj, data, icon, cur_x - icon->ie_IconWidth/2, cur_y) && loops < 5000)
{
cur_y++;

if (cur_y + icon->ie_IconHeight > data->icld_ViewX + data->icld_ViewHeight) *//* on both sides -1 *//*
{
    cur_x += 72;
    cur_y = data->icld_ViewY + 4;
}
}

IconList_PlaceIcon(obj, data, icon, cur_x - icon->ie_IconWidth/2, cur_y);

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
///
*/

///OM_NEW()
/**************************************************************************
OM_NEW
**************************************************************************/
IPTR IconList__OM_NEW(struct IClass *CLASS, Object *obj, struct opSet *message)
{
    struct IconList_DATA  *data = NULL;
    struct TextFont      *icl_WindowFont = NULL;
//    struct RastPort      *icl_RastPort = NULL;

#if defined(DEBUG_ILC_FUNCS)
D(bug("[IconList]: %s()\n", __PRETTY_FUNCTION__));
#endif

    icl_WindowFont = (struct TextFont *) GetTagData(MUIA_Font, (IPTR) NULL, message->ops_AttrList);

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

#if defined(DEBUG_ILC_FUNCS)
D(bug("[IconList] %s: SELF = 0x%p, muiRenderInfo = 0x%p\n", __PRETTY_FUNCTION__, obj, muiRenderInfo(obj)));
#endif
    NewList((struct List*)&data->icld_IconList);
    NewList((struct List*)&data->icld_SelectionList);
    data->icld_IconLabelFont = icl_WindowFont;  

    /* Get/Set initial values */
#warning "TODO: TrimVolumeNames should be prefs settable"
    data->icld__Option_TrimVolumeNames = TRUE;
#warning "TODO: Adjust overlap by window border width"
    data->icld__Option_IconBorderOverlap = 10;

    data->icld__Option_IconListMode   = (UBYTE)GetTagData(MUIA_IconList_IconListMode, 0, message->ops_AttrList);
    data->icld__Option_LabelTextMode   = (UBYTE)GetTagData(MUIA_IconList_LabelText_Mode, 0, message->ops_AttrList);
    data->icld__Option_LabelTextMaxLen = (ULONG)GetTagData(MUIA_IconList_LabelText_MaxLineLen, ILC_ICONLABEL_MAXLINELEN_DEFAULT, message->ops_AttrList);

    if ( data->icld__Option_LabelTextMaxLen < ILC_ICONLABEL_SHORTEST )
        data->icld__Option_LabelTextMaxLen = ILC_ICONLABEL_MAXLINELEN_DEFAULT;

    data->icld__Option_LastLabelTextMaxLen = data->icld__Option_LabelTextMaxLen;

#if defined(DEBUG_ILC_FUNCS)
D(bug("[IconList] %s: MaxLineLen : %ld\n", __PRETTY_FUNCTION__, data->icld__Option_LabelTextMaxLen));
#endif
    data->ehn.ehn_Events   = IDCMP_MOUSEBUTTONS | IDCMP_RAWKEY;
    data->ehn.ehn_Priority = 0;
    data->ehn.ehn_Flags    = 0;
    data->ehn.ehn_Object   = obj;
    data->ehn.ehn_Class    = CLASS;

    data->icld_SortFlags    = ICONLIST_SORT_BY_NAME;
    data->icld_DisplayFlags = ICONLIST_DISP_SHOWINFO;

    __iconlist_UpdateLabels_hook.h_Entry = (HOOKFUNC)IconList__HookFunc_UpdateLabelsFunc;
    
    DoMethod
    (
        obj, MUIM_Notify, MUIA_IconList_LabelText_MaxLineLen, MUIV_EveryTime,
        (IPTR)obj, 3, 
        MUIM_CallHook, &__iconlist_UpdateLabels_hook, (IPTR)CLASS
    );

    DoMethod
    (
        obj, MUIM_Notify, MUIA_IconList_LabelText_MultiLine, MUIV_EveryTime,
        (IPTR)obj, 3, 
        MUIM_CallHook, &__iconlist_UpdateLabels_hook, (IPTR)CLASS
    );

#if defined(DEBUG_ILC_FUNCS)
D(bug("[IconList] obj = %ld\n", obj));
#endif
    return (IPTR)obj;
}
///

///OM_DISPOSE()
/**************************************************************************
OM_DISPOSE
**************************************************************************/
IPTR IconList__OM_DISPOSE(struct IClass *CLASS, Object *obj, Msg message)
{
    struct IconList_DATA *data = INST_DATA(CLASS, obj);
    struct IconEntry    *node = NULL;

#if defined(DEBUG_ILC_FUNCS)
D(bug("[IconList]: %s()\n", __PRETTY_FUNCTION__));
#endif
    
#ifdef __AROS__
    ForeachNode(&data->icld_IconList, node)
#else
    Foreach_Node(&data->icld_IconList, node);
#endif
    {
        if (node->ie_DiskObj) FreeDiskObject(node->ie_DiskObj);
    }

    if (data->icld_Pool) DeletePool(data->icld_Pool);

    DoSuperMethodA(CLASS,obj,message);
    return 0;
}
///

///OM_SET()
/**************************************************************************
OM_SET
**************************************************************************/
IPTR IconList__OM_SET(struct IClass *CLASS, Object *obj, struct opSet *message)
{
    struct IconList_DATA *data = INST_DATA(CLASS, obj);
    struct TagItem    *tag = NULL,
                        *tags = NULL;

    WORD             oldleft = data->icld_ViewX,
                         oldtop = data->icld_ViewY;
                         //oldwidth = data->icld_ViewWidth,
                         //oldheight = data->icld_ViewHeight;

#if defined(DEBUG_ILC_FUNCS) && defined(DEBUG_ILC_ATTRIBS)
D(bug("[IconList]: %s()\n", __PRETTY_FUNCTION__));
#endif

    /* parse initial taglist */
    for (tags = message->ops_AttrList; (tag = NextTagItem((TAGITEM)&tags)); )
    {
        switch (tag->ti_Tag)
        {
            case MUIA_Virtgroup_Left:
#if defined(DEBUG_ILC_ATTRIBS)
D(bug("[IconList] %s: MUIA_Virtgroup_Left %ld\n", __PRETTY_FUNCTION__, tag->ti_Data));
#endif
                if (data->icld_ViewX != tag->ti_Data)
                    data->icld_ViewX = tag->ti_Data;
                break;
    
            case MUIA_Virtgroup_Top:
#if defined(DEBUG_ILC_ATTRIBS)
D(bug("[IconList] %s: MUIA_Virtgroup_Top %ld\n", __PRETTY_FUNCTION__, tag->ti_Data));
#endif
                if (data->icld_ViewY != tag->ti_Data)
                    data->icld_ViewY = tag->ti_Data;
                break;

            case MUIA_IconList_Rastport:
#if defined(DEBUG_ILC_ATTRIBS)
D(bug("[IconList] %s: MUIA_IconList_Rastport 0x%p\n", __PRETTY_FUNCTION__, tag->ti_Data));
#endif
                data->icld_DisplayRastPort = (struct RastPort*)tag->ti_Data;
                data->icld_DrawOffsetX = _mleft(obj);
                data->icld_DrawOffsetY = _mtop(obj);
                if (data->icld_BufferRastPort != NULL)
                {
                    //Buffer still set!?!?!
                }
                SET(obj, MUIA_IconList_BufferRastport, tag->ti_Data);
                break;

            case MUIA_IconList_BufferRastport:
#if defined(DEBUG_ILC_ATTRIBS)
D(bug("[IconList] %s: MUIA_IconList_BufferRastport 0x%p\n", __PRETTY_FUNCTION__, tag->ti_Data));
#endif
                data->icld_BufferRastPort = (struct RastPort*)tag->ti_Data;
                break;

            case MUIA_Font:
#if defined(DEBUG_ILC_ATTRIBS)
D(bug("[IconList] %s: MUIA_Font 0x%p\n", __PRETTY_FUNCTION__, tag->ti_Data));
#endif
                data->icld_IconLabelFont = (struct TextFont*)tag->ti_Data;
                break;

            case MUIA_IconList_LabelInfoText_Font:
#if defined(DEBUG_ILC_ATTRIBS)
D(bug("[IconList] %s: MUIA_IconList_LabelInfoText_Font 0x%p\n", __PRETTY_FUNCTION__, tag->ti_Data));
#endif
                data->icld_IconInfoFont = (struct TextFont*)tag->ti_Data;
                break;
            
            case MUIA_IconList_DisplayFlags:
                {
#if defined(DEBUG_ILC_ATTRIBS)
D(bug("[IconList] %s: MUIA_IconList_DisplayFlags %08x\n", __PRETTY_FUNCTION__, tag->ti_Data));
#endif
                    data->icld_DisplayFlags = (ULONG)tag->ti_Data;

                    if (data->icld_DisplayFlags & ICONLIST_DISP_BUFFERED)
                    {
                        struct BitMap *tmp_BuffBitMap = NULL;
                        ULONG tmp_RastDepth;

#if defined(DEBUG_ILC_ATTRIBS) && defined(DEBUG_ILC_ICONRENDERING)
D(bug("[IconList] %s: MUIA_IconList_DisplayFlags & ICONLIST_DISP_BUFFERED\n", __PRETTY_FUNCTION__));
#endif
                        if ((data->icld_BufferRastPort) && (data->icld_BufferRastPort != data->icld_DisplayRastPort))
                        {
                            //Free up the buffers rastport and bitmap so we can replace them ..
                            FreeBitMap(data->icld_BufferRastPort->BitMap);
                            FreeRastPort(data->icld_BufferRastPort);
                            SET(obj, MUIA_IconList_BufferRastport, NULL);
                        }
                        tmp_RastDepth = GetCyberMapAttr(data->icld_DisplayRastPort->BitMap, CYBRMATTR_DEPTH);
                        if ((tmp_BuffBitMap = AllocBitMap(data->icld_ViewWidth,
                                            data->icld_ViewHeight,
                                            tmp_RastDepth,
                                            BMF_CLEAR,
                                            data->icld_DisplayRastPort->BitMap))!=NULL)
                        {
                          if ((data->icld_BufferRastPort = CreateRastPort())!=NULL)
                          {
                            data->icld_BufferRastPort->BitMap = tmp_BuffBitMap;
                            SET(obj, MUIA_IconList_BufferRastport, data->icld_BufferRastPort);
                            data->icld_DrawOffsetX = 0;
                            data->icld_DrawOffsetY = 0;
                          }
                          else
                          {
                            FreeBitMap(tmp_BuffBitMap);
                            data->icld_BufferRastPort = data->icld_DisplayRastPort;
                            data->icld_DrawOffsetX = _mleft(obj);
                            data->icld_DrawOffsetY = _mtop(obj);
                          }
                        }
                    }
                    else
                    {
                        if ((data->icld_BufferRastPort) && (data->icld_BufferRastPort != data->icld_DisplayRastPort))
                        {
                            //Free up the buffers rastport and bitmap since they are no longer needed ..
                            FreeBitMap(data->icld_BufferRastPort->BitMap);
                            FreeRastPort(data->icld_BufferRastPort);
                            data->icld_BufferRastPort = data->icld_DisplayRastPort;
                            data->icld_DrawOffsetX = _mleft(obj);
                            data->icld_DrawOffsetY = _mtop(obj);
                        }
                    }
                    SET(obj, MUIA_IconList_Changed, TRUE);
                }
                break;

            case MUIA_IconList_SortFlags:
#if defined(DEBUG_ILC_ATTRIBS)
bug("[IconList] %s: MUIA_IconList_SortFlags %08x\n", __PRETTY_FUNCTION__, tag->ti_Data);
#endif
                data->icld_SortFlags = (ULONG)tag->ti_Data;
                break;
                
            case MUIA_IconList_IconListMode:
#if defined(DEBUG_ILC_ATTRIBS)
D(bug("[IconList] %s: MUIA_IconList_IconListMode %d\n", __PRETTY_FUNCTION__, tag->ti_Data));
#endif
                data->icld__Option_IconListMode = (UBYTE)tag->ti_Data;
                break;
            
            case MUIA_IconList_LabelText_Mode:
#if defined(DEBUG_ILC_ATTRIBS)
D(bug("[IconList] %s: MUIA_IconList_LabelText_Mode %d\n", __PRETTY_FUNCTION__, tag->ti_Data));
#endif
                data->icld__Option_LabelTextMode = (UBYTE)tag->ti_Data;
                break;
            
            case MUIA_IconList_LabelText_MaxLineLen:
#if defined(DEBUG_ILC_ATTRIBS)
D(bug("[IconList] %s: MUIA_IconList_LabelText_MaxLineLen %d\n", __PRETTY_FUNCTION__, tag->ti_Data));
#endif
                if (tag->ti_Data >= ILC_ICONLABEL_SHORTEST)
                {
                    data->icld__Option_LabelTextMaxLen = (ULONG)tag->ti_Data;
                }
                else
                {
                    data->icld__Option_LabelTextMaxLen = ILC_ICONLABEL_MAXLINELEN_DEFAULT;
                }
                break;
            
            case MUIA_IconList_LabelText_MultiLine:
#if defined(DEBUG_ILC_ATTRIBS)
D(bug("[IconList] %s: MUIA_IconList_LabelText_MultiLine %d\n", __PRETTY_FUNCTION__, tag->ti_Data));
#endif
                data->icld__Option_LabelTextMultiLine = (ULONG)tag->ti_Data;
                if (data->icld__Option_LabelTextMultiLine == 0)data->icld__Option_LabelTextMultiLine = 1;
                break;

            case MUIA_IconList_LabelText_MultiLineOnFocus:
#if defined(DEBUG_ILC_ATTRIBS)
D(bug("[IconList] %s: MUIA_IconList_LabelText_MultiLineOnFocus %d\n", __PRETTY_FUNCTION__, tag->ti_Data));
#endif
                data->icld__Option_LabelTextMultiLineOnFocus = (BOOL)tag->ti_Data;
                break;

            case MUIA_IconList_Icon_HorizontalSpacing:
#if defined(DEBUG_ILC_ATTRIBS)
D(bug("[IconList] %s: MUIA_IconList_Icon_HorizontalSpacing %d\n", __PRETTY_FUNCTION__, tag->ti_Data));
#endif
                data->icld__Option_IconHorizontalSpacing = (UBYTE)tag->ti_Data;
                break;

            case MUIA_IconList_Icon_VerticalSpacing:
#if defined(DEBUG_ILC_ATTRIBS)
D(bug("[IconList] %s: MUIA_IconList_Icon_VerticalSpacing %d\n", __PRETTY_FUNCTION__, tag->ti_Data));
#endif
                data->icld__Option_IconVerticalSpacing = (UBYTE)tag->ti_Data;
                break;

            case MUIA_IconList_Icon_ImageSpacing:
#if defined(DEBUG_ILC_ATTRIBS)
D(bug("[IconList] %s: MUIA_IconList_Icon_ImageSpacing %d\n", __PRETTY_FUNCTION__, tag->ti_Data));
#endif
                data->icld__Option_IconImageSpacing = (UBYTE)tag->ti_Data;
                break;

            case MUIA_IconList_LabelText_HorizontalPadding:
#if defined(DEBUG_ILC_ATTRIBS)
D(bug("[IconList] %s: MUIA_IconList_LabelText_HorizontalPadding %d\n", __PRETTY_FUNCTION__, tag->ti_Data));
#endif
                data->icld__Option_LabelTextHorizontalPadding = (UBYTE)tag->ti_Data;
                break;

            case MUIA_IconList_LabelText_VerticalPadding:
#if defined(DEBUG_ILC_ATTRIBS)
D(bug("[IconList] %s: MUIA_IconList_LabelText_VerticalPadding %d\n", __PRETTY_FUNCTION__, tag->ti_Data));
#endif
                data->icld__Option_LabelTextVerticalPadding = (UBYTE)tag->ti_Data;
                break;

            case MUIA_IconList_LabelText_BorderWidth:
#if defined(DEBUG_ILC_ATTRIBS)
D(bug("[IconList] %s: MUIA_IconList_LabelText_BorderWidth %d\n", __PRETTY_FUNCTION__, tag->ti_Data));
#endif
                data->icld__Option_LabelTextBorderWidth = (UBYTE)tag->ti_Data;
                break;

            case MUIA_IconList_LabelText_BorderHeight:
#if defined(DEBUG_ILC_ATTRIBS)
D(bug("[IconList] %s: MUIA_IconList_LabelText_BorderHeight %d\n", __PRETTY_FUNCTION__, tag->ti_Data));
#endif
                data->icld__Option_LabelTextBorderHeight = (UBYTE)tag->ti_Data;
                break;

            case MUIA_IconList_LabelText_Pen:
                data->icld_LabelPen = (ULONG)tag->ti_Data;
                break;

            case MUIA_IconList_LabelText_ShadowPen:
                data->icld_LabelShadowPen = (ULONG)tag->ti_Data;
                break;

            case MUIA_IconList_LabelInfoText_Pen:
                data->icld_InfoPen = (ULONG)tag->ti_Data;
                break;

            case MUIA_IconList_LabelInfoText_ShadowPen:
                data->icld_InfoShadowPen = (ULONG)tag->ti_Data;
                break;

            /* Settings defined by the view class */
            case MUIA_IconListview_FixedBackground:
#if defined(DEBUG_ILC_ATTRIBS)
D(bug("[IconList] %s: MUIA_IconListview_FixedBackground\n", __PRETTY_FUNCTION__));
#endif
                data->icld__Option_IconListFixedBackground = (BOOL)tag->ti_Data;
                break;

            case MUIA_IconListview_ScaledBackground:
#if defined(DEBUG_ILC_ATTRIBS)
D(bug("[IconList] %s: MUIA_IconListview_ScaledBackground\n", __PRETTY_FUNCTION__));
#endif
                data->icld__Option_IconListScaledBackground = (BOOL)tag->ti_Data;
                break;

            /* We listen for MUIA_Background and set default values for known types */
            case MUIA_Background:
#if defined(DEBUG_ILC_ATTRIBS) && defined(DEBUG_ILC_ICONRENDERING)
D(bug("[IconList] %s: MUIA_Background\n", __PRETTY_FUNCTION__));
#endif
                {
                    char *bgmode_string = (char *)tag->ti_Data;
                    BYTE this_mode = bgmode_string[0] - 48;

#if defined(DEBUG_ILC_ATTRIBS) && defined(DEBUG_ILC_ICONRENDERING)
    D(bug("[IconList] %s: MUIA_Background | MUI BG Mode = %d\n", __PRETTY_FUNCTION__, this_mode));
#endif
                    switch (this_mode)
                    {
                    case 0:
                        //MUI Pattern
                        NNSET(obj, MUIA_IconListview_FixedBackground, FALSE);
                        NNSET(obj, MUIA_IconListview_ScaledBackground, FALSE);
                        break;
                    case 2:
                        //MUI RGB color
                        NNSET(obj, MUIA_IconListview_FixedBackground, FALSE);
                        NNSET(obj, MUIA_IconListview_ScaledBackground, FALSE);
                        break;
                    case 7:
                        //Zune Gradient
                        NNSET(obj, MUIA_IconListview_FixedBackground, TRUE);
                        NNSET(obj, MUIA_IconListview_ScaledBackground, TRUE);
                        break;
                    case 5:
                        //Image
                        NNSET(obj, MUIA_IconListview_FixedBackground, FALSE);
                        NNSET(obj, MUIA_IconListview_ScaledBackground, FALSE);
                        break;
                    }
                }
                break;
            case MUIA_IconList_IconsDropped:
                data->icld_DragDropEvent = (struct IconList_Drop_Event *)tag->ti_Data;
                break;
        }
    }

#if defined(DEBUG_ILC_ATTRIBS)
D(bug("[IconList] %s(), out of switch\n", __PRETTY_FUNCTION__));
#endif
    if ((oldleft != data->icld_ViewX) || (oldtop != data->icld_ViewY))
    {
        data->icld_UpdateMode = UPDATE_SCROLL;
        data->update_scrolldx = data->icld_ViewX - oldleft;
        data->update_scrolldy = data->icld_ViewY - oldtop;
#if defined(DEBUG_ILC_ATTRIBS)
D(bug("[IconList] %s(), call MUI_Redraw()\n", __PRETTY_FUNCTION__));
#endif
        MUI_Redraw(obj, MADF_DRAWUPDATE);
    }

#if defined(DEBUG_ILC_ATTRIBS)
D(bug("[IconList] %s(), call DoSuperMethodA()\n", __PRETTY_FUNCTION__));
#endif
    return DoSuperMethodA(CLASS, obj, (Msg)message);
}
///

///OM_GET()
/**************************************************************************
OM_GET
**************************************************************************/
IPTR IconList__OM_GET(struct IClass *CLASS, Object *obj, struct opGet *message)
{
    /* small macro to simplify return value storage */
#define STORE *(message->opg_Storage)
    struct IconList_DATA *data = INST_DATA(CLASS, obj);

#if defined(DEBUG_ILC_FUNCS) && defined(DEBUG_ILC_ATTRIBS)
D(bug("[IconList]: %s()\n", __PRETTY_FUNCTION__));
#endif

    switch (message->opg_AttrID)
    {
        case MUIA_IconList_Rastport:                    STORE = (IPTR)data->icld_DisplayRastPort; return 1;
        case MUIA_IconList_BufferRastport:              STORE = (IPTR)data->icld_BufferRastPort; return 1;
        case MUIA_IconList_BufferLeft:                  STORE = (IPTR)data->icld_DrawOffsetX; return 1;
        case MUIA_IconList_BufferTop:                   STORE = (IPTR)data->icld_DrawOffsetY; return 1;
        case MUIA_IconList_BufferWidth:
        case MUIA_IconList_Width:                       STORE = (IPTR)data->icld_AreaWidth; return 1;
        case MUIA_IconList_BufferHeight:
        case MUIA_IconList_Height:                      STORE = (IPTR)data->icld_AreaHeight; return 1;
        case MUIA_IconList_IconsDropped:                STORE = (IPTR)data->icld_DragDropEvent; return 1;
        case MUIA_IconList_Clicked:                     STORE = (IPTR)&data->icld_ClickEvent; return 1;
        case MUIA_IconList_IconListMode:                STORE = (IPTR)data->icld__Option_IconListMode; return 1;
        case MUIA_IconList_LabelText_Mode:              STORE = (IPTR)data->icld__Option_LabelTextMode; return 1;
        case MUIA_IconList_LabelText_MaxLineLen:        STORE = (IPTR)data->icld__Option_LabelTextMaxLen; return 1;
        case MUIA_IconList_LabelText_MultiLine:         STORE = (IPTR)data->icld__Option_LabelTextMultiLine; return 1;
        case MUIA_IconList_LabelText_MultiLineOnFocus:  STORE = (IPTR)data->icld__Option_LabelTextMultiLineOnFocus; return 1;
        case MUIA_IconList_DisplayFlags:                STORE = (IPTR)data->icld_DisplayFlags; return 1;
        case MUIA_IconList_SortFlags:                   STORE = (IPTR)data->icld_SortFlags; return 1;

        case MUIA_IconList_FocusIcon:                   STORE = (IPTR)data->icld_FocusIcon; return 1;

        case MUIA_Font:                                 STORE = (IPTR)data->icld_IconLabelFont; return 1;
        case MUIA_IconList_LabelText_Pen:               STORE = (IPTR)data->icld_LabelPen; return 1;
        case MUIA_IconList_LabelText_ShadowPen:         STORE = (IPTR)data->icld_LabelShadowPen; return 1;
        case MUIA_IconList_LabelInfoText_Font:          STORE = (IPTR)data->icld_IconInfoFont; return 1;
        case MUIA_IconList_LabelInfoText_Pen:           STORE = (IPTR)data->icld_InfoPen; return 1;
        case MUIA_IconList_LabelInfoText_ShadowPen:     STORE = (IPTR)data->icld_InfoShadowPen; return 1;

        case MUIA_IconList_Icon_HorizontalSpacing:      STORE = (IPTR)data->icld__Option_IconHorizontalSpacing; return 1;
        case MUIA_IconList_Icon_VerticalSpacing:        STORE = (IPTR)data->icld__Option_IconVerticalSpacing; return 1;
        case MUIA_IconList_Icon_ImageSpacing:           STORE = (IPTR)data->icld__Option_IconImageSpacing; return 1;
        case MUIA_IconList_LabelText_HorizontalPadding: STORE = (IPTR)data->icld__Option_LabelTextHorizontalPadding; return 1;
        case MUIA_IconList_LabelText_VerticalPadding:   STORE = (IPTR)data->icld__Option_LabelTextVerticalPadding; return 1;
        case MUIA_IconList_LabelText_BorderWidth:       STORE = (IPTR)data->icld__Option_LabelTextBorderWidth; return 1;
        case MUIA_IconList_LabelText_BorderHeight:      STORE = (IPTR)data->icld__Option_LabelTextBorderHeight; return 1;

        /* Settings defined by the view class */
        case MUIA_IconListview_FixedBackground:         STORE = (IPTR)data->icld__Option_IconListFixedBackground; return 1;
        case MUIA_IconListview_ScaledBackground:        STORE = (IPTR)data->icld__Option_IconListScaledBackground; return 1;
            
        /* ICON obj Changes */
        case MUIA_Virtgroup_Left:                       STORE = (IPTR)data->icld_ViewX; return 1;
        case MUIA_Virtgroup_Top:                        STORE = (IPTR)data->icld_ViewY; return 1;
        case MUIA_Group_ChildList:                      STORE = (IPTR)&data->icld_IconList; return 1; /* Get our list object */
    }

    if (DoSuperMethodA(CLASS, obj, (Msg) message))
        return 1;

    return 0;
#undef STORE
}
///

IPTR IconList__OM_ADDMEMBER(struct IClass *CLASS, Object *obj, APTR message)
{
#if defined(DEBUG_ILC_FUNCS)
D(bug("[IconList]: %s()\n", __PRETTY_FUNCTION__));
#endif

    return NULL;
}

IPTR IconList__OM_REMMEMBER(struct IClass *CLASS, Object *obj, APTR message)
{
#if defined(DEBUG_ILC_FUNCS)
D(bug("[IconList]: %s()\n", __PRETTY_FUNCTION__));
#endif

    return NULL;
}

///MUIM_Setup()
/**************************************************************************
MUIM_Setup
**************************************************************************/
IPTR IconList__MUIM_Setup(struct IClass *CLASS, Object *obj, struct MUIP_Setup *message)
{
    struct IconList_DATA *data = INST_DATA(CLASS, obj);
    struct IconEntry    *node = NULL;
    IPTR                 geticon_error = 0;

#if defined(DEBUG_ILC_FUNCS)
D(bug("[IconList]: %s()\n", __PRETTY_FUNCTION__));
#endif

    if (!DoSuperMethodA(CLASS, obj, (Msg) message)) return 0;

    DoMethod(_win(obj), MUIM_Window_AddEventHandler, (IPTR)&data->ehn);

    /* Get Internal Objects to use if not set .. */
    data->icld_DisplayRastPort = NULL;
    data->icld_BufferRastPort = NULL;

    if (data->icld_IconLabelFont == NULL)   data->icld_IconLabelFont = _font(obj);
    if (data->icld_IconInfoFont == NULL)    data->icld_IconInfoFont = data->icld_IconLabelFont;
#if defined(DEBUG_ILC_ICONRENDERING)
D(bug("[IconList] %s: Use Font @ 0x%p, RastPort @ 0x%p\n", __PRETTY_FUNCTION__, data->icld_IconLabelFont, data->icld_BufferRastPort ));
#endif

    /* Set our base options .. */
    data->icld_LabelPen                           = _pens(obj)[MPEN_SHINE];
    data->icld_LabelShadowPen                     = _pens(obj)[MPEN_SHADOW];
    data->icld_InfoPen                            = _pens(obj)[MPEN_SHINE];
    data->icld_InfoShadowPen                      = _pens(obj)[MPEN_SHADOW];

    data->icld__Option_LabelTextMultiLine         = 1;
    data->icld__Option_LastLabelTextMultiLine = data->icld__Option_LabelTextMultiLine;

    data->icld__Option_LabelTextMultiLineOnFocus  = FALSE;
    
    data->icld__Option_IconHorizontalSpacing      = ILC_ICON_HORIZONTALMARGIN_DEFAULT;
    data->icld__Option_IconVerticalSpacing        = ILC_ICON_VERTICALMARGIN_DEFAULT;
    data->icld__Option_IconImageSpacing           = ILC_ICONLABEL_IMAGEMARGIN_DEFAULT;
    data->icld__Option_LabelTextHorizontalPadding = ILC_ICONLABEL_HORIZONTALTEXTMARGIN_DEFAULT;
    data->icld__Option_LabelTextVerticalPadding   = ILC_ICONLABEL_VERTICALTEXTMARGIN_DEFAULT;
    data->icld__Option_LabelTextBorderWidth       = ILC_ICONLABEL_BORDERWIDTH_DEFAULT;
    data->icld__Option_LabelTextBorderHeight      = ILC_ICONLABEL_BORDERHEIGHT_DEFAULT;

#ifdef __AROS__
    ForeachNode(&data->icld_IconList, node)
#else
    Foreach_Node(&data->icld_IconList, node);
#endif
    {
        if (!node->ie_DiskObj)
        {
            IPTR iconlistScreen = _screen(obj);
#if defined(DEBUG_ILC_ICONRENDERING)
bug("[IconList] %s: IconList Screen @ 0x%p)\n", __PRETTY_FUNCTION__, iconlistScreen);
#endif
            if (!(node->ie_DiskObj = GetIconTags(node->ie_IconNode.ln_Name,
                                                (iconlistScreen) ? ICONGETA_Screen : TAG_IGNORE, iconlistScreen,
                                                (iconlistScreen) ? ICONGETA_RemapIcon : TAG_IGNORE, TRUE,
                                                ICONGETA_GenerateImageMasks, TRUE,
                                                ICONGETA_FailIfUnavailable, FALSE,
                                                ICONA_ErrorCode, &geticon_error,
                                                TAG_DONE)))
            {
#if defined(DEBUG_ILC_ICONRENDERING)
D(bug("[IconList] %s: Failed to obtain Icon '%s's diskobj! (error code = 0x%p)\n", __PRETTY_FUNCTION__, node->ie_IconNode.ln_Name, geticon_error));
#endif
                /*  We should probably remove this node if the icon cant be obtained ? */
            }
        }
    }
    return 1;
}
///

///MUIM_Show()
/**************************************************************************
MUIM_Show
**************************************************************************/
IPTR IconList__MUIM_Show(struct IClass *CLASS, Object *obj, struct MUIP_Show *message)
{
    struct IconList_DATA *data = INST_DATA(CLASS, obj);
    LONG                newleft,
                        newtop;
    IPTR                rc;

#if defined(DEBUG_ILC_FUNCS)
D(bug("[IconList]: %s()\n", __PRETTY_FUNCTION__));
#endif

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
            SetAttrs(obj, MUIA_Virtgroup_Left, newleft,
                MUIA_Virtgroup_Top, newtop,
                TAG_DONE);
        }

        /* Get Internal Objects to use if not set .. */
        if (data->icld_DisplayRastPort == NULL)
        {
            if (_rp(obj) != NULL)
            {
                data->icld_DisplayRastPort = CloneRastPort(_rp(obj));
            }
            else
            {
#if defined(DEBUG_ILC_ICONRENDERING)
D(bug("[IconList] IconList__MUIM_Show: ERROR - NULL RastPort!\n"));
#endif
            }
        }

        if (data->icld_DisplayFlags & ICONLIST_DISP_BUFFERED)
        {
            struct BitMap *tmp_BuffBitMap = NULL;
            ULONG tmp_RastDepth = GetCyberMapAttr(data->icld_DisplayRastPort->BitMap, CYBRMATTR_DEPTH);
            if ((tmp_BuffBitMap = AllocBitMap(data->icld_ViewWidth,
                                data->icld_ViewHeight,
                                tmp_RastDepth,
                                BMF_CLEAR,
                                data->icld_DisplayRastPort->BitMap))!=NULL)
            {
              if ((data->icld_BufferRastPort = CreateRastPort())!=NULL)
              {
                data->icld_BufferRastPort->BitMap = tmp_BuffBitMap;
                data->icld_DrawOffsetX = 0;
                data->icld_DrawOffsetY = 0;
              }
              else
              {
                FreeBitMap(tmp_BuffBitMap);
                data->icld_BufferRastPort = data->icld_DisplayRastPort;
                data->icld_DrawOffsetX = _mleft(obj);
                data->icld_DrawOffsetY = _mtop(obj);
              }
            }
        }
        else
        {
            data->icld_BufferRastPort = data->icld_DisplayRastPort;
            data->icld_DrawOffsetX = _mleft(obj);
            data->icld_DrawOffsetY = _mtop(obj);
        }
        if (data->icld_IconLabelFont == NULL)       data->icld_IconLabelFont = _font(obj);
        if (data->icld_IconInfoFont == NULL)        data->icld_IconInfoFont = data->icld_IconLabelFont;
#if defined(DEBUG_ILC_ICONRENDERING)
D(bug("[IconList] IconList__MUIM_Show: Use Font @ 0x%p, RastPort @ 0x%p\n", data->icld_IconLabelFont, data->icld_BufferRastPort ));
#endif

        if ((data->icld_BufferRastPort) && (data->icld_IconLabelFont))
            SetFont(data->icld_BufferRastPort, data->icld_IconLabelFont);
    }
    return rc;
}
///

///MUIM_Hide()
/**************************************************************************
MUIM_Hide
**************************************************************************/
IPTR IconList__MUIM_Hide(struct IClass *CLASS, Object *obj, struct MUIP_Hide *message)
{
    struct IconList_DATA *data = INST_DATA(CLASS, obj);
    IPTR                rc;

#if defined(DEBUG_ILC_FUNCS)
D(bug("[IconList]: %s()\n", __PRETTY_FUNCTION__));
#endif

    if ((rc = DoSuperMethodA(CLASS, obj, (Msg)message)))
    {
        if ((data->icld_BufferRastPort) && (data->icld_BufferRastPort != data->icld_DisplayRastPort))
        {
            FreeBitMap(data->icld_BufferRastPort->BitMap);
            FreeRastPort(data->icld_BufferRastPort);
        }

        data->icld_BufferRastPort = NULL;
        
        if (data->icld_DisplayRastPort)
            FreeRastPort(data->icld_DisplayRastPort);

        data->icld_DisplayRastPort = NULL;
    }
    return rc;
}
///

///MUIM_Cleanup()
/**************************************************************************
MUIM_Cleanup
**************************************************************************/
IPTR IconList__MUIM_Cleanup(struct IClass *CLASS, Object *obj, struct MUIP_Cleanup *message)
{
    struct IconList_DATA *data = INST_DATA(CLASS, obj);
    struct IconEntry    *node = NULL;

#if defined(DEBUG_ILC_FUNCS)
D(bug("[IconList]: %s()\n", __PRETTY_FUNCTION__));
#endif

#ifdef __AROS__
    ForeachNode(&data->icld_IconList, node)
#else
    Foreach_Node(&data->icld_IconList, node);
#endif
    {
        if (node->ie_DiskObj)
        {
            FreeDiskObject(node->ie_DiskObj);
            node->ie_DiskObj = NULL;
        }
    }

    DoMethod(_win(obj), MUIM_Window_RemEventHandler, (IPTR)&data->ehn);

    return DoSuperMethodA(CLASS, obj, (Msg)message);
}
///

///MUIM_AskMinMax()
/**************************************************************************
MUIM_AskMinMax
**************************************************************************/
IPTR IconList__MUIM_AskMinMax(struct IClass *CLASS, Object *obj, struct MUIP_AskMinMax *message)
{
    ULONG rc = DoSuperMethodA(CLASS, obj, (Msg) message);

#if defined(DEBUG_ILC_FUNCS)
D(bug("[IconList]: %s()\n", __PRETTY_FUNCTION__));
#endif

    message->MinMaxInfo->MinWidth  += 96;
    message->MinMaxInfo->MinHeight += 64;

    message->MinMaxInfo->DefWidth  += 200;
    message->MinMaxInfo->DefHeight += 180;

    message->MinMaxInfo->MaxWidth = MUI_MAXMAX;
    message->MinMaxInfo->MaxHeight = MUI_MAXMAX;

    return rc;
}
///

///MUIM_Layout()
/**************************************************************************
MUIM_Layout
**************************************************************************/
IPTR IconList__MUIM_Layout(struct IClass *CLASS, Object *obj,struct MUIP_Layout *message)
{
    struct IconList_DATA *data = INST_DATA(CLASS, obj);
    ULONG rc;

#if defined(DEBUG_ILC_FUNCS)
D(bug("[IconList]: %s()\n", __PRETTY_FUNCTION__));
#endif

    rc = DoSuperMethodA(CLASS, obj, (Msg)message);

    data->icld_ViewWidth = _mwidth(obj);
    data->icld_ViewHeight = _mheight(obj);

    return rc;
}
///

///MUIM_Draw()
/**************************************************************************
MUIM_Draw - draw the IconList
**************************************************************************/
IPTR DrawCount;
IPTR IconList__MUIM_Draw(struct IClass *CLASS, Object *obj, struct MUIP_Draw *message)
{   
    struct IconList_DATA    *data = INST_DATA(CLASS, obj);
    struct IconEntry       *icon = NULL;

    APTR                   clip;

    ULONG                  update_oldwidth = 0,
                           update_oldheight = 0;

    LONG                  clear_xoffset = 0,
                           clear_yoffset = 0;

    IPTR          draw_id = DrawCount++;

#if defined(DEBUG_ILC_FUNCS)
D(bug("[IconList]: %s(obj @ 0x%p)\n", __PRETTY_FUNCTION__, obj));
#endif

D(bug("[IconList] %s: id %d\n", __PRETTY_FUNCTION__, draw_id));
    
    DoSuperMethodA(CLASS, obj, (Msg)message);

    if (!(data->icld__Option_IconListFixedBackground))
    {
        clear_xoffset = data->icld_ViewX;
        clear_yoffset = data->icld_ViewY;
    }

    // If window size changes, only update needed areas
    if (data->update_oldwidth == 0) data->update_oldwidth = data->icld_ViewWidth;
    if (data->update_oldheight == 0) data->update_oldheight = data->icld_ViewHeight;
    if ((data->update_oldwidth != data->icld_ViewWidth) || (data->update_oldheight != data->icld_ViewHeight))
    {
        if (data->icld_UpdateMode != UPDATE_SCROLL)
        { 
            data->icld_UpdateMode = UPDATE_RESIZE;
            update_oldwidth = data->update_oldwidth;
            update_oldheight = data->update_oldheight;
            data->update_oldwidth = data->icld_ViewWidth;
            data->update_oldheight = data->icld_ViewHeight;
        }
    }

    if ((message->flags & MADF_DRAWUPDATE) || (data->icld_UpdateMode == UPDATE_RESIZE))
    {
#if defined(DEBUG_ILC_ICONRENDERING)
        D(
            if (message->flags & MADF_DRAWUPDATE)
            {
                bug("[IconList] %s#%d: MADF_DRAWUPDATE\n", __PRETTY_FUNCTION__, draw_id);
            }
            else
            {
                bug("[IconList] %s#%d: UPDATE_RESIZE\n", __PRETTY_FUNCTION__, draw_id);
            }
        )
#endif
        if ((data->icld_UpdateMode == UPDATE_SINGLEICON) && (data->update_icon != NULL)) /* draw only a single icon at update_icon */
        {
            struct Rectangle rect;

D(bug("[IconList] %s#%d: UPDATE_SINGLEICON (icon @ 0x%p)\n", __PRETTY_FUNCTION__, draw_id, data->update_icon));

            IconList_GetIconAreaRectangle(obj, data, data->update_icon, &rect);
    
            rect.MinX += _mleft(obj) + (data->update_icon->ie_IconX - data->icld_ViewX);
            rect.MaxX += _mleft(obj) + (data->update_icon->ie_IconX - data->icld_ViewX);
            rect.MinY += _mtop(obj) + (data->update_icon->ie_IconY - data->icld_ViewY);
            rect.MaxY += _mtop(obj) + (data->update_icon->ie_IconY - data->icld_ViewY);

            if (data->icld__Option_IconListMode == ICON_LISTMODE_GRID)
            {
                if (data->update_icon->ie_AreaWidth < data->icld_IconAreaLargestWidth)
                {
                    rect.MinX += ((data->icld_IconAreaLargestWidth - data->update_icon->ie_AreaWidth)/2);
                    rect.MaxX += ((data->icld_IconAreaLargestWidth - data->update_icon->ie_AreaWidth)/2);
                }

                if (data->update_icon->ie_AreaHeight < data->icld_IconAreaLargestHeight)
                {
                    rect.MinY += ((data->icld_IconAreaLargestHeight - data->update_icon->ie_AreaHeight)/2);
                    rect.MaxY += ((data->icld_IconAreaLargestHeight - data->update_icon->ie_AreaHeight)/2);
                }
            }

            clip = MUI_AddClipping(muiRenderInfo(obj), _mleft(obj), _mtop(obj), _mwidth(obj), _mheight(obj));

#if defined(DEBUG_ILC_ICONRENDERING)
D(bug("[IconList] %s#%d: UPDATE_SINGLEICON: Calling MUIM_DrawBackground (A)\n", __PRETTY_FUNCTION__, draw_id));
#endif
            DoMethod(obj, MUIM_DrawBackground, 
                rect.MinX, rect.MinY,
                rect.MaxX - rect.MinX + 1, rect.MaxY - rect.MinY + 1,
                clear_xoffset, clear_yoffset, 
                0);

            /* We could have deleted also other icons so they must be redrawn */
#ifdef __AROS__
            ForeachNode(&data->icld_IconList, icon)
#else
            Foreach_Node(&data->icld_IconList, icon);
#endif
            {
                if ((icon != data->update_icon) && (icon->ie_Flags & ICONENTRY_FLAG_VISIBLE))
                {
                    struct Rectangle rect2;
                    IconList_GetIconAreaRectangle(obj, data, icon, &rect2);

                    rect2.MinX += _mleft(obj) - data->icld_ViewX + icon->ie_IconX;
                    rect2.MaxX += _mleft(obj) - data->icld_ViewX + icon->ie_IconX;
                    rect2.MinY += _mtop(obj) - data->icld_ViewY + icon->ie_IconY;
                    rect2.MaxY += _mtop(obj) - data->icld_ViewY + icon->ie_IconY;

                    if (data->icld__Option_IconListMode == ICON_LISTMODE_GRID)
                    {
                        if (icon->ie_AreaWidth < data->icld_IconAreaLargestWidth)
                        {
                            rect2.MinX += ((data->icld_IconAreaLargestWidth - icon->ie_AreaWidth)/2);
                            rect2.MaxX += ((data->icld_IconAreaLargestWidth - icon->ie_AreaWidth)/2);
                        }

                        if (icon->ie_AreaHeight < data->icld_IconAreaLargestHeight)
                        {
                            rect2.MinY += ((data->icld_IconAreaLargestHeight - icon->ie_AreaHeight)/2);
                            rect2.MaxY += ((data->icld_IconAreaLargestHeight - icon->ie_AreaHeight)/2);
                        }
                    }

                    if (RectAndRect(&rect, &rect2))
                    {  
                        // Update icon here
                        icon->ie_Flags |= ICONENTRY_FLAG_NEEDSUPDATE;
                        DoMethod(obj, MUIM_IconList_DrawEntry, icon, ICONENTRY_DRAWMODE_PLAIN);
                        DoMethod(obj, MUIM_IconList_DrawEntryLabel, icon, ICONENTRY_DRAWMODE_PLAIN);
                        icon->ie_Flags &= ~ICONENTRY_FLAG_NEEDSUPDATE;
                    }
                }
            }

            icon->ie_Flags |= ICONENTRY_FLAG_NEEDSUPDATE;
            DoMethod(obj, MUIM_IconList_DrawEntry, data->update_icon, ICONENTRY_DRAWMODE_PLAIN);
            DoMethod(obj, MUIM_IconList_DrawEntryLabel, data->update_icon, ICONENTRY_DRAWMODE_PLAIN);
            icon->ie_Flags &= ~ICONENTRY_FLAG_NEEDSUPDATE;
            data->icld_UpdateMode = 0;
            data->update_icon = NULL;

            if (data->icld_DisplayRastPort != data->icld_BufferRastPort)
            {
#if defined(DEBUG_ILC_ICONRENDERING)
D(bug("[IconList] %s#%d: UPDATE_SINGLEICON Blitting to front rastport..\n", __PRETTY_FUNCTION__, draw_id));
#endif 
                BltBitMapRastPort(data->icld_BufferRastPort->BitMap,
                          rect.MinX - _mleft(obj), rect.MinY - _mtop(obj),
                          data->icld_DisplayRastPort,
                          rect.MinX, rect.MinY,
                          rect.MaxX - rect.MinX + 1, rect.MaxY - rect.MinY + 1,
                          0xC0);
            }

            MUI_RemoveClipping(muiRenderInfo(obj),clip);
            goto draw_done;
        }
        else if (data->icld_UpdateMode == UPDATE_SCROLL)
        {
            struct Region     *region = NULL;
            struct Rectangle  xrect,
                                yrect;
            BOOL            scroll_caused_damage;

#if defined(DEBUG_ILC_ICONRENDERING)
D(bug("[IconList] %s#%d: UPDATE_SCROLL.\n", __PRETTY_FUNCTION__, draw_id));
#endif 

            if (!data->icld__Option_IconListFixedBackground)
            {
                scroll_caused_damage = (_rp(obj)->Layer->Flags & LAYERREFRESH) ? FALSE : TRUE;
        
                data->icld_UpdateMode = 0;

                if ((abs(data->update_scrolldx) >= _mwidth(obj)) ||
                    (abs(data->update_scrolldy) >= _mheight(obj)))
                {
#if defined(DEBUG_ILC_ICONRENDERING)
D(bug("[IconList] %s#%d: UPDATE_SCROLL: Moved outside current view -> Causing Redraw .. MADF_DRAWOBJECT\n", __PRETTY_FUNCTION__, draw_id));
#endif 
                    MUI_Redraw(obj, MADF_DRAWOBJECT);
                    goto draw_done;
                }

                if (!(region = NewRegion()))
                {
#if defined(DEBUG_ILC_ICONRENDERING)
D(bug("[IconList] %s#%d: UPDATE_SCROLL: Couldnt Alloc Region -> Causing Redraw ...MADF_DRAWOBJECT\n", __PRETTY_FUNCTION__, draw_id));
#endif
                    MUI_Redraw(obj, MADF_DRAWOBJECT);
                    goto draw_done;
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

#if defined(DEBUG_ILC_ICONRENDERING)
D(bug("[IconList] %s#%d: UPDATE_SCROLL: Scrolling Raster..\n", __PRETTY_FUNCTION__, draw_id));
#endif
                if (data->icld_DisplayRastPort == data->icld_BufferRastPort)
                {
                    ScrollRasterBF(data->icld_BufferRastPort,
                                            data->update_scrolldx,
                                            data->update_scrolldy,
                                            _mleft(obj),
                                            _mtop(obj),
                                            _mright(obj),
                                            _mbottom(obj));
                }
                else
                {
                    ScrollRasterBF(data->icld_BufferRastPort,
                                            data->update_scrolldx,
                                            data->update_scrolldy,
                                            0,
                                            0,
                                            _mwidth(obj),
                                            _mheight(obj));
                }

                scroll_caused_damage = scroll_caused_damage && (_rp(obj)->Layer->Flags & LAYERREFRESH) ? TRUE : FALSE;

                clip = MUI_AddClipRegion(muiRenderInfo(obj), region);
            }

#if defined(DEBUG_ILC_ICONRENDERING)
D(bug("[IconList] %s#%d: UPDATE_SCROLL: Causing Redraw -> MADF_DRAWOBJECT..\n", __PRETTY_FUNCTION__, draw_id));
#endif
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
#if defined(DEBUG_ILC_ICONRENDERING)
D(bug("[IconList] %s#%d: UPDATE_SCROLL: Causing Redraw -> MADF_DRAWOBJECT..\n", __PRETTY_FUNCTION__, draw_id));
#endif
                        MUI_EndRefresh(muiRenderInfo(obj), 0);
                    }
                }
            }
            if (data->icld_DisplayRastPort != data->icld_BufferRastPort)
            {
#if defined(DEBUG_ILC_ICONRENDERING)
D(bug("[IconList] %s#%d: UPDATE_SCROLL: Blitting to front rastport..\n", __PRETTY_FUNCTION__, draw_id));
#endif 
                BltBitMapRastPort(data->icld_BufferRastPort->BitMap,
                          0, 0,
                          data->icld_DisplayRastPort,
                          _mleft(obj), _mtop(obj),
                          _mwidth(obj), _mheight(obj),
                          0xC0);
            }
            goto draw_done;
        }
        else if (data->icld_UpdateMode == UPDATE_RESIZE)
        {
            struct Region       *region = NULL;
            struct Rectangle    wrect,
                                hrect;
            ULONG               diffw = 0,
                                diffh = 0;

#if defined(DEBUG_ILC_ICONRENDERING)
D(bug("[IconList] %s#%d: UPDATE_RESIZE.\n", __PRETTY_FUNCTION__, draw_id));
#endif 

            if ((data->icld_BufferRastPort) && (data->icld_BufferRastPort != data->icld_DisplayRastPort))
            {
                //Free up the buffers rastport and bitmap so we can replace them ..
                struct Bitmap *bitmap_Old = (struct Bitmap *)data->icld_BufferRastPort->BitMap;
                struct Bitmap *bitmap_New;
    
                ULONG tmp_RastDepth;

                data->icld_BufferRastPort->BitMap = NULL;

                FreeRastPort(data->icld_BufferRastPort);
                SET(obj, MUIA_IconList_BufferRastport, NULL);

                tmp_RastDepth = GetCyberMapAttr(data->icld_DisplayRastPort->BitMap, CYBRMATTR_DEPTH);
                if ((bitmap_New = (struct Bitmap *)AllocBitMap(data->icld_ViewWidth,
                                    data->icld_ViewHeight,
                                    tmp_RastDepth,
                                    BMF_CLEAR,
                                    data->icld_DisplayRastPort->BitMap))!=NULL)
                {
                    if ((data->icld_BufferRastPort = CreateRastPort())!=NULL)
                    {
                        data->icld_BufferRastPort->BitMap = (struct BitMap *)bitmap_New;
                        SET(obj, MUIA_IconList_BufferRastport, data->icld_BufferRastPort);
                        data->icld_DrawOffsetX = 0;
                        data->icld_DrawOffsetY = 0;
                    }
                    else
                    {
                        FreeBitMap((struct BitMap *)bitmap_New);
                        data->icld_BufferRastPort = data->icld_DisplayRastPort;
                        data->icld_DrawOffsetX = _mleft(obj);
                        data->icld_DrawOffsetY = _mtop(obj);
                    }
                }
                
                if (bitmap_Old != (struct Bitmap *)data->icld_BufferRastPort->BitMap)
                    FreeBitMap((struct BitMap *)bitmap_Old);
            }

            data->icld_UpdateMode = 0;

            if (!data->icld__Option_IconListScaledBackground)
            {
                if (!(region = NewRegion()))
                {
#if defined(DEBUG_ILC_ICONRENDERING)
D(bug("[IconList] %s#%d: UPDATE_RESIZE: Causing Redraw -> MADF_DRAWOBJECT..\n", __PRETTY_FUNCTION__, draw_id));
#endif
                    MUI_Redraw(obj, MADF_DRAWOBJECT);
                    goto draw_done;
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
                    goto draw_done;
                }
            }

#if defined(DEBUG_ILC_ICONRENDERING)
D(bug("[IconList] %s#%d: UPDATE_RESIZE: Causing Redraw -> MADF_DRAWOBJECT..\n", __PRETTY_FUNCTION__, draw_id));
#endif
            MUI_Redraw(obj, MADF_DRAWOBJECT);

            if (!data->icld__Option_IconListScaledBackground)
            {
                if (diffh||diffw)
                {
                        data->update_rect1 = data->update_rect2 = NULL;
                        MUI_RemoveClipRegion(muiRenderInfo(obj), clip);
                } else DisposeRegion(region);
            }

            goto draw_done;
        }
    }

    if (message->flags & MADF_DRAWOBJECT)
    {
        struct Rectangle viewrect;

#if defined(DEBUG_ILC_ICONRENDERING)
D(bug("[IconList] %s#%d: MADF_DRAWOBJECT\n", __PRETTY_FUNCTION__, draw_id));
#endif

        clip = MUI_AddClipping(muiRenderInfo(obj), _mleft(obj), _mtop(obj), _mwidth(obj), _mheight(obj));

        viewrect.MinX = _mleft(obj);
        viewrect.MaxX = _mleft(obj) + _mwidth(obj);
        viewrect.MinY = _mtop(obj);
        viewrect.MaxY = _mtop(obj) + _mheight(obj);

#if defined(DEBUG_ILC_ICONRENDERING)
D(bug("[IconList] %s#%d: MADF_DRAWOBJECT: Calling MUIM_DrawBackground (B)\n", __PRETTY_FUNCTION__, draw_id));
#endif
        DoMethod(
            obj, MUIM_DrawBackground, _mleft(obj), _mtop(obj), _mwidth(obj), _mheight(obj),
            clear_xoffset, clear_yoffset, 0
        );
  #ifdef __AROS__
        ForeachNode(&data->icld_IconList, icon)
  #else
        Foreach_Node(&data->icld_IconList, icon);
  #endif
        {
            if ((icon->ie_Flags & ICONENTRY_FLAG_VISIBLE) &&
                (icon->ie_DiskObj) &&
                (icon->ie_IconX != NO_ICON_POSITION) &&
                (icon->ie_IconY != NO_ICON_POSITION))
            {
                struct Rectangle iconrect;
                IconList_GetIconAreaRectangle(obj, data, icon, &iconrect);

                iconrect.MinX += _mleft(obj) - data->icld_ViewX + icon->ie_IconX;
                iconrect.MaxX += _mleft(obj) - data->icld_ViewX + icon->ie_IconX;
                iconrect.MinY += _mtop(obj) - data->icld_ViewY + icon->ie_IconY;
                iconrect.MaxY += _mtop(obj) - data->icld_ViewY + icon->ie_IconY;

                if (RectAndRect(&viewrect, &iconrect))
                {
                    DoMethod(obj, MUIM_IconList_DrawEntry, icon, ICONENTRY_DRAWMODE_PLAIN);
                    DoMethod(obj, MUIM_IconList_DrawEntryLabel, icon, ICONENTRY_DRAWMODE_PLAIN);
                }
            }
        }

        if (data->icld_DisplayRastPort != data->icld_BufferRastPort)
        {
#if defined(DEBUG_ILC_ICONRENDERING)
D(bug("[IconList] %s#%d: MADF_DRAWOBJECT: Blitting to front rastport..\n", __PRETTY_FUNCTION__, draw_id));
#endif 
      BltBitMapRastPort(data->icld_BufferRastPort->BitMap,
            0, 0,
            data->icld_DisplayRastPort,
            _mleft(obj), _mtop(obj),
            _mwidth(obj), _mheight(obj),
            0xC0);
        }

        MUI_RemoveClipping(muiRenderInfo(obj), clip);
    }
    data->icld_UpdateMode = 0;

draw_done:;
    
    D(bug("[IconList] %s: Draw finished for id %d\n", __PRETTY_FUNCTION__, draw_id));

    return 0;
}
///

///IconList__MUIM_IconList_Update()
/**************************************************************************
MUIM_IconList_Refresh
Implemented by subclasses
**************************************************************************/
IPTR IconList__MUIM_IconList_Update(struct IClass *CLASS, Object *obj, struct MUIP_IconList_Update *message)
{
    struct IconList_DATA *data = INST_DATA(CLASS, obj);

#if defined(DEBUG_ILC_FUNCS)
D(bug("[IconList]: %s()\n", __PRETTY_FUNCTION__));
#endif

    data->icld_FocusIcon = NULL;
    SET(obj, MUIA_IconList_Changed, TRUE);

    return 1;
}
///

///MUIM_IconList_Clear()
/**************************************************************************
MUIM_IconList_Clear
**************************************************************************/
IPTR IconList__MUIM_IconList_Clear(struct IClass *CLASS, Object *obj, struct MUIP_IconList_Clear *message)
{
    struct IconList_DATA *data = INST_DATA(CLASS, obj);
    struct IconEntry    *node = NULL;

#if defined(DEBUG_ILC_FUNCS)
D(bug("[IconList]: %s()\n", __PRETTY_FUNCTION__));
#endif

    while ((node = (struct IconEntry*)RemTail((struct List*)&data->icld_IconList)))
    {
        DoMethod(obj, MUIM_IconList_DestroyEntry, node);
    }

    data->icld_SelectionLastClicked = NULL;
    data->icld_FocusIcon = NULL;
    
    data->icld_ViewX = data->icld_ViewY = data->icld_AreaWidth = data->icld_AreaHeight = 0;

D(bug("[IconList]: %s: call SetSuperAttrs()\n", __PRETTY_FUNCTION__));
    SetSuperAttrs(CLASS, obj, MUIA_Virtgroup_Left, data->icld_ViewX,
                  MUIA_Virtgroup_Top, data->icld_ViewY,
            TAG_DONE);
D(bug("[IconList]: %s: call SetAttrs()\n", __PRETTY_FUNCTION__));
    SetAttrs(obj, MUIA_Virtgroup_Left, data->icld_ViewX,
                  MUIA_Virtgroup_Top, data->icld_ViewY,
            TAG_DONE);

D(bug("[IconList]: %s: Set MUIA_IconList_Width and MUIA_IconList_Height\n", __PRETTY_FUNCTION__));
    SetAttrs(obj, MUIA_IconList_Width, data->icld_AreaWidth,
        MUIA_IconList_Height, data->icld_AreaHeight,
        TAG_DONE);

D(bug("[IconList]: %s: call MUI_Redraw()\n", __PRETTY_FUNCTION__));
    MUI_Redraw(obj,MADF_DRAWOBJECT);
    return 1;
}
///

///IconList__MUIM_IconList_DestroyEntry()
IPTR IconList__MUIM_IconList_DestroyEntry(struct IClass *CLASS, Object *obj, struct MUIP_IconList_DestroyEntry *message)
{
    struct IconList_DATA *data = INST_DATA(CLASS, obj);

#if defined(DEBUG_ILC_FUNCS)
D(bug("[IconList]: %s()\n", __PRETTY_FUNCTION__));
#endif

    if (message->icon)
    {
        if (message->icon->ie_Flags & ICONENTRY_FLAG_SELECTED)
        {
            if (data->icld_SelectionLastClicked == message->icon)
            {
                struct IconList_Entry *nextentry    = &message->icon->ie_IconListEntry;

                /* get selected entries from SOURCE iconlist */
                DoMethod(obj, MUIM_IconList_NextIcon, MUIV_IconList_NextIcon_Selected, (IPTR)&nextentry);
                if (nextentry)
                    data->icld_SelectionLastClicked = (struct IconEntry *)((IPTR)nextentry - ((IPTR)&message->icon->ie_IconListEntry - (IPTR)message->icon));
                else
                    data->icld_SelectionLastClicked = NULL;
            }
            if (data->icld_FocusIcon == message->icon)
                data->icld_FocusIcon = data->icld_SelectionLastClicked;

            Remove(&message->icon->ie_SelectionNode);
        }
        if (message->icon->ie_TxtBuf_DisplayedLabel) FreeVecPooled(data->icld_Pool, message->icon->ie_TxtBuf_DisplayedLabel);
        if (message->icon->ie_TxtBuf_PROT) FreePooled(data->icld_Pool, message->icon->ie_TxtBuf_PROT, 8);
        if (message->icon->ie_TxtBuf_SIZE) FreePooled(data->icld_Pool, message->icon->ie_TxtBuf_SIZE, 30);
        if (message->icon->ie_TxtBuf_TIME) FreePooled(data->icld_Pool, message->icon->ie_TxtBuf_TIME, LEN_DATSTRING);
        if (message->icon->ie_TxtBuf_DATE) FreePooled(data->icld_Pool, message->icon->ie_TxtBuf_DATE, LEN_DATSTRING);
        if (message->icon->ie_DiskObj) FreeDiskObject(message->icon->ie_DiskObj);
        if (message->icon->ie_FileInfoBlock) FreeMem(message->icon->ie_FileInfoBlock, sizeof(struct FileInfoBlock));
        if (message->icon->ie_IconListEntry.label) FreePooled(data->icld_Pool, message->icon->ie_IconListEntry.label, strlen(message->icon->ie_IconListEntry.label)+1);
        if (message->icon->ie_IconNode.ln_Name) FreePooled(data->icld_Pool, message->icon->ie_IconNode.ln_Name, strlen(message->icon->ie_IconNode.ln_Name)+1);
        FreePooled(data->icld_Pool, message->icon, sizeof(struct IconEntry));
    }
    return TRUE;
}
///

///IconList__MUIM_IconList_CreateEntry()
/**************************************************************************
MUIM_IconList_CreateEntry.
Returns 0 on failure otherwise it returns the icons entry ..
**************************************************************************/
IPTR IconList__MUIM_IconList_CreateEntry(struct IClass *CLASS, Object *obj, struct MUIP_IconList_CreateEntry *message)
{
    struct IconList_DATA  *data = INST_DATA(CLASS, obj);
    struct IconEntry     *entry = NULL;
    struct DateTime    dt;
    struct DateStamp     now;
    UBYTE            *sp = NULL;

    struct DiskObject    *dob = NULL;
    struct Rectangle     rect;

    IPTR                 geticon_error = 0;

#if defined(DEBUG_ILC_FUNCS)
D(bug("[IconList]: %s()\n", __PRETTY_FUNCTION__));
#endif

    /*disk object (icon)*/
    if (message->icon_dob == NULL)
    {
        IPTR iconlistScreen = _screen(obj);
D(bug("[IconList] %s: IconList Screen @ 0x%p)\n", __PRETTY_FUNCTION__, iconlistScreen));

        dob = GetIconTags
        (
            message->filename,
            (iconlistScreen) ? ICONGETA_Screen : TAG_IGNORE, iconlistScreen,
            (iconlistScreen) ? ICONGETA_RemapIcon : TAG_IGNORE, TRUE,
            ICONGETA_FailIfUnavailable,        FALSE,
            ICONGETA_GenerateImageMasks,       TRUE,
            ICONA_ErrorCode,                   &geticon_error,
            TAG_DONE
        );
        
        if (dob == NULL)
        {
D(bug("[IconList] %s: Fatal: Couldnt get DiskObject! (error code = 0x%p)\n", __PRETTY_FUNCTION__, geticon_error));

            return (IPTR)NULL;
        }
    }
    else
    {
        dob = message->icon_dob;
    }

D(bug("[IconList] %s: DiskObject @ 0x%p\n", __PRETTY_FUNCTION__, dob));

    if ((entry = AllocPooled(data->icld_Pool, sizeof(struct IconEntry))) == NULL)
    {
D(bug("[IconList] %s: Failed to Allocate Entry Storage!\n", __PRETTY_FUNCTION__));
        FreeDiskObject(dob);
        return (IPTR)NULL;
    }
    memset(entry, 0, sizeof(struct IconEntry));
    entry->ie_Flags |= ICONENTRY_FLAG_NEEDSUPDATE;
    entry->ie_IconListEntry.ile_IconEntry = entry;

    /* Allocate Text Buffers */
    
    if ((entry->ie_TxtBuf_DATE = AllocPooled(data->icld_Pool, LEN_DATSTRING)) == NULL)
    {
D(bug("[IconList] %s: Failed to Allocate Entry DATE Storage!\n", __PRETTY_FUNCTION__));
        DoMethod(obj, MUIM_IconList_DestroyEntry, entry);
        return (IPTR)NULL;
    }
    memset(entry->ie_TxtBuf_DATE, 0, LEN_DATSTRING);

    if ((entry->ie_TxtBuf_TIME = AllocPooled(data->icld_Pool, LEN_DATSTRING)) == NULL)
    {
D(bug("[IconList] %s: Failed to Allocate Entry TIME string Storage!\n", __PRETTY_FUNCTION__));
        DoMethod(obj, MUIM_IconList_DestroyEntry, entry);
        return (IPTR)NULL;
    }
    memset(entry->ie_TxtBuf_TIME, 0, LEN_DATSTRING);

    if ((entry->ie_TxtBuf_SIZE = AllocPooled(data->icld_Pool, 30)) == NULL)
    {
D(bug("[IconList] %s: Failed to Allocate Entry SIZE string Storage!\n", __PRETTY_FUNCTION__));
        DoMethod(obj, MUIM_IconList_DestroyEntry, entry);
        return (IPTR)NULL;
    }
    memset(entry->ie_TxtBuf_SIZE, 0, 30);

    if ((entry->ie_TxtBuf_PROT = AllocPooled(data->icld_Pool, 8)) == NULL)
    {
D(bug("[IconList] %s: Failed to Allocate Entry PROT Flag string Storage!\n", __PRETTY_FUNCTION__));
        DoMethod(obj, MUIM_IconList_DestroyEntry, entry);
        return (IPTR)NULL;
    }
    memset(entry->ie_TxtBuf_PROT, 0, 8);
    
    /*alloc filename*/
    if ((entry->ie_IconNode.ln_Name = AllocPooled(data->icld_Pool, strlen(message->filename) + 1)) == NULL)
    {
D(bug("[IconList] %s: Failed to Allocate Entry filename string Storage!\n", __PRETTY_FUNCTION__));
        DoMethod(obj, MUIM_IconList_DestroyEntry, entry);
        return (IPTR)NULL;
    }

    /*alloc icon label*/
    if ((entry->ie_IconListEntry.label = AllocPooled(data->icld_Pool, strlen(message->label) + 1)) == NULL)
    {
D(bug("[IconList] %s: Failed to Allocate Entry label string Storage!\n", __PRETTY_FUNCTION__));
        DoMethod(obj, MUIM_IconList_DestroyEntry, entry);
        return (IPTR)NULL;
    }

    /*file info block*/
    if(message->fib != NULL)
    {
        if ((entry->ie_FileInfoBlock = AllocMem(sizeof(struct FileInfoBlock), MEMF_CLEAR)) != NULL)
        {
            CopyMem(message->fib, entry->ie_FileInfoBlock, sizeof(struct FileInfoBlock));

            if (entry->ie_FileInfoBlock->fib_DirEntryType > 0)
            {
                strcpy(entry->ie_TxtBuf_SIZE, "Drawer");
            }
            else
            {
                FmtSizeToString(entry->ie_TxtBuf_SIZE, entry->ie_FileInfoBlock->fib_Size);
            }

            dt.dat_Stamp    = entry->ie_FileInfoBlock->fib_Date;
            dt.dat_Format   = FORMAT_DEF;
            dt.dat_Flags    = 0;
            dt.dat_StrDay   = NULL;
            dt.dat_StrDate  = entry->ie_TxtBuf_DATE;
            dt.dat_StrTime  = entry->ie_TxtBuf_TIME;

            DateToStr(&dt);
            DateStamp(&now);

            /*if modified today show time, otherwise just show date*/
            if (now.ds_Days == entry->ie_FileInfoBlock->fib_Date.ds_Days)
                entry->ie_Flags |= ICONENTRY_FLAG_TODAY;
            else
                entry->ie_Flags &= ~ICONENTRY_FLAG_TODAY;

            sp = entry->ie_TxtBuf_PROT;
            *sp++ = (entry->ie_FileInfoBlock->fib_Protection & FIBF_SCRIPT)  ? 's' : '-';
            *sp++ = (entry->ie_FileInfoBlock->fib_Protection & FIBF_PURE)    ? 'p' : '-';
            *sp++ = (entry->ie_FileInfoBlock->fib_Protection & FIBF_ARCHIVE) ? 'a' : '-';
            *sp++ = (entry->ie_FileInfoBlock->fib_Protection & FIBF_READ)    ? '-' : 'r';
            *sp++ = (entry->ie_FileInfoBlock->fib_Protection & FIBF_WRITE)   ? '-' : 'w';
            *sp++ = (entry->ie_FileInfoBlock->fib_Protection & FIBF_EXECUTE) ? '-' : 'e';
            *sp++ = (entry->ie_FileInfoBlock->fib_Protection & FIBF_DELETE)  ? '-' : 'd';
            *sp++ = '\0';
        
            entry->ie_IconListEntry.type = entry->ie_FileInfoBlock->fib_DirEntryType;
        }
    }
    else
    {
        entry->ie_IconListEntry.type = ST_USERDIR;
    }

    /* Override type if specified during createntry */
    if (message->type != 0)
    {
        entry->ie_IconListEntry.type = message->type;
D(bug("[IconList] %s: Overide Entry Type. New Type = %x\n", __PRETTY_FUNCTION__, entry->ie_IconListEntry.type));
    }
    else
    {
D(bug("[IconList] %s: Entry Type = %x\n", __PRETTY_FUNCTION__, entry->ie_IconListEntry.type));
    }

    strcpy(entry->ie_IconNode.ln_Name, message->filename);
    strcpy(entry->ie_IconListEntry.label, message->label);

    if (IconList__LabelFunc_CreateLabel(obj, data, entry) != (IPTR)NULL)
    {
        entry->ie_DiskObj = dob;
        entry->ie_IconListEntry.udata = NULL;

        /* Use a geticonrectangle routine that gets textwidth! */
        IconList_GetIconAreaRectangle(obj, data, entry, &rect);

        AddTail((struct List*)&data->icld_IconList, (struct Node*)&entry->ie_IconNode);

        return (IPTR)entry;
    }

    DoMethod(obj, MUIM_IconList_DestroyEntry, entry);
    return (IPTR)NULL;
}
///

///DoWheelMove()
static void DoWheelMove(struct IClass *CLASS, Object *obj, LONG wheelx, LONG wheely, UWORD qual)
{
    struct IconList_DATA *data = INST_DATA(CLASS, obj);

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
        SetAttrs(obj, MUIA_Virtgroup_Left, newleft,
            MUIA_Virtgroup_Top, newtop,
            TAG_DONE);
    }
}
///

///MUIM_HandleEvent()
/**************************************************************************
MUIM_HandleEvent
**************************************************************************/
IPTR IconList__MUIM_HandleEvent(struct IClass *CLASS, Object *obj, struct MUIP_HandleEvent *message)
{
    struct IconList_DATA *data = INST_DATA(CLASS, obj);

#if defined(DEBUG_ILC_FUNCS)
D(bug("[IconList]: %s()\n", __PRETTY_FUNCTION__));
#endif

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
D(bug("[IconList] %s: Processing mouse wheel event\n", __PRETTY_FUNCTION__));
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
D(bug("[IconList] %s: Processing key up event\n", __PRETTY_FUNCTION__));
#endif

                    switch(message->imsg->Code)
                    {
                        case RAWKEY_RETURN:
                            rawkey_handled = TRUE;

#if defined(DEBUG_ILC_KEYEVENTS)
D(bug("[IconList] %s: RAWKEY_RETURN\n", __PRETTY_FUNCTION__));
#endif

                            if (data->icld_FocusIcon) active_entry = data->icld_FocusIcon;
                            else if (data->icld_SelectionLastClicked) active_entry = data->icld_SelectionLastClicked;

                            if (active_entry)
                            {
                                if (!(active_entry->ie_Flags & ICONENTRY_FLAG_SELECTED))
                                {
                                    active_entry->ie_Flags |= ICONENTRY_FLAG_SELECTED;
                                    AddTail(&data->icld_SelectionList, &active_entry->ie_SelectionNode);
                                    data->icld_UpdateMode = UPDATE_SINGLEICON;
                                    data->update_icon = active_entry;
                                    MUI_Redraw(obj, MADF_DRAWUPDATE);
                                }
                                data->icld_SelectionLastClicked = active_entry;
                                data->icld_FocusIcon = active_entry;

                                SET(obj, MUIA_IconList_DoubleClick, TRUE);
                            }
                            break;

                        case RAWKEY_SPACE:
                            rawkey_handled = TRUE;

#if defined(DEBUG_ILC_KEYEVENTS)
D(bug("[IconList] %s: RAWKEY_SPACE\n", __PRETTY_FUNCTION__));
#endif

                            if (data->icld_FocusIcon) active_entry = data->icld_FocusIcon;
                            else if (data->icld_SelectionLastClicked) active_entry = data->icld_SelectionLastClicked;

                            if (!(message->imsg->Qualifier & IEQUALIFIER_LSHIFT) && ((data->icld_SelectionLastClicked)||(data->icld_SelectionLastClicked != active_entry)))
                            {
#if defined(DEBUG_ILC_KEYEVENTS)
D(bug("[IconList] %s: SPACE: Clearing selected icons ..\n", __PRETTY_FUNCTION__));
#endif
                                DoMethod(obj, MUIM_IconList_UnselectAll);
                            }

                            if (active_entry)
                            {
                                if (!(active_entry->ie_Flags & ICONENTRY_FLAG_SELECTED))
                                {
                                    AddTail(&data->icld_SelectionList, &active_entry->ie_SelectionNode);
                                    active_entry->ie_Flags |= ICONENTRY_FLAG_SELECTED;
                                    data->icld_SelectionLastClicked = active_entry;
                                }
                                else
                                {
                                    Remove(&active_entry->ie_SelectionNode);
                                    active_entry->ie_Flags &= ~ICONENTRY_FLAG_SELECTED;
                                }

                                data->icld_FocusIcon = active_entry;

                                data->icld_UpdateMode = UPDATE_SINGLEICON;
                                data->update_icon = active_entry;
                                MUI_Redraw(obj, MADF_DRAWUPDATE);
                            }
                            break;

                        case RAWKEY_PAGEUP:
                            rawkey_handled = TRUE;

#if defined(DEBUG_ILC_KEYEVENTS)
D(bug("[IconList] %s: RAWKEY_PAGEUP\n", __PRETTY_FUNCTION__));
#endif

                            if (data->icld_AreaHeight > data->icld_ViewHeight)
                            {
                                new_ViewY -= data->icld_ViewHeight;
                                if (new_ViewY< 0)
                                    new_ViewY = 0;
                            }

                            if (new_ViewY != data->icld_ViewY)
                            {
                                SET(obj, MUIA_Virtgroup_Top, new_ViewY);
                            }
                            break;

                        case RAWKEY_PAGEDOWN:
                            rawkey_handled = TRUE;

#if defined(DEBUG_ILC_KEYEVENTS)
D(bug("[IconList] %s: RAWKEY_PAGEDOWN\n", __PRETTY_FUNCTION__));
#endif

                            if (data->icld_AreaHeight > data->icld_ViewHeight)
                            {
                                new_ViewY += data->icld_ViewHeight;
                                if (new_ViewY > (data->icld_AreaHeight - data->icld_ViewHeight))
                                    new_ViewY = data->icld_AreaHeight - data->icld_ViewHeight;
                            }

                            if (new_ViewY != data->icld_ViewY)
                            {
                                SET(obj, MUIA_Virtgroup_Top, new_ViewY);
                            }
                            break;

                        case RAWKEY_UP:
                            rawkey_handled = TRUE;

#if defined(DEBUG_ILC_KEYEVENTS)
D(bug("[IconList] %s: RAWKEY_UP\n", __PRETTY_FUNCTION__));
#endif

                            if (data->icld_FocusIcon)
                            {
                                start_entry = data->icld_FocusIcon;
#if defined(DEBUG_ILC_KEYEVENTS)
D(bug("[IconList] %s: UP: Clearing existing focused icon @ 0x%p\n", __PRETTY_FUNCTION__, start_entry));
#endif

                                start_entry->ie_Flags &= ~ICONENTRY_FLAG_FOCUS;
                                data->icld_UpdateMode = UPDATE_SINGLEICON;
                                data->update_icon = start_entry;
                                MUI_Redraw(obj, MADF_DRAWUPDATE);

                                start_X = start_entry->ie_IconX;
                                start_Y = start_entry->ie_IconY;
#if defined(DEBUG_ILC_KEYEVENTS)
D(bug("[IconList] %s: UP: start_icon @ 0x%p '%s' - start_X %d, start_Y %d\n", __PRETTY_FUNCTION__, start_entry, start_entry->ie_IconListEntry.label, start_X, start_Y));
#endif
                                if (data->icld__Option_IconListMode == ICON_LISTMODE_GRID)
                                {
                                    if (start_entry->ie_AreaWidth < data->icld_IconAreaLargestWidth)
                                    {
                                        start_X = start_X - ((data->icld_IconAreaLargestWidth - start_entry->ie_AreaWidth)/2);
#if defined(DEBUG_ILC_KEYEVENTS)
D(bug("[IconList] %s: UP: adjusted start_X for grid = %d\n", __PRETTY_FUNCTION__, start_X));
#endif
                                    }
                                }

                                if ((active_entry = Node_PreviousVisible(start_entry)) && !(data->icld_DisplayFlags & ICONLIST_DISP_VERTICAL))
                                {
                                    //Check if we are at the edge of the icon area ..
#if defined(DEBUG_ILC_KEYEVENTS)
D(bug("[IconList] %s: UP: active_entry @ 0x%p '%s' , X %d, Y %d\n", __PRETTY_FUNCTION__, active_entry, active_entry->ie_IconListEntry.label, active_entry->ie_IconX, active_entry->ie_IconY));
#endif
                                    active_Y = active_entry->ie_IconY;

                                    if (active_Y == start_Y)
                                    {

#if defined(DEBUG_ILC_KEYEVENTS)
D(bug("[IconList] %s: UP: active_entry is on our row (not at the edge)\n", __PRETTY_FUNCTION__));
#endif
                                        entry_next = active_entry;
                                        next_X = entry_next->ie_IconX;
                                        if (data->icld__Option_IconListMode == ICON_LISTMODE_GRID)
                                        {
                                            if (entry_next->ie_AreaWidth < data->icld_IconAreaLargestWidth)
                                                next_X = next_X - ((data->icld_IconAreaLargestWidth - entry_next->ie_AreaWidth)/2);
                                        }
                                    }
                                }
#if defined(DEBUG_ILC_KEYEVENTS)
D(bug("[IconList] %s: UP: using start_X %d, start_Y %d\n", __PRETTY_FUNCTION__, start_X, start_Y));
#endif
                            }

                            if (!(message->imsg->Qualifier & IEQUALIFIER_LSHIFT) && ((data->icld_SelectionLastClicked)&&(data->icld_SelectionLastClicked != active_entry)))
                            {
#if defined(DEBUG_ILC_KEYEVENTS)
D(bug("[IconList] %s: UP: Clearing selected icons ..\n", __PRETTY_FUNCTION__));
#endif
                                DoMethod(obj, MUIM_IconList_UnselectAll);
                            }

#if defined(DEBUG_ILC_KEYEVENTS)
D(bug("[IconList] %s: UP: active = 0x%p, next = 0x%p\n", __PRETTY_FUNCTION__, active_entry, entry_next));
#endif
                            if (!(active_entry))
                            {
                                // If nothing is selected we will use the last visible icon ..
                                active_entry = Node_LastVisible(&data->icld_IconList);
                                start_X = active_entry->ie_IconX;
                                if (data->icld__Option_IconListMode == ICON_LISTMODE_GRID)
                                {
                                    if (active_entry->ie_AreaWidth < data->icld_IconAreaLargestWidth)
                                        start_X = start_X - ((data->icld_IconAreaLargestWidth - active_entry->ie_AreaWidth)/2);
                                }
                                start_Y = active_entry->ie_IconY;
                            }

                            while (active_entry != NULL)
                            {
#if defined(DEBUG_ILC_KEYEVENTS)
D(bug("[IconList] %s: UP: Checking active @ 0x%p\n", __PRETTY_FUNCTION__, active_entry));
#endif
                                if (data->icld_DisplayFlags & ICONLIST_DISP_VERTICAL)
                                {
                                    // Return the first visible since the list flow direction matches
                                    // our cursor direction
                                    if (active_entry->ie_Flags & ICONENTRY_FLAG_VISIBLE)
                                        break;
                                }
                                else
                                {
                                    active_X = active_entry->ie_IconX;

                                    if (data->icld__Option_IconListMode == ICON_LISTMODE_GRID)
                                    {
                                        if (active_entry->ie_AreaWidth < data->icld_IconAreaLargestWidth)
                                            x_diff = ((data->icld_IconAreaLargestWidth - active_entry->ie_AreaWidth)/2);
                                    }
                                    active_Y = active_entry->ie_IconY;
                                    
                                    if (start_entry)
                                    {
                                        if (entry_next)
                                        {
                                            if ((active_entry->ie_Flags & ICONENTRY_FLAG_VISIBLE) &&
                                                (active_Y < start_Y) &&
                                                (((active_X - x_diff) >= start_X ) &&
                                                ((active_X - x_diff) <= (start_X + start_entry->ie_AreaWidth + (x_diff*2)))))
                                            {
#if defined(DEBUG_ILC_KEYEVENTS)
D(bug("[IconList] %s: UP: (A) entry 0x%p matches\n", __PRETTY_FUNCTION__, active_entry));
#endif
                                                break;
                                            }
                                            else if (active_entry == (struct IconEntry *)GetHead(&data->icld_IconList))
                                            {
#if defined(DEBUG_ILC_KEYEVENTS)
D(bug("[IconList] %s: UP: (A) reached list start .. restarting from the end ..\n", __PRETTY_FUNCTION__));
#endif
                                                start_X = next_X;

                                                if ((entry_next = Node_PreviousVisible(entry_next)))
                                                {
                                                    if (entry_next->ie_IconX > start_X)
                                                        entry_next = NULL;
                                                    else
                                                    {
                                                        next_X = entry_next->ie_IconX;
                                                        if (data->icld__Option_IconListMode == ICON_LISTMODE_GRID)
                                                        {
                                                            if (entry_next->ie_AreaWidth < data->icld_IconAreaLargestWidth)
                                                                next_X = next_X - ((data->icld_IconAreaLargestWidth - entry_next->ie_AreaWidth)/2);
                                                        }
                                                    }
                                                }
                                                start_Y = 0;
#if defined(DEBUG_ILC_KEYEVENTS)
D(bug("[IconList] %s: UP: (A) startx = %d, start_Y = %d, next_X = %d, entry_next @ 0x%p\n", __PRETTY_FUNCTION__, start_X, start_Y, next_X, entry_next));
#endif
                                                active_entry = Node_LastVisible(&data->icld_IconList);
                                            }
                                        }
                                        else
                                        {
                                            if ((active_entry->ie_Flags & ICONENTRY_FLAG_VISIBLE) &&
                                                (active_Y < start_Y) &&
                                                ((active_X + x_diff) < (start_X + start_entry->ie_AreaWidth + x_diff)))
                                            {
#if defined(DEBUG_ILC_KEYEVENTS)
D(bug("[IconList] %s: UP: (B) entry 0x%p matches\n", __PRETTY_FUNCTION__, active_entry));
#endif
                                                break;
                                            }
                                        }
                                    }
                                    else
                                    {
                                        if (active_entry->ie_Flags & ICONENTRY_FLAG_VISIBLE)
                                        {
#if defined(DEBUG_ILC_KEYEVENTS)
D(bug("[IconList] %s: UP: (C) entry 0x%p matches\n", __PRETTY_FUNCTION__, active_entry));
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
D(bug("[IconList] %s: UP: No Next UP Node - Getting Last visible icon ..\n", __PRETTY_FUNCTION__));
#endif
                                /* We didnt find a "next UP" icon so just use the last visible */
                                active_entry = Node_LastVisible(&data->icld_IconList);
                            }
                            
                            if (active_entry)
                            {
                                if (!(active_entry->ie_Flags & ICONENTRY_FLAG_FOCUS))
                                {
                                    active_entry->ie_Flags |= ICONENTRY_FLAG_FOCUS;
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
D(bug("[IconList] %s: RAWKEY_DOWN\n", __PRETTY_FUNCTION__));
#endif

                            if (data->icld_FocusIcon)
                            {
                                start_entry = data->icld_FocusIcon;
#if defined(DEBUG_ILC_KEYEVENTS)
D(bug("[IconList] %s: DOWN: Clearing existing focused icon @ 0x%p\n", __PRETTY_FUNCTION__, start_entry));
#endif

                                start_entry->ie_Flags &= ~ICONENTRY_FLAG_FOCUS;
                                data->icld_UpdateMode = UPDATE_SINGLEICON;
                                data->update_icon = start_entry;
                                MUI_Redraw(obj, MADF_DRAWUPDATE);

                                start_X = start_entry->ie_IconX;
                                start_Y = start_entry->ie_IconY;
#if defined(DEBUG_ILC_KEYEVENTS)
D(bug("[IconList] %s: DOWN: start_icon @ 0x%p '%s' - start_X %d, start_Y %d\n", __PRETTY_FUNCTION__, start_entry, start_entry->ie_IconListEntry.label, start_X, start_Y));
#endif
                                if (data->icld__Option_IconListMode == ICON_LISTMODE_GRID)
                                {
                                    if (start_entry->ie_AreaWidth < data->icld_IconAreaLargestWidth)
                                    {
                                        start_X = start_X - ((data->icld_IconAreaLargestWidth - start_entry->ie_AreaWidth)/2);
#if defined(DEBUG_ILC_KEYEVENTS)
D(bug("[IconList] %s: DOWN: adjusted start_X for grid = %d\n", __PRETTY_FUNCTION__, start_X));
#endif
                                    }
                                }

                                if ((active_entry = Node_NextVisible(start_entry)) &&
                                    (!(data->icld_DisplayFlags & ICONLIST_DISP_VERTICAL)))
                                {
#if defined(DEBUG_ILC_KEYEVENTS)
D(bug("[IconList] %s: DOWN: active_entry @ 0x%p '%s' , X %d, Y %d\n", __PRETTY_FUNCTION__, active_entry, active_entry->ie_IconListEntry.label, active_entry->ie_IconX, active_entry->ie_IconY));
#endif
                                    active_Y = active_entry->ie_IconY;

                                    if (active_Y == start_Y)
                                    {

#if defined(DEBUG_ILC_KEYEVENTS)
D(bug("[IconList] %s: DOWN: active_entry is on our row (not at the edge)\n", __PRETTY_FUNCTION__));
#endif
                                        entry_next = active_entry;
                                        next_X = entry_next->ie_IconX;
                                        if (data->icld__Option_IconListMode == ICON_LISTMODE_GRID)
                                        {
                                            if (entry_next->ie_AreaWidth < data->icld_IconAreaLargestWidth)
                                                next_X = next_X - ((data->icld_IconAreaLargestWidth - entry_next->ie_AreaWidth)/2);
                                        }
                                    }
                                }
#if defined(DEBUG_ILC_KEYEVENTS)
D(bug("[IconList] %s: DOWN: using start_X %d, start_Y %d\n", __PRETTY_FUNCTION__, start_X, start_Y));
#endif
                            }

                            if (!(message->imsg->Qualifier & IEQUALIFIER_LSHIFT) && ((data->icld_SelectionLastClicked)&&(data->icld_SelectionLastClicked != active_entry)))
                            {
#if defined(DEBUG_ILC_KEYEVENTS)
D(bug("[IconList] %s: DOWN: Clearing selected icons ..\n", __PRETTY_FUNCTION__));
#endif
                                DoMethod(obj, MUIM_IconList_UnselectAll);
                            }

#if defined(DEBUG_ILC_KEYEVENTS)
D(bug("[IconList] %s: DOWN: active = 0x%p, next = 0x%p\n", __PRETTY_FUNCTION__, active_entry, entry_next));
#endif
                            if (!(active_entry))
                            {
                                // If nothing is selected we will use the First visible icon ..
                                active_entry = Node_FirstVisible(&data->icld_IconList);
                                start_X = active_entry->ie_IconX;
                                if (data->icld__Option_IconListMode == ICON_LISTMODE_GRID)
                                {
                                    if (active_entry->ie_AreaWidth < data->icld_IconAreaLargestWidth)
                                        start_X = start_X - ((data->icld_IconAreaLargestWidth - active_entry->ie_AreaWidth)/2);
                                }
                                start_Y = active_entry->ie_IconY;
                            }

                            while (active_entry != NULL)
                            {
#if defined(DEBUG_ILC_KEYEVENTS)
D(bug("[IconList] %s: DOWN: Checking active @ 0x%p\n", __PRETTY_FUNCTION__, active_entry));
#endif
                                if (data->icld_DisplayFlags & ICONLIST_DISP_VERTICAL)
                                {
                                    if (active_entry->ie_Flags & ICONENTRY_FLAG_VISIBLE)
                                        break;
                                }
                                else
                                {
                                    active_X = active_entry->ie_IconX;

                                    if (data->icld__Option_IconListMode == ICON_LISTMODE_GRID)
                                    {
                                        if (active_entry->ie_AreaWidth < data->icld_IconAreaLargestWidth)
                                            x_diff = ((data->icld_IconAreaLargestWidth - active_entry->ie_AreaWidth)/2);
                                    }
                                    active_Y = active_entry->ie_IconY;

                                    if (start_entry)
                                    {
                                        if (entry_next)
                                        {
                                            if ((active_entry->ie_Flags & ICONENTRY_FLAG_VISIBLE) &&
                                                (active_Y > start_Y) &&
                                                (((active_X - x_diff) >= start_X ) &&
                                                ((active_X - x_diff) <= (start_X + start_entry->ie_AreaWidth + (x_diff*2)))))
                                            {
#if defined(DEBUG_ILC_KEYEVENTS)
D(bug("[IconList] %s: DOWN: (A) entry 0x%p matches\n", __PRETTY_FUNCTION__, active_entry));
#endif
                                                break;
                                            }
                                            else if (active_entry == (struct IconEntry *)GetTail(&data->icld_IconList))
                                            {
#if defined(DEBUG_ILC_KEYEVENTS)
D(bug("[IconList] %s: DOWN: (A) reached list end .. starting at the beginng ..\n", __PRETTY_FUNCTION__));
#endif
                                                start_X = entry_next->ie_IconX;
                                                if (data->icld__Option_IconListMode == ICON_LISTMODE_GRID)
                                                {
                                                    if (entry_next->ie_AreaWidth < data->icld_IconAreaLargestWidth)
                                                        start_X = start_X - ((data->icld_IconAreaLargestWidth - entry_next->ie_AreaWidth)/2);
                                                }

                                                if ((entry_next = (struct IconEntry *)Node_NextVisible(entry_next)))
                                                {
                                                    if (entry_next->ie_IconX < start_X)
                                                        entry_next = NULL;
                                                    else
                                                    {
                                                        next_X = entry_next->ie_IconX;
                                                        if (data->icld__Option_IconListMode == ICON_LISTMODE_GRID)
                                                        {
                                                            if (entry_next->ie_AreaWidth < data->icld_IconAreaLargestWidth)
                                                                next_X = next_X + ((data->icld_IconAreaLargestWidth - entry_next->ie_AreaWidth)/2);
                                                        }
                                                    }
                                                }
                                                start_Y = 0;
#if defined(DEBUG_ILC_KEYEVENTS)
D(bug("[IconList] %s: DOWN: (A) startx = %d, start_Y = %d, next_X = %d, entry_next @ 0x%p\n", __PRETTY_FUNCTION__, start_X, start_Y, next_X, entry_next));
#endif
                                                active_entry = Node_FirstVisible(&data->icld_IconList);
                                            }
                                        }
                                        else
                                        {
                                            if ((active_entry->ie_Flags & ICONENTRY_FLAG_VISIBLE) &&
                                                (active_Y > start_Y) &&
                                                (active_X > start_X - 1))
                                            {
#if defined(DEBUG_ILC_KEYEVENTS)
D(bug("[IconList] %s: DOWN: (B) entry 0x%p matches\n", __PRETTY_FUNCTION__, active_entry));
#endif
                                                break;
                                            }
                                        }
                                    }
                                    else
                                    {
                                        if (active_entry->ie_Flags & ICONENTRY_FLAG_VISIBLE)
                                        {
#if defined(DEBUG_ILC_KEYEVENTS)
D(bug("[IconList] %s: DOWN: (C) entry 0x%p matches\n", __PRETTY_FUNCTION__, active_entry));
#endif
                                            break;
                                        }
                                    }
                                }
                                active_entry = (struct IconEntry *)GetSucc(&active_entry->ie_IconNode);
                            }

                            if (!(active_entry))
                            {
#if defined(DEBUG_ILC_KEYEVENTS)
D(bug("[IconList] %s: DOWN: No Next DOWN Node - Getting first visable icon ..\n", __PRETTY_FUNCTION__));
#endif
                                /* We didnt find a "next DOWN" icon so just use the first visible */
                                active_entry =  Node_FirstVisible(&data->icld_IconList);
                            }
                            
                            if (active_entry)
                            {
                                if (!(active_entry->ie_Flags & ICONENTRY_FLAG_FOCUS))
                                {
                                    active_entry->ie_Flags |= ICONENTRY_FLAG_FOCUS;
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
D(bug("[IconList] %s: RAWKEY_LEFT\n", __PRETTY_FUNCTION__));
#endif

                            if (data->icld_FocusIcon)
                            {
                                start_entry = data->icld_FocusIcon;
#if defined(DEBUG_ILC_KEYEVENTS)
D(bug("[IconList] %s: LEFT: Clearing existing focused icon @ 0x%p\n", __PRETTY_FUNCTION__, start_entry));
#endif

                                start_entry->ie_Flags &= ~ICONENTRY_FLAG_FOCUS;
                                data->icld_UpdateMode = UPDATE_SINGLEICON;
                                data->update_icon = start_entry;
                                MUI_Redraw(obj, MADF_DRAWUPDATE);

                                start_X = start_entry->ie_IconX;
                                if (data->icld__Option_IconListMode == ICON_LISTMODE_GRID)
                                {
                                    if (start_entry->ie_AreaWidth < data->icld_IconAreaLargestWidth)
                                        start_X = start_X - ((data->icld_IconAreaLargestWidth - start_entry->ie_AreaWidth)/2);
                                }
                                start_Y = start_entry->ie_IconY;

#if defined(DEBUG_ILC_KEYEVENTS)
D(bug("[IconList] %s: LEFT: start_X %d, start_Y %d\n", __PRETTY_FUNCTION__, start_X, start_Y));
#endif

                                if (!(active_entry = Node_NextVisible(start_entry)) && (!(data->icld_DisplayFlags & ICONLIST_DISP_VERTICAL)))
                                {
                                    active_entry = (struct IconEntry *)GetHead(&data->icld_IconList);
#if defined(DEBUG_ILC_KEYEVENTS)
D(bug("[IconList] %s: LEFT: Start at the beginning (Active @ 0x%p) using icon X + Width\n", __PRETTY_FUNCTION__, active_entry));
#endif
                                    start_X = start_X + start_entry->ie_AreaWidth;
                                    if (data->icld__Option_IconListMode == ICON_LISTMODE_GRID)
                                    {
                                        if (start_entry->ie_AreaWidth < data->icld_IconAreaLargestWidth)
                                            start_X = start_X + ((data->icld_IconAreaLargestWidth - start_entry->ie_AreaWidth)/2);
                                    }

                                    start_Y = 0;
                                    entry_next = NULL;
                                }
                                else if (active_entry && (!(data->icld_DisplayFlags & ICONLIST_DISP_VERTICAL)))
                                {
#if defined(DEBUG_ILC_KEYEVENTS)
D(bug("[IconList] %s: LEFT: Active @ 0x%p, X %d\n", __PRETTY_FUNCTION__, active_entry, active_entry->ie_IconX));
#endif
                                    if ((entry_next = Node_NextVisible(start_entry)))
                                    {
#if defined(DEBUG_ILC_KEYEVENTS)
D(bug("[IconList] %s: LEFT: Next @ 0x%p, X %d\n", __PRETTY_FUNCTION__, entry_next, entry_next->ie_IconX));
#endif

                                        if (entry_next->ie_IconX < start_X)
                                            entry_next = NULL;
                                        else
                                        {
                                            next_X = entry_next->ie_IconX;
                                            if (data->icld__Option_IconListMode == ICON_LISTMODE_GRID)
                                            {
                                                if (entry_next->ie_AreaWidth < data->icld_IconAreaLargestWidth)
                                                    next_X = next_X - ((data->icld_IconAreaLargestWidth - entry_next->ie_AreaWidth)/2);
                                            }
                                        }
                                    }
                                }
#if defined(DEBUG_ILC_KEYEVENTS)
D(bug("[IconList] %s: LEFT: using start_X %d, start_Y %d\n", __PRETTY_FUNCTION__, start_X, start_Y));
#endif
                            }

                            if (!(message->imsg->Qualifier & IEQUALIFIER_LSHIFT) && ((data->icld_SelectionLastClicked)&&(data->icld_SelectionLastClicked != active_entry)))
                            {
#if defined(DEBUG_ILC_KEYEVENTS)
D(bug("[IconList] %s: LEFT: Clearing selected icons ..\n", __PRETTY_FUNCTION__));
#endif
                                DoMethod(obj, MUIM_IconList_UnselectAll);
                            }

#if defined(DEBUG_ILC_KEYEVENTS)
D(bug("[IconList] %s: LEFT: active = 0x%p, next = 0x%p\n", __PRETTY_FUNCTION__, active_entry, entry_next));
#endif

                            if (!(active_entry))
                            {
                                active_entry = (struct IconEntry *)GetHead(&data->icld_IconList);
                            }

                            while (active_entry != NULL)
                            {
#if defined(DEBUG_ILC_KEYEVENTS)
D(bug("[IconList] %s: LEFT: Checking active @ 0x%p\n", __PRETTY_FUNCTION__, active_entry));
#endif
                                if (data->icld_DisplayFlags & ICONLIST_DISP_VERTICAL)
                                {
                                    if (active_entry->ie_Flags & ICONENTRY_FLAG_VISIBLE)
                                        break;
                                }
                                else
                                {
                                    LONG active_entry_X = active_entry->ie_IconX;
                                    LONG active_entry_Y;
                                    if (data->icld__Option_IconListMode == ICON_LISTMODE_GRID)
                                    {
                                        if (active_entry->ie_AreaWidth < data->icld_IconAreaLargestWidth)
                                            active_entry_X = active_entry_X - ((data->icld_IconAreaLargestWidth - active_entry->ie_AreaWidth)/2);
                                    }
                                    active_entry_Y = active_entry->ie_IconY;
                                    
                                    if (start_entry)
                                    {
                                        if (entry_next)
                                        {
                                            if ((active_entry->ie_Flags & ICONENTRY_FLAG_VISIBLE) &&
                                                (active_entry_Y > start_Y) &&
                                                ((active_entry_X > start_X - 1) &&
                                                (active_entry_X < next_X)))
                                            {
#if defined(DEBUG_ILC_KEYEVENTS)
D(bug("[IconList] %s: LEFT: (A) entry 0x%p matches\n", __PRETTY_FUNCTION__, active_entry));
#endif
                                                break;
                                            }
                                            else if (active_entry == (struct IconEntry *)GetTail(&data->icld_IconList))
                                            {
#if defined(DEBUG_ILC_KEYEVENTS)
D(bug("[IconList] %s: LEFT: (A) reached list end .. starting at the beginng ..\n", __PRETTY_FUNCTION__));
#endif
                                                start_X = entry_next->ie_IconX;
                                                if (data->icld__Option_IconListMode == ICON_LISTMODE_GRID)
                                                {
                                                    if (entry_next->ie_AreaWidth < data->icld_IconAreaLargestWidth)
                                                        start_X = start_X - ((data->icld_IconAreaLargestWidth - entry_next->ie_AreaWidth)/2);
                                                }

                                                if ((entry_next = Node_NextVisible(entry_next)))
                                                {
                                                    if (entry_next->ie_IconX < start_X)
                                                        entry_next = NULL;
                                                    else
                                                    {
                                                        next_X = entry_next->ie_IconX;
                                                        if (data->icld__Option_IconListMode == ICON_LISTMODE_GRID)
                                                        {
                                                            if (entry_next->ie_AreaWidth < data->icld_IconAreaLargestWidth)
                                                                next_X = next_X + ((data->icld_IconAreaLargestWidth - entry_next->ie_AreaWidth)/2);
                                                        }
                                                    }
                                                }
                                                start_Y = 0;
#if defined(DEBUG_ILC_KEYEVENTS)
D(bug("[IconList] %s: LEFT: (A) startx = %d, start_Y = %d, next_X = %d, entry_next @ 0x%p\n", __PRETTY_FUNCTION__, start_X, start_Y, next_X, entry_next));
#endif
                                                active_entry = (struct IconEntry *)GetHead(&data->icld_IconList);
                                            }
                                        }
                                        else
                                        {
                                            if ((active_entry->ie_Flags & ICONENTRY_FLAG_VISIBLE) &&
                                                (active_entry_Y > start_Y) &&
                                                (active_entry_X > start_X - 1))
                                            {
#if defined(DEBUG_ILC_KEYEVENTS)
D(bug("[IconList] %s: LEFT: (B) entry 0x%p matches\n", __PRETTY_FUNCTION__, active_entry));
#endif
                                                break;
                                            }
                                        }
                                    }
                                    else
                                    {
                                        if (active_entry->ie_Flags & ICONENTRY_FLAG_VISIBLE)
                                        {
#if defined(DEBUG_ILC_KEYEVENTS)
D(bug("[IconList] %s: LEFT: (C) entry 0x%p matches\n", __PRETTY_FUNCTION__, active_entry));
#endif
                                            break;
                                        }
                                    }
                                }
                                active_entry = (struct IconEntry *)GetSucc(&active_entry->ie_IconNode);
                            }

                            if (!(active_entry))
                            {
#if defined(DEBUG_ILC_KEYEVENTS)
D(bug("[IconList] %s: LEFT: No Next LEFT Node - Getting first visable icon ..\n", __PRETTY_FUNCTION__));
#endif
                                /* We didnt find a "next LEFT" icon so just use the last visible */
                                active_entry =  (struct IconEntry *)GetHead(&data->icld_IconList);
                                while ((active_entry != NULL) &&(!(active_entry->ie_Flags & ICONENTRY_FLAG_VISIBLE)))
                                {
                                    active_entry = (struct IconEntry *)GetSucc(&active_entry->ie_IconNode);
                                }
                            }
                            
                            if (active_entry)
                            {
                                if (!(active_entry->ie_Flags & ICONENTRY_FLAG_FOCUS))
                                {
                                    active_entry->ie_Flags |= ICONENTRY_FLAG_FOCUS;
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
D(bug("[IconList] %s: RAWKEY_RIGHT\n", __PRETTY_FUNCTION__));
#endif

                            if (data->icld_FocusIcon)
                            {
                                start_entry = data->icld_FocusIcon;
#if defined(DEBUG_ILC_KEYEVENTS)
D(bug("[IconList] %s: RIGHT: Clearing existing focused icon @ 0x%p\n", __PRETTY_FUNCTION__, start_entry));
#endif
                                start_entry->ie_Flags &= ~ICONENTRY_FLAG_FOCUS;
                                data->icld_UpdateMode = UPDATE_SINGLEICON;
                                data->update_icon = start_entry;
                                MUI_Redraw(obj, MADF_DRAWUPDATE);

                                start_X = start_entry->ie_IconX;
                                if (data->icld__Option_IconListMode == ICON_LISTMODE_GRID)
                                {
                                    if (start_entry->ie_AreaWidth < data->icld_IconAreaLargestWidth)
                                        start_X = start_X - ((data->icld_IconAreaLargestWidth - start_entry->ie_AreaWidth)/2);
                                }
                                start_Y = start_entry->ie_IconY;

#if defined(DEBUG_ILC_KEYEVENTS)
D(bug("[IconList] %s: RIGHT: start_X %d, start_Y %d\n", __PRETTY_FUNCTION__, start_X, start_Y));
#endif
                                if (!(active_entry = Node_NextVisible(start_entry)) && (!(data->icld_DisplayFlags & ICONLIST_DISP_VERTICAL)))
                                {
                                    active_entry = (struct IconEntry *)GetHead(&data->icld_IconList);
#if defined(DEBUG_ILC_KEYEVENTS)
D(bug("[IconList] %s: RIGHT: Start at the beginning (Active @ 0x%p) using icon X + Width\n", __PRETTY_FUNCTION__, active_entry));
#endif
                                    start_X = 0;
                                    start_Y = start_Y + start_entry->ie_AreaHeight;
                                    entry_next = NULL;
                                }
                                else if (active_entry && (data->icld_DisplayFlags & ICONLIST_DISP_VERTICAL))
                                {
#if defined(DEBUG_ILC_KEYEVENTS)
D(bug("[IconList] %s: RIGHT: Active @ 0x%p, X %d\n", __PRETTY_FUNCTION__, active_entry, active_entry->ie_IconX));
#endif
                                    if ((entry_next = Node_NextVisible(start_entry)))
                                    {
#if defined(DEBUG_ILC_KEYEVENTS)
D(bug("[IconList] %s: RIGHT: Next @ 0x%p, X %d\n", __PRETTY_FUNCTION__, entry_next, entry_next->ie_IconX));
#endif

                                        if (entry_next->ie_IconY < start_Y)
                                            entry_next = NULL;
                                        else
                                            next_Y = entry_next->ie_IconY;
                                    }
                                }
#if defined(DEBUG_ILC_KEYEVENTS)
D(bug("[IconList] %s: RIGHT: using start_X %d, start_Y %d\n", __PRETTY_FUNCTION__, start_X, start_Y));
#endif
                            }

                            if (!(message->imsg->Qualifier & IEQUALIFIER_LSHIFT) && ((data->icld_SelectionLastClicked)&&(data->icld_SelectionLastClicked != active_entry)))
                            {
#if defined(DEBUG_ILC_KEYEVENTS)
D(bug("[IconList] %s: RIGHT: Clearing selected icons ..\n", __PRETTY_FUNCTION__));
#endif
                                DoMethod(obj, MUIM_IconList_UnselectAll);
                            }

#if defined(DEBUG_ILC_KEYEVENTS)
D(bug("[IconList] %s: RIGHT: active = 0x%p, next = 0x%p\n", __PRETTY_FUNCTION__, active_entry, entry_next));
#endif

                            if (!(active_entry))
                            {
                                active_entry = (struct IconEntry *)GetHead(&data->icld_IconList);
                            }

                            while (active_entry != NULL)
                            {
#if defined(DEBUG_ILC_KEYEVENTS)
D(bug("[IconList] %s: RIGHT: Checking active @ 0x%p\n", __PRETTY_FUNCTION__, active_entry));
#endif
                                if (!(data->icld_DisplayFlags & ICONLIST_DISP_VERTICAL))
                                {
                                    if (active_entry->ie_Flags & ICONENTRY_FLAG_VISIBLE)
                                        break;
                                }
                                else
                                {
                                    LONG active_entry_X = active_entry->ie_IconX;
            LONG active_entry_Y;
                                    if (data->icld__Option_IconListMode == ICON_LISTMODE_GRID)
                                    {
                                        if (active_entry->ie_AreaWidth < data->icld_IconAreaLargestWidth)
                                            active_entry_X = active_entry_X - ((data->icld_IconAreaLargestWidth - active_entry->ie_AreaWidth)/2);
                                    }
                                    active_entry_Y = active_entry->ie_IconY;
                                    
                                    if (start_entry)
                                    {
                                        if (entry_next)
                                        {
                                            if ((active_entry->ie_Flags & ICONENTRY_FLAG_VISIBLE) &&
                                                (active_entry_X > start_X) &&
                                                ((active_entry_Y > start_Y - 1) &&
                                                (active_entry_Y < next_Y)))
                                            {
#if defined(DEBUG_ILC_KEYEVENTS)
D(bug("[IconList] %s: RIGHT: (A) entry 0x%p matches\n", __PRETTY_FUNCTION__, active_entry));
#endif
                                                break;
                                            }
                                            else if (active_entry == (struct IconEntry *)GetTail(&data->icld_IconList))
                                            {
#if defined(DEBUG_ILC_KEYEVENTS)
D(bug("[IconList] %s: RIGHT: (A) reached list end .. starting at the beginng ..\n", __PRETTY_FUNCTION__));
#endif
                                                start_Y = entry_next->ie_IconY;

                                                if ((entry_next = Node_NextVisible(entry_next)))
                                                {
                                                    if (entry_next->ie_IconY < start_Y)
                                                        entry_next = NULL;
                                                    else
                                                    {
                                                        next_Y = entry_next->ie_IconY;
                                                    }
                                                }
                                                start_Y = 0;
#if defined(DEBUG_ILC_KEYEVENTS)
D(bug("[IconList] %s: RIGHT: (A) startx = %d, start_Y = %d, next_X = %d, entry_next @ 0x%p\n", __PRETTY_FUNCTION__, start_X, start_Y, next_X, entry_next));
#endif
                                                active_entry = (struct IconEntry *)GetHead(&data->icld_IconList);
                                            }
                                        }
                                        else
                                        {
                                            if ((active_entry->ie_Flags & ICONENTRY_FLAG_VISIBLE) &&
                                                (active_entry_X > start_X) &&
                                                (active_entry_Y > start_Y - 1))
                                            {
#if defined(DEBUG_ILC_KEYEVENTS)
D(bug("[IconList] %s: RIGHT: (B) entry 0x%p matches\n", __PRETTY_FUNCTION__, active_entry));
#endif
                                                break;
                                            }
                                        }
                                    }
                                    else
                                    {
                                        if (active_entry->ie_Flags & ICONENTRY_FLAG_VISIBLE)
                                        {
#if defined(DEBUG_ILC_KEYEVENTS)
D(bug("[IconList] %s: RIGHT: (C) entry 0x%p matches\n", __PRETTY_FUNCTION__, active_entry));
#endif
                                            break;
                                        }
                                    }
                                }
                                active_entry = (struct IconEntry *)GetSucc(&active_entry->ie_IconNode);
                            }

                            if (!(active_entry))
                            {
#if defined(DEBUG_ILC_KEYEVENTS)
D(bug("[IconList] %s: RIGHT: No Next RIGHT Node - Getting first visable icon ..\n", __PRETTY_FUNCTION__));
#endif
                                /* We didnt find a "next RIGHT" icon so just use the first visible */
                                active_entry =  (struct IconEntry *)GetHead(&data->icld_IconList);
                                while ((active_entry != NULL) &&(!(active_entry->ie_Flags & ICONENTRY_FLAG_VISIBLE)))
                                {
                                    active_entry = (struct IconEntry *)GetSucc(&active_entry->ie_IconNode);
                                }
                            }
                            
                            if (active_entry)
                            {
                                if (!(active_entry->ie_Flags & ICONENTRY_FLAG_FOCUS))
                                {
                                    active_entry->ie_Flags |= ICONENTRY_FLAG_FOCUS;
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
D(bug("[IconList] %s: RAWKEY_HOME\n", __PRETTY_FUNCTION__));
#endif

                            if (data->icld_FocusIcon)
                            {
                                data->icld_FocusIcon->ie_Flags &= ~ICONENTRY_FLAG_FOCUS;
                                data->icld_UpdateMode = UPDATE_SINGLEICON;
                                data->update_icon = data->icld_FocusIcon;
                                MUI_Redraw(obj, MADF_DRAWUPDATE);
                            }

                            active_entry =  Node_FirstVisible(&data->icld_IconList);
                            
                            if ((active_entry) && (!(active_entry->ie_Flags & ICONENTRY_FLAG_FOCUS)))
                            {
                                active_entry->ie_Flags |= ICONENTRY_FLAG_FOCUS;
                                data->icld_UpdateMode = UPDATE_SINGLEICON;
                                data->update_icon = active_entry;
                                MUI_Redraw(obj, MADF_DRAWUPDATE);
                            }
                            data->icld_FocusIcon = active_entry;
                            break;

                        case RAWKEY_END:
                            rawkey_handled = TRUE;

#if defined(DEBUG_ILC_KEYEVENTS)
D(bug("[IconList] %s: RAWKEY_END\n", __PRETTY_FUNCTION__));
#endif

                            if (data->icld_FocusIcon)
                            {
                                data->icld_FocusIcon->ie_Flags &= ~ICONENTRY_FLAG_FOCUS;
                                data->icld_UpdateMode = UPDATE_SINGLEICON;
                                data->update_icon = data->icld_FocusIcon;
                                MUI_Redraw(obj, MADF_DRAWUPDATE);
                            }

                            active_entry = Node_LastVisible(&data->icld_IconList);
                            
                            if ((active_entry) && (!(active_entry->ie_Flags & ICONENTRY_FLAG_FOCUS)))
                            {
                                active_entry->ie_Flags |= ICONENTRY_FLAG_FOCUS;
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
                        struct IconEntry      *node = NULL;
                        struct IconEntry      *new_selected = NULL;
                        struct Rectangle     rect;

//                        int selections = 0;
                        BOOL icon_doubleclicked;

                        /* check if clicked on icon */
#ifdef __AROS__
                        ForeachNode(&data->icld_IconList, node)
#else
                        Foreach_Node(&data->icld_IconList, node);
#endif
                        {
                            if (node->ie_Flags & ICONENTRY_FLAG_VISIBLE)
                            {
                                BOOL update_icon = FALSE;

                                rect.MinX = node->ie_IconX;
                                rect.MaxX = node->ie_IconX + node->ie_AreaWidth - 1;
                                rect.MinY = node->ie_IconY;
                                rect.MaxY = node->ie_IconY + node->ie_AreaHeight - 1;

                                if ((data->icld__Option_IconListMode == ICON_LISTMODE_GRID) &&
                                    (node->ie_AreaWidth < data->icld_IconAreaLargestWidth))
                                {
                                    rect.MinX += ((data->icld_IconAreaLargestWidth - node->ie_AreaWidth)/2);
                                    rect.MaxX += ((data->icld_IconAreaLargestWidth - node->ie_AreaWidth)/2);
                                }

                                if ((((mx + data->icld_ViewX) >= rect.MinX) && ((mx + data->icld_ViewX) <= rect.MaxX )) &&
                                    (((my + data->icld_ViewY) >= rect.MinY) && ((my + data->icld_ViewY) <= rect.MaxY )) &&
                                    !new_selected)
                                {
                                    new_selected = node;
#if defined(DEBUG_ILC_EVENTS)
D(bug("[IconList] %s: Icon '%s' clicked on ..\n", __PRETTY_FUNCTION__, node->ie_IconListEntry.label));
#endif
                                }

                                if (node->ie_Flags & ICONENTRY_FLAG_SELECTED)
                                {
                                    if ((new_selected != node) &&
                                        (!(message->imsg->Qualifier & (IEQUALIFIER_LSHIFT | IEQUALIFIER_RSHIFT))))
                                    {
                                        Remove(&node->ie_SelectionNode);
                                        node->ie_Flags &= ~ICONENTRY_FLAG_SELECTED;
                                        update_icon = TRUE;
                                    }
                                }

                                if ((node->ie_Flags & ICONENTRY_FLAG_FOCUS) && (new_selected != node))
                                {
                                    node->ie_Flags &= ~ICONENTRY_FLAG_FOCUS;
                                    update_icon = TRUE;
                                }

                                if (update_icon)
                                {
                                    data->icld_UpdateMode = UPDATE_SINGLEICON;
                                    data->update_icon = node;
                                    MUI_Redraw(obj, MADF_DRAWUPDATE);
#if defined(DEBUG_ILC_EVENTS)
D(bug("[IconList] %s: Rendered icon '%s'\n", __PRETTY_FUNCTION__, node->ie_IconListEntry.label));
#endif
                                }
                            }
                        }

                        icon_doubleclicked = FALSE;

                        if ((DoubleClick(data->last_secs, data->last_mics, message->imsg->Seconds, message->imsg->Micros)) && (data->icld_SelectionLastClicked == new_selected))
                        {
D(bug("[IconList] %s: Icon double-clicked\n", __PRETTY_FUNCTION__));
                            icon_doubleclicked = TRUE;
                        }

                        if (new_selected == NULL)
                        {
                            /* No icon clicked on ... Start Lasso-selection */
                            data->icld_LassoActive = TRUE;
                            if (!(message->imsg->Qualifier & (IEQUALIFIER_LSHIFT | IEQUALIFIER_RSHIFT)))
                            {
                                data->icld_SelectionLastClicked = NULL;
                                data->icld_FocusIcon = NULL;
                            }
                            data->icld_LassoRectangle.MinX = mx - data->view_rect.MinX + data->icld_ViewX;  
                            data->icld_LassoRectangle.MinY = my - data->view_rect.MinY + data->icld_ViewY;
                            data->icld_LassoRectangle.MaxX = mx - data->view_rect.MinX + data->icld_ViewX;
                            data->icld_LassoRectangle.MaxY = my - data->view_rect.MinY + data->icld_ViewY; 
 
                            /* Draw initial Lasso frame */
                            IconList_InvertLassoOutlines(obj, &data->icld_LassoRectangle);
                        }
                        else
                        {
                            struct IconEntry      *update_icon = NULL;

                            data->icld_LassoActive = FALSE;

                            if (!(new_selected->ie_Flags & ICONENTRY_FLAG_SELECTED))
                            {
                                AddTail(&data->icld_SelectionList, &new_selected->ie_SelectionNode);
                                new_selected->ie_Flags |= ICONENTRY_FLAG_SELECTED;
                                update_icon = new_selected;

                                if (!(new_selected->ie_Flags & ICONENTRY_FLAG_FOCUS))
                                {
                                    new_selected->ie_Flags |= ICONENTRY_FLAG_FOCUS;
                                    update_icon = new_selected;
                                    data->icld_FocusIcon = new_selected;
                                }
                            }
                            else if ((icon_doubleclicked == FALSE) && (message->imsg->Qualifier & (IEQUALIFIER_LSHIFT | IEQUALIFIER_RSHIFT)))
                            {
                                Remove(&new_selected->ie_SelectionNode);
                                new_selected->ie_Flags &= ~ICONENTRY_FLAG_SELECTED;
                                update_icon = new_selected;
                                new_selected = NULL;
                            }

                            if (update_icon != NULL)
                            {
                                data->icld_UpdateMode = UPDATE_SINGLEICON;
                                data->update_icon = update_icon;
                                MUI_Redraw(obj, MADF_DRAWUPDATE);
#if defined(DEBUG_ILC_EVENTS)
D(bug("[IconList] %s: Rendered 'new_selected' icon '%s'\n", __PRETTY_FUNCTION__, update_icon->ie_IconListEntry.label));
#endif
                            }
                        }                       

                        data->icld_ClickEvent.shift = !!(message->imsg->Qualifier & (IEQUALIFIER_LSHIFT | IEQUALIFIER_RSHIFT));
                        data->icld_ClickEvent.entry = new_selected ? &new_selected->ie_IconListEntry : NULL;
                        SET(obj, MUIA_IconList_Clicked, (IPTR)&data->icld_ClickEvent);

                        if (icon_doubleclicked)
                        {
                            SET(obj, MUIA_IconList_DoubleClick, TRUE);
                        }
                        else if (!data->mouse_pressed)
                        {
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

                        data->icld_SelectionLastClicked = new_selected;

                        data->click_x = mx;
                        data->click_y = my;

						SET(obj, MUIA_IconList_SelectionChanged, TRUE);

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
                        if (data->icld_LassoActive == TRUE)
                        {
                            // End Lasso-selection
                            struct Rectangle   old_lasso;
                            struct IconEntry   *node = NULL;

                            //Clear Lasso Frame..
                            GetAbsoluteLassoRect(data, &old_lasso); 
                            IconList_InvertLassoOutlines(obj, &old_lasso);

                            data->icld_LassoActive = FALSE;

                            //Remove Lasso flag from affected icons..
#ifdef __AROS__
                            ForeachNode(&data->icld_IconList, node)
#else
                            Foreach_Node(&data->icld_IconList, node);
#endif
                            {
                                if (node->ie_Flags & ICONENTRY_FLAG_LASSO)
                                {
                                    node->ie_Flags &= ~ICONENTRY_FLAG_LASSO;
                                }
                            }
							SET(obj, MUIA_IconList_SelectionChanged, TRUE);
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

                    if (data->icld_SelectionLastClicked && (data->icld_LassoActive == FALSE) && 
                        ((abs(move_x - data->click_x) >= 2) || (abs(move_y - data->click_y) >= 2)))
                    {
                        // Icon(s) being dragged ....
                        DoMethod(_win(obj),MUIM_Window_RemEventHandler, (IPTR)&data->ehn);
                        data->ehn.ehn_Events &= ~IDCMP_MOUSEMOVE;
                        DoMethod(_win(obj),MUIM_Window_AddEventHandler, (IPTR)&data->ehn);
            
                        data->mouse_pressed &= ~LEFT_BUTTON;
            
                        data->touch_x = move_x + data->icld_ViewX - data->icld_SelectionLastClicked->ie_IconX;
                        data->touch_y = move_y + data->icld_ViewY - data->icld_SelectionLastClicked->ie_IconY;
                        DoMethod(obj,MUIM_DoDrag, data->touch_x, data->touch_y, 0);
                    }
                    else if (data->icld_LassoActive == TRUE)
                    {
                        //Lasso active ..
                        struct Rectangle    new_lasso,
                                            old_lasso;
                        struct Rectangle    iconrect;

                        struct IconEntry    *node = NULL;
//                        struct IconEntry    *new_selected = NULL; 

                        /* Remove previous Lasso frame */
                        GetAbsoluteLassoRect(data, &old_lasso);                          
                        IconList_InvertLassoOutlines(obj, &old_lasso);

                        /* if the mouse leaves our visible area scroll the view */
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
                                SetAttrs(obj, MUIA_Virtgroup_Left, newleft, MUIA_Virtgroup_Top, newtop, TAG_DONE);
                            }
                        } 

                        /* update Lasso coordinates */
                        data->icld_LassoRectangle.MaxX = mx - data->view_rect.MinX + data->icld_ViewX;
                        data->icld_LassoRectangle.MaxY = my - data->view_rect.MinY + data->icld_ViewY;

                        /* get absolute Lasso coordinates */
                        GetAbsoluteLassoRect(data, &new_lasso);

#ifdef __AROS__
                        ForeachNode(&data->icld_IconList, node)
#else
                        Foreach_Node(&data->icld_IconList, node);
#endif
                        {
                            IPTR update_icon = (IPTR)NULL;

                            if (node->ie_Flags & ICONENTRY_FLAG_VISIBLE)
                            {
                                iconrect.MinX = node->ie_IconX;
                                iconrect.MaxX = node->ie_IconX + node->ie_AreaWidth - 1;
                                iconrect.MinY = node->ie_IconY;
                                iconrect.MaxY = node->ie_IconY + node->ie_AreaHeight - 1;
                                if ((data->icld__Option_IconListMode == ICON_LISTMODE_GRID) &&
                                    (node->ie_AreaWidth < data->icld_IconAreaLargestWidth))
                                {
                                    iconrect.MinX += ((data->icld_IconAreaLargestWidth - node->ie_AreaWidth)/2);
                                    iconrect.MaxX += ((data->icld_IconAreaLargestWidth - node->ie_AreaWidth)/2);
                                }

                                if ((((new_lasso.MaxX + data->icld_ViewX) >= iconrect.MinX) && ((new_lasso.MinX + data->icld_ViewX) <= iconrect.MaxX)) &&
                                    (((new_lasso.MaxY + data->icld_ViewY) >= iconrect.MinY) && ((new_lasso.MinY + data->icld_ViewY) <= iconrect.MaxY)))
                                {
                                    //Icon is inside our lasso ..
                                     if (!(node->ie_Flags & ICONENTRY_FLAG_LASSO))
                                     {
                                         /* check if icon was already selected before */
                                        if (node->ie_Flags & ICONENTRY_FLAG_SELECTED)
                                        {
                                            Remove(&node->ie_SelectionNode);
                                            node->ie_Flags &= ~ICONENTRY_FLAG_SELECTED;
                                        }
                                        else
                                        {
                                            AddTail(&data->icld_SelectionList, &node->ie_SelectionNode);
                                            node->ie_Flags |= ICONENTRY_FLAG_SELECTED;
                                        }
                                         node->ie_Flags |= ICONENTRY_FLAG_LASSO;
                                         update_icon = (IPTR)node;
                                     }
                                } 
                                else if (node->ie_Flags & ICONENTRY_FLAG_LASSO)
                                {
                                    //Icon is no longer inside our lasso - revert its selected state
                                    if (node->ie_Flags & ICONENTRY_FLAG_SELECTED)
                                    {
                                        Remove(&node->ie_SelectionNode);
                                        node->ie_Flags &= ~ICONENTRY_FLAG_SELECTED;
                                    }
                                    else
                                    {
                                        AddTail(&data->icld_SelectionList, &node->ie_SelectionNode);
                                        node->ie_Flags |= ICONENTRY_FLAG_SELECTED;
                                    }
                                    node->ie_Flags &= ~ICONENTRY_FLAG_LASSO;
                                    update_icon = (IPTR)node;
                                }

                                if (update_icon)
                                {
                                    data->icld_UpdateMode = UPDATE_SINGLEICON;
                                    data->update_icon = (struct IconEntry *)update_icon;
                                    MUI_Redraw(obj, MADF_DRAWUPDATE);
                                }
                            }
                        }
                        /* Draw Lasso frame */                         
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
                        SetAttrs(obj, MUIA_Virtgroup_Left, newleft,
                            MUIA_Virtgroup_Top, newtop,
                            TAG_DONE);
                    }
        
                    return MUI_EventHandlerRC_Eat;
                }
    
            break;
        }
    }

    return 0;
}
///

///MUIM_IconList_NextIcon()
/**************************************************************************
MUIM_IconList_NextIcon
**************************************************************************/
IPTR IconList__MUIM_IconList_NextIcon(struct IClass *CLASS, Object *obj, struct MUIP_IconList_NextIcon *message)
{
    struct IconList_DATA    *data = INST_DATA(CLASS, obj);
    struct IconEntry       *node = NULL;
    struct IconList_Entry  *ent = NULL;
    IPTR                    node_successor = (IPTR)NULL;

    if (message->entry == NULL) return (IPTR)NULL;
    ent = *message->entry;

#if defined(DEBUG_ILC_FUNCS)
D(bug("[IconList]: %s()\n", __PRETTY_FUNCTION__));
#endif

    if ((IPTR)ent == (IPTR)MUIV_IconList_NextIcon_Start)
    {
D(bug("[IconList] %s: Finding First Entry ..\n", __PRETTY_FUNCTION__));
		if (message->nextflag == MUIV_IconList_NextIcon_Selected)
		{
			node = (struct IconEntry *)GetHead(&data->icld_SelectionList);
			if (node != NULL)
			{
				node = (struct IconEntry *)((IPTR)node - ((IPTR)&node->ie_SelectionNode - (IPTR)node));
			}
		}
		else if (message->nextflag == MUIV_IconList_NextIcon_Visible)
		{
			node = (struct IconEntry *)GetHead(&data->icld_IconList);
			while (node != NULL)
			{
				if (node->ie_Flags & ICONENTRY_FLAG_VISIBLE)
					break;

				node = (struct IconEntry *)GetSucc(&node->ie_IconNode);
			}
		}
    }
    else if ((IPTR)ent != (IPTR)MUIV_IconList_NextIcon_End)
    {
		node = (struct IconEntry *)((IPTR)ent - ((IPTR)&node->ie_IconListEntry - (IPTR)node));
		if (message->nextflag == MUIV_IconList_NextIcon_Selected)
		{
			node_successor = (IPTR)GetSucc(&node->ie_SelectionNode);
			if (node_successor != (IPTR)NULL)
				node = (struct IconEntry *)((IPTR)node_successor - ((IPTR)&node->ie_SelectionNode - (IPTR)node));
			else
			{
D(bug("[IconList] %s: GetSucc() == NULL\n", __PRETTY_FUNCTION__));
				node = NULL;
			}
		}
		else if (message->nextflag == MUIV_IconList_NextIcon_Visible)
		{
			node = (struct IconEntry *)GetSucc(&node->ie_IconNode);
			while (node != NULL)
			{
				if (node->ie_Flags & ICONENTRY_FLAG_VISIBLE)
					break;

				node = (struct IconEntry *)GetSucc(&node->ie_IconNode);
			}
		}
    }

    if (node == NULL)
    {
D(bug("[IconList] %s: Returning MUIV_IconList_NextIcon_End\n", __PRETTY_FUNCTION__));

        *message->entry = (struct IconList_Entry *)MUIV_IconList_NextIcon_End;
    }
    else
    {
D(bug("[IconList] %s: Returning entry for '%s'\n", __PRETTY_FUNCTION__, node->ie_IconListEntry.label));

        *message->entry = &node->ie_IconListEntry;
    }

    return (IPTR)NULL;
}
///

///MUIM_IconList_GetIconPrivate()
/**************************************************************************
MUIM_IconList_GetIconPrivate
**************************************************************************/
IPTR IconList__MUIM_IconList_GetIconPrivate(struct IClass *CLASS, Object *obj, struct MUIP_IconList_GetIconPrivate *message)
{
//    struct IconList_DATA    *data = INST_DATA(CLASS, obj);
    struct IconEntry       *node = NULL;

    if (message->entry == NULL) return (IPTR)NULL;

    node = (struct IconEntry *)((IPTR)message->entry - ((IPTR)&node->ie_IconListEntry - (IPTR)node));

    return (IPTR)node;
}

///MUIM_CreateDragImage()
/**************************************************************************
MUIM_CreateDragImage
**************************************************************************/
IPTR IconList__MUIM_CreateDragImage(struct IClass *CLASS, Object *obj, struct MUIP_CreateDragImage *message)
{
    struct IconList_DATA     *data = INST_DATA(CLASS, obj);
    struct MUI_DragImage    *img = NULL;
    LONG                    first_x = -1,
                            first_y = -1;

#if defined(DEBUG_ILC_FUNCS)
D(bug("[IconList]: %s()\n", __PRETTY_FUNCTION__));
#endif

    if (!(data->icld_SelectionLastClicked)) DoSuperMethodA(CLASS, obj, (Msg)message);

    if ((img = (struct MUI_DragImage *)AllocVec(sizeof(struct MUIP_CreateDragImage), MEMF_CLEAR)))
    {
        struct Node      *node = NULL;
        struct IconEntry *entry = NULL;

        LONG depth = GetBitMapAttr(_screen(obj)->RastPort.BitMap, BMA_DEPTH);
    
#if defined(CREATE_FULL_DRAGIMAGE)
#ifdef __AROS__
        ForeachNode(&data->icld_SelectionList, node)
#else
        Foreach_Node(&data->icld_SelectionList, node);
#endif
        {
            entry = (struct IconEntry *)((IPTR)node - ((IPTR)&entry->ie_SelectionNode - (IPTR)entry));
            if ((entry->ie_Flags & ICONENTRY_FLAG_VISIBLE) && (entry->ie_Flags & ICONENTRY_FLAG_SELECTED))
            {
                if ((first_x == -1) || ((first_x != -1) && (entry->ie_IconX < first_x))) first_x = entry->ie_IconX;
                if ((first_y == -1) || ((first_y != -1) && (entry->ie_IconY < first_y))) first_y = entry->ie_IconY;
                if ((entry->ie_IconX + entry->ie_IconWidth) > img->width)   img->width = entry->ie_IconX + entry->ie_IconWidth;
                if ((entry->ie_IconY + entry->ie_IconHeight) > img->height) img->height = entry->ie_IconY + entry->ie_IconHeight;
            }
        }
        img->width = (img->width - first_x) + 2;
        img->height = (img->height - first_y) + 2;
#else
        entry = data->icld_SelectionLastClicked;
        img->width = entry->ie_IconWidth;
        img->height = entry->ie_IconHeight;
        first_x = entry->ie_IconX;
        first_y = entry->ie_IconY;
#endif

        img->touchx = data->click_x - first_x;
        img->touchy = data->click_y - first_y;

        if ((img->bm = AllocBitMap(img->width, img->height, depth, BMF_CLEAR, _screen(obj)->RastPort.BitMap)))
        {
            struct RastPort temprp;
            InitRastPort(&temprp);
            temprp.BitMap = img->bm;

#if defined(CREATE_FULL_DRAGIMAGE)
            ForeachNode(&data->icld_SelectionList, node)
            {
                entry = (struct IconEntry *)((IPTR)node - ((IPTR)&entry->ie_SelectionNode - (IPTR)entry));
                if ((entry->ie_Flags & ICONENTRY_FLAG_VISIBLE) && (entry->ie_Flags & ICONENTRY_FLAG_SELECTED))
                {
                    DrawIconStateA
                        (
                            &temprp, entry->ie_DiskObj, NULL,
                            (entry->ie_IconX + 1) - first_x, (entry->ie_IconY + 1) - first_y,
                            IDS_SELECTED,
                            __iconList_DrawIconStateTags
                        );
                }
            }
#else
            DrawIconStateA
                (
                    &temprp, entry->ie_DiskObj, NULL,
                    0, 0,
                    IDS_SELECTED,
                    __iconList_DrawIconStateTags
                );
#endif
            RastPortSetAlpha(&temprp, data->click_x, data->click_y, img->width, img->height, 0x80, RPALPHAFLAT);
            DeinitRastPort(&temprp);
        }
    
        img->touchx = message->touchx;
        img->touchy = message->touchy;
        img->flags = 0;
#if defined(__MORPHOS__)
        img->dragmode = DD_TRANSPARENT;
#endif
    }
    return (IPTR)img;
}
///

///MUIM_DeleteDragImage()
/**************************************************************************
MUIM_DeleteDragImage
**************************************************************************/
IPTR IconList__MUIM_DeleteDragImage(struct IClass *CLASS, Object *obj, struct MUIP_DeleteDragImage *message)
{
    struct IconList_DATA *data = INST_DATA(CLASS, obj);

#if defined(DEBUG_ILC_FUNCS)
D(bug("[IconList]: %s()\n", __PRETTY_FUNCTION__));
#endif

    if (!(data->icld_SelectionLastClicked)) return DoSuperMethodA(CLASS,obj,(Msg)message);

    if (message->di)
    {
        if (message->di->bm)
            FreeBitMap(message->di->bm);
        FreeVec(message->di);
    }
    return (IPTR)NULL;
}
///

///MUIM_DragQuery()
/**************************************************************************
MUIM_DragQuery
**************************************************************************/
IPTR IconList__MUIM_DragQuery(struct IClass *CLASS, Object *obj, struct MUIP_DragQuery *message)
{
#if defined(DEBUG_ILC_FUNCS)
D(bug("[IconList]: %s()\n", __PRETTY_FUNCTION__));
#endif
    if (message->obj == obj)
        return MUIV_DragQuery_Accept;
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
        if (is_iconlist)
            return MUIV_DragQuery_Accept;
    }

    return MUIV_DragQuery_Refuse;
}
///

///MUIM_DragDrop()
/**************************************************************************
MUIM_DragDrop
**************************************************************************/
IPTR IconList__MUIM_DragDrop(struct IClass *CLASS, Object *obj, struct MUIP_DragDrop *message)
{
    struct IconList_DATA *data = INST_DATA(CLASS, obj);

#if defined(DEBUG_ILC_FUNCS)
D(bug("[IconList]: %s()\n", __PRETTY_FUNCTION__));
#endif

    struct IconList_Entry *entry    = (IPTR)MUIV_IconList_NextIcon_Start;

    if (data->icld_DragDropEvent)
    {
        struct IconList_Drop_SourceEntry *clean_node;
#if defined(DEBUG_ILC_ICONDRAGDROP)
        D(bug("[IconList] %s: Cleaning existing IconList_Drop_Event @ %p\n", __PRETTY_FUNCTION__, data->icld_DragDropEvent));
#endif
        while ((clean_node = (struct IconList_Drop_SourceEntry *)RemTail(&data->icld_DragDropEvent->drop_SourceList)) != NULL)
        {
            FreeVec(clean_node->dropse_Node.ln_Name);
            FreeMem(clean_node, sizeof(struct IconList_Drop_SourceEntry));
        }
        if (data->icld_DragDropEvent->drop_TargetPath) FreeVec(data->icld_DragDropEvent->drop_TargetPath);
        FreeMem(data->icld_DragDropEvent, sizeof(struct IconList_Drop_Event));
        data->icld_DragDropEvent = NULL;
    }

    /* SANITY CHECK: Get first selected entry from SOURCE iconlist */
    DoMethod(message->obj, MUIM_IconList_NextIcon, MUIV_IconList_NextIcon_Selected, (IPTR)&entry);

    if ((entry) && (entry != (IPTR)MUIV_IconList_NextIcon_End))
    {
        /* Ok.. atleast one icon was dropped .. */
        BOOL                            iconMove = FALSE;
        struct IconEntry                *node = NULL;
        struct IconEntry                *drop_target_node = NULL;
        STRPTR                          directory_path = NULL;
        struct IconList_Drop_Event      *dragDropEvent = NULL;

        GET(obj, MUIA_IconDrawerList_Drawer, &directory_path);

        if ((dragDropEvent = AllocMem(sizeof(struct IconList_Drop_Event), MEMF_CLEAR)) == NULL)
        {
#if defined(DEBUG_ILC_ICONDRAGDROP)
            D(bug("[IconList] %s: Failed to allocate IconList_Drop_Event Storage!\n", __PRETTY_FUNCTION__));
#endif
            goto dragdropdone;
        }
        D(bug("[IconList] %s: Allocated IconList_Drop_Event @ %p\n", __PRETTY_FUNCTION__, dragDropEvent));

        NewList(&dragDropEvent->drop_SourceList);

        /* go through list and check if dropped on icon */
#ifdef __AROS__
        ForeachNode(&data->icld_IconList, node)
#else
        Foreach_Node(&data->icld_IconList, node);
#endif
        {
           if ((node->ie_Flags & ICONENTRY_FLAG_VISIBLE) &&
                   (message->x >= (node->ie_IconX - data->icld_ViewX)) && 
                   (message->x <  (node->ie_IconX - data->icld_ViewX + node->ie_AreaWidth))  &&
                   (message->y >= (node->ie_IconY - data->icld_ViewY + _mtop(obj)))  && 
                   (message->y <  (node->ie_IconY - data->icld_ViewY + node->ie_AreaHeight + _mtop(obj))))
           {
               drop_target_node = node;
               break;
           } 
        }

        if (drop_target_node && 
            ((drop_target_node->ie_IconListEntry.type == ST_SOFTLINK)   ||
             (drop_target_node->ie_IconListEntry.type == ST_ROOT)       ||
             (drop_target_node->ie_IconListEntry.type == ST_USERDIR)    ||
             (drop_target_node->ie_IconListEntry.type == ST_LINKDIR)    ||
             (drop_target_node->ie_IconListEntry.type == ST_FILE)       ||
             (drop_target_node->ie_IconListEntry.type == ST_LINKFILE)))
        {
            if ((drop_target_node->ie_IconListEntry.type != ST_ROOT) && (drop_target_node->ie_IconListEntry.type != ST_SOFTLINK))
            {
                if (directory_path)
                {
                    int fulllen = strlen(directory_path) + strlen(drop_target_node->ie_IconListEntry.label) + 2;

                    if ((dragDropEvent->drop_TargetPath = AllocVec(fulllen, MEMF_CLEAR)) == NULL)
                    {
#if defined(DEBUG_ILC_ICONDRAGDROP)
                        D(bug("[IconList] %s: Failed to allocate IconList_Drop_Event->drop_TargetPath Storage!\n", __PRETTY_FUNCTION__));
#endif
                        goto dragdropdone;
                    }
                    strcpy(dragDropEvent->drop_TargetPath, directory_path);
                    AddPart(dragDropEvent->drop_TargetPath, drop_target_node->ie_IconListEntry.label, fulllen);
                }
            }
            else
            {
                if ((dragDropEvent->drop_TargetPath = AllocVec(strlen(drop_target_node->ie_IconListEntry.label) + 1, MEMF_CLEAR)) == NULL)
                {
#if defined(DEBUG_ILC_ICONDRAGDROP)
                    D(bug("[IconList] %s: Failed to allocate IconList_Drop_Event->drop_TargetPath Storage!\n", __PRETTY_FUNCTION__));
#endif
                    goto dragdropdone;
                }
                strcpy(dragDropEvent->drop_TargetPath, drop_target_node->ie_IconListEntry.label);
            }

#if defined(DEBUG_ILC_ICONDRAGDROP)
            D(bug("[IconList] %s: Target Icon Full Path = '%s'\n", __PRETTY_FUNCTION__, dragDropEvent->drop_TargetPath));
#endif
            /* mark the Icon the selection was dropped on*/
            //drop_target_node->ie_Flags |= ICONENTRY_FLAG_SELECTED;
            //data->icld_UpdateMode = UPDATE_SINGLEICON;
            //data->update_icon = drop_target_node;
            //MUI_Redraw(obj,MADF_DRAWUPDATE);
        }
        else
        {
            /* not dropped on icon -> get path of DESTINATION iconlist */
            /* Note: directory_path is NULL when dropped on Wanderer's desktop */
            if ((message->obj != obj) && directory_path)
            {
#if defined(DEBUG_ILC_ICONDRAGDROP)
                D(bug("[IconList] %s: drop entry: Icons dropped in window '%s'\n", __PRETTY_FUNCTION__, directory_path));
#endif
                /* copy path */
                if ((dragDropEvent->drop_TargetPath = AllocVec(strlen(directory_path) + 1, MEMF_CLEAR)) != NULL)
                {
                    strcpy(dragDropEvent->drop_TargetPath, directory_path);
                }
                else
                {
#if defined(DEBUG_ILC_ICONDRAGDROP)
                    D(bug("[IconList] %s: Failed to allocate IconList_Drop_Event->drop_TargetPath Storage!\n", __PRETTY_FUNCTION__));
#endif
                    goto dragdropdone;
                }
            }
            else if (message->obj == obj)
            {
#if defined(DEBUG_ILC_ICONDRAGDROP)
                D(bug("[IconList] %s: drop entry: Icon Move detected ..\n", __PRETTY_FUNCTION__));
#endif
                iconMove = TRUE;
                SET(obj, MUIA_IconList_IconsMoved, (IPTR)entry); // Now notify
                MUI_Redraw(obj,MADF_DRAWOBJECT);
                DoMethod(obj, MUIM_IconList_CoordsSort);
            }
            else
            {
#if defined(DEBUG_ILC_ICONDRAGDROP)
                D(bug("[IconList] %s: Icons Dropped on Wanderer Desktop (unhandled)!\n", __PRETTY_FUNCTION__));
#endif
                iconMove = TRUE;
            }
        }
        if (!(iconMove))
        {
            int copycount = 0;
            /* Create list of entries to copy .. */
            entry = (IPTR)MUIV_IconList_NextIcon_Start;
            while (entry != (IPTR)MUIV_IconList_NextIcon_End)
            {
                DoMethod(message->obj, MUIM_IconList_NextIcon, MUIV_IconList_NextIcon_Selected, (IPTR)&entry);

                if (entry != (IPTR)MUIV_IconList_NextIcon_End)
                {
                    struct IconList_Drop_SourceEntry *sourceEntry = NULL;
                    sourceEntry = AllocMem(sizeof(struct IconList_Drop_SourceEntry), MEMF_CLEAR);
                    if ((entry->type != ST_ROOT) && (entry->type != ST_SOFTLINK))
                    {
                        int fulllen = 0;
                        IPTR path = 0;

                        GET(message->obj, MUIA_IconDrawerList_Drawer, &path);

                        if (path)
                        {
#warning "TODO: Get full path for source icons .. "
                            fulllen = strlen(path) + strlen(entry->ile_IconEntry->ie_IconNode.ln_Name) + 2;
                            sourceEntry->dropse_Node.ln_Name = AllocVec(fulllen, MEMF_CLEAR);
                            strcpy(sourceEntry->dropse_Node.ln_Name, path);
                            AddPart(sourceEntry->dropse_Node.ln_Name, entry->ile_IconEntry->ie_IconNode.ln_Name, fulllen);
#if defined(DEBUG_ILC_ICONDRAGDROP)
                            D(bug("[IconList] %s: Source Icon (Full Path) = '%s'\n", __PRETTY_FUNCTION__, sourceEntry->dropse_Node.ln_Name));
#endif
                        }
                    }
                    else
                    {
                        sourceEntry->dropse_Node.ln_Name = AllocVec(strlen(entry->ile_IconEntry->ie_IconNode.ln_Name) + 1, MEMF_CLEAR);
                        strcpy(sourceEntry->dropse_Node.ln_Name, entry->ile_IconEntry->ie_IconNode.ln_Name);
#if defined(DEBUG_ILC_ICONDRAGDROP)
                        D(bug("[IconList] %s: Source Icon = '%s'\n", __PRETTY_FUNCTION__, sourceEntry->dropse_Node.ln_Name));
#endif
                    }
                    if ((strcasecmp(dragDropEvent->drop_TargetPath, sourceEntry->dropse_Node.ln_Name)) != 0)
                    {
                        copycount += 1;
                        AddTail(&dragDropEvent->drop_SourceList, &sourceEntry->dropse_Node);
                    }
                    else
                    {
#if defined(DEBUG_ILC_ICONDRAGDROP)
                        D(bug("[IconList] %s: Source == Dest, Skipping!\n", __PRETTY_FUNCTION__));
#endif
                        FreeVec(sourceEntry->dropse_Node.ln_Name);
                        FreeMem(sourceEntry, sizeof(struct IconList_Drop_SourceEntry));
                    }
                }
            }
            if (copycount > 0)
            {
                dragDropEvent->drop_TargetObj = (IPTR)obj;

#if defined(DEBUG_ILC_ICONDRAGDROP)
                D(bug("[IconList] %s: Causing DROP notification..\n", __PRETTY_FUNCTION__));
#endif
                SET(obj, MUIA_IconList_IconsDropped, (IPTR)dragDropEvent);
                DoMethod(obj, MUIM_IconList_CoordsSort);
            }
            else
            {
                if (dragDropEvent->drop_TargetPath) FreeVec(dragDropEvent->drop_TargetPath);
                FreeMem(dragDropEvent, sizeof(struct IconList_Drop_Event));
            }
        }
    }
    else
    {
#if defined(DEBUG_ILC_ICONDRAGDROP)
        D(bug("[IconList] %s: BUG - DragDrop recieved with no source icons!\n", __PRETTY_FUNCTION__));
#endif
        NNSET(obj, MUIA_IconList_IconsDropped, (IPTR)NULL);
    }
    
dragdropdone:
    return DoSuperMethodA(CLASS, obj, (Msg)message);
}
///

///MUIM_UnselectAll()
/**************************************************************************
MUIM_UnselectAll
**************************************************************************/
IPTR IconList__MUIM_IconList_UnselectAll(struct IClass *CLASS, Object *obj, Msg message)
{
    struct IconList_DATA 	*data = INST_DATA(CLASS, obj);
    struct Node         	*node = NULL, *next_node = NULL;
	BOOL					changed = FALSE;

#if defined(DEBUG_ILC_FUNCS)
D(bug("[IconList]: %s()\n", __PRETTY_FUNCTION__));
#endif

    data->icld_SelectionLastClicked = NULL;
    data->icld_FocusIcon = NULL;
#ifdef __AROS__
    ForeachNodeSafe(&data->icld_SelectionList, node, next_node)
#else
    Foreach_NodeSafe(&data->icld_SelectionList, node, next_node);
#endif
    {
        struct IconEntry    *entry = (struct IconEntry *)((IPTR)node - ((IPTR)&entry->ie_SelectionNode - (IPTR)entry));
        BOOL                update_icon = FALSE;

        if (entry->ie_Flags & ICONENTRY_FLAG_VISIBLE)
        {
            if (entry->ie_Flags & ICONENTRY_FLAG_SELECTED)
            {
                Remove(node);
                entry->ie_Flags &= ~ICONENTRY_FLAG_SELECTED;
                update_icon = TRUE;
            }
            if (entry->ie_Flags & ICONENTRY_FLAG_FOCUS)
            {
                entry->ie_Flags &= ~ICONENTRY_FLAG_FOCUS;
                update_icon = TRUE;
            }
        }

        if (update_icon)
        {
			changed = TRUE;
            data->icld_UpdateMode = UPDATE_SINGLEICON;
            data->update_icon = entry;
            MUI_Redraw(obj, MADF_DRAWUPDATE);
        }
    }

	if (changed)
		SET(obj, MUIA_IconList_SelectionChanged, TRUE);
	
    return 1;
}
///

///MUIM_SelectAll()
/**************************************************************************
MUIM_SelectAll
**************************************************************************/
IPTR IconList__MUIM_IconList_SelectAll(struct IClass *CLASS, Object *obj, Msg message)
{
    struct IconList_DATA 	*data = INST_DATA(CLASS, obj);
    struct IconEntry    	*node = NULL;
	BOOL					changed = FALSE;

#if defined(DEBUG_ILC_FUNCS)
D(bug("[IconList]: %s()\n", __PRETTY_FUNCTION__));
#endif

    node = (struct IconEntry *)GetHead(&data->icld_IconList);

    while (node != NULL)
    {
        if (node->ie_Flags & ICONENTRY_FLAG_VISIBLE)
        {
            BOOL update_icon = FALSE;

            if (!(node->ie_Flags & ICONENTRY_FLAG_SELECTED))
            {
                AddTail(&data->icld_SelectionList, &node->ie_SelectionNode);
                node->ie_Flags |= ICONENTRY_FLAG_SELECTED;
                update_icon = TRUE;

                data->icld_SelectionLastClicked = node;
            }
            else if (node->ie_Flags & ICONENTRY_FLAG_FOCUS)
            {
                node->ie_Flags &= ~ICONENTRY_FLAG_FOCUS;
                update_icon = TRUE;
            }

            if (update_icon)
            {
                changed = TRUE;
                data->icld_UpdateMode = UPDATE_SINGLEICON;
                data->update_icon = node;
                MUI_Redraw(obj, MADF_DRAWUPDATE);
            }
        }
        node = (struct IconEntry *)GetSucc(&node->ie_IconNode);
    }

    if ((data->icld_SelectionLastClicked) && (data->icld_SelectionLastClicked != data->icld_FocusIcon))
    {
        data->icld_FocusIcon = data->icld_SelectionLastClicked;
        if (!(data->icld_FocusIcon->ie_Flags & ICONENTRY_FLAG_FOCUS))
        {
            data->icld_FocusIcon->ie_Flags &= ~ICONENTRY_FLAG_FOCUS;
            data->icld_FocusIcon->ie_Flags |= ICONENTRY_FLAG_FOCUS;
            data->icld_UpdateMode = UPDATE_SINGLEICON;
            data->update_icon = data->icld_FocusIcon;
            MUI_Redraw(obj, MADF_DRAWUPDATE);
        }
    }

    if (changed)
        SET(obj, MUIA_IconList_SelectionChanged, TRUE);

    return 1;
}
///

///IconList__MUIM_IconList_CoordsSort()
IPTR IconList__MUIM_IconList_CoordsSort(struct IClass *CLASS, Object *obj, struct MUIP_IconList_Sort *message)
{
    struct IconList_DATA *data = INST_DATA(CLASS, obj);

    struct IconEntry    *entry = NULL,
                        *test_icon = NULL;

    struct List         list_VisibleIcons;
    struct List         list_HiddenIcons;

    /*
        perform a quick sort of the iconlist based on icon coords
        this method DOESNT cause any visual output.
    */
#if defined(DEBUG_ILC_FUNCS) && defined(DEBUG_ILC_ICONSORTING)
D(bug("[IconList]: %s()\n", __PRETTY_FUNCTION__));
#endif

    NewList((struct List*)&list_VisibleIcons);
    NewList((struct List*)&list_HiddenIcons);

    /*move list into our local list struct(s)*/
    while ((entry = (struct IconEntry *)RemTail((struct List*)&data->icld_IconList)))
    {
        if (entry->ie_Flags & ICONENTRY_FLAG_VISIBLE)
            AddHead((struct List*)&list_VisibleIcons, (struct Node *)&entry->ie_IconNode);
        else
            AddHead((struct List*)&list_HiddenIcons, (struct Node *)&entry->ie_IconNode);
    }

    while ((entry = (struct IconEntry *)RemTail((struct List*)&list_VisibleIcons)))
    {
        if ((test_icon = (struct IconEntry *)GetTail(&data->icld_IconList)) != NULL)
        {
            while (test_icon != NULL)
            {
                if (((data->icld_DisplayFlags & ICONLIST_DISP_VERTICAL) && (test_icon->ie_IconX > entry->ie_IconX)) ||
                    (!(data->icld_DisplayFlags & ICONLIST_DISP_VERTICAL) && (test_icon->ie_IconY > entry->ie_IconY)))
                {
                    test_icon = (struct IconEntry *)GetPred(&test_icon->ie_IconNode);
                    continue;
                }
                else break;
            }

            while (test_icon != NULL)
            {
                if (((data->icld_DisplayFlags & ICONLIST_DISP_VERTICAL) && (test_icon->ie_IconY > entry->ie_IconY)) ||
                    (!(data->icld_DisplayFlags & ICONLIST_DISP_VERTICAL) && (test_icon->ie_IconX > entry->ie_IconX)))
                {
                    test_icon = (struct IconEntry *)GetPred(&test_icon->ie_IconNode);
                    continue;
                }
                else break;
            }
            Insert((struct List*)&data->icld_IconList, (struct Node *)&entry->ie_IconNode, (struct Node *)&test_icon->ie_IconNode);
        }
        else
            AddTail((struct List*)&data->icld_IconList, (struct Node *)&entry->ie_IconNode);
    }
#if defined(DEBUG_ILC_ICONSORTING)
D(bug("[IconList] %s: Done\n", __PRETTY_FUNCTION__));
#endif

    while ((entry = (struct IconEntry *)RemTail((struct List*)&list_HiddenIcons)))
    {
        AddTail((struct List*)&data->icld_IconList, (struct Node *)&entry->ie_IconNode);
    }

#if defined(DEBUG_ILC_ICONSORTING_DUMP)
    #ifdef __AROS__
    ForeachNode(&data->icld_IconList, entry)
    #else   
    Foreach_Node(&data->icld_IconList, entry);
    #endif
    {
D(bug("[IconList] %s: %d   %d   '%s'\n", __PRETTY_FUNCTION__, entry->ie_IconX, entry->ie_IconY, entry->ie_IconListEntry.label));
    }
#endif

    return TRUE;
}
///

///MUIM_Sort()
/**************************************************************************
MUIM_Sort - sortsort
**************************************************************************/
IPTR IconList__MUIM_IconList_Sort(struct IClass *CLASS, Object *obj, struct MUIP_IconList_Sort *message)
{
    struct IconList_DATA *data = INST_DATA(CLASS, obj);
    struct IconEntry    *entry = NULL,
                        *icon1 = NULL,
                        *icon2 = NULL;
    struct List         list_VisibleIcons,
                        list_SortedIcons,
                        list_HiddenIcons;

    BOOL                sortme, enqueue = FALSE;
    int                 i, visible_count = 0;

#if defined(DEBUG_ILC_FUNCS) && defined(DEBUG_ILC_ICONSORTING)
D(bug("[IconList]: %s()\n", __PRETTY_FUNCTION__));
#endif

    /* Reset incase view options have changed .. */
    data->icld_IconAreaLargestWidth = 0;
    data->icld_IconAreaLargestHeight = 0;
    data->icld_IconLargestHeight = 0;
    data->icld_LabelLargestHeight = 0;

    if ((data->icld_SortFlags & ICONLIST_SORT_MASK) != 0)
    {
#if defined(DEBUG_ILC_ICONSORTING)
D(bug("[IconList] %s: Sorting (Flags %x)\n", __PRETTY_FUNCTION__, (data->icld_SortFlags & ICONLIST_SORT_MASK)));
#endif
        NewList((struct List*)&list_VisibleIcons);
        NewList((struct List*)&list_SortedIcons);
        NewList((struct List*)&list_HiddenIcons);

        /*move list into our local list struct(s)*/
        while ((entry = (struct IconEntry *)RemTail((struct List*)&data->icld_IconList)))
        {
            if (entry->ie_DiskObj)
            {
                if (entry->ie_IconX != entry->ie_DiskObj->do_CurrentX)
                {
                    entry->ie_IconX = entry->ie_DiskObj->do_CurrentX;
                    if ((data->icld_SortFlags & ICONLIST_SORT_MASK) == 0)
                        entry->ie_Flags |= ICONENTRY_FLAG_NEEDSUPDATE;
                }
                if (entry->ie_IconY != entry->ie_DiskObj->do_CurrentY)
                {
                    entry->ie_IconY = entry->ie_DiskObj->do_CurrentY;
                    if ((data->icld_SortFlags & ICONLIST_SORT_MASK) == 0)
                        entry->ie_Flags |= ICONENTRY_FLAG_NEEDSUPDATE;
                }
            }

            if (!(entry->ie_Flags & ICONENTRY_FLAG_HASICON))
            {
                if (data->icld_DisplayFlags & ICONLIST_DISP_SHOWINFO)
                {
                    if (entry->ie_Flags & ICONENTRY_FLAG_VISIBLE)
                    {
                        entry->ie_Flags &= ~(ICONENTRY_FLAG_VISIBLE|ICONENTRY_FLAG_NEEDSUPDATE);
                    }
                }
                else if (!(entry->ie_Flags & ICONENTRY_FLAG_VISIBLE))
                {
                    entry->ie_Flags |= (ICONENTRY_FLAG_VISIBLE|ICONENTRY_FLAG_NEEDSUPDATE);
                }
            }
            else
            {
                if (!(entry->ie_Flags & ICONENTRY_FLAG_VISIBLE))
                {
                    entry->ie_Flags |= (ICONENTRY_FLAG_VISIBLE|ICONENTRY_FLAG_NEEDSUPDATE);
                }
            }

            /* Now we have fixed visibility lets dump them into the correct list for sorting */
            if (entry->ie_Flags & ICONENTRY_FLAG_VISIBLE)
            {
                if(entry->ie_AreaWidth > data->icld_IconAreaLargestWidth) data->icld_IconAreaLargestWidth = entry->ie_AreaWidth;
                if(entry->ie_AreaHeight > data->icld_IconAreaLargestHeight) data->icld_IconAreaLargestHeight = entry->ie_AreaHeight;
                if(entry->ie_IconHeight > data->icld_IconLargestHeight) data->icld_IconLargestHeight = entry->ie_IconHeight;
                if((entry->ie_AreaHeight - entry->ie_IconHeight) > data->icld_LabelLargestHeight) data->icld_LabelLargestHeight = entry->ie_AreaHeight - entry->ie_IconHeight;

                if (((data->icld_SortFlags & ICONLIST_SORT_MASK) == 0) && (entry->ie_IconX == NO_ICON_POSITION))
                    AddTail((struct List*)&list_VisibleIcons, (struct Node *)&entry->ie_IconNode);
                else
                    AddHead((struct List*)&list_VisibleIcons, (struct Node *)&entry->ie_IconNode);
                visible_count++;
            }
            else
            {
                if (entry->ie_Flags & ICONENTRY_FLAG_SELECTED)
                {
                    Remove(&entry->ie_SelectionNode);
                }
                entry->ie_Flags &= ~(ICONENTRY_FLAG_SELECTED|ICONENTRY_FLAG_FOCUS);
                if (data->icld_SelectionLastClicked == entry) data->icld_SelectionLastClicked = NULL;
                if (data->icld_FocusIcon == entry) data->icld_FocusIcon = data->icld_SelectionLastClicked;
                AddHead((struct List*)&list_HiddenIcons, (struct Node *)&entry->ie_IconNode);
            }
        }

        /* Copy each visible icon entry back to the main list, sorting as we go*/

        while ((entry = (struct IconEntry *)RemHead((struct List*)&list_VisibleIcons)))
        {
            icon1 = (struct IconEntry *)GetHead(&list_SortedIcons);
            icon2 = NULL;

            sortme = FALSE;

            if (visible_count > 1)
            {
    //D(bug(" - %s %s %s %i\n",entry->ie_IconListEntry.label,entry->ie_TxtBuf_DATE,entry->ie_TxtBuf_TIME,entry->ie_FileInfoBlock->fib_Size));

                while (icon1)
                {
                    if((icon1->ie_IconListEntry.type == ST_ROOT)||(data->icld_SortFlags & ICONLIST_SORT_DRAWERS_MIXED))
                    {
                        /*volume list or drawers mixed*/
                        sortme = TRUE;
                    }
                    else
                    {
                        /*drawers first*/
                        if ((icon1->ie_IconListEntry.type == ST_USERDIR) && (entry->ie_IconListEntry.type == ST_USERDIR))
                        {
                            sortme = TRUE;
                        }
                        else
                        {
                            if ((icon1->ie_IconListEntry.type != ST_USERDIR) && (entry->ie_IconListEntry.type != ST_USERDIR))
                                sortme = TRUE;
                            else
                            {
                                /* we are the first drawer to arrive or we need to insert ourselves
                                   due to being sorted to the end of the drawers*/

                                if ((!icon2 || icon2->ie_IconListEntry.type == ST_USERDIR) &&
                                    (entry->ie_IconListEntry.type == ST_USERDIR) &&
                                    (icon1->ie_IconListEntry.type != ST_USERDIR))
                                {
    //D(bug("force %s\n",entry->ie_IconListEntry.label));
                                    break;
                                }
                            }
                        }
                    }

                    if (sortme)
                    {
                        i = 0;
                
                        if ((data->icld_SortFlags & ICONLIST_SORT_MASK) == ICONLIST_SORT_BY_DATE)
                        {
                            /* Sort by Date */
                            i = CompareDates((const struct DateStamp *)&entry->ie_FileInfoBlock->fib_Date,(const struct DateStamp *)&icon1->ie_FileInfoBlock->fib_Date);
                        }
                        else if ((data->icld_SortFlags & ICONLIST_SORT_MASK) == ICONLIST_SORT_BY_SIZE)
                        {
                            /* Sort by Size .. */
                            i = entry->ie_FileInfoBlock->fib_Size - icon1->ie_FileInfoBlock->fib_Size;
                        }
                        else if (((data->icld_SortFlags & ICONLIST_SORT_MASK) == ICONLIST_SORT_MASK) && (entry->ie_IconListEntry.type != ST_ROOT))
                        {
                           /* Sort by Type .. */
#warning "TODO: Sort icons based on type using datatypes"
                        }
                        else
                        {
                            if (
                                    ((data->icld_SortFlags & ICONLIST_SORT_MASK) == ICONLIST_SORT_MASK) || 
                                    ((data->icld_SortFlags & ICONLIST_SORT_MASK) == ICONLIST_SORT_BY_NAME) ||
                                    (entry->ie_IconX == NO_ICON_POSITION)
                               )
                            {
                                /* Sort by Name .. */
                                i = Stricmp(entry->ie_IconListEntry.label, icon1->ie_IconListEntry.label);
                                if ((data->icld_SortFlags & ICONLIST_SORT_MASK) == ICONLIST_SORT_MASK)
                                        enqueue = TRUE;
                            }
                            else
                            {
                                /* coord sort */
#warning "TODO: Implement default coord sorting.."
                            }
                        }

                        if (data->icld_SortFlags & ICONLIST_SORT_REVERSE)
                        {
                            if (i > 0)
                                break;
                        }
                        else if	(i < 0)
                            break;
                    }
                    icon2 = icon1;
                    icon1 = (struct IconEntry *)GetSucc(&icon1->ie_IconNode);
                }
            }
            Insert((struct List*)&list_SortedIcons, (struct Node *)&entry->ie_IconNode, (struct Node *)&icon2->ie_IconNode);
        }
	if (enqueue)
	{
            /* Quickly resort based on node priorities .. */
            while ((entry = (struct IconEntry *)RemHead((struct List*)&list_SortedIcons)))
            {
                Enqueue((struct List*)&data->icld_IconList, (struct Node *)&entry->ie_IconNode);
            }
	}
	else
	{
            while ((entry = (struct IconEntry *)RemHead((struct List*)&list_SortedIcons)))
            {
                AddTail((struct List*)&data->icld_IconList, (struct Node *)&entry->ie_IconNode);
            }
	}
    }
    else
    {
#if defined(DEBUG_ILC_ICONSORTING)
D(bug("[IconList] %s: Coord Sorting\n", __PRETTY_FUNCTION__));
#endif
        DoMethod(obj, MUIM_IconList_CoordsSort);
    }

    DoMethod(obj, MUIM_IconList_PositionIcons);
    MUI_Redraw(obj, MADF_DRAWOBJECT);

    if ((data->icld_SortFlags & ICONLIST_SORT_MASK) != 0)
    {
        DoMethod(obj, MUIM_IconList_CoordsSort);

        /* leave hidden icons on a seperate list to speed up normal list parsing ? */
        while ((entry = (struct IconEntry *)RemHead((struct List*)&list_HiddenIcons)))
        {
            AddTail((struct List*)&data->icld_IconList, (struct Node *)&entry->ie_IconNode);
        }
    }
    SET(obj, MUIA_IconList_Changed, TRUE);

    return 1;
}
///

///MUIM_DragReport()
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

#if defined(DEBUG_ILC_FUNCS)
D(bug("[IconList]: %s()\n", __PRETTY_FUNCTION__));
#endif

    l = WhichLayer(&wnd->WScreen->LayerInfo, wnd->LeftEdge + message->x, wnd->TopEdge + message->y);

    if (l != wnd->WLayer) return MUIV_DragReport_Abort;

    return MUIV_DragReport_Continue;
}
///

///MUIM_IconList_UnknownDropDestination()
/**************************************************************************
 MUIM_IconList_UnknownDropDestination
**************************************************************************/

IPTR IconList__MUIM_UnknownDropDestination(struct IClass *CLASS, Object *obj, struct MUIP_UnknownDropDestination *message)
{
#if defined(DEBUG_ILC_FUNCS)
D(bug("[IconList]: %s()\n", __PRETTY_FUNCTION__));
#endif
D(bug("[IconList] %s: icons dropped on custom window \n", __PRETTY_FUNCTION__));

    SET(obj, MUIA_IconList_AppWindowDrop, (IPTR)message); /* Now notify */

    return 0;
}
///

#if WANDERER_BUILTIN_ICONLIST
BOOPSI_DISPATCHER(IPTR,IconList_Dispatcher, CLASS, obj, message)
{
#ifdef __AROS__
    switch (message->MethodID)
#else
    struct IClass *CLASS = cl;
    Msg message = msg;

    switch (msg->MethodID)
#endif
    {
        case OM_NEW:                            return IconList__OM_NEW(CLASS, obj, (struct opSet *)message);
        case OM_DISPOSE:                        return IconList__OM_DISPOSE(CLASS, obj, message);
        case OM_SET:                            return IconList__OM_SET(CLASS, obj, (struct opSet *)message);
        case OM_GET:                            return IconList__OM_GET(CLASS, obj, (struct opGet *)message);
        case OM_ADDMEMBER:                      return IconList__OM_ADDMEMBER(CLASS, obj, (APTR)message);
        case OM_REMMEMBER:                      return IconList__OM_REMMEMBER(CLASS, obj, (APTR)message);

        case MUIM_Setup:                        return IconList__MUIM_Setup(CLASS, obj, (struct MUIP_Setup *)message);
        
        case MUIM_Show:                         return IconList__MUIM_Show(CLASS,obj, (struct MUIP_Show *)message);
        case MUIM_Hide:                         return IconList__MUIM_Hide(CLASS,obj, (struct MUIP_Hide *)message);
        case MUIM_Cleanup:                      return IconList__MUIM_Cleanup(CLASS, obj, (struct MUIP_Cleanup *)message);
        case MUIM_AskMinMax:                    return IconList__MUIM_AskMinMax(CLASS, obj, (struct MUIP_AskMinMax *)message);
        case MUIM_Draw:                         return IconList__MUIM_Draw(CLASS, obj, (struct MUIP_Draw *)message);
#ifdef __AROS__
        case MUIM_Layout:                       return IconList__MUIM_Layout(CLASS, obj, (struct MUIP_Layout *)message);
#endif
        case MUIM_HandleEvent:                  return IconList__MUIM_HandleEvent(CLASS, obj, (struct MUIP_HandleEvent *)message);
        case MUIM_CreateDragImage:              return IconList__MUIM_CreateDragImage(CLASS, obj, (APTR)message);
        case MUIM_DeleteDragImage:              return IconList__MUIM_DeleteDragImage(CLASS, obj, (APTR)message);
        case MUIM_DragQuery:                    return IconList__MUIM_DragQuery(CLASS, obj, (APTR)message);
        case MUIM_DragReport:                   return IconList__MUIM_DragReport(CLASS, obj, (APTR)message);
        case MUIM_DragDrop:                     return IconList__MUIM_DragDrop(CLASS, obj, (APTR)message);
#ifdef __AROS__
        case MUIM_UnknownDropDestination:       return IconList__MUIM_UnknownDropDestination(CLASS, obj, (APTR)message);       
#endif
        case MUIM_IconList_Update:              return IconList__MUIM_IconList_Update(CLASS, obj, (APTR)message);
        case MUIM_IconList_Clear:               return IconList__MUIM_IconList_Clear(CLASS, obj, (APTR)message);
        case MUIM_IconList_RethinkDimensions:   return IconList__MUIM_IconList_RethinkDimensions(CLASS, obj, (APTR)message);
        case MUIM_IconList_CreateEntry:         return IconList__MUIM_IconList_CreateEntry(CLASS, obj, (APTR)message);
        case MUIM_IconList_DestroyEntry:        return IconList__MUIM_IconList_DestroyEntry(CLASS, obj, (APTR)message);
        case MUIM_IconList_DrawEntry:           return IconList__MUIM_IconList_DrawEntry(CLASS, obj, (APTR)message);
        case MUIM_IconList_DrawEntryLabel:      return IconList__MUIM_IconList_DrawEntryLabel(CLASS, obj, (APTR)message);
        case MUIM_IconList_NextIcon:            return IconList__MUIM_IconList_NextIcon(CLASS, obj, (APTR)message);
        case MUIM_IconList_GetIconPrivate:      return IconList__MUIM_IconList_GetIconPrivate(CLASS, obj, (APTR)message);
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

#ifdef __AROS__
/* Class descriptor. */
const struct __MUIBuiltinClass _MUI_IconList_desc = { 
    MUIC_IconList, 
    MUIC_Area, 
    sizeof(struct IconList_DATA), 
    (void*)IconList_Dispatcher
};
#endif
#endif /* WANDERER_BUILTIN_ICONLIST */

#ifndef __AROS__
struct MUI_CustomClass  *initIconListClass(void)
{
  return (struct MUI_CustomClass *) MUI_CreateCustomClass(NULL, MUIC_Area, NULL, sizeof(struct IconList_DATA), ENTRY(IconList_Dispatcher));
}
#endif
