/*
    Copyright  2002-2012, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/memory.h>
#include <intuition/icclass.h>
#include <intuition/gadgetclass.h>
#include <clib/alib_protos.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/muimaster.h>
#include <proto/cybergraphics.h>

#ifdef __AROS__
#include <intuition/windecorclass.h>
#endif
#include <cybergraphx/cybergraphics.h>
#include <graphics/rpattr.h>

#include "../datatypescache.h"
#include "../imspec_intern.h"

#include "mui.h"
#include "muimaster_intern.h"
#include "support.h"
#include "prefs.h"

/*  #define MYDEBUG 1 */
#include "debug.h"

extern struct Library *MUIMasterBase;

#define SCALE16_METHOD 2        /* 1 or 2 */

#if SCALE16_METHOD != 1
#define calcscale16(x)
#endif

struct Prop_DATA
{
    ULONG entries;
    LONG first;
    ULONG visible;
    LONG deltafactor;
#if SCALE16_METHOD == 1
    ULONG scale16;
#endif
    LONG gadgetid;

    int horiz;
    int usewinborder;
    int sb_type;
    UWORD minwidth, minheight;
    UWORD maxwidth, maxheight;
    UWORD defwidth, defheight;
    UWORD propwidth, propheight;

    Object *prop_object;
    struct MUI_EventHandlerNode ehn;
    struct Hook dhook;
    struct dt_node *node;
    Object *obj;
    struct NewImage *buffer;
    struct NewImage *temp;
    struct BitMap *mapbuffer;
    struct BitMap *maptemp;
    struct RastPort *tmprp;
};

#if SCALE16_METHOD == 1
static void calcscale16(struct Prop_DATA *data)
{
    if (data->entries < 65535)
    {
        data->scale16 = 0x10000;
    }
    else
    {
        unsigned long long v =
            ((unsigned long long)data->entries) * 0x10000 / 65535;

        data->scale16 = (ULONG) v;
    }
}
#endif

static ULONG downscale(struct Prop_DATA *data, ULONG val)
{
#if SCALE16_METHOD == 1
    if (data->scale16 != 0x10000)
    {
        unsigned long long v =
            ((unsigned long long)val) * 0x10000 / data->scale16;
        val = (ULONG) v;
    }
#else
    if (data->entries >= 0x10000)
    {
        unsigned long long v =
            ((unsigned long long)val) * 65535 / data->entries;
        val = (ULONG) v;
    }

#endif
    return val;
}

static ULONG upscale(struct Prop_DATA *data, ULONG val)
{
#if SCALE16_METHOD == 1
    if (data->scale16 != 0x10000)
    {
        unsigned long long v =
            ((unsigned long long)val) * data->scale16 / 0x10000;
        val = (ULONG) v;
    }
#else
    if (data->entries >= 0x10000)
    {
        unsigned long long v =
            ((unsigned long long)val) * data->entries / 65535;
        val = (ULONG) v;
    }
#endif
    return val;
}

IPTR Prop__OM_NEW(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct Prop_DATA *data;
    struct TagItem *tags, *tag;

    obj = (Object *) DoSuperNewTags(cl, obj, NULL,
        MUIA_Background, MUII_PropBack, TAG_MORE, (IPTR) msg->ops_AttrList);

    if (!obj)
        return FALSE;

    data = INST_DATA(cl, obj);
    data->deltafactor = 1;

    /* parse initial taglist */
    for (tags = msg->ops_AttrList; (tag = NextTagItem(&tags));)
    {
        switch (tag->ti_Tag)
        {
        case MUIA_Prop_Entries:
            data->entries = tag->ti_Data;
            break;
        case MUIA_Prop_First:
            data->first = tag->ti_Data;
            break;
        case MUIA_Prop_Horiz:
            data->horiz = tag->ti_Data != 0;
            break;
        case MUIA_Prop_Slider:
            break;
        case MUIA_Prop_UseWinBorder:
            data->usewinborder = tag->ti_Data;
            break;
        case MUIA_Prop_Visible:
            data->visible = tag->ti_Data;
            break;

        case MUIA_Prop_DeltaFactor:
            data->deltafactor = tag->ti_Data;
            break;
        }
    }

    if (data->horiz)
    {
        data->minwidth = 6;
        data->minheight = 6;
        data->maxwidth = MUI_MAXMAX;
        data->maxheight = MUI_MAXMAX;
        data->defwidth = 50;
        data->defheight = 6;
    }
    else
    {
        data->minwidth = 6;
        data->minheight = 6;
        data->maxwidth = MUI_MAXMAX;
        data->maxheight = MUI_MAXMAX;
        data->defwidth = 6;
        data->defheight = 50;
    }

    if (data->first < 0)
        data->first = 0;

    data->ehn.ehn_Events = IDCMP_IDCMPUPDATE;
    data->ehn.ehn_Priority = 0;
    data->ehn.ehn_Flags = 0;
    data->ehn.ehn_Object = obj;
    data->ehn.ehn_Class = cl;

    if (data->usewinborder)
        _flags(obj) |= MADF_BORDERGADGET;

    calcscale16(data);

    return (IPTR) obj;
}

