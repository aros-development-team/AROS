#ifndef BOOTMOUSE_H
#define BOOTMOUSE_H

#include <intuition/intuition.h>
#include <intuition/intuitionbase.h>
#include <libraries/mui.h>
#include <libraries/gadtools.h>

#define ID_ABOUT        0x55555555
#define ID_STORE_CONFIG 0xaaaaaaaa
#define ID_DEF_CONFIG   0xaaaaaaab

struct ClsDevCfg
{
    ULONG cdc_ChunkID;
    ULONG cdc_Length;
    IPTR  cdc_Wheelmouse;
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
    struct PsdEndpoint *nch_EP1;          /* Endpoint 1 */
    struct PsdPipe     *nch_EP1Pipe;      /* Endpoint 1 pipe */
    IPTR                nch_EP1PktSize;   /* Size of EP1 packets */
    UBYTE              *nch_EP1Buf;       /* Packet buffer for EP1 */
    struct Task        *nch_ReadySigTask; /* Task to send ready signal to */
    LONG                nch_ReadySignal;  /* Signal to send when ready */
    struct Task        *nch_Task;         /* Subtask */
    struct MsgPort     *nch_TaskMsgPort;  /* Message Port of Subtask */
    struct MsgPort     *nch_InpMsgPort;   /* input.device MsgPort */
    struct IOStdReq    *nch_InpIOReq;     /* input.device IORequest */
    struct InputEvent   nch_FakeEvent;    /* Input Event */
    struct Library     *nch_InputBase;    /* Pointer to input.device base */
    IPTR                nch_IfNum;        /* Interface number */

    STRPTR              nch_DevIDString;  /* Device ID String */
    STRPTR              nch_IfIDString;   /* Interface ID String */

    BOOL                nch_UsingDefaultCfg;
    struct ClsDevCfg   *nch_CDC;

    struct Library     *nch_MUIBase;       /* MUI master base */
    struct Library     *nch_PsdBase;       /* Poseidon base */
    struct Library     *nch_IntBase;       /* Intuition base */
    struct Task        *nch_GUITask;       /* GUI Task */
    struct NepClassHid *nch_GUIBinding;    /* Window of binding that's open */

    Object             *nch_App;
    Object             *nch_MainWindow;
    Object             *nch_WheelmouseObj;

    Object             *nch_UseObj;
    Object             *nch_SetDefaultObj;
    Object             *nch_CloseObj;

    Object             *nch_AboutMI;
    Object             *nch_UseMI;
    Object             *nch_SetDefaultMI;
    Object             *nch_MUIPrefsMI;
};

struct NepHidBase
{
    struct Library      nh_Library;       /* standard */
    UWORD               nh_Flags;         /* various flags */

    struct Library     *nh_UtilityBase;   /* Utility base */

    struct List         nh_Bindings;      /* List of bindings created */

    struct NepClassHid  nh_DummyNCH;      /* Dummy NCH for default config */
};

#endif /* BOOTMOUSE_H */
