/*
    Copyright © 2002, The AROS Development Team. 
    All rights reserved.
    
    $Id$
*/

#include <stdio.h>

#include <graphics/gfx.h>
#include <graphics/view.h>
#include <graphics/gfxmacros.h>
#include <intuition/gadgetclass.h>
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

#include "mui.h"
#include "muimaster_intern.h"
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
    struct IClass  *notifyclass;
    struct Hook     sliderhook, wheelhook, gradhook;
    Object  	   *rslider, *gslider, *bslider, *colfield, *wheel, *grad;       
    ULONG   	    rgb[3];
    UWORD    	    gradpenarray[3];
    LONG    	    gradpen;
    BOOL    	    truecolor;
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
    
    struct TagItem tags[] =
    {
    	{0                   , 0},
	{MUIA_Coloradjust_RGB, 0},
	{TAG_DONE   	    	    	    	    }
    };

    tags[0].ti_Tag = guntotag[gun];
    tags[0].ti_Data = data->rgb[gun];
    tags[1].ti_Data = (IPTR)data->rgb;
    
    CoerceMethod(data->notifyclass, obj, OM_SET, (IPTR)tags, NULL);
}

static void NotifyAll(Object *obj, struct MUI_ColoradjustData *data)
{
    struct IClass *cl = OCLASS(obj)->cl_Super->cl_Super;
    
    struct TagItem tags[] =
    {
    	{MUIA_Coloradjust_Red  , 0 },
	{MUIA_Coloradjust_Green, 0 },
	{MUIA_Coloradjust_Blue , 0 },
	{MUIA_Coloradjust_RGB  , 0 },
	{TAG_DONE   	    	   }
    };
    tags[0].ti_Data = data->rgb[0];
    tags[1].ti_Data = data->rgb[1];
    tags[2].ti_Data = data->rgb[2];
    tags[3].ti_Data = (IPTR)data->rgb;
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

	if (data->gradpen != -1)
	{
    	    hsb.cw_Brightness = 0xFFFFFFFF;
    	    ConvertHSBToRGB(&hsb, &cw);

	    SetRGB32(&_screen(obj)->ViewPort, data->gradpen, cw.cw_Red, cw.cw_Green, cw.cw_Blue);
	    if (data->truecolor) MUI_Redraw(data->grad, MADF_DRAWOBJECT);
	}
    }
    
    NotifyGun(obj, data, gun);
}

static void WheelFunc(struct Hook *hook, Object *obj, APTR msg)
{
    struct MUI_ColoradjustData *data = *(struct MUI_ColoradjustData **)msg;
    struct ColorWheelHSB    	hsb;
    struct ColorWheelRGB    	cw;
    
    hsb.cw_Hue        = xget(data->wheel, WHEEL_Hue);
    hsb.cw_Saturation = xget(data->wheel, WHEEL_Saturation);
    hsb.cw_Brightness = 0xFFFF - xget(data->grad, GRAD_CurVal);
    hsb.cw_Brightness |= (hsb.cw_Brightness << 16) ;

    ConvertHSBToRGB(&hsb, &cw);
    
    data->rgb[0] = cw.cw_Red;
    data->rgb[1] = cw.cw_Green;
    data->rgb[2] = cw.cw_Blue;
    
    nnset(data->rslider, MUIA_Numeric_Value, data->rgb[0] >> 24);
    nnset(data->gslider, MUIA_Numeric_Value, data->rgb[1] >> 24);
    nnset(data->bslider, MUIA_Numeric_Value, data->rgb[2] >> 24);
    
    nnset(data->colfield, MUIA_Colorfield_RGB, data->rgb);

    if (data->gradpen != -1)
    {
    	hsb.cw_Brightness = 0xFFFFFFFF;
    	ConvertHSBToRGB(&hsb, &cw);
	
	SetRGB32(&_screen(obj)->ViewPort, data->gradpen, cw.cw_Red, cw.cw_Green, cw.cw_Blue);
	if (data->truecolor) MUI_Redraw(data->grad, MADF_DRAWOBJECT);
    }
    
    NotifyAll(obj, data);    
}

