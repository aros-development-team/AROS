#ifndef EGALAXTOUCH_H
#define EGALAXTOUCH_H

#include <intuition/intuition.h>
#include <intuition/intuitionbase.h>
#include <libraries/mui.h>
#include <libraries/gadtools.h>

#if defined(__GNUC__)
# pragma pack(2)
#endif

#define ID_ABOUT            0x55555555
#define ID_STORE_CONFIG     0xaaaaaaaa
#define ID_DEF_CONFIG       0xaaaaaaab
#define ID_UPDATE_LIMITSMIN 0xaaaaaaac
#define ID_UPDATE_LIMITSMAX 0xaaaaaaad
#define ID_UPDATE_MIRRORROT 0xaaaaaaae

struct ClsDevCfg
{
    ULONG cdc_ChunkID;
    ULONG cdc_Length;
    ULONG cdc_MinX;
    ULONG cdc_MinY;
    ULONG cdc_MaxX;
    ULONG cdc_MaxY;
    ULONG cdc_Mirror;
    ULONG cdc_Rotate;
    ULONG cdc_RMBDelay;
    ULONG cdc_RMBTolerance;
    ULONG cdc_RMBMode;
};

#if defined(__GNUC__)
# pragma pack()
#endif

#define RMB_IDLE          0 // do nothing
#define RMB_LMBDOWN       1 // lmb is down
#define RMB_MOVEDTOOFAR   2 // finger has been moved too far
#define RMB_RMBDOWN       3 // convert LMB to RMB
#define RMB_RMBDOWN_LMBUP 4 // finger released, continue RMB
#define RMB_RMBDOWN_LAST  5 // finger pushed again, continue RMB

struct NepClassHid
{
    struct Node         nch_Node;         /* Node linkage */
    struct NepHidBase  *nch_ClsBase;      /* Up linkage */
    struct Library     *nch_Base;         /* Poseidon base */
    struct Library     *nch_HIntBase;     /* Intuition base */
    struct PsdDevice   *nch_Device;       /* Up linkage */
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
    struct IENewTablet  nch_TabletEvent;  /* Tablet Event */
    struct Library     *nch_InputBase;    /* Pointer to input.device base */
    struct Library     *nch_TimerBase;    /* Pointer to timer.deivce base */
    struct timerequest  nch_TimerIOReq;   /* TimerRequest */
    IPTR                nch_IfNum;        /* Interface number */
    ULONG               nch_TabTags[8];
    BOOL                nch_TrackDims;    /* Disable events output */
    UWORD               nch_TouchSpotX;   /* Spot where LMB was activated first */
    UWORD               nch_TouchSpotY;   /* Spot where LMB was activated first */
    struct timeval      nch_TouchTime;    /* Time when touched */
    UWORD               nch_RMBState;     /* RMB State machine activated */
    STRPTR              nch_DevIDString;  /* Device ID String */

    BOOL                nch_UsingDefaultCfg;
    struct ClsDevCfg   *nch_CDC;

    struct Library     *nch_MUIBase;       /* MUI master base */
    struct Library     *nch_PsdBase;       /* Poseidon base */
    struct Library     *nch_IntBase;       /* Intuition base */
    struct Task        *nch_GUITask;       /* GUI Task */
    struct NepClassHid *nch_GUIBinding;    /* Window of binding that's open */
    LONG                nch_TrackingSignal;

    Object             *nch_App;
    Object             *nch_MainWindow;

    Object             *nch_MinXObj;
    Object             *nch_MaxXObj;
    Object             *nch_MinYObj;
    Object             *nch_MaxYObj;
    Object             *nch_MirrorObj;
    Object             *nch_RotateObj;
    Object             *nch_TrackDimsObj;
    Object             *nch_RMBModeObj;
    Object             *nch_RMBDelayObj;
    Object             *nch_RMBToleranceObj;

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

    struct NepClassHid  nh_DummyNCH;     /* Dummy NCH for default config */
};

#endif /* EGALAXTOUCH_H */
