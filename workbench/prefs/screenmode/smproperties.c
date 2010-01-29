/*
    Copyright © 2003-2010, The AROS Development Team. All rights reserved.
    $Id$
*/

#define MUIMASTER_YES_INLINE_STDARG
#define DEBUG 0

#include <libraries/mui.h>

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
    Object *width, *height, *depth,
           *def_width, *def_height;
           
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
    Object *width, *height, *depth,
           *def_width, *def_height;

    Object *autoscroll;
    
    ULONG id;
    
    self = (Object *)DoSuperNewTags
    (
        CLASS, self, NULL,
        MUIA_Group_Horiz, TRUE,
        Child, (IPTR)ColGroup(4),
            Child, (IPTR)Label1(__(MSG_WIDTH)),
            Child, HLeft(width = (Object *)NumericbuttonObject, End),
            Child, (IPTR)(def_width = (Object *)CheckMarkObject, End),
            Child, (IPTR)Label1(__(MSG_DEFAULT)),
                
            Child, (IPTR)Label1(__(MSG_HEIGHT)),
            Child, HLeft(height = (Object *)NumericbuttonObject, End),
            Child, (IPTR)(def_height = (Object *)CheckMarkObject, End),
            Child, (IPTR)Label1(__(MSG_DEFAULT)),
                
            Child, (IPTR)Label1(__(MSG_DEPTH)),
            Child, HLeft(depth = (Object *)NumericbuttonObject, End),
            Child, (IPTR)RectangleObject, End,
            Child, (IPTR)RectangleObject, End,
        End,  
            
        Child, (IPTR)MUI_MakeObject(MUIO_VBar, 20),
            
        Child, (IPTR)HCenter(HGroup,
            Child, (IPTR)Label1(__(MSG_AUTOSCROLL)),
            Child, (IPTR)(autoscroll = (Object *)CheckMarkObject, End),
        End),
        
        TAG_MORE, (IPTR)message->ops_AttrList
    );
    
    if (!self)
        goto err;
    
    D(Printf("[smproperties] Created ScreenModeProperties object 0x%p\n", self));
    data = INST_DATA(CLASS, self);    
    
    data->width      = width;
    data->height     = height;
    data->depth      = depth;
    data->def_width  = def_width;
    data->def_height = def_height;
    data->autoscroll = autoscroll;
    
/*  Check me: this is likely not needed and can cause notifyloops - sonic

    DoMethod
    (
        width, MUIM_Notify, MUIA_Numeric_Value, MUIV_EveryTime,
        (IPTR)self, 3,
        MUIM_Set, MUIA_ScreenModeProperties_Width, MUIV_TriggerValue
    );
    
    DoMethod
    (
        height, MUIM_Notify, MUIA_Numeric_Value, MUIV_EveryTime,
        (IPTR)self, 3,
        MUIM_Set, MUIA_ScreenModeProperties_Height, MUIV_TriggerValue
    );
    
    DoMethod
    (
        depth, MUIM_Notify, MUIA_Numeric_Value, MUIV_EveryTime,
        (IPTR)self, 3,
        MUIM_Set, MUIA_ScreenModeProperties_Depth, MUIV_TriggerValue
    );*/
    
    DoMethod
    (
        def_width, MUIM_Notify, MUIA_Selected, TRUE,
        (IPTR)self, 3,
        MUIM_NoNotifySet, MUIA_ScreenModeProperties_Width, -1
    );

    DoMethod
    (
        width, MUIM_Notify, MUIA_Numeric_Value, MUIV_EveryTime,
        (IPTR)def_width, 3,
        MUIM_Set, MUIA_Selected, FALSE
    );
    
    DoMethod
    (
        def_height, MUIM_Notify, MUIA_Selected, TRUE,
        (IPTR)self, 3,
        MUIM_NoNotifySet, MUIA_ScreenModeProperties_Height, -1
    );

    DoMethod
    (
        height, MUIM_Notify, MUIA_Numeric_Value, MUIV_EveryTime,
        (IPTR)def_height, 3,
        MUIM_Set, MUIA_Selected, FALSE
    );
        
    id = GetTagData(MUIA_ScreenModeProperties_DisplayID, INVALID_ID, message->ops_AttrList);
    D(Printf("[smproperties] Setting initial ModeID 0x%08lX\n", id));
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
    D(Printf("[smproperties] Adjusted width = %lu\n", width));
    return width;
}

