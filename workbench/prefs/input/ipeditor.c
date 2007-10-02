/*
    Copyright  2003-2004, The AROS Development Team. All rights reserved.
    $Id: ipeditor.c 21816 2007-09-25 12:35:29Z chodorowski, dariusb $
*/

// #define MUIMASTER_YES_INLINE_STDARG

#include <exec/types.h>
#include <utility/tagitem.h>
#include <libraries/asl.h>
#include <libraries/mui.h>
#include <prefs/prefhdr.h>
#include <prefs/input.h>
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
#include <aros/debug.h>

#include <mui/NListtree_mcc.h>
#include <mui/NListview_mcc.h>

#include "misc.h"
#include "locale.h"
#include "ipeditor.h"
#include "prefs.h"

extern struct List          keymap_list;
extern struct InputPrefs    inputprefs;
extern struct InputPrefs    restore_prefs;
struct Hook                 keytypes_display_hook;

extern struct MUI_CustomClass *StringifyClass;

static    STRPTR InputTabs[] = {NULL, NULL, NULL, };
static    STRPTR ButtonMappings[] = {NULL, NULL, NULL, NULL, };
static    STRPTR MouseSpeed[] = {NULL, NULL, NULL, NULL, };

/*** Instance Data **********************************************************/

struct IPEditor_DATA
{
    struct InputPrefs  iped_InputPrefs;
    Object *iped_KeyTypes;
    Object *iped_RepeatRate;
    Object *iped_RepeatDelay;
    Object *iped_Accelerated;
    Object *iped_MouseSpeed;
    Object *iped_DoubleClickDelay;
};

/*** Macros *****************************************************************/
#define SETUP_INST_DATA struct IPEditor_DATA *data = INST_DATA(CLASS, self)

