/*
   Copyright (C) 2022, The AROS Development Team. All rights reserved.
*/

#define DEBUG 0
#include <aros/debug.h>

#include <devices/rawkeycodes.h>
#include <zune/prefseditor.h>
#include <zune/customclasses.h>

#include <proto/alib.h>
#include <proto/commodities.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <proto/alib.h>

#include <string.h>
#include <stdio.h>

#include "asleditor.h"
#include "aslstringify.h"
#include "prefs.h"
#include "locale.h"

/*********************************************************************************************/

struct AslEditor_DATA
{
    Object *drawersfirstobj;

    Object *positionobj;
    Object *offsetxobj;
    Object *offsetyobj;

    Object *sizeobj;
    Object *heightpercentobj;
    Object *widthpercentobj;
};

/*********************************************************************************************/

#define SETUP_INST_DATA struct AslEditor_DATA *data = INST_DATA(CLASS, self)

/*********************************************************************************************/

static struct Hook  updatedefaultshook;
static CONST_STRPTR position_labels[7], size_labels[3];

/*********************************************************************************************/

static void SelectDefaultsHook(struct Hook *hook, Object *self, struct AslEditor_DATA **data)
{
    struct AslPrefs *prefs = &aslprefs;

    D(bug("[AslEditor.class] %s()\n", __PRETTY_FUNCTION__));
    
    /* Hardcode for now */
    NNSET((*data)->positionobj, MUIA_Cycle_Active, 2);
    NNSET((*data)->sizeobj, MUIA_Cycle_Active, 1);

    /* Use prefs */
    NNSET((*data)->offsetxobj, MUIA_String_Integer, prefs->ap_RelativeLeft);
    NNSET((*data)->offsetyobj, MUIA_String_Integer, prefs->ap_RelativeTop);

    NNSET((*data)->widthpercentobj, MUIA_Numeric_Value, prefs->ap_RelativeWidth);
    NNSET((*data)->heightpercentobj, MUIA_Numeric_Value, prefs->ap_RelativeHeight);

    /* Disable non-editable for now */
    NNSET((*data)->positionobj, MUIA_Disabled, TRUE);
    NNSET((*data)->sizeobj, MUIA_Disabled, TRUE);
    NNSET((*data)->offsetxobj, MUIA_Disabled, TRUE);
    NNSET((*data)->offsetyobj, MUIA_Disabled, TRUE);
}

static void UpdateDefaultsHook(struct Hook *hook, Object *self, struct AslEditor_DATA **data)
{
    struct AslPrefs *prefs = &aslprefs;
    // IPTR active = 0, tmpval;
    // BOOL changed = FALSE;

    // D(bug("[AslEditor.class] %s()\n", __PRETTY_FUNCTION__));
    
    // GET((*data)->defaultsforobj, MUIA_Cycle_Active, &active);

    // tmpval = prefs->ReqDefaults[active].ReqPos;
    // if ((prefs->ReqDefaults[active].ReqPos = XGET((*data)->positionobj, MUIA_Cycle_Active)) != tmpval)
    //     changed = TRUE;
    // tmpval = prefs->ReqDefaults[active].Size;
    // if ((prefs->ReqDefaults[active].Size = XGET((*data)->sizepercentobj, MUIA_Numeric_Value)) != tmpval)
    //     changed = TRUE;
    // tmpval = prefs->ReqDefaults[active].LeftOffset;
    // if ((prefs->ReqDefaults[active].LeftOffset = XGET((*data)->offsetxobj, MUIA_String_Integer)) != tmpval)
    //     changed = TRUE;
    // tmpval = prefs->ReqDefaults[active].TopOffset;
    // if ((prefs->ReqDefaults[active].TopOffset = XGET((*data)->offsetyobj, MUIA_String_Integer)) != tmpval)
    //     changed = TRUE;
    // tmpval = prefs->ReqDefaults[active].MinEntries;
    // if ((prefs->ReqDefaults[active].MinEntries = XGET((*data)->minvisobj, MUIA_String_Integer)) != tmpval)
    //     changed = TRUE;
    // tmpval = prefs->ReqDefaults[active].MaxEntries;
    // if ((prefs->ReqDefaults[active].MaxEntries = XGET((*data)->maxvisobj, MUIA_String_Integer)) != tmpval)
    //     changed = TRUE;
    
    // if (changed)
    // {
    //     SET( _parent(self), MUIA_PrefsEditor_Changed, TRUE);
    // }
}

