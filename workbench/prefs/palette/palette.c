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

#define COLUMNCOUNT     4

#define MUIM_PEPalette_Init    (TAG_USER | 0x1001)

CONST_STRPTR PEModeStrings[] =
{
    NULL,
    NULL,
};

/*** Instance data **********************************************************/
struct PEPalette_DATA
{
    const char                  **names;
    struct  MUI_Palette_Entry   *entries;
    Object                      *list, *coloradjust;
    ULONG                       numentries;
    Object                      *colorfiledgrp;
    Object                      **colorfieldentries;
    Object                      *palgrp;
    Object                      *palmode;
    ULONG                       *penmap;
    ULONG                       *penmap4;
    ULONG                       *penmap8;
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
    else if (mode == 4)
    {
        IPTR penmode = 0;

        D(bug("[PaletteEditor:Palette] %s: cycle gadget\n", __func__);)
        GET(data->palmode, MUIA_Cycle_Active, &penmode);
        if (DoMethod(data->colorfiledgrp, MUIM_Group_InitChange))
        {
            SET(data->colorfieldentries[4], MUIA_ShowMe, (penmode == 0) ? FALSE : TRUE );
            SET(data->colorfieldentries[5], MUIA_ShowMe, (penmode == 0) ? FALSE : TRUE );
            SET(data->colorfieldentries[6], MUIA_ShowMe, (penmode == 0) ? FALSE : TRUE );
            SET(data->colorfieldentries[7], MUIA_ShowMe, (penmode == 0) ? FALSE : TRUE );
            DoMethod(data->colorfiledgrp, MUIM_Group_ExitChange);
        }
        data->penmap = (penmode == 0) ? data->penmap4 : data->penmap8;
    }
    return 0;
}

VOID initPenMap(ULONG *penmap, ULONG size)
{
    int i;
    for (i = 0; i < size; i ++)
    {
        if (i == 5)
            penmap[i] = 3;
        else if (i == 1 || i == 7)
            penmap[i] = 0;
        else if (i == 3 || i == 8 || i == 10 || i == 12)
            penmap[i] = 2;            
        else
            penmap[i] = 1;
    }
}

IPTR PEPalette__OM_NEW(Class *CLASS, Object *self, struct opSet * msg)
{
    struct PEPalette_DATA *data;
    struct MUI_Palette_Entry *e = (struct MUI_Palette_Entry *)GetTagData(MUIA_Palette_Entries, 0, msg->ops_AttrList);
    Object *list, *coloradjust, *palgrp;
    int i, c = 0;


    Object *clutcGrpObj;

    if (e)
    {
        while (e[c].mpe_ID != MUIV_Palette_Entry_End)
            c++;
    }

    Object *clutcObjs[c];

    clutcGrpObj = GroupObject,
                MUIA_Group_Columns, COLUMNCOUNT,
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
        Child, (IPTR)(palgrp = VGroup,
        End),
        TAG_MORE, (IPTR) msg->ops_AttrList);

    if (self == NULL)
        return (IPTR) NULL;

    PEModeStrings[0] = _(MSG_4COLOR);
    PEModeStrings[1] = _(MSG_MULTICOLOR);

    coloradjust = ColoradjustObject,
            End;

    data = INST_DATA(CLASS, self);

    data->list = list;
    data->coloradjust = coloradjust;
    data->colorfiledgrp = clutcGrpObj;

    data->palgrp = palgrp;

    data->display_hook.h_Entry = HookEntry;
    data->display_hook.h_SubEntry = (HOOKFUNC) display_func;
    data->display_hook.h_Data = data;
    data->setcolor_hook.h_Entry = HookEntry;
    data->setcolor_hook.h_SubEntry = (HOOKFUNC) setcolor_func;
    data->setcolor_hook.h_Data = data;

    NNSET(list, MUIA_List_DisplayHook, (IPTR) & data->display_hook);

    data->numentries = sizeof(clutcObjs) / sizeof(Object *);
    D(bug("[PaletteEditor:Palette] %s: %d pen entries\n", __func__, data->numentries);)

    data->entries = e;
    data->names =
        (const char **)GetTagData(MUIA_Palette_Names, 0, msg->ops_AttrList);
    data->group = GetTagData(MUIA_Palette_Groupable, 0, msg->ops_AttrList);

    if (data->numentries > 0)
    {
        data->penmap4 = AllocMem(data->numentries * sizeof(ULONG), MEMF_CLEAR);
        data->penmap8 = AllocMem(data->numentries * sizeof(ULONG), MEMF_CLEAR);
        data->colorfieldentries = AllocMem(data->numentries * sizeof(Object *), MEMF_ANY);
        initPenMap(data->penmap4, 4);
        initPenMap(data->penmap8, 8);

        for (i = 0; (i < data->numentries) && (data->entries[i].mpe_ID != MUIV_Palette_Entry_End); i++)
        {
            DoMethod(data->list, MUIM_List_InsertSingle, &data->entries[i],
                MUIV_List_Insert_Bottom);

            D(bug("[PaletteEditor:Palette] %s: pen #%d ID = %d\n", __func__, i, data->entries[i].mpe_ID);)
        }
        NNSET(data->list, MUIA_List_Active, 0);
    }

    DoMethod(data->list, MUIM_Notify, MUIA_List_Active, MUIV_EveryTime,
        (IPTR) self, 5, MUIM_CallHook, (IPTR) &data->setcolor_hook,
        (IPTR) data, 1, 0);

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

