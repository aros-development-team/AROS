/*
    Copyright © 2008-2013, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "../portable_macros.h"
#ifndef __AROS__
#define WANDERER_BUILTIN_ICONLIST 1
#else
#define DEBUG 0
#include <aros/debug.h>
#endif

#define DEBUG_ILC_EVENTS
#define DEBUG_ILC_KEYEVENTS
#define DEBUG_ILC_ICONRENDERING
#define DEBUG_ILC_ICONSORTING
#define DEBUG_ILC_ICONSORTING_DUMP
#define DEBUG_ILC_ICONPOSITIONING
#define DEBUG_ILC_LASSO
#define DEBUG_ILC_MEMALLOC

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
#include "icon_attributes.h"
#include "icon.h"
#include "icon_private.h"

#include "iconlist_attributes.h"

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

struct Hook             __icon_UpdateLabels_hook;

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

///RectAndRect()
// Icon/Label Area support functions
int RectAndRect(struct Rectangle *a, struct Rectangle *b)
{
    if ((a->MinX > b->MaxX) || (a->MinY > b->MaxY) || (a->MaxX < b->MinX) || (a->MaxY < b->MinY))
        return 0;
    return 1;
}
///

///Icon_GetIconImageRectangle()
//We don't use icon.library's label drawing so we do this by hand
static void Icon_GetIconImageRectangle(Object *obj, struct Icon_DATA *data, struct Rectangle *rect)
{
#if defined(DEBUG_ILC_ICONPOSITIONING)
D(bug("[Icon]: %s(icon @ %p)\n", __PRETTY_FUNCTION__, icon));
#endif

    /* Get basic width/height */    
    GetIconRectangleA(NULL, data->IcD_DiskObj, NULL, rect, __iconList_DrawIconStateTags);
#if defined(DEBUG_ILC_ICONPOSITIONING)
D(bug("[Icon] %s: MinX %d, MinY %d      MaxX %d, MaxY %d\n", __PRETTY_FUNCTION__, rect->MinX, rect->MinY, rect->MaxX, rect->MaxY));
#endif
    data->IcD_IconWidth  = (rect->MaxX - rect->MinX) + 1;
    data->IcD_IconHeight = (rect->MaxY - rect->MinY) + 1;
    
//    if (data->IcD_IconHeight > data->icld_IconLargestHeight)
//        data->icld_IconLargestHeight = icon->IcD_IconHeight;
}
///