IPTR Prop__OM_DISPOSE(struct IClass *cl, Object *obj, Msg msg)
{
    struct Prop_DATA *data = INST_DATA(cl, obj);

    if (data->prop_object && !data->usewinborder)
    {
        RemoveGadget(_window(obj), (struct Gadget *)data->prop_object);
        DisposeObject(data->prop_object);
    }

    DisposeImageContainer(data->buffer);
    DisposeImageContainer(data->temp);
    if (data->mapbuffer != NULL)
        FreeBitMap(data->mapbuffer);
    if (data->maptemp != NULL)
        FreeBitMap(data->maptemp);
    if (data->tmprp != NULL)
        FreeRastPort(data->tmprp);

    data->buffer = NULL;
    data->temp = NULL;
    data->mapbuffer = NULL;
    data->maptemp = NULL;
    data->tmprp = NULL;

    return DoSuperMethodA(cl, obj, msg);
}

IPTR Prop__OM_SET(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct TagItem *tags, *tag;
    struct Prop_DATA *data = INST_DATA(cl, obj);
    int refresh = 0;
    int only_trigger = 0;

    for (tags = msg->ops_AttrList; (tag = NextTagItem(&tags));)
    {
        switch (tag->ti_Tag)
        {
        case MUIA_Prop_Entries:
            if ((IPTR) data->entries != tag->ti_Data)
            {
                data->entries = tag->ti_Data;
                refresh = 1;
            }
            else
            {
                tag->ti_Tag = TAG_IGNORE;
            }
            break;

        case MUIA_Prop_First:
            if ((IPTR) data->first != tag->ti_Data)
            {
                data->first = tag->ti_Data;
                refresh = 1;
            }
            else
            {
                tag->ti_Tag = TAG_IGNORE;
            }
            break;

        case MUIA_Prop_Slider:
            break;

        case MUIA_Prop_Visible:
            if ((IPTR) data->visible != tag->ti_Data)
            {
                data->visible = tag->ti_Data;
                refresh = 1;
            }
            else
            {
                tag->ti_Tag = TAG_IGNORE;
            }
            break;

        case MUIA_Prop_OnlyTrigger:
            only_trigger = tag->ti_Data;
            break;

        case MUIA_Prop_DeltaFactor:
            data->deltafactor = tag->ti_Data;
            break;
        }
    }

    if (data->first < 0)
        data->first = 0;

    if (data->prop_object && refresh && !only_trigger)
    {
        calcscale16(data);

        /* Rendering will happen here! This could make problems with
         * virtual groups, forward this to MUIM_Draw??? */
        SetAttrs(data->prop_object, ICA_TARGET, NULL, TAG_DONE);
        if (SetGadgetAttrs((struct Gadget *)data->prop_object, _window(obj),
                NULL, PGA_Top, downscale(data, data->first), PGA_Visible,
                downscale(data, data->visible), PGA_Total, downscale(data,
                    data->entries), TAG_DONE))
            RefreshGList((struct Gadget *)data->prop_object, _window(obj),
                NULL, 1);
        SetAttrs(data->prop_object, ICA_TARGET, ICTARGET_IDCMP, TAG_DONE);
    }

    return DoSuperMethodA(cl, obj, (Msg) msg);
}

#define STORE *(msg->opg_Storage)
IPTR Prop__OM_GET(struct IClass *cl, Object *obj, struct opGet *msg)
{
    struct Prop_DATA *data = INST_DATA(cl, obj);
    switch (msg->opg_AttrID)
    {
    case MUIA_Prop_First:
        {
            if (data->prop_object)
            {
                IPTR v;
                /* So we can get a more current value */
                GetAttr(PGA_Top, data->prop_object, &v);
                data->first = upscale(data, v);
            }
            STORE = data->first;
            return 1;
        }
    case MUIA_Prop_Entries:
        STORE = data->entries;
        return 1;
    case MUIA_Prop_Visible:
        STORE = data->visible;
        return 1;

        /* CHECKME: MUIA_Prop_Release

           TextEditor.mcc sets up notification on slider obj which is subclass
           of group and notification on group children (the prop object) will
           be dropped if the child does not return TRUE on OM_GET.

           It may be that MUI handles this differently, because a quick check
           with UAE/MUI showed that a GetAttr() of MUIA_Prop_Release on alider
           object does not work (returns FALSE). Maybe MUI slider class is
           similiar to for exampe NListview.mcc which overrides MUIM_Notify
           where it checks for known attributes and forwards the Method to
           the correct child of the group.
         */
    case MUIA_Prop_Release:
        STORE = 0;
        return 1;
    default:
        return DoSuperMethodA(cl, obj, (Msg) msg);
    }

    return 1;
}
#undef STORE

IPTR Prop__MUIM_AskMinMax(struct IClass *cl, Object *obj,
    struct MUIP_AskMinMax *msg)
{
    struct Prop_DATA *data = INST_DATA(cl, obj);

