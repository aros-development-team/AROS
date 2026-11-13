/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

/* The btmidi.class status window: which device has which CAMD port, and
   BLE MIDI activity. There is nothing to configure: the ports are those of
   the CAMD driver btmidi, and CAMD names them btmidi.in.<n> and
   btmidi.out.<n>. It runs as its own task, opened from Bluetooth
   Preferences. Pairing,
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
    Object *ports_obj;
    Object *service_obj;
    Object *camd_obj;
    Object *rx_obj;
    Object *tx_obj;
    Object *errors_obj;
    Object *reset_obj;
    Object *close_obj;
    Object *about_mi;
    Object *mui_prefs_mi;
};

#define BluetoothBase gui->bt_base
#define MUIMasterBase gui->mui_base
#define IntuitionBase gui->intuition_base

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

/* A line for each port that stands for something. */
static void show_ports(struct btmidi_gui *gui, struct BTMidiBase *base)
{
    static char text[BTMIDI_PORTS * (BTMIDI_NAME_SIZE + 24)];
    char names[BTMIDI_PORTS][BTMIDI_NAME_SIZE];
    BOOL open[BTMIDI_PORTS];
    char *out = text;
    ULONG port;

    Forbid();
    CopyMem(base->port_names, names, sizeof(names));
    for (port = 0; port < BTMIDI_PORTS; port++) {
        open[port] = base->adapters[port].ba_IsOpen;
        if (!base->adapters[port].ba_Owner)
            names[port][0] = 0;
    }
    Permit();
    for (port = 0; port < BTMIDI_PORTS; port++) {
        if (!names[port][0])
            continue;
        btSafeRawDoFmt((STRPTR)out, BTMIDI_NAME_SIZE + 24,
                       (STRPTR)"%s" BTMIDI_DRIVER_NAME ".%lu  %s%s",
                       (IPTR)(out == text ? "" : "\n"), port,
                       (IPTR)names[port], (IPTR)(open[port] ? " (in use)" : ""));
        while (*out)
            out++;
    }
    set(gui->ports_obj, MUIA_Text_Contents, (IPTR)text);
}

/* Fills the activity group from what the service task reports. */
static void show_status(struct btmidi_gui *gui, struct BTMidiBase *base)
{
    struct BTMidiStats stats;
    char buf[160];
    IPTR enabled = FALSE;
    APTR record;
    enum btmidi_driver_state driver_state;
    CONST_STRPTR service, driver;

    Forbid();
    base->activity_pending = FALSE;
    stats = base->stats;
    record = base->record;
    driver_state = base->driver_state;
    Permit();
    show_ports(gui, base);

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

    switch (driver_state) {
    case BTMIDI_DRIVER_INSTALLED:
        driver = (CONST_STRPTR)"Installed as " BTMIDI_DRIVER_PATH "/" BTMIDI_DRIVER_NAME;
        break;
    case BTMIDI_DRIVER_FAILED:
        driver = (CONST_STRPTR)"Could not write " BTMIDI_DRIVER_PATH "/" BTMIDI_DRIVER_NAME;
        break;
    default:
        driver = (CONST_STRPTR)"Not installed yet";
        break;
    }
    set(gui->camd_obj, MUIA_Text_Contents, (IPTR)driver);

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
        MUIA_Application_Description, (IPTR)"Status of the Bluetooth LE MIDI service",
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
                Child, (IPTR)VGroup, GroupFrameT("CAMD ports"),
                    Child, (IPTR)(gui->ports_obj = make_status(gui)),
                    End,
                Child, (IPTR)ColGroup(2), GroupFrameT("Activity"),
                    Child, (IPTR)Label1((IPTR)"Service:"),
                    Child, (IPTR)(gui->service_obj = make_status(gui)),
                    Child, (IPTR)Label1((IPTR)"CAMD driver:"),
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
                    Child, (IPTR)HSpace(0),
                    Child, (IPTR)(gui->close_obj = make_button(gui, (CONST_STRPTR)"\33c Close ")),
                    Child, (IPTR)HSpace(0),
                    End,
                End,
            End),
        End;
    if (!gui->app)
        return FALSE;

    set(gui->ports_obj, MUIA_ShortHelp,
        (IPTR)"Port 0 is this computer, for the phones and computers that\n"
              "connect to it. Each BLE MIDI device it is bound to has a port\n"
              "of its own. CAMD programs find port n as " BTMIDI_DRIVER_NAME ".in.n\n"
              "and " BTMIDI_DRIVER_NAME ".out.n.");
    set(gui->reset_obj, MUIA_ShortHelp, (IPTR)"Set the activity counters back to zero.");
    DoMethod(gui->window, MUIM_Notify, MUIA_Window_CloseRequest, TRUE,
             gui->app, 2, MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);
    DoMethod(gui->close_obj, MUIM_Notify, MUIA_Pressed, FALSE,
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
                show_status(gui, base);   /* MIDI went through, or a port changed */
        }
    }
    set(gui->window, MUIA_Window_Open, FALSE);
    cleanup(base, gui);
    AROS_USERFUNC_EXIT
}
