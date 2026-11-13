#ifndef CDCACM_H
#define CDCACM_H

#include <devices/serial.h>

#include <devices/usb.h>
#include <devices/usb_cdc.h>
#include <exec/devices.h>

/* Misc */

#define DEFREADBUFLEN 2048
#define NUMREADPIPES 16

struct UsbCDCSerialState
{
    struct UsbSetupData uss_Req;
    UWORD               uss_State;
};

struct NepSerialBase
{
    struct Library      nh_Library;       /* standard */
    UWORD               nh_Flags;         /* various flags */

    struct Library     *nh_UtilityBase;   /* utility base */

    struct NepSerDevBase *nh_DevBase;     /* base of device created */
    struct List         nh_Units;         /* List of units available */
};

struct NepSerDevBase
{
    struct Library      np_Library;       /* standard */
    UWORD               np_Flags;         /* various flags */

    BPTR                np_SegList;       /* device seglist */
    struct NepSerialBase *np_ClsBase;     /* pointer to class base */
    struct Library     *np_UtilityBase;   /* cached utilitybase */
};

struct NepClassSerial
{
    struct Unit         ncp_Unit;         /* Unit structure */
    ULONG               ncp_UnitNo;       /* Unit number */
    struct NepSerDevBase *ncp_DevBase;    /* Device base */
    struct Library     *ncp_Base;         /* Poseidon base */
    struct PsdDevice   *ncp_Device;       /* Up linkage */
    struct PsdInterface *ncp_Interface;   /* Up linkage */
    struct Task        *ncp_ReadySigTask; /* Task to send ready signal to */
    LONG                ncp_ReadySignal;  /* Signal to send when ready */
    LONG                ncp_AbortSignal;  /* Signal to abort write on */
    struct Task        *ncp_Task;         /* Subtask */
    struct MsgPort     *ncp_TaskMsgPort;  /* Message Port of Subtask */

    struct PsdInterface *ncp_DataIf;      /* CDC Data Interface */

    struct PsdPipe     *ncp_EP0Pipe;      /* Endpoint 0 pipe */
    struct PsdEndpoint *ncp_EPOut;        /* OUT Endpoint */
    struct PsdPipeStream *ncp_EPOutStream; /* OUT Endpoint stream */
    struct PsdEndpoint *ncp_EPIn;         /* IN Endpoint */
    struct PsdPipeStream *ncp_EPInStream; /* IN Endpoint stream */
    struct PsdEndpoint *ncp_EPInt;        /* NOTIFY Endpoint */
    struct PsdPipe     *ncp_EPIntPipe;    /* NOTIFY Endpoint pipe */
    struct MsgPort     *ncp_DevMsgPort;   /* Message Port for IOParReq */
    UWORD               ncp_UnitProdID;   /* ProductID of unit */
    UWORD               ncp_UnitVendorID; /* VendorID of unit */
    UWORD               ncp_UnitIfNum;    /* Interface number */
    UWORD               ncp_UnitAltIfNum; /* Alternate interface number */
    BOOL                ncp_DenyRequests; /* Do not accept further IO requests */
    BOOL                ncp_DevSuspend;   /* suspend things */

    struct UsbCDCSerialState ncp_SerialStateReq; /* Serial State stuff */

    struct IOExtSer    *ncp_WritePending; /* write IORequest pending */
    struct List         ncp_ReadQueue;    /* List of read requests */
    struct List         ncp_WriteQueue;   /* List of write requests */
};

#endif /* CDCACM_H */