    /*
     ** let our superclass first fill in what it thinks about sizes.
     ** this will e.g. add the size of frame and inner spacing.
     */
    DoSuperMethodA(cl, obj, (Msg) msg);

    msg->MinMaxInfo->MinWidth += data->minwidth;
    msg->MinMaxInfo->DefWidth += data->defwidth;
    msg->MinMaxInfo->MaxWidth = data->maxwidth;

    msg->MinMaxInfo->MinHeight += data->minheight;
    msg->MinMaxInfo->DefHeight += data->defheight;
    msg->MinMaxInfo->MaxHeight = data->maxheight;

    D(bug("Prop %p minheigh=%d\n", obj, msg->MinMaxInfo->MinHeight));

    return TRUE;
}

IPTR Prop__MUIM_Setup(struct IClass *cl, Object *obj,
    struct MUIP_Setup *msg)
{
    struct Prop_DATA *data = INST_DATA(cl, obj);
    ULONG rc = DoSuperMethodA(cl, obj, (Msg) msg);
    if (!rc)
        return 0;

    DoMethod(_win(obj), MUIM_Window_AddEventHandler, (IPTR) & data->ehn);

    data->buffer = NULL;
    data->temp = NULL;
    data->mapbuffer = NULL;
    data->maptemp = NULL;
    data->tmprp = NULL;

    if (!data->usewinborder)
    {
        data->gadgetid = DoMethod(_win(obj), MUIM_Window_AllocGadgetID);
        if (data->horiz)
        {
            data->minwidth = 6;
            data->minheight = 6;
            data->maxwidth = MUI_MAXMAX;
            data->maxheight = MUI_MAXMAX;
            data->defwidth = 50;
            data->defheight = 6;
        }
        else
        {
            data->minwidth = 6;
            data->minheight = 6;
            data->maxwidth = MUI_MAXMAX;
            data->maxheight = MUI_MAXMAX;
            data->defwidth = 6;
            data->defheight = 50;
        }

        struct MUI_AreaData *adata = muiAreaData(obj);
        struct MUI_ImageSpec_intern *spec = adata->mad_Background;

        if (spec)
        {
            if (spec->type == IST_BITMAP)
            {
                struct dt_node *node = spec->u.bitmap.dt;
                if (node)
                {
                    if (node->mode == MODE_PROP)
                    {
                        set(obj, MUIA_Frame, MUIV_Frame_None);

                        if (data->horiz)
                        {
                            data->minheight =
                                node->img_horizontalcontainer->h >> 1;
                            data->defheight = data->minheight;
                            data->maxheight = data->minheight;
                        }
                        else
                        {
                            data->minwidth =
                                node->img_verticalcontainer->w >> 1;
                            data->defwidth = data->minwidth;
                            data->maxwidth = data->minwidth;
                        }
                    }
                }
            }
        }
    }

    return 1;
}

IPTR Prop__MUIM_Cleanup(struct IClass *cl, Object *obj,
    struct MUIP_Cleanup *msg)
{
    struct Prop_DATA *data = INST_DATA(cl, obj);
    if (!data->usewinborder)
    {
        DoMethod(_win(obj), MUIM_Window_FreeGadgetID, data->gadgetid);
    }
    else
    {
        data->prop_object = NULL;
        data->gadgetid = 0;
    }

    DoMethod(_win(obj), MUIM_Window_RemEventHandler, (IPTR) & data->ehn);
    return DoSuperMethodA(cl, obj, (Msg) msg);
}

void WritePixelArrayAlphaToImage(struct NewImage *in, UWORD sx, UWORD sy,
    struct NewImage *out, UWORD xp, UWORD yp, UWORD w, UWORD h)
{
    ULONG argb, rgb;
    UBYTE rs, gs, bs, as, rd, gd, bd;
    UWORD r, g, b;
    UWORD x, y;
    for (y = 0; y < h; y++)
    {
        for (x = 0; x < w; x++)
        {
            argb = in->data[x + sx + (y + sy) * in->w];

            as = GET_A(argb);
            rs = GET_R(argb);
            gs = GET_G(argb);
            bs = GET_B(argb);
            // as = 255;
            rgb = out->data[x + xp + (y + yp) * out->w];

            rd = GET_R(rgb);
            gd = GET_G(rgb);
            bd = GET_B(rgb);


            r = ((rs * as) >> 8) + ((rd * (255 - as)) >> 8);
            g = ((gs * as) >> 8) + ((gd * (255 - as)) >> 8);
            b = ((bs * as) >> 8) + ((bd * (255 - as)) >> 8);

            out->data[x + xp + (y + yp) * out->w] = SET_ARGB(255, r, g, b);

        }
    }
}

