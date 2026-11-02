/*
    Copyright (C) 2002-2026, The AROS Development Team. All rights reserved.
*/

#define MUIMASTER_YES_INLINE_STDARG

#include <stdio.h>

#include <graphics/gfx.h>
#include <graphics/view.h>
#include <graphics/gfxmacros.h>
#include <clib/alib_protos.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/utility.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>

#include <string.h>

#include "mui.h"
#include "muimaster_intern.h"
#include "textengine.h"
#include "support.h"
#include "support_classes.h"
#include "colorfield_private.h"

/*  #define MYDEBUG 1 */
#include "debug.h"

/****** Colorfield.mui/MUIC_Colorfield ************
*
*   NAME
*       MUIC_Colorfield
*
*   SUPERCLASS
*       Area.mui
*
*   DESCRIPTION
*       Display an editable color using a screen pen. Initial RGB components are
*       zero. Without an explicit fixed pen, Setup requests an exclusive pen. A
*       fixed pen is borrowed and its screen color is overwritten with the
*       stored RGB; its previous color is not restored. Use only a pen that the
*       caller is entitled to recolor. The class stores pen numbers in UBYTE,
*       even though the attribute is declared ULONG.
*       Setup returns zero on Area failure, otherwise one, including no-screen
*       and allocation-failure paths. Failed allocation sets the no-pen flag and
*       pen value 255. Cleanup releases only locally allocated pens, sets their
*       stored value to 255, clears the no-pen flag and delegates to Area. Fixed
*       pens are neither released nor restored. Complete normal Cleanup before
*       disposal; there is no local Dispose override.
*
*   METHODS
*       AskMinMax adds one to both minima and sixteen to both defaults, sets
*       both maxima MUI_MAXMAX and returns zero. Draw calls Area and handles
*       full/update flags. It fills with the stored pen or a shadow/background
*       pattern when allocation failed, then clears the RastPort fill pattern.
*       It returns zero normally, one when no RastPort exists after flag
*       acceptance.
*       ConnectParent delegates first and, if already set up with no stored
*       color map, invokes pen setup. That path requires a valid screen and does
*       not perform the Setup method's no-screen check.
*
*   NOTES
*       Color writes copy values in tag order, then deliver superclass
*       notifications before changing the screen color. Component writes add an
*       RGB notification; RGB writes add all three component notifications,
*       using final stored values before the original tags. Equal values are not
*       suppressed. When set up with a screen and no no-pen flag, SetRGB32
*       recolors the stored pen; redraw is requested only above depth eight.
*       Other users of a fixed pen can therefore change appearance too.
*       Pen replacement releases any locally allocated pen but does not allocate
*       a replacement in OM_SET. Setting a sentinel does not clear an
*       already-set fixed-pen flag. A no-pen flag from a failed allocation is
*       not cleared by an ordinary subsequent Pen or RGB set. These are
*       implementation limitations, not a guarantee of switching between
*       automatic and fixed modes.
*
*/
/****** Colorfield.mui/MUIA_Colorfield_Pen ************
*
*   NAME
*       MUIA_Colorfield_Pen
*
*   FUNCTION
*       Set or retrieve the stored screen pen number.
*
*   INPUTS
*       Initialize, set and get, ULONG input truncated to UBYTE. With no Pen tag
*       the initial stored number is zero, but automatic allocation remains
*       selected.
*
*   NOTES
*       An input whose low byte is not 255 sets fixed-pen mode. Low byte 255
*       does not set that flag and does not clear it if already set. Get returns
*       the stored 0..255 value, with no availability check; 255 also represents
*       allocation failure or a released allocated pen. Fixed pens are borrowed
*       and recolored during setup and color updates. Set releases an owned pen
*       before replacing the number but does not obtain another; see class
*       limitations. Repeated constructor Pen tags can leave fixed mode set even
*       when the final number is 255.
*
*/
/****** Colorfield.mui/MUIA_Colorfield_Red ************
*
*   NAME
*       MUIA_Colorfield_Red
*
*   FUNCTION
*       Set or retrieve the red component.
*
*   INPUTS
*       Initialize, set and get, ULONG in 0..0xffffffff. Default zero.
*
*   NOTES
*       Values are stored at full 32-bit precision and copied by value. RGB and
*       component tags share storage and take effect in tag order. Set adds an
*       RGB notification before the original tags, then recolors the screen pen
*       when usable; this changes the color map, not just local display pixels.
*       Get reports stored requested color, not a fresh screen measurement.
*
*/
/****** Colorfield.mui/MUIA_Colorfield_Green ************
*
*   NAME
*       MUIA_Colorfield_Green
*
*   FUNCTION
*       Set or retrieve the green component.
*
*   INPUTS
*       Initialize, set and get, ULONG in 0..0xffffffff. Default zero.
*
*   NOTES
*       Values are stored at full 32-bit precision and copied by value. RGB and
*       component tags share storage and take effect in tag order. Set adds an
*       RGB notification before the original tags, then recolors the screen pen
*       when usable; this changes the color map, not just local display pixels.
*       Get reports stored requested color, not a fresh screen measurement.
*
*/
/****** Colorfield.mui/MUIA_Colorfield_Blue ************
*
*   NAME
*       MUIA_Colorfield_Blue
*
*   FUNCTION
*       Set or retrieve the blue component.
*
*   INPUTS
*       Initialize, set and get, ULONG in 0..0xffffffff. Default zero.
*
*   NOTES
*       Values are stored at full 32-bit precision and copied by value. RGB and
*       component tags share storage and take effect in tag order. Set adds an
*       RGB notification before the original tags, then recolors the screen pen
*       when usable; this changes the color map, not just local display pixels.
*       Get reports stored requested color, not a fresh screen measurement.
*
*/
/****** Colorfield.mui/MUIA_Colorfield_RGB ************
*
*   NAME
*       MUIA_Colorfield_RGB
*
*   FUNCTION
*       Set or retrieve all three stored components.
*
*   INPUTS
*       Initialize and set with a non-NULL pointer to three readable ULONG
*       values in red, green, blue order. Get returns a pointer to the internal
*       three-element array. Default all zero.
*
*   NOTES
*       Input values are copied synchronously; no input pointer is retained. The
*       getter pointer is borrowed until disposal and its contents change on
*       later sets. Set from the same array is supported. Use attributes to
*       change values so notifications and screen updates occur. RGB Set adds
*       Red, Green and Blue notifications with final stored values before
*       forwarding the original tag list. The components use the full 32-bit
*       range.
*
*/

