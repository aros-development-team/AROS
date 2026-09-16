/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Pick a wireless network and connect to it.

    The window shows what is on the air and hands a chosen network to the
    supplicant by writing Wireless.prefs and restarting it. It deliberately
    does not associate by itself: WirelessManager is the one that runs the WPA
    handshake, so it has to be the one that joins.
*/

#define MUIMASTER_YES_INLINE_STDARG

#include <exec/types.h>
#include <libraries/mui.h>
#include <utility/hooks.h>
#include <dos/dos.h>
#include <dos/dostags.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/muimaster.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <clib/alib_protos.h>

#include <stdio.h>
#include <string.h>

#include "wifigui.h"
#include "locale.h"

#define VERSION "$VER: WiFi 1.0 (20.08.2026) AROS Dev Team"

static Object *app, *window, *list, *scanbtn, *connectbtn, *statustext;
static Object *detailsbtn, *disconnectbtn, *devcycle;

/* Entries for the device chooser. MUI keeps the array, so it has to outlive
 * the object; the strings themselves live in the shared message. */
static CONST_STRPTR devnames[WIFI_MAX_DEVS + 1];
static TEXT devlabels[WIFI_MAX_DEVS][WIFI_DEV_MAX + 16];
static Object *keywindow, *keystring, *keytext, *keyok, *keycancel;
static Object *detailswindow, *detailsok;
static Object *dv_network, *dv_security, *dv_ap, *dv_iface, *dv_mac;
static Object *dv_address, *dv_netmask, *dv_broadcast, *dv_gateway, *dv_dns;

/* Set while a status request is made only to fill the details window. */
static BOOL details_pending;

static struct MsgPort *replyport;
static struct WifiMsg *msg;
static struct Process *worker;
static BOOL busy;                       /* a command is in flight */

/* The network the pending CONNECT is for, so a passphrase can be asked for
 * and the same request sent again. */
static TEXT pending_ssid[WIFI_SSID_MAX];
static LONG pending_protected;

/* ------------------------------------------------------------------------- */

static void SetStatus(CONST_STRPTR text)
{
    SET(statustext, MUIA_Text_Contents, (IPTR)text);
}

static BOOL wireless = TRUE;            /* the chosen device can do wireless */

/* Details works on any device - the address is worth seeing either way - so
 * only the wireless actions follow wm_Wireless. */
static void UpdateButtons(void)
{
    SET(scanbtn, MUIA_Disabled, busy || !wireless);
    SET(connectbtn, MUIA_Disabled, busy || !wireless);
    SET(disconnectbtn, MUIA_Disabled, busy || !wireless);
    SET(detailsbtn, MUIA_Disabled, busy);
}

static void SetBusy(BOOL on)
{
    busy = on;
    UpdateButtons();
}

/* Hand the shared message to the worker. Only one is ever in flight. */
static void Send(ULONG cmd)
{
    if (busy || WifiWorkerPort == NULL)
        return;

    msg->wm_Cmd = cmd;
    msg->wm_Msg.mn_Node.ln_Type = NT_MESSAGE;
    msg->wm_Msg.mn_Length = sizeof(*msg);
    msg->wm_Msg.mn_ReplyPort = replyport;
    SetBusy(TRUE);
    PutMsg(WifiWorkerPort, &msg->wm_Msg);
}

/* ------------------------------------------------------------------------- */
/* The list                                                                  */

