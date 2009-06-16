#ifndef PRINTER_H
#define PRINTER_H

#include <intuition/intuition.h>
#include <intuition/intuitionbase.h>
#include <libraries/mui.h>
#include <libraries/gadtools.h>
#include <exec/devices.h>

#define ID_ABOUT        0x55555555
#define ID_STORE_CONFIG 0xaaaaaaaa

struct ClsGlobalCfg
{
    ULONG cgc_ChunkID;
    ULONG cgc_Length;
    ULONG cgc_EpsonInit;
    ULONG cgc_SoftReset;
};

struct NepPrinterBase
{
    struct Library      nh_Library;       /* standard */
    UWORD               nh_Flags;         /* various flags */

    struct Library     *nh_UtilityBase;   /* utility base */
    
    struct NepPrtDevBase *nh_DevBase;       /* base of device created */
    struct List         nh_Units;         /* List of units available */

    struct Library     *nh_MUIBase;       /* MUI master base */
    struct Library     *nh_PsdBase;       /* Poseidon base */
    struct Library     *nh_IntBase;       /* Intuition base */
    struct Task        *nh_GUITask;       /* GUI Task */

    BOOL                nh_UsingDefaultCfg;
    struct ClsGlobalCfg nh_CurrentCGC;

    Object             *nh_App;
    Object             *nh_MainWindow;
    Object             *nh_EpsonInitObj;
    Object             *nh_SoftResetObj;
    Object             *nh_UseObj;
    Object             *nh_CloseObj;

    Object             *nh_AboutMI;
    Object             *nh_UseMI;
    Object             *nh_MUIPrefsMI;
};

struct NepPrtDevBase
{
    struct Library      np_Library;       /* standard */
    UWORD               np_Flags;         /* various flags */

    BPTR                np_SegList;       /* device seglist */
    struct NepPrinterBase *np_ClsBase;    /* pointer to class base */
    struct Library     *np_UtilityBase;   /* cached utilitybase */
};

struct NepClassPrinter
{
    struct Unit         ncp_Unit;         /* Unit structure */
    ULONG               ncp_UnitNo;       /* Unit number */
    struct NepPrinterBase *ncp_ClsBase;   /* Up linkage */
    struct NepPrtDevBase *ncp_DevBase;    /* Device base */
    struct Library     *ncp_Base;         /* Poseidon base */
    struct PsdDevice   *ncp_Device;       /* Up linkage */
    struct PsdConfig   *ncp_Config;       /* Up linkage */
    struct PsdInterface *ncp_Interface;   /* Up linkage */
    struct Task        *ncp_ReadySigTask; /* Task to send ready signal to */
    LONG                ncp_ReadySignal;  /* Signal to send when ready */
    LONG                ncp_AbortSignal;  /* Signal to send to abort a write request */

    struct Task        *ncp_Task;         /* Subtask */
    struct MsgPort     *ncp_TaskMsgPort;  /* Message Port of Subtask */

    struct PsdPipe     *ncp_EP0Pipe;      /* Endpoint 0 pipe */
    struct PsdEndpoint *ncp_EPOut;        /* Endpoint 1 */
    struct PsdPipeStream *ncp_EPOutStream; /* Endpoint 1 pipe */
    struct PsdEndpoint *ncp_EPIn;         /* Endpoint 2 */
    struct PsdPipeStream *ncp_EPInStream; /* Endpoint 2 pipe */
    struct MsgPort     *ncp_DevMsgPort;   /* Message Port for IOParReq */
    UWORD               ncp_UnitProdID;   /* ProductID of unit */
    UWORD               ncp_UnitVendorID; /* VendorID of unit */
    UWORD               ncp_UnitCfgNum;   /* Config of unit */
    UWORD               ncp_UnitIfNum;    /* Interface number */
    UWORD               ncp_UnitAltIfNum; /* Alternate interface number */
    BOOL                ncp_FlushBuffer;  /* Flush last buffer */
    BOOL                ncp_DevSuspend;   /* Suspend printing */
    BOOL                ncp_DenyRequests; /* Do not accept further IO requests */
    struct IOExtPar    *ncp_WritePending; /* Write request pending */
    struct List         ncp_ReadQueue;    /* List of read requests */
    struct List         ncp_WriteQueue;   /* List of write requests */
};

#endif /* PRINTER_H */
