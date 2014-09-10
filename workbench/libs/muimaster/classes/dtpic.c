/*
    Copyright © 2002-2009, The AROS Development Team. All rights reserved.
    $Id$
*/

/* Dtpic.mui. Source based on the one from MUIUndoc */

#define MUIMASTER_YES_INLINE_STDARG

#include <stdio.h>
#include <stdlib.h>

#include <graphics/gfx.h>
#include <graphics/view.h>
#include <clib/alib_protos.h>

#include <datatypes/datatypesclass.h>
#include <datatypes/pictureclass.h>
#include <cybergraphx/cybergraphics.h>

#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/cybergraphics.h>
#include <proto/utility.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <proto/datatypes.h>

#include <string.h>

/*  #define MYDEBUG 1 */
#include "debug.h"

#include "mui.h"
#include "muimaster_intern.h"
#include "support.h"
#include "support_classes.h"
#include "dtpic_private.h"

extern struct Library *MUIMasterBase;

#ifdef DataTypesBase
#undef DataTypesBase
#endif

#define DataTypesBase data->datatypesbase

static void killdto(struct Dtpic_DATA *data)
{
    data->bm = NULL;
    data->bmhd = NULL;

    if (data->dto)
    {
        DisposeDTObject(data->dto);
        data->dto = NULL;
    }

    if (data->datatypesbase)
    {
        CloseLibrary(data->datatypesbase);
    }
};

/*
 * We copy the filename, as the file is opened later in setup and
 * not at once.
 */
IPTR Dtpic__OM_NEW(struct IClass *cl, Object *obj, struct opSet *msg)
{
    obj = (Object *) DoSuperMethodA(cl, obj, (Msg) msg);

    if (obj)
    {
        struct Dtpic_DATA *data = INST_DATA(cl, obj);
        struct TagItem *tags = msg->ops_AttrList;
        struct TagItem *tag;

        while ((tag = NextTagItem(&tags)) != NULL)
        {
            switch (tag->ti_Tag)
            {
            case MUIA_Dtpic_Name:
                data->name = StrDup((char *)tag->ti_Data);
                break;
            }
        }
    }

    return (IPTR) obj;
}

IPTR setup_datatype(struct IClass *cl, Object *obj)
{
    struct Dtpic_DATA *data = INST_DATA(cl, obj);

    if (data->dto)
        killdto(data);          /* Object already existed */

    if (data->name)
    {
        if ((data->datatypesbase = OpenLibrary("datatypes.library", 39)))
        {
            /* Prevent DOS Requesters from showing up */

            struct Process *me = (struct Process *)FindTask(0);
            APTR oldwinptr = me->pr_WindowPtr;

            me->pr_WindowPtr = (APTR) - 1;

            data->dto = NewDTObject(data->name, DTA_GroupID, GID_PICTURE,
                OBP_Precision, PRECISION_IMAGE,
                PDTA_Screen, _screen(obj),
                PDTA_DestMode, PMODE_V43,
                PDTA_UseFriendBitMap, TRUE, TAG_DONE);
            me->pr_WindowPtr = oldwinptr;

            if (data->dto)
            {
                struct FrameInfo fri = { 0 };

                DoMethod(data->dto, DTM_FRAMEBOX, 0, &fri, &fri,
                    sizeof(struct FrameInfo), 0);

                if (fri.fri_Dimensions.Depth > 0)
                {
                    if (DoMethod(data->dto, DTM_PROCLAYOUT, 0, 1))
                    {
                        get(data->dto, PDTA_BitMapHeader, &data->bmhd);

                        if (data->bmhd)
                        {
                            if (data->bmhd->bmh_Masking != mskNone)
                                set(obj, MUIA_FillArea, TRUE);
                            else
                                set(obj, MUIA_FillArea, FALSE);

                            GetDTAttrs(data->dto, PDTA_DestBitMap,
                                &data->bm, TAG_DONE);

                            if (!data->bm)
                            {
                                GetDTAttrs(data->dto, PDTA_BitMap,
                                    &data->bm, TAG_DONE);
                            }

                            if (data->bm)
                                return TRUE;
                        }
                    }
                }
            }
        }
    }
    killdto(data);

    return TRUE;
}

