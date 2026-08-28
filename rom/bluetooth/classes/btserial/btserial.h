#ifndef BTSERIAL_H
#define BTSERIAL_H

/*
 * btserial.class - Bluetooth Serial Port Profile (SPP). Binds to RFCOMM
 * services of registered devices and exposes each as a unit of
 * btserial.device, the way Poseidon's USB serial classes (cdcacm and
 * friends) expose usbmodem.device units.
 */

#include LC_LIBDEFS_FILE

#include <aros/libcall.h>
#include <aros/asmcall.h>
#include <aros/symbolsets.h>

#include <exec/types.h>
#include <exec/lists.h>
#include <exec/memory.h>
#include <exec/devices.h>
#include <exec/io.h>
#include <exec/ports.h>
#include <exec/errors.h>
#include <devices/serial.h>
#include <devices/newstyle.h>
#include <utility/utility.h>
#include <dos/dos.h>

#include <libraries/bluetooth.h>
#include <libraries/btclass.h>

#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/bluetooth.h>

#include <string.h>

#define NewList NEWLIST

/* Misc */

#define DEFREADBUFLEN 2048
#define BTSER_RINGSIZE 8192      /* readahead ring per unit */
#define BTSER_CHUNK    127       /* fallback write chunk (RFCOMM default frame) */

struct BTSerialBase
{
    struct Library      nh_Library;       /* standard */
    UWORD               nh_Flags;         /* various flags */

    struct Library     *nh_UtilityBase;   /* utility base */

    struct BTSerDevBase *nh_DevBase;      /* base of device created */
    struct List         nh_Units;         /* List of units available */
    APTR                nh_Record;        /* our Serial Port SDP record (peers connect to it) */
};

struct BTSerDevBase
{
    struct Library      nsd_Library;      /* standard */
    UWORD               nsd_Flags;        /* various flags */

    BPTR                nsd_SegList;      /* device seglist */
    struct BTSerialBase *nsd_ClsBase;     /* pointer to class base */
    struct Library     *nsd_UtilityBase;  /* cached utilitybase */
};

struct BTSerialUnit
{
    struct Unit         nsu_Unit;         /* Unit structure */
    ULONG               nsu_UnitNo;       /* Unit number */
    struct BTSerDevBase *nsu_DevBase;     /* Device base */
    struct Library     *nsu_Base;         /* bluetooth.library base (unit task) */
    struct BtDevice    *nsu_Device;       /* Up linkage */
    struct BtService   *nsu_Service;      /* the RFCOMM service bound */
    struct BtEndpoint  *nsu_Endpoint;     /* its RFCOMM endpoint */
    struct Task        *nsu_ReadySigTask; /* Task to send ready signal to */
    LONG                nsu_ReadySignal;  /* Signal to send when ready */
    struct Task        *nsu_Task;         /* Subtask */
    struct MsgPort     *nsu_TaskMsgPort;  /* Message Port of Subtask */

    /* the persistent identity of the unit (units survive rebinds) */
    UBYTE               nsu_UnitAddr[6];  /* device BD address */
    UWORD               nsu_UnitChannel;  /* RFCOMM server channel */

    BOOL                nsu_DenyRequests; /* Do not accept further IO requests */
    BOOL                nsu_DevSuspend;   /* suspend things */

    APTR                nsu_ReadCh;       /* read channel (one read always pending) */
    APTR                nsu_WriteCh;      /* write channel */
    UBYTE               nsu_ReadBuf[1024];
    BOOL                nsu_ReadPosted;
    BOOL                nsu_WriteBusy;    /* a write is in flight on nsu_WriteCh */

    /* readahead ring */
    UBYTE               nsu_Ring[BTSER_RINGSIZE];
    ULONG               nsu_RingHead;
    ULONG               nsu_RingTail;

    struct IOExtSer    *nsu_WritePending; /* write IORequest in progress */
    BOOL                nsu_WriteAbort;   /* AbortIO() hit the pending write */
    ULONG               nsu_WriteOffset;  /* bytes of it already sent */
    struct List         nsu_ReadQueue;    /* List of read requests */
    struct List         nsu_WriteQueue;   /* List of write requests */
};

struct BTSerialUnit * GM_UNIQUENAME(bAttemptServiceBinding)(struct BTSerialBase *nh, struct BtService *bsv);
struct BTSerialUnit * GM_UNIQUENAME(bForceServiceBinding)(struct BTSerialBase *nh, struct BtService *bsv);
void GM_UNIQUENAME(bReleaseServiceBinding)(struct BTSerialBase *nh, struct BTSerialUnit *nsu);

AROS_UFP0(void, GM_UNIQUENAME(bSerialTask));

#include "dev.h"

#endif /* BTSERIAL_H */
