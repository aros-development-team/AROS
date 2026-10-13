/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: XInput gamepad protocol -> controller.hidd control table and values.
          See arosxctrl_map.h for the report layout this is based on.
*/

#include "arosxctrl_map.h"

/* ControlDesc field order: kind, index, flags, usage_page, usage, min, max, flat, fuzz, label, locality, companion, name */
#define BTN(i, lbl, nm)                 { vHidd_Controller_Ctl_Button, (i), 0, 0x09, (i) + 1, 0, 1, 0, 0, (lbl), 0, 0, (nm) }
#define AXIS(i, us, mn, mx, fl, loc, nm) { vHidd_Controller_Ctl_Axis, (i), (fl), 0x01, (us), (mn), (mx), 0, 0, 0, (loc), 0, (nm) }
#define CTL_END                         { vHidd_Controller_Ctl_End, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL }

/* OutputDesc field order: kind, index, locality, pad, caps, max, name */
#define OUT(kind, i, loc, max, nm)      { (kind), (i), (loc), 0, 0, (max), (nm) }
#define OUT_END                         { vHidd_Controller_Out_End, 0, 0, 0, 0, 0, NULL }

/* Binding field order: in_kind, in_index, hat_mask, flags, in_min, in_max, out, reserved */
#define B_BTN(std, i)     { vHidd_Controller_Ctl_Button, (i), 0, 0, 0, 0, (std), 0 }
#define B_AXIS(std, i)    { vHidd_Controller_Ctl_Axis, (i), 0, 0, HIDD_CONTROLLER_AXIS_MIN, HIDD_CONTROLLER_AXIS_MAX, (std), 0 }
#define B_TRIG(std, i)    { vHidd_Controller_Ctl_Axis, (i), 0, 0, 0, HIDD_CONTROLLER_TRIGGER_MAX, (std), 0 }
#define B_END             { 0, 0, 0, 0, 0, 0, vHidd_Controller_Std_None, 0 }
#define GP(n)   vHidd_Controller_Std_Button(vHidd_Controller_GP_##n)
#define GPA(n)  vHidd_Controller_Std_Axis(vHidd_Controller_GPA_##n)

const struct Hidd_Controller_ControlDesc arosxctrl_Controls[] =
{
    BTN(XINPUT_IDX_A,           vHidd_Controller_Label_A,         "A"),
    BTN(XINPUT_IDX_B,           vHidd_Controller_Label_B,         "B"),
    BTN(XINPUT_IDX_X,           vHidd_Controller_Label_X,         "X"),
    BTN(XINPUT_IDX_Y,           vHidd_Controller_Label_Y,         "Y"),
    BTN(XINPUT_IDX_LB,          vHidd_Controller_Label_LB,        "LB"),
    BTN(XINPUT_IDX_RB,          vHidd_Controller_Label_RB,        "RB"),
    BTN(XINPUT_IDX_BACK,        vHidd_Controller_Label_Back,      "Back"),
    BTN(XINPUT_IDX_START,       vHidd_Controller_Label_Start,     "Start"),
    BTN(XINPUT_IDX_LEFT_THUMB,  vHidd_Controller_Label_L3,        "Left Stick"),
    BTN(XINPUT_IDX_RIGHT_THUMB, vHidd_Controller_Label_R3,        "Right Stick"),
    BTN(XINPUT_IDX_GUIDE,       vHidd_Controller_Label_Guide,     "Guide"),
    BTN(XINPUT_IDX_DPAD_UP,     vHidd_Controller_Label_DpadUp,    "DPad Up"),
    BTN(XINPUT_IDX_DPAD_DOWN,   vHidd_Controller_Label_DpadDown,  "DPad Down"),
    BTN(XINPUT_IDX_DPAD_LEFT,   vHidd_Controller_Label_DpadLeft,  "DPad Left"),
    BTN(XINPUT_IDX_DPAD_RIGHT,  vHidd_Controller_Label_DpadRight, "DPad Right"),
    AXIS(XINPUT_AXIS_LX, 0x30, -32768, 32767, 0,                            vHidd_Controller_Loc_Left,         "Left X"),
    AXIS(XINPUT_AXIS_LY, 0x31, -32768, 32767, vHidd_Controller_CF_Inverted, vHidd_Controller_Loc_Left,         "Left Y"),
    AXIS(XINPUT_AXIS_RX, 0x33, -32768, 32767, 0,                            vHidd_Controller_Loc_Right,        "Right X"),
    AXIS(XINPUT_AXIS_RY, 0x34, -32768, 32767, vHidd_Controller_CF_Inverted, vHidd_Controller_Loc_Right,        "Right Y"),
    AXIS(XINPUT_AXIS_LT, 0x32, 0, 255,        vHidd_Controller_CF_Unipolar, vHidd_Controller_Loc_LeftTrigger,  "Left Trigger"),
    AXIS(XINPUT_AXIS_RT, 0x35, 0, 255,        vHidd_Controller_CF_Unipolar, vHidd_Controller_Loc_RightTrigger, "Right Trigger"),
    CTL_END
};

const struct Hidd_Controller_OutputDesc arosxctrl_Outputs[] =
{
    OUT(vHidd_Controller_Out_RumbleLow,  0, vHidd_Controller_Loc_Left,  0, "Left motor"),
    OUT(vHidd_Controller_Out_RumbleHigh, 0, vHidd_Controller_Loc_Right, 0, "Right motor"),
    OUT(vHidd_Controller_Out_LEDPlayer,  0, vHidd_Controller_Loc_Body,  4, "Ring of light"),
    OUT_END
};