IPTR Dtpic__MUIM_Setup(struct IClass *cl, Object *obj,
    struct MUIP_Setup *msg)
{
    if (!DoSuperMethodA(cl, obj, (Msg) msg))
        return FALSE;

    return setup_datatype(cl, obj);
}

IPTR Dtpic__MUIM_Cleanup(struct IClass *cl, Object *obj,
    struct MUIP_Cleanup *msg)
{
    struct Dtpic_DATA *data = INST_DATA(cl, obj);

    killdto(data);

    return DoSuperMethodA(cl, obj, (Msg) msg);
}

IPTR Dtpic__MUIM_AskMinMax(struct IClass *cl, Object *obj,
    struct MUIP_AskMinMax *msg)
{
    struct Dtpic_DATA *data = INST_DATA(cl, obj);
    IPTR retval;

    retval = DoSuperMethodA(cl, obj, (Msg) msg);

    if (data->bm)
    {
        msg->MinMaxInfo->MinWidth += data->bmhd->bmh_Width;
        msg->MinMaxInfo->MinHeight += data->bmhd->bmh_Height;
        msg->MinMaxInfo->DefWidth += data->bmhd->bmh_Width;
        msg->MinMaxInfo->DefHeight += data->bmhd->bmh_Height;
        msg->MinMaxInfo->MaxWidth += data->bmhd->bmh_Width;
        msg->MinMaxInfo->MaxHeight += data->bmhd->bmh_Height;
    }

    return retval;
}

IPTR Dtpic__MUIM_Draw(struct IClass *cl, Object *obj,
    struct MUIP_Draw *msg)
{
    struct Dtpic_DATA *data = INST_DATA(cl, obj);

    DoSuperMethodA(cl, obj, (Msg) msg);

    if ((msg->flags & MADF_DRAWOBJECT) && data->bm)
    {
        /* Note: codes taken from picture.datatype GM_RENDER routine */
        ULONG depth = (ULONG) GetBitMapAttr(_rp(obj)->BitMap, BMA_DEPTH);

        if ((depth >= 15) && (data->bmhd->bmh_Masking == mskHasAlpha))
        {
            /* Transparency on high color rast port with alpha channel in 
             * picture */
            ULONG *img =
                AllocVec(_mwidth(obj) * _mheight(obj) * 4, MEMF_ANY);
            if (img)
            {
                struct pdtBlitPixelArray pa;
                pa.MethodID = PDTM_READPIXELARRAY;
                pa.pbpa_PixelData = (UBYTE *) img;
                pa.pbpa_PixelFormat = PBPAFMT_ARGB;
                pa.pbpa_PixelArrayMod = _mwidth(obj) * 4;
                pa.pbpa_Left = 0;
                pa.pbpa_Top = 0;
                pa.pbpa_Width = _mwidth(obj);
                pa.pbpa_Height = _mheight(obj);
                if (DoMethodA(data->dto, (Msg) & pa))
                    WritePixelArrayAlpha(img, 0, 0, _mwidth(obj) * 4,
                        _rp(obj), _mleft(obj), _mtop(obj), _mwidth(obj),
                        _mheight(obj), 0xffffffff);
                FreeVec((APTR) img);
            }
        }
        else
        {
            if (data->bmhd->bmh_Masking == mskHasMask)
            {
                /* Transparency with mask */
                APTR mask = NULL;

                GetDTAttrs(data->dto, PDTA_MaskPlane, (IPTR) & mask,
                    TAG_DONE);

                if (mask)
                    BltMaskBitMapRastPort(data->bm, 0, 0, _rp(obj),
                        _mleft(obj), _mtop(obj), _mwidth(obj),
                        _mheight(obj), 0xE0, (PLANEPTR) mask);
            }
            else
            {
                /* All other cases */
                BltBitMapRastPort(data->bm, 0, 0, _rp(obj), _mleft(obj),
                    _mtop(obj), _mwidth(obj), _mheight(obj), 0xC0);
            }
        }
    }

