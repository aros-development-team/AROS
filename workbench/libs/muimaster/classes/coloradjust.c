/*
    Copyright © 2002, The AROS Development Team. 
    All rights reserved.
    
    $Id$
*/

#include <stdio.h>

#include <graphics/gfx.h>
#include <graphics/view.h>
#include <graphics/gfxmacros.h>
#include <gadgets/colorwheel.h>
#include <gadgets/gradientslider.h>
#include <intuition/icclass.h>
#include <clib/alib_protos.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/utility.h>
#include <proto/intuition.h>
#include <proto/colorwheel.h>

#include <string.h>

#ifdef __AROS__
#include <proto/muimaster.h>
#endif

#include "mui.h"
#include "muimaster_intern.h"
#include "textengine.h"
#include "support.h"

/*  #define MYDEBUG 1 */
#include "debug.h"

extern struct Library *MUIMasterBase;

#define FLAG_FIXED_PEN 	    1
#define FLAG_PEN_ALLOCATED  2
#define FLAG_NO_PEN 	    4

struct MUI_ColoradjustData
{
    struct Library *colorwheelbase;
    struct Library *gradientsliderbase;
    
    struct Hook sliderhook, wheelhook, gradhook;
    Object *rslider, *gslider, *bslider, *colfield, *wheel, *grad;
        
    ULONG rgb[3];
};

#define ColorWheelBase data->colorwheelbase

static IPTR xget(Object *obj, Tag attr)
{
    IPTR storage = 0;
    
    GetAttr(attr, obj, &storage);
    
    return storage;
}

static void NotifyGun(Object *obj, struct MUI_ColoradjustData *data, LONG gun)
{
    static Tag guntotag[3] = 
    {
    	MUIA_Coloradjust_Red,
	MUIA_Coloradjust_Green,
	MUIA_Coloradjust_Blue
    };
    
    struct IClass *cl = OCLASS(obj)->cl_Super->cl_Super;
    
    struct TagItem tags[] =
    {
    	{guntotag[gun]      	, data->rgb[gun]    },
	{MUIA_Coloradjust_RGB   , (IPTR)data->rgb   },
	{TAG_DONE   	    	    	    	    }
    };
    
    CoerceMethod(cl, obj, OM_SET, (IPTR)tags, NULL);
}

static void NotifyAll(Object *obj, struct MUI_ColoradjustData *data)
{
    struct IClass *cl = OCLASS(obj)->cl_Super->cl_Super;
    
    struct TagItem tags[] =
    {
    	{MUIA_Coloradjust_Red  , data->rgb[0]	},
	{MUIA_Coloradjust_Green, data->rgb[1]	},
	{MUIA_Coloradjust_Blue , data->rgb[2]	},
	{MUIA_Coloradjust_RGB  , (IPTR)data->rgb},
	{TAG_DONE   	    	    	    	}
    };
    
    CoerceMethod(cl, obj, OM_SET, (IPTR)tags, NULL);
}

static void SliderFunc(struct Hook *hook, Object *obj, APTR msg)
{
    struct ColorWheelRGB    	cw;
    struct MUI_ColoradjustData *data = *(struct MUI_ColoradjustData **)msg;
    IPTR   gun = ((IPTR *)msg)[1];
    
    ULONG red = xget(data->rslider,MUIA_Numeric_Value);
    ULONG green = xget(data->gslider,MUIA_Numeric_Value);
    ULONG blue = xget(data->bslider,MUIA_Numeric_Value);

    cw.cw_Red = (red<<24)|(red<<16)|(red<<8)|red;
    cw.cw_Green = (green<<24)|(green<<16)|(green<<8)|green;
    cw.cw_Blue = (blue<<24)|(blue<<16)|(blue<<8)|blue;

    data->rgb[0] = cw.cw_Red;
    data->rgb[1] = cw.cw_Green;
    data->rgb[2] = cw.cw_Blue;
    
    nnset(data->colfield, MUIA_Colorfield_RGB, data->rgb);
    
    if (data->wheel)
    {
    	struct ColorWheelHSB hsb;
 		
	ConvertRGBToHSB(&cw, &hsb);
    	nnset(data->wheel, WHEEL_HSB, &hsb);
	nnset(data->grad, GRAD_CurVal, 0xFFFF - (hsb.cw_Brightness >> 16));	
    }
    
    NotifyGun(obj, data, gun);
}

