/*
   Copyright © 2013, The AROS Development Team. All rights reserved.
   $Id$

Desc:
Lang: English
 */

#define DEBUG 1
#include <aros/debug.h>

#include <devices/rawkeycodes.h>
#include <mui/HotkeyString_mcc.h>
#include <zune/prefseditor.h>
#include <zune/customclasses.h>

#include <proto/alib.h>
#include <proto/commodities.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <proto/alib.h>

#include <string.h>
#include <stdio.h>

#include "reqtoolseditor.h"
#include "reqtoolsstringify.h"
#include "prefs.h"
#include "locale.h"

/*********************************************************************************************/

struct ReqToolsEditor_DATA
{
    Object *poptofrontobj;
    Object *usesysfontobj;
    Object *usefunckeysobj;
    Object *colorwheelstyleobj;

    Object *immediatesortobj;
    Object *drawersfirstobj;
    Object *drawersmixedobj;
    Object *diskactivityobj;
    Object *mmbobj;

    Object *defaultsforobj;
    Object *sizepercentobj;
    Object *minvisobj;
    Object *maxvisobj;
    Object *positionobj;
    Object *offsetxobj;
    Object *offsetyobj;
};

/*********************************************************************************************/

#define SETUP_INST_DATA struct ReqToolsEditor_DATA *data = INST_DATA(CLASS, self)

/*********************************************************************************************/

static struct Hook  selectdefaultshook, updatedefaultshook;
static CONST_STRPTR colorwheelstyle_labels[4], defaultsfor_labels[7], position_labels[6];

/*********************************************************************************************/

static void SelectDefaultsHook(struct Hook *hook, Object *self, struct ReqToolsEditor_DATA **data)
{
    struct ReqToolsPrefs *prefs = &reqtoolsprefs;
    IPTR active = 0;

    D(bug("[ReqToolsEditor.class] %s()\n", __PRETTY_FUNCTION__));
    
    GET(self, MUIA_Cycle_Active, &active);

    NNSET((*data)->positionobj, MUIA_Cycle_Active, prefs->ReqDefaults[active].ReqPos);
    NNSET((*data)->sizepercentobj, MUIA_Numeric_Value, prefs->ReqDefaults[active].Size);
    NNSET((*data)->offsetxobj, MUIA_String_Integer, prefs->ReqDefaults[active].LeftOffset);
    NNSET((*data)->offsetyobj, MUIA_String_Integer, prefs->ReqDefaults[active].TopOffset);
    NNSET((*data)->minvisobj, MUIA_String_Integer, prefs->ReqDefaults[active].MinEntries);
    NNSET((*data)->maxvisobj, MUIA_String_Integer, prefs->ReqDefaults[active].MaxEntries);
}

static void UpdateDefaultsHook(struct Hook *hook, Object *self, struct ReqToolsEditor_DATA **data)
{
    struct ReqToolsPrefs *prefs = &reqtoolsprefs;
    IPTR active = 0, tmpval;
    BOOL changed = FALSE;

    D(bug("[ReqToolsEditor.class] %s()\n", __PRETTY_FUNCTION__));
    
    GET((*data)->defaultsforobj, MUIA_Cycle_Active, &active);

    tmpval = prefs->ReqDefaults[active].ReqPos;
    if ((prefs->ReqDefaults[active].ReqPos = XGET((*data)->positionobj, MUIA_Cycle_Active)) != tmpval)
        changed = TRUE;
    tmpval = prefs->ReqDefaults[active].Size;
    if ((prefs->ReqDefaults[active].Size = XGET((*data)->sizepercentobj, MUIA_Numeric_Value)) != tmpval)
        changed = TRUE;
    tmpval = prefs->ReqDefaults[active].LeftOffset;
    if ((prefs->ReqDefaults[active].LeftOffset = XGET((*data)->offsetxobj, MUIA_String_Integer)) != tmpval)
        changed = TRUE;
    tmpval = prefs->ReqDefaults[active].TopOffset;
    if ((prefs->ReqDefaults[active].TopOffset = XGET((*data)->offsetyobj, MUIA_String_Integer)) != tmpval)
        changed = TRUE;
    tmpval = prefs->ReqDefaults[active].MinEntries;
    if ((prefs->ReqDefaults[active].MinEntries = XGET((*data)->minvisobj, MUIA_String_Integer)) != tmpval)
        changed = TRUE;
    tmpval = prefs->ReqDefaults[active].MaxEntries;
    if ((prefs->ReqDefaults[active].MaxEntries = XGET((*data)->maxvisobj, MUIA_String_Integer)) != tmpval)
        changed = TRUE;
    
    if (changed)
    {
        SET( _parent(self), MUIA_PrefsEditor_Changed, TRUE);
    }
}