/*********************************************************************************************/

BOOL Gadgets2AslPrefs(struct AslEditor_DATA *data)
{
    struct AslPrefs *prefs = &aslprefs;
    IPTR active = 0;

    D(bug("[AslEditor.class] %s()\n", __PRETTY_FUNCTION__));

    // Clear options we are about to set..
    // prefs->Flags &= ~(RTPRF_NOSCRTOFRONT | RTPRF_DEFAULTFONT | RTPRF_FKEYS | RTPRB_DOWHEEL | RTPRB_FANCYWHEEL | RTPRF_IMMSORT | RTPRF_DIRSFIRST | RTPRF_DIRSMIXED | RTPRF_NOLED | RTPRF_MMBPARENT);

    // GET(data->poptofrontobj, MUIA_Selected, &active);
    // if (active == 0)
    //     prefs->Flags |= RTPRF_NOSCRTOFRONT;
    // GET(data->usesysfontobj, MUIA_Selected, &active);
    // if (active != 0)
    //     prefs->Flags |= RTPRF_DEFAULTFONT;
    // GET(data->usefunckeysobj, MUIA_Selected, &active);
    // if (active != 0)
    //     prefs->Flags |= RTPRF_FKEYS;
    // GET(data->colorwheelstyleobj, MUIA_Cycle_Active, &active);
    // if (active > 0)
    //     prefs->Flags |= RTPRB_DOWHEEL;
    // if (active > 1)
    //     prefs->Flags |= RTPRB_FANCYWHEEL;
    // GET(data->immediatesortobj, MUIA_Selected, &active);
    // if (active != 0)
    //     prefs->Flags |= RTPRF_IMMSORT;
    // GET(data->drawersfirstobj, MUIA_Selected, &active);
    // if (active != 0)
    //     prefs->Flags |= RTPRF_DIRSFIRST;
    // GET(data->drawersmixedobj, MUIA_Selected, &active);
    // if (active != 0)
    //     prefs->Flags |= RTPRF_DIRSMIXED;
    // GET(data->diskactivityobj, MUIA_Selected, &active);
    // if (active == 0)
    //     prefs->Flags |= RTPRF_NOLED;
    // GET(data->mmbobj, MUIA_Selected, &active);
    // if (active != 0)
    //     prefs->Flags |= RTPRF_MMBPARENT;

    return TRUE;
}

/*********************************************************************************************/

BOOL AslPrefs2Gadgets(struct AslEditor_DATA *data)
{
    struct AslPrefs *prefs = &aslprefs;
    IPTR active = 0;

    D(bug("[AslEditor.class] %s()\n", __PRETTY_FUNCTION__));

    NNSET(data->drawersfirstobj, MUIA_Selected, (prefs->ap_SortDrawers) ? 1 : 0);

    SelectDefaultsHook(NULL, NULL, &data);

    /* Disable non-editable for now */
    NNSET(data->drawersfirstobj, MUIA_Disabled, TRUE);

    return TRUE;
}

/*********************************************************************************************/

Object *AslEditor__Checkmark()
{
    Object *obj = MUI_MakeObject(MUIO_Checkmark, (IPTR)NULL);

    if (obj)
    {
        SET(obj, MUIA_CycleChain, 1);
    }
    return obj;
}

