/*
    Copyright (C) 2002-2026, The AROS Development Team. All rights reserved.
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
#include "locale.h"

/*  #define MYDEBUG 1 */
#include "debug.h"

/****** Coloradjust.mui/MUIC_Coloradjust ************
*
*   NAME
*       MUIC_Coloradjust
*
*   SUPERCLASS
*       Group.mui
*
*   DESCRIPTION
*       Edit RGB using three 0..255 sliders, a Colorfield preview and BOOPSI
*       color-wheel/gradient controls. Stored components default to 0xffffffff
*       each. Programmatic RGB updates preserve all 32 bits, while the sliders
*       display only the high byte. Moving any RGB slider reads all three
*       sliders and replicates each byte into all four bytes, so all components
*       are quantized to eight-bit precision by that interaction.
*       The constructor opens gadgets/colorwheel.gadget and
*       gadgets/gradientslider.gadget. It returns NULL on superclass
*       construction failure. The interface-acquisition/drop macros are no-ops
*       on AROS and do not test library availability. Dispose disposes Group
*       children and closes opened libraries. Normal Cleanup must precede
*       disposal for the gradient pen.
*
*   METHODS
*       Programmatic Set synchronizes sliders, preview and wheel/gradient
*       without child notifications, then forwards the original tags to Group.
*       It does not synthesize all complementary RGB/component notifications.
*       Slider callbacks emit the selected component and RGB; wheel and gradient
*       callbacks emit all components and RGB. The latter paths update state
*       synchronously through hooks.
*       Setup calls Group, prepares a gradient pen when a wheel exists and
*       returns one even if exclusive pen allocation fails; the gradient then
*       retains shine/shadow fallback entries. The gradient pen represents the
*       selected hue/saturation at full brightness. Cleanup releases any gradpen
*       not equal to -1 and delegates to Group.
*
*   NOTES
*       Missing gadget libraries are not a proven safe fallback. Constructor
*       expressions still create Boopsi wrappers under TAG_IGNORE and store
*       their pointers; ignored children are not owned through that Group child
*       tag. Conversion and hooks test wrapper pointers rather than library
*       availability. WheelFunc leaves brightness uninitialized without a
*       gradient; GradFunc assumes a wheel. gradpen is not initialized to -1
*       during construction, so Cleanup without the wheel Setup branch can
*       attempt to release pen zero. Use the complete gadget configuration and
*       normal lifecycle; degraded configurations have no safe operation
*       guarantee in this implementation.
*       RGB input pointers must address three ULONG values. Set uses tag order
*       and does not suppress equal values. Preview updates can recolor screen
*       pens through Colorfield. ModeID is declared but not implemented here.
*
*/
/****** Coloradjust.mui/MUIA_Coloradjust_Red ************
*
*   NAME
*       MUIA_Coloradjust_Red
*
*   FUNCTION
*       Set or retrieve the stored red component.
*
*   INPUTS
*       Initialize, set and get, ULONG in 0..0xffffffff. Default 0xffffffff.
*
*   NOTES
*       Programmatic changes retain all bits and synchronize controls; the
*       slider displays the high byte. RGB and component tags overwrite common
*       storage in tag order. Original setter tags drive notifications, without
*       automatic complementary tags. User movement of any RGB slider quantizes
*       all components as described by the class; wheel/gradient interactions
*       can update all components.
*
*/
/****** Coloradjust.mui/MUIA_Coloradjust_Green ************
*
*   NAME
*       MUIA_Coloradjust_Green
*
*   FUNCTION
*       Set or retrieve the stored green component.
*
*   INPUTS
*       Initialize, set and get, ULONG in 0..0xffffffff. Default 0xffffffff.
*
*   NOTES
*       Programmatic changes retain all bits and synchronize controls; the
*       slider displays the high byte. RGB and component tags overwrite common
*       storage in tag order. Original setter tags drive notifications, without
*       automatic complementary tags. User movement of any RGB slider quantizes
*       all components as described by the class; wheel/gradient interactions
*       can update all components.
*
*/
/****** Coloradjust.mui/MUIA_Coloradjust_Blue ************
*
*   NAME
*       MUIA_Coloradjust_Blue
*
*   FUNCTION
*       Set or retrieve the stored blue component.
*
*   INPUTS
*       Initialize, set and get, ULONG in 0..0xffffffff. Default 0xffffffff.
*
*   NOTES
*       Programmatic changes retain all bits and synchronize controls; the
*       slider displays the high byte. RGB and component tags overwrite common
*       storage in tag order. Original setter tags drive notifications, without
*       automatic complementary tags. User movement of any RGB slider quantizes
*       all components as described by the class; wheel/gradient interactions
*       can update all components.
*
*/
/****** Coloradjust.mui/MUIA_Coloradjust_RGB ************
*
*   NAME
*       MUIA_Coloradjust_RGB
*
*   FUNCTION
*       Set or retrieve the stored RGB triplet.
*
*   INPUTS
*       Initialize and set with a non-NULL pointer to three readable ULONG
*       values ordered red, green, blue. Get returns a borrowed pointer to the
*       internal array. Default all 0xffffffff.
*
*   NOTES
*       The values are copied synchronously, not the pointer. Getter storage
*       lasts until disposal but its contents change during setting or user
*       interaction; passing that array to Set is supported. Use attributes
*       instead of direct writes to synchronize controls. A programmatic RGB Set
*       forwards its original tag without synthesizing component notifications.
*       User interactions have the separate notification rules documented by the
*       class.
*
*/
/****** Coloradjust.mui/MUIA_Coloradjust_ModeID ************
*
*   NAME
*       MUIA_Coloradjust_ModeID
*
*   FUNCTION
*       Declared display-mode attribute without local implementation.
*
*   INPUTS
*       The header declares initialize, set and get with ULONG. This
*       implementation has no local handling, stored value or effective default.
*
*   NOTES
*       Tags are passed to the superclass. The class does not select a screen,
*       depth or mode from this attribute; do not rely on it to configure the
*       editor or on a corresponding getter value.
*
*/