/*********************************************************************************************/

BOOL Gadgets2ReqToolsPrefs(struct ReqToolsEditor_DATA *data)
{
    struct ReqToolsPrefs *prefs = &reqtoolsprefs;
    IPTR active = 0;

    D(bug("[ReqToolsEditor.class] %s()\n", __PRETTY_FUNCTION__));

    // Clear options we are about to set..
    prefs->Flags &= ~(RTPRF_NOSCRTOFRONT | RTPRF_DEFAULTFONT | RTPRF_FKEYS | RTPRB_DOWHEEL | RTPRB_FANCYWHEEL | RTPRF_IMMSORT | RTPRF_DIRSFIRST | RTPRF_DIRSMIXED | RTPRF_NOLED | RTPRF_MMBPARENT);

    GET(data->poptofrontobj, MUIA_Selected, &active);
    if (active == 0)
        prefs->Flags |= RTPRF_NOSCRTOFRONT;
    GET(data->usesysfontobj, MUIA_Selected, &active);
    if (active != 0)
        prefs->Flags |= RTPRF_DEFAULTFONT;
    GET(data->usefunckeysobj, MUIA_Selected, &active);
    if (active != 0)
        prefs->Flags |= RTPRF_FKEYS;
    GET(data->colorwheelstyleobj, MUIA_Cycle_Active, &active);
    if (active > 0)
        prefs->Flags |= RTPRB_DOWHEEL;
    if (active > 1)
        prefs->Flags |= RTPRB_FANCYWHEEL;
    GET(data->immediatesortobj, MUIA_Selected, &active);
    if (active != 0)
        prefs->Flags |= RTPRF_IMMSORT;
    GET(data->drawersfirstobj, MUIA_Selected, &active);
    if (active != 0)
        prefs->Flags |= RTPRF_DIRSFIRST;
    GET(data->drawersmixedobj, MUIA_Selected, &active);
    if (active != 0)
        prefs->Flags |= RTPRF_DIRSMIXED;
    GET(data->diskactivityobj, MUIA_Selected, &active);
    if (active == 0)
        prefs->Flags |= RTPRF_NOLED;
    GET(data->mmbobj, MUIA_Selected, &active);
    if (active != 0)
        prefs->Flags |= RTPRF_MMBPARENT;

    return TRUE;
}

/*********************************************************************************************/

BOOL ReqToolsPrefs2Gadgets(struct ReqToolsEditor_DATA *data)
{
    struct ReqToolsPrefs *prefs = &reqtoolsprefs;
    IPTR active = 0;

    D(bug("[ReqToolsEditor.class] %s()\n", __PRETTY_FUNCTION__));

    NNSET(data->poptofrontobj, MUIA_Selected, (prefs->Flags & RTPRF_NOSCRTOFRONT) ? 0 : 1);
    NNSET(data->usesysfontobj, MUIA_Selected, (prefs->Flags & RTPRF_DEFAULTFONT) ? 1 : 0);
    NNSET(data->usefunckeysobj, MUIA_Selected, (prefs->Flags & RTPRF_FKEYS) ? 1 : 0);
    if ((prefs->Flags & (RTPRB_DOWHEEL | RTPRB_FANCYWHEEL)) == 0)
        active = 0;
    else if ((prefs->Flags & (RTPRB_DOWHEEL | RTPRB_FANCYWHEEL)) == RTPRB_DOWHEEL)
        active = 1;
    else
        active = 2;
    NNSET(data->colorwheelstyleobj, MUIA_Cycle_Active, active);
    NNSET(data->immediatesortobj, MUIA_Selected, (prefs->Flags & RTPRF_IMMSORT) ? 1 : 0);
    NNSET(data->drawersfirstobj, MUIA_Selected, (prefs->Flags & RTPRF_DIRSFIRST) ? 1 : 0);
    NNSET(data->drawersmixedobj, MUIA_Selected, (prefs->Flags & RTPRF_DIRSMIXED) ? 1 : 0);
    NNSET(data->diskactivityobj, MUIA_Selected, (prefs->Flags & RTPRF_NOLED) ? 0 : 1);
    NNSET(data->mmbobj, MUIA_Selected, (prefs->Flags & RTPRF_MMBPARENT) ? 1 : 0);

    SelectDefaultsHook(NULL, data->defaultsforobj, &data);

    return TRUE;
}