extern struct Library *MUIMasterBase;

#define FLAG_FIXED_PEN      1
#define FLAG_PEN_ALLOCATED  2
#define FLAG_NO_PEN         4







/*
 * Construct Area and copy RGB/Pen tags in order. Return object or zero on
 * superclass failure. RGB needs three readable ULONG values; no input
 * pointer is retained. Pen is truncated to UBYTE and fixed mode is only
 * set, never cleared by these tags.
 */
IPTR Colorfield__OM_NEW(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct Colorfield_DATA *data;
    struct TagItem *tags;
    struct TagItem *tag;
    ULONG *rgb;

    obj = (Object *) DoSuperMethodA(cl, obj, (Msg) msg);
    if (!obj)
        return FALSE;

    data = INST_DATA(cl, obj);

    /* parse initial taglist */
    for (tags = msg->ops_AttrList; (tag = NextTagItem(&tags));)
    {
        switch (tag->ti_Tag)
        {
        case MUIA_Colorfield_Red:
            data->rgb[0] = (ULONG) tag->ti_Data;
            break;

        case MUIA_Colorfield_Green:
            data->rgb[1] = (ULONG) tag->ti_Data;
            break;

        case MUIA_Colorfield_Blue:
            data->rgb[2] = (ULONG) tag->ti_Data;
            break;

        case MUIA_Colorfield_RGB:
            rgb = (ULONG *) tag->ti_Data;
            data->rgb[0] = *rgb++;
            data->rgb[1] = *rgb++;
            data->rgb[2] = *rgb++;
            break;

        case MUIA_Colorfield_Pen:
            if ((data->pen = (UBYTE) tag->ti_Data) != (UBYTE)-1)
                data->flags |= FLAG_FIXED_PEN;
            break;

        }
    }

    return (IPTR) obj;
}

