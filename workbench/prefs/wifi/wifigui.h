/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Shared layout between the WiFi window and its worker process.
*/

#ifndef WIFIGUI_H
#define WIFIGUI_H

#include <exec/types.h>
#include <exec/ports.h>
#include <exec/tasks.h>

#define WIFI_MAX_NETS       40
#define WIFI_MAX_DEVS       8       /* radios offered in the chooser */
#define WIFI_MAX_UNITS      4       /* units probed per driver */
#define WIFI_DEV_MAX        64
#define WIFI_SSID_MAX       33      /* 32 octets plus terminator */
#define WIFI_KEY_MAX        64      /* 63-character passphrase plus terminator */

/* One scan result, as the window wants to show it. */
struct WifiNet
{
    TEXT    wn_SSID[WIFI_SSID_MAX];
    LONG    wn_Signal;              /* dBm */
    LONG    wn_Channel;
    TEXT    wn_Protection[8];       /* "open", "WPA", "WPA2" */
    LONG    wn_Known;               /* already in Wireless.prefs */
};

enum
{
    WCMD_DEVICES,       /* which drivers are wireless-capable */
    WCMD_STATUS,        /* what are we associated to, if anything */
    WCMD_SCAN,          /* list what is on the air (takes seconds) */
    WCMD_CONNECT,       /* remember wc_SSID/wc_Key and hand it to the supplicant */
    WCMD_DISCONNECT,    /* stop the supplicant and leave the network */
    WCMD_QUIT
};

/*
 * Result codes a CONNECT can come back with. WRES_NEEDKEY is the interesting
 * one: the worker owns all knowledge of the stored configuration, so it - not
 * the window - decides whether a passphrase has to be asked for.
 */
enum
{
    WRES_OK,
    WRES_FAILED,
    WRES_NEEDKEY
};

/*
 * One message, allocated once by the window and passed back and forth. The
 * device is touched only by the worker: a scan blocks for seconds inside the
 * driver, and the window has to stay alive while it runs.
 */
struct WifiMsg
{
    struct Message  wm_Msg;

    ULONG           wm_Cmd;
    LONG            wm_Result;
    TEXT            wm_Status[96];              /* for the status line */

    TEXT            wm_Device[WIFI_DEV_MAX];
    LONG            wm_Unit;

    /* DEVICES output */
    LONG            wm_DeviceCount;
    TEXT            wm_Devices[WIFI_MAX_DEVS][WIFI_DEV_MAX];
    LONG            wm_DeviceUnits[WIFI_MAX_DEVS];

    /* CONNECT input, STATUS output */
    TEXT            wm_SSID[WIFI_SSID_MAX];
    TEXT            wm_Key[WIFI_KEY_MAX];
    LONG            wm_Protected;

    /* STATUS output. Everything the details window shows is gathered here, so
     * what it displays is never older than the last status request. */
    LONG            wm_Wireless;                /* device can do S2_GETNETWORKS */
    LONG            wm_Associated;
    TEXT            wm_Address[20];             /* "" when the stack has none */
    TEXT            wm_Netmask[20];
    TEXT            wm_Broadcast[20];
    TEXT            wm_Gateway[20];
    TEXT            wm_DNS[64];
    TEXT            wm_MAC[20];
    TEXT            wm_BSSID[20];
    TEXT            wm_Interface[16];

    /* SCAN output */
    LONG            wm_Count;
    struct WifiNet  wm_Nets[WIFI_MAX_NETS];
};

/* Set up by the worker before it signals the window that it is ready. */
extern struct MsgPort *WifiWorkerPort;
extern struct Task *WifiMainTask;

VOID WifiWorker(VOID);

#endif /* WIFIGUI_H */
