#ifndef BTGATT_H
#define BTGATT_H

/*
 * btgatt.class - the GATT server of the Bluetooth stack. GATT services are
 * registered with bluetooth.library (btAddServiceRecord(), BSVP_ATT) by
 * whoever has one to offer, and the library answers the devices that
 * connect. This class decides what they get to see: it enables the services
 * its user wants offered, adds the ones every GATT server has, and makes
 * the radios advertise so that devices can find the machine at all.
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

#define BTGATT_MAXDISABLED 32 /* services the user can switch off */
#define BTGATT_MAXENTRIES  64 /* services the window lists */

/* The class configuration: every service is offered unless it is listed
   here. Services are told apart by their UUID (128 bit, little-endian). */
struct BTGattCfg
{
    ULONG gc_ChunkID;
    ULONG gc_Length;
    ULONG gc_Advertise;   /* let LE devices find and connect to this machine */
    ULONG gc_NumDisabled;
    UBYTE gc_Disabled[BTGATT_MAXDISABLED][16];
};

/* A line of the service list */
struct BTGattEntry
{
    UBYTE ge_Key[16];
    BOOL  ge_Mandatory;   /* cannot be switched off */
    BOOL  ge_Offered;
    char  ge_Name[48];
    char  ge_UUID[40];
    char  ge_Owner[40];
    char  ge_Chars[8];
};

struct BTGattBase
{
    struct Library      nh_Library;       /* standard */
    UWORD               nh_Flags;         /* various flags */

    struct Library     *nh_UtilityBase;   /* utility base */

    struct BTGattCfg    nh_Cfg;           /* the configuration in effect */
    BOOL                nh_UsingDefaultCfg;
    APTR                nh_GAPRecord;     /* the services we add ourselves */
    APTR                nh_GATTRecord;
    APTR                nh_DISRecord;

    /* settings window */
    struct Task        *nh_GUITask;
    struct Library     *nh_BtBase;        /* bluetooth.library base (GUI task) */
    struct Library     *nh_MUIBase;
    struct Library     *nh_IntBase;
    struct BTGattCfg    nh_GUICfg;        /* what the window is editing */
    struct BTGattEntry  nh_Entries[BTGATT_MAXENTRIES];
    struct Hook         nh_DispHook;
    Object             *nh_App;
    Object             *nh_MainWindow;
    Object             *nh_ListObj;
    Object             *nh_OfferObj;
    Object             *nh_AdvObj;
    Object             *nh_SaveObj;
    Object             *nh_UseObj;
    Object             *nh_CloseObj;
    Object             *nh_AboutMI;
    Object             *nh_MUIPrefsMI;
};

AROS_UFP0(void, bGUITask);

#endif /* BTGATT_H */