/*
 * Copy local values, release an owned pen on Pen replacement and prepend
 * dependent-color notifications before the original tags. Return Area
 * result. Notifications run before SetRGB32; valid screen resources and RGB
 * input are required. No replacement allocation or reliable fixed-mode
 * reset occurs here.
 */
IPTR Colorfield__OM_SET(struct IClass *cl, Object *obj,
    struct opSet *msg)
{
    struct Colorfield_DATA *data;
    struct opSet supMsg;
    struct TagItem *tags;
    struct TagItem *tag;
    ULONG *rgb;
    BOOL newcol = FALSE;
    IPTR retval;
    struct TagItem extra_tags[] = {{TAG_IGNORE, 0}, {TAG_IGNORE, 0},
        {TAG_IGNORE, 0}, {TAG_IGNORE, 0},
        {TAG_MORE, (IPTR)msg->ops_AttrList}};

    data = INST_DATA(cl, obj);

    for (tags = msg->ops_AttrList; (tag = NextTagItem(&tags));)
    {
        switch (tag->ti_Tag)
        {
        case MUIA_Colorfield_Red:
            data->rgb[0] = (ULONG) tag->ti_Data;
            newcol = TRUE;
            extra_tags[3].ti_Tag = MUIA_Colorfield_RGB;
            break;

        case MUIA_Colorfield_Green:
            data->rgb[1] = (ULONG) tag->ti_Data;
            newcol = TRUE;
            extra_tags[3].ti_Tag = MUIA_Colorfield_RGB;
            break;

        case MUIA_Colorfield_Blue:
            data->rgb[2] = (ULONG) tag->ti_Data;
            newcol = TRUE;
            extra_tags[3].ti_Tag = MUIA_Colorfield_RGB;
            break;

        case MUIA_Colorfield_RGB:
            rgb = (ULONG *) tag->ti_Data;
            data->rgb[0] = *rgb++;
            data->rgb[1] = *rgb++;
            data->rgb[2] = *rgb++;
            newcol = TRUE;
            extra_tags[0].ti_Tag = MUIA_Colorfield_Red;
            extra_tags[1].ti_Tag = MUIA_Colorfield_Green;
            extra_tags[2].ti_Tag = MUIA_Colorfield_Blue;
            break;

        case MUIA_Colorfield_Pen:
            if ((data->flags & FLAG_PEN_ALLOCATED) && (data->cm))
            {
                ULONG disposepen = data->pen;
                struct ColorMap *cm = data->cm;
                data->flags &= ~(FLAG_PEN_ALLOCATED | FLAG_NO_PEN);
                data->cm = NULL;
                ReleasePen(cm, disposepen);
            }
            if ((data->pen = (UBYTE) tag->ti_Data) != (UBYTE)-1)
                data->flags |= FLAG_FIXED_PEN;
            newcol = TRUE;
            break;

        }
    }

    /* Prepare notification values for inter-dependent attributes */
    extra_tags[0].ti_Data = data->rgb[0];
    extra_tags[1].ti_Data = data->rgb[1];
    extra_tags[2].ti_Data = data->rgb[2];
    extra_tags[3].ti_Data = (IPTR) data->rgb;
    supMsg.MethodID = msg->MethodID;
    supMsg.ops_AttrList = extra_tags;
    supMsg.ops_GInfo = msg->ops_GInfo;
    
    retval = DoSuperMethodA(cl, obj, (Msg) &supMsg);

    if (newcol && (_flags(obj) & MADF_SETUP)
        && !(data->flags & FLAG_NO_PEN))
    {
        if (_screen(obj))
        {
            SetRGB32(&_screen(obj)->ViewPort, data->pen, data->rgb[0],
                data->rgb[1], data->rgb[2]);

            if (GetBitMapAttr(_rp(obj)->BitMap, BMA_DEPTH) > 8)
            {
                MUI_Redraw(obj, MADF_DRAWUPDATE);
            }
        }
    }

    return retval;
}

/*
 * Write stored component, borrowed RGB-array pointer or UBYTE pen value
 * into valid caller storage and return TRUE. Delegate other attributes to
 * Area. No availability check, screen readback or ownership transfer; the
 * array lasts until disposal.
 */