static void WheelFunc(struct Hook *hook, Object *obj, APTR msg)
{
    struct MUI_ColoradjustData *data = *(struct MUI_ColoradjustData **)msg;
    
    data->rgb[0] = xget(data->wheel, WHEEL_Red);
    data->rgb[1] = xget(data->wheel, WHEEL_Green);
    data->rgb[2] = xget(data->wheel, WHEEL_Blue);
    
    nnset(data->rslider, MUIA_Numeric_Value, data->rgb[0] >> 24);
    nnset(data->gslider, MUIA_Numeric_Value, data->rgb[1] >> 24);
    nnset(data->bslider, MUIA_Numeric_Value, data->rgb[2] >> 24);
    
    nnset(data->colfield, MUIA_Colorfield_RGB, data->rgb);

    NotifyAll(obj, data);    
}

static void GradFunc(struct Hook *hook, Object *obj, APTR msg)
{
    struct MUI_ColoradjustData *data = *(struct MUI_ColoradjustData **)msg;
    
    ULONG bright = xget(data->grad, GRAD_CurVal);
    
    bright = 0xFFFF - bright;
    bright |= (bright << 16);
    
    nnset(data->wheel, WHEEL_Brightness, bright);
     
    data->rgb[0] = xget(data->wheel, WHEEL_Red);
    data->rgb[1] = xget(data->wheel, WHEEL_Green);
    data->rgb[2] = xget(data->wheel, WHEEL_Blue);
    
    nnset(data->rslider, MUIA_Numeric_Value, data->rgb[0] >> 24);
    nnset(data->gslider, MUIA_Numeric_Value, data->rgb[1] >> 24);
    nnset(data->bslider, MUIA_Numeric_Value, data->rgb[2] >> 24);
    
    nnset(data->colfield, MUIA_Colorfield_RGB, data->rgb);
    
    NotifyAll(obj, data);    
}

