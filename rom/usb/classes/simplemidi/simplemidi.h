#ifndef SIMPLEMIDI_H
#define SIMPLEMIDI_H

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
    struct Task        *nh_GUITask;       /* GUI Task */

    struct ClsGlobalCfg nh_CurrentCGC;

    BOOL               nh_UsingDefaultCfg;
    Object             *nh_App;
    Object             *nh_MainWindow;
    Object             *nh_MidiMinOctaveObj;
    Object             *nh_KeyMaxOctaveObj;
    Object             *nh_AutoKeyUpObj;
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
    UBYTE              *nch_EPInBuf;      /* Packet buffer for EP1 */
    struct Task        *nch_ReadySigTask; /* Task to send ready signal to */
    LONG                nch_ReadySignal;  /* Signal to send when ready */
    struct Task        *nch_Task;         /* Subtask */
    struct MsgPort     *nch_TaskMsgPort;  /* Message Port of Subtask */
    struct MsgPort     *nch_InpMsgPort;   /* input.device MsgPort */
    struct IOStdReq    *nch_InpIOReq;     /* input.device IORequest */
    struct InputEvent   nch_FakeEvent;    /* Input Event */
    struct Library     *nch_InputBase;    /* Pointer to input.device base */
    IPTR                nch_IfNum;        /* Interface Number */

    UWORD               nch_LastOctave;   /* Last octave used */
};

#endif /* SIMPLEMIDI_H */
