/*
    Copyright  2010, The AROS Development Team. All rights reserved.
    $Id$
*/

#define MUIMASTER_YES_INLINE_STDARG

#include <exec/types.h>
#include <utility/tagitem.h>
#include <libraries/asl.h>
#include <libraries/mui.h>
#include <prefs/pointer.h>

#define DEBUG 1
#include <zune/customclasses.h>
#include <zune/prefseditor.h>

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/muimaster.h>
#include <proto/dos.h>
#include <proto/iffparse.h>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <aros/debug.h>

#include "locale.h"
#include "ppreview.h"
#include "pteditor.h"
#include "prefs.h"

static CONST_STRPTR type_entries[] = {"Normal", "Busy", NULL};

/*** Instance Data **********************************************************/
struct PTEditor_DATA
{
    ULONG oldentry;
    Object *pted_previewImage;
    Object *pted_typeCycle;
    Object *pted_fileString;
    Object *pted_spotButton;
    Object *pted_alphaSlider;
    struct Hook cycleHook;
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

    D(bug("[POINTERPREF] entry %d oldentry %d\n", entry, data->oldentry));

    // store data from previous entry
    strlcpy
    (
        pointerprefs[data->oldentry].filename,
        (STRPTR)XGET(data->pted_fileString, MUIA_String_Contents),
        NAMEBUFLEN
    );
    pointerprefs[data->oldentry].npp.npp_AlphaValue = XGET(data->pted_alphaSlider, MUIA_Numeric_Value);

    // set data of current entry
    NNSET(data->pted_fileString, MUIA_String_Contents, pointerprefs[entry].filename);
    NNSET(data->pted_alphaSlider, MUIA_Numeric_Value, pointerprefs[entry].npp.npp_AlphaValue);

    data->oldentry = entry;

    D(bug("[POINTERPREF/cycleFunction] name 1 %s\nname 2 %s\n", pointerprefs[0].filename, pointerprefs[1].filename));

    AROS_USERFUNC_EXIT
}

/*** Methods ****************************************************************/
Object *PTEditor__OM_NEW(Class *CLASS, Object *self, struct opSet *message)
{
    Object *previewImage, *typeCycle, *fileString, *spotButton, *alphaSlider;

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
            Child, ColGroup(2),
                GroupFrame,
                Child, (IPTR)Label2("Type"),
                Child, (IPTR)(typeCycle = (Object *)CycleObject,
                    MUIA_Cycle_Entries, type_entries,
                End),
                Child, (IPTR)Label2("Load"),
                Child, (IPTR)PopaslObject,
                    MUIA_Popasl_Type, ASL_FileRequest,
                    MUIA_Popstring_String, (IPTR)(fileString = (Object *)StringObject,
                        MUIA_String_MaxLen, NAMEBUFLEN,
                        StringFrame,
                    End),
                    MUIA_Popstring_Button, (IPTR)PopButton(MUII_PopUp),
                End,

                Child, (IPTR)HVSpace,
                Child, (IPTR)(spotButton = SimpleButton("Set Hotspot")),
                Child, (IPTR)Label2("Alpha"),
                Child, (IPTR)(alphaSlider = (Object *)SliderObject,
                    MUIA_Numeric_Min, 0,
                    MUIA_Numeric_Max, 255,
                End),
            End,
        End,
        TAG_DONE
    );

    if (self)
    {
        SETUP_INST_DATA;

        data->cycleHook.h_Entry = (HOOKFUNC)cycleFunction;
        data->cycleHook.h_Data = data;

        data->pted_previewImage = previewImage;
        data->pted_typeCycle    = typeCycle;
        data->pted_fileString   = fileString;
        data->pted_spotButton   = spotButton;
        data->pted_alphaSlider  = alphaSlider;

        DoMethod
        (
            data->pted_typeCycle, MUIM_Notify, MUIA_Cycle_Active, MUIV_EveryTime,
            (IPTR) self, 2, MUIM_CallHook, &data->cycleHook
        );

        DoMethod
        (
            data->pted_fileString, MUIM_Notify, MUIA_String_Acknowledge, MUIV_EveryTime,
            (IPTR) self, 3, MUIM_Set, MUIA_PrefsEditor_Changed, TRUE
        );

        DoMethod
        (
            data->pted_spotButton, MUIM_Notify, MUIA_Pressed, FALSE,
            (IPTR) self, 3, MUIM_Set, MUIA_PrefsEditor_Changed, TRUE
        );

        DoMethod
        (
            data->pted_alphaSlider, MUIM_Notify, MUIA_Numeric_Value, MUIV_EveryTime,
            (IPTR) self, 3, MUIM_Set, MUIA_PrefsEditor_Changed, TRUE
        );

    }

    return self;
}


STATIC void Gadgets2PTPrefs(struct PTEditor_DATA *data)
{
    ULONG entry = XGET(data->pted_typeCycle, MUIA_Cycle_Active);

    // Trigger cycleHook
    SET(data->pted_typeCycle, MUIA_Cycle_Active, entry);

    D(bug("[POINTERPREF/gadgets2prefs] name 1 %s\nname 2 %s\n", pointerprefs[0].filename, pointerprefs[1].filename));

}

STATIC VOID PTPrefs2Gadgets(struct PTEditor_DATA *data)
{
    ULONG entry = XGET(data->pted_typeCycle, MUIA_Cycle_Active);

    NNSET(data->pted_fileString, MUIA_String_Contents, pointerprefs[entry].filename);
    NNSET(data->pted_alphaSlider, MUIA_Numeric_Value, pointerprefs[entry].npp.npp_AlphaValue);

    D(bug("[POINTERPREF/prefs2gadget] name 1 %s\nname 2 %s\n", pointerprefs[0].filename, pointerprefs[1].filename));
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

    D(bug("[POINTERPREF/importFH] name 1 %s\nname 2 %s\n", pointerprefs[0].filename, pointerprefs[1].filename));

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

    D(bug("[POINTERPREF/ExportFH] name 1 %s\nname 2 %s\n", pointerprefs[0].filename, pointerprefs[1].filename));

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