IPTR PEPalette__MUIM_Setup
(
    Class *CLASS, Object *self, struct MUIP_Setup *message
)
{
    struct PEPalette_DATA *data = INST_DATA(CLASS, self);

    ULONG c;
    UWORD i;
    int rowcount;

    if (!(DoSuperMethodA(CLASS, self, (Msg) message)))
        return 0;

    D(
        bug("[PaletteEditor:Palette] %s: screen @ 0x%p\n", __func__, _screen(self));
        bug("[PaletteEditor:Palette] %s: screen bitmap @ 0x%p\n", __func__, _screen(self)->RastPort.BitMap);
    )

    c = GetBitMapAttr(_screen(self)->RastPort.BitMap, BMA_DEPTH);
    D(bug("[PaletteEditor:Palette] %s: c == %d\n", __func__, c);)
    c = (1 << c);
    if (c > 4)
    {
        data->palmode = CycleObject,
                    MUIA_Cycle_Entries, (IPTR)PEModeStrings,
                    MUIA_Cycle_Active, 1,
                End;
        c = 8;
        data->penmap = data->penmap8;
    }
    else
        data->penmap = data->penmap4;

    rowcount = c >> 2;
    D(bug("[PaletteEditor:Palette] %s: %d rows, %d colls\n", __func__, rowcount, COLUMNCOUNT);)

    /* create the clut colorfield objects ...*/
    D(bug("[PaletteEditor:Palette] %s: creating clut cf objects ...\n", __func__);)
    for (i = 0; i < rowcount; i++)
    {
        for(c = 0; c < COLUMNCOUNT; c++)
        {
            data->colorfieldentries[(i * COLUMNCOUNT) + c] = NewObject(CLUTColor_CLASS->mcc_Class, NULL,
                    MUIA_Frame, (c + i == 0) ? MUIV_Frame_ReadList : MUIV_Frame_None,
                    MUIA_CLUTColor_Index, (i * COLUMNCOUNT) + c,
                    MUIA_InputMode, MUIV_InputMode_RelVerify,
                TAG_DONE);
            D(bug("[PaletteEditor:Palette] %s: cf obj #%d @ 0x%p\n", __func__, (i * COLUMNCOUNT) + c, data->colorfieldentries[(i * COLUMNCOUNT) + c]);)
            DoMethod(data->colorfiledgrp, OM_ADDMEMBER, data->colorfieldentries[(i * COLUMNCOUNT) + c]);
        }
    }

    DoMethod(_app(self), MUIM_Application_PushMethod, self, 1, MUIM_PEPalette_Init);

    return 1;
}