extern struct Library *MUIMasterBase;

#define FLAG_FIXED_PEN      1
#define FLAG_PEN_ALLOCATED  2
#define FLAG_NO_PEN         4

#define ColorWheelBase data->colorwheelbase
#define IColorWheel data->icolorwheel

/* private msg structure's passed to the hook functions */
struct MUIP_CANotifyMsg
{
    STACKED struct Coloradjust_DATA *caData;
};

struct MUIP_CASliderNotifyMsg
{
    STACKED struct Coloradjust_DATA *caData;
    STACKED ULONG caGun;
};

/*
 * Issue a synchronous coerced OM_SET containing one component and the RGB
 * pointer, to deliver notifications after a slider edit. Requires gun in
 * 0..2 and live instance/object. Borrowed internal array remains owned by
 * the object; no result or separate failure reporting.
 */
static void NotifyGun(Object *obj, struct Coloradjust_DATA *data, ULONG gun)
{
    static Tag guntotag[3] = {
        MUIA_Coloradjust_Red,
        MUIA_Coloradjust_Green,
        MUIA_Coloradjust_Blue
    };

    struct TagItem tags[] = {
        {0, 0},
        {MUIA_Coloradjust_RGB, 0},
        {TAG_DONE}
    };

    tags[0].ti_Tag = guntotag[gun];
    tags[0].ti_Data = data->rgb[gun];
    tags[1].ti_Data = (IPTR) data->rgb;

    CoerceMethod(OCLASS(obj), obj, OM_SET, (IPTR) tags, TAG_DONE);
}

/*
 * Issue a synchronous coerced OM_SET for all three components and RGB after
 * wheel/gradient changes. Requires live instance/object and stable internal
 * array during callbacks. No ownership transfer or returned status.
 */
