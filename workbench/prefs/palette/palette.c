/*
    Copyright © 2020, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/debug.h>

#include <clib/alib_protos.h>

#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/utility.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <proto/colorwheel.h>

#include <graphics/gfx.h>
#include <graphics/view.h>

#include <utility/hooks.h>
#include <gadgets/colorwheel.h>
#include <gadgets/gradientslider.h>

#include <libraries/mui.h>
#include <zune/customclasses.h>

#include <string.h>
#include <stdio.h>

#include "locale.h"
#include "paleditor.h"
#include "clutcolor.h"
#include "prefs.h"

#define ColorWheelBase data->colorwheelbase

/*** Instance data **********************************************************/
struct PEPalette_DATA
{
    const char                  **names;
    struct  MUI_Palette_Entry   *entries;
    Object                      *list, *coloradjust;
    ULONG                       numentries;
    Object                      *colorfiledgrp;
    Object                      **colorfieldentries;
    ULONG                       *penmap;
    ULONG                       lastindex;
    ULONG                       group;
    ULONG                       rgb[3];
    struct Hook                 display_hook;
    struct Hook                 setcolor_hook;
    char                        buf[20];
};

/* private msg structure passed to the hook function */
struct MUIP_PalNotifyMsg
{
    STACKED struct PEPalette_DATA *palData;
    STACKED ULONG palMode;
    STACKED ULONG palVal;
};

static LONG display_func(struct Hook *hook, char **array,
    struct MUI_Palette_Entry *entry)
{
    struct PEPalette_DATA *data = hook->h_Data;

    /* do any strings exist? */
    if (data->names)
    {
        /* then display user names */
        *array = (char *)data->names[(SIPTR) array[-1]];
        array++;
    }
    else
    {
        /* if no, show default color names */
        sprintf(data->buf, "Color %ld", (long)(array[-1] + 1));
        *array++ = data->buf;
    }
    return 0;
}

static void NotifyGun(Object * self, struct PEPalette_DATA *data, LONG gun)
{
    static Tag guntotag[3] = {
        MUIA_Coloradjust_Red,
        MUIA_Coloradjust_Green,
        MUIA_Coloradjust_Blue
    };

    struct TagItem tags[] = {
        { MUIA_NoNotify, TRUE},
        { 0, 0},
        { MUIA_Coloradjust_RGB, 0},
        { TAG_DONE}
    };

    tags[1].ti_Tag = guntotag[gun];
    tags[1].ti_Data = data->rgb[gun];
    tags[2].ti_Data = (IPTR) data->rgb;

    SetAttrsA(self, tags);
}

static LONG setcolor_func(struct Hook *hook, APTR * self, struct MUIP_PalNotifyMsg *msg)
{
    struct PEPalette_DATA *data = msg->palData;
    ULONG mode = msg->palMode;
    ULONG val = msg->palVal;

    LONG entry = XGET(data->list, MUIA_List_Active);
    if ((entry < 0) || (entry >= data->numentries))
        return 0;
    if (mode == 1)
    {
        D(bug("[PaletteEditor:Palette] %s: pen = %d\n", __func__, entry);)

        if (data->numentries > 0)
        {
            struct MUIP_PalNotifyMsg changepenmsg;

            changepenmsg.palData = data;
            changepenmsg.palMode = 3;
            changepenmsg.palVal = data->penmap[entry];
            setcolor_func(hook, self, &changepenmsg);
        }
    }
    else if (mode == 2)
    {
        D(bug("[PaletteEditor:Palette] %s: colorindex = %d\n", __func__, data->penmap[entry]);)

        data->entries[data->penmap[entry]].mpe_Red =
            XGET(data->coloradjust, MUIA_Coloradjust_Red);
        data->entries[data->penmap[entry]].mpe_Green =
            XGET(data->coloradjust, MUIA_Coloradjust_Green);
        data->entries[data->penmap[entry]].mpe_Blue =
            XGET(data->coloradjust, MUIA_Coloradjust_Blue);

        SetRGB32(&_screen(self)->ViewPort,
            data->penmap[entry],
            data->entries[data->penmap[entry]].mpe_Red,
            data->entries[data->penmap[entry]].mpe_Green,
            data->entries[data->penmap[entry]].mpe_Blue);
        if (GetBitMapAttr(_rp(self)->BitMap, BMA_DEPTH) > 8)
        {
            Object *winObj = _win(self);
            MUI_Redraw((Object *)XGET(winObj, MUIA_Window_RootObject), MADF_DRAWOBJECT);
        }
    }
    else if (mode == 3)
    {
        ULONG r;
        ULONG g;
        ULONG b;

        data->penmap[entry] = val;

        D(bug("[PaletteEditor:Palette] %s: pen %d = colorindex %d\n", __func__, entry, val);)

        r = data->entries[val].mpe_Red;
        g = data->entries[val].mpe_Green;
        b = data->entries[val].mpe_Blue;

        NNSET(data->coloradjust, MUIA_Coloradjust_Red, r);
        NNSET(data->coloradjust, MUIA_Coloradjust_Green, g);
        NNSET(data->coloradjust, MUIA_Coloradjust_Blue, b);
        data->rgb[0] = r;
        data->rgb[1] = g;
        data->rgb[2] = b;
        NotifyGun(data->coloradjust, data, 0);
        NotifyGun(data->coloradjust, data, 1);
        NotifyGun(data->coloradjust, data, 2);

        if (DoMethod(data->colorfiledgrp, MUIM_Group_InitChange))
        {
            if (data->lastindex != val)
            {
                SET(data->colorfieldentries[data->lastindex], MUIA_Frame, MUIV_Frame_None);
            }
            SET(data->colorfieldentries[val], MUIA_Frame, MUIV_Frame_ReadList);
            DoMethod(data->colorfiledgrp, MUIM_Group_ExitChange);
        }
        data->lastindex = val;
    }
    return 0;
}

