#ifndef BLUETOOTH_H
#define BLUETOOTH_H

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

struct ClsDevCfg
{
    ULONG cdc_ChunkID;
    ULONG cdc_Length;
    ULONG cdc_StackAuto;
    ULONG cdc_DefaultUnit;
};

#if defined(__GNUC__)
# pragma pack()
#endif

/* Misc */

struct NepBTDevBase
{
    struct Library      np_Library;       /* standard */
    UWORD               np_Flags;         /* various flags */

    BPTR                np_SegList;       /* device seglist */
    struct NepBTBase   *np_ClsBase;       /* pointer to class base */
    struct Library     *np_UtilityBase;   /* cached utilitybase */
};

struct NepClassBT
{
    struct Unit         ncp_Unit;         /* Unit structure */
    ULONG               ncp_UnitNo;       /* Unit number */
    struct NepBTBase   *ncp_ClsBase;      /* Up linkage */
    struct NepBTDevBase *ncp_DevBase;     /* Device base */
    struct Library     *ncp_Base;         /* Poseidon base */
    struct PsdDevice   *ncp_Device;       /* Up linkage */
    struct PsdConfig   *ncp_Config;       /* Up linkage */
    struct PsdInterface *ncp_Interface;   /* Up linkage */
    struct Task        *ncp_ReadySigTask; /* Task to send ready signal to */
    LONG                ncp_ReadySignal;  /* Signal to send when ready */
    struct Task        *ncp_Task;         /* Subtask */
    struct MsgPort     *ncp_TaskMsgPort;  /* Message Port of Subtask */

    struct PsdPipe     *ncp_EPCmdPipe;    /* Endpoint 0 pipe */
    struct PsdEndpoint *ncp_EPACLOut;     /* Endpoint 1 */
    struct PsdPipe     *ncp_EPACLOutPipe; /* Endpoint 1 pipe */
    struct PsdEndpoint *ncp_EPACLIn;      /* Endpoint 2 */
    struct PsdPipe     *ncp_EPACLInPipe;  /* Endpoint 2 pipe */
    IPTR                ncp_EPACLInMaxPktSize; /* Endpoint 2 max pkt size */
    struct PsdEndpoint *ncp_EPEventInt;   /* Interrupt Endpoint */
    struct PsdPipe     *ncp_EPEventIntPipe; /* Interrupt pipe */
    IPTR                ncp_EPEventIntMaxPktSize; /* Endpoint 2 max pkt size */

    struct MsgPort     *ncp_DevMsgPort;   /* Message Port for IOParReq */
    UWORD               ncp_UnitProdID;   /* ProductID of unit */
    UWORD               ncp_UnitVendorID; /* VendorID of unit */
    UWORD               ncp_UnitCfgNum;   /* Config of unit */
    UWORD               ncp_UnitIfNum;    /* Interface number */
    UWORD               ncp_UnitAltIfNum; /* Alternate interface number */
    BOOL                ncp_DenyRequests; /* Do not accept further IO requests */
    BOOL                ncp_AbortRead;
    BOOL                ncp_AbortWrite;

    struct IOBTHCIReq  *ncp_ReadPending;  /* read IORequest pending */
    struct IOBTHCIReq  *ncp_WritePending; /* write IORequest pending */
    struct List         ncp_ReadQueue;    /* List of read requests */
    struct List         ncp_WriteQueue;   /* List of write requests */

    struct MsgPort     *ncp_EventMsgPort; /* Message Port to relay events to */
    struct MsgPort     *ncp_EventReplyPort; /* Reply port for events */
    ULONG               ncp_EventsPending; /* number of events not replied yet */
    struct BTHCIEventMsg *ncp_CurrEventMsg; /* current event message being filled */

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

    Object             *ncp_UnitObj;
    Object             *ncp_StackAutoObj;

    Object             *ncp_UseObj;
    Object             *ncp_SetDefaultObj;
    Object             *ncp_CloseObj;

    Object             *ncp_AboutMI;
    Object             *ncp_UseMI;
    Object             *ncp_SetDefaultMI;
    Object             *ncp_MUIPrefsMI;

};

struct NepBTBase
{
    struct Library      nh_Library;       /* standard */
    UWORD               nh_Flags;         /* various flags */

    struct Library     *nh_UtilityBase;   /* utility base */

    struct NepBTDevBase *nh_DevBase;      /* base of device created */
    struct List         nh_Units;         /* List of units available */
    APTR                nh_MemPool;       /* Memory Pool */

    BOOL                nh_UsingDefaultCfg;
    struct NepClassBT   nh_DummyNCP;      /* Dummy ncp for default config */
};


#endif /* RAWWRAP_H */