static IPTR DisplayFunc(struct Hook *hook, STRPTR *columns, struct WifiNet *net)
{
    static TEXT signal[8], channel[8];

    if (net == NULL)
    {
        columns[0] = "";
        columns[1] = (STRPTR)_(MSG_COL_NETWORK);
        columns[2] = (STRPTR)_(MSG_COL_SECURITY);
        columns[3] = (STRPTR)_(MSG_COL_SIGNAL);
        columns[4] = (STRPTR)_(MSG_COL_CHANNEL);
        return 0;
    }

    snprintf(signal, sizeof(signal), "%ld", (long)net->wn_Signal);
    snprintf(channel, sizeof(channel), "%ld", (long)net->wn_Channel);

    /* A tick for the network we are on, a dot for one we have a key for. */
    columns[0] = (msg->wm_Associated &&
                  strcmp(msg->wm_SSID, net->wn_SSID) == 0) ? "\xBB" :
                 (net->wn_Known ? "\xB7" : " ");
    columns[1] = net->wn_SSID;
    columns[2] = net->wn_Protection;
    columns[3] = signal;
    columns[4] = channel;

    return 0;
}

static void FillList(void)
{
    LONG i;

    SET(list, MUIA_List_Quiet, TRUE);
    DoMethod(list, MUIM_List_Clear);
    for (i = 0; i < msg->wm_Count; i++)
    {
        DoMethod(list, MUIM_List_InsertSingle, (IPTR)&msg->wm_Nets[i],
            MUIV_List_Insert_Bottom);
    }
    SET(list, MUIA_List_Quiet, FALSE);
}

/* ------------------------------------------------------------------------- */
/* Actions                                                                   */

static IPTR DeviceFunc(struct Hook *hook, Object *caller, void *data)
{
    LONG active = XGET(devcycle, MUIA_Cycle_Active);

    if (active < 0 || active >= msg->wm_DeviceCount)
        return 0;

    /* The list belongs to the device it was scanned with. */
    DoMethod(list, MUIM_List_Clear);
    msg->wm_Count = 0;
    snprintf(msg->wm_Device, sizeof(msg->wm_Device), "%s",
        msg->wm_Devices[active]);
    msg->wm_Unit = msg->wm_DeviceUnits[active];
    Send(WCMD_STATUS);
    return 0;
}

static IPTR ScanFunc(struct Hook *hook, Object *caller, void *data)
{
    SetStatus(_(MSG_SCANNING));
    Send(WCMD_SCAN);
    return 0;
}

/* Ask the worker to connect. It answers WRES_NEEDKEY if it has no passphrase
 * stored for a protected network, and then the requester opens. */
static void StartConnect(CONST_STRPTR key)
{
    snprintf(msg->wm_SSID, sizeof(msg->wm_SSID), "%s", pending_ssid);
    snprintf(msg->wm_Key, sizeof(msg->wm_Key), "%s",
        (const char *)(key != NULL ? key : (CONST_STRPTR)""));
    msg->wm_Protected = pending_protected;

    {
        TEXT text[128];

        snprintf(text, sizeof(text), (const char *)_(MSG_CONNECTING),
            pending_ssid);
        SetStatus(text);
    }
    Send(WCMD_CONNECT);
}

static IPTR ConnectFunc(struct Hook *hook, Object *caller, void *data)
{
    struct WifiNet *net = NULL;
    LONG active = 0;

    GET(list, MUIA_List_Active, &active);
    if (active == MUIV_List_Active_Off)
    {
        SetStatus(_(MSG_SELECT_FIRST));
        return 0;
    }

    DoMethod(list, MUIM_List_GetEntry, active, &net);
    if (net == NULL)
        return 0;

    snprintf(pending_ssid, sizeof(pending_ssid), "%s", net->wn_SSID);
    pending_protected = (strcmp(net->wn_Protection, "open") != 0);
    StartConnect(NULL);
    return 0;
}

static IPTR KeyOkFunc(struct Hook *hook, Object *caller, void *data)
{
    STRPTR key = (STRPTR)XGET(keystring, MUIA_String_Contents);

    SET(keywindow, MUIA_Window_Open, FALSE);
    StartConnect(key);
    SET(keystring, MUIA_String_Contents, (IPTR)"");
    return 0;
}

static IPTR KeyCancelFunc(struct Hook *hook, Object *caller, void *data)
{
    SET(keywindow, MUIA_Window_Open, FALSE);
    SET(keystring, MUIA_String_Contents, (IPTR)"");
    SetStatus(_(MSG_CANCELLED));
    return 0;
}

