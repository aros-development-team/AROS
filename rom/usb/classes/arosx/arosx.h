#ifndef AROSX_H
#define AROSX_H

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
    ULONG cgc_MidiMinOctave;
    ULONG cgc_KeyMaxOctave;
    ULONG cgc_AutoKeyUp;
};

struct NepHidBase
{
    struct Library      nh_Library;       /* standard */
    UWORD               nh_Flags;         /* various flags */
    struct Library     *nh_UtilityBase;   /* utility base */

    struct Library     *nh_MUIBase;       /* MUI master base */
    struct Library     *nh_PsdBase;       /* Poseidon base */
    struct Library     *nh_IntBase;       /* Intuition base */
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
    UBYTE              *nch_EP0Buf;       /* Packet buffer for EP0 */

    struct PsdEndpoint *nch_EPIn;         /* Endpoint 1 */
    struct PsdPipe     *nch_EPInPipe;     /* Endpoint 1 pipe */
    UBYTE              *nch_EPInBuf;      /* Packet buffer for EP1 */

    struct PsdEndpoint *nch_EPOut;         /* Endpoint 2 */
    struct PsdPipe     *nch_EPOutPipe;     /* Endpoint 2 pipe */
    UBYTE              *nch_EPOutBuf;      /* Packet buffer for EP2 */

    struct Task        *nch_ReadySigTask; /* Task to send ready signal to */
    LONG                nch_ReadySignal;  /* Signal to send when ready */
    struct Task        *nch_Task;         /* Subtask */
    struct MsgPort     *nch_TaskMsgPort;  /* Message Port of Subtask */
    struct MsgPort     *nch_InpMsgPort;   /* input.device MsgPort */
    struct IOStdReq    *nch_InpIOReq;     /* input.device IORequest */
    struct InputEvent   nch_FakeEvent;    /* Input Event */
    struct Library     *nch_InputBase;    /* Pointer to input.device base */

    IPTR                nch_IfNum;        /* Interface Number */

    ULONG               nch_TrackingSignal;

    struct Task        *nch_GUITask;       /* GUI Task */
    Object             *nch_App;
    Object             *nch_MainWindow;
    Object             *nch_MidiMinOctaveObj;
    Object             *nch_KeyMaxOctaveObj;
    Object             *nch_AutoKeyUpObj;
    Object             *nch_UseObj;
    Object             *nch_CloseObj;

    Object             *nch_GaugeGroupObject;
    Object             *nch_GaugeObject_stick_lx;
    Object             *nch_GaugeObject_stick_ly;
    Object             *nch_GaugeObject_stick_rx;
    Object             *nch_GaugeObject_stick_ry;

    Object             *nch_AboutMI;
    Object             *nch_UseMI;
    Object             *nch_MUIPrefsMI;

    STRPTR              nch_devname;

    struct PsdDescriptor *nch_pdd;

    UWORD stick_lx;
    UWORD stick_ly;
    UWORD stick_rx;
    UWORD stick_ry;

    BOOL  signallost;

};

#endif /* AROSX_H */