static void NotifyAll(Object *obj, struct Coloradjust_DATA *data)
{
    struct TagItem tags[] = {
        {MUIA_Coloradjust_Red, 0},
        {MUIA_Coloradjust_Green, 0},
        {MUIA_Coloradjust_Blue, 0},
        {MUIA_Coloradjust_RGB, 0},
        {TAG_DONE}
    };
    tags[0].ti_Data = data->rgb[0];
    tags[1].ti_Data = data->rgb[1];
    tags[2].ti_Data = data->rgb[2];
    tags[3].ti_Data = (IPTR) data->rgb;

    CoerceMethod(OCLASS(obj), obj, OM_SET, (IPTR) tags, TAG_DONE);
}

/*
 * Read all three byte-valued sliders, replicate each byte to ULONG, update
 * preview/wheel/gradient without child notifications, then notify selected
 * gun and RGB. Requires gun 0..2, live controls, conversion library and
 * screen when gradient pen is used. No ownership transfer or return status.
 */
static void SliderFunc(struct Hook *hook, Object *obj, struct MUIP_CASliderNotifyMsg *msg)
{
    struct Coloradjust_DATA *data = msg->caData;
    ULONG gun = msg->caGun;
    struct ColorWheelRGB cw;

    ULONG red = XGET(data->rslider, MUIA_Numeric_Value);
    ULONG green = XGET(data->gslider, MUIA_Numeric_Value);
    ULONG blue = XGET(data->bslider, MUIA_Numeric_Value);

    cw.cw_Red = (red << 24) | (red << 16) | (red << 8) | red;
    cw.cw_Green = (green << 24) | (green << 16) | (green << 8) | green;
    cw.cw_Blue = (blue << 24) | (blue << 16) | (blue << 8) | blue;

    data->rgb[0] = cw.cw_Red;
    data->rgb[1] = cw.cw_Green;
    data->rgb[2] = cw.cw_Blue;

    nnset(data->colfield, MUIA_Colorfield_RGB, (IPTR) data->rgb);

    if (data->wheel)
    {
        struct ColorWheelHSB hsb;

        ConvertRGBToHSB(&cw, &hsb);
        nnset(data->wheel, WHEEL_HSB, (IPTR) & hsb);
        if (data->grad)
            nnset(data->grad, GRAD_CurVal, 0xFFFF - (hsb.cw_Brightness >> 16));

        if (data->gradpen != -1)
        {
            hsb.cw_Brightness = 0xFFFFFFFF;
            ConvertHSBToRGB(&hsb, &cw);

            SetRGB32(&_screen(obj)->ViewPort, data->gradpen, cw.cw_Red,
                cw.cw_Green, cw.cw_Blue);
            if ((data->truecolor) && (data->grad))
                MUI_Redraw(data->grad, MADF_DRAWUPDATE);
        }
    }

    NotifyGun(obj, data, gun);
}

/*
 * Convert wheel hue/saturation plus inverse gradient brightness to RGB,
 * synchronize sliders/preview, update full-brightness gradient pen and
 * notify all. Requires live controls, conversion library and screen.
 * Without grad the brightness local is uninitialized; that path is not
 * safe. No returned status.
 */
static void WheelFunc(struct Hook *hook, Object *obj, struct MUIP_CANotifyMsg *msg)
{
    struct Coloradjust_DATA *data = msg->caData;
    struct ColorWheelHSB hsb;
    struct ColorWheelRGB cw;

    hsb.cw_Hue = XGET(data->wheel, WHEEL_Hue);
    hsb.cw_Saturation = XGET(data->wheel, WHEEL_Saturation);
    if (data->grad)
    {
        hsb.cw_Brightness = 0xFFFF - XGET(data->grad, GRAD_CurVal);
        hsb.cw_Brightness |= (hsb.cw_Brightness << 16);
    }
    ConvertHSBToRGB(&hsb, &cw);

    data->rgb[0] = cw.cw_Red;
    data->rgb[1] = cw.cw_Green;
    data->rgb[2] = cw.cw_Blue;

    nnset(data->rslider, MUIA_Numeric_Value, data->rgb[0] >> 24);
    nnset(data->gslider, MUIA_Numeric_Value, data->rgb[1] >> 24);
    nnset(data->bslider, MUIA_Numeric_Value, data->rgb[2] >> 24);

    nnset(data->colfield, MUIA_Colorfield_RGB, (IPTR) data->rgb);

    if (data->gradpen != -1)
    {
        hsb.cw_Brightness = 0xFFFFFFFF;
        ConvertHSBToRGB(&hsb, &cw);

        SetRGB32(&_screen(obj)->ViewPort, data->gradpen, cw.cw_Red,
            cw.cw_Green, cw.cw_Blue);
        if ((data->truecolor) && (data->grad))
            MUI_Redraw(data->grad, MADF_DRAWUPDATE);
    }

    NotifyAll(obj, data);
}