const struct Hidd_Controller_Binding arosxctrl_Bindings[] =
{
    B_BTN(GP(South), XINPUT_IDX_A), B_BTN(GP(East), XINPUT_IDX_B), B_BTN(GP(West), XINPUT_IDX_X), B_BTN(GP(North), XINPUT_IDX_Y),
    B_BTN(GP(LeftShoulder), XINPUT_IDX_LB), B_BTN(GP(RightShoulder), XINPUT_IDX_RB),
    B_BTN(GP(Back), XINPUT_IDX_BACK), B_BTN(GP(Start), XINPUT_IDX_START),
    B_BTN(GP(LeftStick), XINPUT_IDX_LEFT_THUMB), B_BTN(GP(RightStick), XINPUT_IDX_RIGHT_THUMB), B_BTN(GP(Guide), XINPUT_IDX_GUIDE),
    B_BTN(GP(DpadUp), XINPUT_IDX_DPAD_UP), B_BTN(GP(DpadDown), XINPUT_IDX_DPAD_DOWN),
    B_BTN(GP(DpadLeft), XINPUT_IDX_DPAD_LEFT), B_BTN(GP(DpadRight), XINPUT_IDX_DPAD_RIGHT),
    B_AXIS(GPA(LeftX), XINPUT_AXIS_LX), B_AXIS(GPA(LeftY), XINPUT_AXIS_LY),
    B_AXIS(GPA(RightX), XINPUT_AXIS_RX), B_AXIS(GPA(RightY), XINPUT_AXIS_RY),
    B_TRIG(GPA(LeftTrigger), XINPUT_AXIS_LT), B_TRIG(GPA(RightTrigger), XINPUT_AXIS_RT),
    B_END
};

/* wire bit of each raw button index */
static const UWORD arosxctrl_Bits[XINPUT_BUTTON_COUNT] =
{
    XINPUT_BTN_A, XINPUT_BTN_B, XINPUT_BTN_X, XINPUT_BTN_Y,
    XINPUT_BTN_LB, XINPUT_BTN_RB, XINPUT_BTN_BACK, XINPUT_BTN_START,
    XINPUT_BTN_LEFT_THUMB, XINPUT_BTN_RIGHT_THUMB, XINPUT_BTN_GUIDE,
    XINPUT_BTN_DPAD_UP, XINPUT_BTN_DPAD_DOWN, XINPUT_BTN_DPAD_LEFT, XINPUT_BTN_DPAD_RIGHT
};

UWORD arosxctrl_ButtonBit(ULONG index)
{
    return index < XINPUT_BUTTON_COUNT ? arosxctrl_Bits[index] : 0;
}

BOOL arosxctrl_IsInputReport(const UBYTE *buf, ULONG len)
{
    return buf && len >= XINPUT_REPORT_MINLEN && buf[0] == XINPUT_MSG_INPUT && buf[1] == XINPUT_INPUT_LEN;
}

static LONG arosxctrl_S16(const UBYTE *p)
{
    return (LONG)(WORD)((UWORD)p[0] | ((UWORD)p[1] << 8));
}

BOOL arosxctrl_Decode(const UBYTE *buf, ULONG len, struct pHidd_Controller_RawReport *r)
{
    UWORD wire;
    ULONG i;

    if (!arosxctrl_IsInputReport(buf, len))
        return FALSE;

    wire = (UWORD)buf[2] | ((UWORD)buf[3] << 8);
    r->buttons[0] = 0;
    r->buttons[1] = 0;
    for (i = 0; i < XINPUT_BUTTON_COUNT; i++)
        if (wire & arosxctrl_Bits[i])
            r->buttons[0] |= 1UL << i;

    r->axes[XINPUT_AXIS_LT] = buf[4];
    r->axes[XINPUT_AXIS_RT] = buf[5];
    r->axes[XINPUT_AXIS_LX] = arosxctrl_S16(buf + 6);
    r->axes[XINPUT_AXIS_LY] = arosxctrl_S16(buf + 8);
    r->axes[XINPUT_AXIS_RX] = arosxctrl_S16(buf + 10);
    r->axes[XINPUT_AXIS_RY] = arosxctrl_S16(buf + 12);

    r->timestamp = 0;
    r->valid = vHidd_Controller_RR_Buttons | vHidd_Controller_RR_Axes;
    return TRUE;
}

BOOL arosxctrl_SignalLost(const UBYTE *buf, ULONG len)
{
    if (!buf || len < 15)
        return TRUE;
    return (buf[14] & (1 << 4)) ? FALSE : TRUE;
}

BOOL arosxctrl_Wireless(const UBYTE *caps, ULONG len)
{
    if (!caps || len < 19)
        return FALSE;
    return (caps[18] & (1 << 0)) ? TRUE : FALSE;
}

ULONG arosxctrl_BuildRumble(UBYTE *out, UWORD low, UWORD high)
{
    ULONG i;

    for (i = 0; i < XINPUT_RUMBLE_CMD_LEN; i++)
        out[i] = 0;
    out[1] = 0x08;
    out[3] = (UBYTE)(low >> 8);     /* left, large motor  */
    out[4] = (UBYTE)(high >> 8);    /* right, small motor */
    return XINPUT_RUMBLE_CMD_LEN;
}

ULONG arosxctrl_BuildLED(UBYTE *out, WORD player)
{
    ULONG i;

    for (i = 0; i < XINPUT_LED_CMD_LEN; i++)
        out[i] = 0;
    out[0] = 0x01;
    out[1] = 0x03;
    out[2] = (player < 0) ? XINPUT_LED_OFF : XINPUT_LED_PLAYER(player & 3);
    return XINPUT_LED_CMD_LEN;
}
