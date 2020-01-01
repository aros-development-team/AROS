/*
    Copyright © 2011-2020, The AROS Development Team. All rights reserved.
    $Id$
*/

#define MUIMASTER_YES_INLINE_STDARG
#define DEBUG 0

#include <proto/alib.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/utility.h>
#include <proto/intuition.h>

#include <exec/rawfmt.h>
#include <libraries/mui.h>
#include <zune/customclasses.h>
#include <mui/NFloattext_mcc.h>

#include <stdio.h>

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
    Object * objFreqH;
    Object * objFreqK;
    Object * objFeaturesGrp;
    Object * objFeatures;

    ULONG DisplayID;
};

CONST_STRPTR str_empty = "";

static inline Object *makeSMLabel1(char *label)
{
    Object *labObj = MUI_MakeObject(MUIO_Label, (IPTR)(label), MUIO_Label_SingleFrame);
    if (labObj)
    {
        SET(labObj, MUIA_FixWidthTxt , (IPTR)"000000");
    }
    return labObj;
}

Object *ScreenModeAttributes__OM_NEW(Class *CLASS, Object *self, struct opSet *message)
{
    struct ScreenModeAttributes_DATA *data;
    Object  * objColGrp, * objVisibleW, * objVisibleH, * objMinimumW, * objMinimumH, 
            * objMaximumW, * objMaximumH, * objMaximumColors, * objFreqH, * objFreqK,
            * objFeaturesGrp, * objFeatures;

    ULONG id;

    self = (Object *)DoSuperNewTags
    (
        CLASS, self, NULL,
        MUIA_Group_Horiz, TRUE,
        Child, (IPTR)VGroup,
            Child, (IPTR)HGroup,
                Child, (IPTR)(objColGrp = (Object *)ColGroup(5),

                    Child, (IPTR)LLabel1(_(MSG_VISIBLE_SIZE)),
                    Child, (IPTR)LLabel1(": "),
                    Child, (IPTR)(objVisibleW = (Object *)makeSMLabel1("16368")),
                    Child, (IPTR)Label1("x"),
                    Child, (IPTR)(objVisibleH = (Object *)makeSMLabel1("16384")),

                    Child, (IPTR)LLabel1(_(MSG_MINIMUM_SIZE)),
                    Child, (IPTR)LLabel1(": "),
                    Child, (IPTR)(objMinimumW = (Object *)makeSMLabel1("16368")),
                    Child, (IPTR)Label1("x"),
                    Child, (IPTR)(objMinimumH = (Object *)makeSMLabel1("16368")),

                    Child, (IPTR)LLabel1(_(MSG_MAXIMUM_SIZE)),
                    Child, (IPTR)LLabel1(": "),
                    Child, (IPTR)(objMaximumW = (Object *)makeSMLabel1("16368")),
                    Child, (IPTR)Label1("x"),
                    Child, (IPTR)(objMaximumH = (Object *)makeSMLabel1("16368")),

                    Child, (IPTR)LLabel1(_(MSG_MAXIMUM_COLORS)),
                    Child, (IPTR)LLabel1(": "),
                    Child, (IPTR)(objMaximumColors = (Object *)LLabel1("16777216")),
                    Child, (IPTR)RectangleObject, End,
                    Child, (IPTR)RectangleObject, End,

                    Child, (IPTR)RectangleObject,
                        MUIA_FixHeightTxt, (IPTR)str_empty,
                    End,
                    Child, (IPTR)RectangleObject,
                        MUIA_FixHeightTxt, (IPTR)str_empty,
                    End,
                    Child, (IPTR)RectangleObject,
                        MUIA_FixHeightTxt, (IPTR)str_empty,
                    End,
                    Child, (IPTR)RectangleObject,
                        MUIA_FixHeightTxt, (IPTR)str_empty,
                    End,
                    Child, (IPTR)RectangleObject,
                        MUIA_FixHeightTxt, (IPTR)str_empty,
                    End,

                    Child, (IPTR)LLabel1(_(MSG_FREQUENCY)),
                    Child, (IPTR)LLabel1(": "),
                    Child, (IPTR)(objFreqH = (Object *)makeSMLabel1("16368")),
                    Child, (IPTR)Label1(","),
                    Child, (IPTR)(objFreqK = (Object *)makeSMLabel1("16368")),

                    Child, (IPTR)RectangleObject,
                        MUIA_FixHeightTxt, (IPTR)str_empty,
                    End,
                    Child, (IPTR)RectangleObject,
                        MUIA_FixHeightTxt, (IPTR)str_empty,
                    End,
                    Child, (IPTR)RectangleObject,
                        MUIA_FixHeightTxt, (IPTR)str_empty,
                    End,
                    Child, (IPTR)RectangleObject,
                        MUIA_FixHeightTxt, (IPTR)str_empty,
                    End,
                    Child, (IPTR)RectangleObject,
                        MUIA_FixHeightTxt, (IPTR)str_empty,
                    End,
                End),
                Child, (IPTR)RectangleObject,
                    MUIA_FixHeightTxt, (IPTR)str_empty,
                End,
            End,
            Child, (IPTR)(objFeaturesGrp = ScrollgroupObject,
                NoFrame,
                MUIA_Scrollgroup_Contents, (IPTR)(objFeatures = NFloattextObject,
                    NoFrame,
                    MUIA_Background, MUII_BACKGROUND,
                    MUIA_CycleChain, 1,
                    MUIA_Floattext_Text, (IPTR)str_empty,
                End),
            End),
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
    data->objFreqH              = objFreqH;
    data->objFreqK              = objFreqK;
    data->objFeaturesGrp        = objFeaturesGrp;
    data->objFeatures           = objFeatures;

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
                ULONG dflags = 0;
                struct DisplayInfo di;
                struct DimensionInfo dim;
                struct MonitorInfo mi;
                TEXT buffer[1024];
                ULONG pclock = (ULONG)-1;

                D(bug("[smattributes] Set DisplayID = 0x%08lx\n", tag->ti_Data));

                if (GetDisplayInfoData(NULL, (UBYTE *)&di, sizeof(di), DTAG_DISP, tag->ti_Data))
                {
                    pclock = di.PixelSpeed;
                    dflags = di.PropertyFlags;
                }

                if (GetDisplayInfoData(NULL, (UBYTE *)&dim, sizeof(dim), DTAG_DIMS, tag->ti_Data))
                {
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
                else
                {
                    if (DoMethod(data->objColGrp, MUIM_Group_InitChange))
                    {
                        set(data->objVisibleW, MUIA_Text_Contents, (IPTR)str_empty);
                        set(data->objVisibleH, MUIA_Text_Contents, (IPTR)str_empty);
                        set(data->objMinimumW, MUIA_Text_Contents, (IPTR)str_empty);
                        set(data->objMinimumH, MUIA_Text_Contents, (IPTR)str_empty);
                        set(data->objMaximumW, MUIA_Text_Contents, (IPTR)str_empty);
                        set(data->objMaximumH, MUIA_Text_Contents, (IPTR)str_empty);
                        set(data->objMaximumColors, MUIA_Text_Contents, (IPTR)str_empty);
                        DoMethod(data->objColGrp, MUIM_Group_ExitChange);
                    } 
                }

                if ((pclock != (ULONG)-1) && (GetDisplayInfoData(NULL, (UBYTE *)&mi, sizeof(mi), DTAG_MNTR, tag->ti_Data)))
                {
                    IPTR values[1];

                    if (DoMethod(data->objColGrp, MUIM_Group_InitChange))
                    {
                        ULONG phz = (1000000000/ pclock);
                        ULONG pcline = (280/ pclock) * mi.TotalColorClocks;

                        values[0] = phz / pcline;
                        values[1] = values[0] / mi.TotalRows;
                        values[0] /= 1000;
                        RawDoFmt("%ldHz", (RAWARG)&values[1], RAWFMTFUNC_STRING, buffer);
                        set(data->objFreqH, MUIA_Text_Contents, buffer);

                        RawDoFmt("%ldkHz", (RAWARG)values, RAWFMTFUNC_STRING, buffer);
                        set(data->objFreqK, MUIA_Text_Contents, buffer);

                        DoMethod(data->objColGrp, MUIM_Group_ExitChange);
                    }
                }
                else
                {
                    if (DoMethod(data->objColGrp, MUIM_Group_InitChange))
                    {
                        set(data->objFreqH, MUIA_Text_Contents, (IPTR)str_empty);
                        set(data->objFreqK, MUIA_Text_Contents, (IPTR)str_empty);
                        DoMethod(data->objColGrp, MUIM_Group_ExitChange);
                    }
                }

                if (dflags != 0)
                {
                    int offset = 0;

                    if (dflags & DIPF_IS_LACE)
                        offset += sprintf(&buffer[offset], _(MSG_INTERLACED));
                    if (dflags & DIPF_IS_ECS)
                        offset += sprintf(&buffer[offset], _(MSG_WANTS_ECS));
                    if (dflags & DIPF_IS_DUALPF)
                        offset += sprintf(&buffer[offset], _(MSG_DUALP));
                    if (dflags & DIPF_IS_GENLOCK)
                        offset += sprintf(&buffer[offset], _(MSG_GENLOCK));
                    if (dflags & DIPF_IS_DRAGGABLE)
                        offset += sprintf(&buffer[offset], _(MSG_DRAGGABLE));
                    if (dflags & DIPF_IS_PANELLED)
                        offset += sprintf(&buffer[offset], _(MSG_PANELLED));
                    if (dflags & DIPF_IS_BEAMSYNC)
                        offset += sprintf(&buffer[offset], _(MSG_BEAM_SYNC));

                    if ((offset > 0) && DoMethod(data->objFeaturesGrp, MUIM_Group_InitChange))
                    {
                        set(data->objFeatures, MUIA_Floattext_Text, buffer);
                        DoMethod(data->objFeaturesGrp, MUIM_Group_ExitChange);
                    }
                }
                else
                {
                    if (DoMethod(data->objFeaturesGrp, MUIM_Group_InitChange))
                    {
                        set(data->objFeatures, MUIA_Floattext_Text, (IPTR)str_empty);
                        DoMethod(data->objFeaturesGrp, MUIM_Group_ExitChange);
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
