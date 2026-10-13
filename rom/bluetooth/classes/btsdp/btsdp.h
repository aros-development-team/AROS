#ifndef BTSDP_H
#define BTSDP_H

/*
 * btsdp.class - the SDP server of the Bluetooth stack, as far as its user
 * is concerned. Classic services (a serial port, ...) are registered with
 * bluetooth.library (btAddServiceRecord()) by the classes and programs that
 * serve them, and the library answers the devices that browse the records.
 * This class decides which of them are offered, and whether a device that
 * merely scans for the machine is told about them.
 */

#include LC_LIBDEFS_FILE

#include <aros/libcall.h>
#include <aros/asmcall.h>
#include <aros/symbolsets.h>

#include <exec/types.h>
#include <exec/lists.h>
#include <exec/memory.h>
#include <exec/ports.h>
#include <utility/utility.h>
#include <utility/hooks.h>
#include <dos/dos.h>
#include <intuition/intuition.h>
#include <libraries/mui.h>

#include <libraries/bluetooth.h>
#include <libraries/btclass.h>

#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/bluetooth.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <proto/alib.h>

#include <string.h>

#define ID_ABOUT        0x55555555
#define ID_STORE_CONFIG 0xaaaaaaaa
#define ID_DEF_CONFIG   0xaaaaaaab
#define ID_SELECT       0xaaaaaaac
#define ID_TOGGLE       0xaaaaaaad
#define ID_OFFER        0xaaaaaaae

#define BTSDP_MAXDISABLED 32 /* services the user can switch off */
#define BTSDP_MAXENTRIES  64 /* services the window lists */

/* What a classic service is known by in the configuration */
struct BTSdpKey
{
    UWORD sk_UUID16;      /* service class */
    UWORD sk_Protocol;    /* BSVP_RFCOMM / BSVP_L2CAP */
    UWORD sk_Port;        /* RFCOMM server channel or PSM */
    UWORD sk_Pad;
};

/* The class configuration: every service is offered unless it is listed. */
struct BTSdpCfg
{
    ULONG sc_ChunkID;
    ULONG sc_Length;
    ULONG sc_EIRServices; /* tell scanning devices the name and the services */
    ULONG sc_NumDisabled;
    struct BTSdpKey sc_Disabled[BTSDP_MAXDISABLED];
};

/* A line of the service list */
struct BTSdpEntry
{
    struct BTSdpKey se_Key;
    BOOL  se_Offered;
    char  se_Name[48];
    char  se_UUID[12];
    char  se_Via[24];
    char  se_Owner[40];
};

struct BTSdpBase
{
    struct Library      nh_Library;       /* standard */
    UWORD               nh_Flags;         /* various flags */

    struct Library     *nh_UtilityBase;   /* utility base */

    struct BTSdpCfg     nh_Cfg;           /* the configuration in effect */
    BOOL                nh_UsingDefaultCfg;

    /* settings window */
    struct Task        *nh_GUITask;
    struct Library     *nh_BtBase;        /* bluetooth.library base (GUI task) */
    struct Library     *nh_MUIBase;
    struct Library     *nh_IntBase;
    struct BTSdpCfg     nh_GUICfg;        /* what the window is editing */
    struct BTSdpEntry   nh_Entries[BTSDP_MAXENTRIES];
    struct Hook         nh_DispHook;
    Object             *nh_App;
    Object             *nh_MainWindow;
    Object             *nh_ListObj;
    Object             *nh_OfferObj;
    Object             *nh_EIRObj;
    Object             *nh_SaveObj;
    Object             *nh_UseObj;
    Object             *nh_CloseObj;
    Object             *nh_AboutMI;
    Object             *nh_MUIPrefsMI;
};

AROS_UFP0(void, bGUITask);

#endif /* BTSDP_H */
