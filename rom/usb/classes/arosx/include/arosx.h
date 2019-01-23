#ifndef AROSX_LIBRARY_H
#define AROSX_LIBRARY_H

#include <exec/types.h>

#define AROSX_CONTROLLER_TYPE_UNKNOWN  0x00
#define AROSX_CONTROLLER_TYPE_GAMEPAD  0x01

#define AROSX_GAMEPAD_DPAD_UP          0x0001
#define AROSX_GAMEPAD_DPAD_DOWN        0x0002
#define AROSX_GAMEPAD_DPAD_LEFT        0x0004
#define AROSX_GAMEPAD_DPAD_RIGHT       0x0008
#define AROSX_GAMEPAD_START            0x0010
#define AROSX_GAMEPAD_BACK             0x0020
#define AROSX_GAMEPAD_LEFT_THUMB       0x0040
#define AROSX_GAMEPAD_RIGHT_THUMB      0x0080
#define AROSX_GAMEPAD_LEFT_SHOULDER    0x0100
#define AROSX_GAMEPAD_RIGHT_SHOULDER   0x0200
#define AROSX_GAMEPAD_A                0x1000
#define AROSX_GAMEPAD_B                0x2000
#define AROSX_GAMEPAD_X                0x4000
#define AROSX_GAMEPAD_Y                0x8000

struct AROSX_GAMEPAD {
    ULONG   Timestamp;
    UWORD   Buttons;
    UBYTE   LeftTrigger;
    UBYTE   RightTrigger;
    WORD    ThumbLX;
    WORD    ThumbLY;
    WORD    ThumbRX;
    WORD    ThumbRY;
};

#define AROSX_EHMB_CONNECT    0x00
#define AROSX_EHMB_DISCONNECT 0x01
#define AROSX_EHMF_CONNECT    (1L<<AROSX_EHMB_CONNECT)
#define AROSX_EHMF_DISCONNECT (1L<<AROSX_EHMB_DISCONNECT)

struct AROSX_EventHook {
    struct Node         eh_Node;
    struct MsgPort     *eh_MsgPort;
    ULONG               eh_MsgMask;
};

struct AROSX_EventNote {
    struct Message      en_Msg;
    ULONG               en_Event;
    APTR                en_Param1;
    APTR                en_Param2;
};

#endif /* AROSX_LIBRARY_H */