/*
 * Convert inverse gradient brightness with wheel hue/saturation,
 * synchronize sliders/preview and notify all. Requires a live wheel,
 * conversion library and hook data. Uses full brightness if no grad
 * pointer, but does not guard wheel absence. No ownership transfer or
 * returned status.
 */
static void GradFunc(struct Hook *hook, Object *obj, struct MUIP_CANotifyMsg *msg)
{
    struct Coloradjust_DATA *data = msg->caData;
    struct ColorWheelHSB hsb;
    struct ColorWheelRGB cw;

    ULONG bright = 0;
    if (data->grad)
        bright = XGET(data->grad, GRAD_CurVal);

    bright = 0xFFFF - bright;
    bright |= (bright << 16);

    hsb.cw_Hue = XGET(data->wheel, WHEEL_Hue);
    hsb.cw_Saturation = XGET(data->wheel, WHEEL_Saturation);
    hsb.cw_Brightness = bright;

    ConvertHSBToRGB(&hsb, &cw);

    data->rgb[0] = cw.cw_Red;
    data->rgb[1] = cw.cw_Green;
    data->rgb[2] = cw.cw_Blue;

    nnset(data->rslider, MUIA_Numeric_Value, data->rgb[0] >> 24);
    nnset(data->gslider, MUIA_Numeric_Value, data->rgb[1] >> 24);
    nnset(data->bslider, MUIA_Numeric_Value, data->rgb[2] >> 24);

    nnset(data->colfield, MUIA_Colorfield_RGB, (IPTR) data->rgb);

    NotifyAll(obj, data);
}

/*
 * Open gadget libraries, construct Group controls, initialize white RGB,
 * synchronize controls and register hooks. Return object or zero on
 * superclass failure; the AROS interface macro cannot fail. Requires
 * readable RGB input. Missing-library and ignored-child limitations follow
 * the class contract; do not infer a safe degraded mode.
 */
