/*
    Copyright © 2002-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#define MUIMASTER_YES_INLINE_STDARG

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
#include <proto/muimaster.h>

#include <string.h>

#include "mui.h"
#include "muimaster_intern.h"
#include "support.h"
#include "support_classes.h"
#include "coloradjust_private.h"

/*  #define MYDEBUG 1 */
#include "debug.h"

extern struct Library *MUIMasterBase;

#define FLAG_FIXED_PEN 	    1
#define FLAG_PEN_ALLOCATED  2
#define FLAG_NO_PEN 	    4

#define ColorWheelBase data->colorwheelbase

static void NotifyGun(Object *obj, struct Coloradjust_DATA *data, LONG gun)
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

static void NotifyAll(Object *obj, struct Coloradjust_DATA *data)
{
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
    CoerceMethod(data->notifyclass, obj, OM_SET, (IPTR)tags, NULL);
}

static void SliderFunc(struct Hook *hook, Object *obj, APTR msg)
{
    struct ColorWheelRGB    	cw;
    struct Coloradjust_DATA *data = *(struct Coloradjust_DATA **)msg;
    IPTR   gun = ((IPTR *)msg)[1];
    
    ULONG red = XGET(data->rslider,MUIA_Numeric_Value);
    ULONG green = XGET(data->gslider,MUIA_Numeric_Value);
    ULONG blue = XGET(data->bslider,MUIA_Numeric_Value);

    cw.cw_Red = (red<<24)|(red<<16)|(red<<8)|red;
    cw.cw_Green = (green<<24)|(green<<16)|(green<<8)|green;
    cw.cw_Blue = (blue<<24)|(blue<<16)|(blue<<8)|blue;

    data->rgb[0] = cw.cw_Red;
    data->rgb[1] = cw.cw_Green;
    data->rgb[2] = cw.cw_Blue;
    
    nnset(data->colfield, MUIA_Colorfield_RGB, (IPTR)data->rgb);
    
    if (data->wheel)
    {
    	struct ColorWheelHSB hsb;
 		
	ConvertRGBToHSB(&cw, &hsb);
    	nnset(data->wheel, WHEEL_HSB, (IPTR)&hsb);
	nnset(data->grad, GRAD_CurVal, 0xFFFF - (hsb.cw_Brightness >> 16));	

	if (data->gradpen != -1)
	{
    	    hsb.cw_Brightness = 0xFFFFFFFF;
    	    ConvertHSBToRGB(&hsb, &cw);

	    SetRGB32(&_screen(obj)->ViewPort, data->gradpen, cw.cw_Red, cw.cw_Green, cw.cw_Blue);
	    if (data->truecolor) MUI_Redraw(data->grad, MADF_DRAWUPDATE);
	}
    }
    
    NotifyGun(obj, data, gun);
}

static void WheelFunc(struct Hook *hook, Object *obj, APTR msg)
{
    struct Coloradjust_DATA *data = *(struct Coloradjust_DATA **)msg;
    struct ColorWheelHSB    	hsb;
    struct ColorWheelRGB    	cw;
    
    hsb.cw_Hue        = XGET(data->wheel, WHEEL_Hue);
    hsb.cw_Saturation = XGET(data->wheel, WHEEL_Saturation);
    hsb.cw_Brightness = 0xFFFF - XGET(data->grad, GRAD_CurVal);
    hsb.cw_Brightness |= (hsb.cw_Brightness << 16) ;

    ConvertHSBToRGB(&hsb, &cw);
    
    data->rgb[0] = cw.cw_Red;
    data->rgb[1] = cw.cw_Green;
    data->rgb[2] = cw.cw_Blue;
    
    nnset(data->rslider, MUIA_Numeric_Value, data->rgb[0] >> 24);
    nnset(data->gslider, MUIA_Numeric_Value, data->rgb[1] >> 24);
    nnset(data->bslider, MUIA_Numeric_Value, data->rgb[2] >> 24);
    
    nnset(data->colfield, MUIA_Colorfield_RGB, (IPTR)data->rgb);

    if (data->gradpen != -1)
    {
    	hsb.cw_Brightness = 0xFFFFFFFF;
    	ConvertHSBToRGB(&hsb, &cw);
	
	SetRGB32(&_screen(obj)->ViewPort, data->gradpen, cw.cw_Red, cw.cw_Green, cw.cw_Blue);
	if (data->truecolor) MUI_Redraw(data->grad, MADF_DRAWUPDATE);
    }
    
    NotifyAll(obj, data);    
}

