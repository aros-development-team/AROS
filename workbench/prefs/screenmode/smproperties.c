#define MUIMASTER_YES_INLINE_STDARG
#define DEBUG 0

#include <libraries/mui.h>

#include <proto/exec.h>
#include <proto/muimaster.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/utility.h>

#include <zune/customclasses.h>

#include "smproperties.h"

struct ScreenModeProperties_DATA
{
    Object *width, *height, *depth;
    ULONG DisplayID;
};

#define HLeft(obj...) \
    (IPTR)(HGroup, (IPTR)GroupSpacing(0), Child, (IPTR)(obj), Child, (IPTR)HSpace(0), End)

Object *ScreenModeProperties__OM_NEW(Class *CLASS, Object *self, struct opSet *message)
{
    struct ScreenModeProperties_DATA *data;	 
    Object *width, *height, *depth;     
    ULONG id;
    
    self = (Object *)DoSuperNewTags
    (
        CLASS, self, NULL,
        Child, (IPTR)HGroup,
	    Child, (IPTR)GroupObject,
	        MUIA_Group_Columns, 2,
	        Child, (IPTR)Label("\33lWidth:"),
	        Child, HLeft(width = NumericbuttonObject, End),
	        Child, (IPTR)Label("\33lHeight:"),
	        Child, HLeft(height = NumericbuttonObject, End),
	        Child, (IPTR)Label("\33lDepth:"),
	        Child, HLeft(depth = NumericbuttonObject, End),
		MUIA_Weight, 50,
	    End,  
	End,
	
        TAG_MORE, (IPTR)message->ops_AttrList
    );
    
    if (!self)
        goto err;
    
    data = INST_DATA(CLASS, self);    
    data->width  = width;
    data->height = height;
    data->depth  = depth;
    
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
    );
    
    id = GetTagData(MUIA_ScreenModeProperties_DisplayID, INVALID_ID, message->ops_AttrList); 
    set(self, MUIA_ScreenModeProperties_DisplayID, id);
    
    return self;

err:
    CoerceMethod(CLASS, self, OM_DISPOSE);
    return NULL;
}

IPTR ScreenModeProperties__OM_SET(Class *CLASS, Object *self, struct opSet *message)
{
    struct ScreenModeProperties_DATA *data = INST_DATA(CLASS, self);    
    struct TagItem *tags, *tag;
    struct TagItem noforward_tags[] =
    {
        {MUIA_Group_Forward , FALSE                       },
        {TAG_MORE           , (IPTR)message->ops_AttrList }
    };
    struct opSet noforward_message = *message;
    noforward_message.ops_AttrList = noforward_tags;
    
    ULONG id        = INVALID_ID;
    IPTR  no_notify = TAG_IGNORE;
    
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
		    { no_notify,            TRUE },
		    { MUIA_Numeric_Min,        0 },
		    { MUIA_Numeric_Max,        0 },
		    { MUIA_Numeric_Default,    0 },
		    { TAG_DONE,                0 }
		};
	        struct TagItem height_tags[] =
		{
		    { no_notify,            TRUE },
		    { MUIA_Numeric_Min,        0 },
		    { MUIA_Numeric_Max,        0 },
		    { MUIA_Numeric_Default,    0 },
		    { TAG_DONE,                0 }
		};
	        struct TagItem depth_tags[] =
		{
		    { no_notify,            TRUE },
		    { MUIA_Numeric_Min,        0 },
		    { MUIA_Numeric_Max,        0 },
		    { MUIA_Numeric_Default,    0 },
		    { TAG_DONE,                0 }
		};
	        
                struct DimensionInfo dim;
		
                if (GetDisplayInfoData(NULL, (UBYTE *)&dim, sizeof(dim), DTAG_DIMS, tag->ti_Data))
                {
                    width_tags[1].ti_Data  = dim.MinRasterWidth;
                    height_tags[1].ti_Data = dim.MinRasterHeight;
                    depth_tags[1].ti_Data  = 1;
	    
                    width_tags[2].ti_Data  = dim.MaxRasterWidth;
                    height_tags[2].ti_Data = dim.MaxRasterHeight;
	            depth_tags[2].ti_Data  = dim.MaxDepth;
	    
                    width_tags[3].ti_Data  = dim.Nominal.MaxX - dim.Nominal.MinX + 1;
                    height_tags[3].ti_Data = dim.Nominal.MaxY - dim.Nominal.MinY + 1;
	            depth_tags[3].ti_Data  = dim.MaxDepth;
                    
		    id = tag->ti_Data;
		}
		    
  	        data->DisplayID = id;
 	        nfset(self, MUIA_Disabled, id == INVALID_ID);
		
		SetAttrsA(data->width,  width_tags);
		SetAttrsA(data->height, height_tags);
		SetAttrsA(data->depth,  depth_tags);		
                
		break;
	    }
	    
	    case MUIA_ScreenModeProperties_Width:
	        if (id != INVALID_ID)
		{
		    WORD width = tag->ti_Data;
		    if (width != -1)
		        SetAttrs(data->width, no_notify, TRUE, MUIA_Numeric_Value, width, TAG_DONE);
		    else
		        DoMethod(data->width, MUIM_Numeric_SetDefault);
		}
		break;
		
	    case MUIA_ScreenModeProperties_Height:
	        if (id != INVALID_ID)
		{
		    WORD height = tag->ti_Data;
		    if (height != -1)
		        SetAttrs(data->height, no_notify, TRUE, MUIA_Numeric_Value, height, TAG_DONE);
		    else
		        DoMethod(data->height, MUIM_Numeric_SetDefault);
		}
		break;
	    
	    case MUIA_ScreenModeProperties_Depth:
	        if (id != INVALID_ID)
		{
		    WORD depth = tag->ti_Data;
		    if (depth != -1)
		        SetAttrs(data->depth, no_notify, TRUE, MUIA_Numeric_Value, depth, TAG_DONE);
		    else
		        DoMethod(data->depth, MUIM_Numeric_SetDefault);
		}
		break;
	}
    }		    

    return DoSuperMethodA(CLASS, self, (Msg)&noforward_message);	
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
	    *message->opg_Storage = XGET(data->width, MUIA_Numeric_Value);
            break;
        
	case MUIA_ScreenModeProperties_Height:
	    *message->opg_Storage = XGET(data->height, MUIA_Numeric_Value);
            break;
        
	case MUIA_ScreenModeProperties_Depth:
	    *message->opg_Storage = XGET(data->depth, MUIA_Numeric_Value);
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
