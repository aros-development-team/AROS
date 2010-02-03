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
/* #define DEBUG 1 */
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
    Object *pted_previewImage;
    Object *pted_typeCycle;
    Object *pted_fileString;
    Object *pted_spotButton;
    Object *pted_alphaSlider;
};

STATIC VOID PTPrefs2Gadgets(struct PTEditor_DATA *data);
STATIC VOID Gadgets2PTPrefs(struct PTEditor_DATA *data);

/*** Macros *****************************************************************/
#define SETUP_INST_DATA struct PTEditor_DATA *data = INST_DATA(CLASS, self)

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
                    ASLFO_MaxHeight, 100,
                    MUIA_Popstring_String, (IPTR)(fileString = (Object *)StringObject,
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

        data->pted_previewImage = previewImage;
        data->pted_typeCycle    = typeCycle;
        data->pted_fileString   = fileString;
        data->pted_spotButton   = spotButton;
        data->pted_alphaSlider  = alphaSlider;
#if 0
        DoMethod
        (
            data->baudrate, MUIM_Notify, MUIA_Cycle_Active, MUIV_EveryTime,
            (IPTR) self, 3, MUIM_Set, MUIA_PrefsEditor_Changed, TRUE
        );
#endif

        PTPrefs2Gadgets(data);
    }

    return self;
}


STATIC void Gadgets2PTPrefs(struct PTEditor_DATA *data)
{
#if 0
    serialprefs.sp_BitsPerChar = XGET(data->databits, MUIA_Cycle_Active);
#endif
}

STATIC VOID PTPrefs2Gadgets(struct PTEditor_DATA *data)
{
#if 0
    NNSET(data->databits, MUIA_Cycle_Active, serialprefs.sp_BitsPerChar);
#endif
}

IPTR PTEditor__MUIM_PrefsEditor_ImportFH (
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

    D(bug("[seredit class] SerEdit Class Export\n"));

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

    D(bug("[seredit class] SerEdit Class SetDefaults\n"));

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