/****************************************************************
 The display function for the KeyTypes listview
*****************************************************************/
AROS_UFH3
(
    void, keytypes_display_func,
    AROS_UFHA(struct Hook *, h, A0),
    AROS_UFHA(ULONG, nop, A2),
    AROS_UFHA(struct MUIP_NListtree_DisplayMessage *, msg, A1)
)
{
    AROS_USERFUNC_INIT

    if ( msg->TreeNode != NULL )
    {
        struct ListviewEntry *entry;

        entry = (struct ListviewEntry *) msg->TreeNode->tn_Name;
        
        *msg->Array = &entry->displayname;

        if ((msg->TreeNode->tn_Flags & TNF_SELECTED) == TNF_SELECTED) *msg->Preparse = (STRPTR) "\033b";

    }

    AROS_USERFUNC_EXIT

}
//     struct  MUI_NListtree_TreeNode *TreeNode;
//     LONG    EntryPos;
//     STRPTR  *Array;
//     STRPTR  *Preparse;
/*** Methods ****************************************************************/
Object *IPEditor__OM_NEW(Class *CLASS, Object *self, struct opSet *message)
{

    Object *keyTypes;
    Object *RepeatRate;
    Object *RepeatDelay;
    Object *Accelerated;
    Object *GadMouseSpeed;
    Object *DoubleClickDelay;

    struct ListviewEntry *entry;

    keytypes_display_hook.h_Entry = (HOOKFUNC)keytypes_display_func;

    InputTabs[0] = __(MSG_GAD_TAB_KEYBOARD);
    InputTabs[1] = __(MSG_GAD_TAB_MOUSE);

    ButtonMappings[0] = __(MSG_GAD_MOUSE_MAPPING_SELECT);
    ButtonMappings[1] = __(MSG_GAD_MOUSE_MAPPING_MENU);
    ButtonMappings[2] = __(MSG_GAD_MOUSE_MAPPING_THIRD);

    MouseSpeed[0] = __(MSG_GAD_MOUSE_SPEED_SLOW);
    MouseSpeed[1] = __(MSG_GAD_MOUSE_SPEED_NORMAL);
    MouseSpeed[2] = __(MSG_GAD_MOUSE_SPEED_FAST);

    self = (Object *) DoSuperNewTags
    (
        CLASS, self, NULL,
        
        MUIA_PrefsEditor_Name,        __(MSG_NAME),
        MUIA_PrefsEditor_Path, (IPTR) "SYS/Input.prefs",
        
        Child, (IPTR) RegisterGroup(InputTabs),
            Child, VGroup,
                Child, HGroup,
                    Child, VGroup,
                        GroupFrameT(__(MSG_GAD_KEY_TYPE)),
                        MUIA_Weight, 40,
                        Child, NListviewObject,
                            MUIA_NListview_NList,	keyTypes = NListtreeObject,
                                InputListFrame,
                                MUIA_NListtree_EmptyNodes, FALSE,
                                MUIA_NListtree_AutoVisible, MUIV_NListtree_AutoVisible_Expand,
                                MUIM_NListtree_Select, MUIV_NListtree_Select_Active,
                                MUIA_NListtree_DupNodeName, FALSE,
                                MUIA_NListtree_DragDropSort, FALSE,
                                MUIA_NListtree_MultiSelect, MUIV_NListtree_MultiSelect_None,
                                MUIA_NListtree_DisplayHook, (IPTR) &keytypes_display_hook,
                            End,
                        End,
                    End,
                    Child, VGroup,
                        MUIA_Weight, 60,
                        Child, HGroup,
                            GroupFrameT(__(MSG_GAD_KEY_REPEAT_RATE)),
                            Child, RepeatRate = NewObject(StringifyClass->mcc_Class,0,
                                MUIA_MyStringifyType,    STRINGIFY_RepeatRate,
                                MUIA_Numeric_Value, 0,
                                MUIA_Numeric_Min, 0,
                                MUIA_Numeric_Max, 12,
                            TAG_DONE),
                        End,
                        Child, HGroup,
                            GroupFrameT(__(MSG_GAD_KEY_REPEAT_DELAY)),
                            Child, RepeatDelay = NewObject(StringifyClass->mcc_Class,0,
                                MUIA_MyStringifyType,    STRINGIFY_RepeatDelay,
                                MUIA_Numeric_Value, 0,
                                MUIA_Numeric_Min, 0,
                                MUIA_Numeric_Max, 74,
                            TAG_DONE),
                        End,
                        Child, VGroup,
                            GroupFrameT(__(MSG_GAD_KEY_TEST)),
                            Child, HVSpace,
                            Child, StringObject,
                                StringFrame,
                            End,
                            Child, HVSpace,
                        End,
                    End,
                End,
            End,
            Child, VGroup,
                Child, HGroup,
                    Child, VGroup,
                        Child, ColGroup(2),
                            GroupFrameT(__(MSG_GAD_MOUSE_BUTTON_MAPPING)),
                            Child, Label1(__(MSG_GAD_MOUSE_LEFT)),
                            Child, MUI_MakeObject(MUIO_Cycle, NULL, ButtonMappings),
                            Child, Label1(__(MSG_GAD_MOUSE_MIDDLE)),
                            Child, MUI_MakeObject(MUIO_Cycle, NULL, ButtonMappings),
                            Child, Label1(__(MSG_GAD_MOUSE_RIGHT)),
                            Child, MUI_MakeObject(MUIO_Cycle, NULL, ButtonMappings),
                        End,
                        Child, HVSpace,
                    End,
                    Child, VGroup,
                        Child, ColGroup(2),
                            GroupFrameT(__(MSG_GAD_MOUSE_SPEED)),
                            Child, GadMouseSpeed = MUI_MakeObject(MUIO_Cycle, NULL, MouseSpeed),
                            Child, HVSpace,
                            Child, Label1(__(MSG_GAD_MOUSE_ACCELERATED)),
                            Child, Accelerated = MUI_MakeObject(MUIO_Checkmark, NULL),
                        End,
                        Child, HGroup,
                            GroupFrameT(__(MSG_GAD_MOUSE_DOUBLE_CLICK_DELAY)),
                            Child, DoubleClickDelay = NewObject(StringifyClass->mcc_Class,0,
                                MUIA_MyStringifyType,    STRINGIFY_DoubleClickDelay,
                                MUIA_Numeric_Value, 0,
                                MUIA_Numeric_Min, 0,
                                MUIA_Numeric_Max, 199,
                            TAG_DONE),
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

        IPTR    pos = MUIV_NListtree_Insert_ListNode_Root;
        IPTR    root = MUIV_NListtree_Insert_ListNode_Root;
        IPTR    Flags;
        ForeachNode(&keymap_list, entry)
        {
            Flags = 0;
            if (entry->modelnode) {pos = MUIV_NListtree_Insert_ListNode_Root; Flags = TNF_LIST; }

            root = DoMethod(keyTypes, MUIM_NListtree_Insert, entry, 0, pos, MUIV_NListtree_Insert_PrevNode_Tail, Flags);
            entry->Node = root;

            if (pos == MUIV_NListtree_Insert_ListNode_Root) pos = root;

        }

        DoMethod(RepeatRate, MUIM_Notify, MUIA_Numeric_Value, MUIV_EveryTime, (IPTR) self, 3, MUIM_Set, MUIA_PrefsEditor_Changed, TRUE);
        DoMethod(RepeatDelay, MUIM_Notify, MUIA_Numeric_Value, MUIV_EveryTime, (IPTR) self, 3, MUIM_Set, MUIA_PrefsEditor_Changed, TRUE);
        DoMethod(DoubleClickDelay, MUIM_Notify, MUIA_Numeric_Value, MUIV_EveryTime, (IPTR) self, 3, MUIM_Set, MUIA_PrefsEditor_Changed, TRUE);

        DoMethod(GadMouseSpeed, MUIM_Notify, MUIA_Cycle_Active, MUIV_EveryTime, (IPTR) self, 3, MUIM_Set, MUIA_PrefsEditor_Changed, TRUE);

        DoMethod(keyTypes, MUIM_Notify, MUIA_NListtree_Active, MUIV_EveryTime, (IPTR) self, 3, MUIM_Set, MUIA_PrefsEditor_Changed, TRUE);

        DoMethod(Accelerated, MUIM_Notify, MUIA_Selected, MUIV_EveryTime, (IPTR) self, 3, MUIM_Set, MUIA_PrefsEditor_Changed, TRUE);

    }
    
    return self;
}