/*********************************************************************************************/

Object *ReqToolsEditor__Checkmark()
{
    Object *obj = MUI_MakeObject(MUIO_Checkmark, (IPTR)NULL);

    if (obj)
    {
        SET(obj, MUIA_CycleChain, 1);
    }
    return obj;
}

Object *ReqToolsEditor__Cycle(const UBYTE **labels)
{
    Object *obj = MUI_MakeObject(MUIO_Cycle, (IPTR)NULL, labels);

    if (obj)
    {
        SET(obj, MUIA_CycleChain, 1);
    }
    return obj;
}

/*********************************************************************************************/

IPTR ReqToolsEditor__OM_NEW
(
    Class *CLASS, Object *self, struct opSet *message
)
{
    struct ReqToolsEditor_DATA *data = NULL;

    struct TagItem ReqToolsStringifyTags[] = {
        { MUIA_Numeric_Value, 0 },
        { MUIA_Numeric_Min, 0   },
        { MUIA_Numeric_Max, 100 },
        { TAG_DONE, (IPTR)NULL  }
    };

    //
    Object *poptofrontobj;
    Object *usesysfontobj;
    Object *usefunckeysobj;
    Object *colorwheelstyleobj;

    Object *immediatesortobj;
    Object *drawersfirstobj;
    Object *drawersmixedobj;
    Object *diskactivityobj;
    Object *mmbobj;

    Object *defaultsforobj;
    Object *positionobj;
    Object *sizepercentobj;
    Object *offsetxobj;
    Object *offsetyobj;
    Object *minvisobj;
    Object *maxvisobj;

    D(bug("[ReqToolsEditor.class] %s()\n", __PRETTY_FUNCTION__));

    selectdefaultshook.h_Entry = HookEntry;
    selectdefaultshook.h_SubEntry = (HOOKFUNC)SelectDefaultsHook;
    updatedefaultshook.h_Entry = HookEntry;
    updatedefaultshook.h_SubEntry = (HOOKFUNC)UpdateDefaultsHook;

    colorwheelstyle_labels[0] = _(MSG_WHEEL_NONE);
    colorwheelstyle_labels[1] = _(MSG_WHEEL_SIMPLE);
    colorwheelstyle_labels[2] = _(MSG_WHEEL_FANCY);

    defaultsfor_labels[0] = _(MSG_FILEREQ);
    defaultsfor_labels[1] = _(MSG_FONTREQ);
    defaultsfor_labels[2] = _(MSG_PALETTEREQ);
    defaultsfor_labels[3] = _(MSG_SCRMODEREQ);
    defaultsfor_labels[4] = _(MSG_VOLUMEREQ);
    defaultsfor_labels[5] = _(MSG_OTHERREQ);

    position_labels[0] = _(MSG_POINTER);
    position_labels[1] = _(MSG_CENTERWIN);
    position_labels[2] = _(MSG_CENTERSCR);
    position_labels[3] = _(MSG_TOPLEFTWIN);
    position_labels[4] = _(MSG_TOPLEFTSCR);

    sizepercentobj = (Object *)NewObjectA(ReqToolsStringify_CLASS->mcc_Class, NULL, ReqToolsStringifyTags);

    self = (Object *) DoSuperNewTags
    (
        CLASS, self, NULL,

        MUIA_PrefsEditor_Name,     __(MSG_WINDOW_TITLE),
        MUIA_PrefsEditor_Path,     (IPTR) "ReqTools.prefs",
        MUIA_PrefsEditor_IconTool, (IPTR) "SYS:Prefs/ReqTools",

        Child, HGroup,
            Child, (IPTR)VGroup,
                MUIA_Weight, 0,
                Child, VGroup,
                    GroupFrameT(__(MSG_GENERAL)),
                    Child, HGroup,
                        Child, HVSpace,
                        Child, ColGroup(2),
                            Child, Label1(__(MSG_POPSCREEN)),
                            Child, (poptofrontobj = ReqToolsEditor__Checkmark()),
                            Child, Label1(__(MSG_DEFAULTFONT)),
                            Child, (usesysfontobj = ReqToolsEditor__Checkmark()),
                            Child, Label1(__(MSG_FKEYS)),
                            Child, (usefunckeysobj = ReqToolsEditor__Checkmark()),
                        End,
                    End,
                    Child, (colorwheelstyleobj = ReqToolsEditor__Cycle(colorwheelstyle_labels)),
                    Child, HVSpace,
                End,
                Child, VGroup,
                    GroupFrameT(__(MSG_FILEREQUESTER)),
                    Child, HGroup,
                        Child, HVSpace,
                        Child, ColGroup(2),
                            Child, Label1(__(MSG_IMMEDIATESORT)),
                            Child, (immediatesortobj = ReqToolsEditor__Checkmark()),
                            Child, Label1(__(MSG_DRAWERSFIRST)),
                            Child, (drawersfirstobj = ReqToolsEditor__Checkmark()),
                            Child, Label1(__(MSG_DRAWERSMIXED)),
                            Child, (drawersmixedobj = ReqToolsEditor__Checkmark()),
                            Child, Label1(__(MSG_LED)),
                            Child, (diskactivityobj = ReqToolsEditor__Checkmark()),
                            Child, Label1(__(MSG_MMB_PARENT)),
                            Child, (mmbobj = ReqToolsEditor__Checkmark()),
                        End,
                    End,
                    Child, HVSpace,
                End,
            End,
            Child, VGroup,
                MUIA_Frame, MUIV_Frame_Group,
                Child, ColGroup(2),
                    Child, Label1(__(MSG_REQUESTER)),
                    Child, (defaultsforobj = ReqToolsEditor__Cycle(defaultsfor_labels)),
                    Child, HVSpace,
                    Child, RectangleObject,
                        MUIA_Background, MUII_FILL,
                        MUIA_FixHeight, 2,
                    End,
                    Child, Label1(__(MSG_POSITION)),
                    Child, (positionobj = ReqToolsEditor__Cycle(position_labels)),
                    Child, Label1(__(MSG_SIZE)),
                    Child, sizepercentobj,
                    Child, VGroup,
                        Child, TextObject,
                            StringFrame,
                            MUIA_FramePhantomHoriz, TRUE,
                            MUIA_Text_Contents, __(MSG_OFFSET),
                        End,
                        Child, TextObject,
                            StringFrame,
                            MUIA_FramePhantomHoriz, TRUE,
                            MUIA_Text_Contents, " ",
                        End,
                        Child, TextObject,
                            StringFrame,
                            MUIA_FramePhantomHoriz, TRUE,
                            MUIA_Text_Contents, __(MSG_NR_OF_ENTRIES),
                        End,
                        Child, TextObject,
                            StringFrame,
                            MUIA_FramePhantomHoriz, TRUE,
                            MUIA_Text_Contents, " ",
                        End,
                    End,
                    Child, VGroup,
                        Child, ColGroup(2),
                            Child, Label1("X"),
                            Child, (IPTR)(offsetxobj = StringObject,
                                    StringFrame,
                                    MUIA_Weight, 0,
                                    MUIA_CycleChain, TRUE,
                                    MUIA_String_Accept, (IPTR)"0123456789",
                                    MUIA_FixWidthTxt, (IPTR)"55555",
                                End),
                            Child, Label1("Y"),
                            Child, (IPTR)(offsetyobj = StringObject,
                                    StringFrame,
                                    MUIA_Weight, 0,
                                    MUIA_CycleChain, TRUE,
                                    MUIA_String_Accept, (IPTR)"0123456789",
                                    MUIA_FixWidthTxt, (IPTR)"55555",
                                End),
                            Child, Label1(__(MSG_MIN)),
                            Child, (IPTR)(minvisobj = StringObject,
                                    StringFrame,
                                    MUIA_Weight, 0,
                                    MUIA_CycleChain, TRUE,
                                    MUIA_String_Accept, (IPTR)"0123456789",
                                    MUIA_FixWidthTxt, (IPTR)"55555",
                                End),
                            Child, Label1(__(MSG_MAX)),
                            Child, (IPTR)(maxvisobj = StringObject,
                                    StringFrame,
                                    MUIA_Weight, 0,
                                    MUIA_CycleChain, TRUE,
                                    MUIA_String_Accept, (IPTR)"0123456789",
                                    MUIA_FixWidthTxt, (IPTR)"55555",
                                End),
                        End,
                    End,
                End,
                Child, HVSpace,
            End,
        End,
        TAG_DONE
    );

    if (self != NULL)
    {
        data = INST_DATA(CLASS, self);
        
        D(bug("[ReqToolsEditor.class] %s: Self @ 0x%p, Data @ 0x%p\n", __PRETTY_FUNCTION__, self, data));

        data->colorwheelstyleobj = colorwheelstyleobj;
        data->poptofrontobj = poptofrontobj;
        data->usesysfontobj = usesysfontobj;
        data->usefunckeysobj = usefunckeysobj;

        data->immediatesortobj = immediatesortobj;
        data->drawersfirstobj = drawersfirstobj;
        data->drawersmixedobj = drawersmixedobj;
        data->diskactivityobj = diskactivityobj;
        data->mmbobj = mmbobj;

        data->defaultsforobj = defaultsforobj;
        data->positionobj = positionobj;
        data->sizepercentobj = sizepercentobj;
        data->offsetxobj = offsetxobj;
        data->offsetyobj = offsetyobj;
        data->minvisobj = minvisobj;
        data->maxvisobj = maxvisobj;

        DoMethod
        (
            poptofrontobj, MUIM_Notify, MUIA_Selected, MUIV_EveryTime,
            (IPTR) self, 3, MUIM_Set, MUIA_PrefsEditor_Changed, TRUE
        );

        DoMethod
        (
            usesysfontobj, MUIM_Notify, MUIA_Cycle_Active, MUIV_EveryTime,
            (IPTR) self, 3, MUIM_Set, MUIA_PrefsEditor_Changed, TRUE
        );

        DoMethod
        (
            usefunckeysobj, MUIM_Notify, MUIA_Selected, MUIV_EveryTime,
            (IPTR) self, 3, MUIM_Set, MUIA_PrefsEditor_Changed, TRUE
        );

        DoMethod
        (
            colorwheelstyleobj, MUIM_Notify, MUIA_Cycle_Active, MUIV_EveryTime,
            (IPTR) self, 3, MUIM_Set, MUIA_PrefsEditor_Changed, TRUE
        );

        DoMethod
        (
            immediatesortobj, MUIM_Notify, MUIA_Selected, MUIV_EveryTime,
            (IPTR) self, 3, MUIM_Set, MUIA_PrefsEditor_Changed, TRUE
        );

        DoMethod
        (
            drawersfirstobj, MUIM_Notify, MUIA_Selected, MUIV_EveryTime,
            (IPTR) self, 3, MUIM_Set, MUIA_PrefsEditor_Changed, TRUE
        );

        DoMethod
        (
            drawersmixedobj, MUIM_Notify, MUIA_Selected, MUIV_EveryTime,
            (IPTR) self, 3, MUIM_Set, MUIA_PrefsEditor_Changed, TRUE
        );

        DoMethod
        (
            diskactivityobj, MUIM_Notify, MUIA_Selected, MUIV_EveryTime,
            (IPTR) self, 3, MUIM_Set, MUIA_PrefsEditor_Changed, TRUE
        );

        DoMethod
        (
            mmbobj, MUIM_Notify, MUIA_Selected, MUIV_EveryTime,
            (IPTR) self, 3, MUIM_Set, MUIA_PrefsEditor_Changed, TRUE
        );

        DoMethod
        (
            defaultsforobj, MUIM_Notify, MUIA_Cycle_Active, MUIV_EveryTime,
            (IPTR)defaultsforobj, 3, MUIM_CallHook, (IPTR)&selectdefaultshook, (IPTR)data
        );

        DoMethod
        (
            positionobj, MUIM_Notify, MUIA_Cycle_Active, MUIV_EveryTime,
            (IPTR) self, 3, MUIM_CallHook, (IPTR)&updatedefaultshook, (IPTR)data
        );

        DoMethod
        (
            sizepercentobj, MUIM_Notify, MUIA_Selected, MUIV_EveryTime,
            (IPTR) self, 3, MUIM_CallHook, (IPTR)&updatedefaultshook, (IPTR)data
        );

        DoMethod
        (
            offsetxobj, MUIM_Notify, MUIA_Selected, MUIV_EveryTime,
            (IPTR) self, 3, MUIM_CallHook, (IPTR)&updatedefaultshook, (IPTR)data
        );

        DoMethod
        (
            offsetyobj, MUIM_Notify, MUIA_Selected, MUIV_EveryTime,
            (IPTR) self, 3, MUIM_CallHook, (IPTR)&updatedefaultshook, (IPTR)data
        );

        DoMethod
        (
            minvisobj, MUIM_Notify, MUIA_Selected, MUIV_EveryTime,
            (IPTR) self, 3, MUIM_CallHook, (IPTR)&updatedefaultshook, (IPTR)data
        );

        DoMethod
        (
            maxvisobj, MUIM_Notify, MUIA_Selected, MUIV_EveryTime,
            (IPTR) self, 3, MUIM_CallHook, (IPTR)&updatedefaultshook, (IPTR)data
        );

        ReqToolsPrefs2Gadgets(data);
    }
    return (IPTR) self;

}