static void AskForKey(void)
{
    TEXT prompt[128];

    snprintf(prompt, sizeof(prompt), (const char *)_(MSG_PASSWORD_FOR),
        pending_ssid);
    SET(keytext, MUIA_Text_Contents, (IPTR)prompt);
    SET(keystring, MUIA_String_Contents, (IPTR)"");
    SET(keywindow, MUIA_Window_Open, TRUE);
    SET(keywindow, MUIA_Window_ActiveObject, (IPTR)keystring);
}

/* An empty field says more as a dash than as a blank. */
static void SetField(Object *obj, CONST_STRPTR value)
{
    SET(obj, MUIA_Text_Contents,
        (IPTR)((value != NULL && value[0] != '\0') ? value
                                                   : (CONST_STRPTR)"-"));
}

static void ShowDetails(void)
{
    CONST_STRPTR security = (CONST_STRPTR)"";
    LONG i;

    /* Protection is a property of the beacon, so it comes from the scan. */
    for (i = 0; i < msg->wm_Count; i++)
    {
        if (strcmp((const char *)msg->wm_Nets[i].wn_SSID,
                   (const char *)msg->wm_SSID) == 0)
        {
            security = (CONST_STRPTR)msg->wm_Nets[i].wn_Protection;
            break;
        }
    }

    SetField(dv_network, msg->wm_Associated ? (CONST_STRPTR)msg->wm_SSID
                                            : (CONST_STRPTR)"");
    SetField(dv_security, msg->wm_Associated ? security : (CONST_STRPTR)"");
    SetField(dv_ap, (CONST_STRPTR)msg->wm_BSSID);
    SetField(dv_iface, (CONST_STRPTR)msg->wm_Interface);
    SetField(dv_mac, (CONST_STRPTR)msg->wm_MAC);
    SetField(dv_address, (CONST_STRPTR)msg->wm_Address);
    SetField(dv_netmask, (CONST_STRPTR)msg->wm_Netmask);
    SetField(dv_broadcast, (CONST_STRPTR)msg->wm_Broadcast);
    SetField(dv_gateway, (CONST_STRPTR)msg->wm_Gateway);
    SetField(dv_dns, (CONST_STRPTR)msg->wm_DNS);

    SET(detailswindow, MUIA_Window_Open, TRUE);
}

static IPTR DetailsFunc(struct Hook *hook, Object *caller, void *data)
{
    /* Ask again rather than show whatever the last command left behind. */
    details_pending = TRUE;
    Send(WCMD_STATUS);
    return 0;
}

static IPTR DetailsOkFunc(struct Hook *hook, Object *caller, void *data)
{
    SET(detailswindow, MUIA_Window_Open, FALSE);
    return 0;
}

static IPTR DisconnectFunc(struct Hook *hook, Object *caller, void *data)
{
    SetStatus(_(MSG_DISCONNECTING));
    Send(WCMD_DISCONNECT);
    return 0;
}

/* A reply came back from the worker. */
static void HandleReply(void)
{
    struct WifiMsg *m;

    while ((m = (struct WifiMsg *)GetMsg(replyport)) != NULL)
    {
        SetBusy(FALSE);

        switch (m->wm_Cmd)
        {
        case WCMD_SCAN:
            FillList();
            SetStatus(m->wm_Status);
            break;

        case WCMD_CONNECT:
            if (m->wm_Result == WRES_NEEDKEY)
            {
                /* The worker says why it is asking - a first attempt and a
                 * rejected passphrase look the same in the requester. */
                SetStatus(m->wm_Status);
                AskForKey();
                break;
            }
            SetStatus(m->wm_Status);
            DoMethod(list, MUIM_List_Redraw, MUIV_List_Redraw_All);
            break;

        case WCMD_STATUS:
            wireless = m->wm_Wireless;
            UpdateButtons();
            SetStatus(m->wm_Status);
            DoMethod(list, MUIM_List_Redraw, MUIV_List_Redraw_All);
            if (details_pending)
            {
                details_pending = FALSE;
                ShowDetails();
            }
            break;

        case WCMD_DISCONNECT:
            SetStatus(m->wm_Status);
            DoMethod(list, MUIM_List_Redraw, MUIV_List_Redraw_All);
            break;
        }
    }
}