static void GradFunc(struct Hook *hook, Object *obj, APTR msg)
{
    struct MUI_ColoradjustData *data = *(struct MUI_ColoradjustData **)msg;
    struct ColorWheelHSB    	hsb;
    struct ColorWheelRGB    	cw;
    
    ULONG bright = xget(data->grad, GRAD_CurVal);
    
    bright = 0xFFFF - bright;
    bright |= (bright << 16);

    hsb.cw_Hue        = xget(data->wheel, WHEEL_Hue);
    hsb.cw_Saturation = xget(data->wheel, WHEEL_Saturation);
    hsb.cw_Brightness = bright;

    ConvertHSBToRGB(&hsb, &cw);
    
    data->rgb[0] = cw.cw_Red;
    data->rgb[1] = cw.cw_Green;
    data->rgb[2] = cw.cw_Blue;
    
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
		MUIA_Boopsi_Remember, GRAD_PenArray,
		MUIA_Boopsi_Remember, GRAD_KnobPixels,
		GA_Left     	    , 0,
    	    	GA_Top	    	    , 0,
    	    	GA_Width    	    , 0,
		GA_Height   	    , 0,
		GRAD_KnobPixels     , 8,
		PGA_Freedom 	    , LORIENT_VERT,
		ICA_TARGET  	    , ICTARGET_IDCMP,
		//MUIA_FillArea	    , TRUE,
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
    data->notifyclass = cl->cl_Super->cl_Super;
        
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
	nnset(data->grad, GRAD_PenArray, data->gradpenarray);
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
    struct MUI_ColoradjustData  *data;
    struct Library  	    	*colorwheelbase;
    struct Library  	    	*gradientsliderbase;
    IPTR    	    	    	 retval;
    
    data = INST_DATA(cl, obj);
    
    colorwheelbase = data->colorwheelbase;
    gradientsliderbase = data->gradientsliderbase;
    
    retval = DoSuperMethodA(cl, obj, msg);
    
    if (colorwheelbase) CloseLibrary(colorwheelbase);
    if (gradientsliderbase) CloseLibrary(gradientsliderbase);
    
    return retval;
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
	D(bug("coloradjust: sliders set to %ld, %ld, %ld\n", data->rgb[0] >> 24,
	      data->rgb[1] >> 24, data->rgb[2] >> 24));

	nnset(data->colfield, MUIA_Colorfield_RGB, data->rgb);

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
	    
	    if ((_flags(obj) & MADF_SETUP) && (data->gradpen != -1))
	    {
   	    	hsb.cw_Brightness = 0xFFFFFFFF;
    	    	ConvertHSBToRGB(&hsb, &cw);
	
	    	SetRGB32(&_screen(obj)->ViewPort, data->gradpen, cw.cw_Red, cw.cw_Green, cw.cw_Blue);
	    	if (data->truecolor)
		{
		    MUI_Redraw(data->grad, MADF_DRAWOBJECT);
		}
	    }
	    
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

/**************************************************************************
 MUIM_Setup
**************************************************************************/
static IPTR Coloradjust_Setup(struct IClass *cl, Object *obj, struct MUIP_Setup *msg)
{
    struct MUI_ColoradjustData *data = INST_DATA(cl,obj);

    if (!(DoSuperMethodA(cl, obj, (Msg)msg))) return 0;

    if (data->wheel)
    {  
    	struct ColorWheelHSB hsb;
	struct ColorWheelRGB rgb;
	
	hsb.cw_Hue        = xget(data->wheel, WHEEL_Hue);
	hsb.cw_Saturation = xget(data->wheel, WHEEL_Saturation);
	hsb.cw_Brightness = 0xFFFFFFFF;

    	ConvertHSBToRGB(&hsb, &rgb);
	
	data->gradpenarray[0] = _pens(obj)[MPEN_SHINE];
	data->gradpenarray[1] = _pens(obj)[MPEN_SHADOW];
	data->gradpenarray[2] = (UWORD)~0;

	data->gradpen = ObtainPen(_screen(obj)->ViewPort.ColorMap,
	    	    	    (ULONG)-1,
			    rgb.cw_Red,
			    rgb.cw_Green,
			    rgb.cw_Blue,
			    PENF_EXCLUSIVE);

	if (data->gradpen != -1)
	{
    	    data->gradpenarray[0] = data->gradpen;
	}
	
	data->truecolor = GetBitMapAttr(_screen(obj)->RastPort.BitMap, BMA_DEPTH) >= 15;
	
    }
    
    return 1;
}

/**************************************************************************
 MUIM_Cleanup
**************************************************************************/
static IPTR Coloradjust_Cleanup(struct IClass *cl, Object *obj, struct MUIP_Cleanup *msg)
{
    struct MUI_ColoradjustData *data = INST_DATA(cl,obj);

    if (data->gradpen != -1)
    {
    	ReleasePen(_screen(obj)->ViewPort.ColorMap, data->gradpen);
	data->gradpen = -1;
    }
    
    return DoSuperMethodA(cl, obj, (Msg)msg);
}

/*************************************************************************
 MUIM_Show
**************************************************************************/
static IPTR Coloradjust_Show(struct IClass *cl, Object *obj, struct MUIP_Show *msg)
{
    struct MUI_ColoradjustData *data = INST_DATA(cl,obj);

    DoSuperMethodA(cl, obj, (Msg)msg);

    if ((data->wheel) && (data->gradpen != -1))
    {
    	struct ColorWheelHSB hsb;
	struct ColorWheelRGB cw;
	
	cw.cw_Red   = data->rgb[0];
	cw.cw_Green = data->rgb[1];
	cw.cw_Blue  = data->rgb[2];

	ConvertRGBToHSB(&cw, &hsb);
	nnset(data->wheel, WHEEL_HSB, (IPTR)&hsb);
	nnset(data->grad, GRAD_CurVal, 0xFFFF - (hsb.cw_Brightness >> 16));   

	D(bug("Coloradjust_Show: SetRGB32 %lx, %lx, %lx\n", cw.cw_Red, cw.cw_Green, cw.cw_Blue));
	SetRGB32(&_screen(obj)->ViewPort, data->gradpen, cw.cw_Red, cw.cw_Green, cw.cw_Blue);
    }
    return 0;
}

BOOPSI_DISPATCHER(IPTR, Coloradjust_Dispatcher, cl, obj, msg)
{
    switch (msg->MethodID)
    {
	case OM_NEW: return Coloradjust_New(cl, obj, (struct opSet *)msg);
    	case OM_DISPOSE: return Coloradjust_Dispose(cl, obj, msg);
	case OM_SET: return Coloradjust_Set(cl, obj, (struct opSet *)msg);
	case OM_GET: return Coloradjust_Get(cl, obj, (struct opGet *)msg);
	case MUIM_Setup: return Coloradjust_Setup(cl, obj, (struct MUIP_Setup *)msg);
	case MUIM_Cleanup: return Coloradjust_Cleanup(cl, obj, (struct MUIP_Cleanup *)msg);
	case MUIM_Show: return Coloradjust_Show(cl, obj, (struct MUIP_Show *)msg);
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

