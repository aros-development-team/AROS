/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

/* The btmidi.class settings window: CAMD port names and BLE MIDI activity.
   It runs as its own task, opened from Bluetooth Preferences. Pairing,
   advertising and whether the service is offered stay in Bluetooth
   Preferences and the GATT server settings. */

#include "btmidi.h"

#include <exec/memory.h>
#include <intuition/intuition.h>
#include <libraries/mui.h>

#include <proto/alib.h>
#include <proto/bluetooth.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>

#include <string.h>

struct btmidi_gui {
    struct Library *bt_base;
    struct Library *mui_base;
    struct Library *intuition_base;
    Object *app;
    Object *window;
    Object *node_obj;
    Object *in_obj;
    Object *out_obj;
    Object *service_obj;
    Object *camd_obj;
    Object *rx_obj;
    Object *tx_obj;
    Object *errors_obj;
    Object *reset_obj;
    Object *defaults_obj;
    Object *save_obj;
    Object *use_obj;
    Object *cancel_obj;
    Object *about_mi;
    Object *mui_prefs_mi;
};

#define BluetoothBase gui->bt_base
#define MUIMasterBase gui->mui_base
#define IntuitionBase gui->intuition_base

#define ID_SAVE     1
#define ID_USE      2
#define ID_DEFAULTS 3
#define ID_RESET    4
#define ID_ABOUT    5

static Object *make_button(struct btmidi_gui *gui, CONST_STRPTR label)
{
    return TextObject, ButtonFrame,
        MUIA_Background, MUII_ButtonBack,
        MUIA_CycleChain, 1,
        MUIA_InputMode, MUIV_InputMode_RelVerify,
        MUIA_Text_Contents, (IPTR)label,
        End;
}

static Object *make_name(struct btmidi_gui *gui, CONST_STRPTR help)
{
    return StringObject, StringFrame,
        MUIA_CycleChain, 1,
        MUIA_String_MaxLen, BTMIDI_NAME_SIZE,
        MUIA_ShortHelp, (IPTR)help,
        End;
}

static void show_names(struct btmidi_gui *gui, const struct BTMidiCfg *cfg)
{
    set(gui->node_obj, MUIA_String_Contents, (IPTR)cfg->mc_NodeName);
    set(gui->in_obj, MUIA_String_Contents, (IPTR)cfg->mc_InName);
    set(gui->out_obj, MUIA_String_Contents, (IPTR)cfg->mc_OutName);
}

static void read_name(struct btmidi_gui *gui, Object *obj, char *name,
                      const char *fallback)
{
    STRPTR text = NULL;
    ULONG start = 0, end;

    get(obj, MUIA_String_Contents, &text);
    if (text) {
        end = 0;
        while (end < BTMIDI_NAME_SIZE - 1 && text[end])
            end++;
        while (start < end && text[start] == ' ')
            start++;
        while (end > start && text[end - 1] == ' ')
            end--;
        CopyMem(text + start, name, end - start);
        name[end - start] = 0;
    } else {
        name[0] = 0;
    }
    if (!name[0])
        strcpy(name, fallback);
}

/* Fills the activity group from what the service task reports. */
static void show_status(struct btmidi_gui *gui, struct BTMidiBase *base)
{
    struct BTMidiStats stats;
    char node[BTMIDI_NAME_SIZE];
    char buf[160];
    IPTR enabled = FALSE;
    APTR record;
    enum btmidi_camd_state camd_state;
    CONST_STRPTR service;

    Forbid();
    base->activity_pending = FALSE;
    stats = base->stats;
    record = base->record;
    camd_state = base->camd_state;
    CopyMem(base->cfg.mc_NodeName, node, sizeof(node));
    Permit();

    if (record) {
        btLockReadBase();
        btGetAttrs(BGA_SERVICERECORD, record, BSRA_Enabled, &enabled, TAG_END);
        btUnlockBase();
        service = enabled ? (CONST_STRPTR)"Offered to connecting devices"
                          : (CONST_STRPTR)"Registered, but switched off in the GATT server settings";
    } else {
        service = (CONST_STRPTR)"Not registered";
    }
    set(gui->service_obj, MUIA_Text_Contents, (IPTR)service);

    switch (camd_state) {
    case BTMIDI_CAMD_OPEN:
        btSafeRawDoFmt((STRPTR)buf, sizeof(buf), (STRPTR)"Open as \"%s\"", (IPTR)node);
        break;
    case BTMIDI_CAMD_DEFAULTS:
        btSafeRawDoFmt((STRPTR)buf, sizeof(buf),
                       (STRPTR)"The names failed; open as \"%s\"",
                       (IPTR)BTMIDI_DEFAULT_NODE);
        break;
    default:
        strcpy(buf, "Not open (is camd.library installed?)");
        break;
    }
    set(gui->camd_obj, MUIA_Text_Contents, (IPTR)buf);

    btSafeRawDoFmt((STRPTR)buf, sizeof(buf), (STRPTR)"%lu packets, %lu MIDI messages",
                   stats.ms_RxPackets, stats.ms_RxMessages);
    set(gui->rx_obj, MUIA_Text_Contents, (IPTR)buf);
    btSafeRawDoFmt((STRPTR)buf, sizeof(buf), (STRPTR)"%lu packets, %lu MIDI messages",
                   stats.ms_TxPackets, stats.ms_TxMessages);
    set(gui->tx_obj, MUIA_Text_Contents, (IPTR)buf);
    btSafeRawDoFmt((STRPTR)buf, sizeof(buf), (STRPTR)"%lu", stats.ms_Errors);
    set(gui->errors_obj, MUIA_Text_Contents, (IPTR)buf);
}