/* ------------------------------------------------------------------------- */

int main(void)
{
    struct Hook scan_hook, connect_hook, display_hook, keyok_hook, keycancel_hook;
    struct Hook details_hook, detailsok_hook, disconnect_hook, device_hook;
    TEXT configured[WIFI_DEV_MAX];
    LONG i, preselect = 0;
    ULONG sigs = 0, replysig;
    int rc = RETURN_FAIL;

    Locale_Initialize();

    replyport = CreateMsgPort();
    msg = AllocVec(sizeof(struct WifiMsg), MEMF_PUBLIC | MEMF_CLEAR);
    if (replyport == NULL || msg == NULL)
    {
        PutStr(_(MSG_ERR_NOMEM));
        goto cleanup;
    }
    replysig = 1UL << replyport->mp_SigBit;

    /* The worker owns every call that can block. */
    WifiMainTask = FindTask(NULL);
    SetSignal(0, SIGF_SINGLE);
    worker = CreateNewProcTags(NP_Entry, (IPTR)WifiWorker,
        NP_Name, (IPTR)"WiFi worker", TAG_END);
    if (worker == NULL)
    {
        PutStr(_(MSG_ERR_WORKER_START));
        goto cleanup;
    }
    Wait(SIGF_SINGLE);
    if (WifiWorkerPort == NULL)
    {
        PutStr(_(MSG_ERR_WORKER_FAILED));
        goto cleanup;
    }

    /*
     * The device list has to be there before the chooser can be built, so this
     * one request is made synchronously - it is startup, and the window is not
     * on screen yet to be blocked. The worker opens each driver to find out
     * whether it is a radio, so only wireless devices come back.
     */
    Send(WCMD_DEVICES);
    WaitPort(replyport);
    GetMsg(replyport);
    SetBusy(FALSE);

    /* The unit only earns a mention when the driver offers more than one. */
    for (i = 0; i < msg->wm_DeviceCount; i++)
    {
        if (msg->wm_DeviceUnits[i] != 0)
            snprintf(devlabels[i], sizeof(devlabels[i]), "%s unit %ld",
                msg->wm_Devices[i], (long)msg->wm_DeviceUnits[i]);
        else
            snprintf(devlabels[i], sizeof(devlabels[i]), "%s",
                msg->wm_Devices[i]);
        devnames[i] = (CONST_STRPTR)devlabels[i];
    }
    if (msg->wm_DeviceCount == 0)
        devnames[0] = _(MSG_NO_DEVICE);
    devnames[msg->wm_DeviceCount ? msg->wm_DeviceCount : 1] = NULL;

    /* Start on the device the preferences chose, if it is one of ours. */
    if (GetVar("AROSTCP/WirelessDevice", configured, sizeof(configured),
        LV_VAR) > 0)
    {
        STRPTR space = (STRPTR)strchr((const char *)configured, ' ');
        LONG unit = 0;

        /* "<device> UNIT <n>", the form the network preferences write. */
        if (space != NULL)
        {
            STRPTR p = (STRPTR)strstr((const char *)(space + 1), "UNIT");

            *space = '\0';
            if (p != NULL)
            {
                for (p += 4; *p == ' '; p++)
                    ;
                StrToLong(p, &unit);
            }
        }
        for (i = 0; i < msg->wm_DeviceCount; i++)
        {
            if (Stricmp(msg->wm_Devices[i], configured) == 0 &&
                msg->wm_DeviceUnits[i] == unit)
            {
                preselect = i;
                break;
            }
        }
    }
    if (msg->wm_DeviceCount > 0)
    {
        snprintf(msg->wm_Device, sizeof(msg->wm_Device), "%s",
            msg->wm_Devices[preselect]);
        msg->wm_Unit = msg->wm_DeviceUnits[preselect];
    }

    scan_hook.h_Entry = HookEntry;
    scan_hook.h_SubEntry = (HOOKFUNC)ScanFunc;
    connect_hook.h_Entry = HookEntry;
    connect_hook.h_SubEntry = (HOOKFUNC)ConnectFunc;
    display_hook.h_Entry = HookEntry;
    display_hook.h_SubEntry = (HOOKFUNC)DisplayFunc;
    keyok_hook.h_Entry = HookEntry;
    keyok_hook.h_SubEntry = (HOOKFUNC)KeyOkFunc;
    keycancel_hook.h_Entry = HookEntry;
    keycancel_hook.h_SubEntry = (HOOKFUNC)KeyCancelFunc;
    details_hook.h_Entry = HookEntry;
    details_hook.h_SubEntry = (HOOKFUNC)DetailsFunc;
    detailsok_hook.h_Entry = HookEntry;
    detailsok_hook.h_SubEntry = (HOOKFUNC)DetailsOkFunc;
    disconnect_hook.h_Entry = HookEntry;
    disconnect_hook.h_SubEntry = (HOOKFUNC)DisconnectFunc;
    device_hook.h_Entry = HookEntry;
    device_hook.h_SubEntry = (HOOKFUNC)DeviceFunc;

    app = ApplicationObject,
        MUIA_Application_Title,       (IPTR)"WiFi",
        MUIA_Application_Version,     (IPTR)VERSION,
        MUIA_Application_Description, __(MSG_DESCRIPTION),
        MUIA_Application_Base,        (IPTR)"WIFI",
        MUIA_Application_SingleTask,  TRUE,

        SubWindow, (IPTR)(window = WindowObject,
            MUIA_Window_Title,    __(MSG_WIN_MAIN),
            MUIA_Window_ID,       MAKE_ID('W','I','F','I'),
            MUIA_Window_SizeGadget, TRUE,
            MUIA_Window_Width,    MUIV_Window_Width_Visible(35),
            WindowContents, (IPTR)(VGroup,
                Child, (IPTR)(HGroup,
                    Child, (IPTR)Label((char *)_(MSG_LAB_DEVICE)),
                    Child, (IPTR)(devcycle = CycleObject,
                        MUIA_Cycle_Entries, (IPTR)devnames,
                        MUIA_Cycle_Active, preselect,
                        MUIA_CycleChain, 1,
                    End),
                End),
                Child, (IPTR)(ListviewObject,
                    MUIA_CycleChain, 1,
                    MUIA_Listview_List, (IPTR)(list = ListObject,
                        InputListFrame,
                        MUIA_List_DisplayHook, (IPTR)&display_hook,
                        MUIA_List_Format, (IPTR)"BAR,WEIGHT=300 BAR,BAR,P=\33r BAR,P=\33r",
                        MUIA_List_Title, TRUE,
                    End),
                End),
                Child, (IPTR)(statustext = TextObject,
                    TextFrame,
                    MUIA_Background, MUII_TextBack,
                    MUIA_Text_Contents, __(MSG_STARTING),
                End),
                Child, (IPTR)(HGroup,
                    Child, (IPTR)(scanbtn = SimpleButton((char *)_(MSG_BTN_SCAN))),
                    Child, (IPTR)(detailsbtn = SimpleButton((char *)_(MSG_BTN_DETAILS))),
                    Child, (IPTR)HVSpace,
                    Child, (IPTR)(disconnectbtn = SimpleButton((char *)_(MSG_BTN_DISCONNECT))),
                    Child, (IPTR)(connectbtn = SimpleButton((char *)_(MSG_BTN_CONNECT))),
                End),
            End),
        End),

        SubWindow, (IPTR)(detailswindow = WindowObject,
            MUIA_Window_Title,    __(MSG_WIN_DETAILS),
            MUIA_Window_ID,       MAKE_ID('W','I','F','D'),
            WindowContents, (IPTR)(VGroup,
                Child, (IPTR)(ColGroup(2),
                    GroupFrameT((char *)_(MSG_GRP_WIRELESS)),
                    Child, (IPTR)Label((char *)_(MSG_LAB_NETWORK)),
                    Child, (IPTR)(dv_network = TextObject, End),
                    Child, (IPTR)Label((char *)_(MSG_LAB_SECURITY)),
                    Child, (IPTR)(dv_security = TextObject, End),
                    Child, (IPTR)Label((char *)_(MSG_LAB_AP)),
                    Child, (IPTR)(dv_ap = TextObject, End),
                    Child, (IPTR)Label((char *)_(MSG_LAB_MAC)),
                    Child, (IPTR)(dv_mac = TextObject, End),
                End),
                Child, (IPTR)(ColGroup(2),
                    GroupFrameT((char *)_(MSG_GRP_NETWORK)),
                    Child, (IPTR)Label((char *)_(MSG_LAB_INTERFACE)),
                    Child, (IPTR)(dv_iface = TextObject, End),
                    Child, (IPTR)Label((char *)_(MSG_LAB_ADDRESS)),
                    Child, (IPTR)(dv_address = TextObject, End),
                    Child, (IPTR)Label((char *)_(MSG_LAB_NETMASK)),
                    Child, (IPTR)(dv_netmask = TextObject, End),
                    Child, (IPTR)Label((char *)_(MSG_LAB_BROADCAST)),
                    Child, (IPTR)(dv_broadcast = TextObject, End),
                    Child, (IPTR)Label((char *)_(MSG_LAB_ROUTER)),
                    Child, (IPTR)(dv_gateway = TextObject, End),
                    Child, (IPTR)Label((char *)_(MSG_LAB_DNS)),
                    Child, (IPTR)(dv_dns = TextObject, End),
                End),
                Child, (IPTR)(HGroup,
                    Child, (IPTR)HVSpace,
                    Child, (IPTR)(detailsok = SimpleButton((char *)_(MSG_BTN_OK))),
                End),
            End),
        End),

        /* The password is asked for when it is needed, not kept on display. */
        SubWindow, (IPTR)(keywindow = WindowObject,
            MUIA_Window_Title,    __(MSG_WIN_PASSWORD),
            MUIA_Window_ID,       MAKE_ID('W','I','F','K'),
            MUIA_Window_Activate, TRUE,
            WindowContents, (IPTR)(VGroup,
                Child, (IPTR)(keytext = TextObject,
                    MUIA_Text_Contents, __(MSG_LAB_PASSWORD),
                End),
                Child, (IPTR)(keystring = StringObject,
                    StringFrame,
                    MUIA_String_Secret, TRUE,
                    MUIA_String_MaxLen, WIFI_KEY_MAX,
                    MUIA_CycleChain, 1,
                End),
                Child, (IPTR)(HGroup,
                    Child, (IPTR)(keycancel = SimpleButton((char *)_(MSG_BTN_CANCEL))),
                    Child, (IPTR)HVSpace,
                    Child, (IPTR)(keyok = SimpleButton((char *)_(MSG_BTN_OK))),
                End),
            End),
        End),
    End;

    if (app == NULL)
    {
        PutStr(_(MSG_ERR_APPLICATION));
        goto cleanup;
    }

    DoMethod(window, MUIM_Notify, MUIA_Window_CloseRequest, TRUE,
        (IPTR)app, 2, MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);
    DoMethod(devcycle, MUIM_Notify, MUIA_Cycle_Active, MUIV_EveryTime,
        (IPTR)devcycle, 3, MUIM_CallHook, (IPTR)&device_hook, NULL);
    DoMethod(scanbtn, MUIM_Notify, MUIA_Pressed, FALSE,
        (IPTR)scanbtn, 3, MUIM_CallHook, (IPTR)&scan_hook, NULL);
    DoMethod(connectbtn, MUIM_Notify, MUIA_Pressed, FALSE,
        (IPTR)connectbtn, 3, MUIM_CallHook, (IPTR)&connect_hook, NULL);
    /* Double-clicking a network is the obvious way to join it. */
    DoMethod(list, MUIM_Notify, MUIA_Listview_DoubleClick, TRUE,
        (IPTR)list, 3, MUIM_CallHook, (IPTR)&connect_hook, NULL);

    DoMethod(keyok, MUIM_Notify, MUIA_Pressed, FALSE,
        (IPTR)keyok, 3, MUIM_CallHook, (IPTR)&keyok_hook, NULL);
    DoMethod(keystring, MUIM_Notify, MUIA_String_Acknowledge, MUIV_EveryTime,
        (IPTR)keyok, 3, MUIM_CallHook, (IPTR)&keyok_hook, NULL);
    DoMethod(keycancel, MUIM_Notify, MUIA_Pressed, FALSE,
        (IPTR)keycancel, 3, MUIM_CallHook, (IPTR)&keycancel_hook, NULL);
    DoMethod(keywindow, MUIM_Notify, MUIA_Window_CloseRequest, TRUE,
        (IPTR)keycancel, 3, MUIM_CallHook, (IPTR)&keycancel_hook, NULL);

    DoMethod(detailsbtn, MUIM_Notify, MUIA_Pressed, FALSE,
        (IPTR)detailsbtn, 3, MUIM_CallHook, (IPTR)&details_hook, NULL);
    DoMethod(disconnectbtn, MUIM_Notify, MUIA_Pressed, FALSE,
        (IPTR)disconnectbtn, 3, MUIM_CallHook, (IPTR)&disconnect_hook, NULL);
    DoMethod(detailsok, MUIM_Notify, MUIA_Pressed, FALSE,
        (IPTR)detailsok, 3, MUIM_CallHook, (IPTR)&detailsok_hook, NULL);
    DoMethod(detailswindow, MUIM_Notify, MUIA_Window_CloseRequest, TRUE,
        (IPTR)detailsok, 3, MUIM_CallHook, (IPTR)&detailsok_hook, NULL);

    SET(window, MUIA_Window_Open, TRUE);
    if (!XGET(window, MUIA_Window_Open))
    {
        PutStr(_(MSG_ERR_WINDOW));
        goto cleanup;
    }

    /* Show where we stand, then look around. */
    Send(WCMD_STATUS);

    while ((LONG)DoMethod(app, MUIM_Application_NewInput, (IPTR)&sigs)
           != MUIV_Application_ReturnID_Quit)
    {
        if (sigs != 0)
        {
            sigs = Wait(sigs | replysig | SIGBREAKF_CTRL_C);
            if (sigs & SIGBREAKF_CTRL_C)
                break;
            if (sigs & replysig)
                HandleReply();
        }
    }

    rc = RETURN_OK;

cleanup:
    if (app != NULL)
        MUI_DisposeObject(app);

    if (WifiWorkerPort != NULL)
    {
        /* Let any command in flight finish before taking the port away. */
        while (busy)
        {
            WaitPort(replyport);
            HandleReply();
        }
        SetSignal(0, SIGF_SINGLE);
        msg->wm_Cmd = WCMD_QUIT;
        msg->wm_Msg.mn_ReplyPort = replyport;
        msg->wm_Msg.mn_Length = sizeof(*msg);
        PutMsg(WifiWorkerPort, &msg->wm_Msg);
        WaitPort(replyport);
        GetMsg(replyport);
        Wait(SIGF_SINGLE);              /* worker has deleted its port */
    }

    FreeVec(msg);
    if (replyport != NULL)
        DeleteMsgPort(replyport);

    Locale_Deinitialize();

    return rc;
}