/*********************************************************************************************/

IPTR ReqToolsEditor__MUIM_PrefsEditor_ImportFH (
    Class *CLASS, Object *self,
    struct MUIP_PrefsEditor_ImportFH *message
)
{
    SETUP_INST_DATA;
    BOOL success = TRUE;

    D(bug("[ReqToolsEditor.class] %s()\n", __PRETTY_FUNCTION__));

    success = Prefs_ImportFH(message->fh);
    if (success) ReqToolsPrefs2Gadgets(data);

    return success;
}

/*********************************************************************************************/

IPTR ReqToolsEditor__MUIM_PrefsEditor_ExportFH
(
    Class *CLASS, Object *self,
    struct MUIP_PrefsEditor_ExportFH *message
)
{
    SETUP_INST_DATA;
    BOOL success = TRUE;

    D(bug("[ReqToolsEditor.class] %s()\n", __PRETTY_FUNCTION__));

    Gadgets2ReqToolsPrefs(data);
    success = Prefs_ExportFH(message->fh);

    return success;
}

/*********************************************************************************************/

IPTR ReqToolsEditor__MUIM_PrefsEditor_SetDefaults
(
    Class *CLASS, Object *self, Msg message
)
{
    SETUP_INST_DATA;
    BOOL success = TRUE;

    D(bug("[ReqToolsEditor.class] %s()\n", __PRETTY_FUNCTION__));

    success = Prefs_Default();
    if (success) ReqToolsPrefs2Gadgets(data);

    return success;
}

/*** Setup ******************************************************************/
ZUNE_CUSTOMCLASS_4
(
    ReqToolsEditor, NULL, MUIC_PrefsEditor, NULL,
    OM_NEW,                       struct opSet *,
    MUIM_PrefsEditor_ImportFH,    struct MUIP_PrefsEditor_ImportFH *,
    MUIM_PrefsEditor_ExportFH,    struct MUIP_PrefsEditor_ExportFH *,
    MUIM_PrefsEditor_SetDefaults, Msg
);

