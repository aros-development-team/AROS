#ifndef AROSXCTRL_MAP_H
#define AROSXCTRL_MAP_H

/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: XInput gamepad protocol -> controller.hidd control table and values.

    This unit depends on nothing but the controller.hidd public header, so it
    can be unit tested without Poseidon (developer/debug/test/controller/
    xinput_test.c compiles it in). arosxcontroller.c uses it to describe a pad
    and to translate its interrupt reports and output commands.

    The layout follows the XInput gamepad report as documented for the Xbox
    360 controller (Microsoft XINPUT_GAMEPAD, Linux xpad driver): a 20 byte
    input message with the button bitmask in bytes 2-3 (little endian), the
    triggers in bytes 4 and 5 (0..255) and the four stick axes as signed 16
    bit little endian values in bytes 6..13, +Y meaning up.
*/

#include <exec/types.h>
#include <hidd/controller.h>

/* input message: byte 0 type, byte 1 length */
#define XINPUT_MSG_INPUT        0x00
#define XINPUT_INPUT_LEN        0x14
#define XINPUT_REPORT_LEN       20
#define XINPUT_REPORT_MINLEN    14      /* bytes carrying buttons, triggers and sticks */

/* button bits in bytes 2-3, as seen on the wire (byte 2 | byte 3 << 8) */
#define XINPUT_BTN_DPAD_UP      0x0001
#define XINPUT_BTN_DPAD_DOWN    0x0002
#define XINPUT_BTN_DPAD_LEFT    0x0004
#define XINPUT_BTN_DPAD_RIGHT   0x0008
#define XINPUT_BTN_START        0x0010
#define XINPUT_BTN_BACK         0x0020
#define XINPUT_BTN_LEFT_THUMB   0x0040
#define XINPUT_BTN_RIGHT_THUMB  0x0080
#define XINPUT_BTN_LB           0x0100
#define XINPUT_BTN_RB           0x0200
#define XINPUT_BTN_GUIDE        0x0400
#define XINPUT_BTN_A            0x1000
#define XINPUT_BTN_B            0x2000
#define XINPUT_BTN_X            0x4000
#define XINPUT_BTN_Y            0x8000

/* raw button indices of the control table */
enum {
    XINPUT_IDX_A = 0, XINPUT_IDX_B, XINPUT_IDX_X, XINPUT_IDX_Y,
    XINPUT_IDX_LB, XINPUT_IDX_RB, XINPUT_IDX_BACK, XINPUT_IDX_START,
    XINPUT_IDX_LEFT_THUMB, XINPUT_IDX_RIGHT_THUMB, XINPUT_IDX_GUIDE,
    XINPUT_IDX_DPAD_UP, XINPUT_IDX_DPAD_DOWN, XINPUT_IDX_DPAD_LEFT, XINPUT_IDX_DPAD_RIGHT,
    XINPUT_BUTTON_COUNT
};

/* raw axis indices of the control table */
enum {
    XINPUT_AXIS_LX = 0, XINPUT_AXIS_LY, XINPUT_AXIS_RX, XINPUT_AXIS_RY,
    XINPUT_AXIS_LT, XINPUT_AXIS_RT,
    XINPUT_AXIS_COUNT
};

/* output commands on the interrupt OUT pipe */
#define XINPUT_RUMBLE_CMD_LEN   8       /* 00 08 00 <left> <right> 00 00 00 */
#define XINPUT_LED_CMD_LEN      12      /* 01 03 <pattern> padded with zeros */

/* LED patterns (Linux xpad): 0 off, 2..5 blink then hold player 1..4 */
#define XINPUT_LED_OFF          0x00
#define XINPUT_LED_PLAYER(n)    (0x02 + (n))

/* wireless capability: byte 18 bit 0 of the 20 byte vendor request 1 answer */
#define XINPUT_CAPS_LEN         20

/* the tables the driver registers with controller.hidd */
extern const struct Hidd_Controller_ControlDesc arosxctrl_Controls[];
extern const struct Hidd_Controller_OutputDesc  arosxctrl_Outputs[];
extern const struct Hidd_Controller_Binding     arosxctrl_Bindings[];

/* Wire bit of raw button index (XINPUT_IDX_*) */
UWORD arosxctrl_ButtonBit(ULONG index);

/* TRUE when buf holds a gamepad input message (type 0x00, length 0x14) */
BOOL arosxctrl_IsInputReport(const UBYTE *buf, ULONG len);

/* Fill a raw report from an input message; returns FALSE if buf is not one */
BOOL arosxctrl_Decode(const UBYTE *buf, ULONG len, struct pHidd_Controller_RawReport *r);

/* Byte 14 bit 4 clear: the (wireless) pad is asleep or out of range */
BOOL arosxctrl_SignalLost(const UBYTE *buf, ULONG len);

/* Byte 18 bit 0 of the capability answer: wireless receiver */
BOOL arosxctrl_Wireless(const UBYTE *caps, ULONG len);

/* Build output commands; both return the number of bytes to send */
ULONG arosxctrl_BuildRumble(UBYTE *out, UWORD low, UWORD high);
ULONG arosxctrl_BuildLED(UBYTE *out, WORD player);

#endif /* AROSXCTRL_MAP_H */