static Object *make_status(struct btmidi_gui *gui)
{
    return TextObject, TextFrame,
        MUIA_Background, MUII_TextBack,
        End;
}

static BOOL build_window(struct btmidi_gui *gui)
{
    gui->app = ApplicationObject,
        MUIA_Application_Title, (IPTR)MOD_NAME_STRING,
        MUIA_Application_Version, (IPTR)VERSION_STRING,
        MUIA_Application_Copyright, (IPTR)"(c) 2026 The AROS Development Team",
        MUIA_Application_Author, (IPTR)"The AROS Development Team",
        MUIA_Application_Description, (IPTR)"Settings for the Bluetooth LE MIDI service",
        MUIA_Application_Base, (IPTR)"BTMIDI",
        MUIA_Application_Menustrip, (IPTR)MenustripObject,
            Child, (IPTR)MenuObjectT((IPTR)"Project"),
                Child, (IPTR)(gui->about_mi = MenuitemObject,
                    MUIA_Menuitem_Title, (IPTR)"About...",
                    MUIA_Menuitem_Shortcut, (IPTR)"?",
                    End),
                End,
            Child, (IPTR)MenuObjectT((IPTR)"Settings"),
                Child, (IPTR)(gui->mui_prefs_mi = MenuitemObject,
                    MUIA_Menuitem_Title, (IPTR)"MUI Settings",
                    MUIA_Menuitem_Shortcut, (IPTR)"M",
                    End),
                End,
            End,

        SubWindow, (IPTR)(gui->window = WindowObject,
            MUIA_Window_ID, MAKE_ID('M','A','I','N'),
            MUIA_Window_Title, (IPTR)"Bluetooth LE MIDI",
            MUIA_HelpNode, (IPTR)MOD_NAME_STRING,

            WindowContents, (IPTR)VGroup,
                Child, (IPTR)ColGroup(2), GroupFrameT("CAMD ports"),
                    Child, (IPTR)Label2((IPTR)"Device:"),
                    Child, (IPTR)(gui->node_obj = make_name(gui,
                        (CONST_STRPTR)"The name of the CAMD device BLE MIDI appears as.")),
                    Child, (IPTR)Label2((IPTR)"From Bluetooth:"),
                    Child, (IPTR)(gui->in_obj = make_name(gui,
                        (CONST_STRPTR)"The cluster that carries MIDI received from\n"
                                      "the connected phone or computer.")),
                    Child, (IPTR)Label2((IPTR)"To Bluetooth:"),
                    Child, (IPTR)(gui->out_obj = make_name(gui,
                        (CONST_STRPTR)"The cluster whose MIDI is sent to\n"
                                      "the connected phone or computer.")),
                    End,
                Child, (IPTR)ColGroup(2), GroupFrameT("Activity"),
                    Child, (IPTR)Label1((IPTR)"Service:"),
                    Child, (IPTR)(gui->service_obj = make_status(gui)),
                    Child, (IPTR)Label1((IPTR)"CAMD:"),
                    Child, (IPTR)(gui->camd_obj = make_status(gui)),
                    Child, (IPTR)Label1((IPTR)"Received:"),
                    Child, (IPTR)(gui->rx_obj = make_status(gui)),
                    Child, (IPTR)Label1((IPTR)"Sent:"),
                    Child, (IPTR)(gui->tx_obj = make_status(gui)),
                    Child, (IPTR)Label1((IPTR)"Errors:"),
                    Child, (IPTR)HGroup,
                        Child, (IPTR)(gui->errors_obj = make_status(gui)),
                        Child, (IPTR)(gui->reset_obj = make_button(gui,
                            (CONST_STRPTR)"\33c Reset ")),
                        End,
                    End,
                Child, (IPTR)HGroup,
                    MUIA_Group_SameWidth, TRUE,
                    Child, (IPTR)(gui->save_obj = make_button(gui, (CONST_STRPTR)"\33c Save ")),
                    Child, (IPTR)(gui->use_obj = make_button(gui, (CONST_STRPTR)"\33c Use ")),
                    Child, (IPTR)(gui->defaults_obj = make_button(gui, (CONST_STRPTR)"\33c Defaults ")),
                    Child, (IPTR)(gui->cancel_obj = make_button(gui, (CONST_STRPTR)"\33c Cancel ")),
                    End,
                End,
            End),
        End;
    if (!gui->app)
        return FALSE;

    set(gui->reset_obj, MUIA_ShortHelp, (IPTR)"Set the activity counters back to zero.");
    DoMethod(gui->window, MUIM_Notify, MUIA_Window_CloseRequest, TRUE,
             gui->app, 2, MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);
    DoMethod(gui->save_obj, MUIM_Notify, MUIA_Pressed, FALSE,
             gui->app, 2, MUIM_Application_ReturnID, ID_SAVE);
    DoMethod(gui->use_obj, MUIM_Notify, MUIA_Pressed, FALSE,
             gui->app, 2, MUIM_Application_ReturnID, ID_USE);
    DoMethod(gui->defaults_obj, MUIM_Notify, MUIA_Pressed, FALSE,
             gui->app, 2, MUIM_Application_ReturnID, ID_DEFAULTS);
    DoMethod(gui->cancel_obj, MUIM_Notify, MUIA_Pressed, FALSE,
             gui->app, 2, MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);
    DoMethod(gui->reset_obj, MUIM_Notify, MUIA_Pressed, FALSE,
             gui->app, 2, MUIM_Application_ReturnID, ID_RESET);
    DoMethod(gui->about_mi, MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime,
             gui->app, 2, MUIM_Application_ReturnID, ID_ABOUT);
    DoMethod(gui->mui_prefs_mi, MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime,
             gui->app, 2, MUIM_Application_OpenConfigWindow, 0);
    return TRUE;
}

