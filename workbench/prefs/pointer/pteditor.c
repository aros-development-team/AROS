/*
    Copyright © 2010-2011, The AROS Development Team. All rights reserved.
    $Id$
*/

#define MUIMASTER_YES_INLINE_STDARG

#include <libraries/asl.h>

// #define DEBUG 1
#include <zune/customclasses.h>
#include <zune/prefseditor.h>

#include <proto/alib.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>

#include <aros/debug.h>

#include "locale.h"
#include "ppreview.h"
#include "pteditor.h"
#include "prefs.h"

/*** Instance Data **********************************************************/
struct PTEditor_DATA
{
    ULONG   pded_oldentry;
    Object *pted_previewImage;
    Object *pted_typeCycle;
    Object *pted_fileString;
    Object *pted_alphaSlider;
    Object *pted_hotspotButton;
    struct Hook pted_cycleHook;
    struct Hook pted_filenameHook;
};

STATIC VOID PTPrefs2Gadgets(struct PTEditor_DATA *data);
STATIC VOID Gadgets2PTPrefs(struct PTEditor_DATA *data);

/*** Macros *****************************************************************/
#define SETUP_INST_DATA struct PTEditor_DATA *data = INST_DATA(CLASS, self)

/*** Hooks ******************************************************************/
AROS_UFH3(VOID, cycleFunction,
AROS_UFHA(struct Hook *, h, A0),
AROS_UFHA(Object *, obj, A2),
AROS_UFHA(APTR, msg, A1))
{
    AROS_USERFUNC_INIT

    struct PTEditor_DATA *data = h->h_Data;

    ULONG entry = XGET(data->pted_typeCycle, MUIA_Cycle_Active);

    D(bug("[POINTERPREF] entry %d oldentry %d\n", entry, data->pded_oldentry));

    // store data from previous entry
    strlcpy
    (
        pointerprefs[data->pded_oldentry].filename,
        (STRPTR)XGET(data->pted_fileString, MUIA_String_Contents),
        NAMEBUFLEN
    );
    pointerprefs[data->pded_oldentry].npp.npp_AlphaValue =
        XGET(data->pted_alphaSlider, MUIA_Numeric_Value) * 0x0101;
    pointerprefs[data->pded_oldentry].npp.npp_X =
        - XGET(data->pted_previewImage, MUIA_PPreview_HSpotX);
    pointerprefs[data->pded_oldentry].npp.npp_Y =
        - XGET(data->pted_previewImage, MUIA_PPreview_HSpotY);

    // set data of current entry
    NNSET(data->pted_fileString,  MUIA_String_Contents, pointerprefs[entry].filename);
    NNSET(data->pted_alphaSlider, MUIA_Numeric_Value,   pointerprefs[entry].npp.npp_AlphaValue >> 8);

    SetAttrs
    (
        data->pted_previewImage,
        MUIA_PPreview_FileName, pointerprefs[entry].filename,
        MUIA_PPreview_Alpha,    pointerprefs[entry].npp.npp_AlphaValue,
        MUIA_PPreview_HSpotX,   - pointerprefs[entry].npp.npp_X,
        MUIA_PPreview_HSpotY,   - pointerprefs[entry].npp.npp_Y
    );

    data->pded_oldentry = entry;

    AROS_USERFUNC_EXIT
}

AROS_UFH3(VOID, filenameFunction,
AROS_UFHA(struct Hook *, h, A0),
AROS_UFHA(Object *, obj, A2),
AROS_UFHA(APTR, msg, A1))
{
    AROS_USERFUNC_INIT

    struct PTEditor_DATA *data = h->h_Data;

    STRPTR filename = (STRPTR)XGET(data->pted_fileString, MUIA_String_Contents);
    SET(data->pted_previewImage, MUIA_PPreview_FileName, filename);
    SET(obj, MUIA_PrefsEditor_Changed, TRUE);

    AROS_USERFUNC_EXIT
}

