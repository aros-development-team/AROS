#ifndef RAWWRAP_H
#define RAWWRAP_H

#include <intuition/intuition.h>
#include <intuition/intuitionbase.h>
#include <libraries/mui.h>
#include <libraries/gadtools.h>
#include <exec/devices.h>

#if defined(__GNUC__)
# pragma pack(2)
#endif

#define ID_ABOUT        0x55555555
#define ID_STORE_CONFIG 0xaaaaaaaa
#define ID_DEF_CONFIG   0xaaaaaaab

struct ClsGlobalCfg
{
    ULONG cgc_ChunkID;
    ULONG cgc_Length;
    ULONG cgc_BindVendor;
    ULONG cgc_BindAll;
};

struct ClsDevCfg
{
    ULONG cdc_ChunkID;
    ULONG cdc_Length;
    ULONG cdc_OutNakTimeout;
    ULONG cdc_InNakTimeout;
    ULONG cdc_InBufferMode;
    ULONG cdc_InBufferSize;
    ULONG cdc_ShortReadTerm;
    ULONG cdc_DefaultUnit;
    ULONG cdc_UnitExclusive;
};

#if defined(__GNUC__)
# pragma pack()
#endif

/* Misc */

#define READBUFCHUNK 256

#define BUFMODE_NO        0
#define BUFMODE_READAHEAD 1
#define BUFMODE_READONREQ 2

struct NepRawDevBase
{
    struct Library      np_Library;       /* standard */
    UWORD               np_Flags;         /* various flags */

    BPTR                np_SegList;       /* device seglist */
    struct NepRawWrapBase *np_ClsBase;    /* pointer to class base */
    struct Library     *np_UtilityBase;   /* cached utilitybase */
};

struct NepClassRawWrap
{
    struct Unit         ncp_Unit;         /* Unit structure */
    ULONG               ncp_UnitNo;       /* Unit number */
    struct NepRawWrapBase *ncp_ClsBase;   /* Up linkage */
    struct NepRawDevBase *ncp_DevBase;    /* Device base */
    struct Library     *ncp_Base;         /* Poseidon base */
    struct PsdDevice   *ncp_Device;       /* Up linkage */
    struct PsdConfig   *ncp_Config;       /* Up linkage */
    struct PsdInterface *ncp_Interface;   /* Up linkage */
    struct Task        *ncp_ReadySigTask; /* Task to send ready signal to */
    LONG                ncp_ReadySignal;  /* Signal to send when ready */
    struct Task        *ncp_Task;         /* Subtask */
    struct MsgPort     *ncp_TaskMsgPort;  /* Message Port of Subtask */

    struct PsdPipe     *ncp_EP0Pipe;      /* Endpoint 0 pipe */
    struct PsdEndpoint *ncp_EPOut;        /* Endpoint 1 */
    struct PsdPipe     *ncp_EPOutPipe;    /* Endpoint 1 pipe */
    struct PsdEndpoint *ncp_EPIn;         /* Endpoint 2 */
    struct PsdPipe     *ncp_EPInPipe;     /* Endpoint 2 pipe */
    IPTR                ncp_EPInMaxPktSize; /* Endpoint 2 max pkt size */
    struct MsgPort     *ncp_DevMsgPort;   /* Message Port for IOParReq */
    UWORD               ncp_UnitProdID;   /* ProductID of unit */
    UWORD               ncp_UnitVendorID; /* VendorID of unit */
    UWORD               ncp_UnitCfgNum;   /* Config of unit */
    UWORD               ncp_UnitIfNum;    /* Interface number */
    UWORD               ncp_UnitAltIfNum; /* Alternate interface number */
    BOOL                ncp_DenyRequests; /* Do not accept further IO requests */
    UBYTE              *ncp_ReadBuffer;   /* Read Buffer Address */
    ULONG               ncp_RBufSize;     /* Size of read buffer */
    ULONG               ncp_RBufWOffset;  /* Offset inside buffer for incoming next byte(s) */
    ULONG               ncp_RBufROffset;  /* Offset inside buffer for next read */
    ULONG               ncp_RBufFull;     /* Bytes in buffer */
    ULONG               ncp_RBufWrap;     /* Offset where read buffer wraps around */
    BOOL                ncp_ShortPktRead; /* if short packet has been read, abort */
    BOOL                ncp_AbortRead;    /* if true, abort pending read */
    BOOL                ncp_AbortWrite;   /* if true, abort pending write */

    struct IOStdReq    *ncp_ReadPending;  /* read IORequest pending */
    struct IOStdReq    *ncp_WritePending; /* write IORequest pending */
    struct List         ncp_ReadQueue;    /* List of read requests */
    struct List         ncp_WriteQueue;   /* List of write requests */

    STRPTR              ncp_DevIDString;  /* Device ID String */
    STRPTR              ncp_IfIDString;   /* Interface ID String */

    BOOL                ncp_UsingDefaultCfg;
    struct ClsDevCfg   *ncp_CDC;

    struct Library     *ncp_MUIBase;       /* MUI master base */
    struct Library     *ncp_PsdBase;       /* Poseidon base */
    struct Library     *ncp_IntBase;       /* Intuition base */
    struct Task        *ncp_GUITask;       /* GUI Task */
    struct NepClassHid *ncp_GUIBinding;    /* Window of binding that's open */

    Object             *ncp_App;
    Object             *ncp_MainWindow;

    Object             *ncp_BindVendorObj;
    Object             *ncp_BindAllObj;

    Object             *ncp_UnitObj;
    Object             *ncp_UnitExclObj;
    Object             *ncp_InNakTimeoutObj;
    Object             *ncp_OutNakTimeoutObj;
    Object             *ncp_InBufferModeObj;
    Object             *ncp_InBufferSizeObj;
    Object             *ncp_ShortReadTermObj;

    Object             *ncp_UseObj;
    Object             *ncp_SetDefaultObj;
    Object             *ncp_CloseObj;

    Object             *ncp_AboutMI;
    Object             *ncp_UseMI;
    Object             *ncp_SetDefaultMI;
    Object             *ncp_MUIPrefsMI;

};

struct NepRawWrapBase
{
    struct Library      nh_Library;       /* standard */
    UWORD               nh_Flags;         /* various flags */

    struct Library     *nh_UtilityBase;   /* utility base */

    struct NepRawDevBase *nh_DevBase;       /* base of device created */
    struct List         nh_Units;         /* List of units available */

    struct ClsGlobalCfg nh_CurrentCGC;

    struct NepClassRawWrap nh_DummyNCP;     /* Dummy ncp for default config */
};


#endif /* RAWWRAP_H */