IPTR Coloradjust__OM_NEW(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct Coloradjust_DATA *data;
    struct TagItem *tags;
    struct TagItem *tag;
    struct Library *colorwheelbase, *gradientsliderbase;
    Object *rslider, *gslider, *bslider;
    Object *colfield, *wheel = NULL, *grad = NULL;
    ULONG *rgb;

    colorwheelbase = OpenLibrary("gadgets/colorwheel.gadget", 0);
    gradientsliderbase = OpenLibrary("gadgets/gradientslider.gadget", 0);

    obj = (Object *) DoSuperNewTags
        (cl, obj, NULL,
        MUIA_Group_Columns, 2,
        MUIA_Group_VertSpacing, 1,
        Child, (IPTR) Label1(_(MSG_COLORADJUST_RED)),
        Child, (IPTR) (rslider = (Object *)SliderObject,
            MUIA_Group_Horiz, TRUE,
            MUIA_Numeric_Min, 0,
            MUIA_Numeric_Max, 255,
        End),
        Child, (IPTR) Label1(_(MSG_COLORADJUST_GREEN)),
        Child, (IPTR) (gslider = (Object *)SliderObject,
            MUIA_Group_Horiz, TRUE,
            MUIA_Numeric_Min, 0,
            MUIA_Numeric_Max, 255,
        End),
        Child, (IPTR) Label1(_(MSG_COLORADJUST_BLUE)),
        Child, (IPTR) (bslider = (Object *)SliderObject,
            MUIA_Group_Horiz, TRUE,
            MUIA_Numeric_Min, 0,
            MUIA_Numeric_Max, 255,
        End),
        Child, (IPTR) VSpace(1),
        Child, (IPTR) VSpace(1),
        Child, (IPTR) (colfield = (Object *) ColorfieldObject,
            TextFrame,
            MUIA_Weight, 0,
        End),
        Child, (IPTR) (HGroup,
            MUIA_Group_HorizSpacing, 2,
            (!colorwheelbase) ? TAG_IGNORE : Child,
                (IPTR) (wheel = (Object *) BoopsiObject,
                MUIA_Boopsi_ClassID, (IPTR) "colorwheel.gadget",
                MUIA_Boopsi_MinWidth, 16,
                MUIA_Boopsi_MinHeight, 16,
                MUIA_Boopsi_Remember, WHEEL_Saturation,
                MUIA_Boopsi_Remember, WHEEL_Hue,
                MUIA_Boopsi_TagScreen, WHEEL_Screen,
                WHEEL_Screen, (IPTR) NULL,
                GA_Left, 0,
                GA_Top, 0,
                GA_Width, 0,
                GA_Height, 0,
                ICA_TARGET, ICTARGET_IDCMP,
                MUIA_FillArea, TRUE,
            End),
            (!gradientsliderbase) ? TAG_IGNORE : Child,
                (IPTR) (grad = (Object *) BoopsiObject,
                MUIA_Boopsi_ClassID, (IPTR) "gradientslider.gadget",
                MUIA_Boopsi_MinWidth, 16,
                MUIA_Boopsi_MinHeight, 16,
                MUIA_Boopsi_MaxWidth, 16,
                MUIA_Boopsi_Remember, GRAD_CurVal,
                MUIA_Boopsi_Remember, GRAD_PenArray,
                MUIA_Boopsi_Remember, GRAD_KnobPixels,
                GA_Left, 0,
                GA_Top, 0,
                GA_Width, 0,
                GA_Height, 0,
                GRAD_KnobPixels, 8,
                PGA_Freedom, LORIENT_VERT,
                ICA_TARGET, ICTARGET_IDCMP,
                MUIA_FillArea, TRUE,
            End),
            (!colorwheelbase && !gradientsliderbase) ? Child : TAG_IGNORE,
                (IPTR)HVSpace,
        End),
        TAG_MORE, (IPTR) msg->ops_AttrList);

    if (!obj)
    {
        if (gradientsliderbase)
            CloseLibrary(gradientsliderbase);
        if (colorwheelbase)
            CloseLibrary(colorwheelbase);
        return FALSE;
    }

    data = INST_DATA(cl, obj);

    data->colorwheelbase = colorwheelbase;
    data->gradientsliderbase = gradientsliderbase;
    if (!EXEC_INTERFACE_GET_MAIN(data->icolorwheel, data->colorwheelbase))
    {
        CoerceMethod(cl, obj, OM_DISPOSE);
        return (IPTR)NULL;
    }

    data->sliderhook.h_Entry = HookEntry;
    data->sliderhook.h_SubEntry = (HOOKFUNC) SliderFunc;

    data->wheelhook.h_Entry = HookEntry;
    data->wheelhook.h_SubEntry = (HOOKFUNC) WheelFunc;

    data->gradhook.h_Entry = HookEntry;
    data->gradhook.h_SubEntry = (HOOKFUNC) GradFunc;

    data->rslider = rslider;
    data->gslider = gslider;
    data->bslider = bslider;
    data->colfield = colfield;
    data->wheel = wheel;
    data->grad = grad;

    data->rgb[0] = data->rgb[1] = data->rgb[2] = 0xFFFFFFFF;

    /* parse initial taglist */
    for (tags = msg->ops_AttrList; (tag = NextTagItem(&tags));)
    {
        switch (tag->ti_Tag)
        {
        case MUIA_Coloradjust_Red:
            data->rgb[0] = (ULONG) tag->ti_Data;
            break;

        case MUIA_Coloradjust_Green:
            data->rgb[1] = (ULONG) tag->ti_Data;
            break;

        case MUIA_Coloradjust_Blue:
            data->rgb[2] = (ULONG) tag->ti_Data;
            break;

        case MUIA_Coloradjust_RGB:
            rgb = (ULONG *) tag->ti_Data;
            data->rgb[0] = *rgb++;
            data->rgb[1] = *rgb++;
            data->rgb[2] = *rgb++;
            break;

        }
    }

    nnset(colfield, MUIA_Colorfield_RGB, (IPTR) data->rgb);
    nnset(rslider, MUIA_Numeric_Value, data->rgb[0] >> 24);
    nnset(gslider, MUIA_Numeric_Value, data->rgb[1] >> 24);
    nnset(bslider, MUIA_Numeric_Value, data->rgb[2] >> 24);

    if (data->wheel )
    {
        struct ColorWheelRGB cw;
        struct ColorWheelHSB hsb;

        cw.cw_Red = data->rgb[0];
        cw.cw_Green = data->rgb[1];
        cw.cw_Blue = data->rgb[2];

        ConvertRGBToHSB(&cw, &hsb);

        nnset(data->wheel, WHEEL_HSB, (IPTR) & hsb);
        /* just to be sure - colorwheel seems to have some problems */
        nnset(data->wheel, WHEEL_Saturation, hsb.cw_Saturation);
        nnset(data->wheel, WHEEL_Hue, hsb.cw_Hue);
        if (data->grad)
        {
            nnset(data->grad, GRAD_CurVal, 0xFFFF - (hsb.cw_Brightness >> 16));
            nnset(data->grad, GRAD_PenArray, (IPTR) data->gradpenarray);
        }
    }

    DoMethod(rslider, MUIM_Notify, MUIA_Numeric_Value, MUIV_EveryTime,
        (IPTR) obj, 4, MUIM_CallHook, (IPTR) & data->sliderhook,
        (IPTR) data, 0);
    DoMethod(gslider, MUIM_Notify, MUIA_Numeric_Value, MUIV_EveryTime,
        (IPTR) obj, 4, MUIM_CallHook, (IPTR) & data->sliderhook,
        (IPTR) data, 1);
    DoMethod(bslider, MUIM_Notify, MUIA_Numeric_Value, MUIV_EveryTime,
        (IPTR) obj, 4, MUIM_CallHook, (IPTR) & data->sliderhook,
        (IPTR) data, 2);

    if (data->wheel )
    {
        DoMethod(data->wheel , MUIM_Notify, WHEEL_Hue, MUIV_EveryTime,
            (IPTR) obj, 3, MUIM_CallHook, (IPTR) & data->wheelhook,
            (IPTR) data);
        DoMethod(data->wheel , MUIM_Notify, WHEEL_Saturation, MUIV_EveryTime,
            (IPTR) obj, 3, MUIM_CallHook, (IPTR) & data->wheelhook,
            (IPTR) data);
    }
    if (data->grad)
    {
        DoMethod(data->grad, MUIM_Notify, GRAD_CurVal, MUIV_EveryTime,
            (IPTR) obj, 3, MUIM_CallHook, (IPTR) & data->gradhook,
            (IPTR) data);
    }
    return (IPTR) obj;
}

