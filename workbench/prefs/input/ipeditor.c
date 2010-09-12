/*
    Copyright  2003-2010, The AROS Development Team. All rights reserved.
    $Id$
*/

#define MUIMASTER_YES_INLINE_STDARG

#include <exec/types.h>
#include <utility/tagitem.h>
#include <libraries/asl.h>
#include <libraries/mui.h>
#include <prefs/prefhdr.h>
#include <prefs/input.h>
#include <zune/customclasses.h>
#include <zune/prefseditor.h>

#include <proto/alib.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/muimaster.h>
#include <proto/dos.h>
#include <proto/iffparse.h>

#include <string.h>
#include <stdio.h>

#define DEBUG 0
#include <aros/debug.h>

#include "misc.h"
#include "locale.h"
#include "ipeditor.h"
#include "prefs.h"
#include "stringify.h"

static    CONST_STRPTR InputTabs[] = {NULL, NULL, NULL, };
static    CONST_STRPTR MouseSpeed[] = {NULL, NULL, NULL, NULL, };

static struct Hook display_hook;

/*** Instance Data **********************************************************/
struct IPEditor_DATA
{
    Object *iped_KeyTypes;
    Object *iped_RepeatRate;
    Object *iped_RepeatDelay;
    Object *iped_Accelerated;
    Object *iped_MouseSpeed;
    Object *iped_DoubleClickDelay;
    Object *iped_LeftHandedMouse;
};

/*** Local Functions ********************************************************/
static BOOL InputPrefs2Gadgets(struct IPEditor_DATA *data);
static BOOL Gadgets2InputPrefs(struct IPEditor_DATA *data);

/*** Macros *****************************************************************/
#define SETUP_INST_DATA struct IPEditor_DATA *data = INST_DATA(CLASS, self)

/****************************************************************
 The display function for the KeyTypes listview
*****************************************************************/
static void keytypes_display_func(struct Hook *h, char **array, struct ListviewEntry * entry)
{
    *array++ = entry->displayflag;
    *array   = entry->node.ln_Name;
}