static inline UWORD AdjustHeight(UWORD height, struct ScreenModeProperties_DATA *data)
{
    if (height < data->MinHeight)
	height = data->MinHeight;
    if (height > data->MaxHeight)
	height = data->MaxHeight;
    D(Printf("[smproperties] Adjusted height = %lu\n", height));
    return height;
}

IPTR ScreenModeProperties__OM_SET(Class *CLASS, Object *self, struct opSet *message)
{
    struct ScreenModeProperties_DATA *data = INST_DATA(CLASS, self);    
    const struct TagItem *tags;
    struct TagItem *tag;
    ULONG id        = INVALID_ID;
    IPTR  no_notify = TAG_IGNORE;
    IPTR ret;
    WORD width, height, depth;
    
    DB2(Printf("[smproperties] OM_SET called\n"));
    for (tags = message->ops_AttrList; (tag = NextTagItem(&tags)); )
    {
        switch (tag->ti_Tag)
        {
            case MUIA_NoNotify:
                no_notify = MUIA_NoNotify;
                break;
                
            case MUIA_ScreenModeProperties_DisplayID:
            {
                struct TagItem width_tags[] =
                {
                    { MUIA_NoNotify,        TRUE },
                    { MUIA_Numeric_Min,        0 },
                    { MUIA_Numeric_Max,        0 },
                    { MUIA_Numeric_Default,    0 },
		    { MUIA_Numeric_Value,      0 },
                    { TAG_DONE,                0 }
                };
                struct TagItem height_tags[] =
                {
                    { MUIA_NoNotify,        TRUE },
                    { MUIA_Numeric_Min,        0 },
                    { MUIA_Numeric_Max,        0 },
                    { MUIA_Numeric_Default,    0 },
		    { MUIA_Numeric_Value,      0 },
                    { TAG_DONE,                0 }
                };
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
		struct DisplayInfo dinf;
                
                BOOL autoscroll;
		
		D(Printf("[smproperties] Set DisplayID = 0x%08lx\n", tag->ti_Data));
                
                if (GetDisplayInfoData(NULL, (UBYTE *)&dim, sizeof(dim), DTAG_DIMS, tag->ti_Data))
                {
		    IPTR width, height;

                    width_tags[1].ti_Data  = dim.MinRasterWidth;
                    height_tags[1].ti_Data = dim.MinRasterHeight;
                    depth_tags[1].ti_Data  = 1;
            
                    width_tags[2].ti_Data  = dim.MaxRasterWidth;
                    height_tags[2].ti_Data = dim.MaxRasterHeight;
                    depth_tags[2].ti_Data  = dim.MaxDepth;
            
                    width_tags[3].ti_Data  = dim.Nominal.MaxX - dim.Nominal.MinX + 1;
                    height_tags[3].ti_Data = dim.Nominal.MaxY - dim.Nominal.MinY + 1;
                    depth_tags[3].ti_Data  = dim.MaxDepth;
		    
		    D(Printf("[smproperties] Obtained DimensionsInfo:\n"));
		    D(Printf("[smproperties] Minimum raster: %lux%lux1\n", dim.MinRasterWidth, dim.MinRasterHeight));
		    D(Printf("[smproperties] Maximum raster: %lux%lux%lu\n", dim.MaxRasterWidth, dim.MaxRasterHeight, dim. MaxDepth));
		    D(Printf("[smproperties] Display size: %lux%lu\n", width_tags[3].ti_Data, height_tags[3].ti_Data));
                    
                    id = tag->ti_Data;
		    data->DefWidth = width_tags[3].ti_Data;
		    data->DefHeight = height_tags[3].ti_Data;
		    data->DefDepth = depth_tags[3].ti_Data;
		    data->MinWidth = dim.MinRasterWidth;
		    data->MinHeight = dim.MinRasterHeight;
		    data->MaxWidth = dim.MaxRasterWidth;
		    data->MaxHeight = dim.MaxRasterHeight;
		    
		    GetAttr(MUIA_Selected, data->def_width, &width);
		    GetAttr(MUIA_Selected, data->def_height, &height);
		    if (width)
		        width_tags[4].ti_Data = width_tags[3].ti_Data;
		    else {
		        GetAttr(MUIA_Numeric_Value, data->width, &width);
			width_tags[4].ti_Data = AdjustWidth(width, data);
		    }
		    if (height)
			height_tags[4].ti_Data = height_tags[3].ti_Data;
		    else {
		        GetAttr(MUIA_Numeric_Value, data->height, &height);
		        height_tags[4].ti_Data = AdjustHeight(height, data);
		    }
                }
		
		data->VariableDepth = TRUE;
		if (GetDisplayInfoData(NULL, (UBYTE *)&dinf, sizeof(dinf), DTAG_DISP, tag->ti_Data)) {
		
		    /* Check me: original AmigaOS screenmode prefs do not allow to change depth for CyberGFX
		       screenmodes. Here i attempt to do it too, however i don't know how it's detected.
		       Here i rely on DIPF_IS_FOREIGN flag - sonic */
		    if (dinf.PropertyFlags & DIPF_IS_FOREIGN) {
		        data->VariableDepth = FALSE;
		        depth_tags[3].ti_Tag = MUIA_Numeric_Value;
			depth_tags[4].ti_Data = TRUE;
		    }
		}
                
                /* Enable autoscroll only if the maximum sizes are bigger than 
                   the resolution.  */
                   
                autoscroll = width_tags[2].ti_Data  > width_tags[3].ti_Data  &&
                             height_tags[2].ti_Data > height_tags[3].ti_Data;
    
                data->DisplayID = id;
		
                SetAttrs(self, MUIA_Disabled, id == INVALID_ID, TAG_DONE);
		if (id == INVALID_ID) {
		    nnset(data->def_width, MUIA_Selected, TRUE);
		    nnset(data->def_height, MUIA_Selected, TRUE);
		}

                SetAttrsA(data->width,  width_tags);
                SetAttrsA(data->height, height_tags);
                SetAttrsA(data->depth,  depth_tags);

                SetAttrs(data->autoscroll, no_notify, TRUE, 
                                           MUIA_Disabled, !autoscroll,
                                           MUIA_Selected, autoscroll,
                                           TAG_DONE);
                
                break;
            }
            
            case MUIA_ScreenModeProperties_Width:
                width = tag->ti_Data;
		    
		D(Printf("[smproperties] Set Width = %ld\n", width));
		if (no_notify == TAG_IGNORE)
                    nnset(data->def_width, MUIA_Selected, width == -1);
                if (width == -1)
		    width = data->DefWidth;
		else
		    width = AdjustWidth(width, data);
                SetAttrs(data->width, no_notify, TRUE, MUIA_Numeric_Value, width, TAG_DONE);
                break;
                
            case MUIA_ScreenModeProperties_Height:
                height = tag->ti_Data;
		    
		D(Printf("[smproperties] Set Height = %ld\n", height));
		if (no_notify == TAG_IGNORE)
                    nnset(data->def_height, MUIA_Selected, height == -1);
                if (height == -1)
		    height = data->DefHeight;
		else
		    height = AdjustHeight(height, data);
                SetAttrs(data->height, no_notify, TRUE, MUIA_Numeric_Value, height, TAG_DONE);
                break;
            
            case MUIA_ScreenModeProperties_Depth:
                if (data->VariableDepth)
                {
                    depth = tag->ti_Data;
		    
		    D(Printf("[smproperties] Set Depth = %ld\n", depth));
                    if (depth == -1)
		        depth = data->DefDepth;
                    SetAttrs(data->depth, no_notify, TRUE, MUIA_Numeric_Value, depth, TAG_DONE);
                }
                break;
            
            case MUIA_ScreenModeProperties_Autoscroll:
	    
	        D(Printf("[smproperties] Set Autoscroll = %lu\n", tag->ti_Data));
                if (id != INVALID_ID && !XGET(data->autoscroll, MUIA_Disabled))
                    SetAttrs(data->autoscroll, no_notify, TRUE, MUIA_Selected, tag->ti_Data != 0);
                break;
        }
    }

    DB2(Printf("[smproperties] Calling OM_SET() on superclass\n"));
    ret = DoSuperMethodA(CLASS, self, (Msg)message);
    DB2(Printf("[smproperties] OM_SET() on superclass returned %ld\n", ret));
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
            if (XGET(data->def_width, MUIA_Selected) == TRUE)
                *message->opg_Storage = -1;
            else
                *message->opg_Storage = XGET(data->width, MUIA_Numeric_Value);
            break;
        
        case MUIA_ScreenModeProperties_Height:
            if (XGET(data->def_height, MUIA_Selected) == TRUE)
                *message->opg_Storage = -1;
            else
                *message->opg_Storage = XGET(data->height, MUIA_Numeric_Value);
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
