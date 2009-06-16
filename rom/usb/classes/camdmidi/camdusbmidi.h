#ifndef CAMDUSBMIDI_H
#define CAMDUSBMIDI_H

#include <intuition/intuition.h>
#include <intuition/intuitionbase.h>
#include <libraries/mui.h>
#include <libraries/gadtools.h>

#define ID_ABOUT        0x55555555
#define ID_STORE_CONFIG 0xaaaaaaaa

struct ClsGlobalCfg
{
    ULONG cgc_ChunkID;
    ULONG cgc_Length;
};

struct CAMDAdapter
{
    APTR                ca_ActivateFunc;  /* activate function -- filled by CAMD Driver */
    BOOL                ca_IsOpen;        /* TRUE if opened */
    struct Hook         ca_CAMDRXFunc;    /* RX function in CAMD driver */
    struct Interrupt    ca_CAMDTXFunc;    /* TX function in CAMD driver (internal) */
    ULONG               ca_PortNum;       /* Port Number -- filled by OpenPort() */
    APTR                ca_TXFunc;        /* TX Function -- filled by OpenPort() */
    APTR                ca_RXFunc;        /* RX Function -- filled by OpenPort() */
    APTR                ca_UserData;      /* Handler user data -- filled by OpenPort() */
    UBYTE              *ca_TXBuffer;      /* Ringbuffer to send data (to USB) */
    ULONG               ca_TXBufSize;     /* Size of ringbuffer */
    ULONG               ca_TXWritePos;    /* Writing position */
    ULONG               ca_TXReadPos;     /* Bytes already sent */
    struct MsgPort     *ca_MsgPort;       /* Pointer to MsgPort to signal task for TX data ready */
    BOOL                ca_SysExMode;     /* is in SysEx sending mode */
    ULONG               ca_SysExData;     /* three bytes of sysex information */
    UWORD               ca_SysExNum;      /* number of sysex bytes */
};

struct NepHidBase
{
    struct Library      nh_Library;       /* standard */
    UWORD               nh_Flags;         /* various flags */
    struct Library     *nh_UtilityBase;   /* utility base */
    struct List         nh_Bindings;      /* List of bindings created */

    struct Library     *nh_MUIBase;       /* MUI master base */
    struct Library     *nh_PsdBase;       /* Poseidon base */
    struct Library     *nh_IntBase;       /* Intuition base */
    struct Task        *nh_GUITask;       /* GUI Task */

    struct ClsGlobalCfg nh_CurrentCGC;
    BOOL                nh_UsingDefaultCfg;

    struct CAMDAdapter  nh_CAMDAdapters[16];

    Object             *nh_App;
    Object             *nh_MainWindow;
    Object             *nh_UseObj;
    Object             *nh_CloseObj;

    Object             *nh_AboutMI;
    Object             *nh_UseMI;
    Object             *nh_MUIPrefsMI;
};

struct NepClassHid
{
    struct Node         nch_Node;         /* Node linkage */
    struct NepHidBase  *nch_ClsBase;      /* Up linkage */
    struct Library     *nch_Base;         /* Poseidon base */
    struct PsdDevice   *nch_Device;       /* Up linkage */
    struct PsdConfig   *nch_Config;       /* Up linkage */
    struct PsdInterface *nch_Interface;   /* Up linkage */
    struct PsdPipe     *nch_EP0Pipe;      /* Endpoint 0 pipe */
    struct PsdEndpoint *nch_EPIn;         /* Endpoint 1 */
    struct PsdPipe     *nch_EPInPipe;     /* Endpoint 1 pipe */
    struct PsdEndpoint *nch_EPOut;        /* Endpoint 2 */
    struct PsdPipe     *nch_EPOutPipe;    /* Endpoint 2 pipe */
    UBYTE              *nch_EPInBuf;      /* Packet buffer for EP1 */
    UBYTE              *nch_EPOutBuf;     /* Packet buffer for EP2 */
    struct Task        *nch_ReadySigTask; /* Task to send ready signal to */
    LONG                nch_ReadySignal;  /* Signal to send when ready */
    struct Task        *nch_Task;         /* Subtask */
    struct MsgPort     *nch_TaskMsgPort;  /* Message Port of Subtask */
    IPTR                nch_IfNum;        /* Interface Number */
    UBYTE               nch_ShortID[32];  /* generated ID string for driver */
};

#endif /* CAMDUSBMIDI_H */