static void GradFunc(struct Hook *hook, Object *obj, APTR msg)
{
    struct Coloradjust_DATA *data = *(struct Coloradjust_DATA **)msg;
    struct ColorWheelHSB    	hsb;
    struct ColorWheelRGB    	cw;
    
    ULONG bright = XGET(data->grad, GRAD_CurVal);
    
    bright = 0xFFFF - bright;
    bright |= (bright << 16);

    hsb.cw_Hue        = XGET(data->wheel, WHEEL_Hue);
    hsb.cw_Saturation = XGET(data->wheel, WHEEL_Saturation);
    hsb.cw_Brightness = bright;

    ConvertHSBToRGB(&hsb, &cw);
    
    data->rgb[0] = cw.cw_Red;
    data->rgb[1] = cw.cw_Green;
    data->rgb[2] = cw.cw_Blue;
    
    nnset(data->rslider, MUIA_Numeric_Value, data->rgb[0] >> 24);
    nnset(data->gslider, MUIA_Numeric_Value, data->rgb[1] >> 24);
    nnset(data->bslider, MUIA_Numeric_Value, data->rgb[2] >> 24);
    
    nnset(data->colfield, MUIA_Colorfield_RGB, (IPTR)data->rgb);
    
    NotifyAll(obj, data);    
}


