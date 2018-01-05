/*
    Copyright © 2001-2013, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "../portable_macros.h"
#if !defined(__AROS__)
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

#define CREATE_FULL_DRAGIMAGE

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include <dos/dos.h>
#include <dos/datetime.h>
#include <dos/filehandler.h>

#include <exec/memory.h>
#include <exec/rawfmt.h>
#include <graphics/gfx.h>
#include <graphics/view.h>
#include <graphics/rpattr.h>
#include <workbench/icon.h>
#include <workbench/workbench.h>

#if defined(__AROS__)
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

#if defined(__AROS__)
#include <prefs/prefhdr.h>
#include <prefs/wanderer.h>
#else
#include <prefs_AROS/prefhdr.h>
#include <prefs_AROS/wanderer.h>
#endif

#include <proto/cybergraphics.h>

#if defined(__AROS__)
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

#if !defined(__AROS__)
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

static struct Hook      __iconlist_UpdateLabels_hook;

// N.B: We Handle frame/background rendering so make sure icon.library doesnt do it ..
static struct TagItem          __iconList_DrawIconStateTags[] = {
    { ICONDRAWA_Frameless, TRUE         },
    { ICONDRAWA_Borderless, TRUE        },
    { ICONDRAWA_EraseBackground, FALSE  },
    { TAG_DONE,                         }
};

#if 0
static struct TagItem                __iconList_BackBuffLayerTags[] =
{
    { LA_Visible, FALSE                        },
    { TAG_DONE,                                }
};
#endif

#ifndef NO_ICON_POSITION
#define NO_ICON_POSITION                        (0x8000000) /* belongs to workbench/workbench.h */
#endif

#define UPDATE_HEADERENTRY                      1
#define UPDATE_SINGLEENTRY                      2
#define UPDATE_SCROLL                           3
#define UPDATE_RESIZE                           4

#define LEFT_BUTTON                             1
#define RIGHT_BUTTON                            2
#define MIDDLE_BUTTON                           4

#define ICONLIST_DRAWMODE_NORMAL                1
#define ICONLIST_DRAWMODE_FAST                  2

/* Values used for List View-Mode */
#define COLOR_COLUMN_BACKGROUND                 0
#define COLOR_COLUMN_BACKGROUND_SORTED          1
#define COLOR_COLUMN_BACKGROUND_LASSO           2
#define COLOR_COLUMN_BACKGROUND_LASSO_SORTED    3

#define COLOR_SELECTED_BACKGROUND               4
#define COLOR_SELECTED_BACKGROUND_SORTED        5

#define MIN_COLUMN_WIDTH    10

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

#define HEADERLINE_SPACING_TOP          3
#define HEADERLINE_SPACING_BOTTOM       3
#define HEADERLINE_EXTRAHEIGHT          (HEADERLINE_SPACING_TOP + HEADERLINE_SPACING_BOTTOM)

#define HEADERLINE_SPACING_LEFT         1
#define HEADERLINE_SPACING_RIGHT        1
#define HEADERLINE_EXTRAWIDTH           (HEADERLINE_SPACING_LEFT + HEADERLINE_SPACING_RIGHT)

#define HEADERENTRY_SPACING_LEFT        4
#define HEADERENTRY_SPACING_RIGHT       4
#define HEADERENTRY_EXTRAWIDTH          (HEADERENTRY_SPACING_LEFT + HEADERENTRY_SPACING_RIGHT)

enum
{
    INDEX_TYPE,
    INDEX_NAME,
    INDEX_SIZE,
    INDEX_PROTECTION,
    INDEX_LASTACCESS,
    INDEX_COMMENT
};

/**************************************************************************
                         Support Functions
**************************************************************************/

#define ForeachPrevNode(list, node)                        \
for                                                        \
(                                                          \
    node = (void *)(((struct List *)(list))->lh_TailPred); \
    ((struct Node *)(node))->ln_Pred;                      \
    node = (void *)(((struct Node *)(node))->ln_Pred)      \
)

#define RPALPHAFLAT     (1 << 0)
#define RPALPHARADIAL   (1 << 1)

static void RastPortSetAlpha(struct RastPort *arport, ULONG ax, ULONG ay, ULONG width, ULONG height, UBYTE val, UBYTE alphamode)
{
    ULONG       x, y;
    ULONG       alphaval, pixelval;
    APTR        buffer, pixelptr;

    if ((buffer = AllocVec(width * height * sizeof(ULONG), MEMF_ANY)) == NULL)
        return;

    ReadPixelArray(buffer, 0, 0, width * sizeof(ULONG), arport, 0, 0, width, height, RECTFMT_ARGB);

    pixelptr = buffer;

    for (y = 0; y < height; y++)
    {
        for (x = 0; x < width; x++)
        {
            if((pixelval = *((ULONG *)pixelptr)))
            {
                if (alphamode == RPALPHARADIAL){
                    //Set the alpha value based on distance from ax,ay
                } else {
                    alphaval = val;
                }
                pixelval = (pixelval & 0xffffff00) | alphaval;
                *((ULONG *)pixelptr) = pixelval;
            }
            pixelptr += sizeof(ULONG);
        }
    }

    WritePixelArray(buffer, 0, 0, width * sizeof(ULONG), arport, 0, 0, width, height, RECTFMT_ARGB);
    FreeVec(buffer);
}

///RectAndRect()
// Entry/Label Area support functions
static int RectAndRect(struct Rectangle *a, struct Rectangle *b)
{
    if ((a->MinX > b->MaxX) || (a->MinY > b->MaxY) || (a->MaxX < b->MinX) || (a->MaxY < b->MinY))
        return 0;
    return 1;
}
///

///RegionAndRect()
/*
 * offx and offy define the largest jump from b->MinX b->MinY that is safe not to
 * miss a free zone of size b
 */
