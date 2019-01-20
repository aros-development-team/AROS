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

struct AROSXBase {
    struct Library          arosx_LibNode;
    struct AROSXClassBase  *arosxb;
};

struct AROSXClassBase
{
    struct Library               Library;     /* standard */

    struct Library              *MUIBase;     /* MUI master base */
    struct Library              *PsdBase;     /* Poseidon base */

    struct AROSXBase            *AROSXBase;

    ULONG                        tv_secs;
    ULONG                        tv_micro;

    UBYTE                        arosxc_count;

    struct SignalSemaphore       arosxc_lock;

    struct AROSXClassController *arosxc_1;
    struct AROSXClassController *arosxc_2;
    struct AROSXClassController *arosxc_3;
    struct AROSXClassController *arosxc_4;

    struct SignalSemaphore       event_lock;
    struct List                  event_port_list;
    struct MsgPort               event_reply_port;

};

struct AROSXClassController
{
    struct Node         Node;         /* Node linkage */

    UBYTE   id;
    UBYTE   name[64];

    UBYTE   controller_type;

    ULONG                        initial_tv_secs;
    ULONG                        initial_tv_micro;

    struct AROSXClassController_status {
        BOOL    connected;

        BOOL    wireless;
        BOOL    signallost;

        UBYTE   battery_type;
        UBYTE   battery_level;
    } status;

    union {
        struct AROSX_GAMEPAD arosx_gamepad;
    };

    struct List                  event_port_list;

    struct Device               *TimerBase;
    struct MsgPort              *TimerMP;
    struct timerequest          *TimerIO;

    struct AROSXClassBase  *arosxb;      /* Up linkage */

    struct Library     *Base;         /* Poseidon base */
    struct PsdDevice   *Device;       /* Up linkage */
    struct PsdConfig   *Config;       /* Up linkage */
    struct PsdInterface *Interface;   /* Up linkage */

    struct PsdPipe     *EP0Pipe;      /* Endpoint 0 pipe */
    UBYTE              *EP0Buf;       /* Packet buffer for EP0 */

    struct PsdEndpoint *EPIn;         /* Endpoint 1 */
    struct PsdPipe     *EPInPipe;     /* Endpoint 1 pipe */
    UBYTE              *EPInBuf;      /* Packet buffer for EP1 */

    struct PsdEndpoint *EPOut;         /* Endpoint 2 */
    struct PsdPipe     *EPOutPipe;     /* Endpoint 2 pipe */
    UBYTE              *EPOutBuf;      /* Packet buffer for EP2 */

    struct Task        *ReadySigTask; /* Task to send ready signal to */
    LONG                ReadySignal;  /* Signal to send when ready */
    struct Task        *Task;         /* Subtask */
    struct MsgPort     *TaskMsgPort;  /* Message Port of Subtask */
    struct MsgPort     *InpMsgPort;   /* input.device MsgPort */
    struct IOStdReq    *InpIOReq;     /* input.device IORequest */
    struct InputEvent   FakeEvent;    /* Input Event */
    struct Library     *InputBase;    /* Pointer to input.device base */

    IPTR                IfNum;        /* Interface Number */

    STRPTR              devname;

    struct PsdDescriptor *pdd;
    UBYTE                *xinput_desc;

    struct Task        *GUITask;       /* GUI Task */

    /*
        TODO: Make GUI task allocate these
    */

    Object             *App;
    Object             *MainWindow;
    Object             *UseObj;
    Object             *CloseObj;

    Object             *GamepadGroupObject;
    Object             *GamepadObject_button_a;
    Object             *GamepadObject_button_b;
    Object             *GamepadObject_button_x;
    Object             *GamepadObject_button_y;
    Object             *GamepadObject_button_ls;
    Object             *GamepadObject_button_rs;
    Object             *GamepadObject_left_thumb;
    Object             *GamepadObject_right_thumb;

    Object             *GamepadObject_dpad_left;
    Object             *GamepadObject_dpad_right;
    Object             *GamepadObject_dpad_up;
    Object             *GamepadObject_dpad_down;

    Object             *GamepadObject_button_back;
    Object             *GamepadObject_button_start;

    Object             *GamepadObject_left_trigger;
    Object             *GamepadObject_right_trigger;
    Object             *GamepadObject_left_stick_x;
    Object             *GamepadObject_left_stick_y;
    Object             *GamepadObject_right_stick_x;
    Object             *GamepadObject_right_stick_y;

    Object             *AboutMI;
    Object             *UseMI;
    Object             *MUIPrefsMI;

};

#endif /* AROSXClass_H */