/**************************************************************************
 OM_NEW
**************************************************************************/
static IPTR Coloradjust_New(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct MUI_ColoradjustData   *data;
    struct TagItem  	    	*tag, *tags;
    struct Library  	    	*colorwheelbase, *gradientsliderbase;
    Object  	    	    	*rslider, *gslider, *bslider;
    Object  	    	    	*colfield, *wheel = NULL, *grad = NULL;
    ULONG   	    	    	*rgb;

    colorwheelbase = OpenLibrary("gadgets/colorwheel.gadget", 0);
    gradientsliderbase = OpenLibrary("gadgets/gradientslider.gadget", 0);
    
    obj = (Object *)DoSuperNew(cl, obj,
	MUIA_Group_Columns, 2,
	Child, Label2("Red:"),
	Child, rslider = SliderObject, MUIA_Group_Horiz, TRUE, MUIA_Numeric_Min, 0, MUIA_Numeric_Max, 255, End,
	Child, Label2("Green:"),
	Child, gslider = SliderObject, MUIA_Group_Horiz, TRUE, MUIA_Numeric_Min, 0, MUIA_Numeric_Max, 255, End,
	Child, Label2("Blue:"),
	Child, bslider = SliderObject, MUIA_Group_Horiz, TRUE, MUIA_Numeric_Min, 0, MUIA_Numeric_Max, 255, End,
	Child, colfield = ColorfieldObject, TextFrame, End,
	Child, (!colorwheelbase || !gradientsliderbase) ? HVSpace : HGroup,
	    Child, wheel = BoopsiObject,
	    	MUIA_Boopsi_ClassID, "colorwheel.gadget",
		MUIA_Boopsi_MinWidth, 60,
		MUIA_Boopsi_MinHeight, 60,
		MUIA_Boopsi_Remember , WHEEL_Saturation,
		MUIA_Boopsi_Remember , WHEEL_Hue,
		MUIA_Boopsi_Remember , WHEEL_Brightness,
		MUIA_Boopsi_TagScreen, WHEEL_Screen,
		WHEEL_Screen	     , NULL,
		GA_Left     	     , 0,
		GA_Top	    	     , 0,
		GA_Width    	     , 0,
		GA_Height   	     , 0,
		ICA_TARGET  	     , ICTARGET_IDCMP,
		MUIA_FillArea	     , TRUE,
		End,
	    Child, grad = BoopsiObject,
	    	MUIA_Boopsi_ClassID, "gradientslider.gadget",
		MUIA_Boopsi_MinWidth, 16,
		MUIA_Boopsi_MinHeight, 16,
		MUIA_Boopsi_MaxWidth, 16,
		MUIA_Boopsi_Remember, GRAD_CurVal,
		GA_Left     	    , 0,
    	    	GA_Top	    	    , 0,
    	    	GA_Width    	    , 0,
		GA_Height   	    , 0,
		GRAD_KnobPixels     , 8,
		PGA_Freedom 	    , LORIENT_VERT,
		ICA_TARGET  	    , ICTARGET_IDCMP,
		MUIA_FillArea	    , TRUE,
		End,
	    End,
	
	TAG_MORE, msg->ops_AttrList);
    
    if (!obj)
    {
    	CloseLibrary(gradientsliderbase);
    	CloseLibrary(colorwheelbase);
    	return FALSE;
    }
            
    data = INST_DATA(cl, obj);

    data->colorwheelbase = colorwheelbase;
    data->gradientsliderbase = gradientsliderbase;
    
    data->sliderhook.h_Entry 	= HookEntry;
    data->sliderhook.h_SubEntry = (HOOKFUNC)SliderFunc;
    
    data->wheelhook.h_Entry = HookEntry;
    data->wheelhook.h_SubEntry = (HOOKFUNC)WheelFunc;
    
    data->gradhook.h_Entry = HookEntry;
    data->gradhook.h_SubEntry = (HOOKFUNC)GradFunc;
    
    data->rslider   = rslider;
    data->gslider   = gslider;
    data->bslider   = bslider;
    data->colfield  = colfield;
    data->wheel     = wheel;
    data->grad	    = grad;
    
    data->rgb[0] = data->rgb[1] = data->rgb[2] = 0xFFFFFFFF;
        
    /* parse initial taglist */
    for (tags = msg->ops_AttrList; (tag = NextTagItem(&tags)); )
    {
	switch (tag->ti_Tag)
	{
	    case MUIA_Coloradjust_Red:
	    	data->rgb[0] = (ULONG)tag->ti_Data;
		break;
		
	    case MUIA_Coloradjust_Green:
	    	data->rgb[1] = (ULONG)tag->ti_Data;
		break;
		
	    case MUIA_Coloradjust_Blue:
	    	data->rgb[2] = (ULONG)tag->ti_Data;
		break;
		
	    case MUIA_Coloradjust_RGB:
	    	rgb = (ULONG *)tag->ti_Data;
		data->rgb[0] = *rgb++;
		data->rgb[1] = *rgb++;
		data->rgb[2] = *rgb++;
		break;
		
   	}
    }
 
    nnset(colfield, MUIA_Colorfield_RGB, data->rgb);
    nnset(rslider, MUIA_Numeric_Value, data->rgb[0] >> 24);
    nnset(gslider, MUIA_Numeric_Value, data->rgb[1] >> 24);
    nnset(bslider, MUIA_Numeric_Value, data->rgb[2] >> 24);

    if (wheel)
    {
    	struct ColorWheelRGB cw;
	struct ColorWheelHSB hsb;
	
        cw.cw_Red   = data->rgb[0];
    	cw.cw_Green = data->rgb[1];
    	cw.cw_Blue  = data->rgb[2];

	ConvertRGBToHSB(&cw, &hsb);

    	nnset(wheel, WHEEL_HSB, (IPTR)&hsb);
	nnset(data->grad, GRAD_CurVal, 0xFFFF - (hsb.cw_Brightness >> 16));
    }
        
    DoMethod(rslider, MUIM_Notify, MUIA_Numeric_Value, MUIV_EveryTime, (IPTR)obj, 4, MUIM_CallHook, (IPTR)&data->sliderhook, (IPTR)data, 0);
    DoMethod(gslider, MUIM_Notify, MUIA_Numeric_Value, MUIV_EveryTime, (IPTR)obj, 4, MUIM_CallHook, (IPTR)&data->sliderhook, (IPTR)data, 1);
    DoMethod(bslider, MUIM_Notify, MUIA_Numeric_Value, MUIV_EveryTime, (IPTR)obj, 4, MUIM_CallHook, (IPTR)&data->sliderhook, (IPTR)data, 2);

    if (wheel)
    {
    	DoMethod(wheel, MUIM_Notify, WHEEL_Hue, MUIV_EveryTime, (IPTR)obj, 3, MUIM_CallHook, (IPTR)&data->wheelhook, (IPTR)data);
    	DoMethod(wheel, MUIM_Notify, WHEEL_Saturation, MUIV_EveryTime, (IPTR)obj, 3, MUIM_CallHook, (IPTR)&data->wheelhook, (IPTR)data);
    	DoMethod(grad, MUIM_Notify, GRAD_CurVal, MUIV_EveryTime, (IPTR)obj, 3, MUIM_CallHook, (IPTR)&data->gradhook, (IPTR)data);
		
    }    
    return (IPTR)obj;
}

/**************************************************************************
 OM_DISPOSE
**************************************************************************/