/*** Methods ****************************************************************/
Object *PTEditor__OM_NEW(Class *CLASS, Object *self, struct opSet *message)
{
    static CONST_STRPTR type_entries[3];
    type_entries[0] = (CONST_STRPTR) __(MSG_TYPE_NORMAL);
    type_entries[1] = (CONST_STRPTR) __(MSG_TYPE_BUSY);
    type_entries[2] = NULL;

    Object *previewImage, *typeCycle, *fileString, *alphaSlider, *hotspotButton;

    self = (Object *) DoSuperNewTags
    (
        CLASS, self, NULL,
        MUIA_PrefsEditor_Name, _(MSG_WINTITLE),
        MUIA_PrefsEditor_Path, (IPTR)"SYS/pointer.prefs",
        MUIA_PrefsEditor_IconTool, (IPTR)"SYS:Prefs/Pointer",

        Child, HGroup,
            Child, (IPTR)(previewImage = (Object *)PPreviewObject,
                GroupFrame,
            End),
            Child, (IPTR)ColGroup(2),
                GroupFrame,
                Child, (IPTR)Label2(__(MSG_TYPE)),
                Child, (IPTR)(typeCycle = (Object *)CycleObject,
                    MUIA_Cycle_Entries, (IPTR)type_entries,
                    MUIA_CycleChain, 1,
                End),
                Child, (IPTR)Label2(__(MSG_FILENAME)),
                Child, (IPTR)PopaslObject,
                    MUIA_Popasl_Type, ASL_FileRequest,
                    MUIA_Popstring_String, (IPTR)(fileString = (Object *)StringObject,
                        MUIA_String_MaxLen, NAMEBUFLEN,
                        StringFrame,
                        MUIA_CycleChain, 1,
                    End),
                    MUIA_Popstring_Button, (IPTR)PopButton(MUII_PopFile),
                End,
                Child, (IPTR)Label2(__(MSG_ALPHA)),
                Child, (IPTR)(alphaSlider = (Object *)SliderObject,
                    MUIA_Numeric_Min, 20,
                    MUIA_Numeric_Max, 255,
                    MUIA_CycleChain, 1,
                End),
                Child, (IPTR)Label2(__(MSG_HOTSPOT)),
                Child, (IPTR)(hotspotButton = MUI_NewObject(MUIC_Text,
                    ButtonFrame,
                    MUIA_Font, MUIV_Font_Button,
                    MUIA_Text_Contents, (IPTR)__(MSG_SETVIEW),
                    MUIA_Text_PreParse, (IPTR)"\33c",
                    MUIA_InputMode    , MUIV_InputMode_Toggle,
                    MUIA_Background   , MUII_ButtonBack,
                    MUIA_CycleChain   , 1,
                TAG_DONE)),
            End,
        End,
        TAG_DONE
    );

    if (self)
    {
        SETUP_INST_DATA;

        data->pted_cycleHook.h_Entry = (HOOKFUNC)cycleFunction;
        data->pted_cycleHook.h_Data  = data;

        data->pted_filenameHook.h_Entry = (HOOKFUNC)filenameFunction;
        data->pted_filenameHook.h_Data  = data;

        data->pted_previewImage = previewImage;
        data->pted_typeCycle    = typeCycle;
        data->pted_fileString   = fileString;
        data->pted_alphaSlider  = alphaSlider;
        data->pted_hotspotButton= hotspotButton;

        DoMethod
        (
            data->pted_typeCycle, MUIM_Notify, MUIA_Cycle_Active, MUIV_EveryTime,
            (IPTR) self, 2, MUIM_CallHook, &data->pted_cycleHook
        );

        DoMethod
        (
            data->pted_fileString, MUIM_Notify, MUIA_String_Acknowledge, MUIV_EveryTime,
            (IPTR) self, 2, MUIM_CallHook, &data->pted_filenameHook
        );

        DoMethod
        (
            data->pted_alphaSlider, MUIM_Notify, MUIA_Numeric_Value, MUIV_EveryTime,
            (IPTR) self, 3, MUIM_Set, MUIA_PrefsEditor_Changed, TRUE
        );

        DoMethod
        (
            data->pted_hotspotButton, MUIM_Notify, MUIA_Selected, MUIV_EveryTime,
            (IPTR) data->pted_previewImage, 3, MUIM_Set, MUIA_PPreview_SetHSpot, MUIV_TriggerValue
        );

    }

    return self;
}


STATIC void Gadgets2PTPrefs(struct PTEditor_DATA *data)
{
    ULONG entry = XGET(data->pted_typeCycle, MUIA_Cycle_Active);

    // Trigger cycleHook
    SET(data->pted_typeCycle, MUIA_Cycle_Active, entry);

}

STATIC VOID PTPrefs2Gadgets(struct PTEditor_DATA *data)
{
    ULONG entry = XGET(data->pted_typeCycle, MUIA_Cycle_Active);

    NNSET(data->pted_fileString, MUIA_String_Contents, pointerprefs[entry].filename);
    NNSET(data->pted_alphaSlider, MUIA_Numeric_Value, pointerprefs[entry].npp.npp_AlphaValue >> 8);

    SetAttrs
    (
        data->pted_previewImage,
        MUIA_PPreview_FileName, pointerprefs[entry].filename,
        MUIA_PPreview_Alpha,    pointerprefs[entry].npp.npp_AlphaValue,
        MUIA_PPreview_HSpotX,   - pointerprefs[entry].npp.npp_X,
        MUIA_PPreview_HSpotY,   - pointerprefs[entry].npp.npp_Y
    );
}

IPTR PTEditor__MUIM_PrefsEditor_ImportFH
(
    Class *CLASS, Object *self,
    struct MUIP_PrefsEditor_ImportFH *message
)
{
    SETUP_INST_DATA;
    BOOL success = TRUE;

    success = Prefs_ImportFH(message->fh);
    if (success) PTPrefs2Gadgets(data);

    return success;
}

IPTR PTEditor__MUIM_PrefsEditor_ExportFH
(
    Class *CLASS, Object *self,
    struct MUIP_PrefsEditor_ExportFH *message
)
{
    SETUP_INST_DATA;
    BOOL success = TRUE;

    Gadgets2PTPrefs(data);
    success = Prefs_ExportFH(message->fh);

    return success;
}

IPTR PTEditor__MUIM_PrefsEditor_SetDefaults
(
    Class *CLASS, Object *self, Msg message
)
{
    SETUP_INST_DATA;
    BOOL success = TRUE;

    success = Prefs_Default();
    if (success) PTPrefs2Gadgets(data);

    return success;
}

/*** Setup ******************************************************************/
ZUNE_CUSTOMCLASS_4
(
    PTEditor, NULL, MUIC_PrefsEditor, NULL,
    OM_NEW,                       struct opSet *,
    MUIM_PrefsEditor_ImportFH,    struct MUIP_PrefsEditor_ImportFH *,
    MUIM_PrefsEditor_ExportFH,    struct MUIP_PrefsEditor_ExportFH *,
    MUIM_PrefsEditor_SetDefaults, Msg
);
