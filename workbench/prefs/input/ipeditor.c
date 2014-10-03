/*
    Copyright  2003-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#define MUIMASTER_YES_INLINE_STDARG

#include <devices/rawkeycodes.h>
#include <libraries/kms.h>
#include <mui/HotkeyString_mcc.h>
#include <zune/customclasses.h>
#include <zune/prefseditor.h>

#include <proto/alib.h>
#include <proto/dos.h>
#include <proto/commodities.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>

#define DEBUG 0
#include <aros/debug.h>

#include "misc.h"
#include "keymap.h"
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
    Object *iped_KeyTypesView;
    Object *iped_KeyTypes;
    Object *iped_DefKey;
    Object *iped_AltKey;
    Object *iped_SetDefKey;
    Object *iped_SetAltKey;
    Object *iped_RepeatRate;
    Object *iped_RepeatDelay;
    Object *iped_SwitchEnable;
    Object *iped_SwitchKey;
    Object *iped_Accelerated;
    Object *iped_MouseSpeed;
    Object *iped_DoubleClickDelay;
    Object *iped_LeftHandedMouse;

    struct Hook iped_setHook;
    struct Hook iped_setEnableHook;
    struct Hook iped_switchEnableHook;
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

AROS_UFH3(static void, setFunction,
	  AROS_UFHA(struct Hook *, h, A0),
	  AROS_UFHA(Object *, obj, A2),
	  AROS_UFHA(APTR *, msg, A1))
{
    AROS_USERFUNC_INIT

    struct IPEditor_DATA *data = h->h_Data;
    Object *kmobj = msg[0];
    struct ListviewEntry *entry = NULL;

    DoMethod(data->iped_KeyTypes, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, &entry);
    SET(kmobj, MUIA_Keymap_Keymap, entry);
    SET(obj, MUIA_PrefsEditor_Changed, TRUE);   

    AROS_USERFUNC_EXIT
}

AROS_UFH3(static void, setEnableFunction,
	  AROS_UFHA(struct Hook *, h, A0),
	  AROS_UFHA(Object *, obj, A2),
	  AROS_UFHA(APTR, msg, A1))
{
    AROS_USERFUNC_INIT

    struct IPEditor_DATA *data = h->h_Data;
    IPTR sw_enabled = FALSE;

    SET(data->iped_SetDefKey, MUIA_Disabled, FALSE);
    GET(data->iped_SwitchEnable, MUIA_Selected, &sw_enabled);
    SET(data->iped_SetAltKey, MUIA_Disabled, !sw_enabled);

    AROS_USERFUNC_EXIT
}

AROS_UFH3(static void, switchEnableFunction,
	  AROS_UFHA(struct Hook *, h, A0),
	  AROS_UFHA(Object *, obj, A2),
	  AROS_UFHA(APTR, msg, A1))
{
    AROS_USERFUNC_INIT

    struct IPEditor_DATA *data = h->h_Data;
    IPTR disabled = 0;
    IPTR active = 0;

    GET(data->iped_SwitchEnable, MUIA_Selected, &disabled);
    disabled = !disabled;
    SET(data->iped_SwitchKey, MUIA_Disabled, disabled);
    SET(data->iped_AltKey, MUIA_Disabled, disabled);
    GET(data->iped_KeyTypes, MUIA_List_Active, &active);
    SET(data->iped_SetAltKey, MUIA_Disabled, disabled || (active == MUIV_List_Active_Off));

    SET(obj, MUIA_PrefsEditor_Changed, TRUE);

    AROS_USERFUNC_EXIT
}



/*** Methods ****************************************************************/
Object *IPEditor__OM_NEW(Class *CLASS, Object *self, struct opSet *message)
{
    Object *keyTypesView;
    Object *keyTypes;
    Object *defKey;
    Object *altKey;
    Object *RepeatRate;
    Object *RepeatDelay;
    Object *switchEnable;
    Object *switchKey;
    Object *Accelerated;
    Object *GadMouseSpeed;
    Object *DoubleClickDelay;
    Object *LeftHandedMouse;
    Object *setDefKey;
    Object *setAltKey;

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
                        MUIA_Weight, 50,
                        Child, (IPTR)(keyTypesView = ListviewObject,
                            MUIA_Listview_List, (IPTR)(keyTypes = (Object *)ListObject,
                                InputListFrame,
                                MUIA_List_AutoVisible, TRUE,
                                MUIA_List_MinLineHeight, 19,
                                MUIA_List_Format, (IPTR)"P=\033c,",
                                MUIA_List_DisplayHook, (IPTR)&display_hook,
                            End),
                        End),
                        Child, (IPTR)ColGroup(3),
                            Child, (IPTR)Label1(__(MSG_GAD_KEY_DEF)),
                            Child, (IPTR)(defKey = KeymapObject,
                            	MUIA_Weight, 255,
                            End),
                            Child, (IPTR)(setDefKey = TextObject, 
                            	ButtonFrame,
				MUIA_Font, MUIV_Font_Button,
				MUIA_Text_HiCharIdx, '_',
				MUIA_Text_Contents, __(MSG_GAD_KEY_SET),
				MUIA_Text_PreParse, (IPTR)"\33c",
				MUIA_InputMode, MUIV_InputMode_RelVerify,
				MUIA_Background, MUII_ButtonBack,
				MUIA_CycleChain, TRUE,
				MUIA_Disabled, TRUE,
			    End),
                            Child, (IPTR)Label1(__(MSG_GAD_KEY_ALT)),
                            Child, (IPTR)(altKey = KeymapObject,
                            	MUIA_Weight, 255,
                            	MUIA_Disabled, TRUE,
                            End),
                            Child, (IPTR)(setAltKey = TextObject, 
                            	ButtonFrame,
				MUIA_Font, MUIV_Font_Button,
				MUIA_Text_HiCharIdx, '_',
				MUIA_Text_Contents, __(MSG_GAD_KEY_SET),
				MUIA_Text_PreParse, (IPTR)"\33c",
				MUIA_InputMode, MUIV_InputMode_RelVerify,
				MUIA_Background, MUII_ButtonBack,
				MUIA_CycleChain, TRUE,
				MUIA_Disabled, TRUE,
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
                        Child, (IPTR)ColGroup(2),
                            GroupFrameT(__(MSG_GAD_KEY_SWITCH)),
                            Child, (IPTR)Label1(__(MSG_GAD_KEY_SWITCH_ENABLE)),
                            Child, (IPTR)HGroup,
                            	Child, (IPTR)(switchEnable = MUI_MakeObject(MUIO_Checkmark, NULL)),
                            	Child, (IPTR)MUI_MakeObject(MUIO_HSpace, 0),
                            End,
                            Child, (IPTR)Label1(__(MSG_GAD_KEY_SWITCH_KEY)),
                            Child, (IPTR)(switchKey = MUI_NewObject("HotkeyString.mcc",
                                StringFrame,
                                MUIA_Disabled, TRUE,
                            TAG_DONE)),
                        End,
                        Child, (IPTR)VGroup,
                            GroupFrameT(__(MSG_GAD_KEY_TEST)),
                            Child, (IPTR)StringObject,
                                StringFrame,
                            End,
                        End,
                        Child, (IPTR)HVSpace,
                    End,
                End,
            End,
            Child, (IPTR)VGroup,
                Child, (IPTR)VGroup,
                    GroupFrameT(__(MSG_GAD_MOUSE_SPEED)),
                    Child, (IPTR)(GadMouseSpeed = MUI_MakeObject(MUIO_Cycle, NULL, MouseSpeed)),
                    Child, (IPTR)HGroup,
                        Child, (IPTR)HSpace(0),
                        Child, (IPTR)Label1(__(MSG_GAD_MOUSE_ACCELERATED)),
                        Child, (IPTR)(Accelerated = MUI_MakeObject(MUIO_Checkmark, NULL)),
                    End,
                End,
                Child, (IPTR)VGroup,
                    GroupFrameT(__(MSG_GAD_MOUSE_BUTTON_SETTINGS)),
                    Child, (IPTR)ColGroup(2),
                        Child, (IPTR)Label1(__(MSG_GAD_MOUSE_DOUBLE_CLICK_DELAY)),
                        Child, (IPTR)(DoubleClickDelay = (Object *)StringifyObject,
                            MUIA_MyStringifyType, STRINGIFY_DoubleClickDelay,
                            MUIA_Numeric_Value, 0,
                            MUIA_Numeric_Min, 0,
                            MUIA_Numeric_Max, 199,
                        End),
                        Child, (IPTR)Label1(__(MSG_GAD_LEFT_HANDED_MOUSE)),
                        Child, (IPTR)HGroup,
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
        data->iped_KeyTypesView = keyTypesView;
        data->iped_KeyTypes = keyTypes;
        data->iped_DefKey = defKey;
        data->iped_AltKey = altKey;
        data->iped_SetDefKey = setDefKey;
        data->iped_SetAltKey = setAltKey;
        data->iped_SwitchEnable = switchEnable;
        data->iped_SwitchKey = switchKey;
        data->iped_Accelerated = Accelerated;
        data->iped_MouseSpeed = GadMouseSpeed;
        data->iped_DoubleClickDelay = DoubleClickDelay;
        data->iped_LeftHandedMouse = LeftHandedMouse;

	data->iped_setHook.h_Entry = (HOOKFUNC)setFunction;
	data->iped_setHook.h_Data = data;
	data->iped_setEnableHook.h_Entry = (HOOKFUNC)setEnableFunction;
	data->iped_setEnableHook.h_Data = data;
	data->iped_switchEnableHook.h_Entry = (HOOKFUNC)switchEnableFunction;
	data->iped_switchEnableHook.h_Data = data;

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

            if (!root) {
                /* TODO: Do we want to check for an error? */
            }
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

        DoMethod(keyTypes, MUIM_Notify, MUIA_List_Active, MUIV_EveryTime,
                 self, 2, MUIM_CallHook, &data->iped_setEnableHook);

        DoMethod(setDefKey, MUIM_Notify, MUIA_Pressed, FALSE,
                 self, 3, MUIM_CallHook, &data->iped_setHook, defKey);

        DoMethod(keyTypesView, MUIM_Notify, MUIA_Listview_DoubleClick,
            MUIV_EveryTime, self, 3, MUIM_CallHook, &data->iped_setHook,
            defKey);

        DoMethod(setAltKey, MUIM_Notify, MUIA_Pressed, FALSE,
            	 self, 3, MUIM_CallHook, &data->iped_setHook, altKey);

        DoMethod(switchEnable, MUIM_Notify, MUIA_Selected, MUIV_EveryTime,
        	 self, 2, MUIM_CallHook, &data->iped_switchEnableHook);

	DoMethod(switchKey, MUIM_Notify, MUIA_String_Contents, MUIV_EveryTime,
		 self, 3, MUIM_Set, MUIA_PrefsEditor_Changed, TRUE);

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
    STRPTR key = NULL;
    struct InputXpression ix = {IX_VERSION, 0};

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

    struct ListviewEntry *entry = NULL;

    GET(data->iped_DefKey, MUIA_Keymap_Keymap, &entry);

    if (entry != NULL)
    {
        D(Printf("Gadgets2Prefs: Default keymap: %s\n", entry->realname));
        strncpy(inputprefs.ip_KeymapName, entry->realname, sizeof(inputprefs.ip_KeymapName));
    }
    else
    {
	strncpy(inputprefs.ip_Keymap, DEFAULT_KEYMAP, sizeof(inputprefs.ip_Keymap));
        strncpy(inputprefs.ip_KeymapName, DEFAULT_KEYMAP, sizeof(inputprefs.ip_KeymapName));
    }

    GET(data->iped_LeftHandedMouse, MUIA_Selected, &val);
    inputprefs.ip_SwitchMouseButtons = val;

    GET(data->iped_SwitchEnable, MUIA_Selected, &val);
    kmsprefs.kms_Enabled = val;

    GET(data->iped_AltKey, MUIA_Keymap_Keymap, &entry);
    if (entry)
	strncpy(kmsprefs.kms_AltKeymap, entry->realname, sizeof(kmsprefs.kms_AltKeymap));
    else
	kmsprefs.kms_AltKeymap[0] = 0;

    GET(data->iped_SwitchKey, MUIA_String_Contents, &key);
    if (ParseIX(key, &ix))
    {
	D(Printf("Gadgets2Prefs: IX parse error\n"));
	kmsprefs.kms_SwitchQual = KMS_QUAL_DISABLE;
	kmsprefs.kms_SwitchCode = KMS_CODE_NOKEY;
    }
    else
    {
	D(Printf("Gadgets2Prefs: Switch qualifier 0x%04lX, code 0x%04lX\n", ix.ix_Qualifier, ix.ix_Code));
        kmsprefs.kms_SwitchQual = ix.ix_Qualifier;
        /*
         * ParseIX() sets ix_Code to zero if the expressions consists only of qualifiers.
         * This can be considered a bug, because 0x00 is a valid code for RAWKEY_TILDE key.
         * This means that this key can't be a hotkey.
         * CHECKME: is it the same as in AmigaOS(tm), or it's AROS bug?
         */
	kmsprefs.kms_SwitchCode = ix.ix_Code ? ix.ix_Code : KMS_CODE_NOKEY;
    }

    return TRUE;
}

static BOOL InputPrefs2Gadgets(struct IPEditor_DATA *data)
{
    ULONG rrate = 12 -(inputprefs.ip_KeyRptSpeed.tv_micro / 20000);
    ULONG rdelay = ((inputprefs.ip_KeyRptDelay.tv_micro + (inputprefs.ip_KeyRptDelay.tv_secs * 1000000)) / 20000) - 1;
    ULONG dcdelay = ((inputprefs.ip_DoubleClick.tv_micro + (inputprefs.ip_DoubleClick.tv_secs * 1000000)) / 20000) - 1;
    char *keymap = inputprefs.ip_KeymapName;
    struct InputXpression ix =
    {
        IX_VERSION,
        IECLASS_RAWKEY,
	RAWKEY_LAMIGA,
	0xFFFF,
        kmsprefs.kms_SwitchQual,
        IX_NORMALQUALS,
        0
    };

    D(Printf("Prefs2Gadgets: Switch qualifier 0x%04lX, code 0x%04lX\n", kmsprefs.kms_SwitchQual, kmsprefs.kms_SwitchCode));

    /*
     * In order to specify qualifier-only hotkey any qualifier key code will do.
     * We use RAWKEY_LAMIGA for this purpose.
     * This is HotKeyString.mcc's feature.
     */
    if (kmsprefs.kms_SwitchCode != KMS_CODE_NOKEY)
    	ix.ix_Code = kmsprefs.kms_SwitchCode;

    if (!keymap[0])
    	keymap = inputprefs.ip_Keymap;

    NNSET(data->iped_RepeatRate, MUIA_Numeric_Value, (IPTR) rrate);
    NNSET(data->iped_RepeatDelay, MUIA_Numeric_Value, (IPTR) rdelay);
    NNSET(data->iped_DoubleClickDelay, MUIA_Numeric_Value, (IPTR) dcdelay);
    NNSET(data->iped_Accelerated, MUIA_Selected, (IPTR) (inputprefs.ip_MouseAccel != 0) ? TRUE : FALSE);

    struct ListviewEntry *entry;

    ForeachNode(&keymap_list, entry)
    {
        if (!stricmp(keymap, entry->realname))
            SET(data->iped_DefKey, MUIA_Keymap_Keymap, entry);
	if (!stricmp(kmsprefs.kms_AltKeymap, entry->realname))
	    SET(data->iped_AltKey, MUIA_Keymap_Keymap, entry);
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

    SET(data->iped_SwitchEnable, MUIA_Selected, kmsprefs.kms_Enabled);
    NNSET(data->iped_SwitchKey, MUIA_HotkeyString_IX, &ix);

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

IPTR IPEditor__MUIM_PrefsEditor_SetDefaults
(
    Class *CLASS, Object *self, Msg message
)
{
    SETUP_INST_DATA;
    BOOL success = TRUE;

    success = Prefs_Default();
    if (success)
        InputPrefs2Gadgets(data);

    return success;
}

/*** Setup ******************************************************************/
ZUNE_CUSTOMCLASS_4
(
    IPEditor, NULL, MUIC_PrefsEditor, NULL,
    OM_NEW,                       struct opSet *,
    MUIM_PrefsEditor_ImportFH,    struct MUIP_PrefsEditor_ImportFH *,
    MUIM_PrefsEditor_ExportFH,    struct MUIP_PrefsEditor_ExportFH *,
    MUIM_PrefsEditor_SetDefaults, Msg
);