IPTR Colorfield__OM_GET(struct IClass *cl, Object *obj,
    struct opGet *msg)
{
    struct Colorfield_DATA *data = INST_DATA(cl, obj);
    IPTR *store = msg->opg_Storage;

    switch (msg->opg_AttrID)
    {
    case MUIA_Colorfield_Red:
        *store = data->rgb[0];
        break;

    case MUIA_Colorfield_Green:
        *store = data->rgb[1];
        break;

    case MUIA_Colorfield_Blue:
        *store = data->rgb[2];
        break;

    case MUIA_Colorfield_RGB:
        *(ULONG **) store = data->rgb;
        break;

    case MUIA_Colorfield_Pen:
        *store = data->pen;
        break;

    default:
        return DoSuperMethodA(cl, obj, (Msg) msg);
    }

    return TRUE;
}

/*
 * Use the current screen color map to recolor a borrowed fixed pen or
 * obtain an exclusive pen. Store a truncated UBYTE result and
 * allocation/no-pen flags. Requires a valid screen and instance data. No
 * return status; failed acquisition is represented by no-pen and value 255.
 */
void Colorfield_SetupPen(Object *obj, struct Colorfield_DATA *data)
{
    data->cm = _screen(obj)->ViewPort.ColorMap;

    if (data->flags & FLAG_FIXED_PEN)
    {
        SetRGB32(&_screen(obj)->ViewPort,
            data->pen, data->rgb[0], data->rgb[1], data->rgb[2]);
    }
    else
    {
        LONG pen;

        pen = ObtainPen(data->cm,
            (ULONG) -1,
            data->rgb[0], data->rgb[1], data->rgb[2], PENF_EXCLUSIVE);

        if (pen == -1)
        {
            data->flags |= FLAG_NO_PEN;
            data->pen = -1;
        }
        else
        {
            data->pen = (UBYTE) pen;
            data->flags |= FLAG_PEN_ALLOCATED;
        }
    }
}

/*
 * Call Area; return zero on failure, one without a screen or after pen
 * setup even on acquisition failure. Requires normal lifecycle; acquired
 * resources belong to the instance until Cleanup.
 */
IPTR Colorfield__MUIM_Setup(struct IClass *cl, Object *obj,
    struct MUIP_Setup *msg)
{
    struct Colorfield_DATA *data = INST_DATA(cl, obj);

    if (!(DoSuperMethodA(cl, obj, (Msg) msg)))
        return 0;

    if (!_screen(obj))
        return 1;

    Colorfield_SetupPen(obj, data);

    return 1;
}

/*
 * Release only an allocated pen using its saved color map, clear ownership
 * and no-pen flags, and return Area result. Requires matched setup
 * resources. Fixed pens and their colors are not restored; no borrowed
 * object ownership changes.
 */
IPTR Colorfield__MUIM_Cleanup(struct IClass *cl, Object *obj,
    struct MUIP_Cleanup *msg)
{
    struct Colorfield_DATA *data = INST_DATA(cl, obj);

    if (data->flags & FLAG_PEN_ALLOCATED)
    {
        ULONG disposepen = data->pen;
        struct ColorMap *cm = data->cm;
        data->flags &= ~FLAG_PEN_ALLOCATED;
        data->cm = NULL;
        data->pen = -1;
        ReleasePen(cm, disposepen);
    }
    data->flags &= ~FLAG_NO_PEN;

    return DoSuperMethodA(cl, obj, (Msg) msg);
}

/*
 * Add local minimum one and default sixteen on both axes after Area; assign
 * unlimited maxima and return zero. Requires writable MinMaxInfo. No
 * resource acquisition or relayout is performed.
 */
IPTR Colorfield__MUIM_AskMinMax(struct IClass *cl, Object *obj,
    struct MUIP_AskMinMax *msg)
{
    DoSuperMethodA(cl, obj, (Msg) msg);

    msg->MinMaxInfo->MinWidth += 1;
    msg->MinMaxInfo->MinHeight += 1;
    msg->MinMaxInfo->DefWidth += 16;
    msg->MinMaxInfo->DefHeight += 16;
    msg->MinMaxInfo->MaxWidth = MUI_MAXMAX;
    msg->MinMaxInfo->MaxHeight = MUI_MAXMAX;