int WriteTiledImageHorizontal(struct NewImage *dst, struct RastPort *maprp,
    struct NewImage *ni, int sx, int sy, int sw, int sh, int xp, int yp,
    int dw, int dh)
{
    int w = dw;
    int x = xp;
    int ddw;
    if ((sw == 0) || (dw == 0))
        return xp;

    while (w > 0)
    {
        ddw = sw;
        if (w < ddw)
            ddw = w;

        if (dst != NULL)
            WritePixelArrayAlphaToImage(ni, sx, sy, dst, x, yp, ddw, dh);
        if ((maprp != NULL) && (ni->bitmap != NULL))
        {
            if (ni->mask)
            {
                BltMaskBitMapRastPort(ni->bitmap, sx, sy, maprp, x, yp, ddw,
                    dh, 0xe0, (PLANEPTR) ni->mask);
            }
            else
                BltBitMapRastPort(ni->bitmap, sx, sy, maprp, x, yp, ddw, dh,
                    0xc0);
        }
        w -= ddw;
        x += ddw;
    }
    return x;
}

int WriteTiledImageVertical(struct NewImage *dst, struct RastPort *maprp,
    struct NewImage *ni, int sx, int sy, int sw, int sh, int xp, int yp,
    int dw, int dh)
{
    int h = dh;
    int y = yp;
    int ddh;
    if ((sh == 0) || (dh == 0))
        return yp;
    while (h > 0)
    {
        ddh = sh;
        if (h < ddh)
            ddh = h;

        if (dst != NULL)
            WritePixelArrayAlphaToImage(ni, sx, sy, dst, xp, y, dw, ddh);
        if ((maprp != NULL) && (ni->bitmap != NULL))
        {
            if (ni->mask)
            {
                BltMaskBitMapRastPort(ni->bitmap, sx, sy, maprp, xp, y, dw,
                    ddh, 0xe0, (PLANEPTR) ni->mask);
            }
            else
                BltBitMapRastPort(ni->bitmap, sx, sy, maprp, xp, y, dw, ddh,
                    0xc0);
        }
        h -= ddh;
        y += ddh;
    }
    return y;
}