Object *AslEditor__Cycle(const UBYTE **labels)
{
    Object *obj = MUI_MakeObject(MUIO_Cycle, (IPTR)NULL, labels);

    if (obj)
    {
        SET(obj, MUIA_CycleChain, 1);
    }
    return obj;
}

/*********************************************************************************************/

IPTR AslEditor__OM_NEW
(
    Class *CLASS, Object *self, struct opSet *message
)
{
    struct AslEditor_DATA *data = NULL;

    struct TagItem AslStringifyTags[] = {
        { MUIA_Numeric_Value, 25 },
        { MUIA_Numeric_Min, 25   },
        { MUIA_Numeric_Max, 100  },
        { TAG_DONE, (IPTR)NULL   }
    };

    //

    Object *drawersfirstobj;

    Object *positionobj;
    Object *offsetxobj;
    Object *offsetyobj;

    Object *sizeobj;
    Object *heightpercentobj;
    Object *widthpercentobj;

    D(bug("[AslEditor.class] %s()\n", __PRETTY_FUNCTION__));

    updatedefaultshook.h_Entry = HookEntry;
    updatedefaultshook.h_SubEntry = (HOOKFUNC)UpdateDefaultsHook;

    position_labels[0] = "Default";
    position_labels[1] = _(MSG_CENTERWIN);
    position_labels[2] = _(MSG_CENTERSCR);
    position_labels[3] = _(MSG_TOPLEFTWIN); // Allow offset
    position_labels[4] = _(MSG_TOPLEFTSCR); // Allow offset
    position_labels[5] = _(MSG_POINTER);

    size_labels[0] = "Default";
    size_labels[1] = "Relative";

    heightpercentobj = (Object *)NewObjectA(AslStringify_CLASS->mcc_Class, NULL, AslStringifyTags);
    widthpercentobj = (Object *)NewObjectA(AslStringify_CLASS->mcc_Class, NULL, AslStringifyTags);

    self = (Object *) DoSuperNewTags
    (
        CLASS, self, NULL,

        MUIA_PrefsEditor_Name,     __(MSG_WINDOW_TITLE),
        MUIA_PrefsEditor_Path,     (IPTR) "Asl.prefs",
        MUIA_PrefsEditor_IconTool, (IPTR) "SYS:Prefs/Asl",

        Child, HGroup,
            Child, VGroup,
                GroupFrameT(__(MSG_GENERAL)),
                Child, ColGroup(2),
                    Child, Label1(__(MSG_POSITION)),
                    Child, (positionobj = AslEditor__Cycle(position_labels)),
                    Child, Label1(__(MSG_OFFSET)),
                    Child, HVSpace,
                    Child, Label1("X:"),
                    Child, HGroup,
                        Child, (IPTR)(offsetxobj = StringObject,
                            StringFrame,
                            MUIA_Weight, 0,
                            MUIA_CycleChain, TRUE,
                            MUIA_String_Accept, (IPTR)"0123456789",
                            MUIA_FixWidthTxt, (IPTR)"55555",
                        End),
                        Child, HVSpace,
                    End,
                    Child, Label1("Y:"),
                    Child, HGroup,
                        Child, (IPTR)(offsetyobj = StringObject,
                            StringFrame,
                            MUIA_Weight, 0,
                            MUIA_CycleChain, TRUE,
                            MUIA_String_Accept, (IPTR)"0123456789",
                            MUIA_FixWidthTxt, (IPTR)"55555",
                        End),
                        Child, HVSpace,
                    End,
                    Child, Label1("Size"),
                    Child, (sizeobj = AslEditor__Cycle(size_labels)),
                    Child, Label1("Size (% of visible width):"),
                    Child, widthpercentobj,
                    Child, Label1(__(MSG_SIZE)),
                    Child, heightpercentobj,
                End,
                Child, HVSpace,
            End,
            Child, HGroup,
                GroupFrameT(__(MSG_FILEREQUESTER)),
                Child, ColGroup(2),
                    Child, (drawersfirstobj = AslEditor__Checkmark()),
                    Child, LLabel1(__(MSG_DRAWERSFIRST)),
                    Child, HVSpace,
                    Child, HVSpace,
                End,
                Child, HVSpace,
            End,
        End,
        TAG_DONE
    );

    if (self != NULL)
    {
        data = INST_DATA(CLASS, self);
        
        D(bug("[AslEditor.class] %s: Self @ 0x%p, Data @ 0x%p\n", __PRETTY_FUNCTION__, self, data));

        data->drawersfirstobj = drawersfirstobj;

        data->positionobj = positionobj;
        data->offsetxobj = offsetxobj;
        data->offsetyobj = offsetyobj;

        data->sizeobj = sizeobj;
        data->heightpercentobj = heightpercentobj;
        data->widthpercentobj = widthpercentobj;

        DoMethod
        (
            drawersfirstobj, MUIM_Notify, MUIA_Selected, MUIV_EveryTime,
            (IPTR) self, 3, MUIM_Set, MUIA_PrefsEditor_Changed, TRUE
        );

        DoMethod
        (
            sizeobj, MUIM_Notify, MUIA_Cycle_Active, MUIV_EveryTime,
            (IPTR) self, 3, MUIM_CallHook, (IPTR)&updatedefaultshook, (IPTR)data
        );

        DoMethod
        (
            heightpercentobj, MUIM_Notify, MUIA_Selected, MUIV_EveryTime,
            (IPTR) self, 3, MUIM_CallHook, (IPTR)&updatedefaultshook, (IPTR)data
        );

        DoMethod
        (
            widthpercentobj, MUIM_Notify, MUIA_Selected, MUIV_EveryTime,
            (IPTR) self, 3, MUIM_CallHook, (IPTR)&updatedefaultshook, (IPTR)data
        );

        DoMethod
        (
            positionobj, MUIM_Notify, MUIA_Cycle_Active, MUIV_EveryTime,
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

        AslPrefs2Gadgets(data);
    }
    return (IPTR) self;

}

/*********************************************************************************************/

IPTR AslEditor__MUIM_PrefsEditor_ImportFH (
    Class *CLASS, Object *self,
    struct MUIP_PrefsEditor_ImportFH *message
)
{
    SETUP_INST_DATA;
    BOOL success = TRUE;

    D(bug("[AslEditor.class] %s()\n", __PRETTY_FUNCTION__));

    success = Prefs_ImportFH(message->fh);
    if (success) AslPrefs2Gadgets(data);

    return success;
}

/*********************************************************************************************/

IPTR AslEditor__MUIM_PrefsEditor_ExportFH
(
    Class *CLASS, Object *self,
    struct MUIP_PrefsEditor_ExportFH *message
)
{
    SETUP_INST_DATA;
    BOOL success = TRUE;

    D(bug("[AslEditor.class] %s()\n", __PRETTY_FUNCTION__));

    Gadgets2AslPrefs(data);
    success = Prefs_ExportFH(message->fh);

    return success;
}

/*********************************************************************************************/

IPTR AslEditor__MUIM_PrefsEditor_SetDefaults
(
    Class *CLASS, Object *self, Msg message
)
{
    SETUP_INST_DATA;
    BOOL success = TRUE;

    D(bug("[AslEditor.class] %s()\n", __PRETTY_FUNCTION__));

    success = Prefs_Default();
    if (success) AslPrefs2Gadgets(data);

    return success;
}

/*** Setup ******************************************************************/
ZUNE_CUSTOMCLASS_4
(
    AslEditor, NULL, MUIC_PrefsEditor, NULL,
    OM_NEW,                       struct opSet *,
    MUIM_PrefsEditor_ImportFH,    struct MUIP_PrefsEditor_ImportFH *,
    MUIM_PrefsEditor_ExportFH,    struct MUIP_PrefsEditor_ExportFH *,
    MUIM_PrefsEditor_SetDefaults, Msg
);