static IPTR Coloradjust_Dispose(struct IClass *cl, Object *obj, Msg msg)
{
    struct MUI_ColoradjustData   *data;
    
    data = INST_DATA(cl, obj);
    
    if (data->colorwheelbase) CloseLibrary(data->colorwheelbase);
    if (data->gradientsliderbase) CloseLibrary(data->gradientsliderbase);
    
    return DoSuperMethodA(cl, obj, msg);
}

/**************************************************************************
 OM_SET
**************************************************************************/
static IPTR Coloradjust_Set(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct MUI_ColoradjustData   *data;
    struct TagItem  	    	*tag, *tags;
    ULONG   	    	    	*rgb;
    IPTR    	    	    	 retval;
    BOOL    	    	    	 newcol = FALSE;
    
    data = INST_DATA(cl, obj);

    for (tags = msg->ops_AttrList; (tag = NextTagItem(&tags)); )
    {
	switch (tag->ti_Tag)
	{
	    case MUIA_Coloradjust_Red:
	    	data->rgb[0] = (ULONG)tag->ti_Data;
		newcol = TRUE;
		break;
		
	    case MUIA_Coloradjust_Green:
	    	data->rgb[1] = (ULONG)tag->ti_Data;
		newcol = TRUE;
		break;
		
	    case MUIA_Coloradjust_Blue:
	    	data->rgb[2] = (ULONG)tag->ti_Data;
		newcol = TRUE;
		break;
		
	    case MUIA_Coloradjust_RGB:
	    	rgb = (ULONG *)tag->ti_Data;
		data->rgb[0] = *rgb++;
		data->rgb[1] = *rgb++;
		data->rgb[2] = *rgb++;
    	    	newcol = TRUE;		
		break;
		
    	}
    }

    retval = DoSuperMethodA(cl, obj, (Msg)msg);
    
    if (newcol)
    {
    	nnset(data->rslider, MUIA_Numeric_Value, data->rgb[0] >> 24);
	nnset(data->gslider, MUIA_Numeric_Value, data->rgb[1] >> 24);
	nnset(data->bslider, MUIA_Numeric_Value, data->rgb[2] >> 24);
	
	if (data->wheel)
	{
    	    struct ColorWheelRGB cw;
	    struct ColorWheelHSB hsb;

            cw.cw_Red   = data->rgb[0];
    	    cw.cw_Green = data->rgb[1];
    	    cw.cw_Blue  = data->rgb[2];

    	    ConvertRGBToHSB(&cw, &hsb);
    	    nnset(data->wheel, WHEEL_HSB, (IPTR)&hsb);
	    nnset(data->grad, GRAD_CurVal, 0xFFFF - (hsb.cw_Brightness >> 16));
    	}
	
	if (_flags(obj) & MADF_SETUP)
	{
	    MUI_Redraw(data->colfield, MADF_DRAWUPDATE);
	}
    }
    
    return retval;
}

/**************************************************************************
 OM_GET
**************************************************************************/
static ULONG  Coloradjust_Get(struct IClass *cl, Object * obj, struct opGet *msg)
{
    struct MUI_ColoradjustData *data  = INST_DATA(cl, obj);
    IPTR    	    	      *store = msg->opg_Storage;

    switch (msg->opg_AttrID)
    {
    	case MUIA_Coloradjust_Red:
	    *store = data->rgb[0];
	    break;
	    
	case MUIA_Coloradjust_Green:
	    *store = data->rgb[1];
	    break;
	    
	case MUIA_Coloradjust_Blue:
	    *store = data->rgb[2];
	    break;
	    
	case MUIA_Coloradjust_RGB:
	    *(ULONG **)store = data->rgb;
	    break;
	    
    	default:
	    return DoSuperMethodA(cl, obj, (Msg)msg);
    }

    return TRUE;
}


BOOPSI_DISPATCHER(IPTR, Coloradjust_Dispatcher, cl, obj, msg)
{
    switch (msg->MethodID)
    {
	case OM_NEW: return Coloradjust_New(cl, obj, (struct opSet *)msg);
    	case OM_DISPOSE: return Coloradjust_Dispose(cl, obj, msg);
	case OM_SET: return Coloradjust_Set(cl, obj, (struct opSet *)msg);
	case OM_GET: return Coloradjust_Get(cl, obj, (struct opGet *)msg);
    }
    
    return DoSuperMethodA(cl, obj, msg);
}

/*
 * Class descriptor.
 */
const struct __MUIBuiltinClass _MUI_Coloradjust_desc = { 
    MUIC_Coloradjust, 
    MUIC_Group, 
    sizeof(struct MUI_ColoradjustData), 
    (void*)Coloradjust_Dispatcher 
};