/*
 * Invoke the interface-drop macro (a no-op on AROS), dispose Group
 * children, then close saved library bases and return Group result.
 * Requires prior Cleanup of gradient pen and no active callbacks. Does not
 * independently dispose wrappers omitted from child tags.
 */
IPTR Coloradjust__OM_DISPOSE(struct IClass *cl, Object *obj, Msg msg)
{
    struct Coloradjust_DATA *data;
    struct Library *colorwheelbase;
    struct Library *gradientsliderbase;
    IPTR retval;

    data = INST_DATA(cl, obj);

    EXEC_INTERFACE_DROP(data->icolorwheel);
    colorwheelbase = data->colorwheelbase;
    gradientsliderbase = data->gradientsliderbase;

    retval = DoSuperMethodA(cl, obj, msg);

    if (colorwheelbase)
        CloseLibrary(colorwheelbase);
    if (gradientsliderbase)
        CloseLibrary(gradientsliderbase);

    return retval;
}

/*
 * Copy recognized RGB values in tag order, synchronize controls with child
 * notifications suppressed, update an active gradient pen, then return
 * Group setter result for original tags. Requires valid RGB array and
 * gadget configuration. No synthesized complementary notifications or
 * ownership transfer.
 */
IPTR Coloradjust__OM_SET(struct IClass *cl, Object *obj,
    struct opSet *msg)
{
    struct Coloradjust_DATA *data;
    struct TagItem *tags;
    struct TagItem *tag;
    ULONG *rgb;
    BOOL newcol = FALSE;