IPTR PEPalette__OM_NEW(Class *CLASS, Object *self, struct opSet * msg)
{
    struct PEPalette_DATA *data;
    //struct TagItem                  *tag, *tags;
    struct MUI_Palette_Entry *e = (struct MUI_Palette_Entry *)GetTagData(MUIA_Palette_Entries, 0, msg->ops_AttrList);
    Object *list, *coloradjust;
    int i, c = 0;

    int collcount, rowcount;
    Object *clutcGrpObj;

    if (e)
    {
        while (e->mpe_ID != MUIV_Palette_Entry_End)
        {
            c++;
            e++;
        }
    }

    D(bug("[PaletteEditor:Palette] %s: count = %d\n", __func__, c);)

    collcount = 4;
    rowcount = c / 4;

    D(bug("[PaletteEditor:Palette] %s: %d rows, %d colls\n", __func__, rowcount, collcount);)

    Object *clutcObjs[c];

    clutcGrpObj = GroupObject,
                MUIA_Group_Columns, collcount,
                TextFrame,
                MUIA_Group_Spacing, 1,
                MUIA_Weight, 20,
            End,

    D(bug("[PaletteEditor:Palette] %s: grp obj @ 0x%p\n", __func__, clutcGrpObj);)

    self = (Object *) DoSuperNewTags(CLASS, self, NULL,
        GroupFrame,
        MUIA_Background, MUII_ButtonBack,
        MUIA_Group_Horiz, TRUE,
        Child, (IPTR)(ListviewObject,
                MUIA_Listview_List, (IPTR)(list = ListObject,
            End),
        End),
        Child, VGroup,
            Child, (IPTR)clutcGrpObj,
            Child, (IPTR)(coloradjust = ColoradjustObject,
            End),
        End,
        TAG_MORE, (IPTR) msg->ops_AttrList);

    if (self == NULL)
        return (IPTR) NULL;

    data = INST_DATA(CLASS, self);

    data->list = list;
    data->coloradjust = coloradjust;
    data->colorfiledgrp = clutcGrpObj;

    data->display_hook.h_Entry = HookEntry;
    data->display_hook.h_SubEntry = (HOOKFUNC) display_func;
    data->display_hook.h_Data = data;
    data->setcolor_hook.h_Entry = HookEntry;
    data->setcolor_hook.h_SubEntry = (HOOKFUNC) setcolor_func;
    data->setcolor_hook.h_Data = data;

    NNSET(list, MUIA_List_DisplayHook, (IPTR) & data->display_hook);

    /* create the clut colorfield objects ...*/
    D(bug("[PaletteEditor:Palette] %s: creating clut objects ...\n", __func__);)
    for (i = 0; i < rowcount; i++)
    {
        for(c = 0; c < collcount; c++)
        {
            clutcObjs[(i * collcount) + c] = NewObject(CLUTColor_CLASS->mcc_Class, NULL,
                    MUIA_Frame, (c + i == 0) ? MUIV_Frame_ReadList : MUIV_Frame_None,
                    MUIA_CLUTColor_Index, (i * collcount) + c,
                    MUIA_InputMode, MUIV_InputMode_RelVerify,
                TAG_DONE);
            D(bug("[PaletteEditor:Palette] %s: ca obj #%d @ 0x%p\n", __func__, (i * collcount) + c, clutcObjs[(i * collcount) + c]);)
            DoMethod(clutcGrpObj, OM_ADDMEMBER, clutcObjs[(i * collcount) + c]);
        }
    }

    data->numentries = sizeof(clutcObjs) / sizeof(Object *);
    D(bug("[PaletteEditor:Palette] %s: %d clut entries\n", __func__, data->numentries);)

    data->entries = e;
    data->names =
        (const char **)GetTagData(MUIA_Palette_Names, 0, msg->ops_AttrList);
    data->group = GetTagData(MUIA_Palette_Groupable, 0, msg->ops_AttrList);

    if (data->numentries > 0)
    {
        data->penmap = AllocMem(data->numentries * sizeof(ULONG), MEMF_CLEAR);
        data->colorfieldentries = AllocMem(data->numentries * sizeof(Object *), MEMF_ANY);
        CopyMem(clutcObjs, data->colorfieldentries, data->numentries * sizeof(Object *));
        for (i = 0; i < data->numentries; i++)
        {
            DoMethod(data->list, MUIM_List_InsertSingle, &data->entries[i],
                MUIV_List_Insert_Bottom);
            DoMethod(clutcObjs[i], MUIM_Notify, MUIA_Pressed, FALSE,
                (IPTR) self, 5, MUIM_CallHook, (IPTR) &data->setcolor_hook,
                (IPTR) data, 3, i);
            D(bug("[PaletteEditor:Palette] %s: id #%d\n", __func__, data->entries[i].mpe_ID);)
            data->penmap[data->entries[i].mpe_ID] = i;
        }

        NNSET(data->coloradjust, MUIA_Coloradjust_Red,
            data->entries[0].mpe_Red);
        NNSET(data->coloradjust, MUIA_Coloradjust_Green,
            data->entries[0].mpe_Green);
        NNSET(data->coloradjust, MUIA_Coloradjust_Blue,
            data->entries[0].mpe_Blue);
        NNSET(data->list, MUIA_List_Active, 0);

        data->rgb[0] = data->entries[0].mpe_Red;
        data->rgb[1] = data->entries[0].mpe_Green;
        data->rgb[2] = data->entries[0].mpe_Blue;
    }

    DoMethod(data->list, MUIM_Notify, MUIA_List_Active, MUIV_EveryTime,
        (IPTR) self, 5, MUIM_CallHook, (IPTR) &data->setcolor_hook,
        (IPTR) data, 1, 0);

    DoMethod(data->coloradjust, MUIM_Notify, MUIA_Coloradjust_Red, MUIV_EveryTime,
        (IPTR) self, 5, MUIM_CallHook, (IPTR) &data->setcolor_hook,
        (IPTR) data, 2, 0);
    DoMethod(data->coloradjust, MUIM_Notify, MUIA_Coloradjust_Green, MUIV_EveryTime,
        (IPTR) self, 5, MUIM_CallHook, (IPTR) &data->setcolor_hook,
        (IPTR) data, 2, 1);
    DoMethod(data->coloradjust, MUIM_Notify, MUIA_Coloradjust_Blue, MUIV_EveryTime,
        (IPTR) self, 5, MUIM_CallHook, (IPTR) &data->setcolor_hook,
        (IPTR) data, 2, 2);

    return (IPTR) self;
}