static int RegionAndRect(struct Region * a, struct Rectangle *b, LONG *offx, LONG *offy)
{
    D(bug("Region (%d, %d)(%d, %d), Rect (%d, %d)(%d, %d)\n",
            (LONG)a->bounds.MinX, (LONG)a->bounds.MinY, (LONG)a->bounds.MaxX, (LONG)a->bounds.MaxY,
            (LONG)b->MinX,        (LONG)b->MinY,        (LONG)b->MaxX,        (LONG)b->MaxY));

    /* First check with region bounds */
    if (RectAndRect(&a->bounds, b) == 0)
        return 0;

    if (a->RegionRectangle)
    {
        struct RegionRectangle * c = a->RegionRectangle;
        while(c)
        {
            struct Rectangle d = {
                    a->bounds.MinX + c->bounds.MinX,
                    a->bounds.MinY + c->bounds.MinY,
                    a->bounds.MinX + c->bounds.MaxX,
                    a->bounds.MinY + c->bounds.MaxY
            }; /* We need absolute coordinates */
            struct Rectangle intersect;

            if (AndRectRect(&d, b, &intersect))
            {
                *offx = (LONG)(intersect.MaxX - intersect.MinX + 1);
                *offy = (LONG)(intersect.MaxY - intersect.MinY + 1);
                return 1;
            }
            c = c->Next;
        }
    }

    return 0;
}

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
    struct IconEntry    *current_Node = (struct IconEntry *)GetHead(icon_list);

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
    struct IconEntry    *current_Node = (struct IconEntry *)GetTail(icon_list);

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
    UQUAD       d;
    const UBYTE       *ch;
    struct
    {
        ULONG    val;
        ULONG    dec;
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

    RawDoFmt(array.dec ? "%lu.%lu" : "%lu", (RAWARG)&array, RAWFMTFUNC_STRING, buf);

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
    WORD        minx = data->icld_LassoRectangle.MinX;
    WORD        miny = data->icld_LassoRectangle.MinY;
    WORD        maxx = data->icld_LassoRectangle.MaxX;
    WORD        maxy = data->icld_LassoRectangle.MaxY;

#if defined(DEBUG_ILC_LASSO) || defined(DEBUG_ILC_FUNCS)
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
    struct Rectangle    r, clipped_r;
    
#if defined(DEBUG_ILC_RENDERING) || defined(DEBUG_ILC_FUNCS)
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
static void IconList_InvertLassoOutlines(Object *obj, struct IconList_DATA *data, struct Rectangle *rect)
{
    struct Rectangle    lasso;
    struct Rectangle    clip;
    
#if defined(DEBUG_ILC_LASSO) || defined(DEBUG_ILC_FUNCS)
    D(bug("[IconList]: %s()\n", __PRETTY_FUNCTION__));
#endif

    /* get abolute iconlist coords */
    lasso.MinX = rect->MinX + _mleft(obj);
    lasso.MaxX = rect->MaxX + _mleft(obj);
    lasso.MinY = rect->MinY + _mtop(obj);
    lasso.MaxY = rect->MaxY + _mtop(obj);
  
    clip.MinX = _mleft(obj);
    clip.MinY = _mtop(obj);
    if ((data->icld_DisplayFlags & ICONLIST_DISP_MODELIST) == ICONLIST_DISP_MODELIST)
    {
        clip.MinY += data->icld_LVMAttribs->lmva_HeaderHeight;
    }
    clip.MaxX = _mright(obj);
    clip.MaxY = _mbottom(obj);
    
    /* horizontal lasso lines */
    IconList_InvertPixelRect(_rp(obj), lasso.MinX + 2, lasso.MinY, lasso.MaxX - 1, lasso.MinY + 1, &clip);
    IconList_InvertPixelRect(_rp(obj), lasso.MinX, lasso.MaxY, lasso.MaxX + 1, lasso.MaxY + 1, &clip);

    /* vertical lasso lines */
    IconList_InvertPixelRect(_rp(obj), lasso.MinX, lasso.MinY, lasso.MinX + 1, lasso.MaxY - 1, &clip);
    IconList_InvertPixelRect(_rp(obj), lasso.MaxX, lasso.MinY, lasso.MaxX + 1, lasso.MaxY - 1, &clip);
} 
///

///IconList_GetIconImageRectangle()
//We don't use icon.library's label drawing so we do this by hand
static void IconList_GetIconImageRectangle(Object *obj, struct IconList_DATA *data, struct IconEntry *entry, struct Rectangle *rect)
{
#if defined(DEBUG_ILC_ICONPOSITIONING) || defined(DEBUG_ILC_FUNCS)
    D(bug("[IconList]: %s(entry @ %p)\n", __PRETTY_FUNCTION__, entry));
#endif

    /* Get basic width/height */    
    GetIconRectangleA(NULL, entry->ie_DiskObj, NULL, rect, __iconList_DrawIconStateTags);
#if defined(DEBUG_ILC_ICONPOSITIONING)
    D(bug("[IconList] %s: MinX %d, MinY %d      MaxX %d, MaxY %d\n", __PRETTY_FUNCTION__, rect->MinX, rect->MinY, rect->MaxX, rect->MaxY));
#endif
    entry->ie_IconWidth  = (rect->MaxX - rect->MinX) + 1;
    entry->ie_IconHeight = (rect->MaxY - rect->MinY) + 1;
    
    if (entry->ie_IconHeight > data->icld_IconLargestHeight)
        data->icld_IconLargestHeight = entry->ie_IconHeight;
}
///

///IconList_GetIconImageOffsets()
static void IconList_GetIconImageOffsets(struct IconList_DATA *data, struct IconEntry *entry, LONG *offsetx, LONG *offsety)
{
    *offsetx = *offsety = 0;
    if (entry->ie_IconWidth < entry->ie_AreaWidth)
         *offsetx += (entry->ie_AreaWidth - entry->ie_IconWidth)/2;

    if ((data->icld__Option_IconListMode == ICON_LISTMODE_GRID) &&
            (entry->ie_AreaWidth < data->icld_IconAreaLargestWidth))
        *offsetx += ((data->icld_IconAreaLargestWidth - entry->ie_AreaWidth)/2);

    if ((data->icld__Option_IconListMode == ICON_LISTMODE_GRID) &&
            (entry->ie_AreaHeight < data->icld_IconAreaLargestHeight))
        *offsety += ((data->icld_IconAreaLargestHeight - entry->ie_AreaHeight)/2);
}

///IconList_GetIconLabelRectangle()
static void IconList_GetIconLabelRectangle(Object *obj, struct IconList_DATA *data, struct IconEntry *entry, struct Rectangle *rect)
{
    ULONG       outline_offset = 0;
    ULONG       textwidth = 0;

#if defined(DEBUG_ILC_ICONPOSITIONING) || defined(DEBUG_ILC_FUNCS)
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
    
    /* Get entry box width including text width */
    if ((entry->ie_IconListEntry.label != NULL) && (entry->ie_TxtBuf_DisplayedLabel != NULL))
    {
        ULONG curlabel_TotalLines;
        SetFont(data->icld_BufferRastPort, data->icld_IconLabelFont);
        
        rect->MinX = 0;
        rect->MaxX = (((data->icld__Option_LabelTextHorizontalPadding + data->icld__Option_LabelTextBorderWidth) * 2) + entry->ie_TxtBuf_DisplayedLabelWidth + outline_offset) - 1;
    
        rect->MinY = 0;

        curlabel_TotalLines = entry->ie_SplitParts;
        if (curlabel_TotalLines == 0)
            curlabel_TotalLines = 1;
        if (curlabel_TotalLines > data->icld__Option_LabelTextMultiLine)
            curlabel_TotalLines = data->icld__Option_LabelTextMultiLine;
        
        rect->MaxY = (((data->icld__Option_LabelTextBorderHeight + data->icld__Option_LabelTextVerticalPadding) * 2) + 
                     ((data->icld_IconLabelFont->tf_YSize + outline_offset) * curlabel_TotalLines)) - 1;

        /*  Date/size sorting has the date/size appended under the entry label
            only list regular files like this (drawers have no size/date output) */
        if(
            entry->ie_IconListEntry.type != ST_USERDIR && 
            ((data->icld_SortFlags & MUIV_IconList_Sort_BySize) || (data->icld_SortFlags & MUIV_IconList_Sort_ByDate))
        )
        {
            SetFont(data->icld_BufferRastPort, data->icld_IconInfoFont);

            if( (data->icld_SortFlags & MUIV_IconList_Sort_BySize) && !(data->icld_SortFlags & MUIV_IconList_Sort_ByDate) )
            {
                entry->ie_TxtBuf_SIZEWidth = TextLength(data->icld_BufferRastPort, entry->ie_TxtBuf_SIZE, strlen(entry->ie_TxtBuf_SIZE));
                textwidth = entry->ie_TxtBuf_SIZEWidth;
            }
            else
            {
                if( !(data->icld_SortFlags & MUIV_IconList_Sort_BySize) && (data->icld_SortFlags & MUIV_IconList_Sort_ByDate) )
                {
                    if( entry->ie_Flags & ICONENTRY_FLAG_TODAY )
                    {
                        entry->ie_TxtBuf_TIMEWidth = TextLength(data->icld_BufferRastPort, entry->ie_TxtBuf_TIME, strlen(entry->ie_TxtBuf_TIME));
                        textwidth = entry->ie_TxtBuf_TIMEWidth;
                    }
                    else
                    {
                        entry->ie_TxtBuf_DATEWidth = TextLength(data->icld_BufferRastPort, entry->ie_TxtBuf_DATE, strlen(entry->ie_TxtBuf_DATE));
                        textwidth = entry->ie_TxtBuf_DATEWidth;
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
static void IconList_GetIconAreaRectangle(Object *obj, struct IconList_DATA *data, struct IconEntry *entry, struct Rectangle *rect)
{
    struct Rectangle    labelrect;
    ULONG               iconlabel_Width;
    ULONG               iconlabel_Height;

#if defined(DEBUG_ILC_ICONPOSITIONING) || defined(DEBUG_ILC_FUNCS)
    D(bug("[IconList]: %s()\n", __PRETTY_FUNCTION__));
#endif

    /* Get entry box width including text width */
    memset(rect, 0, sizeof(struct Rectangle));

    IconList_GetIconImageRectangle(obj, data, entry, rect);

    entry->ie_AreaWidth = entry->ie_IconWidth;
    if (data->icld__Option_IconListMode == ICON_LISTMODE_GRID)
    {
        entry->ie_AreaHeight = data->icld_IconLargestHeight;
    }
    else
    {
        entry->ie_AreaHeight = entry->ie_IconHeight;
    }

    IconList_GetIconLabelRectangle(obj, data, entry, &labelrect);

    iconlabel_Width = ((labelrect.MaxX - labelrect.MinX) + 1);
    iconlabel_Height = ((labelrect.MaxY - labelrect.MinY) + 1);
    
    if (iconlabel_Width > entry->ie_AreaWidth)
        entry->ie_AreaWidth = iconlabel_Width;

    entry->ie_AreaHeight = entry->ie_AreaHeight + data->icld__Option_IconImageSpacing + iconlabel_Height;
    
    /* Store */
    rect->MaxX = (rect->MinX + entry->ie_AreaWidth) - 1;
    rect->MaxY = (rect->MinY + entry->ie_AreaHeight) - 1;
    
    if (entry->ie_AreaWidth > data->icld_IconAreaLargestWidth) data->icld_IconAreaLargestWidth = entry->ie_AreaWidth;
    if (entry->ie_AreaHeight > data->icld_IconAreaLargestHeight) data->icld_IconAreaLargestHeight = entry->ie_AreaHeight;
}
///

static LONG FirstVisibleColumnNumber(struct IconList_DATA *data)
{
    LONG i;
    LONG retval = -1;

    if (data->icld_LVMAttribs != NULL)
    {
        for(i = 0; i < NUM_COLUMNS; i++)
        {
            LONG index = data->icld_LVMAttribs->lmva_ColumnPos[i];

            if (data->icld_LVMAttribs->lmva_ColumnFlags[index] & LVMCF_COLVISIBLE)
            {
                retval = i;
                break;
            }
        }
    }

    return retval;
}

static LONG LastVisibleColumnNumber(struct IconList_DATA *data)
{
    LONG i;
    LONG retval = -1;

    if (data->icld_LVMAttribs != NULL)
    {
        for(i = 0; i < NUM_COLUMNS; i++)
        {
            LONG index = data->icld_LVMAttribs->lmva_ColumnPos[i];

            if (data->icld_LVMAttribs->lmva_ColumnFlags[index] & LVMCF_COLVISIBLE)
            {
                retval = i;
            }
        }
    }

    return retval;
    
}

///NullifyLasso()
// Remove the lasso from the screen
static void NullifyLasso(struct IconList_DATA *data, Object *obj)
{
    /* End Lasso-selection */
    struct Rectangle    old_lasso;
    struct IconEntry    *node = NULL;
    struct Window       *thisWindow = NULL;

    #if defined(DEBUG_ILC_EVENTS) || defined(DEBUG_ILC_LASSO)
                            D(bug("[IconList] %s: Removing Lasso\n", __PRETTY_FUNCTION__));
#endif

    /* Stop handling INTUITICKS */
    GET(obj, MUIA_Window, &thisWindow);
    if (thisWindow)
    {
        ModifyIDCMP(thisWindow, (thisWindow->IDCMPFlags & ~(IDCMP_INTUITICKS)));
        if ((data->ehn.ehn_Events & IDCMP_INTUITICKS))
        {
            DoMethod(_win(obj), MUIM_Window_RemEventHandler, (IPTR)&data->ehn);
            data->ehn.ehn_Events &= ~IDCMP_INTUITICKS;
            DoMethod(_win(obj), MUIM_Window_AddEventHandler, (IPTR)&data->ehn);
        }
    }

    /* Clear Lasso Frame.. */
    GetAbsoluteLassoRect(data, &old_lasso);
    IconList_InvertLassoOutlines(obj, data, &old_lasso);

    data->icld_LassoActive = FALSE;

    /* Remove Lasso flag from affected icons.. */
#if defined(__AROS__)
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
///

static void RenderEntryField(Object *obj, struct IconList_DATA *data,
        struct IconEntry *entry, struct Rectangle *rect, LONG index, BOOL firstvis,
        BOOL lastvis, struct RastPort * rp)
{
    STRPTR text = NULL, renderflag = "<UHOH>";
    struct TextExtent te;
    ULONG fit;

    if (entry->ie_Flags & ICONENTRY_FLAG_SELECTED)
    {
        FillPixelArray(rp,
            rect->MinX, rect->MinY,
            rect->MaxX - rect->MinX + 1, rect->MaxY - rect->MinY,
            0x0A246A);
    }

    rect->MinX += ENTRY_SPACING_LEFT;
    rect->MaxX -= ENTRY_SPACING_RIGHT;
    rect->MinY += LINE_SPACING_TOP;
    rect->MaxY -= LINE_SPACING_BOTTOM;

    if (firstvis) rect->MinX += LINE_SPACING_LEFT;
    if (lastvis)  rect->MaxX -= LINE_SPACING_RIGHT;

    if (!entry) return;

    switch(index)
    {
        case INDEX_TYPE:
            /* Special case !! we draw an image instead .. */
            text = renderflag;
            break;

        case INDEX_NAME:
            text = entry->ie_IconListEntry.label;
            break;

        case INDEX_SIZE:
            text = entry->ie_TxtBuf_SIZE;
            break;

        case INDEX_LASTACCESS:
            text = AllocVec(strlen(entry->ie_TxtBuf_DATE) + strlen(entry->ie_TxtBuf_TIME) + 5, MEMF_CLEAR);
            if (text)
                sprintf(text, "%s at %s", entry->ie_TxtBuf_DATE,
                    entry->ie_TxtBuf_TIME);
            break;

        case INDEX_COMMENT:
            text = entry->ie_FileInfoBlock->fib_Comment;
            break;

        case INDEX_PROTECTION:
            text = entry->ie_TxtBuf_PROT;
            break;                            
    }

    if (!text) return;
    if (!text[0]) return;

    if (text == renderflag)
    {
        if (entry->ie_IconListEntry.type == ST_USERDIR)
        {
            if (data->icld_LVMAttribs->lvma_IconDrawer)
            {
                DrawIconStateA
                  (
                    rp, data->icld_LVMAttribs->lvma_IconDrawer, NULL,
                    rect->MinX + 1, rect->MinY + 1,
                    (entry->ie_Flags & ICONENTRY_FLAG_SELECTED) ? IDS_SELECTED : IDS_NORMAL,
                    __iconList_DrawIconStateTags
                  );
            }
            else
            {
                FillPixelArray(rp,
                    rect->MinX + 1, rect->MinY + 1,
                    rect->MaxX - rect->MinX - 1, rect->MaxY - rect->MinY - 1,
                    0xc0f0f0);
            }
        }
        else
        {
            if (data->icld_LVMAttribs->lvma_IconFile)
            {
                DrawIconStateA
                  (
                    rp, data->icld_LVMAttribs->lvma_IconFile, NULL,
                    rect->MinX + 1, rect->MinY + 1,
                    (entry->ie_Flags & ICONENTRY_FLAG_SELECTED) ? IDS_SELECTED : IDS_NORMAL,
                    __iconList_DrawIconStateTags
                  );
            }
            else
            {
                FillPixelArray(rp,
                    rect->MinX + 1, rect->MinY + 1,
                    rect->MaxX - rect->MinX - 1, rect->MaxY - rect->MinY - 1,
                    0xe0e0e0);
            }
        }
    }
    else
    {
        fit = TextFit(rp, text, strlen(text), &te, NULL, 1,
                       rect->MaxX - rect->MinX + 1,
                       rect->MaxY - rect->MinY + 1);

        if (!fit) return;

        SetABPenDrMd(rp, _pens(obj)[(entry->ie_Flags & ICONENTRY_FLAG_SELECTED) ? MPEN_SHINE : MPEN_TEXT], 0, JAM1);

        if (((rect->MaxY - rect->MinY + 1) - data->icld_IconLabelFont->tf_YSize) > 0)
        {
            rect->MinY += ((rect->MaxY - rect->MinY + 1) - data->icld_IconLabelFont->tf_YSize)/2;
        }
        
        switch(data->icld_LVMAttribs->lmva_ColumnHAlign[index])
        {
            case COLUMN_ALIGN_LEFT:
                Move(rp, rect->MinX, rect->MinY + rp->TxBaseline);
                break;
                
            case COLUMN_ALIGN_RIGHT:
                Move(rp, rect->MaxX - te.te_Width, rect->MinY + rp->TxBaseline);
                break;
                
            case COLUMN_ALIGN_CENTER:
                Move(rp, rect->MinX + (rect->MaxX - rect->MinX + 1 + 1 - te.te_Width) / 2,
                               rect->MinY + rp->TxBaseline);
                break;
                
        }
        Text(rp, text, fit);
    }
    if ((index == INDEX_LASTACCESS) && text)
        FreeVec(text);

}

/**************************************************************************
Draw the entry at its position
**************************************************************************/
///IconList__MUIM_IconList_DrawEntry()
IPTR IconList__MUIM_IconList_DrawEntry(struct IClass *CLASS, Object *obj, struct MUIP_IconList_DrawEntry *message)
{
    struct IconList_DATA        *data = INST_DATA(CLASS, obj);

    BOOL                        outside = FALSE;

    struct Rectangle            iconrect;
    struct Rectangle            objrect;

    ULONG                       objX, objY, objW, objH;
    LONG                        iconX, iconY;

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
    D(bug("[IconList]: %s(message->entry = 0x%p)\n", __PRETTY_FUNCTION__, message->entry));
#endif

    if ((!(message->entry->ie_Flags & ICONENTRY_FLAG_VISIBLE)) ||
        (data->icld_BufferRastPort == NULL) ||
        (!(message->entry->ie_DiskObj)))
    {
#if defined(DEBUG_ILC_ICONRENDERING)
        D(bug("[IconList] %s: Not visible or missing DOB\n", __PRETTY_FUNCTION__));
#endif
        return FALSE;
    }

    /* Set the dimensions of our "view" */
    objrect.MinX = objX;
    objrect.MinY = objY;
    objrect.MaxX = objX + objW - 1;
    objrect.MaxY = objY + objH - 1;

    if ((data->icld_DisplayFlags & ICONLIST_DISP_MODELIST) == ICONLIST_DISP_MODELIST)
    {
        struct Rectangle linerect;
        LONG                 x, i;
        LONG                 firstvis, lastvis;

        linerect.MinX = objX - data->icld_ViewX;
        linerect.MaxX = objX + objW - 1; //linerect.MinX + data->width - 1;
        linerect.MinY = (objY  - data->icld_ViewY) + data->icld_LVMAttribs->lmva_HeaderHeight + (message->drawmode * data->icld_LVMAttribs->lmva_RowHeight);
        linerect.MaxY = linerect.MinY + data->icld_LVMAttribs->lmva_RowHeight - 1;

        if (!AndRectRect(&linerect, &objrect, NULL)) return FALSE;
//        if (!MustRenderRect(data, &linerect)) return;

        SetFont(data->icld_BufferRastPort, data->icld_IconLabelFont);

        x = linerect.MinX + LINE_SPACING_LEFT;

        firstvis = FirstVisibleColumnNumber(data);
        lastvis = LastVisibleColumnNumber(data);

        for(i = 0; i < NUM_COLUMNS; i++)
        {
            struct Rectangle field_rect;
            LONG                      index = data->icld_LVMAttribs->lmva_ColumnPos[i];

            if (!(data->icld_LVMAttribs->lmva_ColumnFlags[i] & LVMCF_COLVISIBLE)) continue;

            field_rect.MinX = (i == firstvis) ? linerect.MinX : x;
            field_rect.MinY = linerect.MinY;
            field_rect.MaxX = x + data->icld_LVMAttribs->lmva_ColumnWidth[index] - 1 + ((i == lastvis) ? LINE_SPACING_RIGHT : 0);
            field_rect.MaxY = linerect.MaxY;
            
/*            if (MustRenderRect(data, &field_rect))
            {*/
                if (AndRectRect(&field_rect, &objrect, NULL))
                {
                    RenderEntryField(obj, data, message->entry, &field_rect, index,
                                     (i == firstvis), (i == lastvis), data->icld_BufferRastPort);
                }
/*            }*/
            x += data->icld_LVMAttribs->lmva_ColumnWidth[index];
        }

        if ((data->icld_LVMAttribs->lvma_Flags & LVMAF_ROWDRAWTOEND) == LVMAF_ROWDRAWTOEND)
        {
            x += LINE_SPACING_RIGHT;

            if (x < linerect.MaxX)
            {
                linerect.MinX = x;

                SetABPenDrMd(data->icld_BufferRastPort, _pens(obj)[MPEN_SHINE], 0, JAM1);
                RectFill(data->icld_BufferRastPort, linerect.MinX, linerect.MinY, linerect.MaxX, linerect.MaxY);
            }
        }
    }
    else
    {
        LONG    offsetx,offsety;

        /* Get the dimensions and affected area of message->entry */
        IconList_GetIconImageRectangle(obj, data, message->entry, &iconrect);

        /* Get offset corrections */
        IconList_GetIconImageOffsets(data, message->entry, &offsetx, &offsety);

        /* Add the relative position offset of the message->entry */
        iconrect.MinX += objX - data->icld_ViewX + message->entry->ie_IconX + offsetx;
        iconrect.MaxX += objX - data->icld_ViewX + message->entry->ie_IconX + offsetx;
        iconrect.MinY += objY - data->icld_ViewY + message->entry->ie_IconY + offsety;
        iconrect.MaxY += objY - data->icld_ViewY + message->entry->ie_IconY + offsety;

        if (!RectAndRect(&iconrect, &objrect))
        {
#if defined(DEBUG_ILC_ICONRENDERING)
        D(bug("[IconList] %s: Entry '%s' image outside of visible area .. skipping\n", __PRETTY_FUNCTION__, message->entry->ie_IconListEntry.label));
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
                if ((outside == TRUE) && RectAndRect(&iconrect, data->update_rect2))
                    outside = FALSE;
            }
            else
            {
                if (!RectAndRect(&iconrect, data->update_rect2))
                    outside = TRUE;
            }
        }

        if (outside == TRUE)
        {
#if defined(DEBUG_ILC_ICONRENDERING)
        D(bug("[IconList] %s: Entry '%s' image outside of update area .. skipping\n", __PRETTY_FUNCTION__, message->entry->ie_IconListEntry.label));
#endif
            return FALSE;
        }

        if (message->drawmode == ICONENTRY_DRAWMODE_NONE) return TRUE;

        // Center entry image
        iconX = iconrect.MinX - objX + data->icld_DrawOffsetX;
        iconY = iconrect.MinY - objY + data->icld_DrawOffsetY;

#if defined(DEBUG_ILC_ICONRENDERING)
    D(bug("[IconList] %s: DrawIconState('%s') .. %d, %d\n", __PRETTY_FUNCTION__, message->entry->ie_IconListEntry.label, iconX, iconY));
#endif
        DrawIconStateA
          (
            data->icld_BufferRastPort, message->entry->ie_DiskObj, NULL,
            iconX, 
            iconY, 
            (message->entry->ie_Flags & ICONENTRY_FLAG_SELECTED) ? IDS_SELECTED : IDS_NORMAL,
            __iconList_DrawIconStateTags
          );
#if defined(DEBUG_ILC_ICONRENDERING)
    D(bug("[IconList] %s: DrawIconState Done\n", __PRETTY_FUNCTION__));
#endif
    }

    return TRUE;
}
///

///IconList__LabelFunc_SplitLabel()
static void IconList__LabelFunc_SplitLabel(Object *obj, struct IconList_DATA *data, struct IconEntry *entry)
{
    ULONG       labelSplit_MaxLabelLineLength = data->icld__Option_LabelTextMaxLen;
    ULONG       labelSplit_LabelLength = strlen(entry->ie_IconListEntry.label);
    ULONG       txwidth;
//    ULONG       labelSplit_FontY = data->icld_IconLabelFont->tf_YSize;
    int         labelSplit_CharsDone,   labelSplit_CharsSplit;
    ULONG       labelSplit_CurSplitWidth;

    if ((data->icld__Option_TrimVolumeNames) && 
         ((entry->ie_IconListEntry.type == ST_ROOT) && (entry->ie_IconListEntry.label[labelSplit_LabelLength - 1] == ':')))
        labelSplit_LabelLength--;

    if (labelSplit_MaxLabelLineLength >= labelSplit_LabelLength)
    {
#if defined(DEBUG_ILC_ICONRENDERING)
        D(bug("[IconList]: %s: Label'%s' doesnt need split (onyl %d chars)\n", __PRETTY_FUNCTION__, entry->ie_IconListEntry.label, labelSplit_LabelLength));
#endif
        return;
    }

    SetFont(data->icld_BufferRastPort, data->icld_IconLabelFont);
    txwidth = TextLength(data->icld_BufferRastPort, entry->ie_IconListEntry.label, labelSplit_MaxLabelLineLength);
#if defined(DEBUG_ILC_ICONRENDERING)
    D(bug("[IconList]: %s: txwidth = %d\n", __PRETTY_FUNCTION__, txwidth));
#endif
    entry->ie_TxtBuf_DisplayedLabel = AllocVecPooled(data->icld_Pool, 256);
    if (entry->ie_TxtBuf_DisplayedLabel != NULL)
        memset(entry->ie_TxtBuf_DisplayedLabel, 0, 256);
    entry->ie_SplitParts = 0;

    labelSplit_CharsDone = 0;
    labelSplit_CharsSplit = 0;
    labelSplit_CurSplitWidth = 0;

    while (labelSplit_CharsDone < labelSplit_LabelLength)
    {
        ULONG   labelSplit_CurSplitLength = labelSplit_LabelLength - labelSplit_CharsDone;
        IPTR    labelSplit_SplitStart = (IPTR)(entry->ie_IconListEntry.label + labelSplit_CharsDone);
        int     tmp_checkoffs = 0;
        IPTR    labelSplit_RemainingCharsAfterSplit;
        IPTR    labelSplit_CurSplitDest;

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

        if (entry->ie_TxtBuf_DisplayedLabel != NULL)
        {
            labelSplit_CurSplitDest = (IPTR)(entry->ie_TxtBuf_DisplayedLabel
                + labelSplit_CharsSplit + entry->ie_SplitParts);

            strncpy((char *)labelSplit_CurSplitDest,
                (char *)labelSplit_SplitStart, labelSplit_CurSplitLength);

            labelSplit_CurSplitWidth = TextLength(data->icld_BufferRastPort,
                (char *)labelSplit_CurSplitDest, labelSplit_CurSplitLength);
        }

        entry->ie_SplitParts = entry->ie_SplitParts + 1;

        labelSplit_CharsDone = labelSplit_CharsDone + labelSplit_CurSplitLength;
        labelSplit_CharsSplit = labelSplit_CharsSplit + labelSplit_CurSplitLength;

        if (labelSplit_CurSplitWidth > entry->ie_TxtBuf_DisplayedLabelWidth) entry->ie_TxtBuf_DisplayedLabelWidth = labelSplit_CurSplitWidth;
    }
    if ((entry->ie_SplitParts <= 1) && entry->ie_TxtBuf_DisplayedLabel)
    {
        FreeVecPooled(data->icld_Pool, entry->ie_TxtBuf_DisplayedLabel);
        entry->ie_TxtBuf_DisplayedLabel = NULL;
        entry->ie_SplitParts = 0;
    }
//  if ((labelSplit_FontY * entry->ie_SplitParts) > data->icld_LabelLargestHeight) data->icld_LabelLargestHeight = (labelSplit_FontY * entry->ie_SplitParts);
}
///

///IconList__LabelFunc_CreateLabel()
static IPTR IconList__LabelFunc_CreateLabel(Object *obj, struct IconList_DATA *data, struct IconEntry *entry)
{
#if defined(DEBUG_ILC_ICONRENDERING) || defined(DEBUG_ILC_FUNCS)
    D(bug("[IconList]: %s('%s')\n", __PRETTY_FUNCTION__, entry->ie_IconListEntry.label));
#endif
    if (entry->ie_TxtBuf_DisplayedLabel)
    {
        FreeVecPooled(data->icld_Pool, entry->ie_TxtBuf_DisplayedLabel);
        entry->ie_TxtBuf_DisplayedLabel = NULL;
        entry->ie_SplitParts = 0;
    }

    if (data->icld__Option_LabelTextMultiLine > 1)
    {
#if defined(DEBUG_ILC_ICONRENDERING)
        D(bug("[IconList]: %s: Attempting to split label ..\n", __PRETTY_FUNCTION__));
#endif
        IconList__LabelFunc_SplitLabel(obj, data, entry);
    }

    if (entry->ie_TxtBuf_DisplayedLabel == NULL)
    { 
        ULONG ie_LabelLength = strlen(entry->ie_IconListEntry.label);
        entry->ie_SplitParts = 1;

#if defined(DEBUG_ILC_ICONRENDERING)
        D(bug("[IconList]: %s: Building unsplit label (len = %d) ..\n", __PRETTY_FUNCTION__, ie_LabelLength));
#endif

        if ((data->icld__Option_TrimVolumeNames) && 
             ((entry->ie_IconListEntry.type == ST_ROOT) && (entry->ie_IconListEntry.label[ie_LabelLength - 1] == ':')))
            ie_LabelLength--;

        if(ie_LabelLength > data->icld__Option_LabelTextMaxLen)
        {
            if (!(entry->ie_TxtBuf_DisplayedLabel = AllocVecPooled(data->icld_Pool, data->icld__Option_LabelTextMaxLen + 1)))
            {
                return (IPTR)NULL;
            }
            memset(entry->ie_TxtBuf_DisplayedLabel, 0, data->icld__Option_LabelTextMaxLen + 1);
            strncpy(entry->ie_TxtBuf_DisplayedLabel, entry->ie_IconListEntry.label, data->icld__Option_LabelTextMaxLen - 3);
            strcat(entry->ie_TxtBuf_DisplayedLabel , " ..");
        }
        else 
        {
            if (!(entry->ie_TxtBuf_DisplayedLabel = AllocVecPooled(data->icld_Pool, ie_LabelLength + 1)))
            {
                return (IPTR)NULL;
            }
            memset(entry->ie_TxtBuf_DisplayedLabel, 0, ie_LabelLength + 1);
            strncpy(entry->ie_TxtBuf_DisplayedLabel, entry->ie_IconListEntry.label, ie_LabelLength );
        }
        entry->ie_TxtBuf_DisplayedLabelWidth = TextLength(data->icld_BufferRastPort, entry->ie_TxtBuf_DisplayedLabel, strlen(entry->ie_TxtBuf_DisplayedLabel));
//    if ((data->icld_IconLabelFont->tf_YSize) > data->icld_LabelLargestHeight) data->icld_LabelLargestHeight = data->icld_IconLabelFont->tf_YSize;
    }

//  if (entry->ie_TxtBuf_DisplayedLabelWidth > data->icld_LabelLargestWidth) data->icld_LabelLargestWidth = entry->ie_TxtBuf_DisplayedLabelWidth;

    return (IPTR)entry->ie_TxtBuf_DisplayedLabel;
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
    Class                       *CLASS = *( Class **)param;
    struct IconList_DATA        *data = INST_DATA(CLASS, obj);

#if defined(DEBUG_ILC_LASSO) || defined(DEBUG_ILC_FUNCS)
    D(bug("[IconList]: %s()\n", __PRETTY_FUNCTION__));
#endif

    if (((data->icld__Option_LabelTextMaxLen != data->icld__Option_LastLabelTextMaxLen) &&
        (data->icld__Option_LabelTextMultiLine > 1)) ||
        (data->icld__Option_LabelTextMultiLine != data->icld__Option_LastLabelTextMultiLine));
    {
        struct IconEntry *iconentry_Current = NULL;
#if defined(__AROS__)
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
    struct IconList_DATA        *data = INST_DATA(CLASS, obj);

    STRPTR                      buf = NULL;
    BOOL                        outside = FALSE;

    struct Rectangle            iconlabelrect;
    struct Rectangle            objrect;

    ULONG                       txtbox_width = 0;
    LONG                        tx,ty,offsetx,offsety;
    LONG                        txwidth; // txheight;

    ULONG                       objX, objY, objW, objH;
    LONG                        labelX, labelY;

    if ((data->icld_DisplayFlags & ICONLIST_DISP_MODELIST) == ICONLIST_DISP_MODELIST)
        return FALSE;

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

#if defined(DEBUG_ILC_ICONRENDERING) || defined(DEBUG_ILC_FUNCS)
    D(bug("[IconList]: %s(message->entry = 0x%p), '%s'\n", __PRETTY_FUNCTION__, message->entry, message->entry->ie_IconListEntry.label));
#endif

    if ((!(message->entry->ie_Flags & ICONENTRY_FLAG_VISIBLE)) ||
        (data->icld_BufferRastPort == NULL) ||
        (!(message->entry->ie_DiskObj)))
    {
#if defined(DEBUG_ILC_ICONRENDERING)
        D(bug("[IconList] %s: Not visible or missing DOB\n", __PRETTY_FUNCTION__));
#endif
        return FALSE;
    }

    /* Get the dimensions and affected area of message->entry's label */
    IconList_GetIconLabelRectangle(obj, data, message->entry, &iconlabelrect);

    /* Add the relative position offset of the message->entry's label */
    offsetx = (objX - data->icld_ViewX) + message->entry->ie_IconX;
    txtbox_width = (iconlabelrect.MaxX - iconlabelrect.MinX) + 1;

    if (txtbox_width < message->entry->ie_AreaWidth)
        offsetx += ((message->entry->ie_AreaWidth - txtbox_width)/2);

    if ((data->icld__Option_IconListMode == ICON_LISTMODE_GRID) &&
        (message->entry->ie_AreaWidth < data->icld_IconAreaLargestWidth))
        offsetx += ((data->icld_IconAreaLargestWidth - message->entry->ie_AreaWidth)/2);

    iconlabelrect.MinX += offsetx;
    iconlabelrect.MaxX += offsetx;

    offsety = (objY - data->icld_ViewY) + message->entry->ie_IconY + data->icld__Option_IconImageSpacing;
    if ((data->icld__Option_IconListMode == ICON_LISTMODE_GRID) &&
        (message->entry->ie_AreaHeight < data->icld_IconAreaLargestHeight))
        offsety += ((data->icld_IconAreaLargestHeight - message->entry->ie_AreaHeight)/2);

    if (data->icld__Option_IconListMode == ICON_LISTMODE_GRID)
    {
        offsety = offsety + data->icld_IconLargestHeight;
    }
    else
    {
        offsety = offsety + message->entry->ie_IconHeight;
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
        (bug("[IconList] %s: Entry '%s' label outside of visible area .. skipping\n", __PRETTY_FUNCTION__, message->entry->ie_IconListEntry.label));
#endif
        return FALSE;
    }

    /* data->update_rect1 and data->update_rect2 may
       point to rectangles to indicate that only icons
       in any of this rectangles need to be drawn      */
    if (data->update_rect1)
    {
        if (!RectAndRect(&iconlabelrect, data->update_rect1))
            outside = TRUE;
    }

    if (data->update_rect2)
    {
        if (data->update_rect1)
        {
            if ((outside == TRUE) && RectAndRect(&iconlabelrect, data->update_rect2))
                outside = FALSE;
        }
        else
        {
            if (!RectAndRect(&iconlabelrect, data->update_rect2))
                outside = TRUE;
        }
    }

    if (outside == TRUE)
    {
#if defined(DEBUG_ILC_ICONRENDERING)
        D(bug("[IconList] %s: Entry '%s' label outside of update area .. skipping\n", __PRETTY_FUNCTION__, message->entry->ie_IconListEntry.label));
#endif
        return FALSE;
    }

    if (message->drawmode == ICONENTRY_DRAWMODE_NONE)
        return TRUE;

    SetABPenDrMd(data->icld_BufferRastPort, _pens(obj)[MPEN_TEXT], 0, JAM1);

    iconlabelrect.MinX = (iconlabelrect.MinX - objX) + data->icld_DrawOffsetX;
    iconlabelrect.MinY = (iconlabelrect.MinY - objY) + data->icld_DrawOffsetY;
    iconlabelrect.MaxX = (iconlabelrect.MaxX - objX) + data->icld_DrawOffsetX;
    iconlabelrect.MaxY = (iconlabelrect.MaxY - objY) + data->icld_DrawOffsetY;

    labelX = iconlabelrect.MinX + data->icld__Option_LabelTextBorderWidth + data->icld__Option_LabelTextHorizontalPadding;
    labelY = iconlabelrect.MinY + data->icld__Option_LabelTextBorderHeight + data->icld__Option_LabelTextVerticalPadding;

    txtarea_width = txtbox_width - ((data->icld__Option_LabelTextBorderWidth + data->icld__Option_LabelTextHorizontalPadding) * 2);

#if defined(DEBUG_ILC_ICONRENDERING)
    D(bug("[IconList] %s: Drawing Label '%s' .. %d, %d\n", __PRETTY_FUNCTION__, message->entry->ie_IconListEntry.label, labelX, labelY));
#endif
    if (message->entry->ie_IconListEntry.label && message->entry->ie_TxtBuf_DisplayedLabel)
    {
        char *curlabel_StrPtr;

        if ((message->entry->ie_Flags & ICONENTRY_FLAG_FOCUS))
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

        curlabel_TotalLines = message->entry->ie_SplitParts;
        curlabel_CurrentLine = 0;

        if (curlabel_TotalLines == 0)
            curlabel_TotalLines = 1;

        if (!(data->icld__Option_LabelTextMultiLineOnFocus) || (data->icld__Option_LabelTextMultiLineOnFocus && (message->entry->ie_Flags & ICONENTRY_FLAG_FOCUS)))
        {
            if (curlabel_TotalLines > data->icld__Option_LabelTextMultiLine)
                curlabel_TotalLines = data->icld__Option_LabelTextMultiLine;
        }
        else
            curlabel_TotalLines = 1;

        curlabel_StrPtr = message->entry->ie_TxtBuf_DisplayedLabel;

        ty = labelY - 1;

#if defined(DEBUG_ILC_ICONRENDERING)
        D(bug("[IconList] %s: Font YSize %d Baseline %d\n", __PRETTY_FUNCTION__,data->icld_IconLabelFont->tf_YSize, data->icld_IconLabelFont->tf_Baseline));
#endif
        for (curlabel_CurrentLine = 0; curlabel_CurrentLine < curlabel_TotalLines; curlabel_CurrentLine++)
        {
            ULONG ie_LabelLength;

            if (curlabel_CurrentLine > 0) curlabel_StrPtr = curlabel_StrPtr + strlen(curlabel_StrPtr) + 1;
            if ((curlabel_CurrentLine >= (curlabel_TotalLines -1)) && (curlabel_TotalLines < message->entry->ie_SplitParts))
            {
                char *tmpLine = curlabel_StrPtr;
                ULONG tmpLen = strlen(tmpLine);

                if ((curlabel_StrPtr = AllocVecPooled(data->icld_Pool, tmpLen + 1)) != NULL)
                {
                    memset(curlabel_StrPtr, 0, tmpLen + 1);
                    strncpy(curlabel_StrPtr, tmpLine, tmpLen - 3);
                    strcat(curlabel_StrPtr , " ..");
                }
                else
                    return FALSE;
                
            }

            ie_LabelLength = strlen(curlabel_StrPtr);
            offset_y = 0;

            // Center message->entry's label
            tx = (labelX + (message->entry->ie_TxtBuf_DisplayedLabelWidth / 2) - (TextLength(data->icld_BufferRastPort, curlabel_StrPtr, strlen(curlabel_StrPtr)) / 2));

            if (message->entry->ie_TxtBuf_DisplayedLabelWidth < txtarea_width)
                tx += ((txtarea_width - message->entry->ie_TxtBuf_DisplayedLabelWidth)/2);

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
            if ((curlabel_CurrentLine >= (curlabel_TotalLines -1)) && (curlabel_TotalLines < message->entry->ie_SplitParts))
            {
                FreeVecPooled(data->icld_Pool, curlabel_StrPtr);
            }
            ty = ty + offset_y;
        }

        /*date/size sorting has the date/size appended under the message->entry label*/

        if ((message->entry->ie_IconListEntry.type != ST_USERDIR) && ((data->icld_SortFlags & (MUIV_IconList_Sort_BySize|MUIV_IconList_Sort_ByDate)) != 0))
        {
            buf = NULL;
            SetFont(data->icld_BufferRastPort, data->icld_IconInfoFont);

            if (data->icld_SortFlags & MUIV_IconList_Sort_BySize)
            {
                buf = message->entry->ie_TxtBuf_SIZE;
                txwidth = message->entry->ie_TxtBuf_SIZEWidth;
            }
            else if (data->icld_SortFlags & MUIV_IconList_Sort_ByDate)
            {
                if (message->entry->ie_Flags & ICONENTRY_FLAG_TODAY)
                {
                    buf  = message->entry->ie_TxtBuf_TIME;
                    txwidth = message->entry->ie_TxtBuf_TIMEWidth;
                }
                else
                {
                    buf = message->entry->ie_TxtBuf_DATE;
                    txwidth = message->entry->ie_TxtBuf_DATEWidth;
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

    return TRUE;
}
///
/**************************************************************************

**************************************************************************/
///IconList__MUIM_IconList_RethinkDimensions()
IPTR IconList__MUIM_IconList_RethinkDimensions(struct IClass *CLASS, Object *obj, struct MUIP_IconList_RethinkDimensions *message)
{
    struct IconList_DATA        *data = INST_DATA(CLASS, obj);

    struct IconEntry            *entry = NULL;
    LONG                        maxx = 0,
                                maxy = 0;
    struct Rectangle            icon_rect;

#if defined(DEBUG_ILC_ICONPOSITIONING) || defined(DEBUG_ILC_FUNCS)
    D(bug("[IconList]: %s()\n", __PRETTY_FUNCTION__));
#endif

    if (message->singleicon != NULL)
    {
        entry = message->singleicon;
        if ((data->icld_DisplayFlags & ICONLIST_DISP_MODELIST) == ICONLIST_DISP_MODELIST)
        {
            maxy = data->icld_LVMAttribs->lmva_RowHeight;
            if ((data->icld_LVMAttribs->lvma_Flags & LVMAF_NOHEADER) == 0)
            {
                maxy += data->icld_LVMAttribs->lmva_HeaderHeight;
            }
        }
        else
        {
            maxx = data->icld_AreaWidth - 1,
            maxy = data->icld_AreaHeight - 1;
        }

#if defined(DEBUG_ILC_ICONPOSITIONING)
        D(bug("[IconList] %s: SingleIcon - maxx = %d, maxy = %d\n", __PRETTY_FUNCTION__, maxx, maxy));
#endif
    }
    else
    {
        if ((data->icld_DisplayFlags & ICONLIST_DISP_MODELIST) == ICONLIST_DISP_MODELIST)
        {
            maxy = data->icld_LVMAttribs->lmva_RowHeight;
            if ((data->icld_LVMAttribs->lvma_Flags & LVMAF_NOHEADER) == 0)
            {
                maxy += data->icld_LVMAttribs->lmva_HeaderHeight;
            }
        }
        entry = (struct IconEntry *)GetHead(&data->icld_IconList);
    }

    while (entry != NULL)
    {
        if (entry->ie_DiskObj &&
            (entry->ie_Flags & ICONENTRY_FLAG_VISIBLE))
        {
            if ((data->icld_DisplayFlags & ICONLIST_DISP_MODELIST) == ICONLIST_DISP_MODELIST)
            {
                maxy += data->icld_LVMAttribs->lmva_RowHeight;
            }
            else
            {
                IconList_GetIconAreaRectangle(obj, data, entry, &icon_rect);

                icon_rect.MaxX += entry->ie_IconX + data->icld__Option_IconHorizontalSpacing;
                if ((data->icld__Option_IconListMode == ICON_LISTMODE_GRID) &&
                    (entry->ie_AreaWidth < data->icld_IconAreaLargestWidth))
                    icon_rect.MaxX += (data->icld_IconAreaLargestWidth - entry->ie_AreaWidth);

                icon_rect.MaxY += entry->ie_IconY + data->icld__Option_IconVerticalSpacing;

                if (icon_rect.MaxX > maxx) maxx = icon_rect.MaxX;
                if (icon_rect.MaxY > maxy) maxy = icon_rect.MaxY;
            }
        }

        if (message->singleicon)
            break;

        entry = (struct IconEntry *)GetSucc(&entry->ie_IconNode);
    }

    if ((data->icld_DisplayFlags & ICONLIST_DISP_MODELIST) == ICONLIST_DISP_MODELIST)
    {
        int col;

        for(col = 0; col < NUM_COLUMNS; col++)
        {
            LONG        index = data->icld_LVMAttribs->lmva_ColumnPos[col];

            if (!(data->icld_LVMAttribs->lmva_ColumnFlags[index] & LVMCF_COLVISIBLE)) continue;

            maxx += data->icld_LVMAttribs->lmva_ColumnWidth[index];
        }
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

/*
 * This function executes the layouting when AutoSort is enabled. This means all icons are layouted regardless if
 * they have Provided position or not.
 */

static VOID IconList_Layout_FullAutoLayout(struct IClass *CLASS, Object *obj)
{
    struct IconList_DATA        *data = INST_DATA(CLASS, obj);
    struct IconEntry            *entry = NULL;
    struct IconEntry            *pass_first = NULL; /* First entry of current column or row */

    LONG                        left = data->icld__Option_IconHorizontalSpacing;
    LONG                        top = data->icld__Option_IconVerticalSpacing;
    LONG                        cur_x = left;
    LONG                        cur_y = top;
    LONG                        gridx = 0;
    LONG                        gridy = 0;
    LONG                        maxw = 0; /* Widest & talest entry in a column or row */
    LONG                        maxh = 0;

    BOOL                        calcnextpos;
    struct Rectangle            iconrect;

    /* Now go to the actual positioning */
    entry = (struct IconEntry *)GetHead(&data->icld_IconList);
    while (entry != NULL)
    {
        calcnextpos = FALSE;
        if ((entry->ie_DiskObj != NULL) && (entry->ie_Flags & ICONENTRY_FLAG_VISIBLE))
        {
            calcnextpos = TRUE;

            /* Set previously calculated position to this icon */
            entry->ie_IconX = cur_x;
            entry->ie_IconY = cur_y;

            if (entry->ie_Flags & ICONENTRY_FLAG_SELECTED)
            {
                if (data->icld_SelectionLastClicked == NULL) data->icld_SelectionLastClicked = entry;
                if (data->icld_FocusIcon == NULL) data->icld_FocusIcon = entry;
            }

            /* Calculate grid size to advanced the coordinate in next step */
            if (data->icld__Option_IconListMode == ICON_LISTMODE_GRID)
            {
                maxw = data->icld_IconAreaLargestWidth + data->icld__Option_IconHorizontalSpacing;
                maxh = data->icld_IconLargestHeight + data->icld__Option_IconImageSpacing + data->icld_LabelLargestHeight + data->icld__Option_IconVerticalSpacing;
                gridx = maxw;
                gridy = maxh;
            }
            else
            {
                if (!(pass_first)) pass_first = entry;

                IconList_GetIconAreaRectangle(obj, data, entry, &iconrect);

                if ((maxw < entry->ie_AreaWidth) || (maxh < entry->ie_AreaHeight))
                {
                    if (maxw < entry->ie_AreaWidth) maxw = entry->ie_AreaWidth;
                    if (maxh < entry->ie_AreaHeight) maxh = entry->ie_AreaHeight;
                    if (pass_first != entry)
                    {
                        entry = pass_first;
                        cur_x = entry->ie_IconX;
                        cur_y = entry->ie_IconY;
                        /* We detected that the new icon it taller/wider than icons so far in this row/column.
                         * We need to re-layout this row/column. */
                        continue;
                    }
                }

                if (data->icld_DisplayFlags & ICONLIST_DISP_VERTICAL)
                {
                    /* Centering */
                    if (entry->ie_AreaWidth < maxw)
                        entry->ie_IconX += ( maxw - entry->ie_AreaWidth ) / 2;

                    gridx = maxw;
                    gridy = entry->ie_AreaHeight + data->icld__Option_IconVerticalSpacing;
                }
                else
                {
                    /* Centering */ /* Icons look better not centered in this case - disabled */
                    /* if (entry->ie_AreaHeight < maxh)
                        entry->ie_IconY += ( maxh - entry->ie_AreaHeight ) / 2; */

                    gridx = entry->ie_AreaWidth + data->icld__Option_IconHorizontalSpacing;
                    gridy = maxh;
                }
            }
        }

        /*
         * Advance to next icon and calculate its position based on grid sizes from previous step.
         * Don't set position - it is done at beginning of the loop.
         */
        if ((entry = (struct IconEntry *)GetSucc(&entry->ie_IconNode)) != NULL)
        {
            if (calcnextpos)
            {
                if (data->icld_DisplayFlags & ICONLIST_DISP_VERTICAL)
                {
                    cur_y += gridy;

                    if ((cur_y >= data->icld_ViewHeight) ||
                        ((data->icld__Option_IconListMode == ICON_LISTMODE_ROUGH) && ((cur_y + entry->ie_AreaHeight - data->icld__Option_IconBorderOverlap) >= data->icld_ViewHeight)) ||
                        ((data->icld__Option_IconListMode == ICON_LISTMODE_GRID) && ((cur_y + data->icld_IconAreaLargestHeight - data->icld__Option_IconBorderOverlap) >= data->icld_ViewHeight)))
                    {
                        /* Wrap "around" if the icon would be below bottom border */
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
                        ((data->icld__Option_IconListMode == ICON_LISTMODE_ROUGH) && ((cur_x + entry->ie_AreaWidth - data->icld__Option_IconBorderOverlap) >= data->icld_ViewWidth)) ||
                        ((data->icld__Option_IconListMode == ICON_LISTMODE_GRID) && ((cur_x + data->icld_IconAreaLargestWidth - data->icld__Option_IconBorderOverlap) >= data->icld_ViewWidth)))
                    {
                        /* Wrap "around" if the icon would be right of right border */
                        cur_x =  left;
                        cur_y += maxh;
                        pass_first = NULL;
                        maxh = 0;
                    }
                }
            }
        }
    }
}

static VOID IconList_Layout_PartialAutoLayout(struct IClass *CLASS, Object *obj)
{
    struct IconList_DATA        *data = INST_DATA(CLASS, obj);
    struct Region               *occupied = NewRegion();
    struct IconEntry            *entry = NULL;
    LONG                        left = data->icld__Option_IconHorizontalSpacing;
    LONG                        top = data->icld__Option_IconVerticalSpacing;
    LONG                        cur_x = left, cur_y  = top;

    entry = (struct IconEntry *)GetHead(&data->icld_IconList);
    while (entry != NULL)
    {
        if ((entry->ie_ProvidedIconX != NO_ICON_POSITION) && (entry->ie_ProvidedIconY != NO_ICON_POSITION)
                && (entry->ie_Flags & ICONENTRY_FLAG_VISIBLE))
        {
            struct Rectangle iconrect = {
                    entry->ie_ProvidedIconX,
                    entry->ie_ProvidedIconY,
                    entry->ie_ProvidedIconX + entry->ie_AreaWidth - 1,
                    entry->ie_ProvidedIconY + entry->ie_AreaHeight - 1
            };

            if (data->icld_DisplayFlags & ICONLIST_DISP_VERTICAL)
                iconrect.MaxY += data->icld__Option_IconVerticalSpacing;
            else
                iconrect.MaxX += data->icld__Option_IconHorizontalSpacing;

            D(bug("Adding %s (%d %d)(%d %d)\n", entry->ie_TxtBuf_DisplayedLabel,
                    (LONG)iconrect.MinX, (LONG)iconrect.MinY, (LONG)iconrect.MaxX, (LONG)iconrect.MaxY));

            OrRectRegion(occupied, &iconrect);
        }
        entry = (struct IconEntry *)GetSucc(&entry->ie_IconNode);
    }

    /* Now go to the actual positioning */
    entry = (struct IconEntry *)GetHead(&data->icld_IconList);
    while (entry != NULL)
    {
        if ((entry->ie_DiskObj != NULL) && (entry->ie_Flags & ICONENTRY_FLAG_VISIBLE))
        {
            if ((entry->ie_ProvidedIconX != NO_ICON_POSITION) && (entry->ie_ProvidedIconY != NO_ICON_POSITION))
            {
                entry->ie_IconX = entry->ie_ProvidedIconX;
                entry->ie_IconY = entry->ie_ProvidedIconY;
            }
            else
            {
                LONG gridx, gridy, stepx = 0, stepy = 0, addx = 0;
                struct Rectangle iconarea;
                BOOL first = TRUE;

                /* Calculate grid size and step */
                if (data->icld__Option_IconListMode == ICON_LISTMODE_GRID)
                {
                    gridx = data->icld_IconAreaLargestWidth + data->icld__Option_IconHorizontalSpacing;
                    gridy = data->icld_IconLargestHeight + data->icld__Option_IconImageSpacing + data->icld_LabelLargestHeight + data->icld__Option_IconVerticalSpacing;
                }
                else
                {
                    if (data->icld_DisplayFlags & ICONLIST_DISP_VERTICAL)
                    {
                        gridx = data->icld_IconAreaLargestWidth; /* This gives better centering effect */
                        gridy = entry->ie_AreaHeight + data->icld__Option_IconVerticalSpacing;
                        addx =  (gridx - entry->ie_AreaWidth) / 2;
                    }
                    else
                    {
                        gridx = entry->ie_AreaWidth + data->icld__Option_IconHorizontalSpacing;
                        gridy = entry->ie_AreaHeight + data->icld__Option_IconVerticalSpacing;
                    }
                }

                /* Find first not occupied spot matching the calculate rectangle */
                do
                {
                    if (data->icld__Option_IconListMode == ICON_LISTMODE_GRID)
                    {
                        /* Overwrite value set via RegionAndRect */
                        stepx = gridx;
                        stepy = gridy;
                    }

                    if (data->icld_DisplayFlags & ICONLIST_DISP_VERTICAL)
                    {
                        /* Advance to next position */
                        if (!first) cur_y += stepy;

                        if ((cur_y >= data->icld_ViewHeight) ||
                                ((cur_y + gridy - data->icld__Option_IconBorderOverlap) >= data->icld_ViewHeight))
                        {
                            /* Wrap "around" if the icon would be below bottom border */
                            cur_x += gridx;
                            cur_y =  top;
                        }
                    }
                    else
                    {
                        /* Advance to next position */
                        if (!first) cur_x += stepx;

                        if ((cur_x >= data->icld_ViewWidth) ||
                            ((cur_x + gridx - data->icld__Option_IconBorderOverlap) >= data->icld_ViewWidth))
                        {
                            /* Wrap "around" if the icon would be right of right border */
                            cur_x =  left;
                            cur_y += gridy;
                        }
                    }

                    iconarea.MinX = cur_x;
                    iconarea.MinY = cur_y;
                    iconarea.MaxX = cur_x + gridx - 1;
                    iconarea.MaxY = cur_y + gridy - 1;

                    first = FALSE;

                } while(RegionAndRect(occupied, &iconarea, &stepx, &stepy));

                entry->ie_IconX = iconarea.MinX + addx;
                entry->ie_IconY = iconarea.MinY;

                /* Add this area to occupied list */
                OrRectRegion(occupied, &iconarea);

                /* Add spacing to next icon */
                if (data->icld_DisplayFlags & ICONLIST_DISP_VERTICAL)
                    cur_y += gridy;
                else
                    cur_x += gridx;
            }
        }

        entry = (struct IconEntry *)GetSucc(&entry->ie_IconNode);
    }

    DisposeRegion(occupied);
}


///IconList__MUIM_IconList_PositionIcons()
/**************************************************************************
MUIM_PositionIcons - Place icons with NO_ICON_POSITION coords somewhere
**************************************************************************/
IPTR IconList__MUIM_IconList_PositionIcons(struct IClass *CLASS, Object *obj, struct MUIP_IconList_PositionIcons *message)
{
    struct IconList_DATA        *data = INST_DATA(CLASS, obj);
    struct IconEntry            *entry = NULL;


#if defined(DEBUG_ILC_ICONPOSITIONING) || defined(DEBUG_ILC_FUNCS)
    D(bug("[IconList]: %s()\n", __PRETTY_FUNCTION__));
#endif
    if ((data->icld_SortFlags & MUIV_IconList_Sort_AutoSort) == 0)
    {
        IconList_Layout_PartialAutoLayout(CLASS, obj);
    }
    else
    {
        IconList_Layout_FullAutoLayout(CLASS, obj);
    }

    /*
     * Set Provided icon position on all icons (this can't be done as part of previous loop!)
     * The icons will not no longer be autolayouted unless MUIV_IconList_Sort_AutoSort is set.
     * This give the stability that new icons appearing won't make existing icons jump from their places
     */
    entry = (struct IconEntry *)GetHead(&data->icld_IconList);
    while (entry != NULL)
    {
        if (entry->ie_Flags & ICONENTRY_FLAG_VISIBLE)
            DoMethod(obj, MUIM_IconList_PropagateEntryPos, entry);

        entry = (struct IconEntry *)GetSucc(&entry->ie_IconNode);
    }


    DoMethod(obj, MUIM_IconList_RethinkDimensions, NULL);
    return (IPTR)NULL;
}
///

///OM_NEW()

#define ICONENTRY_SIZE 16

static inline void CalcHeight(struct ListViewModeAttribs *LVMAttribs, struct TextFont *LabelFont)
{
    ULONG YSize = LabelFont ? LabelFont->tf_YSize : 0;

    LVMAttribs->lmva_HeaderHeight = HEADERLINE_EXTRAHEIGHT + YSize;
    LVMAttribs->lmva_RowHeight = LINE_EXTRAHEIGHT + ((ICONENTRY_SIZE > YSize) ? ICONENTRY_SIZE : YSize);
}

/**************************************************************************
OM_NEW
**************************************************************************/
IPTR IconList__OM_NEW(struct IClass *CLASS, Object *obj, struct opSet *message)
{
    struct IconList_DATA        *data = NULL;
    struct TextFont             *icl_WindowFont = NULL;
//    struct RastPort             *icl_RastPort = NULL;
    int i;

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

/* Setup List View-Mode options */
    if ((data->icld_LVMAttribs = AllocMem(sizeof(struct ListViewModeAttribs), MEMF_CLEAR)) != NULL)
    {
        for(i = 0; i < NUM_COLUMNS; i++)
        {
            data->icld_LVMAttribs->lmva_ColumnPos[i] = i;
            data->icld_LVMAttribs->lmva_ColumnFlags[i] = LVMCF_COLVISIBLE;
            data->icld_LVMAttribs->lmva_ColumnWidth[i] = 100;
            data->icld_LVMAttribs->lmva_ColumnHAlign[i] = COLUMN_ALIGN_LEFT;
            switch (i)
            {
                case INDEX_TYPE:
                    data->icld_LVMAttribs->lmva_ColumnHAlign[i] = COLUMN_ALIGN_RIGHT;
                    data->icld_LVMAttribs->lmva_ColumnFlags[i] |= (LVMCF_COLCLICKABLE|LVMCF_COLSORTABLE);
                    data->icld_LVMAttribs->lmva_ColumnTitle[i] = "Type";
                    data->icld_LVMAttribs->lmva_ColumnWidth[i] = ICONENTRY_SIZE + 2;
                    break;

                case INDEX_NAME:
                    data->icld_LVMAttribs->lmva_ColumnFlags[i] |= (LVMCF_COLCLICKABLE|LVMCF_COLSORTABLE);
                    data->icld_LVMAttribs->lmva_ColumnTitle[i] = "Name";
                    break;

                case INDEX_SIZE:
                    data->icld_LVMAttribs->lmva_ColumnHAlign[i] = COLUMN_ALIGN_RIGHT;
                    data->icld_LVMAttribs->lmva_ColumnFlags[i] |= LVMCF_COLSORTABLE;
                    data->icld_LVMAttribs->lmva_ColumnTitle[i] = "Size";
                    break;

                case INDEX_LASTACCESS:
                    data->icld_LVMAttribs->lmva_ColumnFlags[i] |= LVMCF_COLSORTABLE;
                    data->icld_LVMAttribs->lmva_ColumnTitle[i] = "Last Accessed";
                    break;

                case INDEX_COMMENT:
                    data->icld_LVMAttribs->lmva_ColumnTitle[i] = "Comment";
                    break;

                case INDEX_PROTECTION:
                    data->icld_LVMAttribs->lmva_ColumnTitle[i] = "Protection";
                    break;

                default:
                    data->icld_LVMAttribs->lmva_ColumnTitle[i] = "<Unknown>";
                    break;
            }
        }
        data->icld_LVMAttribs->lmva_LastSelectedColumn = -1;
        data->icld_LVMAttribs->lmva_SortColumn = INDEX_NAME;
        data->icld_LVMAttribs->lvma_Flags = LVMAF_HEADERDRAWTOEND;
/*
 * Seems to be not needed because it's done in MUIM_Setup. No rendering happens before it.
 * Height calculation moved to MUIM_Setup because font pointer can be NULL here (if user-specified
 * font failed to open). In this case we fail back to the font specified in MUI's AreaData, but
 * it becomes known only in MUIM_Setup
 *
         CalcHeight(data->icld_LVMAttribs, data->icld_IconLabelFont); */
    }

    /* Get/Set initial values */
    /* TODO: TrimVolumeNames should be prefs settable */
    data->icld__Option_TrimVolumeNames = TRUE;
    /* TODO: Adjust overlap by window border width */
    data->icld__Option_IconBorderOverlap = 10;

    data->icld__Option_IconListMode   = (UBYTE)GetTagData(MUIA_IconList_IconListMode, 0, message->ops_AttrList);
    data->icld__Option_LabelTextMode   = (UBYTE)GetTagData(MUIA_IconList_LabelText_Mode, 0, message->ops_AttrList);
    data->icld__Option_LabelTextMaxLen = (ULONG)GetTagData(MUIA_IconList_LabelText_MaxLineLen, ILC_ICONLABEL_MAXLINELEN_DEFAULT, message->ops_AttrList);
    data->icld__Option_DragImageTransparent = (BOOL)GetTagData(MUIA_IconList_DragImageTransparent, FALSE, message->ops_AttrList);

    if ( data->icld__Option_LabelTextMaxLen < ILC_ICONLABEL_SHORTEST )
        data->icld__Option_LabelTextMaxLen = ILC_ICONLABEL_MAXLINELEN_DEFAULT;

    data->icld__Option_LastLabelTextMaxLen = data->icld__Option_LabelTextMaxLen;

#if defined(DEBUG_ILC_FUNCS)
    D(bug("[IconList] %s: MaxLineLen : %ld\n", __PRETTY_FUNCTION__, data->icld__Option_LabelTextMaxLen));
#endif
    data->ehn.ehn_Events   = IDCMP_MOUSEBUTTONS | IDCMP_RAWKEY | IDCMP_NEWSIZE | IDCMP_MENUVERIFY;
    data->ehn.ehn_Priority = 0;
    data->ehn.ehn_Flags    = 0;
    data->ehn.ehn_Object   = obj;
    data->ehn.ehn_Class    = CLASS;

    data->icld_SortFlags    = MUIV_IconList_Sort_ByName;
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
    struct IconList_DATA        *data = INST_DATA(CLASS, obj);
    struct IconEntry            *node = NULL;

#if defined(DEBUG_ILC_FUNCS)
    D(bug("[IconList]: %s()\n", __PRETTY_FUNCTION__));
#endif

#if defined(__AROS__)
    ForeachNode(&data->icld_IconList, node)
#else
    Foreach_Node(&data->icld_IconList, node);
#endif
    {
        if (node->ie_DiskObj)
            FreeDiskObject(node->ie_DiskObj);
    }

    if (data->icld_Pool) DeletePool(data->icld_Pool);

    DoSuperMethodA(CLASS,obj,message);
    return (IPTR)NULL;
}
///

///OM_SET()
/**************************************************************************
OM_SET
**************************************************************************/
IPTR IconList__OM_SET(struct IClass *CLASS, Object *obj, struct opSet *message)
{
    struct IconList_DATA        *data = INST_DATA(CLASS, obj);
    struct TagItem              *tag = NULL,
                                *tags = NULL;

    WORD                        oldleft = data->icld_ViewX,
                                oldtop = data->icld_ViewY;
                                //oldwidth = data->icld_ViewWidth,
                                //oldheight = data->icld_ViewHeight;

#if defined(DEBUG_ILC_FUNCS) || defined(DEBUG_ILC_ATTRIBS)
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
                /* FIXME: Should we call CalcHeight() here because our font changed? */
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
                    // ULONG origModeFlags = data->icld_DisplayFlags & (ICONLIST_DISP_MODEDEFAULT|ICONLIST_DISP_MODELABELRIGHT|ICONLIST_DISP_MODELIST);
                    data->icld_DisplayFlags = (ULONG)tag->ti_Data;

                    if (data->icld_DisplayFlags & ICONLIST_DISP_BUFFERED)
                    {
#if defined(DEBUG_ILC_ATTRIBS) || defined(DEBUG_ILC_ICONRENDERING)
                        D(bug("[IconList] %s: MUIA_IconList_DisplayFlags & ICONLIST_DISP_BUFFERED\n", __PRETTY_FUNCTION__));
#endif
                        if ((data->icld_BufferRastPort != NULL)
                            && (data->icld_BufferRastPort != data->icld_DisplayRastPort))
                        {
#if defined(DEBUG_ILC_ATTRIBS) || defined(DEBUG_ILC_ICONRENDERING)
                            D(bug("[IconList] %s: BackLayer @ %p for BackRastport @ %p\n", __PRETTY_FUNCTION__, data->icld_BufferRastPort->Layer, data->icld_BufferRastPort));
#endif
                            if ((GetBitMapAttr(data->icld_BufferRastPort->BitMap, BMA_WIDTH) != data->icld_ViewWidth)
                                || (GetBitMapAttr(data->icld_BufferRastPort->BitMap, BMA_HEIGHT) != data->icld_ViewHeight))
                            {
                                struct Layer *oldLayer = data->icld_BufferRastPort->Layer;
#if defined(DEBUG_ILC_ATTRIBS) || defined(DEBUG_ILC_ICONRENDERING)
                                D(bug("[IconList] %s: Destroying old BackLayer\n", __PRETTY_FUNCTION__));
#endif
                                data->icld_BufferRastPort = data->icld_DisplayRastPort;
                                DeleteLayer(0, oldLayer);
                            }
                        }

                        if ((data->icld_BufferRastPort == NULL) || (data->icld_BufferRastPort == data->icld_DisplayRastPort))
                        {
                            struct BitMap *bitmap_New = NULL;
                            ULONG tmp_RastDepth;
                            struct Layer_Info *li = NULL;

                            tmp_RastDepth = GetCyberMapAttr(data->icld_DisplayRastPort->BitMap, CYBRMATTR_DEPTH);
                            if ((bitmap_New = AllocBitMap(data->icld_ViewWidth,
                                                data->icld_ViewHeight,
                                                tmp_RastDepth,
                                                BMF_CLEAR,
                                                data->icld_DisplayRastPort->BitMap))!=NULL)
                            {
                                if ((data->icld_BufferRastPort = CreateRastPort())!=NULL)
                                {
                                    data->icld_BufferRastPort->BitMap = bitmap_New;
                                    if ((li = NewLayerInfo()))
                                    {
                                        if ((data->icld_BufferRastPort->Layer = CreateUpfrontLayer(li, data->icld_BufferRastPort->BitMap, 0, 0, data->icld_ViewWidth - 1, data->icld_ViewHeight - 1, 0, NULL)))
                                        {
                                           /*
                                            * Mark it as a buffered rastport.
                                            */

#if defined(DEBUG_ILC_ATTRIBS) || defined(DEBUG_ILC_ICONRENDERING)
                                            D(bug("[IconList] %s: FrontRastPort @ %p, New BackLayer @ %p, BackRastport @ %p\n", __PRETTY_FUNCTION__, data->icld_DisplayRastPort, data->icld_BufferRastPort->Layer, data->icld_BufferRastPort));
#endif
                                            SET(obj, MUIA_IconList_BufferRastport, data->icld_BufferRastPort);
                                            data->icld_DrawOffsetX = 0;
                                            data->icld_DrawOffsetY = 0;
                                        }
                                        else
                                            data->icld_BufferRastPort = data->icld_DisplayRastPort;
                                    }
                                    else
                                        data->icld_BufferRastPort = data->icld_DisplayRastPort;
                                }
                                else
                                    data->icld_BufferRastPort = data->icld_DisplayRastPort;
                            }
                            if (data->icld_BufferRastPort == data->icld_DisplayRastPort)
                            {
                                if (bitmap_New) FreeBitMap(bitmap_New);
                                if (li) DisposeLayerInfo(li);
                                data->icld_DrawOffsetX = _mleft(obj);
                                data->icld_DrawOffsetY = _mtop(obj);
                            }
                        }
                    }
                    else
                    {
                        if ((data->icld_BufferRastPort) && (data->icld_BufferRastPort != data->icld_DisplayRastPort))
                        {
                            //Free up the buffers layer, rastport and bitmap since they are no longer needed ..
                            struct Layer *oldLayer = data->icld_BufferRastPort->Layer;
                            data->icld_BufferRastPort = data->icld_DisplayRastPort;
                            InstallClipRegion(oldLayer, NULL);
                            DeleteLayer(0, oldLayer);
                            FreeBitMap(data->icld_BufferRastPort->BitMap);
                            data->icld_DrawOffsetX = _mleft(obj);
                            data->icld_DrawOffsetY = _mtop(obj);
                        }
                    }
                    SET(obj, MUIA_IconList_Changed, TRUE);
                }
                break;

            case MUIA_IconList_SortFlags:
#if defined(DEBUG_ILC_ATTRIBS)
                D(bug("[IconList] %s: MUIA_IconList_SortFlags %08x\n", __PRETTY_FUNCTION__, tag->ti_Data));
#endif
                data->icld_SortFlags = (ULONG)tag->ti_Data;
                break;

            case MUIA_IconList_DragImageTransparent:
                data->icld__Option_DragImageTransparent = (BOOL)tag->ti_Data;
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
#if defined(DEBUG_ILC_ATTRIBS) || defined(DEBUG_ILC_ICONRENDERING)
                D(bug("[IconList] %s: MUIA_Background\n", __PRETTY_FUNCTION__));
#endif
                {
                    char *bgmode_string = (char *)tag->ti_Data;
                    BYTE this_mode = bgmode_string[0] - 48;

#if defined(DEBUG_ILC_ATTRIBS) || defined(DEBUG_ILC_ICONRENDERING)
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
#define STORE                   *(message->opg_Storage)
    struct IconList_DATA        *data = INST_DATA(CLASS, obj);

#if defined(DEBUG_ILC_FUNCS) || defined(DEBUG_ILC_ATTRIBS)
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
        case MUIA_IconList_DragImageTransparent:        STORE = (IPTR)data->icld__Option_DragImageTransparent; return 1;

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
        case MUIA_Family_List:                          STORE = (IPTR)&(data->icld_IconList); return 1; /* Get our list object */

        /* TODO: Get the version/revision from our config.. */
        case MUIA_Version:                              STORE = (IPTR)1; return 1;
        case MUIA_Revision:                             STORE = (IPTR)7; return 1;
    }

    return DoSuperMethodA(CLASS, obj, (Msg) message);
#undef STORE
}
///

IPTR IconList__MUIM_Family_AddHead(struct IClass *CLASS, Object *obj, struct MUIP_Family_AddHead *message)
{
    struct IconList_DATA        *data = INST_DATA(CLASS, obj);
#if defined(DEBUG_ILC_FUNCS)
    D(bug("[IconList]: %s()\n", __PRETTY_FUNCTION__));
#endif

    if (message->obj)
    {
        /* TODO: Use the correct _OBJECT() code when we switch to icon.mui */
//        AddHead(&(data->icld_IconList), (struct Node *)_OBJECT(message->obj));
        AddHead(&(data->icld_IconList), (struct Node *)message->obj);
        return TRUE;
    }
    else
        return FALSE;
}

IPTR IconList__MUIM_Family_AddTail(struct IClass *CLASS, Object *obj, struct MUIP_Family_AddTail *message)
{
    struct IconList_DATA        *data = INST_DATA(CLASS, obj);
#if defined(DEBUG_ILC_FUNCS)
    D(bug("[IconList]: %s()\n", __PRETTY_FUNCTION__));
#endif

    D(bug("[IconList] %s: list @ 0x%p, entry @ 0x%p '%s'\n", __PRETTY_FUNCTION__, &(data->icld_IconList), message->obj, ((struct IconEntry *)message->obj)->ie_IconNode.ln_Name));
    
    if (message->obj)
    {
        /* TODO: Use the correct _OBJECT() code when we switch to icon.mui */
//        AddTail(&(data->icld_IconList), (struct Node *)_OBJECT(message->obj));
        AddTail(&(data->icld_IconList), (struct Node *)message->obj);
        return TRUE;
    }
    else
        return FALSE;

    return (IPTR)NULL;
}
#if !defined(WANDERER_BUILTIN_ICONLIST)
IPTR IconList__OM_ADDMEMBER(struct IClass *CLASS, Object *obj, struct MUIP_Family_AddTail *message)
{
    return IconList__MUIM_Family_AddTail(CLASS, obj, message);
}
#endif

IPTR IconList__MUIM_Family_Remove(struct IClass *CLASS, Object *obj, struct MUIP_Family_Remove *message)
{
    // struct IconList_DATA        *data = INST_DATA(CLASS, obj);
#if defined(DEBUG_ILC_FUNCS)
    D(bug("[IconList]: %s()\n", __PRETTY_FUNCTION__));
#endif

    D(bug("[IconList] %s: entry @ 0x%p '%s'\n", __PRETTY_FUNCTION__, message->obj, ((struct IconEntry *)message->obj)->ie_IconNode.ln_Name));

    if (message->obj)
    {
        /* TODO: Use the correct _OBJECT() code when we switch to icon.mui */
//        Remove((struct Node *)_OBJECT(message->obj));
        Remove((struct Node *)message->obj);
        return TRUE;
    }
    else
        return FALSE;

    return (IPTR)NULL;
}
#if !defined(WANDERER_BUILTIN_ICONLIST)
IPTR IconList__OM_REMMEMBER(struct IClass *CLASS, Object *obj, struct MUIP_Family_Remove *message)
{
    return IconList__MUIM_Family_Remove(CLASS, obj, message);
}
#endif

///MUIM_Setup()
/**************************************************************************
MUIM_Setup
**************************************************************************/
IPTR IconList__MUIM_Setup(struct IClass *CLASS, Object *obj, struct MUIP_Setup *message)
{
    struct IconList_DATA        *data = INST_DATA(CLASS, obj);
    struct IconEntry            *node = NULL;
    IPTR                        geticon_error = 0, iconlistScreen;

#if defined(DEBUG_ILC_FUNCS)
    D(bug("[IconList]: %s()\n", __PRETTY_FUNCTION__));
#endif

    if (!DoSuperMethodA(CLASS, obj, (Msg) message)) return (IPTR)NULL;

    iconlistScreen = (IPTR)_screen(obj);
#if defined(DEBUG_ILC_ICONRENDERING)
    D(bug("[IconList] %s: IconList Screen @ 0x%p)\n", __PRETTY_FUNCTION__, iconlistScreen));
#endif

    DoMethod(_win(obj), MUIM_Window_AddEventHandler, (IPTR)&data->ehn);

    /* Get Internal Objects to use if not set .. */
    data->icld_DisplayRastPort = NULL;
    data->icld_BufferRastPort = NULL;

    if (data->icld_IconLabelFont == NULL)   data->icld_IconLabelFont = _font(obj);
    if (data->icld_IconInfoFont == NULL)    data->icld_IconInfoFont = data->icld_IconLabelFont;

    /*
     * Here we have our font, either from user preferences or from MUI's AreaData.
     * It's right time to set up some sizes.
     */
    if (data->icld_LVMAttribs)
        CalcHeight(data->icld_LVMAttribs, data->icld_IconLabelFont);
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

    if (data->icld_LVMAttribs)
    {
        data->icld_LVMAttribs->lvma_IconDrawer = GetIconTags
        (
            "WANDERER:Icons/drawer",
            (iconlistScreen) ? ICONGETA_Screen : TAG_IGNORE, iconlistScreen,
            (iconlistScreen) ? ICONGETA_RemapIcon : TAG_IGNORE, TRUE,
            ICONGETA_FailIfUnavailable,        TRUE,
            ICONGETA_GenerateImageMasks,       TRUE,
            ICONA_ErrorCode,                   &geticon_error,
            TAG_DONE
        );

#if defined(DEBUG_ILC_ICONRENDERING)
        if (data->icld_LVMAttribs->lvma_IconDrawer == NULL)
        {
            D(bug("[IconList] %s: Couldnt get drawer DiskObject! (error code = 0x%p)\n", __PRETTY_FUNCTION__, geticon_error));
        }
#endif
        data->icld_LVMAttribs->lvma_IconFile = GetIconTags
        (
            "WANDERER:Icons/file",
            (iconlistScreen) ? ICONGETA_Screen : TAG_IGNORE, iconlistScreen,
            (iconlistScreen) ? ICONGETA_RemapIcon : TAG_IGNORE, TRUE,
            ICONGETA_FailIfUnavailable,        TRUE,
            ICONGETA_GenerateImageMasks,       TRUE,
            ICONA_ErrorCode,                   &geticon_error,
            TAG_DONE
        );

#if defined(DEBUG_ILC_ICONRENDERING)
        if (data->icld_LVMAttribs->lvma_IconFile == NULL)
        {
            D(bug("[IconList] %s: Couldnt get file DiskObject! (error code = 0x%p)\n", __PRETTY_FUNCTION__, geticon_error));
        }
#endif
    }

#if defined(__AROS__)
    ForeachNode(&data->icld_IconList, node)
#else
    Foreach_Node(&data->icld_IconList, node);
#endif
    {
        if (!node->ie_DiskObj)
        {
            if (!(node->ie_DiskObj = GetIconTags(node->ie_IconNode.ln_Name,
                                                (iconlistScreen) ? ICONGETA_Screen : TAG_IGNORE, iconlistScreen,
                                                (iconlistScreen) ? ICONGETA_RemapIcon : TAG_IGNORE, TRUE,
                                                ICONGETA_GenerateImageMasks, TRUE,
                                                ICONGETA_FailIfUnavailable, FALSE,
                                                ICONA_ErrorCode, &geticon_error,
                                                TAG_DONE)))
            {
#if defined(DEBUG_ILC_ICONRENDERING)
                D(bug("[IconList] %s: Failed to obtain Entry '%s's diskobj! (error code = 0x%p)\n", __PRETTY_FUNCTION__, node->ie_IconNode.ln_Name, geticon_error));
#endif
                /*  We should probably remove this node if the entry cant be obtained ? */
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
    struct IconList_DATA        *data = INST_DATA(CLASS, obj);
    LONG                        newleft,
                                newtop;
    IPTR                        rc;

#if defined(DEBUG_ILC_FUNCS)
D(bug("[IconList]: %s()\n", __PRETTY_FUNCTION__));
#endif

    if ((rc = DoSuperMethodA(CLASS, obj, (Msg)message)))
    {
        newleft = data->icld_ViewX;
        newtop = data->icld_ViewY;

        if (newleft + _mwidth(obj) > data->icld_AreaWidth)
            newleft = data->icld_AreaWidth - _mwidth(obj);
        if (newleft < 0)
            newleft = 0;

        if (newtop + _mheight(obj) > data->icld_AreaHeight)
            newtop = data->icld_AreaHeight - _mheight(obj);
        if (newtop < 0)
            newtop = 0;

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
#if defined(DEBUG_ILC_ICONRENDERING)
            else
            {
                D(bug("[IconList] IconList__MUIM_Show: ERROR - NULL RastPort!\n"));
            }
#endif
        }

        if (data->icld_DisplayFlags & ICONLIST_DISP_BUFFERED)
        {
            struct BitMap *bitmap_New = NULL;
            struct Layer_Info *li = NULL;

            ULONG tmp_RastDepth = GetCyberMapAttr(data->icld_DisplayRastPort->BitMap, CYBRMATTR_DEPTH);
            if ((bitmap_New = AllocBitMap(data->icld_ViewWidth,
                                data->icld_ViewHeight,
                                tmp_RastDepth,
                                BMF_CLEAR,
                                data->icld_DisplayRastPort->BitMap))!=NULL)
            {
                if ((data->icld_BufferRastPort = CreateRastPort())!=NULL)
                {
                    data->icld_BufferRastPort->BitMap = bitmap_New;
                    if ((li = NewLayerInfo()))
                    {
                        if ((data->icld_BufferRastPort->Layer = CreateUpfrontLayer(li, data->icld_BufferRastPort->BitMap, 0, 0, data->icld_ViewWidth - 1, data->icld_ViewHeight - 1, 0, NULL)))
                        {
                           /*
                            * Mark it as a buffered rastport.
                            */

#if defined(DEBUG_ILC_ATTRIBS) || defined(DEBUG_ILC_ICONRENDERING)
                            D(bug("[IconList] %s: FrontRastPort @ %p, New BackLayer @ %p, BackRastport @ %p\n", __PRETTY_FUNCTION__, data->icld_DisplayRastPort, data->icld_BufferRastPort->Layer, data->icld_BufferRastPort));
#endif
                            SET(obj, MUIA_IconList_BufferRastport, data->icld_BufferRastPort);
                            data->icld_DrawOffsetX = 0;
                            data->icld_DrawOffsetY = 0;
                        }
                        else
                            data->icld_BufferRastPort = data->icld_DisplayRastPort;
                    }
                    else
                        data->icld_BufferRastPort = data->icld_DisplayRastPort;
                }
                else
                    data->icld_BufferRastPort = data->icld_DisplayRastPort;
            }
            if (data->icld_BufferRastPort == data->icld_DisplayRastPort)
            {
                if (bitmap_New) FreeBitMap(bitmap_New);
                if (li) DisposeLayerInfo(li);
                data->icld_DrawOffsetX = _mleft(obj);
                data->icld_DrawOffsetY = _mtop(obj);
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
    struct IconList_DATA        *data = INST_DATA(CLASS, obj);
    IPTR                        rc;

#if defined(DEBUG_ILC_FUNCS)
D(bug("[IconList]: %s()\n", __PRETTY_FUNCTION__));
#endif

    if ((rc = DoSuperMethodA(CLASS, obj, (Msg)message)))
    {
        if ((data->icld_BufferRastPort) && (data->icld_BufferRastPort != data->icld_DisplayRastPort))
        {
            DeleteLayer(0, data->icld_BufferRastPort->Layer);
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
    struct IconList_DATA        *data = INST_DATA(CLASS, obj);
    struct IconEntry            *node = NULL;

#if defined(DEBUG_ILC_FUNCS)
    D(bug("[IconList]: %s()\n", __PRETTY_FUNCTION__));
#endif

#if defined(__AROS__)
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
    ULONG       rc = DoSuperMethodA(CLASS, obj, (Msg) message);

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
    struct IconList_DATA        *data = INST_DATA(CLASS, obj);
    ULONG                       rc;

#if defined(DEBUG_ILC_FUNCS)
    D(bug("[IconList]: %s()\n", __PRETTY_FUNCTION__));
#endif

    rc = DoSuperMethodA(CLASS, obj, (Msg)message);

    data->icld_ViewWidth = _mwidth(obj);
    data->icld_ViewHeight = _mheight(obj);

    return rc;
}
///

static LONG FirstVisibleLine(struct IconList_DATA *data)
{
    return data->icld_ViewY / data->icld_LVMAttribs->lmva_RowHeight;
}

static LONG NumVisibleLines(struct IconList_DATA *data)
{
    LONG visible = data->icld_ViewHeight + data->icld_LVMAttribs->lmva_RowHeight - 1 +
                           (data->icld_ViewY % data->icld_LVMAttribs->lmva_RowHeight);
                   
    visible /= data->icld_LVMAttribs->lmva_RowHeight;
    
    return visible;                   
}

static void RenderListViewModeHeaderField(Object *obj, struct IconList_DATA *data,
                                        struct Rectangle *rect, LONG index, BOOL sel)
{
    IPTR                penFill, penText, penDark, penBright;
    struct Rectangle        rendRect;
    STRPTR                text;
    struct TextExtent        te;
    ULONG                fit;

    if (sel == TRUE)
    {
        penFill                = _pens(obj)[MPEN_HALFSHADOW];
        penBright        = _pens(obj)[MPEN_SHADOW];
        penDark                = _pens(obj)[MPEN_HALFSHINE];
    }
    else
    {
        penFill         = _pens(obj)[MPEN_HALFSHINE];
        penBright        = _pens(obj)[MPEN_SHINE];
        penDark                = _pens(obj)[MPEN_HALFSHADOW];
    }
    penText = _pens(obj)[MPEN_TEXT];

    rendRect.MinX = rect->MinX;
    rendRect.MaxX = rect->MaxX;
    rendRect.MinY = rect->MinY;
    rendRect.MaxY = rect->MaxY;

    if (data->icld_DisplayRastPort != data->icld_BufferRastPort)
    {
        rendRect.MinX -= _mleft(obj);
        rendRect.MaxX -= _mleft(obj);
        rendRect.MinY -= _mtop(obj);
        rendRect.MaxY -= _mtop(obj);
    }

#if defined(DEBUG_ILC_FUNCS)
D(bug("[IconList]: %s(obj @ 0x%p)\n", __PRETTY_FUNCTION__, obj));
#endif

    if (((data->icld_LVMAttribs->lvma_Flags & LVMAF_NOHEADER) == 0) && (index < NUM_COLUMNS))
    {
        text = data->icld_LVMAttribs->lmva_ColumnTitle[index];

        SetAPen(data->icld_BufferRastPort, penFill); /* Background */
        RectFill(data->icld_BufferRastPort, rendRect.MinX + 1, rendRect.MinY + 1,
                           rendRect.MaxX - 1, rendRect.MaxY - 1);

        SetAPen(data->icld_BufferRastPort, penBright); /* Top/Left */
        RectFill(data->icld_BufferRastPort, rendRect.MinX, rendRect.MinY, rendRect.MinX, rendRect.MaxY);
        RectFill(data->icld_BufferRastPort, rendRect.MinX + 1, rendRect.MinY, rendRect.MaxX - 1, rendRect.MinY);

        SetAPen(data->icld_BufferRastPort,penDark); /* Bottom/Right */
        RectFill(data->icld_BufferRastPort, rendRect.MaxX, rendRect.MinY, rendRect.MaxX, rendRect.MaxY);
        RectFill(data->icld_BufferRastPort, rendRect.MinX + 1, rendRect.MaxY, rendRect.MaxX - 1, rendRect.MaxY);

        /* Draw the Sort indicator .. */
        if (index == data->icld_LVMAttribs->lmva_SortColumn)
        {
            LONG x = rendRect.MaxX - 4 - 6;
            LONG y = (rendRect.MinY + rendRect.MaxY + 1) / 2 - 3;

            if (x > rendRect.MinX)
            {
                SetAPen(data->icld_BufferRastPort, _pens(obj)[sel ? MPEN_SHADOW : MPEN_HALFSHADOW]);
                if (data->icld_SortFlags & MUIV_IconList_Sort_Reverse)
                {
                    RectFill(data->icld_BufferRastPort, x, y, x + 5, y + 1);
                    RectFill(data->icld_BufferRastPort, x + 1, y + 2, x + 4, y + 3);
                    RectFill(data->icld_BufferRastPort, x + 2, y + 4, x + 3, y + 5);
                }
                else
                {
                    RectFill(data->icld_BufferRastPort, x, y + 4, x + 5, y + 5);
                    RectFill(data->icld_BufferRastPort, x + 1, y + 2, x + 4, y + 3);
                    RectFill(data->icld_BufferRastPort, x + 2, y, x + 3, y + 1);
                }
            }
        }

        rendRect.MinX += HEADERENTRY_SPACING_LEFT;
        rendRect.MinY += HEADERLINE_SPACING_TOP;
        rendRect.MaxX -= HEADERENTRY_SPACING_RIGHT;
        rendRect.MaxY -= HEADERLINE_SPACING_BOTTOM;

        if (text && text[0])
        {

            fit = TextFit(data->icld_BufferRastPort, text, strlen(text), &te, NULL, 1,
                           rendRect.MaxX - rendRect.MinX + 1,
                           rendRect.MaxY - rendRect.MinY + 1);

            if (!fit) return;

            SetABPenDrMd(data->icld_BufferRastPort, penText, 0, JAM1);
            Move(data->icld_BufferRastPort, rendRect.MinX, rendRect.MinY + data->icld_BufferRastPort->TxBaseline);
            Text(data->icld_BufferRastPort, text, fit);
        }
    }
}

static void RenderListViewModeHeader(Object *obj, struct IconList_DATA *data)
{
    struct Rectangle linerect;
    LONG                 x, i;
    LONG                 firstvis, lastvis;

#if defined(DEBUG_ILC_FUNCS)
D(bug("[IconList]: %s(obj @ 0x%p)\n", __PRETTY_FUNCTION__, obj));
#endif

    if ((data->icld_LVMAttribs->lvma_Flags & LVMAF_NOHEADER) == 0)
    {
        linerect.MinX = _mleft(obj) - data->icld_ViewX;
        linerect.MaxX = _mright(obj);
        linerect.MinY = _mtop(obj);
        linerect.MaxY = _mtop(obj) + data->icld_LVMAttribs->lmva_HeaderHeight - 1;

        SetFont(data->icld_BufferRastPort, data->icld_IconLabelFont);

        x = linerect.MinX + HEADERLINE_SPACING_LEFT;

        firstvis = FirstVisibleColumnNumber(data);
        lastvis = LastVisibleColumnNumber(data);

        for(i = 0; i < NUM_COLUMNS; i++)
        {
            LONG        index = data->icld_LVMAttribs->lmva_ColumnPos[i];

            if (!(data->icld_LVMAttribs->lmva_ColumnFlags[index] & LVMCF_COLVISIBLE)) continue;

            struct Rectangle         field_rect;

            field_rect.MinX = (i == firstvis) ? linerect.MinX : x;
            field_rect.MinY = linerect.MinY;
            field_rect.MaxX = x + data->icld_LVMAttribs->lmva_ColumnWidth[index] - 1 + ((i == lastvis) ? HEADERLINE_SPACING_RIGHT : 0);
            field_rect.MaxY = linerect.MaxY;

            /* data->update_rect1 and data->update_rect2 may
               point to rectangles to indicate that only icons
               in any of this rectangles need to be drawn      */
            if (data->update_rect1)
            {
                RectAndRect(&field_rect, data->update_rect1);
            }

            if (data->update_rect2)
            {
                RectAndRect(&field_rect, data->update_rect2);
            }

            RenderListViewModeHeaderField(obj, data, &field_rect, index, FALSE);
            x += data->icld_LVMAttribs->lmva_ColumnWidth[index];
        }

        if ((data->icld_LVMAttribs->lvma_Flags & LVMAF_HEADERDRAWTOEND) == LVMAF_HEADERDRAWTOEND)
        {
            x += HEADERLINE_SPACING_RIGHT;

            if (x < linerect.MaxX)
            {
                linerect.MinX = x;

//                if (MustRenderRect(data, &linerect))
//                {
                    SetABPenDrMd(data->icld_BufferRastPort, _pens(obj)[MPEN_HALFSHINE], 0, JAM1);
                    RectFill(data->icld_BufferRastPort, linerect.MinX, linerect.MinY, linerect.MaxX, linerect.MaxY);
//                }
            }
        }
    }
}

///MUIM_Draw()
/**************************************************************************
MUIM_Draw - draw the IconList
**************************************************************************/
IPTR DrawCount;
IPTR IconList__MUIM_Draw(struct IClass *CLASS, Object *obj, struct MUIP_Draw *message)
{   
    struct IconList_DATA        *data = INST_DATA(CLASS, obj);
    struct IconEntry            *entry = NULL;

    APTR                        clip = NULL;

    ULONG                       update_oldwidth = 0,
                                update_oldheight = 0;

    LONG                        clear_xoffset = 0,
                                clear_yoffset = 0;

    __unused IPTR               draw_id = DrawCount++;

#if defined(DEBUG_ILC_FUNCS)
    D(bug("[IconList]: %s(obj @ 0x%p)\n", __PRETTY_FUNCTION__, obj));
#endif
#if defined(DEBUG_ILC_ICONRENDERING)
    D(bug("[IconList] %s: id %d\n", __PRETTY_FUNCTION__, draw_id));
#endif

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
        if ((data->icld_UpdateMode == UPDATE_HEADERENTRY) && ((IPTR)data->update_entry < NUM_COLUMNS)) /* draw the header entry */
        {
            struct Rectangle         field_rect;
            LONG                 index, i, firstvis, lastvis;

            firstvis = FirstVisibleColumnNumber(data);
            lastvis = LastVisibleColumnNumber(data);

            field_rect.MinX = _mleft(obj) - data->icld_ViewX;

            field_rect.MinY = _mtop(obj);
            field_rect.MaxY = field_rect.MinY + data->icld_LVMAttribs->lmva_HeaderHeight - 1;

            for(i = 0; i < NUM_COLUMNS; i++)
            {
                index = data->icld_LVMAttribs->lmva_ColumnPos[i];
                if (!(data->icld_LVMAttribs->lmva_ColumnFlags[index] & LVMCF_COLVISIBLE)) continue;

                field_rect.MaxX = field_rect.MinX + data->icld_LVMAttribs->lmva_ColumnWidth[index] - 1;
                if (index == lastvis)
                    field_rect.MaxX += HEADERLINE_SPACING_RIGHT;

                if ((IPTR)data->update_entry != index)
                {                
                    field_rect.MinX += data->icld_LVMAttribs->lmva_ColumnWidth[index];
                    if (index == firstvis)
                        field_rect.MinX += HEADERLINE_SPACING_LEFT;
                }
                else
                    break;
            }

            clip = MUI_AddClipping(muiRenderInfo(obj), _mleft(obj), _mtop(obj), _mright(obj) - _mleft(obj) + 1, data->icld_LVMAttribs->lmva_HeaderHeight);

            if (data->icld_LVMAttribs->lmva_LastSelectedColumn == (IPTR)data->update_entry)
                RenderListViewModeHeaderField(obj, data, &field_rect, (IPTR)data->update_entry, TRUE);
            else
                RenderListViewModeHeaderField(obj, data, &field_rect, (IPTR)data->update_entry, FALSE);

            data->icld_UpdateMode = 0;
            data->update_entry = NULL;

            if (data->icld_DisplayRastPort != data->icld_BufferRastPort)
            {
#if defined(DEBUG_ILC_ICONRENDERING)
                D(bug("[IconList] %s#%d: UPDATE_HEADERENTRY Blitting to front rastport..\n", __PRETTY_FUNCTION__, draw_id));
#endif 
                BltBitMapRastPort(data->icld_BufferRastPort->BitMap,
                          field_rect.MinX - _mleft(obj), field_rect.MinY - _mtop(obj),
                          data->icld_DisplayRastPort,
                          field_rect.MinX, field_rect.MinY,
                          field_rect.MaxX - field_rect.MinX + 1, field_rect.MaxY - field_rect.MinY + 1,
                          0xC0);
            }
            MUI_RemoveClipping(muiRenderInfo(obj), clip);

            goto draw_done;
        }
        else if ((data->icld_UpdateMode == UPDATE_SINGLEENTRY) && (data->update_entry != NULL)) /* draw only a single entry at update_entry */
        {
            struct Rectangle rect;

            if ((data->icld_DisplayFlags & ICONLIST_DISP_MODELIST) == ICONLIST_DISP_MODELIST)
            {
                LONG count = 0, index = -1;

#if defined(DEBUG_ILC_ICONRENDERING)
                D(bug("[IconList] %s#%d: UPDATE_SINGLEENTRY + ICONLIST_DISP_MODELIST\n", __PRETTY_FUNCTION__, draw_id));
#endif
                rect.MinX = _mleft(obj);
                rect.MaxX = _mleft(obj) + _mwidth(obj) - 1;

                ForeachNode(&data->icld_IconList, entry)
                {
                    if (entry == data->update_entry)
                    {
                        index = count;
                        break;
                    }
                    if (entry->ie_Flags & ICONENTRY_FLAG_VISIBLE)
                    {
                        count++;
                    }
                }

                if (index != -1)
                {
                    rect.MinY = _mtop(obj) + data->icld_LVMAttribs->lmva_HeaderHeight - data->icld_ViewY + (index * data->icld_LVMAttribs->lmva_RowHeight);
                    rect.MaxY = rect.MinY + data->icld_LVMAttribs->lmva_RowHeight - 1;

                    if ((rect.MaxY < (_mtop(obj) + data->icld_LVMAttribs->lmva_HeaderHeight))
                        || (rect.MinY > (_mtop(obj) + _mheight(obj) - 1)))
                        goto draw_done;

                    if (rect.MinY < (_mtop(obj) + data->icld_LVMAttribs->lmva_HeaderHeight)) rect.MinY = _mtop(obj) + data->icld_LVMAttribs->lmva_HeaderHeight;
                    if (rect.MaxY > (_mtop(obj) + _mheight(obj) - 1)) rect.MaxY = _mtop(obj) + _mheight(obj) - 1;

                    clip = MUI_AddClipping(muiRenderInfo(obj), rect.MinX, rect.MinY, rect.MaxX - rect.MinX + 1, rect.MaxY - rect.MinY + 1);

                    DoMethod(obj, MUIM_DrawBackground, 
                        rect.MinX, rect.MinY,
                        rect.MaxX - rect.MinX + 1, rect.MaxY - rect.MinY + 1,
                        clear_xoffset, clear_yoffset, 
                        0);
                    
                    entry->ie_Flags |= ICONENTRY_FLAG_NEEDSUPDATE;
                    DoMethod(obj, MUIM_IconList_DrawEntry, data->update_entry, index);
                    entry->ie_Flags &= ~ICONENTRY_FLAG_NEEDSUPDATE;
                    data->icld_UpdateMode = 0;
                    data->update_entry = NULL;
                }
            }
            else
            {
#if defined(DEBUG_ILC_ICONRENDERING)
                D(bug("[IconList] %s#%d: UPDATE_SINGLEENTRY (entry @ 0x%p)\n", __PRETTY_FUNCTION__, draw_id, data->update_entry));
#endif
                IconList_GetIconAreaRectangle(obj, data, data->update_entry, &rect);

                rect.MinX += _mleft(obj) + (data->update_entry->ie_IconX - data->icld_ViewX);
                rect.MaxX += _mleft(obj) + (data->update_entry->ie_IconX - data->icld_ViewX);
                rect.MinY += _mtop(obj) + (data->update_entry->ie_IconY - data->icld_ViewY);
                rect.MaxY += _mtop(obj) + (data->update_entry->ie_IconY - data->icld_ViewY);

                if (data->icld__Option_IconListMode == ICON_LISTMODE_GRID)
                {
                    if (data->update_entry->ie_AreaWidth < data->icld_IconAreaLargestWidth)
                    {
                        rect.MinX += ((data->icld_IconAreaLargestWidth - data->update_entry->ie_AreaWidth)/2);
                        rect.MaxX += ((data->icld_IconAreaLargestWidth - data->update_entry->ie_AreaWidth)/2);
                    }

                    if (data->update_entry->ie_AreaHeight < data->icld_IconAreaLargestHeight)
                    {
                        rect.MinY += ((data->icld_IconAreaLargestHeight - data->update_entry->ie_AreaHeight)/2);
                        rect.MaxY += ((data->icld_IconAreaLargestHeight - data->update_entry->ie_AreaHeight)/2);
                    }
                }

                if (rect.MinX < _mleft(obj)) rect.MinX = _mleft(obj);
                if (rect.MaxX > _mright(obj)) rect.MaxX =_mright(obj);
                if (rect.MinY < _mtop(obj)) rect.MinY = _mtop(obj);
                if (rect.MaxY > _mbottom(obj)) rect.MaxY = _mbottom(obj);
                
                clip = MUI_AddClipping(muiRenderInfo(obj), rect.MinX, rect.MinY, rect.MaxX - rect.MinX + 1, rect.MaxY - rect.MinY + 1);

#if defined(DEBUG_ILC_ICONRENDERING)
                D(bug("[IconList] %s#%d: UPDATE_SINGLEENTRY: Calling MUIM_DrawBackground (A)\n", __PRETTY_FUNCTION__, draw_id));
#endif
                DoMethod(obj, MUIM_DrawBackground, 
                    rect.MinX, rect.MinY,
                    rect.MaxX - rect.MinX + 1, rect.MaxY - rect.MinY + 1,
                    clear_xoffset, clear_yoffset, 
                    0);

                /* We could have deleted also other icons so they must be redrawn */
#if defined(__AROS__)
                ForeachNode(&data->icld_IconList, entry)
#else
                Foreach_Node(&data->icld_IconList, entry);
#endif
                {
                    if ((entry != data->update_entry) && (entry->ie_Flags & ICONENTRY_FLAG_VISIBLE))
                    {
                        struct Rectangle rect2;
                        IconList_GetIconAreaRectangle(obj, data, entry, &rect2);

                        rect2.MinX += _mleft(obj) - data->icld_ViewX + entry->ie_IconX;
                        rect2.MaxX += _mleft(obj) - data->icld_ViewX + entry->ie_IconX;
                        rect2.MinY += _mtop(obj) - data->icld_ViewY + entry->ie_IconY;
                        rect2.MaxY += _mtop(obj) - data->icld_ViewY + entry->ie_IconY;

                        if (data->icld__Option_IconListMode == ICON_LISTMODE_GRID)
                        {
                            if (entry->ie_AreaWidth < data->icld_IconAreaLargestWidth)
                            {
                                rect2.MinX += ((data->icld_IconAreaLargestWidth - entry->ie_AreaWidth)/2);
                                rect2.MaxX += ((data->icld_IconAreaLargestWidth - entry->ie_AreaWidth)/2);
                            }

                            if (entry->ie_AreaHeight < data->icld_IconAreaLargestHeight)
                            {
                                rect2.MinY += ((data->icld_IconAreaLargestHeight - entry->ie_AreaHeight)/2);
                                rect2.MaxY += ((data->icld_IconAreaLargestHeight - entry->ie_AreaHeight)/2);
                            }
                        }

                        if (RectAndRect(&rect, &rect2))
                        {  
                            // Update entry here
                            entry->ie_Flags |= ICONENTRY_FLAG_NEEDSUPDATE;
                            DoMethod(obj, MUIM_IconList_DrawEntry, entry, ICONENTRY_DRAWMODE_PLAIN);
                            DoMethod(obj, MUIM_IconList_DrawEntryLabel, entry, ICONENTRY_DRAWMODE_PLAIN);
                            entry->ie_Flags &= ~ICONENTRY_FLAG_NEEDSUPDATE;
                        }
                    }
                }

                entry->ie_Flags |= ICONENTRY_FLAG_NEEDSUPDATE;
                DoMethod(obj, MUIM_IconList_DrawEntry, data->update_entry, ICONENTRY_DRAWMODE_PLAIN);
                DoMethod(obj, MUIM_IconList_DrawEntryLabel, data->update_entry, ICONENTRY_DRAWMODE_PLAIN);
                entry->ie_Flags &= ~ICONENTRY_FLAG_NEEDSUPDATE;
                data->icld_UpdateMode = 0;
                data->update_entry = NULL;
            }
            if (data->update_entry == NULL)
            {
                if (data->icld_DisplayRastPort != data->icld_BufferRastPort)
                {
#if defined(DEBUG_ILC_ICONRENDERING)
                    D(bug("[IconList] %s#%d: UPDATE_SINGLEENTRY Blitting to front rastport..\n", __PRETTY_FUNCTION__, draw_id));
#endif 
                    BltBitMapRastPort(data->icld_BufferRastPort->BitMap,
                              rect.MinX - _mleft(obj), rect.MinY - _mtop(obj),
                              data->icld_DisplayRastPort,
                              rect.MinX, rect.MinY,
                              rect.MaxX - rect.MinX + 1, rect.MaxY - rect.MinY + 1,
                              0xC0);
                }
                MUI_RemoveClipping(muiRenderInfo(obj), clip);
            }
            goto draw_done;
        }
        else if (data->icld_UpdateMode == UPDATE_SCROLL)
        {
            struct Region       *region = NULL;
            struct Rectangle    xrect,
                                yrect;
            BOOL                scroll_caused_damage = FALSE;

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
                    if (((data->icld_DisplayFlags & ICONLIST_DISP_MODELIST) == ICONLIST_DISP_MODELIST)
                        && (yrect.MinY < (_mtop(obj) + data->icld_LVMAttribs->lmva_HeaderHeight)))
                    {
                            xrect.MinY = data->icld_LVMAttribs->lmva_HeaderHeight;
                    }
                    yrect.MaxX = _mright(obj);
                    yrect.MaxY = _mbottom(obj);

                    OrRectRegion(region, &yrect);

                    data->update_rect2 = &yrect;
                }
                else if (data->update_scrolldy < 0)
                {
                    yrect.MinX = _mleft(obj);
                    yrect.MinY = _mtop(obj);
                    if ((data->icld_DisplayFlags & ICONLIST_DISP_MODELIST) == ICONLIST_DISP_MODELIST)
                    {
                        xrect.MinY += data->icld_LVMAttribs->lmva_HeaderHeight;
                    }
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
                                            _mwidth(obj) - 1,
                                            _mheight(obj) - 1);
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
                //Free up the buffers Layer, rastport and bitmap so we can replace them ..
                if ((GetBitMapAttr(data->icld_BufferRastPort->BitMap, BMA_WIDTH) != data->icld_ViewWidth)
                    || (GetBitMapAttr(data->icld_BufferRastPort->BitMap, BMA_HEIGHT) != data->icld_ViewHeight))
                {
                    struct Layer *oldLayer = data->icld_BufferRastPort->Layer;
#if defined(DEBUG_ILC_ATTRIBS) || defined(DEBUG_ILC_ICONRENDERING)
                    D(bug("[IconList] %s: Destroying old BackLayer\n", __PRETTY_FUNCTION__));
#endif
                    data->icld_BufferRastPort = data->icld_DisplayRastPort;
                    DeleteLayer(0, oldLayer);
                }

                if (data->icld_BufferRastPort == data->icld_DisplayRastPort)
                {
                    struct BitMap *bitmap_New;
                    ULONG tmp_RastDepth;
                    struct Layer_Info *li = NULL;

                    tmp_RastDepth = GetCyberMapAttr(data->icld_DisplayRastPort->BitMap, CYBRMATTR_DEPTH);
                    if ((bitmap_New = AllocBitMap(data->icld_ViewWidth,
                                        data->icld_ViewHeight,
                                        tmp_RastDepth,
                                        BMF_CLEAR,
                                        data->icld_DisplayRastPort->BitMap)) != NULL)
                    {
                        if ((data->icld_BufferRastPort = CreateRastPort()) != NULL)
                        {
                            data->icld_BufferRastPort->BitMap = bitmap_New;
                            if ((li = NewLayerInfo()))
                            {
                                if ((data->icld_BufferRastPort->Layer = CreateUpfrontLayer(li, data->icld_BufferRastPort->BitMap, 0, 0, data->icld_ViewWidth - 1, data->icld_ViewHeight - 1, 0, NULL)))
                                {
                                   /*
                                    * Mark it as a buffered rastport.
                                    */

    #if defined(DEBUG_ILC_ATTRIBS) || defined(DEBUG_ILC_ICONRENDERING)
                                    D(bug("[IconList] %s: FrontRastPort @ %p, New BackLayer @ %p, BackRastport @ %p\n", __PRETTY_FUNCTION__, data->icld_DisplayRastPort, data->icld_BufferRastPort->Layer, data->icld_BufferRastPort));
    #endif
                                    SET(obj, MUIA_IconList_BufferRastport, data->icld_BufferRastPort);
                                    data->icld_DrawOffsetX = 0;
                                    data->icld_DrawOffsetY = 0;
                                }
                                else
                                    data->icld_BufferRastPort = data->icld_DisplayRastPort;
                            }
                            else
                                data->icld_BufferRastPort = data->icld_DisplayRastPort;
                        }
                        else
                            data->icld_BufferRastPort = data->icld_DisplayRastPort;
                    }
                    
                    if (data->icld_BufferRastPort == data->icld_DisplayRastPort)
                    {
                        if (bitmap_New) FreeBitMap(bitmap_New);
                        if (li) DisposeLayerInfo(li);
                        data->icld_DrawOffsetX = _mleft(obj);
                        data->icld_DrawOffsetY = _mtop(obj);
                    }
                }
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
        int current = 0, first = 0, visible = 0;

#if defined(DEBUG_ILC_ICONRENDERING)
        D(bug("[IconList] %s#%d: MADF_DRAWOBJECT\n", __PRETTY_FUNCTION__, draw_id));
#endif

        if ((data->icld_DisplayFlags & ICONLIST_DISP_MODELIST) == ICONLIST_DISP_MODELIST)
        {
            clip = MUI_AddClipping(muiRenderInfo(obj), _mleft(obj), _mtop(obj), _mwidth(obj), data->icld_LVMAttribs->lmva_HeaderHeight);
            RenderListViewModeHeader(obj, data);

            if (data->icld_DisplayRastPort != data->icld_BufferRastPort)
            {
#if defined(DEBUG_ILC_ICONRENDERING)
                D(bug("[IconList] %s#%d: MADF_DRAWOBJECT Blitting Header to front rastport..\n", __PRETTY_FUNCTION__, draw_id));
#endif 
                BltBitMapRastPort(data->icld_BufferRastPort->BitMap,
                          0, 0,
                          data->icld_DisplayRastPort,
                          _mleft(obj), _mtop(obj), _mwidth(obj), data->icld_LVMAttribs->lmva_HeaderHeight,
                          0xC0);
            }

            MUI_RemoveClipping(muiRenderInfo(obj), clip);

            viewrect.MinY = _mtop(obj) + data->icld_LVMAttribs->lmva_HeaderHeight;

            first   = FirstVisibleLine(data);
            visible = NumVisibleLines(data);

            clip = MUI_AddClipping(muiRenderInfo(obj), _mleft(obj), _mtop(obj) + data->icld_LVMAttribs->lmva_HeaderHeight, _mwidth(obj), _mheight(obj) - data->icld_LVMAttribs->lmva_HeaderHeight);
        }
        else
        {
            viewrect.MinY = _mtop(obj);
            clip = MUI_AddClipping(muiRenderInfo(obj), _mleft(obj), _mtop(obj), _mwidth(obj), _mheight(obj));
        }

        viewrect.MaxY = _mtop(obj) + _mheight(obj) - 1;
        viewrect.MinX = _mleft(obj);
        viewrect.MaxX = _mleft(obj) + _mwidth(obj) - 1;

#if defined(DEBUG_ILC_ICONRENDERING)
        D(bug("[IconList] %s#%d: MADF_DRAWOBJECT: Calling MUIM_DrawBackground (B)\n", __PRETTY_FUNCTION__, draw_id));
#endif
        DoMethod(
            obj, MUIM_DrawBackground, viewrect.MinX, viewrect.MinY, (viewrect.MaxX - viewrect.MinX) + 1, (viewrect.MaxY - viewrect.MinY) + 1,
            clear_xoffset, clear_yoffset, 0
        );
#if defined(__AROS__)
        ForeachNode(&data->icld_IconList, entry)
#else
        Foreach_Node(&data->icld_IconList, entry);
#endif
        {
            if ((data->icld_DisplayFlags & ICONLIST_DISP_MODELIST) == ICONLIST_DISP_MODELIST)
            {
                if (entry->ie_Flags & ICONENTRY_FLAG_VISIBLE)
                {
                    if ((current >= first) && (current <= (first + visible)))
                    {
                        DoMethod(obj, MUIM_IconList_DrawEntry, entry, current);
                    }
                    current++;
                }
            }
            else
            {
                if ((entry->ie_Flags & ICONENTRY_FLAG_VISIBLE) &&
                    (entry->ie_DiskObj) &&
                    (entry->ie_IconX != NO_ICON_POSITION) &&
                    (entry->ie_IconY != NO_ICON_POSITION))
                {
                    struct Rectangle iconrect;
                    IconList_GetIconAreaRectangle(obj, data, entry, &iconrect);

                    iconrect.MinX += viewrect.MinX - data->icld_ViewX + entry->ie_IconX;
                    iconrect.MaxX += viewrect.MinX - data->icld_ViewX + entry->ie_IconX;
                    iconrect.MinY += viewrect.MinY - data->icld_ViewY + entry->ie_IconY;
                    iconrect.MaxY += viewrect.MinY - data->icld_ViewY + entry->ie_IconY;

                    if (RectAndRect(&viewrect, &iconrect))
                    {
                        DoMethod(obj, MUIM_IconList_DrawEntry, entry, ICONENTRY_DRAWMODE_PLAIN);
                        DoMethod(obj, MUIM_IconList_DrawEntryLabel, entry, ICONENTRY_DRAWMODE_PLAIN);
                    }
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

#if defined(DEBUG_ILC_ICONRENDERING)
    D(bug("[IconList] %s: Draw finished for id %d\n", __PRETTY_FUNCTION__, draw_id));
#endif 
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
    struct IconList_DATA        *data = INST_DATA(CLASS, obj);

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
    struct IconList_DATA        *data = INST_DATA(CLASS, obj);
    struct IconEntry            *node = NULL;

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
    data->icld_IconAreaLargestWidth = 0;
    data->icld_IconAreaLargestHeight = 0;
    data->icld_IconLargestHeight = 0;
    data->icld_LabelLargestHeight = 0;

#if defined(DEBUG_ILC_ICONRENDERING)
    D(bug("[IconList]: %s: call SetSuperAttrs()\n", __PRETTY_FUNCTION__));
#endif
    SetSuperAttrs(CLASS, obj, MUIA_Virtgroup_Left, data->icld_ViewX,
                  MUIA_Virtgroup_Top, data->icld_ViewY,
            TAG_DONE);

#if defined(DEBUG_ILC_ICONRENDERING)
    D(bug("[IconList]: %s: call SetAttrs()\n", __PRETTY_FUNCTION__));
#endif
    SetAttrs(obj, MUIA_Virtgroup_Left, data->icld_ViewX,
                  MUIA_Virtgroup_Top, data->icld_ViewY,
            TAG_DONE);

#if defined(DEBUG_ILC_ICONRENDERING)
    D(bug("[IconList]: %s: Set MUIA_IconList_Width and MUIA_IconList_Height\n", __PRETTY_FUNCTION__));
#endif
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
    struct IconList_DATA        *data = INST_DATA(CLASS, obj);

#if defined(DEBUG_ILC_FUNCS)
    D(bug("[IconList]: %s()\n", __PRETTY_FUNCTION__));
#endif

    if (message->entry)
    {
        if (message->entry->ie_Flags & ICONENTRY_FLAG_SELECTED)
        {
            if (data->icld_SelectionLastClicked == message->entry)
            {
                struct IconList_Entry *nextentry    = &message->entry->ie_IconListEntry;

                /* get selected entries from SOURCE iconlist */
                DoMethod(obj, MUIM_IconList_NextIcon, MUIV_IconList_NextIcon_Selected, (IPTR)&nextentry);
                if ((nextentry) && ((IPTR)nextentry != MUIV_IconList_NextIcon_End))
                    data->icld_SelectionLastClicked = (struct IconEntry *)((IPTR)nextentry - ((IPTR)&message->entry->ie_IconListEntry - (IPTR)message->entry));
                else
                    data->icld_SelectionLastClicked = NULL;
            }
            if (data->icld_FocusIcon == message->entry)
                data->icld_FocusIcon = data->icld_SelectionLastClicked;

            Remove(&message->entry->ie_SelectionNode);
        }

        if (message->entry->ie_TxtBuf_DisplayedLabel)
            FreeVecPooled(data->icld_Pool, message->entry->ie_TxtBuf_DisplayedLabel);

        if (message->entry->ie_TxtBuf_PROT)
            FreePooled(data->icld_Pool, message->entry->ie_TxtBuf_PROT, 8);

        if (message->entry->ie_TxtBuf_SIZE)
            FreePooled(data->icld_Pool, message->entry->ie_TxtBuf_SIZE, 30);

        if (message->entry->ie_TxtBuf_TIME)
            FreePooled(data->icld_Pool, message->entry->ie_TxtBuf_TIME, LEN_DATSTRING);

        if (message->entry->ie_TxtBuf_DATE)
            FreePooled(data->icld_Pool, message->entry->ie_TxtBuf_DATE, LEN_DATSTRING);

        if (message->entry->ie_DiskObj)
            FreeDiskObject(message->entry->ie_DiskObj);

        if (message->entry->ie_FileInfoBlock)
            FreeMem(message->entry->ie_FileInfoBlock, sizeof(struct FileInfoBlock));

        if (message->entry->ie_IconListEntry.label)
            FreePooled(data->icld_Pool, message->entry->ie_IconListEntry.label, strlen(message->entry->ie_IconListEntry.label)+1);

        if (message->entry->ie_IconNode.ln_Name)
            FreePooled(data->icld_Pool, message->entry->ie_IconNode.ln_Name, strlen(message->entry->ie_IconNode.ln_Name)+1);

        FreePooled(data->icld_Pool, message->entry, sizeof(struct IconEntry));
    }
    return (IPTR)TRUE;
}
///

///IconList__MUIM_IconList_PropagateEntryPos()
IPTR IconList__MUIM_IconList_PropagateEntryPos(struct IClass *CLASS, Object *obj,
        struct MUIP_IconList_PropagateEntryPos *message)
{
    message->entry->ie_ProvidedIconX = message->entry->ie_IconX;
    message->entry->ie_ProvidedIconY = message->entry->ie_IconY;

    return (IPTR)TRUE;
}

//
///IconList__MUIM_IconList_CreateEntry()
/**************************************************************************
MUIM_IconList_CreateEntry.
Returns 0 on failure; otherwise it returns the icon's entry.
**************************************************************************/
IPTR IconList__MUIM_IconList_CreateEntry(struct IClass *CLASS, Object *obj, struct MUIP_IconList_CreateEntry *message)
{
    struct IconList_DATA        *data = INST_DATA(CLASS, obj);
    struct IconEntry            *entry = NULL;
    struct DateTime             dt;
    struct DateStamp            now;
    UBYTE                       *sp = NULL;

    struct DiskObject           *dob = NULL;
    struct Rectangle            rect;

    IPTR                        geticon_error = 0;

#if defined(DEBUG_ILC_FUNCS)
    D(bug("[IconList]: %s()\n", __PRETTY_FUNCTION__));
#endif

    if (message->filename == NULL) {
        D(bug("[IconList] %s: IconList - filename was NULL\n", __PRETTY_FUNCTION__));
        return (IPTR)NULL;
    }

    /*disk object (icon)*/
    if (message->entry_dob == NULL)
    {
        IPTR iconlistScreen = (IPTR)_screen(obj);
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
        dob = message->entry_dob;
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

    /*alloc entry label*/
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

    /* Override type if specified during CreateEntry */
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

    entry->ie_IconListEntry.udata = message->udata;

    entry->ie_IconX = dob->do_CurrentX;
    entry->ie_IconY = dob->do_CurrentY;

    DoMethod(obj, MUIM_IconList_PropagateEntryPos, entry);

    if (IconList__LabelFunc_CreateLabel(obj, data, entry) != (IPTR)NULL)
    {
        entry->ie_DiskObj = dob;

        /* Use a geticonrectangle routine that gets textwidth! */
        IconList_GetIconAreaRectangle(obj, data, entry, &rect);

        return (IPTR)entry;
    }

    DoMethod(obj, MUIM_IconList_DestroyEntry, entry);
    return (IPTR)NULL;
}
///

///IconList__MUIM_IconList_UpdateEntry()
/**************************************************************************
MUIM_IconList_UpdateEntry.
Returns 0 on failure; otherwise it returns the icon's entry.
**************************************************************************/
IPTR IconList__MUIM_IconList_UpdateEntry(struct IClass *CLASS, Object *obj, struct MUIP_IconList_UpdateEntry *message)
{
    struct IconList_DATA        *data = INST_DATA(CLASS, obj);
    // struct DateTime             dt;
    // struct DateStamp            now;
    // UBYTE                       *sp = NULL;

    // struct DiskObject           *dob = NULL;
    struct Rectangle            rect;

    // IPTR                        geticon_error = 0;

#if defined(DEBUG_ILC_FUNCS)
    D(bug("[IconList]: %s()\n", __PRETTY_FUNCTION__));
#endif

    /* Update disk object (icon)*/
/*    if (message->entry_dob == NULL)
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
        dob = message->entry_dob;
    }

D(bug("[IconList] %s: DiskObject @ 0x%p\n", __PRETTY_FUNCTION__, dob));
*/

    /* Update filename */
    if (strcmp(message->entry->ie_IconNode.ln_Name, message->filename) != 0)
    {
        message->entry->ie_Flags |= ICONENTRY_FLAG_NEEDSUPDATE;
        FreePooled(data->icld_Pool, message->entry->ie_IconNode.ln_Name, strlen(message->entry->ie_IconNode.ln_Name) + 1);
        if ((message->entry->ie_IconNode.ln_Name = AllocPooled(data->icld_Pool, strlen(message->filename) + 1)) == NULL)
        {
            D(bug("[IconList] %s: Failed to Allocate Entry filename string Storage!\n", __PRETTY_FUNCTION__));
            DoMethod(obj, MUIM_IconList_DestroyEntry, message->entry);
            return (IPTR)NULL;
        }
        strcpy(message->entry->ie_IconNode.ln_Name, message->filename);
    }

    /* Update entry label */
    if (strcmp(message->entry->ie_IconListEntry.label, message->label) != 0)
    {
        message->entry->ie_Flags |= ICONENTRY_FLAG_NEEDSUPDATE;
        FreePooled(data->icld_Pool, message->entry->ie_IconListEntry.label, strlen(message->entry->ie_IconListEntry.label) + 1);
        if ((message->entry->ie_IconListEntry.label = AllocPooled(data->icld_Pool, strlen(message->label) + 1)) == NULL)
        {
            D(bug("[IconList] %s: Failed to Allocate Entry label string Storage!\n", __PRETTY_FUNCTION__));
            DoMethod(obj, MUIM_IconList_DestroyEntry, message->entry);
            return (IPTR)NULL;
        }
        strcpy(message->entry->ie_IconListEntry.label, message->label);
        if (IconList__LabelFunc_CreateLabel(obj, data, message->entry) == (IPTR)NULL)
        {
            D(bug("[IconList] %s: Failed to create label\n", __PRETTY_FUNCTION__));
            DoMethod(obj, MUIM_IconList_DestroyEntry, message->entry);
            return (IPTR)NULL;
        }
    }

    /* Update file info block */
    if(message->fib != NULL)
    {
        if (!(message->entry->ie_FileInfoBlock))
        {
            if ((message->entry->ie_FileInfoBlock = AllocMem(sizeof(struct FileInfoBlock), MEMF_CLEAR)) != NULL)
            {
                CopyMem(message->fib, message->entry->ie_FileInfoBlock, sizeof(struct FileInfoBlock));
            }
        }

/*        if (entry->ie_FileInfoBlock->fib_DirEntryType > 0)
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

        //if modified today show time, otherwise just show date
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
*/
    }
    else
    {
        if (message->entry->ie_FileInfoBlock)
        {
            FreeMem(message->entry->ie_FileInfoBlock, sizeof(struct FileInfoBlock));
            message->entry->ie_FileInfoBlock = NULL;
        }
        if (message->entry->ie_IconListEntry.type != ST_USERDIR)
        {
            message->entry->ie_Flags |= ICONENTRY_FLAG_NEEDSUPDATE;
            message->entry->ie_IconListEntry.type = ST_USERDIR;
        }
    }

    /* Override type if specified */
    if ((message->type != 0) && (message->entry->ie_IconListEntry.type != message->type))
    {
        message->entry->ie_Flags |= ICONENTRY_FLAG_NEEDSUPDATE;
        message->entry->ie_IconListEntry.type = message->type;
        D(bug("[IconList] %s: Overide Entry Type. New Type = %x\n", __PRETTY_FUNCTION__, message->entry->ie_IconListEntry.type));
    }
    else
    {
        D(bug("[IconList] %s: Entry Type = %x\n", __PRETTY_FUNCTION__, message->entry->ie_IconListEntry.type));
    }

    IconList_GetIconAreaRectangle(obj, data, message->entry, &rect);

    return (IPTR)message->entry;
}
///

///DoWheelMove()
static void DoWheelMove(struct IClass *CLASS, Object *obj, LONG wheelx, LONG wheely, UWORD qual)
{
    struct IconList_DATA        *data = INST_DATA(CLASS, obj);

    LONG                        newleft = data->icld_ViewX,
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

    if (newleft + _mwidth(obj) > data->icld_AreaWidth)
        newleft = data->icld_AreaWidth - _mwidth(obj);
    if (newleft < 0)
        newleft = 0;

    if (newtop + _mheight(obj) > data->icld_AreaHeight)
        newtop = data->icld_AreaHeight - _mheight(obj);
    if (newtop < 0)
        newtop = 0;

    if ((newleft != data->icld_ViewX) || (newtop != data->icld_ViewY))
    {
        SetAttrs(obj, MUIA_Virtgroup_Left, newleft,
            MUIA_Virtgroup_Top, newtop,
            TAG_DONE);
    }
}
///

/* Notes:
 *
 * LEFTDOWN
 *  a) if clicked object is selected, nothing
 *  b) if clicked object is not selected, unselect all, select object
 *  c) if no clicked object, start lasso
 * LEFTUP
 *  a) if in lasso, finish lasso
 * LEFTDOWN + SHIFT
 *  a) if object is selected, unselect it (= remove from multiselection)
 *  b) if object is not selected, select it (= add to multiselection)
 *
 *
 * Expected behaviour:
 * a) you can only "remove" multiselection by clicking on not selected object or on space where there is no icon
 *
 */

static void IconList_HandleNewIconSelection(struct IClass *CLASS, Object *obj, struct MUIP_HandleEvent *message,
        struct IconEntry *new_selected, BOOL *doubleclicked)
{
    struct IconEntry        *node = NULL;
    BOOL                    update_entry = FALSE;
    LONG                    mx = message->imsg->MouseX - _mleft(obj);
    LONG                    my = message->imsg->MouseY - _mtop(obj);
    struct IconList_DATA    *data = INST_DATA(CLASS, obj);
    BOOL                    nounselection = (new_selected != NULL &&
                                (new_selected->ie_Flags & ICONENTRY_FLAG_SELECTED)); /* see notes above */


    /* Check if this is a double click on icon or empty space */
    if ((DoubleClick(data->last_secs, data->last_mics, message->imsg->Seconds, message->imsg->Micros)) && (data->icld_SelectionLastClicked == new_selected))
    {
        #if defined(DEBUG_ILC_EVENTS)
        D(bug("[IconList] %s: Entry double-clicked\n", __PRETTY_FUNCTION__));
        #endif
        *doubleclicked = TRUE;
    }

    /* Deselection lopp */
#if defined(__AROS__)
    ForeachNode(&data->icld_IconList, node)
#else
    Foreach_Node(&data->icld_IconList, node);
#endif
    {
        if (node->ie_Flags & ICONENTRY_FLAG_VISIBLE)
        {
            update_entry = FALSE;

            /* If node that is being checked is selected and it is not the clicked node
             * and no shift pressed and
             * clicked node is not part of selection (see notes above) or this is a double click */
            if (node->ie_Flags & ICONENTRY_FLAG_SELECTED)
            {
                if ((new_selected != node) &&
                    (!(message->imsg->Qualifier & (IEQUALIFIER_LSHIFT | IEQUALIFIER_RSHIFT))) &&
                    (!nounselection || *doubleclicked))
                {
                    Remove(&node->ie_SelectionNode);
                    node->ie_Flags &= ~ICONENTRY_FLAG_SELECTED;
                    update_entry = TRUE;
                }
            }

            /* Remove focus */
            if ((node->ie_Flags & ICONENTRY_FLAG_FOCUS) && (new_selected != node))
            {
                node->ie_Flags &= ~ICONENTRY_FLAG_FOCUS;
                update_entry = TRUE;
            }

            /* Redraw list */
            if (update_entry)
            {
                data->icld_UpdateMode = UPDATE_SINGLEENTRY;
                data->update_entry = node;
                MUI_Redraw(obj, MADF_DRAWUPDATE);
                #if defined(DEBUG_ILC_EVENTS)
                D(bug("[IconList] %s: Rendered entry '%s'\n", __PRETTY_FUNCTION__, node->ie_IconListEntry.label));
                #endif
            }
        }
    }

    if (new_selected != NULL)
    {
        /* Found clicked entry... */
        data->icld_LassoActive = FALSE;
        update_entry = FALSE;

        if (!(new_selected->ie_Flags & ICONENTRY_FLAG_SELECTED))
        {
            /* Add new entry to selection */
            AddTail(&data->icld_SelectionList, &new_selected->ie_SelectionNode);
            new_selected->ie_Flags |= ICONENTRY_FLAG_SELECTED;
            update_entry = TRUE;
        }
        else if ((*doubleclicked == FALSE) && (message->imsg->Qualifier & (IEQUALIFIER_LSHIFT | IEQUALIFIER_RSHIFT)))
        {
            /* Unselect previously selected entry */
            Remove(&new_selected->ie_SelectionNode);
            new_selected->ie_Flags &= ~ICONENTRY_FLAG_SELECTED;
            update_entry = TRUE;
        }

        /* Set focus */
        if (!(new_selected->ie_Flags & ICONENTRY_FLAG_FOCUS))
        {
            new_selected->ie_Flags |= ICONENTRY_FLAG_FOCUS;
            data->icld_FocusIcon = new_selected;
            update_entry = TRUE;
        }

        /* Redraw list */
        if (update_entry)
        {
            data->icld_UpdateMode = UPDATE_SINGLEENTRY;
            data->update_entry = new_selected;
            MUI_Redraw(obj, MADF_DRAWUPDATE);
            #if defined(DEBUG_ILC_EVENTS)
            D(bug("[IconList] %s: Rendered 'new_selected' entry '%s'\n", __PRETTY_FUNCTION__, new_selected->ie_IconListEntry.label));
            #endif
        }
    }
    else
    {
        struct Window * thisWindow = NULL;
        #if defined(DEBUG_ILC_EVENTS) || defined(DEBUG_ILC_LASSO)
        D(bug("[IconList] %s: Starting Lasso\n", __PRETTY_FUNCTION__));
        #endif
        /* No entry clicked on ... Start Lasso-selection */
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
        IconList_InvertLassoOutlines(obj, data, &data->icld_LassoRectangle);

        /* Start handling INTUITICKS */
        GET(obj, MUIA_Window, &thisWindow);
        if (thisWindow)
        {
            ModifyIDCMP(thisWindow, (thisWindow->IDCMPFlags|IDCMP_INTUITICKS));
            if (!(data->ehn.ehn_Events & IDCMP_INTUITICKS))
            {
                DoMethod(_win(obj), MUIM_Window_RemEventHandler, (IPTR)&data->ehn);
                data->ehn.ehn_Events |= IDCMP_INTUITICKS;
                DoMethod(_win(obj), MUIM_Window_AddEventHandler, (IPTR)&data->ehn);
            }
        }
    }
}

///MUIM_HandleEvent()
/**************************************************************************
MUIM_HandleEvent
**************************************************************************/
IPTR IconList__MUIM_HandleEvent(struct IClass *CLASS, Object *obj, struct MUIP_HandleEvent *message)
{
    struct IconList_DATA        *data = INST_DATA(CLASS, obj);

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
            case IDCMP_NEWSIZE:
                bug("[IconList] %s: IDCMP_NEWSIZE\n", __PRETTY_FUNCTION__);
                break;

            case IDCMP_RAWKEY:
                {
#if defined(DEBUG_ILC_EVENTS)
                    D(bug("[IconList] %s: IDCMP_RAWKEY\n", __PRETTY_FUNCTION__));
#endif
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

                    /* Remove the lasso if a key is pressed or the mouse wheel is used */
                    NullifyLasso(data, obj);

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
                                        data->icld_UpdateMode = UPDATE_SINGLEENTRY;
                                        data->update_entry = active_entry;
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

                                    data->icld_UpdateMode = UPDATE_SINGLEENTRY;
                                    data->update_entry = active_entry;
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
                                    D(bug("[IconList] %s: UP: Clearing existing focused entry @ 0x%p\n", __PRETTY_FUNCTION__, start_entry));
#endif

                                    start_entry->ie_Flags &= ~ICONENTRY_FLAG_FOCUS;
                                    data->icld_UpdateMode = UPDATE_SINGLEENTRY;
                                    data->update_entry = start_entry;
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
                                        //Check if we are at the edge of the entry area ..
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
                                    // If nothing is selected we will use the last visible entry ..
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
                                    D(bug("[IconList] %s: UP: No Next UP Node - Getting Last visible entry ..\n", __PRETTY_FUNCTION__));
#endif
                                    /* We didnt find a "next UP" entry so just use the last visible */
                                    active_entry = Node_LastVisible(&data->icld_IconList);
                                }

                                if (active_entry)
                                {
                                    if (!(active_entry->ie_Flags & ICONENTRY_FLAG_FOCUS))
                                    {
                                        active_entry->ie_Flags |= ICONENTRY_FLAG_FOCUS;
                                        data->icld_UpdateMode = UPDATE_SINGLEENTRY;
                                        data->update_entry = active_entry;
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
                                    D(bug("[IconList] %s: DOWN: Clearing existing focused entry @ 0x%p\n", __PRETTY_FUNCTION__, start_entry));
#endif

                                    start_entry->ie_Flags &= ~ICONENTRY_FLAG_FOCUS;
                                    data->icld_UpdateMode = UPDATE_SINGLEENTRY;
                                    data->update_entry = start_entry;
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
                                    // If nothing is selected we will use the First visible entry ..
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
                                    D(bug("[IconList] %s: DOWN: No Next DOWN Node - Getting first visable entry ..\n", __PRETTY_FUNCTION__));
#endif
                                    /* We didnt find a "next DOWN" entry so just use the first visible */
                                    active_entry =  Node_FirstVisible(&data->icld_IconList);
                                }

                                if (active_entry)
                                {
                                    if (!(active_entry->ie_Flags & ICONENTRY_FLAG_FOCUS))
                                    {
                                        active_entry->ie_Flags |= ICONENTRY_FLAG_FOCUS;
                                        data->icld_UpdateMode = UPDATE_SINGLEENTRY;
                                        data->update_entry = active_entry;
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
                                    D(bug("[IconList] %s: LEFT: Clearing existing focused entry @ 0x%p\n", __PRETTY_FUNCTION__, start_entry));
#endif

                                    start_entry->ie_Flags &= ~ICONENTRY_FLAG_FOCUS;
                                    data->icld_UpdateMode = UPDATE_SINGLEENTRY;
                                    data->update_entry = start_entry;
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
                                        D(bug("[IconList] %s: LEFT: Start at the beginning (Active @ 0x%p) using entry X + Width\n", __PRETTY_FUNCTION__, active_entry));
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
                                    D(bug("[IconList] %s: LEFT: No Next LEFT Node - Getting first visable entry ..\n", __PRETTY_FUNCTION__));
#endif
                                    /* We didnt find a "next LEFT" entry so just use the last visible */
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
                                        data->icld_UpdateMode = UPDATE_SINGLEENTRY;
                                        data->update_entry = active_entry;
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
                                    D(bug("[IconList] %s: RIGHT: Clearing existing focused entry @ 0x%p\n", __PRETTY_FUNCTION__, start_entry));
#endif
                                    start_entry->ie_Flags &= ~ICONENTRY_FLAG_FOCUS;
                                    data->icld_UpdateMode = UPDATE_SINGLEENTRY;
                                    data->update_entry = start_entry;
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
                                        D(bug("[IconList] %s: RIGHT: Start at the beginning (Active @ 0x%p) using entry X + Width\n", __PRETTY_FUNCTION__, active_entry));
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
                                    D(bug("[IconList] %s: RIGHT: No Next RIGHT Node - Getting first visable entry ..\n", __PRETTY_FUNCTION__));
#endif
                                    /* We didnt find a "next RIGHT" entry so just use the first visible */
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
                                        data->icld_UpdateMode = UPDATE_SINGLEENTRY;
                                        data->update_entry = active_entry;
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
                                    data->icld_UpdateMode = UPDATE_SINGLEENTRY;
                                    data->update_entry = data->icld_FocusIcon;
                                    MUI_Redraw(obj, MADF_DRAWUPDATE);
                                }

                                active_entry =  Node_FirstVisible(&data->icld_IconList);
                                
                                if ((active_entry) && (!(active_entry->ie_Flags & ICONENTRY_FLAG_FOCUS)))
                                {
                                    active_entry->ie_Flags |= ICONENTRY_FLAG_FOCUS;
                                    data->icld_UpdateMode = UPDATE_SINGLEENTRY;
                                    data->update_entry = active_entry;
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
                                    data->icld_UpdateMode = UPDATE_SINGLEENTRY;
                                    data->update_entry = data->icld_FocusIcon;
                                    MUI_Redraw(obj, MADF_DRAWUPDATE);
                                }

                                active_entry = Node_LastVisible(&data->icld_IconList);

                                if ((active_entry) && (!(active_entry->ie_Flags & ICONENTRY_FLAG_FOCUS)))
                                {
                                    active_entry->ie_Flags |= ICONENTRY_FLAG_FOCUS;
                                    data->icld_UpdateMode = UPDATE_SINGLEENTRY;
                                    data->update_entry = active_entry;
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
#if defined(DEBUG_ILC_EVENTS)
                D(bug("[IconList] %s: IDCMP_MOUSEBUTTONS\n", __PRETTY_FUNCTION__));
#endif
                if (message->imsg->Code == SELECTDOWN)
                {
                    /* Check if mouse pressed on iconlist area */
                    if (mx >= 0 && mx < _width(obj) && my >= 0 && my < _height(obj))
                    {
                        BOOL                doubleclicked = FALSE; /* both icon and empty space */
                        struct IconEntry    *node = NULL;
                        struct IconEntry    *new_selected = NULL;

                        if ((data->icld_DisplayFlags & ICONLIST_DISP_MODELIST) == ICONLIST_DISP_MODELIST)
                        {
                            /* LIST-VIEW HANDLING */

                            LONG clickColumn = -1;
    
                            LONG x = _mleft(obj) - data->icld_ViewX + LINE_SPACING_LEFT;
                            LONG index, w, i;

                            /* Find column in which click happened */
                            for(i = 0; i < NUM_COLUMNS; i++)
                            {
                                index = data->icld_LVMAttribs->lmva_ColumnPos[i];
                                
                                if (!(data->icld_LVMAttribs->lmva_ColumnFlags[index] & LVMCF_COLVISIBLE)) continue;
                                
                                w = data->icld_LVMAttribs->lmva_ColumnWidth[index];
                                
                                if ((mx >= x) && (mx < x + w))
                                {
                                    clickColumn = index;
                                    break;
                                }
                                x += w;
                            }
        
                            if (((data->icld_LVMAttribs->lvma_Flags & LVMAF_NOHEADER) == 0) && (my <= data->icld_LVMAttribs->lmva_HeaderHeight))
                            {
                                /* Click on header, update list */
                                data->icld_LVMAttribs->lmva_LastSelectedColumn = clickColumn;

                                data->icld_UpdateMode = UPDATE_HEADERENTRY;
                                data->update_entry = (APTR)(IPTR)clickColumn;

                                MUI_Redraw(obj, MADF_DRAWUPDATE);

                            }
                            else
                            {
                                LONG current = 0, index = (my - data->icld_LVMAttribs->lmva_HeaderHeight + data->icld_ViewY) / data->icld_LVMAttribs->lmva_RowHeight;

                                /* Check if clicked on entry */
                                ForeachNode(&data->icld_IconList, node)
                                {
                                    if (node->ie_Flags & ICONENTRY_FLAG_VISIBLE)
                                    {
                                        /* Is this node clicked? */
                                        if (current == index)
                                        {
                                            new_selected = node;
                                            break;
                                        }

                                        current++;
                                    }
                                }

                                /* Handle actions */
                                IconList_HandleNewIconSelection(CLASS, obj, message, new_selected, &doubleclicked);
                            }
                        }
                        else
                        {
                            /* ICON-VIEW HANDLING */

                            struct Rectangle     rect;

                            /* Check if clicked on entry */
#if defined(__AROS__)
                            ForeachNode(&data->icld_IconList, node)
#else
                            Foreach_Node(&data->icld_IconList, node);
#endif
                            {
                                if (node->ie_Flags & ICONENTRY_FLAG_VISIBLE)
                                {
                                    /* Is this node clicked? */
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
                                        (((my + data->icld_ViewY) >= rect.MinY) && ((my + data->icld_ViewY) <= rect.MaxY )))
                                    {
                                        new_selected = node;
#if defined(DEBUG_ILC_EVENTS)
                                        D(bug("[IconList] %s: Entry '%s' clicked on ..\n", __PRETTY_FUNCTION__, node->ie_IconListEntry.label));
#endif
                                        break;
                                    }
                                }
                            }

                            /* Handle actions */
                            IconList_HandleNewIconSelection(CLASS, obj, message, new_selected, &doubleclicked);
                        }
                
                        if (new_selected && (new_selected->ie_Flags & ICONENTRY_FLAG_SELECTED))
                            data->icld_SelectionLastClicked = new_selected;
                        else
                            data->icld_SelectionLastClicked = NULL;

                        data->click_x = mx;
                        data->click_y = my;

                        SET(obj, MUIA_IconList_SelectionChanged, TRUE);

                        data->icld_ClickEvent.shift = !!(message->imsg->Qualifier & (IEQUALIFIER_LSHIFT | IEQUALIFIER_RSHIFT));
                        data->icld_ClickEvent.entry = data->icld_SelectionLastClicked ? &data->icld_SelectionLastClicked->ie_IconListEntry : NULL;
                        SET(obj, MUIA_IconList_Clicked, (IPTR)&data->icld_ClickEvent);

                        if (doubleclicked)
                        {
                            SET(obj, MUIA_IconList_DoubleClick, TRUE);
                        }

                        if ((!data->mouse_pressed) &&
                                (!doubleclicked || (doubleclicked && (data->icld_SelectionLastClicked == NULL))))
                        {
                            data->last_secs = message->imsg->Seconds;
                            data->last_mics = message->imsg->Micros;
            
                            /* After a double click you often open a new window
                            * and since Zune doesn't not support the faking
                            * of SELECTUP events only change the Events
                            * if not doubleclicked on an icon */

                            data->mouse_pressed |= LEFT_BUTTON;

                            /* Start listening to mouse events */
                            if (!(data->ehn.ehn_Events & IDCMP_MOUSEMOVE))
                            {
                                DoMethod(_win(obj), MUIM_Window_RemEventHandler, (IPTR)&data->ehn);
                                data->ehn.ehn_Events |= IDCMP_MOUSEMOVE;
                                DoMethod(_win(obj), MUIM_Window_AddEventHandler, (IPTR)&data->ehn);
                            }
                        }
                        
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

                        /* Start listening to mouse events */
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
                            NullifyLasso(data, obj);
                        }
                        else if (data->icld_LVMAttribs->lmva_LastSelectedColumn != -1)
                        {
                            ULONG        orig_sortflags = data->icld_SortFlags;

                            if (data->icld_LVMAttribs->lmva_SortColumn == data->icld_LVMAttribs->lmva_LastSelectedColumn)
                            {
                                if (data->icld_SortFlags & MUIV_IconList_Sort_Reverse)
                                    data->icld_SortFlags &= ~MUIV_IconList_Sort_Reverse;
                                else
                                    data->icld_SortFlags |= MUIV_IconList_Sort_Reverse;
                            }

                            switch (data->icld_LVMAttribs->lmva_LastSelectedColumn)
                            {
                                case INDEX_NAME:
                                    data->icld_SortFlags &= ~MUIV_IconList_Sort_Orders;
                                    data->icld_SortFlags |= MUIV_IconList_Sort_ByName;
                                    break;

                                case INDEX_SIZE:
                                    data->icld_SortFlags &= ~MUIV_IconList_Sort_Orders;
                                    data->icld_SortFlags |= MUIV_IconList_Sort_BySize;
                                    break;

                                case INDEX_LASTACCESS:
                                    data->icld_SortFlags &= ~MUIV_IconList_Sort_Orders;
                                    data->icld_SortFlags |= MUIV_IconList_Sort_ByDate;
                                    break;
                            }

                            if (orig_sortflags != data->icld_SortFlags)
                            {
                                data->icld_LVMAttribs->lmva_SortColumn = data->icld_LVMAttribs->lmva_LastSelectedColumn;

                                data->icld_LVMAttribs->lmva_LastSelectedColumn = -1;
                            }
                            DoMethod(obj, MUIM_IconList_Sort);

                        }

                        data->mouse_pressed &= ~LEFT_BUTTON;
                    }

                    if (message->imsg->Code == MIDDLEUP)
                    {
                        data->mouse_pressed &= ~MIDDLE_BUTTON;
                    }

                    /* Stop listening to mouse move events is no buttons pressed */
                    if ((data->ehn.ehn_Events & IDCMP_MOUSEMOVE) && !data->mouse_pressed)
                    {
                        DoMethod(_win(obj),MUIM_Window_RemEventHandler, (IPTR)&data->ehn);
                        data->ehn.ehn_Events &= ~IDCMP_MOUSEMOVE;
                        DoMethod(_win(obj),MUIM_Window_AddEventHandler, (IPTR)&data->ehn);
                    }
                }
                break;

            case IDCMP_INTUITICKS:
                {
#if defined(DEBUG_ILC_EVENTS)
                    D(bug("[IconList] %s: IDCMP_INTUITICKS (%d, %d)\n", __PRETTY_FUNCTION__, mx, my));
#endif
                    if ((data->icld_LassoActive == FALSE)||(!(data->mouse_pressed & LEFT_BUTTON)))
                    {
                        break;
                    }
                    if (((mx >= 0) && (mx <= _mwidth(obj))) &&
                        ((my >= 0) && (my <= _mheight(obj))))
                        break;
                }

            case IDCMP_MOUSEMOVE:
#if defined(DEBUG_ILC_EVENTS)
                D(bug("[IconList] %s: IDCMP_MOUSEMOVE\n", __PRETTY_FUNCTION__));
#endif
                if (data->mouse_pressed & LEFT_BUTTON)
                {
                    LONG move_x = mx;
                    LONG move_y = my;

                    if (data->icld_SelectionLastClicked && (data->icld_LassoActive == FALSE) && 
                        ((abs(move_x - data->click_x) >= 2) || (abs(move_y - data->click_y) >= 2)))
                    {
                        LONG touch_x, touch_y;

                        /* Entry(s) being dragged .... */
                        DoMethod(_win(obj),MUIM_Window_RemEventHandler, (IPTR)&data->ehn);
                        data->ehn.ehn_Events &= ~IDCMP_MOUSEMOVE;
                        DoMethod(_win(obj),MUIM_Window_AddEventHandler, (IPTR)&data->ehn);

                        data->mouse_pressed &= ~LEFT_BUTTON;

                        /* Pass view relative coords */
                        touch_x = move_x + data->icld_ViewX;
                        touch_y = move_y + data->icld_ViewY;
                        DoMethod(obj,MUIM_DoDrag, touch_x, touch_y, 0);
                    }
                    else if (data->icld_LassoActive == TRUE)
                    {
#if defined(DEBUG_ILC_EVENTS) || defined(DEBUG_ILC_LASSO)
                        D(bug("[IconList] %s: Update Lasso\n", __PRETTY_FUNCTION__));
#endif
                        /* Lasso active */
                        struct Rectangle    new_lasso,
                                            old_lasso;
                        struct Rectangle    iconrect;

                        struct IconEntry    *node = NULL;
//                        struct IconEntry    *new_selected = NULL; 

                        /* Remove previous Lasso frame */
                        GetAbsoluteLassoRect(data, &old_lasso);                          
                        IconList_InvertLassoOutlines(obj, data, &old_lasso);

                        /* if the mouse leaves our visible area scroll the view */
                        if (mx < 0 || mx >= _mwidth(obj) || my < 0 || my >= _mheight(obj))
                        {
                            LONG newleft = data->icld_ViewX;
                            LONG newtop = data->icld_ViewY;

                            if (mx >= _mwidth(obj)) newleft += (mx - _mwidth(obj));
                               else if (mx < 0) newleft += mx;
                            if (my >= _mheight(obj)) newtop += (my - _mheight(obj));
                               else if (my < 0) newtop += my;

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

                        LONG current = 0, startIndex = 0, endIndex = 0;
                        
                        if ((data->icld_DisplayFlags & ICONLIST_DISP_MODELIST) == ICONLIST_DISP_MODELIST)
                        {
                            LONG    minY = data->icld_LassoRectangle.MinY,
                                    maxY = data->icld_LassoRectangle.MaxY;

                            if (minY > maxY)
                            {
                                minY ^= maxY;
                                maxY ^= minY;
                                minY ^= maxY;
                            }

                            startIndex = ((minY + 1) - data->icld_LVMAttribs->lmva_HeaderHeight) / data->icld_LVMAttribs->lmva_RowHeight;
                            endIndex = ((maxY - 1) - data->icld_LVMAttribs->lmva_HeaderHeight) / data->icld_LVMAttribs->lmva_RowHeight;
                        }

#if defined(__AROS__)
                        ForeachNode(&data->icld_IconList, node)
#else
                        Foreach_Node(&data->icld_IconList, node);
#endif
                        {
                            IPTR update_entry = (IPTR)NULL;

                            if ((data->icld_DisplayFlags & ICONLIST_DISP_MODELIST) == ICONLIST_DISP_MODELIST)
                            {
                                if (node->ie_Flags & ICONENTRY_FLAG_VISIBLE)
                                {
                                    update_entry = FALSE;

                                    if ((current >= startIndex) && (current <= endIndex))
                                    {
                                        //Entry is inside our lasso ..
                                         if (!(node->ie_Flags & ICONENTRY_FLAG_LASSO))
                                         {
                                             /* check if entry was already selected before */
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
                                            update_entry = (IPTR)node;
                                         }
                                    }
                                    else if (node->ie_Flags & ICONENTRY_FLAG_LASSO)
                                    {
                                        //Entry is no longer inside our lasso - revert its selected state
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
                                        update_entry = (IPTR)node;
                                    }

                                    current++;
                                }
                            }
                            else
                            {
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
                                        //Entry is inside our lasso ..
                                         if (!(node->ie_Flags & ICONENTRY_FLAG_LASSO))
                                         {
                                             /* check if entry was already selected before */
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
                                            update_entry = (IPTR)node;
                                         }
                                    } 
                                    else if (node->ie_Flags & ICONENTRY_FLAG_LASSO)
                                    {
                                        //Entry is no longer inside our lasso - revert its selected state
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
                                        update_entry = (IPTR)node;
                                    }
                                }
                            }
                            if (update_entry)
                            {
                                data->icld_UpdateMode = UPDATE_SINGLEENTRY;
                                data->update_entry = (struct IconEntry *)update_entry;
                                MUI_Redraw(obj, MADF_DRAWUPDATE);
                            }
                        }
                        /* Draw Lasso frame */                         
                        IconList_InvertLassoOutlines(obj, data, &new_lasso);                        
                    }

                    return MUI_EventHandlerRC_Eat;
                }
                else if (data->mouse_pressed & MIDDLE_BUTTON)
                {
                    /* Content is being scrolled */
                    LONG newleft, newtop;

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

            case IDCMP_MENUVERIFY:

#if defined(DEBUG_ILC_EVENTS)
        D(bug("[IconList] %s: IDCMP_MENUVERIFY\n", __PRETTY_FUNCTION__));
#endif

                if (data->icld_LassoActive == TRUE)
                {
                    /* Remove the lasso if the right mouse button is pressed */
                    NullifyLasso(data, obj);
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
    struct IconList_DATA        *data = INST_DATA(CLASS, obj);
    struct IconEntry            *node = NULL;
    struct IconList_Entry       *ent = NULL;
    IPTR                        node_successor = (IPTR)NULL;

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
//    struct IconList_DATA        *data = INST_DATA(CLASS, obj);
    struct IconEntry            *node = NULL;

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
    struct IconList_DATA        *data = INST_DATA(CLASS, obj);
    struct MUI_DragImage        *img = NULL;
    LONG                        first_x = -1,
                                first_y = -1;
    BOOL                        transp = XGET(obj, MUIA_IconList_DragImageTransparent);

#if defined(DEBUG_ILC_FUNCS) || defined(DEBUG_ILC_ICONDRAGDROP)
    D(bug("[IconList]: %s()\n", __PRETTY_FUNCTION__));
#endif

    if (!(data->icld_SelectionLastClicked))
        DoSuperMethodA(CLASS, obj, (Msg)message);

    if ((img = (struct MUI_DragImage *)AllocVec(sizeof(struct MUIP_CreateDragImage), MEMF_CLEAR)))
    {
        struct Node      *node = NULL;
        struct IconEntry *entry = NULL;

#if defined(CREATE_FULL_DRAGIMAGE)
#if defined(__AROS__)
        ForeachNode(&data->icld_SelectionList, node)
#else
        Foreach_Node(&data->icld_SelectionList, node);
#endif
        {
            entry = (struct IconEntry *)((IPTR)node - ((IPTR)&entry->ie_SelectionNode - (IPTR)entry));
            if ((entry->ie_Flags & ICONENTRY_FLAG_VISIBLE) && (entry->ie_Flags & ICONENTRY_FLAG_SELECTED))
            {
                if ((data->icld_DisplayFlags & ICONLIST_DISP_MODELIST) != ICONLIST_DISP_MODELIST)
                {
                    if ((first_x == -1) || ((first_x != -1) && (entry->ie_IconX < first_x))) first_x = entry->ie_IconX;
                    if ((first_y == -1) || ((first_y != -1) && (entry->ie_IconY < first_y))) first_y = entry->ie_IconY;

                    if (data->icld__Option_IconListMode == ICON_LISTMODE_ROUGH)
                    {
                        if ((entry->ie_IconX + entry->ie_AreaWidth) > img->width)   img->width = entry->ie_IconX + entry->ie_AreaWidth;
                        if ((entry->ie_IconY + entry->ie_AreaHeight) > img->height) img->height = entry->ie_IconY + entry->ie_AreaHeight;
                    }

                    if (data->icld__Option_IconListMode == ICON_LISTMODE_GRID)
                    {
                        if ((entry->ie_IconX + data->icld_IconAreaLargestWidth) > img->width)   img->width = entry->ie_IconX + data->icld_IconAreaLargestWidth;
                        if ((entry->ie_IconY + data->icld_IconAreaLargestHeight) > img->height) img->height = entry->ie_IconY + data->icld_IconAreaLargestHeight;
                    }
                }
                else
                {
                    img->height += data->icld_LVMAttribs->lmva_RowHeight;
                }
            }
        }
        if ((data->icld_DisplayFlags & ICONLIST_DISP_MODELIST) == ICONLIST_DISP_MODELIST)
        {
            first_x = 0;
            first_y = -message->touchy;
            img->width = data->icld_LVMAttribs->lmva_ColumnWidth[data->icld_LVMAttribs->lmva_ColumnPos[INDEX_TYPE]] +
                            data->icld_LVMAttribs->lmva_ColumnWidth[data->icld_LVMAttribs->lmva_ColumnPos[INDEX_NAME]];
            img->height += 2;
        }
        else
        {
            img->width = (img->width - first_x) + 2;
            img->height = (img->height - first_y) + 2;
        }
#else
        entry = data->icld_SelectionLastClicked;
        if ((data->icld_DisplayFlags & ICONLIST_DISP_MODELIST) == ICONLIST_DISP_MODELIST)
        {
            img->width = _mright(obj) - _mleft(obj);
            img->height = data->icld_LVMAttribs->lmva_RowHeight;
            first_x = 0;
            first_y = 0;
        }
        else
        {
            img->width = entry->ie_IconWidth;
            img->height = entry->ie_IconHeight;
            first_x = entry->ie_IconX;
            first_y = entry->ie_IconY;
        }
#endif

        if (transp)
        {
            /* Request 32-bit, because the image will have alpha channel */
            img->bm = AllocBitMap(img->width, img->height, 32, BMF_CLEAR, NULL);
        }
        else
        {
            LONG depth = GetBitMapAttr(_screen(obj)->RastPort.BitMap, BMA_DEPTH);
            img->bm = AllocBitMap(img->width, img->height, depth, BMF_CLEAR, _screen(obj)->RastPort.BitMap);
        }

        if (img->bm)
        {
            struct RastPort temprp;
            InitRastPort(&temprp);
            temprp.BitMap = img->bm;
            ULONG minY = 0;

#if defined(CREATE_FULL_DRAGIMAGE)
            ForeachNode(&data->icld_SelectionList, node)
            {
                entry = (struct IconEntry *)((IPTR)node - ((IPTR)&entry->ie_SelectionNode - (IPTR)entry));
                if ((entry->ie_Flags & ICONENTRY_FLAG_VISIBLE) && (entry->ie_Flags & ICONENTRY_FLAG_SELECTED))
                {
                    if ((data->icld_DisplayFlags & ICONLIST_DISP_MODELIST) == ICONLIST_DISP_MODELIST)
                    {
                        struct Rectangle field_rect;
                        ULONG selected = entry->ie_Flags & ICONENTRY_FLAG_SELECTED;
                        entry->ie_Flags &= ~ICONENTRY_FLAG_SELECTED; /* Drawing as not selected actually looks better */

                        field_rect.MinX = 0; field_rect.MaxX = img->width - 1;
                        field_rect.MinY = minY; field_rect.MaxY = field_rect.MinY + data->icld_LVMAttribs->lmva_RowHeight - 1;
                        RenderEntryField(obj, data, entry, &field_rect, INDEX_TYPE, TRUE, FALSE, &temprp);

                        field_rect.MinX = data->icld_LVMAttribs->lmva_ColumnWidth[data->icld_LVMAttribs->lmva_ColumnPos[INDEX_TYPE]] - 1;
                        field_rect.MaxX = img->width - 1;
                        field_rect.MinY = minY; field_rect.MaxY = field_rect.MinY + data->icld_LVMAttribs->lmva_RowHeight - 1;
                        RenderEntryField(obj, data, entry, &field_rect, INDEX_NAME, FALSE, FALSE, &temprp);

                        minY += data->icld_LVMAttribs->lmva_RowHeight;

                        entry->ie_Flags |= selected;
                    }
                    else
                    {
                        LONG offsetx , offsety;

                        IconList_GetIconImageOffsets(data, entry, &offsetx, &offsety);

                        DrawIconStateA
                            (
                                &temprp, entry->ie_DiskObj, NULL,
                                (entry->ie_IconX + 1) - first_x + offsetx, (entry->ie_IconY + 1) - first_y + offsety,
                                IDS_SELECTED,
                                __iconList_DrawIconStateTags
                            );
                    }
                }
            }
#else
            if ((data->icld_DisplayFlags & ICONLIST_DISP_MODELIST) == ICONLIST_DISP_MODELIST)
            {
                SetABPenDrMd(&temprp, _pens(obj)[MPEN_SHINE], 0, JAM1);
                RectFill(&temprp, 0, 0, img->width, img->height);
            }
            else
            {
                DrawIconStateA
                    (
                        &temprp, entry->ie_DiskObj, NULL,
                        0, 0,
                        IDS_SELECTED,
                        __iconList_DrawIconStateTags
                    );
            }
#endif
            if (transp)
                RastPortSetAlpha(&temprp, data->click_x, data->click_y, img->width, img->height, 0xC0, RPALPHAFLAT);
            DeinitRastPort(&temprp);
        }

        /* Convert view relative coords to drag image relative. This is done because the "object" that is being
         * dragged is virtual (its a collection of icons) and the coords passed to DoDrag are not relative to this
         * "object"
         */
        img->touchx = first_x + message->touchx;
        img->touchy = first_y + message->touchy;

        if (transp)
            img->flags = MUIF_DRAGIMAGE_SOURCEALPHA;
        else
            img->flags = 0;
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
    struct IconList_DATA        *data = INST_DATA(CLASS, obj);

#if defined(DEBUG_ILC_FUNCS) || defined(DEBUG_ILC_ICONDRAGDROP)
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
#if defined(DEBUG_ILC_FUNCS) || defined(DEBUG_ILC_ICONDRAGDROP)
    D(bug("[IconList]: %s()\n", __PRETTY_FUNCTION__));
#endif
    
    /* TODO: highlight the possible drop target entry .. */
    
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
    struct IconList_DATA        *data = INST_DATA(CLASS, obj);

#if defined(DEBUG_ILC_FUNCS) || defined(DEBUG_ILC_ICONDRAGDROP)
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
        FreeVec(data->icld_DragDropEvent->drop_TargetPath);
        FreeMem(data->icld_DragDropEvent, sizeof(struct IconList_Drop_Event));
        data->icld_DragDropEvent = NULL;
    }

    /* SANITY CHECK: Get first selected entry from SOURCE iconlist */
    DoMethod(message->obj, MUIM_IconList_NextIcon, MUIV_IconList_NextIcon_Selected, (IPTR)&entry);

    if ((entry) && ((IPTR)entry != MUIV_IconList_NextIcon_End))
    {
        /* Ok.. atleast one entry was dropped .. */
        char                             tmp_dirbuff[256];
        BPTR                            tmp_dirlock = (BPTR) NULL;

        BOOL                            iconMove = FALSE;
        struct IconEntry                *node = NULL;
        struct IconEntry                *drop_target_node = NULL;
        STRPTR                          directory_path = NULL;
        struct IconList_Drop_Event      *dragDropEvent = NULL;

        GET(obj, MUIA_IconDrawerList_Drawer, &directory_path);

        /* Properly expand the name incase it uses devices rather than volumes */
        if (directory_path != NULL)
        {
            tmp_dirlock = Lock(directory_path, SHARED_LOCK);
            if (tmp_dirlock)
            {
                if (NameFromLock(tmp_dirlock, tmp_dirbuff, 256) != 0)
                {
                    directory_path = tmp_dirbuff;
                }
                UnLock(tmp_dirlock);
            }
        }
        if ((dragDropEvent = AllocMem(sizeof(struct IconList_Drop_Event), MEMF_CLEAR)) == NULL)
        {
#if defined(DEBUG_ILC_ICONDRAGDROP)
            D(bug("[IconList] %s: Failed to allocate IconList_Drop_Event Storage!\n", __PRETTY_FUNCTION__));
#endif
            goto dragdropdone;
        }
#if defined(DEBUG_ILC_ICONDRAGDROP)
        D(bug("[IconList] %s: Allocated IconList_Drop_Event @ %p\n", __PRETTY_FUNCTION__, dragDropEvent));
#endif

        NewList(&dragDropEvent->drop_SourceList);

        /* go through list and check if dropped on entry */
        int rowCount = 0;

#if defined(__AROS__)
        ForeachNode(&data->icld_IconList, node)
#else
        Foreach_Node(&data->icld_IconList, node);
#endif
        {
            if ((data->icld_DisplayFlags & ICONLIST_DISP_MODELIST) == ICONLIST_DISP_MODELIST)
            {
                if (node->ie_Flags & ICONENTRY_FLAG_VISIBLE)
                {
                    ULONG rowTop = _mtop(obj) + (rowCount * data->icld_LVMAttribs->lmva_RowHeight);
                    rowTop += data->icld_LVMAttribs->lmva_HeaderHeight;

                    if (((message->x > _mleft(obj)) && (message->x < _mright(obj)))
                        && ((message->y > rowTop) && (message->y < (rowTop + data->icld_LVMAttribs->lmva_RowHeight))))
                    {
                        drop_target_node = node;
                        break;
                    }

                    rowCount++;
                }
            }
            else
            {
                struct Rectangle iconbox;
                LONG click_x = message->x - _mleft(obj);
                LONG click_y = message->y - _mtop(obj);
                iconbox.MinX = node->ie_IconX - data->icld_ViewX;
                iconbox.MaxX = (node->ie_IconX + node->ie_AreaWidth) - data->icld_ViewX;
                iconbox.MinY = node->ie_IconY - data->icld_ViewY;
                iconbox.MaxY = (node->ie_IconY + node->ie_AreaHeight)- data->icld_ViewY;

                if ((node->ie_Flags & ICONENTRY_FLAG_VISIBLE) &&
                   (click_x >= iconbox.MinX) && 
                   (click_x <  iconbox.MaxX) &&
                   (click_y >= iconbox.MinY)  && 
                   (click_y <  iconbox.MaxY))
                {
                    drop_target_node = node;
                    break;
                } 
            }
        }

        /* Additional filter - if same window and the target entry is selected (==dragged), then it was intended as a move */
        if ((message->obj == obj) && (drop_target_node) && (drop_target_node->ie_Flags & ICONENTRY_FLAG_SELECTED))
            drop_target_node = NULL;

        if ((drop_target_node != NULL) && 
            ((drop_target_node->ie_IconListEntry.type == ST_SOFTLINK)   ||
             (drop_target_node->ie_IconListEntry.type == ST_ROOT)       ||
             (drop_target_node->ie_IconListEntry.type == ST_USERDIR)    ||
             (drop_target_node->ie_IconListEntry.type == ST_LINKDIR)    ||
             (drop_target_node->ie_IconListEntry.type == ST_FILE)       ||
             (drop_target_node->ie_IconListEntry.type == ST_LINKFILE)))
        {
            /* Dropped on some entry */
            if ((drop_target_node->ie_IconListEntry.type != ST_ROOT) && (drop_target_node->ie_IconListEntry.type != ST_SOFTLINK))
            {
                if (directory_path)
                {
                    int fulllen = strlen(directory_path) + strlen(drop_target_node->ie_IconListEntry.label) + 2;

                    if ((dragDropEvent->drop_TargetPath = AllocVec(fulllen, MEMF_CLEAR)) == NULL)
                    {
                        bug("[IconList] %s: Failed to allocate IconList_Drop_Event->drop_TargetPath Storage!\n", __PRETTY_FUNCTION__);
                        goto dragdropdone;
                    }
                    strcpy(dragDropEvent->drop_TargetPath, directory_path);
                    AddPart(dragDropEvent->drop_TargetPath, drop_target_node->ie_IconListEntry.label, fulllen);
                }
                else
                    goto dragdropdone;
            }
            else
            {
                if ((dragDropEvent->drop_TargetPath = AllocVec(strlen(drop_target_node->ie_IconListEntry.label) + 1, MEMF_CLEAR)) == NULL)
                {
                    bug("[IconList] %s: Failed to allocate IconList_Drop_Event->drop_TargetPath Storage!\n", __PRETTY_FUNCTION__);
                    goto dragdropdone;
                }
                strcpy(dragDropEvent->drop_TargetPath, drop_target_node->ie_IconListEntry.label);
            }

#if defined(DEBUG_ILC_ICONDRAGDROP)
            D(bug("[IconList] %s: Target Entry Full Path = '%s'\n", __PRETTY_FUNCTION__, dragDropEvent->drop_TargetPath));
#endif
            /* mark the Entry the selection was dropped on*/
            //drop_target_node->ie_Flags |= ICONENTRY_FLAG_SELECTED;
            //data->icld_UpdateMode = UPDATE_SINGLEENTRY;
            //data->update_entry = drop_target_node;
            //MUI_Redraw(obj,MADF_DRAWUPDATE);
        }
        else
        {
            /* Not dropped on entry -> get path of DESTINATION iconlist */
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
                D(bug("[IconList] %s: drop entry: Entry Move detected ..\n", __PRETTY_FUNCTION__));
#endif
                iconMove = TRUE;

                /* Adjust entry posiions .. */
#if defined(DEBUG_ILC_ICONDRAGDROP)
                D(bug("[IconList] %s: drop entry: message x,y = %d, %d  click = %d, %d..\n", __PRETTY_FUNCTION__,  message->x,  message->y, data->click_x, data->click_y));
#endif
                LONG offset_x = message->x - (data->click_x + _mleft(obj));
                LONG offset_y = message->y - (data->click_y + _mtop(obj));

                entry = (IPTR)MUIV_IconList_NextIcon_Start;
                while ((IPTR)entry != MUIV_IconList_NextIcon_End)
                {
                    DoMethod(message->obj, MUIM_IconList_NextIcon, MUIV_IconList_NextIcon_Selected, (IPTR)&entry);

                    if ((IPTR)entry != MUIV_IconList_NextIcon_End)
                    {
                        entry->ile_IconEntry->ie_IconX += offset_x;
                        entry->ile_IconEntry->ie_IconY += offset_y;
                        /* Remember new position as provided */
                        DoMethod(obj, MUIM_IconList_PropagateEntryPos, entry->ile_IconEntry);
                    }
                    SET(obj, MUIA_IconList_IconMoved, (IPTR)entry); // Now notify
                }
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
            while ((IPTR)entry != MUIV_IconList_NextIcon_End)
            {
                DoMethod(message->obj, MUIM_IconList_NextIcon, MUIV_IconList_NextIcon_Selected, (IPTR)&entry);

                if ((IPTR)entry != MUIV_IconList_NextIcon_End)
                {
                    struct IconList_Drop_SourceEntry *sourceEntry = NULL;
                    sourceEntry = AllocMem(sizeof(struct IconList_Drop_SourceEntry), MEMF_CLEAR);
                    if ((entry->type != ST_ROOT) && (entry->type != ST_SOFTLINK))
                    {
                        int fulllen = 0;
                        char *path = NULL;

                        GET(message->obj, MUIA_IconDrawerList_Drawer, &path);
                        /* Properly expand the location in case it uses devices rather than volumes */
                        if (path != NULL)
                        {
                            tmp_dirlock = Lock(path, SHARED_LOCK);
                            if (tmp_dirlock)
                            {
                                if (NameFromLock(tmp_dirlock, tmp_dirbuff, 256))
                                {
                                    path = tmp_dirbuff;
                                }
                                UnLock(tmp_dirlock);
                            }

                            if (strcasecmp(dragDropEvent->drop_TargetPath, path) != 0)
                            {
                                fulllen = strlen(path) + strlen(entry->ile_IconEntry->ie_IconNode.ln_Name) + 2;
                                sourceEntry->dropse_Node.ln_Name = AllocVec(fulllen, MEMF_CLEAR);
                                if (sourceEntry->dropse_Node.ln_Name != NULL)
                                {
                                    strcpy(sourceEntry->dropse_Node.ln_Name, path);
                                    AddPart(sourceEntry->dropse_Node.ln_Name, entry->label, fulllen);
                                }
#if defined(DEBUG_ILC_ICONDRAGDROP)
                                D(bug("[IconList] %s: Source Entry (Full Path) = '%s'\n", __PRETTY_FUNCTION__, sourceEntry->dropse_Node.ln_Name));
#endif
                            }
                        }
                    }
                    else
                    {
                        sourceEntry->dropse_Node.ln_Name = AllocVec(strlen(entry->label) + 1, MEMF_CLEAR);
                        if (sourceEntry->dropse_Node.ln_Name != NULL)
                            strcpy(sourceEntry->dropse_Node.ln_Name, entry->label);
#if defined(DEBUG_ILC_ICONDRAGDROP)
                        D(bug("[IconList] %s: Source Entry = '%s'\n", __PRETTY_FUNCTION__, sourceEntry->dropse_Node.ln_Name));
#endif
                    }
                    
                    if ((sourceEntry->dropse_Node.ln_Name != NULL) && (strcasecmp(dragDropEvent->drop_TargetPath, sourceEntry->dropse_Node.ln_Name) != 0))
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
                dragDropEvent->drop_TargetObj = obj;

#if defined(DEBUG_ILC_ICONDRAGDROP)
                D(bug("[IconList] %s: Causing DROP notification..\n", __PRETTY_FUNCTION__));
#endif
                SET(obj, MUIA_IconList_IconsDropped, (IPTR)dragDropEvent);
                DoMethod(obj, MUIM_IconList_CoordsSort);
            }
            else
            {
                FreeVec(dragDropEvent->drop_TargetPath);
                FreeMem(dragDropEvent, sizeof(struct IconList_Drop_Event));
            }
        }
    }
    else
    {
#if defined(DEBUG_ILC_ICONDRAGDROP)
        D(bug("[IconList] %s: BUG - DragDrop received with no source icons!\n", __PRETTY_FUNCTION__));
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
    struct IconList_DATA        *data = INST_DATA(CLASS, obj);
    struct Node                 *node = NULL, *next_node = NULL;
    BOOL                        changed = FALSE;

#if defined(DEBUG_ILC_FUNCS)
    D(bug("[IconList]: %s()\n", __PRETTY_FUNCTION__));
#endif

    data->icld_SelectionLastClicked = NULL;
    data->icld_FocusIcon = NULL;
#if defined(__AROS__)
    ForeachNodeSafe(&data->icld_SelectionList, node, next_node)
#else
    Foreach_NodeSafe(&data->icld_SelectionList, node, next_node);
#endif
    {
        struct IconEntry    *entry = (struct IconEntry *)((IPTR)node - ((IPTR)&entry->ie_SelectionNode - (IPTR)entry));
        BOOL                update_entry = FALSE;

        if (entry->ie_Flags & ICONENTRY_FLAG_VISIBLE)
        {
            if (entry->ie_Flags & ICONENTRY_FLAG_SELECTED)
            {
                Remove(node);
                entry->ie_Flags &= ~ICONENTRY_FLAG_SELECTED;
                update_entry = TRUE;
            }
            if (entry->ie_Flags & ICONENTRY_FLAG_FOCUS)
            {
                entry->ie_Flags &= ~ICONENTRY_FLAG_FOCUS;
                update_entry = TRUE;
            }
        }

        if (update_entry)
        {
                        changed = TRUE;
            data->icld_UpdateMode = UPDATE_SINGLEENTRY;
            data->update_entry = entry;
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
    struct IconList_DATA         *data = INST_DATA(CLASS, obj);
    struct IconEntry            *node = NULL;
    BOOL                        changed = FALSE;

#if defined(DEBUG_ILC_FUNCS)
    D(bug("[IconList]: %s()\n", __PRETTY_FUNCTION__));
#endif

    node = (struct IconEntry *)GetHead(&data->icld_IconList);

    while (node != NULL)
    {
        if (node->ie_Flags & ICONENTRY_FLAG_VISIBLE)
        {
            BOOL update_entry = FALSE;

            if (!(node->ie_Flags & ICONENTRY_FLAG_SELECTED))
            {
                AddTail(&data->icld_SelectionList, &node->ie_SelectionNode);
                node->ie_Flags |= ICONENTRY_FLAG_SELECTED;
                update_entry = TRUE;

                data->icld_SelectionLastClicked = node;
            }
            else if (node->ie_Flags & ICONENTRY_FLAG_FOCUS)
            {
                node->ie_Flags &= ~ICONENTRY_FLAG_FOCUS;
                update_entry = TRUE;
            }

            if (update_entry)
            {
                changed = TRUE;
                data->icld_UpdateMode = UPDATE_SINGLEENTRY;
                data->update_entry = node;
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
            data->icld_UpdateMode = UPDATE_SINGLEENTRY;
            data->update_entry = data->icld_FocusIcon;
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
    struct IconList_DATA        *data = INST_DATA(CLASS, obj);

    struct IconEntry            *entry = NULL,
                                *test_icon = NULL;

    struct List                 list_VisibleIcons;
    struct List                 list_HiddenIcons;
    // struct List                 list_UnplacedIcons;

    /*
        perform a quick sort of the iconlist based on entry coords
        this method DOESNT cause any visual output.
    */
#if defined(DEBUG_ILC_FUNCS) || defined(DEBUG_ILC_ICONSORTING)
    D(bug("[IconList]: %s()\n", __PRETTY_FUNCTION__));
#endif

    NewList((struct List*)&list_VisibleIcons);
    NewList((struct List*)&list_HiddenIcons);

    /*move list into our local list struct(s)*/
    while ((entry = (struct IconEntry *)RemTail((struct List*)&data->icld_IconList)))
    {
        if (entry->ie_Flags & ICONENTRY_FLAG_VISIBLE)
        {
            AddHead((struct List*)&list_VisibleIcons, (struct Node *)&entry->ie_IconNode);
        }
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
#if defined(__AROS__)
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
    struct IconList_DATA        *data = INST_DATA(CLASS, obj);
    struct IconEntry            *entry = NULL,
                                *icon1 = NULL,
                                *icon2 = NULL;

    struct List                 list_VisibleIcons,
                                list_SortedIcons,
                                list_HiddenIcons;

    BOOL                        sortme, reversable = TRUE, enqueue = FALSE;
    int                         i, visible_count = 0;

#if defined(DEBUG_ILC_FUNCS) || defined(DEBUG_ILC_ICONSORTING)
    D(bug("[IconList]: %s()\n", __PRETTY_FUNCTION__));
#endif

    /* Reset incase view options have changed .. */
    data->icld_IconAreaLargestWidth = 0;
    data->icld_IconAreaLargestHeight = 0;
    data->icld_IconLargestHeight = 0;
    data->icld_LabelLargestHeight = 0;

#if defined(DEBUG_ILC_ICONSORTING)
    D(bug("[IconList] %s: Sort-Flags : %x\n", __PRETTY_FUNCTION__, (data->icld_SortFlags & MUIV_IconList_Sort_MASK)));
#endif
    NewList((struct List*)&list_VisibleIcons);
    NewList((struct List*)&list_SortedIcons);
    NewList((struct List*)&list_HiddenIcons);

    /*move list into our local list struct(s)*/
    while ((entry = (struct IconEntry *)RemTail((struct List*)&data->icld_IconList)))
    {
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

            if (((data->icld_SortFlags & MUIV_IconList_Sort_AutoSort) == 0) && (entry->ie_ProvidedIconX == NO_ICON_POSITION))
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

    /* Copy each visible entry back to the main list, sorting as we go*/
    while ((entry = (struct IconEntry *)RemHead((struct List*)&list_VisibleIcons)))
    {
        icon1 = (struct IconEntry *)GetHead(&list_SortedIcons);
        icon2 = NULL;

        sortme = FALSE;

        if (visible_count > 1)
        {
#if defined(DEBUG_ILC_ICONSORTING)
            D(bug("[IconList] %s:  - %s %s %s %i\n", __PRETTY_FUNCTION__, entry->ie_IconListEntry.label, entry->ie_TxtBuf_DATE, entry->ie_TxtBuf_TIME, entry->ie_FileInfoBlock->fib_Size));
#endif
            while (icon1)
            {
                if(((icon1->ie_IconListEntry.type == ST_ROOT) || (icon1->ie_IconListEntry.type == ST_LINKDIR) || (icon1->ie_IconListEntry.type == ST_LINKFILE))
                    || (data->icld_SortFlags & MUIV_IconList_Sort_DrawersMixed))
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
#if defined(DEBUG_ILC_ICONSORTING)
                                D(bug("[IconList] %s: force %s\n", __PRETTY_FUNCTION__, entry->ie_IconListEntry.label));
#endif
                                break;
                            }
                        }
                    }
                }

                if (sortme)
                {
                    i = 0;
            
                    if ((data->icld_SortFlags & MUIV_IconList_Sort_AutoSort) == 0)
                    {
                        if ((entry->ie_ProvidedIconX != NO_ICON_POSITION) &&  (entry->ie_ProvidedIconY != NO_ICON_POSITION))
                        {
                            i = 1;
                        }
                    }
                    
                    if (i == 0)
                    {
                        if (data->icld_SortFlags & MUIV_IconList_Sort_ByDate)
                        {
                            /* Sort by Date */
                            i = CompareDates((const struct DateStamp *)&entry->ie_FileInfoBlock->fib_Date,(const struct DateStamp *)&icon1->ie_FileInfoBlock->fib_Date);
                        }
                        else if (data->icld_SortFlags & MUIV_IconList_Sort_BySize)
                        {
                            /* Sort by Size .. */
                            i = entry->ie_FileInfoBlock->fib_Size - icon1->ie_FileInfoBlock->fib_Size;
                        }
                        else if ((data->icld_SortFlags & MUIV_IconList_Sort_ByType) && ((entry->ie_IconListEntry.type == ST_FILE) || (entry->ie_IconListEntry.type == ST_USERDIR)))
                        {
                           /* Sort by Type .. */
                            /* TODO: Sort icons based on type using datatypes */
                        }
                        else
                        {
                            /* Sort by Name .. */
                            i = Stricmp(entry->ie_IconListEntry.label, icon1->ie_IconListEntry.label);
                            if ((data->icld_SortFlags & MUIV_IconList_Sort_DrawersMixed) == 0) enqueue = TRUE;
                        }
                    }

                    if ((reversable) && data->icld_SortFlags & MUIV_IconList_Sort_Reverse)
                    {
                        if (i > 0)
                            break;
                    }
                    else if (i < 0)
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

    DoMethod(obj, MUIM_IconList_PositionIcons);
    MUI_Redraw(obj, MADF_DRAWOBJECT);

    if ((data->icld_SortFlags & MUIV_IconList_Sort_Orders) != 0)
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
    struct Window       *wnd = _window(obj);
    struct Layer        *l = NULL;

#if defined(DEBUG_ILC_FUNCS) || defined(DEBUG_ILC_ICONDRAGDROP)
    D(bug("[IconList]: %s()\n", __PRETTY_FUNCTION__));
#endif

    LockLayerInfo(&wnd->WScreen->LayerInfo);
    l = WhichLayer(&wnd->WScreen->LayerInfo, wnd->LeftEdge + message->x, wnd->TopEdge + message->y);
    UnlockLayerInfo(&wnd->WScreen->LayerInfo);

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
#if defined(DEBUG_ILC_FUNCS) || defined(DEBUG_ILC_ICONDRAGDROP)
    D(bug("[IconList]: %s()\n", __PRETTY_FUNCTION__));
#endif
#if defined(DEBUG_ILC_ICONDRAGDROP)
    D(bug("[IconList] %s: icons dropped on custom window \n", __PRETTY_FUNCTION__));
#endif

    SET(obj, MUIA_IconList_AppWindowDrop, (IPTR)message); /* Now notify */

    return (IPTR)NULL;
}
///

///MUIM_IconList_MakeEntryVisible()
/**************************************************************************
 Move the visible area so that the selected entry becomes visible ..
**************************************************************************/
IPTR IconList__MUIM_IconList_MakeEntryVisible(struct IClass *CLASS, Object *obj, struct MUIP_IconList_MakeEntryVisible *message)
{
    struct IconList_DATA        *data = INST_DATA(CLASS, obj);
    BOOL                        viewmoved = FALSE;
    struct Rectangle            iconrect, viewrect;

#if defined(DEBUG_ILC_FUNCS)
D(bug("[IconList]: %s()\n", __PRETTY_FUNCTION__));
#endif

    viewrect.MinX = data->icld_ViewX;
    viewrect.MaxX = data->icld_ViewX + data->icld_AreaWidth;
    viewrect.MinY = data->icld_ViewY;
    viewrect.MaxY = data->icld_ViewY + data->icld_AreaHeight;

    IconList_GetIconAreaRectangle(obj, data, message->entry, &iconrect);

    if (!(RectAndRect(&viewrect, &iconrect)))
    {
        viewmoved = TRUE;
        if (message->entry->ie_IconX < data->icld_ViewX)
            data->icld_ViewX = message->entry->ie_IconX;
        else if (message->entry->ie_IconX > (data->icld_ViewX + data->icld_AreaWidth))
            data->icld_ViewX = (message->entry->ie_IconX + message->entry->ie_AreaWidth) - data->icld_AreaWidth;

        if (message->entry->ie_IconY < data->icld_ViewY)
            data->icld_ViewY = message->entry->ie_IconX;
        else if (message->entry->ie_IconY > (data->icld_ViewY + data->icld_AreaHeight))
            data->icld_ViewY = (message->entry->ie_IconY + message->entry->ie_AreaHeight) - data->icld_AreaHeight;
    }

    if (viewmoved)
    {
#if defined(DEBUG_ILC_ICONRENDERING)
        D(bug("[IconList]: %s: call SetSuperAttrs()\n", __PRETTY_FUNCTION__));
#endif
        SetSuperAttrs(CLASS, obj, MUIA_Virtgroup_Left, data->icld_ViewX,
                        MUIA_Virtgroup_Top, data->icld_ViewY,
                        TAG_DONE);

#if defined(DEBUG_ILC_ICONRENDERING)
        D(bug("[IconList]: %s: call SetAttrs()\n", __PRETTY_FUNCTION__));
#endif
        SetAttrs(obj, MUIA_Virtgroup_Left, data->icld_ViewX,
                        MUIA_Virtgroup_Top, data->icld_ViewY,
                        TAG_DONE);

#if defined(DEBUG_ILC_ICONRENDERING)
        D(bug("[IconList]: %s: call MUI_Redraw()\n", __PRETTY_FUNCTION__));
#endif
        MUI_Redraw(obj,MADF_DRAWOBJECT);
    }
    return 1;
}

#if defined(WANDERER_BUILTIN_ICONLIST)
BOOPSI_DISPATCHER(IPTR,IconList_Dispatcher, CLASS, obj, message)
{
#if defined(__AROS__)
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
               case OM_ADDMEMBER:
        case MUIM_Family_AddTail:               return IconList__MUIM_Family_AddTail(CLASS, obj, (APTR)message);
        case MUIM_Family_AddHead:               return IconList__MUIM_Family_AddHead(CLASS, obj, (APTR)message);
        case OM_REMMEMBER:
        case MUIM_Family_Remove:                return IconList__MUIM_Family_Remove(CLASS, obj, (APTR)message);

        case MUIM_Setup:                        return IconList__MUIM_Setup(CLASS, obj, (struct MUIP_Setup *)message);

        case MUIM_Show:                         return IconList__MUIM_Show(CLASS,obj, (struct MUIP_Show *)message);
        case MUIM_Hide:                         return IconList__MUIM_Hide(CLASS,obj, (struct MUIP_Hide *)message);
        case MUIM_Cleanup:                      return IconList__MUIM_Cleanup(CLASS, obj, (struct MUIP_Cleanup *)message);
        case MUIM_AskMinMax:                    return IconList__MUIM_AskMinMax(CLASS, obj, (struct MUIP_AskMinMax *)message);
        case MUIM_Draw:                         return IconList__MUIM_Draw(CLASS, obj, (struct MUIP_Draw *)message);
#if defined(__AROS__)
        case MUIM_Layout:                       return IconList__MUIM_Layout(CLASS, obj, (struct MUIP_Layout *)message);
#endif
        case MUIM_HandleEvent:                  return IconList__MUIM_HandleEvent(CLASS, obj, (struct MUIP_HandleEvent *)message);
        case MUIM_CreateDragImage:              return IconList__MUIM_CreateDragImage(CLASS, obj, (APTR)message);
        case MUIM_DeleteDragImage:              return IconList__MUIM_DeleteDragImage(CLASS, obj, (APTR)message);
        case MUIM_DragQuery:                    return IconList__MUIM_DragQuery(CLASS, obj, (APTR)message);
        case MUIM_DragReport:                   return IconList__MUIM_DragReport(CLASS, obj, (APTR)message);
        case MUIM_DragDrop:                     return IconList__MUIM_DragDrop(CLASS, obj, (APTR)message);
#if defined(__AROS__)
        case MUIM_UnknownDropDestination:       return IconList__MUIM_UnknownDropDestination(CLASS, obj, (APTR)message);       
#endif
        case MUIM_IconList_Update:              return IconList__MUIM_IconList_Update(CLASS, obj, (APTR)message);
        case MUIM_IconList_Clear:               return IconList__MUIM_IconList_Clear(CLASS, obj, (APTR)message);
        case MUIM_IconList_RethinkDimensions:   return IconList__MUIM_IconList_RethinkDimensions(CLASS, obj, (APTR)message);
        case MUIM_IconList_CreateEntry:         return IconList__MUIM_IconList_CreateEntry(CLASS, obj, (APTR)message);
        case MUIM_IconList_UpdateEntry:         return IconList__MUIM_IconList_UpdateEntry(CLASS, obj, (APTR)message);
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
        case MUIM_IconList_MakeEntryVisible:    return IconList__MUIM_IconList_MakeEntryVisible(CLASS, obj, (APTR)message);
    }

    return DoSuperMethodA(CLASS, obj, message);
}
BOOPSI_DISPATCHER_END

#if defined(__AROS__)
/* Class descriptor. */
const struct __MUIBuiltinClass _MUI_IconList_desc = { 
    MUIC_IconList, 
    MUIC_Area, 
    sizeof(struct IconList_DATA), 
    (void*)IconList_Dispatcher
};
#endif
#endif /* WANDERER_BUILTIN_ICONLIST */

#if !defined(__AROS__)
struct MUI_CustomClass  *initIconListClass(void)
{
    return (struct MUI_CustomClass *) MUI_CreateCustomClass(NULL, MUIC_Area, NULL, sizeof(struct IconList_DATA), ENTRY(IconList_Dispatcher));
}
#endif