#ifdef __AROS__
AROS_UFH3
    (void, CustomPropRenderFunc,
    AROS_UFHA(struct Hook *, h, A0),
    AROS_UFHA(Object *, obj, A2),
    AROS_UFHA(struct wdpDrawBorderPropKnob *, msg, A1))
{
    AROS_USERFUNC_INIT

    struct Prop_DATA *node = h->h_Data;
    struct dt_node *data = node->node;

    if (msg->MethodID == WDM_DRAW_BORDERPROPKNOB)
    {
        struct Window *window = msg->wdp_Window;
        struct NewImage *temp;
        struct RastPort *winrp = msg->wdp_RPort;
        struct RastPort *maprp;
        struct Gadget *gadget = msg->wdp_Gadget;
        struct Rectangle *r = msg->wdp_RenderRect;
        struct PropInfo *pi = ((struct PropInfo *)gadget->SpecialInfo);
        BOOL hit = (msg->wdp_Flags & WDF_DBPK_HIT) ? TRUE : FALSE;
        ULONG y, x;
        int size, is, pos;
        UWORD MinX, MinY, MaxX, MaxY, StartX, StartY;


        int i;

        temp = node->temp;
        maprp = node->tmprp;

        if (node->buffer)
            for (i = 0; i < (node->buffer->w * node->buffer->h); i++)
                node->temp->data[i] = node->buffer->data[i];
        if (node->mapbuffer)
            BltBitMap(node->mapbuffer, 0, 0, node->maptemp, 0, 0,
                node->propwidth, node->propheight, 0xc0, 0xff, NULL);


        r = msg->wdp_PropRect;
        StartX = r->MinX;
        StartY = r->MinY;
        MinX = 0;
        MinY = 0;
        MaxX = r->MaxX - r->MinX;
        MaxY = r->MaxY - r->MinY;

        if ((pi->Flags & FREEVERT) != 0)
        {
            is = data->img_verticalcontainer->w >> 1;
            if (window->Flags & WFLG_WINDOWACTIVE)
                pos = 0;
            else
                pos = is;
            y = MinY;
            size =
                MaxY - MinY - data->ContainerTop_s -
                data->ContainerBottom_s + 1;
            y = WriteTiledImageVertical(temp, maprp,
                data->img_verticalcontainer, pos, data->ContainerTop_o, is,
                data->ContainerTop_s, MinX, y, is, data->ContainerTop_s);
            if (size > 0)
                y = WriteTiledImageVertical(temp, maprp,
                    data->img_verticalcontainer, pos,
                    data->ContainerVertTile_o, is,
                    data->ContainerVertTile_s, MinX, y, is, size);
            y = WriteTiledImageVertical(temp, maprp,
                data->img_verticalcontainer, pos, data->ContainerBottom_o,
                is, data->ContainerBottom_s, MinX, y, is,
                data->ContainerBottom_s);
        }
        else if ((pi->Flags & FREEHORIZ) != 0)
        {
            is = data->img_horizontalcontainer->h >> 1;
            if (window->Flags & WFLG_WINDOWACTIVE)
                pos = 0;
            else
                pos = is;
            x = MinX;
            size =
                MaxX - MinX - data->ContainerLeft_s -
                data->ContainerRight_s + 1;
            x = WriteTiledImageHorizontal(temp, maprp,
                data->img_horizontalcontainer, data->ContainerLeft_o, pos,
                data->ContainerLeft_s, is, x, MinY, data->ContainerLeft_s,
                is);
            if (size > 0)
                x = WriteTiledImageHorizontal(temp, maprp,
                    data->img_horizontalcontainer, data->ContainerHorTile_o,
                    pos, data->ContainerHorTile_s, is, x, MinY, size, is);
            x = WriteTiledImageHorizontal(temp, maprp,
                data->img_horizontalcontainer, data->ContainerRight_o, pos,
                data->ContainerRight_s, is, x, MinY, data->ContainerRight_s,
                is);
        }

        r = msg->wdp_RenderRect;
        MinX = r->MinX - StartX;
        MinY = r->MinY - StartY;
        MaxX = r->MaxX - StartX;
        MaxY = r->MaxY - StartY;

        if ((pi->Flags & FREEVERT) != 0)
        {
            is = data->img_verticalknob->w / 3;
            if (hit)
                pos = is;
            else if (window->Flags & WFLG_WINDOWACTIVE)
                pos = 0;
            else
                pos = is * 2;
            y = MinY;
            size = MaxY - MinY - data->KnobTop_s - data->KnobBottom_s + 1;
            y = WriteTiledImageVertical(temp, maprp, data->img_verticalknob,
                pos, data->KnobTop_o, is, data->KnobTop_s, MinX, y, is,
                data->KnobTop_s);
            if (size > 0)
            {
                if (size > data->KnobVertGripper_s)
                {
                    size = size - data->KnobVertGripper_s;
                    int size_bak = size;
                    size = size / 2;
                    if (size > 0)
                        y = WriteTiledImageVertical(temp, maprp,
                            data->img_verticalknob, pos,
                            data->KnobTileTop_o, is, data->KnobTileTop_s,
                            MinX, y, is, size);
                    y = WriteTiledImageVertical(temp, maprp,
                        data->img_verticalknob, pos,
                        data->KnobVertGripper_o, is,
                        data->KnobVertGripper_s, MinX, y, is,
                        data->KnobVertGripper_s);
                    size = size_bak - size;
                    if (size > 0)
                        y = WriteTiledImageVertical(temp, maprp,
                            data->img_verticalknob, pos,
                            data->KnobTileBottom_o, is,
                            data->KnobTileBottom_s, MinX, y, is, size);
                }
                else
                {
                    y = WriteTiledImageVertical(temp, maprp,
                        data->img_verticalknob, pos, data->KnobTileTop_o,
                        is, data->KnobTileTop_s, MinX, y, is, size);
                }
            }
            y = WriteTiledImageVertical(temp, maprp, data->img_verticalknob,
                pos, data->KnobBottom_o, is, data->KnobBottom_s, MinX, y,
                is, data->KnobBottom_s);
        }
        else if ((pi->Flags & FREEHORIZ) != 0)
        {
            is = data->img_horizontalknob->h / 3;
            if (hit)
                pos = is;
            else if (window->Flags & WFLG_WINDOWACTIVE)
                pos = 0;
            else
                pos = is * 2;
            x = MinX;
            size = MaxX - MinX - data->KnobLeft_s - data->KnobRight_s + 1;
            x = WriteTiledImageHorizontal(temp, maprp,
                data->img_horizontalknob, data->KnobLeft_o, pos,
                data->KnobLeft_s, is, x, MinY, data->KnobLeft_s, is);
            if (size > 0)
            {
                if (size > data->KnobHorGripper_s)
                {
                    size = size - data->KnobHorGripper_s;
                    int size_bak = size;
                    size = size / 2;
                    if (size > 0)
                        x = WriteTiledImageHorizontal(temp, maprp,
                            data->img_horizontalknob, data->KnobTileLeft_o,
                            pos, data->KnobTileLeft_s, is, x, MinY, size,
                            is);
                    x = WriteTiledImageHorizontal(temp, maprp,
                        data->img_horizontalknob, data->KnobHorGripper_o,
                        pos, data->KnobHorGripper_s, is, x, MinY,
                        data->KnobHorGripper_s, is);
                    size = size_bak - size;
                    if (size > 0)
                        x = WriteTiledImageHorizontal(temp, maprp,
                            data->img_horizontalknob, data->KnobTileRight_o,
                            pos, data->KnobTileRight_s, is, x, MinY, size,
                            is);
                }
                else
                {
                    x = WriteTiledImageHorizontal(temp, maprp,
                        data->img_horizontalknob, data->KnobTileRight_o,
                        pos, data->KnobTileRight_s, is, x, MinY, size, is);
                }
            }
            x = WriteTiledImageHorizontal(temp, maprp,
                data->img_horizontalknob, data->KnobRight_o, pos,
                data->KnobRight_s, is, x, MinY, data->KnobRight_s, is);
        }

        if (node->temp)
            WritePixelArray(node->temp->data, 0, 0, node->temp->w * 4,
                winrp, _left(node->obj), _top(node->obj), node->temp->w,
                node->temp->h, RECTFMT_ARGB);
        if (node->maptemp)
            BltBitMapRastPort(node->maptemp, 0, 0, winrp, _left(node->obj),
                _top(node->obj), node->propwidth, node->propheight, 0xc0);

    }

    AROS_USERFUNC_EXIT
}
#endif

