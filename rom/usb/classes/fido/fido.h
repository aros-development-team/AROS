#ifndef FIDO_H
#define FIDO_H
/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: fido.class - the FIDO (CTAPHID) interface of a USB security key
          as fido.device (see <devices/fido.h>).
*/

#include <exec/devices.h>
#include <devices/fido.h>

struct NepFidoDevBase
{
    struct Library      np_Library;       /* standard */
    UWORD               np_Flags;         /* various flags */
    BPTR                np_SegList;       /* device seglist */
    struct NepFidoBase *np_ClsBase;       /* pointer to class base */
    struct Library     *np_UtilityBase;   /* cached utilitybase */
};

struct NepClassFido
{
    struct Unit         ncp_Unit;         /* Unit structure */
    ULONG               ncp_UnitNo;       /* Unit number */
    struct NepFidoBase *ncp_ClsBase;      /* Up linkage */
    struct NepFidoDevBase *ncp_DevBase;   /* Device base */
    struct Library     *ncp_Base;         /* Poseidon base */
    struct PsdDevice   *ncp_Device;       /* Up linkage */
    struct PsdConfig   *ncp_Config;       /* Up linkage */
    struct PsdInterface *ncp_Interface;   /* Up linkage */
    struct Task        *ncp_ReadySigTask; /* Task to send ready signal to */
    LONG                ncp_ReadySignal;  /* Signal to send when ready */
    struct Task        *ncp_Task;         /* Subtask */
    struct MsgPort     *ncp_TaskMsgPort;  /* Message Port of Subtask */
    struct PsdPipe     *ncp_EP0Pipe;      /* Endpoint 0 pipe (SET_REPORT without an OUT endpoint) */
    struct PsdEndpoint *ncp_EPOut;        /* Interrupt OUT endpoint, or NULL */
    struct PsdPipe     *ncp_EPOutPipe;
    struct PsdEndpoint *ncp_EPIn;         /* Interrupt IN endpoint */
    struct PsdPipe     *ncp_EPInPipe;
    IPTR                ncp_EPInMaxPktSize;
    IPTR                ncp_EPOutMaxPktSize;
    UWORD               ncp_ReportSize;   /* bytes per report (FIDO_REPORTSIZE) */
    UWORD               ncp_UnitProdID;   /* ProductID of unit */
    UWORD               ncp_UnitVendorID; /* VendorID of unit */
    UWORD               ncp_UnitCfgNum;   /* Config of unit */
    UWORD               ncp_UnitIfNum;    /* Interface number */
    UWORD               ncp_UnitAltIfNum; /* Alternate interface number */
    BOOL                ncp_DenyRequests; /* Do not accept further IO requests */
    BOOL                ncp_AbortRead;    /* if true, abort pending read */
    BOOL                ncp_AbortWrite;   /* if true, abort pending write */
    struct IOStdReq    *ncp_ReadPending;  /* read IORequest pending */
    struct IOStdReq    *ncp_WritePending; /* write IORequest pending */
    struct List         ncp_ReadQueue;    /* List of read requests */
    struct List         ncp_WriteQueue;   /* List of write requests */
    UBYTE              *ncp_InBuf;        /* one report in (ncp_ReportSize) */
    UBYTE              *ncp_OutBuf;       /* one report out */
    STRPTR              ncp_DevIDString;  /* Device ID String */
    STRPTR              ncp_IfIDString;   /* Interface ID String */
    STRPTR              ncp_ProductName;
};

struct NepFidoBase
{
    struct Library      nh_Library;       /* standard */
    UWORD               nh_Flags;         /* various flags */
    struct Library     *nh_UtilityBase;   /* utility base */
    struct NepFidoDevBase *nh_DevBase;    /* base of device created */
    struct List         nh_Units;         /* List of units available */
};

#endif /* FIDO_H */
