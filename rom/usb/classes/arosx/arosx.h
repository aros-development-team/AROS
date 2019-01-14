#ifndef AROSXClass_H
#define AROSXClass_H

#include <libraries/mui.h>
#include <sys/time.h>

#include "include/arosx.h"

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

struct AROSXClassBase
{
    struct Library               nh_Library;     /* standard */

    struct Library              *nh_MUIBase;     /* MUI master base */
    struct Library              *nh_PsdBase;     /* Poseidon base */

    struct SignalSemaphore       nh_arosx_controller_lock;

    struct AROSXClassController *nh_arosx_controller_1;
    struct AROSXClassController *nh_arosx_controller_2;
    struct AROSXClassController *nh_arosx_controller_3;
    struct AROSXClassController *nh_arosx_controller_4;

    struct Library              *nh_AROSXBase;   /* AROSX base */
};

struct AROSXClassController
{
    struct Node         nch_Node;         /* Node linkage */

    UBYTE   id;
    UBYTE   name[64];

    UBYTE   controller_type;

    UBYTE   battery_type;
    UBYTE   battery_level;

    struct AROSXClassController_status {
        BOOL    connected;

        BOOL    wireless;
        BOOL    signallost;
    } status;

    union {
        struct AROSX_GAMEPAD nch_arosx_gamepad;
    };

    /*
        Clean the rest of this nch stuff away
    */
    struct Device               *nch_TimerBase;
    struct MsgPort              *nch_TimerMP;
    struct timerequest          *nch_TimerIO;

    struct AROSXClassBase  *nch_ClsBase;      /* Up linkage */
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

    STRPTR              nch_devname;

    struct PsdDescriptor *nch_pdd;
    UBYTE                *nch_xinput_desc;

    ULONG               nch_TrackingSignal;

    struct Task        *nch_GUITask;       /* GUI Task */
    Object             *nch_App;
    Object             *nch_MainWindow;
    Object             *nch_MidiMinOctaveObj;
    Object             *nch_KeyMaxOctaveObj;
    Object             *nch_AutoKeyUpObj;
    Object             *nch_UseObj;
    Object             *nch_CloseObj;

    Object             *nch_GamepadGroupObject;
    Object             *nch_GamepadObject_button_a;
    Object             *nch_GamepadObject_button_b;
    Object             *nch_GamepadObject_button_x;
    Object             *nch_GamepadObject_button_y;
    Object             *nch_GamepadObject_button_ls;
    Object             *nch_GamepadObject_button_rs;
    Object             *nch_GamepadObject_left_thumb;
    Object             *nch_GamepadObject_right_thumb;

    Object             *nch_GamepadObject_dpad_left;
    Object             *nch_GamepadObject_dpad_right;
    Object             *nch_GamepadObject_dpad_up;
    Object             *nch_GamepadObject_dpad_down;

    Object             *nch_GamepadObject_button_back;
    Object             *nch_GamepadObject_button_start;

    Object             *nch_GamepadObject_left_trigger;
    Object             *nch_GamepadObject_right_trigger;
    Object             *nch_GamepadObject_left_stick_x;
    Object             *nch_GamepadObject_left_stick_y;
    Object             *nch_GamepadObject_right_stick_x;
    Object             *nch_GamepadObject_right_stick_y;

    Object             *nch_AboutMI;
    Object             *nch_UseMI;
    Object             *nch_MUIPrefsMI;

};

#endif /* AROSXClass_H */