IPTR Coloradjust__OM_NEW(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct Coloradjust_DATA   *data;
    struct TagItem  	    	*tag, *tags;
    struct Library  	    	*colorwheelbase, *gradientsliderbase;
    Object  	    	    	*rslider, *gslider, *bslider;
    Object  	    	    	*colfield, *wheel = NULL, *grad = NULL;
    ULONG   	    	    	*rgb;

    colorwheelbase = OpenLibrary("gadgets/colorwheel.gadget", 0);
    gradientsliderbase = OpenLibrary("gadgets/gradientslider.gadget", 0);
    
    obj = (Object *) DoSuperNewTags
    (
        cl, obj, NULL,
        
        MUIA_Group_Columns,     2,
        MUIA_Group_VertSpacing, 1,
        
        Child, (IPTR) Label1("Red:"),
        Child, (IPTR) (rslider = SliderObject, MUIA_Group_Horiz, TRUE, MUIA_Numeric_Min, 0, MUIA_Numeric_Max, 255, End),
        Child, (IPTR) Label1("Green:"),
        Child, (IPTR) (gslider = SliderObject, MUIA_Group_Horiz, TRUE, MUIA_Numeric_Min, 0, MUIA_Numeric_Max, 255, End),
        Child, (IPTR) Label1("Blue:"),
        Child, (IPTR) (bslider = SliderObject, MUIA_Group_Horiz, TRUE, MUIA_Numeric_Min, 0, MUIA_Numeric_Max, 255, End),
        Child, (IPTR) VSpace(1),
        Child, (IPTR) VSpace(1),
        Child, (IPTR) (colfield = ColorfieldObject, TextFrame, MUIA_Weight, 0, End),
        Child, (!colorwheelbase || !gradientsliderbase) ? (IPTR) HVSpace : (IPTR) HGroup,
            /* FIXME: this looks severely broken if the HVSpace path is taken... */
            MUIA_Group_HorizSpacing, 2,
            Child, (IPTR) (wheel = BoopsiObject,
                MUIA_Boopsi_ClassID,   (IPTR) "colorwheel.gadget",
                MUIA_Boopsi_MinWidth,         16,
                MUIA_Boopsi_MinHeight,        16,
                MUIA_Boopsi_Remember,         WHEEL_Saturation,
                MUIA_Boopsi_Remember,         WHEEL_Hue,
                MUIA_Boopsi_TagScreen,        WHEEL_Screen,
                WHEEL_Screen,                 NULL,
                GA_Left,                      0,
                GA_Top,                       0,
                GA_Width,                     0,
                GA_Height,                    0,
                ICA_TARGET,                   ICTARGET_IDCMP,
                MUIA_FillArea,                TRUE,
            End),
            Child, (IPTR) (grad = BoopsiObject,
                MUIA_Boopsi_ClassID,  (IPTR) "gradientslider.gadget",
                MUIA_Boopsi_MinWidth,        16,
                MUIA_Boopsi_MinHeight,       16,
                MUIA_Boopsi_MaxWidth,        16,
                MUIA_Boopsi_Remember,        GRAD_CurVal,
                MUIA_Boopsi_Remember,        GRAD_PenArray,
                MUIA_Boopsi_Remember,        GRAD_KnobPixels,
                GA_Left,                     0,
                GA_Top,                      0,
                GA_Width,                    0,
                GA_Height,                   0,
                GRAD_KnobPixels,             8,
                PGA_Freedom,                 LORIENT_VERT,
                ICA_TARGET,                  ICTARGET_IDCMP,
                MUIA_FillArea,               TRUE,
            End),
        End,
        
        TAG_MORE, (IPTR) msg->ops_AttrList
    );
    
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
 
    nnset(colfield, MUIA_Colorfield_RGB, (IPTR)data->rgb);
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
	/* just to be sure - colorwheel seems to have some problems */
	nnset(wheel, WHEEL_Saturation, hsb.cw_Saturation);
	nnset(wheel, WHEEL_Hue, hsb.cw_Hue);
	nnset(data->grad, GRAD_CurVal, 0xFFFF - (hsb.cw_Brightness >> 16));
	nnset(data->grad, GRAD_PenArray, (IPTR)data->gradpenarray);
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

IPTR Coloradjust__OM_DISPOSE(struct IClass *cl, Object *obj, Msg msg)
{
    struct Coloradjust_DATA  *data;
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

IPTR Coloradjust__OM_SET(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct Coloradjust_DATA   *data;
    struct TagItem  	    	*tag, *tags;
    ULONG   	    	    	*rgb;
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

    if (newcol)
    {
    	nnset(data->rslider, MUIA_Numeric_Value, data->rgb[0] >> 24);
	nnset(data->gslider, MUIA_Numeric_Value, data->rgb[1] >> 24);
	nnset(data->bslider, MUIA_Numeric_Value, data->rgb[2] >> 24);
	D(bug("coloradjust: sliders set to %ld, %ld, %ld\n", data->rgb[0] >> 24,
	      data->rgb[1] >> 24, data->rgb[2] >> 24));

	nnset(data->colfield, MUIA_Colorfield_RGB, (IPTR)data->rgb);

	if (data->wheel)
	{
    	    struct ColorWheelRGB cw;
	    struct ColorWheelHSB hsb;

            cw.cw_Red   = data->rgb[0];
    	    cw.cw_Green = data->rgb[1];
    	    cw.cw_Blue  = data->rgb[2];

    	    ConvertRGBToHSB(&cw, &hsb);
    	    nnset(data->wheel, WHEEL_HSB, (IPTR)&hsb);
	    nnset(data->wheel, WHEEL_Saturation, hsb.cw_Saturation);
	    nnset(data->wheel, WHEEL_Hue, hsb.cw_Hue);
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
    
    return DoSuperMethodA(cl, obj, (Msg)msg);
}

IPTR Coloradjust__OM_GET(struct IClass *cl, Object * obj, struct opGet *msg)
{
    struct Coloradjust_DATA *data  = INST_DATA(cl, obj);
    IPTR    	    	      *store = msg->opg_Storage;

    switch (msg->opg_AttrID)
    {
    	case MUIA_Coloradjust_Red:
	    *store = data->rgb[0];
	    return TRUE;
	    
	case MUIA_Coloradjust_Green:
	    *store = data->rgb[1];
	    return TRUE;
	    
	case MUIA_Coloradjust_Blue:
	    *store = data->rgb[2];
	    return TRUE;
	    
	case MUIA_Coloradjust_RGB:
	    *(IPTR **)store = data->rgb;
	    return TRUE;
    }

    return DoSuperMethodA(cl, obj, (Msg)msg);
}

IPTR Coloradjust__MUIM_Setup(struct IClass *cl, Object *obj, struct MUIP_Setup *msg)
{
    struct Coloradjust_DATA *data = INST_DATA(cl,obj);

    if (!(DoSuperMethodA(cl, obj, (Msg)msg))) return 0;

    if (data->wheel)
    {  
    	struct ColorWheelHSB hsb;
	struct ColorWheelRGB rgb;
	
	rgb.cw_Red   = data->rgb[0];
	rgb.cw_Green = data->rgb[1];
	rgb.cw_Blue  = data->rgb[2];

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

	ConvertRGBToHSB(&rgb, &hsb);
	/* setting this will force wheel to properly set its display */
	nnset(data->wheel, WHEEL_Saturation, hsb.cw_Saturation);
	nnset(data->wheel, WHEEL_Hue, hsb.cw_Hue);

	hsb.cw_Brightness = 0xFFFFFFFF;
	ConvertHSBToRGB(&hsb, &rgb);
	SetRGB32(&_screen(obj)->ViewPort, data->gradpen, rgb.cw_Red, rgb.cw_Green, rgb.cw_Blue);

	data->truecolor = GetBitMapAttr(_screen(obj)->RastPort.BitMap, BMA_DEPTH) >= 15;
    }
    
    return 1;
}

IPTR Coloradjust__MUIM_Cleanup(struct IClass *cl, Object *obj, struct MUIP_Cleanup *msg)
{
    struct Coloradjust_DATA *data = INST_DATA(cl,obj);

    if (data->gradpen != -1)
    {
    	ReleasePen(_screen(obj)->ViewPort.ColorMap, data->gradpen);
	data->gradpen = -1;
    }
    
    return DoSuperMethodA(cl, obj, (Msg)msg);
}

#if ZUNE_BUILTIN_COLORADJUST
BOOPSI_DISPATCHER(IPTR, Coloradjust_Dispatcher, cl, obj, msg)
{
    switch (msg->MethodID)
    {
	case OM_NEW:       return Coloradjust__OM_NEW(cl, obj, (struct opSet *)msg);
    	case OM_DISPOSE:   return Coloradjust__OM_DISPOSE(cl, obj, msg);
	case OM_SET:       return Coloradjust__OM_SET(cl, obj, (struct opSet *)msg);
	case OM_GET:       return Coloradjust__OM_GET(cl, obj, (struct opGet *)msg);
	case MUIM_Setup:   return Coloradjust__MUIM_Setup(cl, obj, (struct MUIP_Setup *)msg);
	case MUIM_Cleanup: return Coloradjust__MUIM_Cleanup(cl, obj, (struct MUIP_Cleanup *)msg);
        default:           return DoSuperMethodA(cl, obj, msg);
    }
}
BOOPSI_DISPATCHER_END

const struct __MUIBuiltinClass _MUI_Coloradjust_desc =
{ 
    MUIC_Coloradjust, 
    MUIC_Group, 
    sizeof(struct Coloradjust_DATA), 
    (void*)Coloradjust_Dispatcher 
};
#endif /* ZUNE_BUILTIN_COLORADJUST */