/*** Methods ****************************************************************/
Object *IPEditor__OM_NEW(Class *CLASS, Object *self, struct opSet *message)
{
    Object *keyTypes;
    Object *RepeatRate;
    Object *RepeatDelay;
    Object *Accelerated;
    Object *GadMouseSpeed;
    Object *DoubleClickDelay;
    Object *LeftHandedMouse;

    struct ListviewEntry *entry;

    display_hook.h_Entry = HookEntry;
    display_hook.h_SubEntry = (HOOKFUNC)keytypes_display_func;

    InputTabs[0] = _(MSG_GAD_TAB_KEYBOARD);
    InputTabs[1] = _(MSG_GAD_TAB_MOUSE);

    MouseSpeed[0] = _(MSG_GAD_MOUSE_SPEED_SLOW);
    MouseSpeed[1] = _(MSG_GAD_MOUSE_SPEED_NORMAL);
    MouseSpeed[2] = _(MSG_GAD_MOUSE_SPEED_FAST);

    self = (Object *) DoSuperNewTags
    (
        CLASS, self, NULL,

        MUIA_PrefsEditor_Name,     __(MSG_NAME),
        MUIA_PrefsEditor_Path,     (IPTR) "SYS/input.prefs",
        MUIA_PrefsEditor_IconTool, (IPTR) "SYS:Prefs/Input",

        Child, (IPTR) RegisterGroup(InputTabs),
            Child, (IPTR)VGroup,
                Child, (IPTR)HGroup,
                    Child, (IPTR)VGroup,
                        GroupFrameT(__(MSG_GAD_KEY_TYPE)),
                        MUIA_Weight, 45,
                        Child, (IPTR)ListviewObject,
                            MUIA_Listview_Input, FALSE,
                            MUIA_Listview_List, (IPTR)(keyTypes = (Object *)ListObject,
                                InputListFrame,
                                MUIA_List_AutoVisible, TRUE,
                                MUIA_List_MinLineHeight, 19,
                                MUIA_List_Format, (IPTR)"P=\033c,",
                                MUIA_List_DisplayHook, (IPTR)&display_hook,
                            End),
                        End,
                    End,
                    Child, (IPTR)VGroup,
                        MUIA_Weight, 55,
                        Child, (IPTR)HGroup,
                            GroupFrameT(__(MSG_GAD_KEY_REPEAT_RATE)),
                            Child, (IPTR)(RepeatRate = (Object *)StringifyObject,
                                MUIA_MyStringifyType, STRINGIFY_RepeatRate,
                                MUIA_Numeric_Value, 0,
                                MUIA_Numeric_Min, 0,
                                MUIA_Numeric_Max, 12,
                            End),
                        End,
                        Child, (IPTR)HGroup,
                            GroupFrameT(__(MSG_GAD_KEY_REPEAT_DELAY)),
                            Child, (IPTR)(RepeatDelay = (Object *)StringifyObject,
                                MUIA_MyStringifyType, STRINGIFY_RepeatDelay,
                                MUIA_Numeric_Value, 0,
                                MUIA_Numeric_Min, 0,
                                MUIA_Numeric_Max, 74,
                            End),
                        End,
                        Child, (IPTR)VGroup,
                            GroupFrameT(__(MSG_GAD_KEY_TEST)),
                            Child, (IPTR)HVSpace,
                            Child, (IPTR)StringObject,
                                StringFrame,
                            End,
                            Child, (IPTR)HVSpace,
                        End,
                    End,
                End,
            End,
            Child, VGroup,
                Child, VGroup,
                    GroupFrameT(__(MSG_GAD_MOUSE_SPEED)),
                    Child, (IPTR)(GadMouseSpeed = MUI_MakeObject(MUIO_Cycle, NULL, MouseSpeed)),
                    Child, HGroup,
                        Child, (IPTR)HSpace(0),
                        Child, (IPTR)Label1(__(MSG_GAD_MOUSE_ACCELERATED)),
                        Child, (IPTR)(Accelerated = MUI_MakeObject(MUIO_Checkmark, NULL)),
                    End,
                End,
                Child, VGroup,
                    GroupFrameT(__(MSG_GAD_MOUSE_BUTTON_SETTINGS)),
                    Child, ColGroup(2),
                        Child, (IPTR)Label1(__(MSG_GAD_MOUSE_DOUBLE_CLICK_DELAY)),
                        Child, (IPTR)(DoubleClickDelay = (Object *)StringifyObject,
                            MUIA_MyStringifyType, STRINGIFY_DoubleClickDelay,
                            MUIA_Numeric_Value, 0,
                            MUIA_Numeric_Min, 0,
                            MUIA_Numeric_Max, 199,
                        End),
                        Child, (IPTR)Label1(__(MSG_GAD_LEFT_HANDED_MOUSE)),
                        Child, HGroup,
                            Child, (IPTR)(LeftHandedMouse = MUI_MakeObject(MUIO_Checkmark, NULL)),
                            Child, (IPTR)HSpace(0),
                        End,
                    End,
                End,
            End,
        End,

        TAG_DONE
    );

    if (self != NULL)
    {
        SETUP_INST_DATA;

        data->iped_RepeatRate = RepeatRate;
        data->iped_RepeatDelay = RepeatDelay;
        data->iped_KeyTypes = keyTypes;
        data->iped_Accelerated = Accelerated;
        data->iped_MouseSpeed = GadMouseSpeed;
        data->iped_DoubleClickDelay = DoubleClickDelay;
        data->iped_LeftHandedMouse = LeftHandedMouse;

        IPTR root;

        ForeachNode(&keymap_list, entry)
        {
            root = DoMethod
            (
                keyTypes,
                MUIM_List_InsertSingle,
                (IPTR)entry,
                MUIV_List_Insert_Bottom
            );
        }

        DoMethod
        (
            RepeatRate, MUIM_Notify, MUIA_Numeric_Value, MUIV_EveryTime,
            (IPTR) self, 3, MUIM_Set, MUIA_PrefsEditor_Changed, TRUE
        );

        DoMethod
        (
            RepeatDelay, MUIM_Notify, MUIA_Numeric_Value, MUIV_EveryTime,
            (IPTR) self, 3, MUIM_Set, MUIA_PrefsEditor_Changed, TRUE
        );

        DoMethod
        (
            DoubleClickDelay, MUIM_Notify, MUIA_Numeric_Value, MUIV_EveryTime,
            (IPTR) self, 3, MUIM_Set, MUIA_PrefsEditor_Changed, TRUE
        );

        DoMethod
        (
            GadMouseSpeed, MUIM_Notify, MUIA_Cycle_Active, MUIV_EveryTime,
            (IPTR) self, 3, MUIM_Set, MUIA_PrefsEditor_Changed, TRUE
        );

        DoMethod
        (
            keyTypes, MUIM_Notify, MUIA_List_Active, MUIV_EveryTime,
            (IPTR) self, 3, MUIM_Set, MUIA_PrefsEditor_Changed, TRUE
        );

        DoMethod
        (
            Accelerated, MUIM_Notify, MUIA_Selected, MUIV_EveryTime,
            (IPTR) self, 3, MUIM_Set, MUIA_PrefsEditor_Changed, TRUE
        );

        DoMethod
        (
            LeftHandedMouse, MUIM_Notify, MUIA_Selected, MUIV_EveryTime,
            (IPTR) self, 3, MUIM_Set, MUIA_PrefsEditor_Changed, TRUE
        );
    }

    return self;
}

