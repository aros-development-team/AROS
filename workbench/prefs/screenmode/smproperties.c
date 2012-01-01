/*
    Copyright © 2003-2011, The AROS Development Team. All rights reserved.
    $Id$
*/

#define MUIMASTER_YES_INLINE_STDARG
#define DEBUG 0

#include <libraries/mui.h>

#include <proto/alib.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/muimaster.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/utility.h>

#include <zune/customclasses.h>

#include "locale.h"

#include "smproperties.h"

struct ScreenModeProperties_DATA
{
    Object *depth, *def_width, *def_height;

    Object *objWidth;
    Object *objHeight;
    Object *autoscroll;
    
    ULONG DisplayID;
    UWORD MinWidth, MinHeight;
    UWORD MaxWidth, MaxHeight;
    UWORD DefWidth, DefHeight;
    UWORD DefDepth;
    BOOL  VariableDepth;
};

#define CheckMarkObject                              \
    ImageObject,                                     \
        ImageButtonFrame,                            \
        MUIA_InputMode      , MUIV_InputMode_Toggle, \
        MUIA_Image_Spec     , MUII_CheckMark,        \
        MUIA_Image_FreeVert , TRUE,                  \
        MUIA_Background     , MUII_ButtonBack,       \
        MUIA_ShowSelState   , FALSE                 

#define HLeft(obj...) \
    (IPTR)(HGroup, (IPTR)GroupSpacing(0), Child, (IPTR)(obj), Child, (IPTR)HSpace(0), End)

#undef HCenter
#define HCenter(obj...) \
    (HGroup, (IPTR)GroupSpacing(0), Child, (IPTR)HSpace(0), Child, (IPTR)(obj), Child, \
    (IPTR)HSpace(0), End)

Object *ScreenModeProperties__OM_NEW(Class *CLASS, Object *self, struct opSet *message)
{
    struct ScreenModeProperties_DATA *data;
    Object *objWidth, *objHeight, *depth,
           *def_width, *def_height;

    Object *autoscroll;
    
    ULONG id;
    
    self = (Object *)DoSuperNewTags
    (
        CLASS, self, NULL,
        MUIA_Group_Horiz, TRUE,
        Child, (IPTR)ColGroup(4),
            Child, (IPTR)Label1(__(MSG_WIDTH)),
            Child, HLeft(objWidth = (Object *)StringObject,
                StringFrame,
                MUIA_String_Accept, (IPTR)"0123456789",
                MUIA_String_MaxLen, 6,
                MUIA_CycleChain, TRUE,
            End),
            Child, (IPTR)(def_width = (Object *)CheckMarkObject, MUIA_CycleChain, TRUE, End),
            Child, (IPTR)Label1(__(MSG_DEFAULT)),
                
            Child, (IPTR)Label1(__(MSG_HEIGHT)),
            Child, HLeft(objHeight = (Object *)StringObject,
                StringFrame,
                MUIA_String_Accept, (IPTR)"0123456789",
                MUIA_String_MaxLen, 6,
                MUIA_CycleChain, TRUE,
            End),
            Child, HLeft(def_height = (Object *)CheckMarkObject, MUIA_CycleChain, TRUE, End),
            Child, (IPTR)Label1(__(MSG_DEFAULT)),
                
            Child, (IPTR)Label1(__(MSG_DEPTH)),
            Child, HLeft(depth = (Object *)NumericbuttonObject, MUIA_CycleChain, TRUE, End),
            Child, (IPTR)RectangleObject, End,
            Child, (IPTR)RectangleObject, End,

            Child, (IPTR)Label1(__(MSG_AUTOSCROLL)),
            Child, HLeft(autoscroll = (Object *)CheckMarkObject, MUIA_CycleChain, TRUE, End),
            Child, (IPTR)RectangleObject, End,
            Child, (IPTR)RectangleObject, End,

        End,  
        
        TAG_MORE, (IPTR)message->ops_AttrList
    );
    
    if (!self)
        goto err;
    
    D(bug("[smproperties] Created ScreenModeProperties object 0x%p\n", self));
    data = INST_DATA(CLASS, self);    
    
    data->objWidth   = objWidth;
    data->objHeight  = objHeight;
    data->depth      = depth;
    data->def_width  = def_width;
    data->def_height = def_height;
    data->autoscroll = autoscroll;

    DoMethod
    (
        objWidth, MUIM_Notify, MUIA_String_Acknowledge, MUIV_EveryTime,
        (IPTR)self, 3,
        MUIM_Set, MUIA_ScreenModeProperties_WidthString, MUIV_TriggerValue
    );
    
    DoMethod
    (
        objHeight, MUIM_Notify, MUIA_String_Acknowledge, MUIV_EveryTime,
        (IPTR)self, 3,
        MUIM_Set, MUIA_ScreenModeProperties_HeightString, MUIV_TriggerValue
    );
    
    DoMethod
    (
        depth, MUIM_Notify, MUIA_Numeric_Value, MUIV_EveryTime,
        (IPTR)self, 3,
        MUIM_Set, MUIA_ScreenModeProperties_Depth, MUIV_TriggerValue
    );
    
    DoMethod
    (
        def_width, MUIM_Notify, MUIA_Selected, TRUE,
        (IPTR)self, 3,
        MUIM_Set, MUIA_ScreenModeProperties_Width, -1
    );

    DoMethod
    (
        def_width, MUIM_Notify, MUIA_Selected, MUIV_EveryTime,
        (IPTR)objWidth, 3,
        MUIM_Set, MUIA_Disabled, MUIV_TriggerValue
    );
    
    DoMethod
    (
        def_height, MUIM_Notify, MUIA_Selected, TRUE,
        (IPTR)self, 3,
        MUIM_Set, MUIA_ScreenModeProperties_Height, -1
    );
    
    DoMethod
    (
        def_height, MUIM_Notify, MUIA_Selected, MUIV_EveryTime,
        (IPTR)objHeight, 3,
        MUIM_Set, MUIA_Disabled, MUIV_TriggerValue
    );

    id = GetTagData(MUIA_ScreenModeProperties_DisplayID, INVALID_ID, message->ops_AttrList);
    D(bug("[smproperties] Setting initial ModeID 0x%08lX\n", id));
    set(self, MUIA_ScreenModeProperties_DisplayID, id);
    
    return self;

err:
    CoerceMethod(CLASS, self, OM_DISPOSE);
    return NULL;
}