BOOL Gadgets2InputPrefs
(
        struct IPEditor_DATA *data
)
{
    IPTR    val;
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

    if (val != 0) inputprefs.ip_MouseAccel = ~0; else inputprefs.ip_MouseAccel = 0;

    GET(data->iped_MouseSpeed, MUIA_Cycle_Active, &val);

    inputprefs.ip_PointerTicks = 1 << (2 - val);

    struct MUI_NListtree_TreeNode *node;

    GET(data->iped_KeyTypes, MUIA_NListtree_Active, &node);

    if (val != NULL)
    {
        struct ListviewEntry *kmn = (struct ListviewEntry *) node->tn_Name;
        if (kmn != NULL)
        {
            strncpy(inputprefs.ip_Keymap, kmn->realname, sizeof(inputprefs.ip_Keymap));
        }
    }

    return TRUE;
}

BOOL InputPrefs2Gadgets
(
    struct IPEditor_DATA *data
)
{
    ULONG rrate = 12 -(inputprefs.ip_KeyRptSpeed.tv_micro / 20000);
    ULONG rdelay = ((inputprefs.ip_KeyRptDelay.tv_micro + (inputprefs.ip_KeyRptDelay.tv_secs * 1000000)) / 20000) - 1;
    ULONG dcdelay = ((inputprefs.ip_DoubleClick.tv_micro + (inputprefs.ip_DoubleClick.tv_secs * 1000000)) / 20000) - 1;
    
    NNSET(data->iped_RepeatRate, MUIA_Numeric_Value, (IPTR) rrate);
    NNSET(data->iped_RepeatDelay, MUIA_Numeric_Value, (IPTR) rdelay);
    NNSET(data->iped_DoubleClickDelay, MUIA_Numeric_Value, (IPTR) dcdelay);
    NNSET(data->iped_Accelerated, MUIA_Selected, (IPTR) (inputprefs.ip_MouseAccel != 0) ? TRUE : FALSE);

    struct ListviewEntry *entry;
    struct ListviewEntry *kmn = NULL;

    ForeachNode(&keymap_list, entry)
    {

        if (!stricmp(inputprefs.ip_Keymap, entry->realname))
        {
            kmn = entry;
            break;
        }
    }

    if (kmn != NULL)
    {
        NNSET(data->iped_KeyTypes, MUIA_NListtree_Active, (IPTR) kmn->Node);
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

    return TRUE;
}

IPTR IPEditor__MUIM_PrefsEditor_ImportFH
(
    Class *CLASS, Object *self, 
    struct MUIP_PrefsEditor_ImportFH *message
)
{
    SETUP_INST_DATA;

    if (LoadPrefs(message->fh))
    {
        CopyPrefs(&inputprefs, &restore_prefs);
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
    struct PrefHeader header; 
    struct IFFHandle *handle;
    BOOL              success = TRUE;
    LONG              error   = 0;

    if (Gadgets2InputPrefs(data))
    {
        return SavePrefs(message->fh);
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

    update_inputdev();
    try_setting_mousespeed();
    try_setting_test_keymap();
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

    CopyPrefs(&restore_prefs, &inputprefs);

    InputPrefs2Gadgets(data);

    update_inputdev();
    try_setting_mousespeed();
    try_setting_test_keymap();
    SET(self, MUIA_PrefsEditor_Changed, FALSE);
    SET(self, MUIA_PrefsEditor_Testing, FALSE);

    return TRUE;
}

/*** Setup ******************************************************************/
ZUNE_CUSTOMCLASS_5
(
    IPEditor, NULL, MUIC_PrefsEditor, NULL,
    OM_NEW,                    struct opSet *,
    MUIM_PrefsEditor_ImportFH, struct MUIP_PrefsEditor_ImportFH *,
    MUIM_PrefsEditor_ExportFH, struct MUIP_PrefsEditor_ExportFH *,
    MUIM_PrefsEditor_Test,     Msg,
    MUIM_PrefsEditor_Revert,   Msg
);
