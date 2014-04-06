/*
 * WPA Supplicant - MUI GUI
 * Copyright (c) 2012, Neil Cafferkey
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Alternatively, this software may be distributed under the terms of BSD
 * license.
 *
 * See README and COPYING for more details.
 */

#include "includes.h"

#include "common.h"
#include "rsn_supp/wpa.h"
#include "config.h"
#include "wpa_supplicant_i.h"
#include "driver_i.h"
#include "common/ieee802_11_defs.h"
#include "ibss_rsn.h"
#include "scan.h"

#include <exec/types.h>
#include <utility/hooks.h>
#include <libraries/mui.h>
#include <dos/dos.h>
#include <intuition/gadgetclass.h>
#include <intuition/icclass.h>

#include <proto/exec.h>
#include <proto/icon.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <clib/alib_protos.h>


struct wpa_global *gui_global;
struct Task *main_task;
static struct wpa_supplicant *wpa_sup;
static Object *app, *window, *str1, *button1, *button2, *list;

static CONST_STRPTR enc_names[] =
{
    (STRPTR) "None",
    (STRPTR) "WEP",
    (STRPTR) "WPA",
    (STRPTR) "WPA2"
};

void wpa_gui_amiga_inspect_scan_results(struct wpa_supplicant *wpa_s,
    struct wpa_scan_results *scan_res)
{
    struct wpa_scan_res *bss;
    size_t i;
    const u8 *ie;
    UWORD enc;

    DoMethod(list, MUIM_List_Clear);

    for (i = 0; i < scan_res->num; i++)
    {
        bss = scan_res->res[i];

        ie = wpa_scan_get_ie(bss, WLAN_EID_SSID);

        if (wpa_scan_get_ie(bss, WLAN_EID_RSN) != NULL)
            enc = 3;
        else if (wpa_scan_get_ie(bss, (u8) WPA_IE_VENDOR_TYPE) != NULL)
            enc = 2;
        else if ((bss->caps & IEEE80211_CAP_PRIVACY) != 0)
            enc = 1;
        else
            enc = 0;

        STRPTR name = AllocVec(ie[1] + 1, MEMF_CLEAR);
        CONST_STRPTR *item = AllocMem(sizeof(STRPTR) * 3, MEMF_ANY);

        if (name != NULL && item != NULL)
        {
            CopyMem(ie + 2, name, ie[1]);
            item[0] = name;
            item[1] = enc_names[enc];
            item[2] = NULL;
            DoMethod(list, MUIM_List_InsertSingle, item,
                MUIV_List_Insert_Top);
        }
    }
}


static IPTR DisplayFunc(struct Hook *hook, STRPTR *columns, STRPTR *entry)
{
    if (entry == NULL)
    {
        columns[0] = "Name";
        columns[1] = "Encryption";
    }
    else
    {
        columns[0] = entry[0];
        columns[1] = entry[1];
    }
    return TRUE;
}


static IPTR ScanFunc(struct Hook *hook, Object * caller, void *data)
{
    wpa_supplicant_req_scan(wpa_sup, 0, 0);

    return TRUE;
}


static IPTR ConnectFunc(struct Hook *hook, Object * caller, void *data)
{
    LONG active = 0;
    STRPTR *entry;
    STRPTR passphrase;

    get(list, MUIA_List_Active, &active);
    if (active == MUIV_List_Active_Off)
        return FALSE;

    DoMethod(list, MUIM_List_GetEntry, active, &entry);

    /* Look for an existing configuration for this network */
    struct wpa_ssid *ssid = wpa_sup->conf->ssid;
    struct wpa_ssid *ls_ssid = NULL;
    while (ssid != NULL
        && os_strncmp(ssid->ssid, entry[0], ssid->ssid_len) != 0)
    {
        if (os_strncmp(ssid->ssid, "linksys_", ssid->ssid_len) == 0)
            ls_ssid = ssid;
        ssid = ssid->next;
    }

    /* If not found, create a new configuration for this network */
    if (ssid == NULL)
    {
        passphrase = (STRPTR) XGET(str1, MUIA_String_Contents);
        ssid = wpa_config_add_network(wpa_sup->conf);
        if (ssid != NULL)
        {
            wpa_config_set_network_defaults(ssid);
            ssid->ssid = os_strdup(entry[0]);
            ssid->ssid_len = strlen(entry[0]);
            if (entry[1] == enc_names[2] || entry[1] == enc_names[3])
            {
                ssid->passphrase = os_strdup(passphrase);
                wpa_config_update_psk(ssid);
            }
            else if (entry[1] == enc_names[1])
            {
            }
        }
    }

    /* Connect to network */
    if (ssid != NULL)
        wpa_supplicant_select_network(wpa_sup, ssid);

    DoMethod(list, MUIM_List_Redraw, MUIV_List_Redraw_Active);

    return TRUE;
}


VOID MUIGUI(VOID)
{
    struct Hook scan_hook, display_hook, connect_hook;
    struct DiskObject *dobj;
    ULONG sigs = 0;

    wpa_sup = &gui_global->ifaces[0];

    scan_hook.h_Entry = HookEntry;
    scan_hook.h_SubEntry = (HOOKFUNC) ScanFunc;
    display_hook.h_Entry = HookEntry;
    display_hook.h_SubEntry = (HOOKFUNC) DisplayFunc;
    connect_hook.h_Entry = HookEntry;
    connect_hook.h_SubEntry = (HOOKFUNC) ConnectFunc;

    app = ApplicationObject,
        MUIA_Application_Title, "Wireless Manager",
        MUIA_Application_DiskObject,
            (IPTR) (dobj = GetDiskObject("ENV:SYS/def_Wireless")),
        SubWindow, window = WindowObject,
            MUIA_Window_Title, "Wireless Manager",
            MUIA_Window_Activate, TRUE,
            MUIA_Window_CloseGadget, FALSE,
            MUIA_Window_Width, 256,
            WindowContents, VGroup,
                Child, ListviewObject, MUIA_Listview_List,
                    (list = ListObject, MUIA_List_DisplayHook, &display_hook,
                        MUIA_List_Format, "WEIGHT=500 BAR, ",
                        MUIA_List_Title, TRUE,
                        End),
                    End,
                Child, HGroup,
                    Child, (IPTR) Label2("Passphrase"),
                    Child, (str1 = StringObject, StringFrame, End),
                    End,
                Child, HGroup,
                    Child, button1 = SimpleButton("Scan"),
                    Child, button2 = SimpleButton("Connect"),
                    End,
                End,
            End,
        End;

    if (app != NULL)
    {
        DoMethod(button1, MUIM_Notify, MUIA_Pressed, FALSE,
            (IPTR) button1, 3, MUIM_CallHook, &scan_hook, list);

        DoMethod(button2, MUIM_Notify, MUIA_Pressed, FALSE,
            (IPTR) list, 3, MUIM_CallHook, &connect_hook, NULL);

        set(app, MUIA_Application_Iconified, TRUE);
        set(window, MUIA_Window_Open, TRUE);

        while ((sigs & SIGBREAKF_CTRL_C) == 0
            && DoMethod(app, MUIM_Application_NewInput,
                (IPTR) &sigs) != MUIV_Application_ReturnID_Quit)
        {
            if (sigs != 0)
                sigs = Wait(sigs | SIGBREAKF_CTRL_C);
        }

        MUI_DisposeObject(app);
        FreeDiskObject(dobj);
    }

    Signal(main_task, SIGF_SINGLE);
}