IPTR Prop__MUIM_Show(struct IClass *cl, Object *obj, struct MUIP_Show *msg)
{
    struct Prop_DATA *data = INST_DATA(cl, obj);
    struct MUI_AreaData *adata = muiAreaData(obj);
    struct MUI_ImageSpec_intern *spec = adata->mad_Background;

    ULONG rc = DoSuperMethodA(cl, obj, (Msg) msg);
#ifdef __AROS__
    data->dhook.h_Entry = (HOOKFUNC) CustomPropRenderFunc;
#endif
    if (!data->usewinborder)
    {
        BOOL isnewlook, completely_visible = TRUE;;

        if (_flags(obj) & MADF_INVIRTUALGROUP)
        {
            Object *wnd = NULL, *parent;

            get(obj, MUIA_WindowObject, &wnd);
            parent = obj;
            while (get(parent, MUIA_Parent, &parent))
            {
                if (!parent)
                    break;
                if (parent == wnd)
                    break;

                if (_flags(parent) & MADF_ISVIRTUALGROUP)
                {
                    if ((_mleft(obj) < _mleft(parent)) ||
                        (_mright(obj) > _mright(parent)) ||
                        (_mtop(obj) < _mtop(parent)) ||
                        (_mbottom(obj) > _mbottom(parent)))
                    {
                        completely_visible = FALSE;
                        D(bug("=== prop object: completely visible FALSE "
                            "for obj %x at %d,%d - %d,%d\n",
                            obj, _mleft(obj), _mtop(obj), _mright(obj),
                            _mbottom(obj)));
                        break;
                    }
                }
            }
        }

        if (completely_visible)
        {
            if (muiGlobalInfo(obj)->mgi_Prefs->scrollbar_type ==
                SCROLLBAR_TYPE_NEWLOOK)
                isnewlook = TRUE;
            else
                isnewlook = FALSE;

            ULONG width = _mwidth(obj);
            ULONG height = _mheight(obj);
#ifdef __AROS__
            struct Hook *dhook = NULL;

            ULONG depth =
                (ULONG) GetBitMapAttr(_window(obj)->RPort->BitMap,
                BMA_DEPTH);
            if (muiGlobalInfo(obj)->mgi_Prefs->scrollbar_type ==
                SCROLLBAR_TYPE_CUSTOM)
            {
                if (spec)
                {
                    if (spec->type == IST_BITMAP)
                    {
                        struct dt_node *node = spec->u.bitmap.dt;
                        if (node)
                        {
                            if (node->mode == MODE_PROP)
                            {
                                data->dhook.h_Entry =
                                    (HOOKFUNC) CustomPropRenderFunc;
                                data->node = node;
                                data->dhook.h_Data = data;
                                data->obj = obj;
                                dhook = &data->dhook;

                                if (data->horiz)
                                    height =
                                        node->img_horizontalcontainer->
                                        h >> 1;
                                else
                                    width =
                                        node->img_verticalcontainer->w >> 1;
                                DisposeImageContainer(data->buffer);
                                DisposeImageContainer(data->temp);
                                data->propwidth = width;
                                data->propheight = height;
                                if (data->mapbuffer)
                                    FreeBitMap(data->mapbuffer);
                                if (data->maptemp)
                                    FreeBitMap(data->maptemp);
                                if (depth >= 15)
                                {
                                    data->buffer =
                                        NewImageContainer(width, height);
                                    data->temp =
                                        NewImageContainer(width, height);
                                    data->mapbuffer = NULL;
                                    data->maptemp = NULL;
                                    if ((data->buffer == NULL)
                                        || (data->temp == NULL))
                                    {
                                        dhook = NULL;
                                        DisposeImageContainer(data->buffer);
                                        DisposeImageContainer(data->temp);
                                        data->buffer = NULL;
                                        data->temp = NULL;
                                    }
                                }
                                else
                                {
                                    data->temp = NULL;
                                    data->buffer = NULL;
                                    data->tmprp = NULL;
                                    data->mapbuffer =
                                        AllocBitMap(width, height, 1,
                                        BMF_MINPLANES,
                                        _window(obj)->RPort->BitMap);
                                    data->maptemp =
                                        AllocBitMap(width, height, 1,
                                        BMF_MINPLANES,
                                        _window(obj)->RPort->BitMap);
                                    data->tmprp = CreateRastPort();
                                    if ((data->mapbuffer == NULL)
                                        || (data->maptemp == NULL)
                                        || (data->tmprp == NULL))
                                    {
                                        dhook = NULL;
                                        if (data->mapbuffer)
                                            FreeBitMap(data->mapbuffer);
                                        if (data->maptemp)
                                            FreeBitMap(data->maptemp);
                                        if (data->tmprp)
                                            FreeRastPort(data->tmprp);
                                        data->mapbuffer = NULL;
                                        data->maptemp = NULL;
                                        data->tmprp = NULL;
                                    }
                                    else
                                        data->tmprp->BitMap = data->maptemp;
                                }
                            }
                        }
                    }
                }
            }
#endif

            if (data->prop_object)
            {
                RemoveGadget(_window(obj),
                    (struct Gadget *)data->prop_object);
                DisposeObject(data->prop_object);
            }

            if ((data->prop_object = NewObject(NULL, "propgclass",
                        GA_Left, _mleft(obj),
                        GA_Top, _mtop(obj),
                        GA_Width, width,
                        GA_Height, height, GA_ID, data->gadgetid,
#ifdef __AROS__
                        /* custom prop gadget implementation (not finished) */
                        PGA_DisplayHook, dhook,
#endif
                        PGA_Freedom, data->horiz ? FREEHORIZ : FREEVERT,
                        PGA_Total, downscale(data, data->entries),
                        PGA_Visible, downscale(data, data->visible),
                        PGA_Top, downscale(data, data->first),
                        PGA_NewLook, isnewlook,
                        PGA_Borderless, TRUE,
                        ICA_TARGET, ICTARGET_IDCMP,  /* for notification */
                        TAG_DONE)))
            {
                AddGadget(_window(obj), (struct Gadget *)data->prop_object,
                    ~0);
            }
        }
    }
    else if (!data->prop_object)
    {
        switch (data->usewinborder)
        {
        case MUIV_Prop_UseWinBorder_Right:
            data->prop_object = muiRenderInfo(obj)->mri_VertProp;
            if (data->prop_object)
            {
                /* Store pointer to this propclass object in
                   propgadget->UserData,
                   so that window class when receiving IDCMP_IDCMUPDATE from
                   arrow gadgets can notify propclass object */

                ((struct Gadget *)data->prop_object)->UserData = obj;
            }
            break;

        case MUIV_Prop_UseWinBorder_Bottom:
            data->prop_object = muiRenderInfo(obj)->mri_HorizProp;
            if (data->prop_object)
            {
                /* Store pointer to this propclass object in
                   propgadget->UserData,
                   so that window class when receiving IDCMP_IDCMUPDATE from
                   arrow gadgets can notify propclass object */

                ((struct Gadget *)data->prop_object)->UserData = obj;
            }
            break;
        }
        if (data->prop_object)
        {
            data->gadgetid = ((struct Gadget *)data->prop_object)->GadgetID;

            SetAttrs(data->prop_object, ICA_TARGET, NULL, TAG_DONE);
            if (SetGadgetAttrs((struct Gadget *)data->prop_object,
                    _window(obj), NULL, PGA_Top, downscale(data,
                        data->first), PGA_Visible, downscale(data,
                        data->visible), PGA_Total, downscale(data,
                        data->entries), TAG_DONE))
            {
                RefreshGList((struct Gadget *)data->prop_object,
                    _window(obj), NULL, 1);
            }
            SetAttrs(data->prop_object, ICA_TARGET, ICTARGET_IDCMP,
                TAG_DONE);
        }
    }

    return rc;
}