static inline UWORD AdjustWidth(UWORD width, struct ScreenModeProperties_DATA *data)
{
    if (width < data->MinWidth)
        width = data->MinWidth;
    if (width > data->MaxWidth)
        width = data->MaxWidth;
    D(bug("[smproperties] Adjusted width = %lu\n", width));
    return width;
}

static inline UWORD AdjustHeight(UWORD height, struct ScreenModeProperties_DATA *data)
{
    if (height < data->MinHeight)
        height = data->MinHeight;
    if (height > data->MaxHeight)
        height = data->MaxHeight;
    D(bug("[smproperties] Adjusted height = %lu\n", height));
    return height;
}

IPTR ScreenModeProperties__OM_SET(Class *CLASS, Object *self, struct opSet *message)
{
    struct ScreenModeProperties_DATA *data = INST_DATA(CLASS, self);    
    struct TagItem *tags;
    struct TagItem *tag;
    ULONG id        = INVALID_ID;
    IPTR  no_notify = TAG_IGNORE;
    IPTR ret;
    WORD depth;
    LONG width, height;
    
    DB2(bug("[smproperties] OM_SET called\n"));
    for (tags = message->ops_AttrList; (tag = NextTagItem(&tags)); )
    {
        switch (tag->ti_Tag)
        {
            case MUIA_NoNotify:
                no_notify = MUIA_NoNotify;
                break;
                
            case MUIA_ScreenModeProperties_DisplayID:
            {
                struct TagItem depth_tags[] =
                {
                    { MUIA_NoNotify,        TRUE },
                    { MUIA_Numeric_Min,        0 },
                    { MUIA_Numeric_Max,        0 },
                    { MUIA_Numeric_Default,    0 },
                    { MUIA_Disabled,	    FALSE},
                    { TAG_DONE,                0 }
                };
                
                struct DimensionInfo dim;
                BOOL autoscroll;

                D(bug("[smproperties] Set DisplayID = 0x%08lx\n", tag->ti_Data));
                
                if (GetDisplayInfoData(NULL, (UBYTE *)&dim, sizeof(dim), DTAG_DIMS, tag->ti_Data))
                {
                    IPTR isdefault, val;

                    depth_tags[1].ti_Data  = dim.MaxDepth > 8 ? dim.MaxDepth : 1;
                    depth_tags[2].ti_Data  = dim.MaxDepth;
                    depth_tags[3].ti_Data  = dim.MaxDepth;

                    id = tag->ti_Data;
                    data->DefWidth = dim.Nominal.MaxX - dim.Nominal.MinX + 1;
                    data->DefHeight = dim.Nominal.MaxY - dim.Nominal.MinY + 1;
                    data->DefDepth = depth_tags[3].ti_Data;
                    data->MinWidth = dim.MinRasterWidth;
                    data->MinHeight = dim.MinRasterHeight;
                    data->MaxWidth = dim.MaxRasterWidth;
                    data->MaxHeight = dim.MaxRasterHeight;

                    D(bug("[smproperties] Obtained DimensionsInfo:\n"));
                    D(bug("[smproperties] Minimum raster: %lux%lux1\n", dim.MinRasterWidth, dim.MinRasterHeight));
                    D(bug("[smproperties] Maximum raster: %lux%lux%lu\n", dim.MaxRasterWidth, dim.MaxRasterHeight, dim.MaxDepth));
                    D(bug("[smproperties] Display size: %lux%lu\n", data->DefWidth, data->DefHeight));
 
                    GetAttr(MUIA_Selected, data->def_width, &isdefault);
                    if (isdefault)
                        width = data->DefWidth;
                    else
                    {
                        GetAttr(MUIA_Numeric_Value, data->objWidth, &val);
                        width = AdjustWidth((LONG)val, data);
                    }

                    GetAttr(MUIA_Selected, data->def_height, &isdefault);
                    if (isdefault)
                        height = data->DefHeight;
                    else
                    {
                        GetAttr(MUIA_String_Integer, data->objHeight, &val);
                        height = AdjustHeight((LONG)val, data);
                    }

                    data->VariableDepth = TRUE;
                    GetAttr(MUIA_Numeric_Value, data->depth, &val);
                    /* Original AmigaOS screenmode prefs do not allow to change depth for CyberGFX
                     * screnmodes if it is high or true color screenmode. */
                    if (dim.MaxDepth > 8)
                    {
                        data->VariableDepth = FALSE;
                        depth_tags[3].ti_Tag = MUIA_Numeric_Value;
                        depth_tags[4].ti_Data = TRUE;
                    } else if (val > dim.MaxDepth) {
                    	/* Make sure depth is always <= dim.MaxDepth */
                        depth_tags[3].ti_Tag = MUIA_Numeric_Value;
                        depth_tags[3].ti_Data = dim.MaxDepth;
                    }	
                }
               
                /* Enable autoscroll if one of the maximum sizes is bigger than 
                   the resolution.  */
                   
                autoscroll = data->MaxWidth > data->DefWidth  ||
                             data->MaxHeight > data->DefHeight;
    
                data->DisplayID = id;

                SetAttrs(self, MUIA_Disabled, id == INVALID_ID, TAG_DONE);
                if (id == INVALID_ID)
                {
                    nnset(data->def_width, MUIA_Selected, TRUE);
                    nnset(data->def_height, MUIA_Selected, TRUE);
                    depth_tags[4].ti_Tag = TAG_DONE;
                }
                else
                {
                    IPTR isdefault;
                    GetAttr(MUIA_Selected, data->def_height, &isdefault);
                    SetAttrs(data->objHeight, MUIA_Disabled, isdefault, TAG_DONE);
                    GetAttr(MUIA_Selected, data->def_width, &isdefault);
                    SetAttrs(data->objWidth, MUIA_Disabled, isdefault, TAG_DONE);
                }

                SetAttrs(data->objWidth, MUIA_String_Integer, width, TAG_DONE);
                SetAttrs(data->objHeight, MUIA_String_Integer, height, TAG_DONE);
                SetAttrsA(data->depth,  depth_tags);

                SetAttrs(data->autoscroll, no_notify, TRUE, 
                                           MUIA_Disabled, !autoscroll,
                                           MUIA_Selected, autoscroll,
                                           TAG_DONE);
                
                break;
            }

            case MUIA_ScreenModeProperties_WidthString:
                D(bug("[smproperties] Set WidthString = %s\n", (CONST_STRPTR)tag->ti_Data));
                StrToLong((CONST_STRPTR)tag->ti_Data, &width);
                SetAttrs(self, MUIA_ScreenModeProperties_Width, width, TAG_DONE);
                break;

            case MUIA_ScreenModeProperties_Width:
                width = tag->ti_Data;

                D(bug("[smproperties] Set Width = %ld\n", width));
                if (width == -1)
                    width = data->DefWidth;
                else
                    width = AdjustWidth(width, data);
                SetAttrs(data->objWidth, MUIA_NoNotify, TRUE, MUIA_String_Integer, width, TAG_DONE);
                SetAttrs(data->def_width, MUIA_NoNotify, TRUE, MUIA_Selected, width == data->DefWidth, TAG_DONE);
                SetAttrs(data->objWidth, MUIA_NoNotify, TRUE, MUIA_Disabled, width == data->DefWidth, TAG_DONE);
                break;

            case MUIA_ScreenModeProperties_HeightString:
                D(bug("[smproperties] Set HeightString = %s\n", (CONST_STRPTR)tag->ti_Data));
                StrToLong((CONST_STRPTR)tag->ti_Data, &height);
                SetAttrs(self, MUIA_ScreenModeProperties_Height, height, TAG_DONE);
                break;
                
            case MUIA_ScreenModeProperties_Height:
                height = tag->ti_Data;

                D(bug("[smproperties] Set Height = %ld\n", height));
                if (height == -1)
                    height = data->DefHeight;
                else
                    height = AdjustHeight(height, data);
                SetAttrs(data->objHeight, MUIA_NoNotify, TRUE, MUIA_String_Integer, height, TAG_DONE);
                SetAttrs(data->def_height, MUIA_NoNotify, TRUE, MUIA_Selected, height == data->DefHeight, TAG_DONE);
                SetAttrs(data->objHeight, MUIA_NoNotify, TRUE, MUIA_Disabled, height == data->DefHeight, TAG_DONE);
                break;
            
            case MUIA_ScreenModeProperties_Depth:
                if (data->VariableDepth)
                {
                    depth = tag->ti_Data;

                    D(bug("[smproperties] Set Depth = %ld\n", depth));
                    if (depth == -1)
                        depth = data->DefDepth;
                    SetAttrs(data->depth, no_notify, TRUE, MUIA_Numeric_Value, depth, TAG_DONE);
                }
                break;
            
            case MUIA_ScreenModeProperties_Autoscroll:

                D(bug("[smproperties] Set Autoscroll = %lu\n", tag->ti_Data));
                if (id != INVALID_ID && !XGET(data->autoscroll, MUIA_Disabled))
                    SetAttrs(data->autoscroll, no_notify, TRUE, MUIA_Selected, tag->ti_Data != 0);
                break;
        }
    }

    DB2(bug("[smproperties] Calling OM_SET() on superclass\n"));
    ret = DoSuperMethodA(CLASS, self, (Msg)message);
    DB2(bug("[smproperties] OM_SET() on superclass returned %ld\n", ret));
    return ret;
}