    data = INST_DATA(cl, obj);

    for (tags = msg->ops_AttrList; (tag = NextTagItem(&tags));)
    {
        switch (tag->ti_Tag)
        {
        case MUIA_Coloradjust_Red:
            data->rgb[0] = (ULONG) tag->ti_Data;
            newcol = TRUE;
            break;

        case MUIA_Coloradjust_Green:
            data->rgb[1] = (ULONG) tag->ti_Data;
            newcol = TRUE;
            break;

        case MUIA_Coloradjust_Blue:
            data->rgb[2] = (ULONG) tag->ti_Data;
            newcol = TRUE;
            break;

        case MUIA_Coloradjust_RGB:
            rgb = (ULONG *) tag->ti_Data;
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
        D(bug("coloradjust: sliders set to %ld, %ld, %ld\n",
                data->rgb[0] >> 24, data->rgb[1] >> 24,
                data->rgb[2] >> 24));

        nnset(data->colfield, MUIA_Colorfield_RGB, (IPTR) data->rgb);

        if (data->wheel)
        {
            struct ColorWheelRGB cw;
            struct ColorWheelHSB hsb;

            cw.cw_Red = data->rgb[0];
            cw.cw_Green = data->rgb[1];
            cw.cw_Blue = data->rgb[2];

            ConvertRGBToHSB(&cw, &hsb);
            nnset(data->wheel, WHEEL_HSB, (IPTR) & hsb);
            nnset(data->wheel, WHEEL_Saturation, hsb.cw_Saturation);
            nnset(data->wheel, WHEEL_Hue, hsb.cw_Hue);
            if (data->grad)
                nnset(data->grad, GRAD_CurVal,
                    0xFFFF - (hsb.cw_Brightness >> 16));

            if ((_flags(obj) & MADF_SETUP) && (data->gradpen != -1))
            {
                hsb.cw_Brightness = 0xFFFFFFFF;
                ConvertHSBToRGB(&hsb, &cw);

                SetRGB32(&_screen(obj)->ViewPort, data->gradpen, cw.cw_Red,
                    cw.cw_Green, cw.cw_Blue);
                if ((data->truecolor) && (data->grad))
                {
                    MUI_Redraw(data->grad, MADF_DRAWOBJECT);
                }
            }
        }
    }

    return DoSuperMethodA(cl, obj, (Msg) msg);
}

/*
 * Return TRUE after storing a component or borrowed internal RGB pointer in
 * valid IPTR storage; delegate other attributes to Group. ModeID is not
 * handled. Array contents may change on interaction/set and lifetime ends
 * at disposal.
 */
IPTR Coloradjust__OM_GET(struct IClass *cl, Object *obj,
    struct opGet *msg)
{
    struct Coloradjust_DATA *data = INST_DATA(cl, obj);
    IPTR *store = msg->opg_Storage;

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
        *(IPTR *) store = (IPTR) & data->rgb[0];
        return TRUE;
    }