IPTR Prop__MUIM_Draw(struct IClass *cl, Object *obj,
    struct MUIP_Draw *msg)
{
    struct Prop_DATA *data = INST_DATA(cl, obj);

    /* No drawings if own border */
    if (data->usewinborder)
        return 0;

    DoSuperMethodA(cl, obj, (Msg) msg);

    if (!(msg->flags & MADF_DRAWOBJECT))
        return 1;

    if (data->buffer || data->mapbuffer)
    {
//       Object *p = NULL;
//         get(obj, MUIA_Parent, &p);
        //if (p)
        //    DoMethod(p, MUIM_DrawParentBackground, _left(obj), _top(obj),
        //    _width(obj), _height(obj), _left(obj), _top(obj), 0);
        //else 
        DoMethod(obj, MUIM_DrawParentBackground, _left(obj), _top(obj),
            _width(obj), _height(obj), _left(obj), _top(obj), 0);

        if (data->buffer)
            ReadPixelArray(data->buffer->data, 0, 0, data->buffer->w * 4,
                _rp(data->obj), _mleft(data->obj), _mtop(data->obj),
                data->buffer->w, data->buffer->h, RECTFMT_ARGB);
        if (data->mapbuffer)
            BltBitMap(_window(data->obj)->RPort->BitMap,
                _window(data->obj)->LeftEdge + _mleft(data->obj),
                _window(data->obj)->TopEdge + _mtop(data->obj),
                data->mapbuffer, 0, 0, data->propwidth, data->propheight,
                0xc0, 0xff, NULL);
    }

    if (data->prop_object)
        RefreshGList((struct Gadget *)data->prop_object, _window(obj), NULL,
            1);
    return 1;
}