static BOOL Gadgets2InputPrefs(struct IPEditor_DATA *data)
{
    IPTR    val = 0;
    ULONG   micros, secs;

    GET(data->iped_RepeatRate, MUIA_Numeric_Value, &val);

    micros = (12 - val) * 20000;

    inputprefs.ip_KeyRptSpeed.tv_micro = micros;
    inputprefs.ip_KeyRptSpeed.tv_secs = 0;

    GET(data->iped_RepeatDelay, MUIA_Numeric_Value, &val);

    micros = 20000 + val * 20000;
    secs = (int) (micros / 1000000);
    micros = micros % 1000000;

    inputprefs.ip_KeyRptDelay.tv_micro = micros;
    inputprefs.ip_KeyRptDelay.tv_secs = secs;

    GET(data->iped_DoubleClickDelay, MUIA_Numeric_Value, &val);

    micros = 20000 + val * 20000;
    secs = (int) (micros / 1000000);
    micros = micros % 1000000;

    inputprefs.ip_DoubleClick.tv_micro = micros;
    inputprefs.ip_DoubleClick.tv_secs = secs;

    GET(data->iped_Accelerated, MUIA_Selected, &val);

    if (val != 0)
        inputprefs.ip_MouseAccel = ~0;
    else
        inputprefs.ip_MouseAccel = 0;

    GET(data->iped_MouseSpeed, MUIA_Cycle_Active, &val);

    inputprefs.ip_PointerTicks = 1 << (2 - val);

    struct ListviewEntry *entry;

    DoMethod
    (
        data->iped_KeyTypes, MUIM_List_GetEntry,
        MUIV_List_GetEntry_Active, &entry
    );

    if (entry != NULL)
    {
        D(bug("IPrefs: selected %s\n", entry->realname));
        strncpy(inputprefs.ip_Keymap, entry->realname, sizeof(inputprefs.ip_Keymap));
    }
    else
    {
        strncpy(inputprefs.ip_Keymap, DEFAULT_KEYMAP, sizeof(inputprefs.ip_Keymap));
    }

    GET(data->iped_LeftHandedMouse, MUIA_Selected, &val);
    inputprefs.ip_SwitchMouseButtons = val;

    return TRUE;
}