    return 0;
}

IPTR Dtpic__OM_DISPOSE(struct IClass *cl, Object *obj, Msg msg)
{
    struct Dtpic_DATA *data = INST_DATA(cl, obj);

    if (data->name)
        FreeVec(data->name);

    return DoSuperMethodA(cl, obj, msg);
}

IPTR Dtpic__OM_SET(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct Dtpic_DATA *data = INST_DATA(cl, obj);
    struct TagItem *tags = msg->ops_AttrList;
    struct TagItem *tag;
    ULONG needs_redraw = 0;

    while ((tag = NextTagItem(&tags)) != NULL)
    {
        switch (tag->ti_Tag)
        {
        case MUIA_Dtpic_Name:
            /* If no filename or different filenames */
            if (!data->name || strcmp(data->name, (char *)tag->ti_Data))
            {
                if (data->name)
                    FreeVec(data->name);
                data->name =
                    AllocVec(strlen((char *)tag->ti_Data) + 1, MEMF_ANY);
                strcpy((char *)data->name, (char *)tag->ti_Data);

                /* Run immediate setup only if base class is setup up */
                if (_flags(obj) & MADF_SETUP)
                    setup_datatype(cl, obj);
                needs_redraw = 1;
            }
            break;
        }
    }

    if (needs_redraw)
    {
        MUI_Redraw(obj, MADF_DRAWOBJECT);
    }

    return DoSuperMethodA(cl, obj, (Msg) msg);
}

IPTR Dtpic__OM_GET(struct IClass *cl, Object *obj, struct opGet *msg)
{
    struct Dtpic_DATA *data = INST_DATA(cl, obj);

    switch (msg->opg_AttrID)
    {
    case MUIA_Dtpic_Name:
        *(msg->opg_Storage) = (IPTR) data->name;
        return TRUE;
    }

    return DoSuperMethodA(cl, obj, (Msg) msg);
}

#if ZUNE_BUILTIN_DTPIC
BOOPSI_DISPATCHER(IPTR, Dtpic_Dispatcher, cl, obj, msg)
{
    switch (msg->MethodID)
    {
    case OM_NEW:
        return Dtpic__OM_NEW(cl, obj, (struct opSet *)msg);
    case MUIM_Setup:
        return Dtpic__MUIM_Setup(cl, obj, (struct MUIP_Setup *)msg);
    case MUIM_Cleanup:
        return Dtpic__MUIM_Cleanup(cl, obj, (struct MUIP_Clean *)msg);
    case MUIM_AskMinMax:
        return Dtpic__MUIM_AskMinMax(cl, obj, (struct MUIP_AskMinMax *)msg);
    case MUIM_Draw:
        return Dtpic__MUIM_Draw(cl, obj, (struct MUIP_Draw *)msg);
    case OM_DISPOSE:
        return Dtpic__OM_DISPOSE(cl, obj, msg);
    case OM_SET:
        return Dtpic__OM_SET(cl, obj, (struct opSet *)msg);
    case OM_GET:
        return Dtpic__OM_GET(cl, obj, (struct opGet *)msg);
    default:
        return DoSuperMethodA(cl, obj, msg);
    }
}
BOOPSI_DISPATCHER_END

const struct __MUIBuiltinClass _MUI_Dtpic_desc =
{
    MUIC_Dtpic,
    MUIC_Area,
    sizeof(struct Dtpic_DATA),
    (void *) Dtpic_Dispatcher
};
#endif /* ZUNE_BUILTIN_DTPIC */