IPTR Prop__MUIM_Hide(struct IClass *cl, Object *obj,
    struct MUIP_Hide *msg)
{
    struct Prop_DATA *data = INST_DATA(cl, obj);

    if (data->prop_object)
    {
        if (!data->usewinborder)
        {
            RemoveGadget(_window(obj), (struct Gadget *)data->prop_object);
            DisposeObject(data->prop_object);
            data->prop_object = NULL;
        }
    }

    return DoSuperMethodA(cl, obj, (Msg) msg);
}

IPTR Prop__MUIM_HandleEvent(struct IClass *cl, Object *obj,
    struct MUIP_HandleEvent *msg)
{
    struct Prop_DATA *data = INST_DATA(cl, obj);

    if (msg->imsg)
    {
        if (msg->imsg->Class == IDCMP_IDCMPUPDATE)
        {
            struct TagItem *tag;
            ULONG v;

            /* Check if we are meant */
            tag = FindTagItem(GA_ID, (struct TagItem *)msg->imsg->IAddress);
            if (!tag)
                return 0;
            if (tag->ti_Data != data->gadgetid)
                return 0;

            /* Check if we PGA_Top has really changed */
            tag =
                FindTagItem(PGA_Top, (struct TagItem *)msg->imsg->IAddress);
            if (!tag)
                return 0;
            v = upscale(data, tag->ti_Data);

            //kprintf("PROP_HandleEvent: PGA_Top %d upscaled %d entries %d\n",
            //    tag->ti_Data, v, data->entries);

            if ((v == data->first)
                && (msg->imsg->Qualifier & IEQUALIFIER_REPEAT))
                return 0;

            if ((LONG) v < 0)
                v = 0;

            SetAttrs(obj, MUIA_Prop_First, v, MUIA_Prop_OnlyTrigger, TRUE,
                MUIA_Prop_Release,
                ((msg->imsg->
                        Qualifier & IEQUALIFIER_REPEAT) ? FALSE : TRUE),
                TAG_DONE);
        }
    }

    return 0;
}

IPTR Prop__MUIM_Prop_Increase(struct IClass *cl, Object *obj,
    struct MUIP_Prop_Increase *msg)
{
    struct Prop_DATA *data = INST_DATA(cl, obj);
    LONG newfirst;

    newfirst = data->first + msg->amount * data->deltafactor;

    if (newfirst + data->visible > data->entries)
        newfirst = data->entries - data->visible;
    if (newfirst != data->first)
        set(obj, MUIA_Prop_First, newfirst);
    return 1;
}

IPTR Prop__MUIM_Prop_Decrease(struct IClass *cl, Object *obj,
    struct MUIP_Prop_Decrease *msg)
{
    struct Prop_DATA *data = INST_DATA(cl, obj);
    LONG newfirst;

    /* We cannot decrease if if are on the top */
    if (data->first <= 0)
        return 1;

    newfirst = data->first - msg->amount * data->deltafactor;

    if (newfirst < 0)
        set(obj, MUIA_Prop_First, 0);
    else if (newfirst != data->first)
        set(obj, MUIA_Prop_First, newfirst);
    return 1;
}


BOOPSI_DISPATCHER(IPTR, Prop_Dispatcher, cl, obj, msg)
{
    switch (msg->MethodID)
    {
    case OM_NEW:
        return Prop__OM_NEW(cl, obj, (struct opSet *)msg);
    case OM_DISPOSE:
        return Prop__OM_DISPOSE(cl, obj, msg);
    case OM_GET:
        return Prop__OM_GET(cl, obj, (struct opGet *)msg);
    case OM_SET:
        return Prop__OM_SET(cl, obj, (struct opSet *)msg);
    case MUIM_Setup:
        return Prop__MUIM_Setup(cl, obj, (APTR) msg);
    case MUIM_Cleanup:
        return Prop__MUIM_Cleanup(cl, obj, (APTR) msg);
    case MUIM_Show:
        return Prop__MUIM_Show(cl, obj, (APTR) msg);
    case MUIM_Hide:
        return Prop__MUIM_Hide(cl, obj, (APTR) msg);
    case MUIM_AskMinMax:
        return Prop__MUIM_AskMinMax(cl, obj, (APTR) msg);
    case MUIM_Draw:
        return Prop__MUIM_Draw(cl, obj, (APTR) msg);
    case MUIM_HandleEvent:
        return Prop__MUIM_HandleEvent(cl, obj, (APTR) msg);
    case MUIM_Prop_Decrease:
        return Prop__MUIM_Prop_Decrease(cl, obj, (APTR) msg);
    case MUIM_Prop_Increase:
        return Prop__MUIM_Prop_Increase(cl, obj, (APTR) msg);
    default:
        return DoSuperMethodA(cl, obj, msg);

    }
}
BOOPSI_DISPATCHER_END

const struct __MUIBuiltinClass _MUI_Prop_desc =
{
    MUIC_Prop,
    MUIC_Area,
    sizeof(struct Prop_DATA),
    (void *) Prop_Dispatcher
};
