#ifndef BTBATTERY_H
#define BTBATTERY_H

/*
 * btbattery.class - the charge of Bluetooth devices. Binds to the GATT
 * Battery Service (0x180F) of registered devices, reads the Battery Level
 * and follows its notifications, and shows it to the system as a telemetry
 * device (a subclass of telemetry.hidd) for as long as the device is
 * connected.
 */

#include LC_LIBDEFS_FILE

#include <aros/libcall.h>
#include <aros/asmcall.h>
#include <aros/symbolsets.h>

#include <exec/types.h>
#include <exec/lists.h>
#include <exec/memory.h>
#include <exec/ports.h>
#include <exec/semaphores.h>
#include <devices/timer.h>
#include <utility/utility.h>
#include <dos/dos.h>

#include <libraries/bluetooth.h>
#include <libraries/btclass.h>

#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/bluetooth.h>

#include <string.h>

#define UUID_BATTERY_SERVICE 0x180f
#define UUID_BATTERY_LEVEL   0x2a19

#define BTBAT_REFRESH_SECS 300 /* a level is read again this often ... */
#define BTBAT_RETRY_SECS   10  /* ... and this soon if it could not be read */
#define BTBAT_SETTLE_SECS  5   /* a new link is left alone this long */

struct BTBatBase
{
    struct Library      nh_Library;       /* standard */
    UWORD               nh_Flags;         /* various flags */

    struct Library     *nh_UtilityBase;   /* utility base */

    /* the telemetry device class, made when the first level is known */
    struct SignalSemaphore nh_TelLock;
    struct Library     *nh_TelHiddBase;   /* telemetry.hidd */
    APTR                nh_TelClass;      /* our CLID_Hidd_Telemetry subclass */
    APTR                nh_TelRoot;       /* where the devices are registered */
    IPTR                nh_TelAB[3];      /* attr bases: Hidd, Hidd_Telemetry, HW */
    IPTR                nh_TelMB[2];      /* method bases: HW, Hidd_Telemetry */
};

struct BTBatBinding
{
    struct BTBatBase   *bb_ClsBase;       /* Up linkage */
    struct Library     *bb_Base;          /* bluetooth.library base (binding task) */
    struct BtDevice    *bb_Device;
    struct BtService   *bb_Service;       /* the Battery Service bound */
    struct BtEndpoint  *bb_Endpoint;      /* its Battery Level characteristic */
    struct Task        *bb_ReadySigTask;  /* Task to send ready signal to */
    LONG                bb_ReadySignal;   /* Signal to send when ready */
    struct Task        *bb_Task;          /* Subtask */
    struct MsgPort     *bb_TaskMsgPort;   /* channels complete here */
    struct MsgPort     *bb_EventPort;     /* the stack's device events */
    APTR                bb_EventHandler;
    struct MsgPort     *bb_TimerPort;
    struct timerequest *bb_TimerReq;
    BOOL                bb_TimerOpen;
    BOOL                bb_TimerPending;

    APTR                bb_NotifyCh;      /* notifications of the level */
    APTR                bb_ReadCh;        /* reads of the level */
    BOOL                bb_CanNotify;
    BOOL                bb_NotifyPosted;
    BOOL                bb_ReadBusy;
    BOOL                bb_Connected;     /* as far as we know */
    UBYTE               bb_NotifyBuf[4];
    UBYTE               bb_ReadBuf[4];
    ULONG               bb_Handle;        /* value handle of the level */

    LONG                bb_Level;         /* percent, -1: not known */
    APTR                bb_Object;        /* the telemetry device, while connected */
    BOOL                bb_Complained;    /* that there is nowhere to show the level */
    char                bb_Name[96];      /* what it is called */
};

AROS_UFP0(void, bBatteryTask);

#endif /* BTBATTERY_H */