    return DoSuperMethodA(cl, obj, (Msg) msg);
}

/*
 * Call Group, return zero on failure; with wheel obtain a gradient pen and
 * configure its full-brightness color and fallback array. Return one even
 * on pen failure. Requires current screen and conversion library; pair with
 * Cleanup.
 */
IPTR Coloradjust__MUIM_Setup(struct IClass *cl, Object *obj,
    struct MUIP_Setup *msg)
{
    struct Coloradjust_DATA *data = INST_DATA(cl, obj);

    if (!(DoSuperMethodA(cl, obj, (Msg) msg)))
        return 0;

    if (data->wheel)
    {
        struct ColorWheelHSB hsb;
        struct ColorWheelRGB rgb;

        rgb.cw_Red = data->rgb[0];
        rgb.cw_Green = data->rgb[1];
        rgb.cw_Blue = data->rgb[2];

        data->gradpenarray[0] = _pens(obj)[MPEN_SHINE];
        data->gradpenarray[1] = _pens(obj)[MPEN_SHADOW];
        data->gradpenarray[2] = (UWORD) ~ 0;

        data->gradpen = ObtainPen(_screen(obj)->ViewPort.ColorMap,
            (ULONG) - 1,
            rgb.cw_Red, rgb.cw_Green, rgb.cw_Blue, PENF_EXCLUSIVE);

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

        if (data->gradpen != -1)
        {
            SetRGB32(&_screen(obj)->ViewPort, data->gradpen, rgb.cw_Red,
                rgb.cw_Green, rgb.cw_Blue);
        }

        data->truecolor =
            GetBitMapAttr(_screen(obj)->RastPort.BitMap, BMA_DEPTH) >= 15;
    }

    return 1;
}

/*
 * Release gradpen when not -1 using the current screen, set it to -1 and
 * return Group cleanup result. Requires valid setup/screen. Constructor
 * does not initialize gradpen to -1; skipping wheel setup can cause release
 * of initial zero.
 */
IPTR Coloradjust__MUIM_Cleanup(struct IClass *cl, Object *obj,
    struct MUIP_Cleanup *msg)
{
    struct Coloradjust_DATA *data = INST_DATA(cl, obj);

    if (data->gradpen != -1)
    {
        ReleasePen(_screen(obj)->ViewPort.ColorMap, data->gradpen);
        data->gradpen = -1;
    }

    return DoSuperMethodA(cl, obj, (Msg) msg);
}

#if ZUNE_BUILTIN_COLORADJUST
/*
 * Dispatch local methods and return their results, delegating other
 * messages to the superclass. Requires ABI-correct messages and a live
 * object except construction. Owned and borrowed resources follow the class
 * lifecycle contract.
 */
BOOPSI_DISPATCHER(IPTR, Coloradjust_Dispatcher, cl, obj, msg)
{
    switch (msg->MethodID)
    {
    case OM_NEW:
        return Coloradjust__OM_NEW(cl, obj, (struct opSet *)msg);
    case OM_DISPOSE:
        return Coloradjust__OM_DISPOSE(cl, obj, msg);
    case OM_SET:
        return Coloradjust__OM_SET(cl, obj, (struct opSet *)msg);
    case OM_GET:
        return Coloradjust__OM_GET(cl, obj, (struct opGet *)msg);
    case MUIM_Setup:
        return Coloradjust__MUIM_Setup(cl, obj, (struct MUIP_Setup *)msg);
    case MUIM_Cleanup:
        return Coloradjust__MUIM_Cleanup(cl, obj,
            (struct MUIP_Cleanup *)msg);
    default:
        return DoSuperMethodA(cl, obj, msg);
    }
}
BOOPSI_DISPATCHER_END

const struct __MUIBuiltinClass _MUI_Coloradjust_desc =
{
    MUIC_Coloradjust,
    MUIC_Group,
    sizeof(struct Coloradjust_DATA),
    (void *) Coloradjust_Dispatcher
};
#endif /* ZUNE_BUILTIN_COLORADJUST */