static void cleanup(struct BTMidiBase *base, struct btmidi_gui *gui)
{
    if (gui) {
        if (gui->app)
            MUI_DisposeObject(gui->app);
        if (MUIMasterBase)
            CloseLibrary(MUIMasterBase);
        if (IntuitionBase)
            CloseLibrary(IntuitionBase);
        if (BluetoothBase)
            CloseLibrary(BluetoothBase);
        FreeVec(gui);
    }
    Forbid();
    base->gui = NULL;
    base->gui_task = NULL;
    base->activity_pending = FALSE;
    --base->library.lib_OpenCnt;
}

AROS_UFH0(void, btmidi_gui_task)
{
    AROS_USERFUNC_INIT
    struct BTMidiBase *base = FindTask(NULL)->tc_UserData;
    struct btmidi_gui *gui;
    struct BTMidiCfg edit;
    IPTR is_open = FALSE, iconified = FALSE;
    ULONG sigs = 0;
    IPTR id;

    ++base->library.lib_OpenCnt;
    if (!(gui = AllocVec(sizeof(*gui), MEMF_PUBLIC | MEMF_CLEAR)) ||
        !(MUIMasterBase = OpenLibrary((CONST_STRPTR)MUIMASTER_NAME, MUIMASTER_VMIN)) ||
        !(IntuitionBase = OpenLibrary((CONST_STRPTR)"intuition.library", 39)) ||
        !(BluetoothBase = OpenLibrary((CONST_STRPTR)"bluetooth.library", 45)) ||
        !build_window(gui)) {
        cleanup(base, gui);
        return;
    }
    base->gui = gui;

    /* the window edits a copy; Use and Save put it into effect */
    Forbid();
    edit = base->cfg;
    Permit();
    show_names(gui, &edit);
    show_status(gui, base);

    get(gui->app, MUIA_Application_Iconified, &iconified);
    set(gui->window, MUIA_Window_Open, TRUE);
    get(gui->window, MUIA_Window_Open, &is_open);
    if (!(is_open || iconified)) {
        cleanup(base, gui);
        return;
    }
    for (;;) {
        id = DoMethod(gui->app, MUIM_Application_NewInput, &sigs);
        if (id == MUIV_Application_ReturnID_Quit)
            break;
        switch (id) {
        case ID_SAVE:
        case ID_USE:
            read_name(gui, gui->node_obj, edit.mc_NodeName, BTMIDI_DEFAULT_NODE);
            read_name(gui, gui->in_obj, edit.mc_InName, BTMIDI_DEFAULT_IN);
            read_name(gui, gui->out_obj, edit.mc_OutName, BTMIDI_DEFAULT_OUT);
            Forbid();
            base->cfg = edit;
            Permit();
            btmidi_store_cfg(base, BluetoothBase, id == ID_SAVE);
            btmidi_reconfigure(base);
            id = MUIV_Application_ReturnID_Quit;
            break;
        case ID_DEFAULTS:
            btmidi_default_cfg(&edit);
            show_names(gui, &edit);
            break;
        case ID_RESET:
            Forbid();
            memset(&base->stats, 0, sizeof(base->stats));
            Permit();
            show_status(gui, base);
            break;
        case ID_ABOUT:
            MUI_RequestA(gui->app, gui->window, 0, NULL, (STRPTR)"OK",
                         (STRPTR)VERSION_STRING, NULL);
            break;
        }
        if (id == MUIV_Application_ReturnID_Quit)
            break;
        if (sigs) {
            sigs = Wait(sigs | SIGBREAKF_CTRL_C | SIGBREAKF_CTRL_F);
            if (sigs & SIGBREAKF_CTRL_C)
                break;
            if (sigs & SIGBREAKF_CTRL_F)
                show_status(gui, base);   /* MIDI went through */
        }
    }
    set(gui->window, MUIA_Window_Open, FALSE);
    cleanup(base, gui);
    AROS_USERFUNC_EXIT
}