IPTR PEPalette__OM_SET(Class *CLASS, Object *self, struct opSet * msg)
{
    struct PEPalette_DATA *data;
    struct TagItem *tag, *tags;

    data = INST_DATA(CLASS, self);

    for (tags = msg->ops_AttrList; (tag = NextTagItem(&tags));)
    {
        switch (tag->ti_Tag)
        {
        case MUIA_Palette_Entries:
            data->entries = (struct MUI_Palette_Entry *)tag->ti_Data;
            break;

        case MUIA_Palette_Names:
            data->names = (const char **)tag->ti_Data;
            break;

        case MUIA_Palette_Groupable:
            data->group = (ULONG) tag->ti_Data;
            break;
        }
    }
    return DoSuperMethodA(CLASS, self, (Msg) msg);
}

IPTR PEPalette__OM_GET(Class *CLASS, Object *self, struct opGet * msg)
{
    struct PEPalette_DATA *data = INST_DATA(CLASS, self);

    IPTR *store = msg->opg_Storage;

    switch (msg->opg_AttrID)
    {
    case MUIA_Palette_Entries:
        *store = (IPTR) data->entries;
        return TRUE;
    case MUIA_Palette_Names:
        *store = (IPTR) data->names;
        return TRUE;

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
        *store = (IPTR) data->rgb;
        return TRUE;
    }
    return DoSuperMethodA(CLASS, self, (Msg) msg);
}

IPTR PEPalette__OM_DISPOSE(Class *CLASS, Object *self, Msg msg)
{
    return DoSuperMethodA(CLASS, self, msg);
}

ZUNE_CUSTOMCLASS_4
(
    PEPalette, NULL, MUIC_Group, NULL,
    OM_NEW,                     struct opSet *,
    OM_DISPOSE,                 Msg,
    OM_SET,                     struct opSet *,
    OM_GET,                     struct opGet *
);