    return 0;
}

/*
 * Call Area, then draw for full/update flags using stored pen or
 * allocation-failure pattern; clear fill pattern afterward. Return one if
 * accepted but no RastPort, otherwise zero. Requires normal setup and
 * drawable bounds; no allocation or ownership transfer.
 */
IPTR Colorfield__MUIM_Draw(struct IClass *cl, Object *obj,
    struct MUIP_Draw *msg)
{
    struct Colorfield_DATA *data = INST_DATA(cl, obj);

    DoSuperMethodA(cl, obj, (Msg) msg);

    if (!(msg->flags & (MADF_DRAWOBJECT | MADF_DRAWUPDATE)))
        return FALSE;

    if (!_rp(obj))
        return TRUE;

    if (data->flags & FLAG_NO_PEN)
    {
        static UWORD pat[] = { 0x1111, 0x2222, 0x4444, 0x8888 };

        SetAfPt(_rp(obj), pat, 2);
        SetABPenDrMd(_rp(obj), _pens(obj)[MPEN_SHADOW],
            _pens(obj)[MPEN_BACKGROUND], JAM2);
    }
    else
    {
        SetABPenDrMd(_rp(obj), data->pen, 0, JAM1);
    }

    RectFill(_rp(obj), _mleft(obj), _mtop(obj), _mright(obj),
        _mbottom(obj));

    SetAfPt(_rp(obj), NULL, 0);

    return 0;
}

/*
 * Return Area connection result after setting up a pen when already set up
 * with no saved color map. That helper dereferences the current screen,
 * which must be valid. Can allocate or recolor a pen; does not check
 * superclass success before its local condition.
 */
IPTR Colorfield__MUIM_ConnectParent(struct IClass *cl, Object *obj,
    struct MUIP_ConnectParent *msg)
{
    struct Colorfield_DATA *data = INST_DATA(cl, obj);
    IPTR retval;

    retval = DoSuperMethodA(cl, obj, (Msg) msg);

    if ((_flags(obj) & MADF_SETUP) && !(data->cm))
    {
        Colorfield_SetupPen(obj, data);
    }
    return retval;
}


#if ZUNE_BUILTIN_COLORFIELD
/*
 * Dispatch local methods and return their results, delegating other
 * messages to the superclass. Requires ABI-correct messages and a live
 * object except construction. Owned and borrowed resources follow the class
 * lifecycle contract.
 */
BOOPSI_DISPATCHER(IPTR, Colorfield_Dispatcher, cl, obj, msg)
{
    switch (msg->MethodID)
    {
    case OM_NEW:
        return Colorfield__OM_NEW(cl, obj, (struct opSet *)msg);
    case OM_SET:
        return Colorfield__OM_SET(cl, obj, (struct opSet *)msg);
    case OM_GET:
        return Colorfield__OM_GET(cl, obj, (struct opGet *)msg);
    case MUIM_Setup:
        return Colorfield__MUIM_Setup(cl, obj, (struct MUIP_Setup *)msg);
    case MUIM_Cleanup:
        return Colorfield__MUIM_Cleanup(cl, obj,
            (struct MUIP_Cleanup *)msg);
    case MUIM_ConnectParent:
        return Colorfield__MUIM_ConnectParent(cl, obj,
            (struct MUIP_ConnectParent *)msg);
    case MUIM_AskMinMax:
        return Colorfield__MUIM_AskMinMax(cl, obj,
            (struct MUIP_AskMinMax *)msg);
    case MUIM_Draw:
        return Colorfield__MUIM_Draw(cl, obj, (struct MUIP_Draw *)msg);
    default:
        return DoSuperMethodA(cl, obj, msg);
    }
}
BOOPSI_DISPATCHER_END

const struct __MUIBuiltinClass _MUI_Colorfield_desc =
{
    MUIC_Colorfield,
    MUIC_Area,
    sizeof(struct Colorfield_DATA),
    (void *) Colorfield_Dispatcher
};
#endif /* ZUNE_BUILTIN_COLORFIELD */
