#ifndef AROSX_LIBRARY_H
#define AROSX_LIBRARY_H

#include <exec/types.h>

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
    UWORD   Buttons;
    BYTE    LeftTrigger;
    BYTE    RightTrigger;
    WORD    ThumbLX;
    WORD    ThumbLY;
    WORD    ThumbRX;
    WORD    ThumbRY;
};

#endif /* AROSX_LIBRARY_H */
