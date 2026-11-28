/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: BOOPSI image classes showing icon.library's default icons.
*/

/*
    This file is the implementation shared by a set of external image
    classes (Disk.image, Drawer.image, ...) for use in MUI image
    specifications ("3:Disk.image"). Each of them shows one of the
    default icons of icon.library, scaled to the area of the image, so
    that applications show the same imagery for disks, drawers and so
    on as the desktop does.

    It is included by the source file of every class, which defines

        ICONIMAGE_NAMES     names of default icons to use, in order of
                            preference, as a list of strings
        ICONIMAGE_TYPE      the icon type used if none of them exists
*/

#include <exec/types.h>
#include <exec/libraries.h>
#include <intuition/intuition.h>
#include <intuition/classes.h>
#include <intuition/classusr.h>
#include <intuition/imageclass.h>
#include <intuition/screens.h>
#include <graphics/rastport.h>
#include <graphics/text.h>
#include <workbench/workbench.h>
#include <workbench/icon.h>
#include <libraries/mui.h>

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/icon.h>
#include <proto/utility.h>

#include <clib/alib_protos.h>
#include <aros/asmcall.h>

#include "iconimage_intern.h"

#define ICONIMAGE_MINSIZE   8

static const CONST_STRPTR iconimage_names[] =
{
    ICONIMAGE_NAMES, NULL
};

static struct DiskObject *IconImage_LoadIcon(void)
{
    struct DiskObject *icon = NULL;
    int i;

    for (i = 0; !icon && iconimage_names[i]; i++)
    {
        icon = GetIconTags(NULL,
            ICONGETA_GetDefaultName, (IPTR) iconimage_names[i],
            ICONGETA_FailIfUnavailable, TRUE,
            TAG_DONE);
    }

    if (!icon)
    {
        icon = GetIconTags(NULL,
            ICONGETA_GetDefaultType, ICONIMAGE_TYPE,
            TAG_DONE);
    }

    return icon;
}

/* Prepares the icon for an area of the given size on the screen */
static void IconImage_Layout(struct IconImageData *data, WORD width,
    WORD height)
{
    ULONG box = PACK_ICON_SCALEBOX(width, height);
    struct Screen *scr = data->iid_Screen;
    struct Screen *locked = NULL;

    if (data->iid_LaidOut && (data->iid_ScaleBox == box))
        return;

    if (!scr)
        scr = locked = LockPubScreen(NULL);

    if (scr)
    {
        IconControl(data->iid_Icon, ICONCTRLA_SetScaleBox, box, TAG_DONE);
        LayoutIcon(data->iid_Icon, scr, TAG_DONE);
        data->iid_ScaleBox = box;
        data->iid_LaidOut = TRUE;
    }

    if (locked)
        UnlockPubScreen(NULL, locked);
}

static IPTR IconImage_New(Class *cl, Object *obj, struct opSet *msg)
{
    struct IconImageData *data;

    obj = (Object *) DoSuperMethodA(cl, obj, (Msg) msg);
    if (!obj)
        return 0;

    data = INST_DATA(cl, obj);
    data->iid_Screen = NULL;
    data->iid_LaidOut = FALSE;
    data->iid_Icon = IconImage_LoadIcon();

    if (!data->iid_Icon)
    {
        CoerceMethod(cl, obj, OM_DISPOSE);
        return 0;
    }

    return (IPTR) obj;
}

static IPTR IconImage_Dispose(Class *cl, Object *obj, Msg msg)
{
    struct IconImageData *data = INST_DATA(cl, obj);

    if (data->iid_Icon)
        FreeDiskObject(data->iid_Icon);

    return DoSuperMethodA(cl, obj, msg);
}

static IPTR IconImage_Draw(Class *cl, Object *obj, struct impDraw *msg)
{
    struct IconImageData *data = INST_DATA(cl, obj);
    struct Image *im = (struct Image *) obj;
    struct Rectangle rect;
    WORD width = im->Width;
    WORD height = im->Height;
    WORD iw, ih;

    if (msg->MethodID == IM_DRAWFRAME)
    {
        width = msg->imp_Dimensions.Width;
        height = msg->imp_Dimensions.Height;
    }

    if (!msg->imp_RPort || (width <= 0) || (height <= 0))
        return 0;

    IconImage_Layout(data, width, height);

    /* centre the icon in the area */
    iw = width;
    ih = height;
    if (GetIconRectangle(msg->imp_RPort, data->iid_Icon, NULL, &rect,
            ICONDRAWA_Frameless, TRUE,
            ICONDRAWA_Borderless, TRUE,
            TAG_DONE))
    {
        iw = rect.MaxX - rect.MinX + 1;
        ih = rect.MaxY - rect.MinY + 1;
    }

    DrawIconState(msg->imp_RPort, data->iid_Icon, NULL,
        im->LeftEdge + msg->imp_Offset.X + ((width - iw) / 2),
        im->TopEdge + msg->imp_Offset.Y + ((height - ih) / 2),
        IDS_NORMAL,
        ICONDRAWA_Frameless, TRUE,
        ICONDRAWA_Borderless, TRUE,
        ICONDRAWA_EraseBackground, FALSE,
        msg->imp_DrInfo ? ICONDRAWA_DrawInfo : TAG_IGNORE,
            (IPTR) msg->imp_DrInfo,
        TAG_DONE);

    return 0;
}

/* Tells MUI how much room the image wants */
static IPTR IconImage_Query(Class *cl, Object *obj,
    struct MUI_BoopsiQuery *msg)
{
    struct IconImageData *data = INST_DATA(cl, obj);
    LONG size = 16;

    data->iid_Screen = msg->mbq_Screen;
    data->iid_LaidOut = FALSE;

    /* as tall as a line of text, so it fits list entries and labels */
    if (msg->mbq_Screen && msg->mbq_Screen->RastPort.Font)
        size = msg->mbq_Screen->RastPort.Font->tf_YSize;
    if (size < ICONIMAGE_MINSIZE)
        size = ICONIMAGE_MINSIZE;

    msg->mbq_MinWidth = ICONIMAGE_MINSIZE;
    msg->mbq_MinHeight = ICONIMAGE_MINSIZE;
    msg->mbq_DefWidth = size;
    msg->mbq_DefHeight = size;
    msg->mbq_MaxWidth = MBQ_MUI_MAXMAX;
    msg->mbq_MaxHeight = MBQ_MUI_MAXMAX;

    return MUIM_BoopsiQuery;
}

BOOPSI_DISPATCHER(IPTR, IconImage_Dispatcher, cl, obj, msg)
{
    switch (msg->MethodID)
    {
    case OM_NEW:
        return IconImage_New(cl, obj, (struct opSet *) msg);

    case OM_DISPOSE:
        return IconImage_Dispose(cl, obj, msg);

    case IM_DRAW:
    case IM_DRAWFRAME:
        return IconImage_Draw(cl, obj, (struct impDraw *) msg);

    case MUIM_BoopsiQuery:
        return IconImage_Query(cl, obj, (struct MUI_BoopsiQuery *) msg);
    }

    return DoSuperMethodA(cl, obj, msg);
}
BOOPSI_DISPATCHER_END