IPTR ScreenModeProperties__OM_GET(Class *CLASS, Object *self, struct opGet *message)
{
    struct ScreenModeProperties_DATA *data = INST_DATA(CLASS, self);    
    
    switch (message->opg_AttrID)
    {
        case MUIA_ScreenModeProperties_DisplayID:
            *message->opg_Storage = data->DisplayID;
            break;
        
        case MUIA_ScreenModeProperties_Width:
            *message->opg_Storage = XGET(data->objWidth, MUIA_String_Integer);
            break;
        
        case MUIA_ScreenModeProperties_Height:
            *message->opg_Storage = XGET(data->objHeight, MUIA_String_Integer);
            break;
        
        case MUIA_ScreenModeProperties_Depth:
            *message->opg_Storage = XGET(data->depth, MUIA_Numeric_Value);
            break;
        
        case MUIA_ScreenModeProperties_Autoscroll:
            *message->opg_Storage = XGET(data->autoscroll, MUIA_Selected);
            break;
        
        default:
            return DoSuperMethodA(CLASS, self, (Msg)message);
    }
    
    return TRUE;
}

ZUNE_CUSTOMCLASS_3
(
    ScreenModeProperties, NULL, MUIC_Group, NULL,   
    OM_NEW,     struct opSet *,
    OM_GET,     struct opGet *,
    OM_SET,     struct opSet *
);
