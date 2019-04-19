/*
    Copyright © 2011-2019, The AROS Development Team. All rights reserved.
    $Id$
*/

#define MUIMASTER_YES_INLINE_STDARG
#define DEBUG 0

#include <libraries/mui.h>
#include <exec/rawfmt.h>

#include <proto/alib.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/utility.h>
#include <proto/intuition.h>
#include <zune/customclasses.h>

#include "locale.h"

#include "smattributes.h"

struct ScreenModeAttributes_DATA
{
    Object * objColGrp;
    Object * objVisibleW;
    Object * objVisibleH;
    Object * objMinimumW;
    Object * objMinimumH;
    Object * objMaximumW;
    Object * objMaximumH;
    Object * objMaximumColors;
    ULONG DisplayID;
};

Object *ScreenModeAttributes__OM_NEW(Class *CLASS, Object *self, struct opSet *message)
{
    struct ScreenModeAttributes_DATA *data;
    Object  * objColGrp, * objVisibleW, * objVisibleH, * objMinimumW, * objMinimumH, 
            * objMaximumW, * objMaximumH, * objMaximumColors;

    ULONG id;

    self = (Object *)DoSuperNewTags
    (
        CLASS, self, NULL,
        MUIA_Group_Horiz, TRUE,
        Child, (IPTR)VGroup,
            Child, (IPTR)(objColGrp = (Object *)ColGroup(4),
                Child, (IPTR)LLabel1(_(MSG_VISIBLE_SIZE)),
                Child, (IPTR)(objVisibleW = (Object *)Label1("16368")),
                Child, (IPTR)Label1("x"),
                Child, (IPTR)(objVisibleH = (Object *)Label1("16384")),

                Child, (IPTR)LLabel1(_(MSG_MINIMUM_SIZE)),
                Child, (IPTR)(objMinimumW = (Object *)Label1("16368")),
                Child, (IPTR)Label1("x"),
                Child, (IPTR)(objMinimumH = (Object *)Label1("16368")),

                Child, (IPTR)LLabel1(_(MSG_MAXIMUM_SIZE)),
                Child, (IPTR)(objMaximumW = (Object *)Label1("16368")),
                Child, (IPTR)Label1("x"),
                Child, (IPTR)(objMaximumH = (Object *)Label1("16368")),

                Child, (IPTR)LLabel1(_(MSG_MAXIMUM_COLORS)),
                Child, (IPTR)(objMaximumColors = (Object *)LLabel1("16777216")),
                Child, (IPTR)RectangleObject, End,
                Child, (IPTR)RectangleObject, End,

            End),  

            Child, (IPTR)RectangleObject, End,

        End,

        TAG_MORE, (IPTR)message->ops_AttrList
    );

    if (!self)
        goto err;

    D(bug("[smattributes] Created ScreenModeAttributes object 0x%p\n", self));
    data = INST_DATA(CLASS, self);

    data->objColGrp             = objColGrp;

    data->objVisibleW           = objVisibleW;
    data->objVisibleH           = objVisibleH;
    data->objMinimumW           = objMinimumW;
    data->objMinimumH           = objMinimumH;
    data->objMaximumW           = objMaximumW;
    data->objMaximumH           = objMaximumH;
    data->objMaximumColors      = objMaximumColors;

    id = GetTagData(MUIA_ScreenModeAttributes_DisplayID, INVALID_ID, message->ops_AttrList);
    D(bug("[smattributes] Setting initial ModeID 0x%08lX\n", id));
    set(self, MUIA_ScreenModeAttributes_DisplayID, id);

    return self;

err:
    CoerceMethod(CLASS, self, OM_DISPOSE);
    return NULL;
}

IPTR ScreenModeAttributes__OM_SET(Class *CLASS, Object *self, struct opSet *message)
{
    struct ScreenModeAttributes_DATA *data = INST_DATA(CLASS, self);    
    struct TagItem *tags;
    struct TagItem *tag;
    IPTR ret;

    DB2(bug("[smattributes] OM_SET called\n"));
    for (tags = message->ops_AttrList; (tag = NextTagItem(&tags)); )
    {
        switch (tag->ti_Tag)
        {
            case MUIA_ScreenModeAttributes_DisplayID:
            {
                struct DimensionInfo dim;

                D(bug("[smattributes] Set DisplayID = 0x%08lx\n", tag->ti_Data));

                if (GetDisplayInfoData(NULL, (UBYTE *)&dim, sizeof(dim), DTAG_DIMS, tag->ti_Data))
                {
                    TEXT buffer[128];
                    ULONG val;

                    if (DoMethod(data->objColGrp, MUIM_Group_InitChange))
                    {
                        val = dim.Nominal.MaxX - dim.Nominal.MinX + 1;
                        RawDoFmt("%ld", (RAWARG)&val, RAWFMTFUNC_STRING, buffer);
                        set(data->objVisibleW, MUIA_Text_Contents, buffer);
                        val = dim.Nominal.MaxY - dim.Nominal.MinY + 1;
                        RawDoFmt("%ld", (RAWARG)&val, RAWFMTFUNC_STRING, buffer);
                        set(data->objVisibleH, MUIA_Text_Contents, buffer);

                        RawDoFmt("%d", (RAWARG)&dim.MinRasterWidth, RAWFMTFUNC_STRING, buffer);
                        set(data->objMinimumW, MUIA_Text_Contents, buffer);
                        RawDoFmt("%d", (RAWARG)&dim.MinRasterHeight, RAWFMTFUNC_STRING, buffer);
                        set(data->objMinimumH, MUIA_Text_Contents, buffer);

                        RawDoFmt("%d", (RAWARG)&dim.MaxRasterWidth, RAWFMTFUNC_STRING, buffer);
                        set(data->objMaximumW, MUIA_Text_Contents, buffer);
                        RawDoFmt("%d", (RAWARG)&dim.MaxRasterHeight, RAWFMTFUNC_STRING, buffer);
                        set(data->objMaximumH, MUIA_Text_Contents, buffer);

                        val = 1 << (dim.MaxDepth > 24 ? 24 : dim.MaxDepth);
                        RawDoFmt("%ld", (RAWARG)&val, RAWFMTFUNC_STRING, buffer);
                        set(data->objMaximumColors, MUIA_Text_Contents, buffer);

                        DoMethod(data->objColGrp, MUIM_Group_ExitChange);
                    }
                }

                SetAttrs(self, MUIA_Disabled, tag->ti_Data == INVALID_ID, TAG_DONE);

                break;
            }
        }
    }

    DB2(bug("[smattributes] Calling OM_SET() on superclass\n"));
    ret = DoSuperMethodA(CLASS, self, (Msg)message);
    DB2(bug("[smattributes] OM_SET() on superclass returned %ld\n", ret));
    return ret;
}

ZUNE_CUSTOMCLASS_2
(
    ScreenModeAttributes, NULL, MUIC_Group, NULL,   
    OM_NEW,     struct opSet *,
    OM_SET,     struct opSet *
);