IPTR PEPalette__MUIM_Cleanup
(
    Class *CLASS, Object *self, struct MUIP_Cleanup *message
)
{
    struct PEPalette_DATA *data = INST_DATA(CLASS, self);

    return DoSuperMethodA(CLASS, self, (Msg) message);
}


IPTR PEPalette__MUIM_PEPalette_Init
(
    Class *CLASS, Object *self, Msg message
)
{
    struct PEPalette_DATA *data = INST_DATA(CLASS, self);
    int i;

    D(bug("[PaletteEditor:Palette] %s: registering objects ...\n", __func__);)

    if (DoMethod(data->palgrp, MUIM_Group_InitChange))
    {
        if (data->palmode)
        {
            DoMethod(data->palgrp, OM_ADDMEMBER, (IPTR)data->palmode);
        }
        DoMethod(data->palgrp, OM_ADDMEMBER, (IPTR)data->colorfiledgrp);
        DoMethod(data->palgrp, OM_ADDMEMBER, (IPTR)data->coloradjust);
        DoMethod(data->palgrp, MUIM_Group_ExitChange);
    }

    if (data->palmode)
    {
        DoMethod(data->palmode, MUIM_Notify, MUIA_Cycle_Active, MUIV_EveryTime,
            (IPTR) self, 5, MUIM_CallHook, (IPTR) &data->setcolor_hook,
            (IPTR) data, 4, 0);
        i = 8;
    }
    else
        i = 4;

    for (; i > 0; i--)
    {
        DoMethod(data->colorfieldentries[i - 1], MUIM_Notify, MUIA_Pressed, FALSE,
            (IPTR) self, 5, MUIM_CallHook, (IPTR) &data->setcolor_hook,
            (IPTR) data, 3, i - 1);
    }
    DoMethod(data->coloradjust, MUIM_Notify, MUIA_Coloradjust_Red, MUIV_EveryTime,
        (IPTR) self, 5, MUIM_CallHook, (IPTR) &data->setcolor_hook,
        (IPTR) data, 2, 0);
    DoMethod(data->coloradjust, MUIM_Notify, MUIA_Coloradjust_Green, MUIV_EveryTime,
        (IPTR) self, 5, MUIM_CallHook, (IPTR) &data->setcolor_hook,
        (IPTR) data, 2, 1);
    DoMethod(data->coloradjust, MUIM_Notify, MUIA_Coloradjust_Blue, MUIV_EveryTime,
        (IPTR) self, 5, MUIM_CallHook, (IPTR) &data->setcolor_hook,
        (IPTR) data, 2, 2);

    D(bug("[PaletteEditor:Palette] %s: adjusting current colors..\n", __func__);)

    data->rgb[0] = data->entries[data->penmap[0]].mpe_Red;
    data->rgb[1] = data->entries[data->penmap[0]].mpe_Green;
    data->rgb[2] = data->entries[data->penmap[0]].mpe_Blue;

    NNSET(data->coloradjust, MUIA_Coloradjust_Red, data->rgb[0]);
    NNSET(data->coloradjust, MUIA_Coloradjust_Green, data->rgb[1]);
    NNSET(data->coloradjust, MUIA_Coloradjust_Blue, data->rgb[2]);

    D(bug("[PaletteEditor:Palette] %s: finished..\n", __func__);)

    return 1;
}

ZUNE_CUSTOMCLASS_7
(
    PEPalette, NULL, MUIC_Group, NULL,
    OM_NEW,                     struct opSet *,
    OM_DISPOSE,                 Msg,
    OM_SET,                     struct opSet *,
    OM_GET,                     struct opGet *,
    MUIM_Setup,                 struct MUIP_Setup *,
    MUIM_Cleanup,               struct MUIP_Cleanup *,
    MUIM_PEPalette_Init,        Msg
);