static BOOL InputPrefs2Gadgets(struct IPEditor_DATA *data)
{
    ULONG rrate = 12 -(inputprefs.ip_KeyRptSpeed.tv_micro / 20000);
    ULONG rdelay = ((inputprefs.ip_KeyRptDelay.tv_micro + (inputprefs.ip_KeyRptDelay.tv_secs * 1000000)) / 20000) - 1;
    ULONG dcdelay = ((inputprefs.ip_DoubleClick.tv_micro + (inputprefs.ip_DoubleClick.tv_secs * 1000000)) / 20000) - 1;

    NNSET(data->iped_RepeatRate, MUIA_Numeric_Value, (IPTR) rrate);
    NNSET(data->iped_RepeatDelay, MUIA_Numeric_Value, (IPTR) rdelay);
    NNSET(data->iped_DoubleClickDelay, MUIA_Numeric_Value, (IPTR) dcdelay);
    NNSET(data->iped_Accelerated, MUIA_Selected, (IPTR) (inputprefs.ip_MouseAccel != 0) ? TRUE : FALSE);

    struct ListviewEntry *entry;
    LONG pos = 0;

    NNSET(data->iped_KeyTypes, MUIA_List_Active, MUIV_List_Active_Off);
    ForeachNode(&keymap_list, entry)
    {
        if (!stricmp(inputprefs.ip_Keymap, entry->realname))
        {
            NNSET(data->iped_KeyTypes, MUIA_List_Active, pos);
            break;
        }

        ++pos;
    }

    IPTR    active = 0;

    switch(inputprefs.ip_PointerTicks)
    {
        case 3:
        case 4:
            active = 0;
            break;

        case 2:
            active = 1;
            break;

        case 1:
        default:
            active = 2;
            break;
    }

    NNSET(data->iped_MouseSpeed, MUIA_Cycle_Active, active);

    NNSET(data->iped_LeftHandedMouse, MUIA_Selected, inputprefs.ip_SwitchMouseButtons);

    return TRUE;
}

IPTR IPEditor__MUIM_PrefsEditor_ImportFH
(
    Class *CLASS, Object *self,
    struct MUIP_PrefsEditor_ImportFH *message
)
{
    SETUP_INST_DATA;

    if (Prefs_ImportFH(message->fh))
    {
        Prefs_Backup();
        IPTR back = InputPrefs2Gadgets(data);
        SET(self, MUIA_PrefsEditor_Changed, FALSE);
        SET(self, MUIA_PrefsEditor_Testing, FALSE);
        return back;
    }
    return FALSE;
}

IPTR IPEditor__MUIM_PrefsEditor_ExportFH
(
    Class *CLASS, Object *self,
    struct MUIP_PrefsEditor_ExportFH *message
)
{
    SETUP_INST_DATA;

    if (Gadgets2InputPrefs(data))
    {
        return Prefs_ExportFH(message->fh);
    }
    return FALSE;
}

IPTR IPEditor__MUIM_PrefsEditor_Test
(
    Class *CLASS, Object *self, Msg message
)
{

    SETUP_INST_DATA;

    Gadgets2InputPrefs(data);
    Prefs_Test();

    SET(self, MUIA_PrefsEditor_Changed, FALSE);
    SET(self, MUIA_PrefsEditor_Testing, TRUE);

    return TRUE;
}

IPTR IPEditor__MUIM_PrefsEditor_Revert
(
    Class *CLASS, Object *self, Msg message
)
{
    SETUP_INST_DATA;

    Prefs_Restore();
    InputPrefs2Gadgets(data);
    Prefs_Test();

    SET(self, MUIA_PrefsEditor_Changed, FALSE);
    SET(self, MUIA_PrefsEditor_Testing, FALSE);

    return TRUE;
}

IPTR IPEditor__MUIM_PrefsEditor_SetDefaults
(
    Class *CLASS, Object *self, Msg message
)
{
    SETUP_INST_DATA;
    BOOL success = TRUE;

    success = Prefs_Default();
    if (success)
    {
        InputPrefs2Gadgets(data);
        Prefs_Backup();
        Prefs_Test();
    }

    return success;
}

/*** Setup ******************************************************************/
ZUNE_CUSTOMCLASS_6
(
    IPEditor, NULL, MUIC_PrefsEditor, NULL,
    OM_NEW,                       struct opSet *,
    MUIM_PrefsEditor_ImportFH,    struct MUIP_PrefsEditor_ImportFH *,
    MUIM_PrefsEditor_ExportFH,    struct MUIP_PrefsEditor_ExportFH *,
    MUIM_PrefsEditor_Test,        Msg,
    MUIM_PrefsEditor_Revert,      Msg,
    MUIM_PrefsEditor_SetDefaults, Msg
);