///Icon_GetIconLabelRectangle()
static void Icon_GetIconLabelRectangle(Object *obj, struct Icon_DATA *data, struct Rectangle *rect)
{
    ULONG      outline_offset = 0;
    ULONG      textwidth = 0;

#if defined(DEBUG_ILC_ICONPOSITIONING)
D(bug("[Icon]: %s()\n", __PRETTY_FUNCTION__));
#endif

    switch ( XGET(_parent(obj), MUIA_IconList_LabelText_Mode) )
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
    if ((icon->IcD_Label_TXTBUFF != NULL) && (icon->IcD_DisplayedLabel_TXTBUFF != NULL))
    {
  ULONG curlabel_TotalLines;
        SetFont(data->icld_BufferRastPort, data->icld_IconLabelFont);
        
        rect->MinX = 0;
        rect->MaxX = (((data->icld__Option_LabelTextHorizontalPadding + data->icld__Option_LabelTextBorderWidth) * 2) + icon->IcD_DisplayedLabel_Width + outline_offset) - 1;
    
        rect->MinY = 0;

        curlabel_TotalLines = icon->IcD_DisplayedLabel_SplitParts;
        if (curlabel_TotalLines == 0)
            curlabel_TotalLines = 1;
        if (curlabel_TotalLines > XGET(_parent(obj), MUIA_Icon_LabelText_MultiLine))
            curlabel_TotalLines = XGET(_parent(obj), MUIA_Icon_LabelText_MultiLine);
        
        rect->MaxY = (((data->icld__Option_LabelTextBorderHeight + data->icld__Option_LabelTextVerticalPadding) * 2) + 
                     ((data->icld_IconLabelFont->tf_YSize + outline_offset) * curlabel_TotalLines)) - 1;

        /*  Date/size sorting has the date/size appended under the icon label
            only list regular files like this (drawers have no size/date output) */
        if(
            icon->IcD_IconEntry.type != ST_USERDIR && 
            ((data->icld_SortFlags & ICONLIST_SORT_BY_SIZE) || (data->icld_SortFlags & ICONLIST_SORT_BY_DATE))
        )
        {
            SetFont(data->icld_BufferRastPort, data->icld_IconInfoFont);

            if( (data->icld_SortFlags & ICONLIST_SORT_BY_SIZE) && !(data->icld_SortFlags & ICONLIST_SORT_BY_DATE) )
            {
                icon->IcD_Size_Width = TextLength(data->icld_BufferRastPort, icon->IcD_Size_TXTBUFF, strlen(icon->IcD_Size_TXTBUFF));
                textwidth = icon->IcD_Size_Width;
            }
            else
            {
                if( !(data->icld_SortFlags & ICONLIST_SORT_BY_SIZE) && (data->icld_SortFlags & ICONLIST_SORT_BY_DATE) )
                {
                    if( icon->IcD_Flags & ICONENTRY_FLAG_TODAY )
                    {
                        icon->IcD_Time_Width = TextLength(data->icld_BufferRastPort, icon->IcD_Time_TXTBUFF, strlen(icon->IcD_Time_TXTBUFF));
                        textwidth = icon->IcD_Time_Width;
                    }
                    else
                    {
                        icon->IcD_Date_Width = TextLength(data->icld_BufferRastPort, icon->IcD_Date_TXTBUFF, strlen(icon->IcD_Date_TXTBUFF));
                        textwidth = icon->IcD_Date_Width;
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

///Icon_GetIconAreaRectangle()
static void Icon_GetIconAreaRectangle(Object *obj, struct Icon_DATA *data, struct IconEntry *icon, struct Rectangle *rect)
{
    struct Rectangle labelrect;
    ULONG iconlabel_Width;
    ULONG iconlabel_Height;

#if defined(DEBUG_ILC_ICONPOSITIONING)
D(bug("[Icon]: %s()\n", __PRETTY_FUNCTION__));
#endif

    /* Get icon box width including text width */
    memset(rect, 0, sizeof(struct Rectangle));

    Icon_GetIconImageRectangle(obj, data, icon, rect);

    icon->IcD_AreaWidth = icon->IcD_IconWidth;
    if (data->icld__Option_IconMode == ICON_LISTMODE_GRID)
    {
        icon->IcD_AreaHeight = data->icld_IconLargestHeight;
    }
    else
    {
        icon->IcD_AreaHeight = icon->IcD_IconHeight;
    }

    Icon_GetIconLabelRectangle(obj, data, icon, &labelrect);

    iconlabel_Width = ((labelrect.MaxX - labelrect.MinX) + 1);
    iconlabel_Height = ((labelrect.MaxY - labelrect.MinY) + 1);
    
    if (iconlabel_Width > icon->IcD_AreaWidth)
        icon->IcD_AreaWidth = iconlabel_Width;

    icon->IcD_AreaHeight = icon->IcD_AreaHeight + data->icld__Option_IconImageSpacing + iconlabel_Height;
    
    /* Store */
    rect->MaxX = (rect->MinX + icon->IcD_AreaWidth) - 1;
    rect->MaxY = (rect->MinY + icon->IcD_AreaHeight) - 1;
    
    if (icon->IcD_AreaWidth > data->icld_IconAreaLargestWidth) data->icld_IconAreaLargestWidth = icon->IcD_AreaWidth;
    if (icon->IcD_AreaHeight > data->icld_IconAreaLargestHeight) data->icld_IconAreaLargestHeight = icon->IcD_AreaHeight;
}
///
/**************************************************************************
Draw the icon at its position
**************************************************************************/
///Icon__MUIM_Icon_DrawEntry()
IPTR Icon__MUIM_Icon_DrawEntry(struct IClass *CLASS, Object *obj, struct MUIP_Icon_DrawEntry *message)
{
    struct Icon_DATA *data = INST_DATA(CLASS, obj);

    struct Rectangle iconrect;
    struct Rectangle objrect;

    //LONG tx,ty;
    LONG offsetx,offsety;
    //LONG txwidth, txheight;

    ULONG iconX;
    ULONG iconY;

#if defined(DEBUG_ILC_ICONRENDERING)
D(bug("[Icon]: %s(message->icon = 0x%p)\n", __PRETTY_FUNCTION__, message->icon));
#endif

    if ((!(message->icon->IcD_Flags & ICONENTRY_FLAG_VISIBLE)) ||
        (data->icld_BufferRastPort == NULL) ||
        (!(message->icon->IcD_DiskObj)))
    {
#if defined(DEBUG_ILC_ICONRENDERING)
D(bug("[Icon] %s: Not visible or missing DOB\n", __PRETTY_FUNCTION__));
#endif
        return FALSE;
    }
    
    /* Get the dimensions and affected area of message->icon */
    Icon_GetIconImageRectangle(obj, data, message->icon, &iconrect);

    /* Add the relative position offset of the message->icon */
    offsetx = _mleft(obj) - data->icld_ViewX + message->icon->IcD_IconX;
    /* Centre our image with our text */
    if (message->icon->IcD_IconWidth < message->icon->IcD_AreaWidth)
        offsetx += (message->icon->IcD_AreaWidth - message->icon->IcD_IconWidth)/2;

    if ((data->icld__Option_IconMode == ICON_LISTMODE_GRID) &&
        (message->icon->IcD_AreaWidth < data->icld_IconAreaLargestWidth))
        offsetx += ((data->icld_IconAreaLargestWidth - message->icon->IcD_AreaWidth)/2);

    iconrect.MinX += offsetx;
    iconrect.MaxX += offsetx;

    offsety = _mtop(obj) - data->icld_ViewY + message->icon->IcD_IconY;
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
    
    // Center icon image
    iconX = iconrect.MinX - _mleft(obj) + data->icld_DrawOffsetX;
    iconY = iconrect.MinY - _mtop(obj) + data->icld_DrawOffsetY;

    DrawIconStateA
        (
            data->icld_BufferRastPort ? data->icld_BufferRastPort : data->icld_BufferRastPort, message->icon->IcD_DiskObj, NULL,
            iconX, 
            iconY, 
            (message->icon->IcD_Flags & ICONENTRY_FLAG_SELECTED) ? IDS_SELECTED : IDS_NORMAL,
            __iconList_DrawIconStateTags
        );

    return TRUE;
}
///

///Icon__LabelFunc_SplitLabel()
void Icon__LabelFunc_SplitLabel(Object *obj, struct Icon_DATA *data, struct IconEntry *icon)
{
    ULONG       labelSplit_MaxLabelLineLength = XGET(_parent(obj), MUIA_Icon_LabelText_MaxLineLen);
    ULONG       labelSplit_LabelLength = strlen(icon->IcD_Label_TXTBUFF);
    ULONG       txwidth;
    ULONG       labelSplit_FontY = data->icld_IconLabelFont->tf_YSize;
     int        labelSplit_CharsDone,   labelSplit_CharsSplit;
    ULONG         labelSplit_CurSplitWidth;

    if ((data->icld__Option_TrimVolumeNames) && 
        ((icon->IcD_IconEntry.type == ST_ROOT) && (icon->IcD_Label_TXTBUFF[labelSplit_LabelLength - 1] == ':')))
        labelSplit_LabelLength--;

    if (labelSplit_MaxLabelLineLength >= labelSplit_LabelLength)
    {
#if defined(DEBUG_ILC_ICONRENDERING)
D(bug("[Icon]: %s: Label'%s' doesnt need split (onyl %d chars)\n", __PRETTY_FUNCTION__, icon->IcD_Label_TXTBUFF, labelSplit_LabelLength));
#endif
        return;
    }

    SetFont(data->icld_BufferRastPort, data->icld_IconLabelFont);
    txwidth = TextLength(data->icld_BufferRastPort, icon->IcD_Label_TXTBUFF, labelSplit_MaxLabelLineLength);
#if defined(DEBUG_ILC_ICONRENDERING)
D(bug("[Icon]: %s: txwidth = %d\n", __PRETTY_FUNCTION__, txwidth));
#endif
    icon->IcD_DisplayedLabel_TXTBUFF = AllocVecPooled(data->icld_Pool, 256);
    memset(icon->IcD_DisplayedLabel_TXTBUFF, 0, 256);
    icon->IcD_DisplayedLabel_SplitParts = 0;
    
    labelSplit_CharsDone = 0;
    labelSplit_CharsSplit = 0;

    while (labelSplit_CharsDone < labelSplit_LabelLength)
    {
        ULONG labelSplit_CurSplitLength = labelSplit_LabelLength - labelSplit_CharsDone;
        IPTR  labelSplit_SplitStart = icon->IcD_Label_TXTBUFF + labelSplit_CharsDone;
        int  tmp_checkoffs = 0;
  IPTR   labelSplit_RemainingCharsAfterSplit;
  IPTR labelSplit_CurSplitDest;

        while (*(char *)(labelSplit_SplitStart) == ' ')
        {
            //Skip preceding spaces..
            labelSplit_SplitStart = labelSplit_SplitStart + 1;
            labelSplit_CurSplitLength = labelSplit_CurSplitLength - 1;
            labelSplit_CharsDone = labelSplit_CharsDone + 1;
        }

        while(TextLength(data->icld_BufferRastPort, labelSplit_SplitStart, labelSplit_CurSplitLength) < txwidth) labelSplit_CurSplitLength++;
        while(TextLength(data->icld_BufferRastPort, labelSplit_SplitStart, labelSplit_CurSplitLength) > txwidth) labelSplit_CurSplitLength--;
#if defined(DEBUG_ILC_ICONRENDERING)
D(bug("[Icon]: %s: labelSplit_CurSplitLength = %d\n", __PRETTY_FUNCTION__, labelSplit_CurSplitLength));
#endif

#if defined(DEBUG_ILC_ICONRENDERING)
            D(bug("[Icon]: %s: Attempting to find neat split ", __PRETTY_FUNCTION__));
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
            D(bug("[Icon]: %s: Couldnt find neat split : Still %d chars\n", __PRETTY_FUNCTION__, labelSplit_RemainingCharsAfterSplit));
#endif
            if (labelSplit_RemainingCharsAfterSplit <= ILC_ICONLABEL_SHORTEST)
            {
                labelSplit_CurSplitLength = labelSplit_CurSplitLength + (labelSplit_RemainingCharsAfterSplit - ILC_ICONLABEL_SHORTEST);
            }
        }
        if ((labelSplit_CharsDone + labelSplit_CurSplitLength) > labelSplit_LabelLength) labelSplit_CurSplitLength = labelSplit_LabelLength - labelSplit_CharsDone;

        labelSplit_CurSplitDest = icon->IcD_DisplayedLabel_TXTBUFF + labelSplit_CharsSplit + icon->IcD_DisplayedLabel_SplitParts;
        
        strncpy(labelSplit_CurSplitDest, labelSplit_SplitStart, labelSplit_CurSplitLength);
        
        labelSplit_CurSplitWidth = TextLength(data->icld_BufferRastPort, labelSplit_CurSplitDest, labelSplit_CurSplitLength);
        
        icon->IcD_DisplayedLabel_SplitParts = icon->IcD_DisplayedLabel_SplitParts + 1;
        
        labelSplit_CharsDone = labelSplit_CharsDone + labelSplit_CurSplitLength;
        labelSplit_CharsSplit = labelSplit_CharsSplit + labelSplit_CurSplitLength;
        
        if (labelSplit_CurSplitWidth > icon->IcD_DisplayedLabel_Width) icon->IcD_DisplayedLabel_Width = labelSplit_CurSplitWidth;
    }
    if ((icon->IcD_DisplayedLabel_SplitParts <= 1) && icon->IcD_DisplayedLabel_TXTBUFF)
    {
        FreeVecPooled(data->icld_Pool, icon->IcD_DisplayedLabel_TXTBUFF);
        icon->IcD_DisplayedLabel_TXTBUFF = NULL;
        icon->IcD_DisplayedLabel_SplitParts = 0;
    }
//  if ((labelSplit_FontY * icon->IcD_DisplayedLabel_SplitParts) > data->icld_LabelLargestHeight) data->icld_LabelLargestHeight = (labelSplit_FontY * icon->IcD_DisplayedLabel_SplitParts);
}
///

///Icon__LabelFunc_CreateLabel()
IPTR Icon__LabelFunc_CreateLabel(Object *obj, struct Icon_DATA *data, struct IconEntry *icon)
{
#if defined(DEBUG_ILC_ICONRENDERING)
D(bug("[Icon]: %s('%s')\n", __PRETTY_FUNCTION__, icon->IcD_Label_TXTBUFF));
#endif
    if (icon->IcD_DisplayedLabel_TXTBUFF)
    {
        FreeVecPooled(data->icld_Pool, icon->IcD_DisplayedLabel_TXTBUFF);
        icon->IcD_DisplayedLabel_TXTBUFF = NULL;
        icon->IcD_DisplayedLabel_SplitParts = 0;
    }
    
    if (XGET(_parent(obj), MUIA_Icon_LabelText_MultiLine) > 1)
    {
#if defined(DEBUG_ILC_ICONRENDERING)
D(bug("[Icon]: %s: Attempting to split label ..\n", __PRETTY_FUNCTION__));
#endif
        Icon__LabelFunc_SplitLabel(obj, data, icon);
    }
    
    if (icon->IcD_DisplayedLabel_TXTBUFF == NULL)
    { 
        ULONG IcD_LabelLength = strlen(icon->IcD_Label_TXTBUFF);
        icon->IcD_DisplayedLabel_SplitParts = 1;

#if defined(DEBUG_ILC_ICONRENDERING)
D(bug("[Icon]: %s: Building unsplit label (len = %d) ..\n", __PRETTY_FUNCTION__, IcD_LabelLength));
#endif

        if ((data->icld__Option_TrimVolumeNames) && 
            ((icon->IcD_IconEntry.type == ST_ROOT) && (icon->IcD_Label_TXTBUFF[IcD_LabelLength - 1] == ':')))
            IcD_LabelLength--;

        if(IcD_LabelLength > XGET(_parent(obj), MUIA_Icon_LabelText_MaxLineLen))
        {
            if (!(icon->IcD_DisplayedLabel_TXTBUFF = AllocVecPooled(data->icld_Pool, XGET(_parent(obj), MUIA_Icon_LabelText_MaxLineLen) + 1)))
            {
                return NULL;
            }
            memset(icon->IcD_DisplayedLabel_TXTBUFF, 0, XGET(_parent(obj), MUIA_Icon_LabelText_MaxLineLen) + 1);
            strncpy(icon->IcD_DisplayedLabel_TXTBUFF, icon->IcD_Label_TXTBUFF, XGET(_parent(obj), MUIA_Icon_LabelText_MaxLineLen) - 3);
            strcat(icon->IcD_DisplayedLabel_TXTBUFF , " ..");
        }
        else 
        {
            if (!(icon->IcD_DisplayedLabel_TXTBUFF = AllocVecPooled(data->icld_Pool, IcD_LabelLength + 1)))
            {
                return NULL;
            }
            memset(icon->IcD_DisplayedLabel_TXTBUFF, 0, IcD_LabelLength + 1);
            strncpy(icon->IcD_DisplayedLabel_TXTBUFF, icon->IcD_Label_TXTBUFF, IcD_LabelLength );
        }
        icon->IcD_DisplayedLabel_Width = TextLength(data->icld_BufferRastPort, icon->IcD_DisplayedLabel_TXTBUFF, strlen(icon->IcD_DisplayedLabel_TXTBUFF));
//    if ((data->icld_IconLabelFont->tf_YSize) > data->icld_LabelLargestHeight) data->icld_LabelLargestHeight = data->icld_IconLabelFont->tf_YSize;
    }

//  if (icon->IcD_DisplayedLabel_Width > data->icld_LabelLargestWidth) data->icld_LabelLargestWidth = icon->IcD_DisplayedLabel_Width;

    return icon->IcD_DisplayedLabel_TXTBUFF;
}
///

///Icon__MUIM_Icon_DrawEntryLabel()
IPTR Icon__MUIM_Icon_DrawEntryLabel(struct IClass *CLASS, Object *obj, struct MUIP_Icon_DrawEntry *message)
{
    struct Icon_DATA *data = INST_DATA(CLASS, obj);

    STRPTR          buf = NULL;

    struct Rectangle  iconlabelrect;
    struct Rectangle  objrect;

    ULONG               txtbox_width = 0;
    LONG        tx,ty,offsetx,offsety;
    LONG        txwidth; // txheight;
    
    ULONG labelX;
    ULONG labelY;
    ULONG txtarea_width;
    ULONG curlabel_TotalLines, curlabel_CurrentLine, offset_y;

#if defined(DEBUG_ILC_ICONRENDERING)
D(bug("[Icon]: %s(message->icon = 0x%p), '%s'\n", __PRETTY_FUNCTION__, message->icon, message->icon->IcD_Label_TXTBUFF));
#endif

    if ((!(message->icon->IcD_Flags & ICONENTRY_FLAG_VISIBLE)) ||
        (data->icld_BufferRastPort == NULL) ||
        (!(message->icon->IcD_DiskObj)))
    {
#if defined(DEBUG_ILC_ICONRENDERING)
D(bug("[Icon] %s: Not visible or missing DOB\n", __PRETTY_FUNCTION__));
#endif
        return FALSE;
    }
    
    /* Get the dimensions and affected area of message->icon's label */
    Icon_GetIconLabelRectangle(obj, data, message->icon, &iconlabelrect);

    /* Add the relative position offset of the message->icon's label */
    offsetx = (_mleft(obj) - data->icld_ViewX) + message->icon->IcD_IconX;
    txtbox_width = (iconlabelrect.MaxX - iconlabelrect.MinX) + 1;

    if (txtbox_width < message->icon->IcD_AreaWidth)
        offsetx += ((message->icon->IcD_AreaWidth - txtbox_width)/2);

    if ((data->icld__Option_IconMode == ICON_LISTMODE_GRID) &&
        (message->icon->IcD_AreaWidth < data->icld_IconAreaLargestWidth))
        offsetx += ((data->icld_IconAreaLargestWidth - message->icon->IcD_AreaWidth)/2);
    
    iconlabelrect.MinX += offsetx;
    iconlabelrect.MaxX += offsetx;

    offsety = (_mtop(obj) - data->icld_ViewY) + message->icon->IcD_IconY + data->icld__Option_IconImageSpacing;
    if (data->icld__Option_IconMode == ICON_LISTMODE_GRID)
    {
        offsety = offsety + data->icld_IconLargestHeight;
    }
    else
    {
        offsety = offsety + message->icon->IcD_IconHeight;
    }
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
    
    SetABPenDrMd(data->icld_BufferRastPort, _pens(obj)[MPEN_TEXT], 0, JAM1);

    iconlabelrect.MinX = (iconlabelrect.MinX - _mleft(obj)) + data->icld_DrawOffsetX;
    iconlabelrect.MinY = (iconlabelrect.MinY - _mtop(obj)) + data->icld_DrawOffsetY;
    iconlabelrect.MaxX = (iconlabelrect.MaxX - _mleft(obj)) + data->icld_DrawOffsetX;
    iconlabelrect.MaxY = (iconlabelrect.MaxY - _mtop(obj)) + data->icld_DrawOffsetY;

    labelX = iconlabelrect.MinX + data->icld__Option_LabelTextBorderWidth + data->icld__Option_LabelTextHorizontalPadding;
    labelY = iconlabelrect.MinY + data->icld__Option_LabelTextBorderHeight + data->icld__Option_LabelTextVerticalPadding;

    txtarea_width = txtbox_width - ((data->icld__Option_LabelTextBorderWidth + data->icld__Option_LabelTextHorizontalPadding) * 2);

    if (message->icon->IcD_Label_TXTBUFF && message->icon->IcD_DisplayedLabel_TXTBUFF)
    {
  char *curlabel_StrPtr;

        if ((message->icon->IcD_Flags & ICONENTRY_FLAG_FOCUS) && ((BOOL)XGET(_win(obj), MUIA_Window_Activate)))
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

        curlabel_TotalLines = message->icon->IcD_DisplayedLabel_SplitParts;
              curlabel_CurrentLine = 0;

        if (curlabel_TotalLines == 0)
            curlabel_TotalLines = 1;

        if (!(data->icld__Option_LabelTextMultiLineOnFocus) || (data->icld__Option_LabelTextMultiLineOnFocus && (message->icon->IcD_Flags & ICONENTRY_FLAG_FOCUS)))
        {
            if (curlabel_TotalLines > XGET(_parent(obj), MUIA_Icon_LabelText_MultiLine))
                curlabel_TotalLines = XGET(_parent(obj), MUIA_Icon_LabelText_MultiLine);
        }
        else
            curlabel_TotalLines = 1;

        curlabel_StrPtr = message->icon->IcD_DisplayedLabel_TXTBUFF;
        
        ty = labelY - 1;

D(bug("[Icon] %s: Font YSize %d Baseline %d\n", __PRETTY_FUNCTION__,data->icld_IconLabelFont->tf_YSize, data->icld_IconLabelFont->tf_Baseline));

        for (curlabel_CurrentLine = 0; curlabel_CurrentLine < curlabel_TotalLines; curlabel_CurrentLine++)
        {
      ULONG IcD_LabelLength;

            if (curlabel_CurrentLine > 0) curlabel_StrPtr = curlabel_StrPtr + strlen(curlabel_StrPtr) + 1;
            if ((curlabel_CurrentLine >= (curlabel_TotalLines -1)) && (curlabel_TotalLines < message->icon->IcD_DisplayedLabel_SplitParts))
            {
                char *tmpLine = curlabel_StrPtr;
                ULONG tmpLen = strlen(tmpLine);

                if ((curlabel_StrPtr = AllocVecPooled(data->icld_Pool,
                    tmpLen + 1)) != NULL)
                {
                    memset(curlabel_StrPtr, 0, tmpLen + 1);
                    strncpy(curlabel_StrPtr, tmpLine, tmpLen - 3);
                    strcat(curlabel_StrPtr , " ..");
                }
                else return FALSE;
                
            }
            
            IcD_LabelLength = strlen(curlabel_StrPtr);
            offset_y = 0;

            // Center message->icon's label
            tx = (labelX + (message->icon->IcD_DisplayedLabel_Width / 2) - (TextLength(data->icld_BufferRastPort, curlabel_StrPtr, strlen(curlabel_StrPtr)) / 2));

            if (message->icon->IcD_DisplayedLabel_Width < txtarea_width)
                tx += ((txtarea_width - message->icon->IcD_DisplayedLabel_Width)/2);

            ty = ty + data->icld_IconLabelFont->tf_YSize;

            switch ( data->icld__Option_LabelTextMode )
            {
                case ICON_TEXTMODE_DROPSHADOW:
                    SetAPen(data->icld_BufferRastPort, data->icld_LabelShadowPen);
                    Move(data->icld_BufferRastPort, tx + 1, ty + 1); 
                    Text(data->icld_BufferRastPort, curlabel_StrPtr, IcD_LabelLength);
                    offset_y = 1;
                case ICON_TEXTMODE_PLAIN:
                    SetAPen(data->icld_BufferRastPort, data->icld_LabelPen);
                    Move(data->icld_BufferRastPort, tx, ty); 
                    Text(data->icld_BufferRastPort, curlabel_StrPtr, IcD_LabelLength);
                    break;
                    
                default:
                    // Outline mode:
                    
                    SetSoftStyle(data->icld_BufferRastPort, FSF_BOLD, AskSoftStyle(data->icld_BufferRastPort));

                    SetAPen(data->icld_BufferRastPort, data->icld_LabelShadowPen);
                    Move(data->icld_BufferRastPort, tx + 1, ty ); 
                    Text(data->icld_BufferRastPort, curlabel_StrPtr, IcD_LabelLength);
                    Move(data->icld_BufferRastPort, tx - 1, ty ); 
                    Text(data->icld_BufferRastPort, curlabel_StrPtr, IcD_LabelLength);
                    Move(data->icld_BufferRastPort, tx, ty + 1);  
                    Text(data->icld_BufferRastPort, curlabel_StrPtr, IcD_LabelLength);
                    Move(data->icld_BufferRastPort, tx, ty - 1);
                    Text(data->icld_BufferRastPort, curlabel_StrPtr, IcD_LabelLength);
                    
                    SetAPen(data->icld_BufferRastPort, data->icld_LabelPen);
                    Move(data->icld_BufferRastPort, tx , ty ); 
                    Text(data->icld_BufferRastPort, curlabel_StrPtr, IcD_LabelLength);
                    
                    SetSoftStyle(data->icld_BufferRastPort, FS_NORMAL, AskSoftStyle(data->icld_BufferRastPort));
                    offset_y = 2;
                    break;
            }
            if ((curlabel_CurrentLine >= (curlabel_TotalLines -1)) && (curlabel_TotalLines < message->icon->IcD_DisplayedLabel_SplitParts))
            {
                FreeVecPooled(data->icld_Pool, curlabel_StrPtr);
            }
            ty = ty + offset_y;
        }

        /*date/size sorting has the date/size appended under the message->icon label*/

        if ((message->icon->IcD_IconEntry.type != ST_USERDIR) && ((data->icld_SortFlags & ICONLIST_SORT_BY_SIZE|ICONLIST_SORT_BY_DATE) != 0))
        {
            buf = NULL;
            SetFont(data->icld_BufferRastPort, data->icld_IconInfoFont);

            if ((data->icld_SortFlags & ICONLIST_SORT_MASK) == ICONLIST_SORT_BY_SIZE)
            {
                buf = message->icon->IcD_Size_TXTBUFF;
                txwidth = message->icon->IcD_Size_Width;
            }
            else if ((data->icld_SortFlags & ICONLIST_SORT_MASK) == ICONLIST_SORT_BY_DATE)
            {
                if (message->icon->IcD_Flags & ICONENTRY_FLAG_TODAY)
                {
                    buf  = message->icon->IcD_Time_TXTBUFF;
                    txwidth = message->icon->IcD_Time_Width;
                }
                else
                {
                    buf = message->icon->IcD_Date_TXTBUFF;
                    txwidth = message->icon->IcD_Date_Width;
                }
            }

            if (buf)
            {
                ULONG IcD_LabelLength = strlen(buf);
                tx = labelX;

                if (txwidth < txtarea_width)
                    tx += ((txtarea_width - txwidth)/2);

                ty = labelY + ((data->icld__Option_LabelTextVerticalPadding + data->icld_IconLabelFont->tf_YSize ) * curlabel_TotalLines) + data->icld_IconInfoFont->tf_YSize;

                switch ( data->icld__Option_LabelTextMode )
                {
                    case ICON_TEXTMODE_DROPSHADOW:
                        SetAPen(data->icld_BufferRastPort, data->icld_InfoShadowPen);
                        Move(data->icld_BufferRastPort, tx + 1, ty + 1); Text(data->icld_BufferRastPort, buf, IcD_LabelLength);
                    case ICON_TEXTMODE_PLAIN:
                        SetAPen(data->icld_BufferRastPort, XGET(_parent(obj), MUIA_IconList_LabelInfoText_Pen));
                        Move(data->icld_BufferRastPort, tx, ty); Text(data->icld_BufferRastPort, buf, IcD_LabelLength);
                        break;
                        
                    default:
                        // Outline mode..
                        SetSoftStyle(data->icld_BufferRastPort, FSF_BOLD, AskSoftStyle(data->icld_BufferRastPort));
                        SetAPen(data->icld_BufferRastPort, data->icld_InfoShadowPen);
                        
                        Move(data->icld_BufferRastPort, tx + 1, ty ); 
                        Text(data->icld_BufferRastPort, buf, IcD_LabelLength);
                        Move(data->icld_BufferRastPort, tx - 1, ty );  
                        Text(data->icld_BufferRastPort, buf, IcD_LabelLength);
                        Move(data->icld_BufferRastPort, tx, ty - 1 );  
                        Text(data->icld_BufferRastPort, buf, IcD_LabelLength);
                        Move(data->icld_BufferRastPort, tx, ty + 1 );  
                        Text(data->icld_BufferRastPort, buf, IcD_LabelLength);
                        
                        SetAPen(data->icld_BufferRastPort, XGET(_parent(obj), MUIA_IconList_LabelInfoText_Pen));
                        
                        Move(data->icld_BufferRastPort, tx, ty );
                        Text(data->icld_BufferRastPort, buf, IcD_LabelLength);
                        
                        SetSoftStyle(data->icld_BufferRastPort, FS_NORMAL, AskSoftStyle(data->icld_BufferRastPort));
                        break;
                }
            }
        }
    }
    
    return TRUE;
}

///OM_NEW()
/**************************************************************************
OM_NEW
**************************************************************************/
IPTR Icon__OM_NEW(struct IClass *CLASS, Object *obj, struct opSet *message)
{
    struct Icon_DATA  *data = NULL;

D(bug("[Icon]: %s()\n", __PRETTY_FUNCTION__));

    obj = (Object *)DoSuperNewTags(CLASS, obj, NULL,
        MUIA_FillArea, FALSE,
        TAG_MORE, (IPTR) message->ops_AttrList);

    if (!obj) return FALSE;

    data = INST_DATA(CLASS, obj);

    struct DateTime    dt;
    struct DateStamp     now;
    UBYTE            *sp = NULL;

    struct Rectangle     rect;

    IPTR                 geticon_error = 0;

D(bug("[Icon]: %s()\n", __PRETTY_FUNCTION__));

    /*disk object (icon)*/
    if (message->icon_dob == NULL)
    {
        data->dob = GetIconTags
        (
            message->filename, 
            ICONGETA_FailIfUnavailable,        FALSE,
            ICONGETA_GenerateImageMasks,       TRUE,
            ICONA_ErrorCode,                   &geticon_error,
            TAG_DONE
        );

        if (data->dob == NULL)
        {
D(bug("[Icon] %s: Fatal: Couldnt get DiskObject! (error code = 0x%p)\n", __PRETTY_FUNCTION__, geticon_error));

            return NULL;
        }
    }
    else
    {
        data->dob = message->icon_dob;
    }

D(bug("[Icon] %s: DiskObject @ 0x%p\n", __PRETTY_FUNCTION__, data->dob));

    if ((entry = AllocPooled(data->icld_Pool, sizeof(struct IconEntry))) == NULL)
    {
D(bug("[Icon] %s: Failed to Allocate Entry Storage!\n", __PRETTY_FUNCTION__));
        FreeDiskObject(data->dob);
        return NULL;
    }
    memset(entry, 0, sizeof(struct IconEntry));

    /* Allocate Text Buffers */

    if ((data->IcD_Date_TXTBUFF = AllocPooled(data->icld_Pool, LEN_DATSTRING)) == NULL)
    {
D(bug("[Icon] %s: Failed to Allocate Entry DATE Storage!\n", __PRETTY_FUNCTION__));
        DoMethod(obj, MUIM_Icon_DestroyEntry, entry);
        return NULL;
    }
    memset(data->IcD_Date_TXTBUFF, 0, LEN_DATSTRING);

    if ((data->IcD_Time_TXTBUFF = AllocPooled(data->icld_Pool, LEN_DATSTRING)) == NULL)
    {
D(bug("[Icon] %s: Failed to Allocate Entry TIME string Storage!\n", __PRETTY_FUNCTION__));
        DoMethod(obj, MUIM_Icon_DestroyEntry, entry);
        return NULL;
    }
    memset(data->IcD_Time_TXTBUFF, 0, LEN_DATSTRING);

    if ((data->IcD_Size_TXTBUFF = AllocPooled(data->icld_Pool, 30)) == NULL)
    {
D(bug("[Icon] %s: Failed to Allocate Entry SIZE string Storage!\n", __PRETTY_FUNCTION__));
        DoMethod(obj, MUIM_Icon_DestroyEntry, entry);
        return NULL;
    }
    memset(data->IcD_Size_TXTBUFF, 0, 30);

    if ((data->IcD_TxtBuf_PROT = AllocPooled(data->icld_Pool, 8)) == NULL)
    {
D(bug("[Icon] %s: Failed to Allocate Entry PROT Flag string Storage!\n", __PRETTY_FUNCTION__));
        DoMethod(obj, MUIM_Icon_DestroyEntry, entry);
        return NULL;
    }
    memset(data->IcD_TxtBuf_PROT, 0, 8);

    /*alloc filename*/
    if ((data->IcD_IconEntry.filename = AllocPooled(data->icld_Pool, strlen(message->filename) + 1)) == NULL)
    {
D(bug("[Icon] %s: Failed to Allocate Entry filename string Storage!\n", __PRETTY_FUNCTION__));
        DoMethod(obj, MUIM_Icon_DestroyEntry, entry);
        return NULL;
    }

    /*alloc icon label*/
    if ((data->IcD_Label_TXTBUFF = AllocPooled(data->icld_Pool, strlen(message->label) + 1)) == NULL)
    {
D(bug("[Icon] %s: Failed to Allocate Entry label string Storage!\n", __PRETTY_FUNCTION__));
        DoMethod(obj, MUIM_Icon_DestroyEntry, entry);
        return NULL;
    }

    /*file info block*/
    if(message->fib != NULL)
    {
        entry->IcD_FileInfoBlock = *message->fib;

        if (entry->IcD_FileInfoBlock.fib_DirEntryType > 0)
        {
            strcpy(entry->IcD_Size_TXTBUFF, "Drawer");
        }
        else
        {
            int i = entry->IcD_FileInfoBlock.fib_Size;

            /*show byte size for small files*/
            if (i > 9999)
                sprintf(entry->IcD_Size_TXTBUFF, "%ld KB", (LONG)(i/1024));
            else
                sprintf(entry->IcD_Size_TXTBUFF, "%ld B", (LONG)i);
        }

        dt.dat_Stamp    = entry->IcD_FileInfoBlock.fib_Date;
        dt.dat_Format   = FORMAT_DEF;
        dt.dat_Flags    = 0;
        dt.dat_StrDay   = NULL;
        dt.dat_StrDate  = entry->IcD_Date_TXTBUFF;
        dt.dat_StrTime  = entry->IcD_Time_TXTBUFF;

        DateToStr(&dt);
        DateStamp(&now);

        /*if modified today show time, otherwise just show date*/
        if (now.ds_Days == entry->IcD_FileInfoBlock.fib_Date.ds_Days)
            entry->IcD_Flags |= ICONENTRY_FLAG_TODAY;
        else
            entry->IcD_Flags &= ~ICONENTRY_FLAG_TODAY;

        sp = entry->IcD_TxtBuf_PROT;
        *sp++ = (entry->IcD_FileInfoBlock.fib_Protection & FIBF_SCRIPT)  ? 's' : '-';
        *sp++ = (entry->IcD_FileInfoBlock.fib_Protection & FIBF_PURE)    ? 'p' : '-';
        *sp++ = (entry->IcD_FileInfoBlock.fib_Protection & FIBF_ARCHIVE) ? 'a' : '-';
        *sp++ = (entry->IcD_FileInfoBlock.fib_Protection & FIBF_READ)    ? '-' : 'r';
        *sp++ = (entry->IcD_FileInfoBlock.fib_Protection & FIBF_WRITE)   ? '-' : 'w';
        *sp++ = (entry->IcD_FileInfoBlock.fib_Protection & FIBF_EXECUTE) ? '-' : 'e';
        *sp++ = (entry->IcD_FileInfoBlock.fib_Protection & FIBF_DELETE)  ? '-' : 'd';
        *sp++ = '\0';
    
        entry->IcD_IconEntry.type = entry->IcD_FileInfoBlock.fib_DirEntryType;
    }
    else
    {
        entry->IcD_IconEntry.type = ST_USERDIR;
    }

    /* Override type if specified during createntry */
    if (message->type != 0)
    {
        entry->IcD_IconEntry.type = message->type;
D(bug("[Icon] %s: Overide Entry Type. New Type = %x\n", __PRETTY_FUNCTION__, entry->IcD_IconEntry.type));
    }
    else
    {
D(bug("[Icon] %s: Entry Type = %x\n", __PRETTY_FUNCTION__, entry->IcD_IconEntry.type));
    }

    strcpy(entry->IcD_IconEntry.filename, message->filename);
    strcpy(entry->IcD_Label_TXTBUFF, message->label);

    if (Icon__LabelFunc_CreateLabel(obj, data, entry) != NULL)
    {
        entry->IcD_DiskObj = dob;
        entry->IcD_IconEntry.udata = NULL;

        /* Use a geticonrectangle routine that gets textwidth! */
        Icon_GetIconAreaRectangle(obj, data, entry, &rect);

        AddTail((struct List*)&data->icld_Icon, (struct Node*)&entry->IcD_IconNode);

        return (IPTR)entry;
    }

    DoMethod(obj, MUIM_Icon_DestroyEntry, entry);
    return NULL;
    
D(bug("[Icon] %s: SELF = 0x%p, muiRenderInfo = 0x%p\n", __PRETTY_FUNCTION__, obj, muiRenderInfo(obj)));

    return (IPTR)obj;
}
///

///OM_DISPOSE()
/**************************************************************************
OM_DISPOSE
**************************************************************************/
IPTR Icon__OM_DISPOSE(struct IClass *CLASS, Object *obj, Msg message)
{
    struct Icon_DATA *data = INST_DATA(CLASS, obj);
    struct IconEntry    *node = NULL;

D(bug("[Icon]: %s()\n", __PRETTY_FUNCTION__));
    
    DoSuperMethodA(CLASS,obj,message);
    return 0;
}
///

///OM_SET()
/**************************************************************************
OM_SET
**************************************************************************/
IPTR Icon__OM_SET(struct IClass *CLASS, Object *obj, struct opSet *message)
{
    struct Icon_DATA *data = INST_DATA(CLASS, obj);
    struct TagItem    *tag = NULL,
                        *tags = NULL;

    WORD             oldleft = data->icld_ViewX,
                         oldtop = data->icld_ViewY;
                         //oldwidth = data->icld_ViewWidth,
                         //oldheight = data->icld_ViewHeight;

D(bug("[Icon]: %s()\n", __PRETTY_FUNCTION__));

    for (tags = message->ops_AttrList; (tag = NextTagItem((TAGITEM)&tags)); )
    {
        switch (tag->ti_Tag)
        {
            case MUIA_Icon_Left:
D(bug("[Icon] %s: MUIA_Icon_Left %ld\n", __PRETTY_FUNCTION__, tag->ti_Data));
                if (data->icld_ViewX != tag->ti_Data)
                    data->icld_ViewX = tag->ti_Data;
            break;
    
            case MUIA_Icon_Top:
D(bug("[Icon] %s: MUIA_Icon_Top %ld\n", __PRETTY_FUNCTION__, tag->ti_Data));
                if (data->icld_ViewY != tag->ti_Data)
                    data->icld_ViewY = tag->ti_Data;
            break;

            case MUIA_Icon_Rastport:
D(bug("[Icon] %s: MUIA_Icon_Rastport 0x%p\n", __PRETTY_FUNCTION__, tag->ti_Data));
                data->icld_DisplayRastPort = (struct RastPort*)tag->ti_Data;
                data->icld_DrawOffsetX = _mleft(obj);
                data->icld_DrawOffsetY = _mtop(obj);
                if (data->icld_BufferRastPort != NULL)
                {
                    //Buffer still set!?!?!
                }
                SET(obj, MUIA_Icon_BufferRastport, tag->ti_Data);
                break;

            case MUIA_Icon_BufferRastport:
D(bug("[Icon] %s: MUIA_Icon_BufferRastport 0x%p\n", __PRETTY_FUNCTION__, tag->ti_Data));
                data->icld_BufferRastPort = (struct RastPort*)tag->ti_Data;
                break;

            case MUIA_Font:
D(bug("[Icon] %s: MUIA_Font 0x%p\n", __PRETTY_FUNCTION__, tag->ti_Data));
                data->icld_IconLabelFont = (struct TextFont*)tag->ti_Data;
                break;

            case MUIA_Icon_LabelInfoText_Font:
D(bug("[Icon] %s: MUIA_Icon_LabelInfoText_Font 0x%p\n", __PRETTY_FUNCTION__, tag->ti_Data));
                data->icld_IconInfoFont = (struct TextFont*)tag->ti_Data;
                break;
            
            case MUIA_Icon_DisplayFlags:
            {
D(bug("[Icon] %s: MUIA_Icon_DisplayFlags\n", __PRETTY_FUNCTION__));
                data->icld_DisplayFlags = (ULONG)tag->ti_Data;
                if (data->icld_DisplayFlags & ICONLIST_DISP_BUFFERED)
                {
                    struct BitMap *tmp_BuffBitMap = NULL;
        ULONG tmp_RastDepth;

#if defined(DEBUG_ILC_ICONRENDERING)
D(bug("[Icon] %s: MUIA_Icon_DisplayFlags & ICONLIST_DISP_BUFFERED\n", __PRETTY_FUNCTION__));
#endif
                    if ((data->icld_BufferRastPort) && (data->icld_BufferRastPort != data->icld_DisplayRastPort))
                    {
                        //Free up the buffers rastport and bitmap so we can replace them ..
                        FreeBitMap(data->icld_BufferRastPort->BitMap);
                        FreeRastPort(data->icld_BufferRastPort);
                        SET(obj, MUIA_Icon_BufferRastport, NULL);
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
                        SET(obj, MUIA_Icon_BufferRastport, data->icld_BufferRastPort);
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
            }
            break;

            case MUIA_Icon_SortFlags:
D(bug("[Icon] %s: MUIA_Icon_SortFlags\n", __PRETTY_FUNCTION__));
                data->icld_SortFlags = (ULONG)tag->ti_Data;
                break;
                
            case MUIA_Icon_IconMode:
D(bug("[Icon] %s: MUIA_Icon_IconMode %d\n", __PRETTY_FUNCTION__, tag->ti_Data));
                data->icld__Option_IconMode = (UBYTE)tag->ti_Data;
                break;
            
            case MUIA_Icon_LabelText_Mode:
D(bug("[Icon] %s: MUIA_Icon_LabelText_Mode %d\n", __PRETTY_FUNCTION__, tag->ti_Data));
                data->icld__Option_LabelTextMode = (UBYTE)tag->ti_Data;
                break;

            case MUIA_Icon_LabelText_MultiLineOnFocus:
D(bug("[Icon] %s: MUIA_Icon_LabelText_MultiLineOnFocus %d\n", __PRETTY_FUNCTION__, tag->ti_Data));
                data->icld__Option_LabelTextMultiLineOnFocus = (BOOL)tag->ti_Data;
                break;

            case MUIA_Icon_Icon_HorizontalSpacing:
D(bug("[Icon] %s: MUIA_Icon_Icon_HorizontalSpacing %d\n", __PRETTY_FUNCTION__, tag->ti_Data));
                data->icld__Option_IconHorizontalSpacing = (UBYTE)tag->ti_Data;
                break;

            case MUIA_Icon_Icon_VerticalSpacing:
D(bug("[Icon] %s: MUIA_Icon_Icon_VerticalSpacing %d\n", __PRETTY_FUNCTION__, tag->ti_Data));
                data->icld__Option_IconVerticalSpacing = (UBYTE)tag->ti_Data;
                break;

            case MUIA_Icon_Icon_ImageSpacing:
D(bug("[Icon] %s: MUIA_Icon_Icon_ImageSpacing %d\n", __PRETTY_FUNCTION__, tag->ti_Data));
                data->icld__Option_IconImageSpacing = (UBYTE)tag->ti_Data;
                break;

            case MUIA_Icon_LabelText_HorizontalPadding:
D(bug("[Icon] %s: MUIA_Icon_LabelText_HorizontalPadding %d\n", __PRETTY_FUNCTION__, tag->ti_Data));
                data->icld__Option_LabelTextHorizontalPadding = (UBYTE)tag->ti_Data;
                break;

            case MUIA_Icon_LabelText_VerticalPadding:
D(bug("[Icon] %s: MUIA_Icon_LabelText_VerticalPadding %d\n", __PRETTY_FUNCTION__, tag->ti_Data));
                data->icld__Option_LabelTextVerticalPadding = (UBYTE)tag->ti_Data;
                break;

            case MUIA_Icon_LabelText_BorderWidth:
D(bug("[Icon] %s: MUIA_Icon_LabelText_BorderWidth %d\n", __PRETTY_FUNCTION__, tag->ti_Data));
                data->icld__Option_LabelTextBorderWidth = (UBYTE)tag->ti_Data;
                break;

            case MUIA_Icon_LabelText_BorderHeight:
D(bug("[Icon] %s: MUIA_Icon_LabelText_BorderHeight %d\n", __PRETTY_FUNCTION__, tag->ti_Data));
                data->icld__Option_LabelTextBorderHeight = (UBYTE)tag->ti_Data;
                break;

            case MUIA_Icon_LabelText_Pen:
                data->icld_LabelPen = (ULONG)tag->ti_Data;
                break;

            case MUIA_Icon_LabelText_ShadowPen:
                data->icld_LabelShadowPen = (ULONG)tag->ti_Data;
                break;

            case MUIA_Icon_LabelInfoText_ShadowPen:
                data->icld_InfoShadowPen = (ULONG)tag->ti_Data;
                break;

            /* Settings defined by the view class */
            case MUIA_Iconview_FixedBackground:
D(bug("[Icon] %s: MUIA_Iconview_FixedBackground\n", __PRETTY_FUNCTION__));
                data->icld__Option_IconFixedBackground = (BOOL)tag->ti_Data;
                break;

            case MUIA_Iconview_ScaledBackground:
D(bug("[Icon] %s: MUIA_Iconview_ScaledBackground\n", __PRETTY_FUNCTION__));
                data->icld__Option_IconScaledBackground = (BOOL)tag->ti_Data;
                break;

            /* We listen for MUIA_Background and set default values for known types */
            case MUIA_Background:
#if defined(DEBUG_ILC_ICONRENDERING)
D(bug("[Icon] %s: MUIA_Background\n", __PRETTY_FUNCTION__));
#endif
            {
                char *bgmode_string = (char *)tag->ti_Data;
                BYTE this_mode = bgmode_string[0] - 48;

#if defined(DEBUG_ILC_ICONRENDERING)
D(bug("[Icon] %s: MUIA_Background | MUI BG Mode = %d\n", __PRETTY_FUNCTION__, this_mode));
#endif
                switch (this_mode)
                {
                case 0:
                    //MUI Pattern
                    NNSET(obj, MUIA_Iconview_FixedBackground, FALSE);
                    NNSET(obj, MUIA_Iconview_ScaledBackground, FALSE);
                    break;
                case 2:
                    //MUI RGB color
                    NNSET(obj, MUIA_Iconview_FixedBackground, FALSE);
                    NNSET(obj, MUIA_Iconview_ScaledBackground, FALSE);
                    break;
                case 7:
                    //Zune Gradient
                    NNSET(obj, MUIA_Iconview_FixedBackground, TRUE);
                    NNSET(obj, MUIA_Iconview_ScaledBackground, TRUE);
                    break;
                case 5:
                    //Image
                    NNSET(obj, MUIA_Iconview_FixedBackground, FALSE);
                    NNSET(obj, MUIA_Iconview_ScaledBackground, FALSE);
                    break;
                }
            }
            return (IPTR)FALSE;
        }
    }

D(bug("[Icon] %s(), out of switch\n", __PRETTY_FUNCTION__));

    if ((oldleft != data->icld_ViewX) || (oldtop != data->icld_ViewY))
    {
        data->icld_UpdateMode = UPDATE_SCROLL;
        data->update_scrolldx = data->icld_ViewX - oldleft;
        data->update_scrolldy = data->icld_ViewY - oldtop;
D(bug("[Icon] %s(), call MUI_Redraw()\n", __PRETTY_FUNCTION__));
        MUI_Redraw(obj, MADF_DRAWUPDATE);
    }

D(bug("[Icon] %s(), call DoSuperMethodA()\n", __PRETTY_FUNCTION__));
    return DoSuperMethodA(CLASS, obj, (struct opSet *)message);
}
///

///OM_GET()
/**************************************************************************
OM_GET
**************************************************************************/
IPTR Icon__OM_GET(struct IClass *CLASS, Object *obj, struct opGet *message)
{
    /* small macro to simplify return value storage */
#define STORE *(message->opg_Storage)
    struct Icon_DATA *data = INST_DATA(CLASS, obj);

D(bug("[Icon]: %s()\n", __PRETTY_FUNCTION__));

    switch (message->opg_AttrID)
    {
        case MUIA_Icon_Rastport:                     STORE = (IPTR)data->icld_DisplayRastPort; return 1;
        case MUIA_Icon_BufferRastport:               STORE = (IPTR)data->icld_BufferRastPort; return 1;
        case MUIA_Icon_BufferLeft:                   STORE = (IPTR)data->icld_DrawOffsetX; return 1;
        case MUIA_Icon_BufferTop:                    STORE = (IPTR)data->icld_DrawOffsetY; return 1;
        case MUIA_Icon_Left:                         STORE = (IPTR)data->icld_ViewX; return 1;
        case MUIA_Icon_Top:                          STORE = (IPTR)data->icld_ViewY; return 1;
        case MUIA_Icon_BufferWidth:
        case MUIA_Icon_Width:                        STORE = (IPTR)data->icld_AreaWidth; return 1;
        case MUIA_Icon_BufferHeight:
        case MUIA_Icon_Height:                       STORE = (IPTR)data->icld_AreaHeight; return 1;
        case MUIA_Icon_IconsDropped:                 STORE = (IPTR)&data->icld_DragDropEvent; return 1;
        case MUIA_Icon_Clicked:                      STORE = (IPTR)&data->icld_ClickEvent; return 1;
        case MUIA_Icon_IconMode:                 STORE = (IPTR)data->icld__Option_IconMode; return 1;
        case MUIA_Icon_LabelText_Mode:               STORE = (IPTR)data->icld__Option_LabelTextMode; return 1;
        case MUIA_Icon_LabelText_MultiLineOnFocus:   STORE = (IPTR)data->icld__Option_LabelTextMultiLineOnFocus; return 1;
        case MUIA_Icon_DisplayFlags:                 STORE = (IPTR)data->icld_DisplayFlags; return 1;
        case MUIA_Icon_SortFlags:                    STORE = (IPTR)data->icld_SortFlags; return 1;

        case MUIA_Icon_FocusIcon:                    STORE = (IPTR)data->icld_FocusIcon; return 1;

        case MUIA_Font:                                  STORE = (IPTR)data->icld_IconLabelFont; return 1;
        case MUIA_Icon_LabelText_Pen:                STORE = (IPTR)data->icld_LabelPen; return 1;
        case MUIA_Icon_LabelText_ShadowPen:          STORE = (IPTR)data->icld_LabelShadowPen; return 1;
        case MUIA_Icon_LabelInfoText_Font:           STORE = (IPTR)data->icld_IconInfoFont; return 1;
        case MUIA_Icon_LabelInfoText_ShadowPen:      STORE = (IPTR)data->icld_InfoShadowPen; return 1;

        case MUIA_Icon_Icon_HorizontalSpacing:       STORE = (IPTR)data->icld__Option_IconHorizontalSpacing; return 1;
        case MUIA_Icon_Icon_VerticalSpacing:         STORE = (IPTR)data->icld__Option_IconVerticalSpacing; return 1;
        case MUIA_Icon_Icon_ImageSpacing:            STORE = (IPTR)data->icld__Option_IconImageSpacing; return 1;
        case MUIA_Icon_LabelText_HorizontalPadding:  STORE = (IPTR)data->icld__Option_LabelTextHorizontalPadding; return 1;
        case MUIA_Icon_LabelText_VerticalPadding:    STORE = (IPTR)data->icld__Option_LabelTextVerticalPadding; return 1;
        case MUIA_Icon_LabelText_BorderWidth:        STORE = (IPTR)data->icld__Option_LabelTextBorderWidth; return 1;
        case MUIA_Icon_LabelText_BorderHeight:       STORE = (IPTR)data->icld__Option_LabelTextBorderHeight; return 1;

        /* Settings defined by the view class */
        case MUIA_Iconview_FixedBackground:          STORE = (IPTR)data->icld__Option_IconFixedBackground; return 1;
        case MUIA_Iconview_ScaledBackground:         STORE = (IPTR)data->icld__Option_IconScaledBackground; return 1;
    }

    if (DoSuperMethodA(CLASS, obj, (Msg) message)) 
        return 1;
    return 0;
#undef STORE
}
///

///MUIM_Setup()
/**************************************************************************
MUIM_Setup
**************************************************************************/
IPTR Icon__MUIM_Setup(struct IClass *CLASS, Object *obj, struct MUIP_Setup *message)
{
    struct Icon_DATA *data = INST_DATA(CLASS, obj);
    struct IconEntry    *node = NULL;
    IPTR                 geticon_error = 0;

D(bug("[Icon]: %s()\n", __PRETTY_FUNCTION__));

    if (!DoSuperMethodA(CLASS, obj, (Msg) message)) return 0;

    DoMethod(_win(obj), MUIM_Window_AddEventHandler, (IPTR)&data->ehn);

    /* Get Internal Objects to use if not set .. */
    data->icld_DisplayRastPort = NULL;
    data->icld_BufferRastPort = NULL;

    if (data->icld_IconLabelFont == NULL)   data->icld_IconLabelFont = _font(obj);
    if (data->icld_IconInfoFont == NULL)    data->icld_IconInfoFont = data->icld_IconLabelFont;
D(bug("[Icon] %s: Use Font @ 0x%p, RastPort @ 0x%p\n", __PRETTY_FUNCTION__, data->icld_IconLabelFont, data->icld_BufferRastPort ));

    /* Set our base options .. */
    data->icld_LabelPen                           = _pens(obj)[MPEN_SHINE];
    data->icld_LabelShadowPen                     = _pens(obj)[MPEN_SHADOW];
    data->icld_InfoShadowPen                      = _pens(obj)[MPEN_SHADOW];

    data->icld__Option_LabelTextMultiLineOnFocus  = FALSE;
    
    data->icld__Option_IconHorizontalSpacing      = ILC_ICON_HORIZONTALMARGIN_DEFAULT;
    data->icld__Option_IconVerticalSpacing        = ILC_ICON_VERTICALMARGIN_DEFAULT;
    data->icld__Option_IconImageSpacing           = ILC_ICONLABEL_IMAGEMARGIN_DEFAULT;
    data->icld__Option_LabelTextHorizontalPadding = ILC_ICONLABEL_HORIZONTALTEXTMARGIN_DEFAULT;
    data->icld__Option_LabelTextVerticalPadding   = ILC_ICONLABEL_VERTICALTEXTMARGIN_DEFAULT;
    data->icld__Option_LabelTextBorderWidth       = ILC_ICONLABEL_BORDERWIDTH_DEFAULT;
    data->icld__Option_LabelTextBorderHeight      = ILC_ICONLABEL_BORDERHEIGHT_DEFAULT;
    
    #ifdef __AROS__
    ForeachNode(&data->icld_Icon, node)
    #else
    Foreach_Node(&data->icld_Icon, node);
    #endif
    {
        if (!node->IcD_DiskObj)
        {
            if (!(node->IcD_DiskObj = GetIconTags(node->IcD_IconEntry.filename, ICONGETA_GenerateImageMasks, TRUE, ICONGETA_FailIfUnavailable, FALSE, ICONA_ErrorCode, &geticon_error, TAG_DONE)))
            {
D(bug("[Icon] %s: Failed to obtain Icon '%s's diskobj! (error code = 0x%p)\n", __PRETTY_FUNCTION__, node->IcD_IconEntry.filename, geticon_error));
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
IPTR Icon__MUIM_Show(struct IClass *CLASS, Object *obj, struct MUIP_Show *message)
{
    struct Icon_DATA *data = INST_DATA(CLASS, obj);
    LONG                newleft,
                        newtop;
    IPTR                rc;

D(bug("[Icon]: %s()\n", __PRETTY_FUNCTION__));

    if ((rc = DoSuperMethodA(CLASS, obj, (Msg)message)))
    {
    }
    return rc;
}
///

///MUIM_Hide()
/**************************************************************************
MUIM_Hide
**************************************************************************/
IPTR Icon__MUIM_Hide(struct IClass *CLASS, Object *obj, struct MUIP_Hide *message)
{
    struct Icon_DATA *data = INST_DATA(CLASS, obj);
    IPTR                rc;

D(bug("[Icon]: %s()\n", __PRETTY_FUNCTION__));

    if ((rc = DoSuperMethodA(CLASS, obj, (Msg)message)))
    {
    }
    return rc;
}
///

///MUIM_Cleanup()
/**************************************************************************
MUIM_Cleanup
**************************************************************************/
IPTR Icon__MUIM_Cleanup(struct IClass *CLASS, Object *obj, struct MUIP_Cleanup *message)
{
    struct Icon_DATA *data = INST_DATA(CLASS, obj);
    struct IconEntry    *node = NULL;

D(bug("[Icon]: %s()\n", __PRETTY_FUNCTION__));

    return DoSuperMethodA(CLASS, obj, (Msg)message);
}
///

///MUIM_AskMinMax()
/**************************************************************************
MUIM_AskMinMax
**************************************************************************/
IPTR Icon__MUIM_AskMinMax(struct IClass *CLASS, Object *obj, struct MUIP_AskMinMax *message)
{
    ULONG rc = DoSuperMethodA(CLASS, obj, (Msg) message);

D(bug("[Icon]: %s()\n", __PRETTY_FUNCTION__));

    /* Get Icon "Image" Size in MinN */
    message->MinMaxInfo->MinWidth  += 96;
    message->MinMaxInfo->MinHeight += 64;

    /* Get Icon "Label" Size in DefN */
    message->MinMaxInfo->DefWidth  += 200;
    message->MinMaxInfo->DefHeight += 180;

    /* Get Complete Icon Area Size in MaxN */
    message->MinMaxInfo->MaxWidth = MUI_MAXMAX;
    message->MinMaxInfo->MaxHeight = MUI_MAXMAX;

    return rc;
}
///

///MUIM_Layout()
/**************************************************************************
MUIM_Layout
**************************************************************************/
IPTR Icon__MUIM_Layout(struct IClass *CLASS, Object *obj,struct MUIP_Layout *message)
{
    struct Icon_DATA *data = INST_DATA(CLASS, obj);
    ULONG rc;

D(bug("[Icon]: %s()\n", __PRETTY_FUNCTION__));

    rc = DoSuperMethodA(CLASS, obj, (Msg)message);

    return rc;
}
///

///MUIM_Draw()
/**************************************************************************
MUIM_Draw - draw the Icon
**************************************************************************/
IPTR DrawCount;
IPTR Icon__MUIM_Draw(struct IClass *CLASS, Object *obj, struct MUIP_Draw *message)
{   
    struct Icon_DATA    *data = INST_DATA(CLASS, obj);
    struct IconEntry       *icon = NULL;

    APTR                   clip;

    ULONG                  update_oldwidth = 0,
                           update_oldheight = 0;

    LONG                  clear_xoffset = 0,
                           clear_yoffset = 0;

    IPTR          draw_id = DrawCount++;

D(bug("[Icon]: %s(obj @ 0x%p)\n", __PRETTY_FUNCTION__, obj));

D(bug("[Icon] %s: id %d\n", __PRETTY_FUNCTION__, draw_id));
    
    DoSuperMethodA(CLASS, obj, (Msg)message);

    if (!(data->icld__Option_IconFixedBackground))
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
        if (message->flags & MADF_DRAWUPDATE)
        {
D(bug("[Icon] %s#%d: MADF_DRAWUPDATE\n", __PRETTY_FUNCTION__, draw_id));
        }
        else
        {
D(bug("[Icon] %s#%d: UPDATE_RESIZE\n", __PRETTY_FUNCTION__, draw_id));
        }
#endif
        if ((data->icld_UpdateMode == UPDATE_SINGLEICON) && (data->update_icon != NULL)) /* draw only a single icon at update_icon */
        {
            struct Rectangle rect;

D(bug("[Icon] %s#%d: UPDATE_SINGLEICON (icon @ 0x%p)\n", __PRETTY_FUNCTION__, draw_id, data->update_icon));

            Icon_GetIconAreaRectangle(obj, data, data->update_icon, &rect);
    
            rect.MinX += _mleft(obj) + (data->update_icon->IcD_IconX - data->icld_ViewX);
            rect.MaxX += _mleft(obj) + (data->update_icon->IcD_IconX - data->icld_ViewX);
            rect.MinY += _mtop(obj) + (data->update_icon->IcD_IconY - data->icld_ViewY);
            rect.MaxY += _mtop(obj) + (data->update_icon->IcD_IconY - data->icld_ViewY);

            if (data->icld__Option_IconMode == ICON_LISTMODE_GRID)
            {
                if (data->update_icon->IcD_AreaWidth < data->icld_IconAreaLargestWidth)
                {
                    rect.MinX += ((data->icld_IconAreaLargestWidth - data->update_icon->IcD_AreaWidth)/2);
                    rect.MaxX += ((data->icld_IconAreaLargestWidth - data->update_icon->IcD_AreaWidth)/2);
                }

                if (data->update_icon->IcD_AreaHeight < data->icld_IconAreaLargestHeight)
                {
                    rect.MinY += ((data->icld_IconAreaLargestHeight - data->update_icon->IcD_AreaHeight)/2);
                    rect.MaxY += ((data->icld_IconAreaLargestHeight - data->update_icon->IcD_AreaHeight)/2);
                }
            }

            clip = MUI_AddClipping(muiRenderInfo(obj), _mleft(obj), _mtop(obj), _mwidth(obj), _mheight(obj));

#if defined(DEBUG_ILC_ICONRENDERING)
D(bug("[Icon] %s#%d: UPDATE_SINGLEICON: Calling MUIM_DrawBackground (A)\n", __PRETTY_FUNCTION__, draw_id));
#endif
            DoMethod(obj, MUIM_DrawBackground, 
                rect.MinX, rect.MinY,
                rect.MaxX - rect.MinX + 1, rect.MaxY - rect.MinY + 1,
                clear_xoffset, clear_yoffset, 
                0);

            /* We could have deleted also other icons so they must be redrawn */
            #ifdef __AROS__
      ForeachNode(&data->icld_Icon, icon)
      #else
      Foreach_Node(&data->icld_Icon, icon);
        #endif
            {
                if ((icon != data->update_icon) && (icon->IcD_Flags & ICONENTRY_FLAG_VISIBLE))
                {
                    struct Rectangle rect2;
                    Icon_GetIconAreaRectangle(obj, data, icon, &rect2);

                    rect2.MinX += _mleft(obj) - data->icld_ViewX + icon->IcD_IconX;
                    rect2.MaxX += _mleft(obj) - data->icld_ViewX + icon->IcD_IconX;
                    rect2.MinY += _mtop(obj) - data->icld_ViewY + icon->IcD_IconY;
                    rect2.MaxY += _mtop(obj) - data->icld_ViewY + icon->IcD_IconY;

                    if (data->icld__Option_IconMode == ICON_LISTMODE_GRID)
                    {
                        if (icon->IcD_AreaWidth < data->icld_IconAreaLargestWidth)
                        {
                            rect2.MinX += ((data->icld_IconAreaLargestWidth - icon->IcD_AreaWidth)/2);
                            rect2.MaxX += ((data->icld_IconAreaLargestWidth - icon->IcD_AreaWidth)/2);
                        }

                        if (icon->IcD_AreaHeight < data->icld_IconAreaLargestHeight)
                        {
                            rect2.MinY += ((data->icld_IconAreaLargestHeight - icon->IcD_AreaHeight)/2);
                            rect2.MaxY += ((data->icld_IconAreaLargestHeight - icon->IcD_AreaHeight)/2);
                        }
                    }

                    if (RectAndRect(&rect, &rect2))
                    {  
                        // Update icon here
                        DoMethod(obj, MUIM_Icon_DrawEntry, icon, ICONENTRY_DRAWMODE_PLAIN);
                        DoMethod(obj, MUIM_Icon_DrawEntryLabel, icon, ICONENTRY_DRAWMODE_PLAIN);
                    }
                }
            }

            DoMethod(obj, MUIM_Icon_DrawEntry, data->update_icon, ICONENTRY_DRAWMODE_PLAIN);
            DoMethod(obj, MUIM_Icon_DrawEntryLabel, data->update_icon, ICONENTRY_DRAWMODE_PLAIN);
            data->icld_UpdateMode = 0;
            data->update_icon = NULL;

            if (data->icld_DisplayRastPort != data->icld_BufferRastPort)
            {
#if defined(DEBUG_ILC_ICONRENDERING)
D(bug("[Icon] %s#%d: UPDATE_SINGLEICON Blitting to front rastport..\n", __PRETTY_FUNCTION__, draw_id));
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
D(bug("[Icon] %s#%d: UPDATE_SCROLL.\n", __PRETTY_FUNCTION__, draw_id));
#endif 

            if (!data->icld__Option_IconFixedBackground)
            {
                scroll_caused_damage = (_rp(obj)->Layer->Flags & LAYERREFRESH) ? FALSE : TRUE;
        
                data->icld_UpdateMode = 0;

                if ((abs(data->update_scrolldx) >= _mwidth(obj)) ||
                    (abs(data->update_scrolldy) >= _mheight(obj)))
                {
#if defined(DEBUG_ILC_ICONRENDERING)
D(bug("[Icon] %s#%d: UPDATE_SCROLL: Moved outside current view\n", __PRETTY_FUNCTION__, draw_id));
#endif 
                    MUI_Redraw(obj, MADF_DRAWOBJECT);
                    goto draw_done;
                }

                if (!(region = NewRegion()))
                {
#if defined(DEBUG_ILC_ICONRENDERING)
D(bug("[Icon] %s#%d: UPDATE_SCROLL: Couldnt Alloc Region\n", __PRETTY_FUNCTION__, draw_id));
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
D(bug("[Icon] %s#%d: UPDATE_SCROLL: Scrolling Raster..\n", __PRETTY_FUNCTION__, draw_id));
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
D(bug("[Icon] %s#%d: UPDATE_SCROLL: Causing Redraw..\n", __PRETTY_FUNCTION__, draw_id));
#endif
            MUI_Redraw(obj, MADF_DRAWOBJECT);

            data->update_rect1 = data->update_rect2 = NULL;

            if (!data->icld__Option_IconFixedBackground)
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
            if (data->icld_DisplayRastPort != data->icld_BufferRastPort)
            {
#if defined(DEBUG_ILC_ICONRENDERING)
D(bug("[Icon] %s#%d: UPDATE_SCROLL: Blitting to front rastport..\n", __PRETTY_FUNCTION__, draw_id));
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
D(bug("[Icon] %s#%d: UPDATE_RESIZE.\n", __PRETTY_FUNCTION__, draw_id));
#endif 

            if ((data->icld_BufferRastPort) && (data->icld_BufferRastPort != data->icld_DisplayRastPort))
            {
                //Free up the buffers rastport and bitmap so we can replace them ..
                struct Bitmap *bitmap_Old = data->icld_BufferRastPort->BitMap;
                struct Bitmap *bitmap_New;
    
    ULONG tmp_RastDepth;

                data->icld_BufferRastPort->BitMap = NULL;

                FreeRastPort(data->icld_BufferRastPort);
                SET(obj, MUIA_Icon_BufferRastport, NULL);

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
                        SET(obj, MUIA_Icon_BufferRastport, data->icld_BufferRastPort);
                        data->icld_DrawOffsetX = 0;
                        data->icld_DrawOffsetY = 0;
                    }
                    else
                    {
                        FreeBitMap(bitmap_New);
                        data->icld_BufferRastPort = data->icld_DisplayRastPort;
                        data->icld_DrawOffsetX = _mleft(obj);
                        data->icld_DrawOffsetY = _mtop(obj);
                    }
                }
                
                if (bitmap_Old != data->icld_BufferRastPort->BitMap)
                    FreeBitMap(bitmap_Old);
            }

            data->icld_UpdateMode = 0;

            if (!data->icld__Option_IconScaledBackground)
            {
                if (!(region = NewRegion()))
                {
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

            MUI_Redraw(obj, MADF_DRAWOBJECT);

            if (!data->icld__Option_IconScaledBackground)
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
D(bug("[Icon] %s#%d: MADF_DRAWOBJECT\n", __PRETTY_FUNCTION__, draw_id));
#endif

        clip = MUI_AddClipping(muiRenderInfo(obj), _mleft(obj), _mtop(obj), _mwidth(obj), _mheight(obj));

        viewrect.MinX = _mleft(obj);
        viewrect.MaxX = _mleft(obj) + _mwidth(obj);
        viewrect.MinY = _mtop(obj);
        viewrect.MaxY = _mtop(obj) + _mheight(obj);

#if defined(DEBUG_ILC_ICONRENDERING)
D(bug("[Icon] %s#%d: MADF_DRAWOBJECT: Calling MUIM_DrawBackground (B)\n", __PRETTY_FUNCTION__, draw_id));
#endif
        DoMethod(
            obj, MUIM_DrawBackground, _mleft(obj), _mtop(obj), _mwidth(obj), _mheight(obj),
            clear_xoffset, clear_yoffset, 0
        );
  #ifdef __AROS__
        ForeachNode(&data->icld_Icon, icon)
  #else
        Foreach_Node(&data->icld_Icon, icon);
  #endif
        {
            if ((icon->IcD_Flags & ICONENTRY_FLAG_VISIBLE) &&
                (icon->IcD_DiskObj) &&
                (icon->IcD_IconX != NO_ICON_POSITION) &&
                (icon->IcD_IconY != NO_ICON_POSITION))
            {
                struct Rectangle iconrect;
                Icon_GetIconAreaRectangle(obj, data, icon, &iconrect);

                iconrect.MinX += _mleft(obj) - data->icld_ViewX + icon->IcD_IconX;
                iconrect.MaxX += _mleft(obj) - data->icld_ViewX + icon->IcD_IconX;
                iconrect.MinY += _mtop(obj) - data->icld_ViewY + icon->IcD_IconY;
                iconrect.MaxY += _mtop(obj) - data->icld_ViewY + icon->IcD_IconY;

                if (RectAndRect(&viewrect, &iconrect))
                {
                    DoMethod(obj, MUIM_Icon_DrawEntry, icon, ICONENTRY_DRAWMODE_PLAIN);
                    DoMethod(obj, MUIM_Icon_DrawEntryLabel, icon, ICONENTRY_DRAWMODE_PLAIN);
                }
            }
        }

        if (data->icld_DisplayRastPort != data->icld_BufferRastPort)
        {
#if defined(DEBUG_ILC_ICONRENDERING)
D(bug("[Icon] %s#%d: MADF_DRAWOBJECT: Blitting to front rastport..\n", __PRETTY_FUNCTION__, draw_id));
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
    
    D(bug("[Icon] %s: Draw finished for id %d\n", __PRETTY_FUNCTION__, draw_id));

    return 0;
}
///

///Icon__MUIM_Icon_Update()
/**************************************************************************
MUIM_Icon_Refresh
Implemented by subclasses
**************************************************************************/
IPTR Icon__MUIM_Icon_Update(struct IClass *CLASS, Object *obj, struct MUIP_Icon_Update *message)
{
    struct Icon_DATA *data = INST_DATA(CLASS, obj);

D(bug("[Icon]: %s()\n", __PRETTY_FUNCTION__));

    return 1;
}
///

///Icon__MUIM_Icon_DestroyEntry()
IPTR Icon__MUIM_Icon_DestroyEntry(struct IClass *CLASS, Object *obj, struct MUIP_Icon_DestroyEntry *message)
{
    struct Icon_DATA *data = INST_DATA(CLASS, obj);

D(bug("[Icon]: %s()\n", __PRETTY_FUNCTION__));

    if (message->icon)
    {
        if (message->icon->IcD_Flags & ICONENTRY_FLAG_SELECTED)
        {
            Remove(&message->icon->IcD_SelectionNode);
        }
        FreeVecPooled(data->icld_Pool, message->icon->IcD_DisplayedLabel_TXTBUFF);
        if (message->icon->IcD_TxtBuf_PROT)
            FreePooled(data->icld_Pool, message->icon->IcD_TxtBuf_PROT, 8);
        if (message->icon->IcD_Size_TXTBUFF)
            FreePooled(data->icld_Pool, message->icon->IcD_Size_TXTBUFF, 30);
        if (message->icon->IcD_Time_TXTBUFF)
            FreePooled(data->icld_Pool, message->icon->IcD_Time_TXTBUFF,
                LEN_DATSTRING);
        if (message->icon->IcD_Date_TXTBUFF)
            FreePooled(data->icld_Pool, message->icon->IcD_Date_TXTBUFF,
                LEN_DATSTRING);
        if (message->icon->IcD_DiskObj) FreeDiskObject(message->icon->IcD_DiskObj);
        if (message->icon->IcD_Label_TXTBUFF) FreePooled(data->icld_Pool, message->icon->IcD_Label_TXTBUFF, strlen(message->icon->IcD_Label_TXTBUFF)+1);
        if (message->icon->IcD_IconEntry.filename) FreePooled(data->icld_Pool, message->icon->IcD_IconEntry.filename, strlen(message->icon->IcD_IconEntry.filename)+1);
    }
    return TRUE;
}
///

///Icon__MUIM_Icon_CreateEntry()
/**************************************************************************
MUIM_Icon_CreateEntry.
Returns 0 on failure otherwise it returns the icons entry ..
**************************************************************************/
IPTR Icon__MUIM_Icon_CreateEntry(struct IClass *CLASS, Object *obj, struct MUIP_Icon_CreateEntry *message)
{

}
///

#if WANDERER_BUILTIN_ICONLIST
BOOPSI_DISPATCHER(IPTR,Icon_Dispatcher, CLASS, obj, message)
{
    #ifdef __AROS__
    switch (message->MethodID)
    #else
    struct IClass *CLASS = cl;
    Msg message = msg;

    switch (msg->MethodID)
    #endif
    {
        case OM_NEW:                            return Icon__OM_NEW(CLASS, obj, (struct opSet *)message);
        case OM_DISPOSE:                        return Icon__OM_DISPOSE(CLASS, obj, message);
        case OM_SET:                            return Icon__OM_SET(CLASS, obj, (struct opSet *)message);
        case OM_GET:                            return Icon__OM_GET(CLASS, obj, (struct opGet *)message);
        
        case MUIM_Setup:                        return Icon__MUIM_Setup(CLASS, obj, (struct MUIP_Setup *)message);
        
        case MUIM_Show:                         return Icon__MUIM_Show(CLASS,obj, (struct MUIP_Show *)message);
        case MUIM_Hide:                         return Icon__MUIM_Hide(CLASS,obj, (struct MUIP_Hide *)message);
        case MUIM_Cleanup:                      return Icon__MUIM_Cleanup(CLASS, obj, (struct MUIP_Cleanup *)message);
        case MUIM_AskMinMax:                    return Icon__MUIM_AskMinMax(CLASS, obj, (struct MUIP_AskMinMax *)message);
        case MUIM_Draw:                         return Icon__MUIM_Draw(CLASS, obj, (struct MUIP_Draw *)message);
  #ifdef __AROS__
        case MUIM_Layout:                       return Icon__MUIM_Layout(CLASS, obj, (struct MUIP_Layout *)message);
  #endif
        case MUIM_CreateDragImage:              return Icon__MUIM_CreateDragImage(CLASS, obj, (APTR)message);
        case MUIM_DeleteDragImage:              return Icon__MUIM_DeleteDragImage(CLASS, obj, (APTR)message);

        case MUIM_Icon_DrawEntry:           return Icon__MUIM_Icon_DrawEntry(CLASS, obj, (APTR)message);
        case MUIM_Icon_DrawEntryLabel:      return Icon__MUIM_Icon_DrawEntryLabel(CLASS, obj, (APTR)message);
        case MUIM_Icon_GetIconPrivate:      return Icon__MUIM_Icon_GetIconPrivate(CLASS, obj, (APTR)message);
    }
    return DoSuperMethodA(CLASS, obj, message);
}
BOOPSI_DISPATCHER_END

#ifdef __AROS__
/* Class descriptor. */
const struct __MUIBuiltinClass _MUI_Icon_desc = { 
    MUIC_Icon, 
    MUIC_Area, 
    sizeof(struct Icon_DATA), 
    (void*)Icon_Dispatcher
};
#endif
#endif

#ifndef __AROS__
struct MUI_CustomClass  *initIconClass(void)
{
  return (struct MUI_CustomClass *) MUI_CreateCustomClass(NULL, MUIC_Area, NULL, sizeof(struct Icon_DATA), ENTRY(Icon_Dispatcher));
}

#endif
