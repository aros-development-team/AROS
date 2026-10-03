/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: simulated controller profiles modelled on documented hardware.

    Every profile cites the public source its layout was taken from. Values
    are not measured on real devices (none were available); when hardware
    becomes available the control tables must be compared against the real
    descriptors.
*/

#include <hidd/controller.h>
#include "virtualpad_intern.h"

/* ControlDesc field order: kind, index, flags, usage_page, usage, min, max, flat, fuzz, label, locality, companion, name */
#define BTN(i, lbl, nm)                 { vHidd_Controller_Ctl_Button, (i), 0, 0x09, (i) + 1, 0, 1, 0, 0, (lbl), 0, 0, (nm) }
#define AXIS(i, pg, us, mn, mx, fl, loc, nm) { vHidd_Controller_Ctl_Axis, (i), (fl), (pg), (us), (mn), (mx), 0, 0, 0, (loc), 0, (nm) }
#define HAT(i)                          { vHidd_Controller_Ctl_Hat, (i), vHidd_Controller_CF_Hat8, 0x01, 0x39, 0, 7, 0, 0, 0, 0, 0, "Hat" }
#define TOUCH(i, nm)                    { vHidd_Controller_Ctl_Touchpad, (i), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, (nm) }
#define SENSOR(i, us, rate, nm)         { vHidd_Controller_Ctl_Sensor, (i), 0, 0, (us), 0, (rate), 0, 0, 0, 0, 0, (nm) }
#define CTL_END                         { vHidd_Controller_Ctl_End, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL }

/* OutputDesc field order: kind, index, locality, pad, caps, max, name */
#define OUT(kind, i, loc, caps, max, nm) { (kind), (i), (loc), 0, (caps), (max), (nm) }
#define OUT_END                          { vHidd_Controller_Out_End, 0, 0, 0, 0, 0, NULL }

/* Binding field order: in_kind, in_index, hat_mask, flags, in_min, in_max, out, reserved */
#define B_BTN(std, i)     { vHidd_Controller_Ctl_Button, (i), 0, 0, 0, 0, (std), 0 }
#define B_HAT(std, i, m)  { vHidd_Controller_Ctl_Hat, (i), (m), 0, 0, 0, (std), 0 }
#define B_AXIS(std, i)    { vHidd_Controller_Ctl_Axis, (i), 0, 0, HIDD_CONTROLLER_AXIS_MIN, HIDD_CONTROLLER_AXIS_MAX, (std), 0 }
#define B_TRIG(std, i)    { vHidd_Controller_Ctl_Axis, (i), 0, 0, 0, HIDD_CONTROLLER_TRIGGER_MAX, (std), 0 }
#define B_END             { 0, 0, 0, 0, 0, 0, vHidd_Controller_Std_None, 0 }
#define GP(n)   vHidd_Controller_Std_Button(vHidd_Controller_GP_##n)
#define GPA(n)  vHidd_Controller_Std_Axis(vHidd_Controller_GPA_##n)
#define ARCH(n) vHidd_Controller_Std_Arch(vHidd_Controller_ARCH_##n)
#define DPAD_HAT(h) B_HAT(GP(DpadUp), h, vHidd_Controller_Hat_Up), B_HAT(GP(DpadRight), h, vHidd_Controller_Hat_Right), \
                    B_HAT(GP(DpadDown), h, vHidd_Controller_Hat_Down), B_HAT(GP(DpadLeft), h, vHidd_Controller_Hat_Left)

#define UNI (vHidd_Controller_CF_Unipolar)
#define LOC_L vHidd_Controller_Loc_Left
#define LOC_R vHidd_Controller_Loc_Right
#define LOC_LT vHidd_Controller_Loc_LeftTrigger
#define LOC_RT vHidd_Controller_Loc_RightTrigger

/*****************************************************************************************
    x360 - Xbox 360 wired controller (XInput)
    Source: XINPUT_GAMEPAD (Microsoft XInput reference), SDL3 src/joystick/hidapi/SDL_hidapi_xbox360.c
    (20 byte report: buttons in bytes 2-3, triggers 0..255, sticks Sint16).
*****************************************************************************************/
static const struct Hidd_Controller_ControlDesc x360_controls[] =
{
    BTN(0, vHidd_Controller_Label_A, "A"), BTN(1, vHidd_Controller_Label_B, "B"), BTN(2, vHidd_Controller_Label_X, "X"), BTN(3, vHidd_Controller_Label_Y, "Y"),
    BTN(4, vHidd_Controller_Label_LB, "LB"), BTN(5, vHidd_Controller_Label_RB, "RB"), BTN(6, vHidd_Controller_Label_Back, "Back"), BTN(7, vHidd_Controller_Label_Start, "Start"),
    BTN(8, vHidd_Controller_Label_L3, "Left Stick"), BTN(9, vHidd_Controller_Label_R3, "Right Stick"), BTN(10, vHidd_Controller_Label_Guide, "Guide"),
    BTN(11, vHidd_Controller_Label_DpadUp, "DPad Up"), BTN(12, vHidd_Controller_Label_DpadDown, "DPad Down"), BTN(13, vHidd_Controller_Label_DpadLeft, "DPad Left"), BTN(14, vHidd_Controller_Label_DpadRight, "DPad Right"),
    AXIS(0, 0x01, 0x30, -32768, 32767, 0, LOC_L, "Left X"),
    AXIS(1, 0x01, 0x31, -32768, 32767, vHidd_Controller_CF_Inverted, LOC_L, "Left Y"),   /* XInput: +Y up */
    AXIS(2, 0x01, 0x33, -32768, 32767, 0, LOC_R, "Right X"),
    AXIS(3, 0x01, 0x34, -32768, 32767, vHidd_Controller_CF_Inverted, LOC_R, "Right Y"),
    AXIS(4, 0x01, 0x32, 0, 255, UNI, LOC_LT, "Left Trigger"),
    AXIS(5, 0x01, 0x35, 0, 255, UNI, LOC_RT, "Right Trigger"),
    CTL_END
};
static const struct Hidd_Controller_OutputDesc x360_outputs[] =
{
    OUT(vHidd_Controller_Out_RumbleLow,  0, LOC_L, 0, 0, "Left motor"),
    OUT(vHidd_Controller_Out_RumbleHigh, 0, LOC_R, 0, 0, "Right motor"),
    OUT(vHidd_Controller_Out_LEDPlayer,  0, 0, 0, 4, "Ring of light"),
    OUT_END
};
/* XINPUT_GAMEPAD order: the layout an XInput driver class knows and supplies itself */
static const struct Hidd_Controller_Binding x360_bindings[] =
{
    B_BTN(GP(South), 0), B_BTN(GP(East), 1), B_BTN(GP(West), 2), B_BTN(GP(North), 3),
    B_BTN(GP(LeftShoulder), 4), B_BTN(GP(RightShoulder), 5),
    B_BTN(GP(Back), 6), B_BTN(GP(Start), 7),
    B_BTN(GP(LeftStick), 8), B_BTN(GP(RightStick), 9), B_BTN(GP(Guide), 10),
    B_BTN(GP(DpadUp), 11), B_BTN(GP(DpadDown), 12), B_BTN(GP(DpadLeft), 13), B_BTN(GP(DpadRight), 14),
    B_AXIS(GPA(LeftX), 0), B_AXIS(GPA(LeftY), 1), B_AXIS(GPA(RightX), 2), B_AXIS(GPA(RightY), 3),
    B_TRIG(GPA(LeftTrigger), 4), B_TRIG(GPA(RightTrigger), 5),
    B_END
};

/*****************************************************************************************
    xone - Xbox Wireless Controller (model 1708) over Bluetooth HID
    Source: SDL3 gamecontrollerdb entry 050000005e040000fd02000003090000 (Linux,
    Bluetooth) and the Linux xpadneo documentation of the BLE HID report: X/Y and
    Z/Rz sticks 0..65535, Brake/Accelerator 0..1023, hat 1..8, 16 buttons.
*****************************************************************************************/
static const struct Hidd_Controller_ControlDesc xone_controls[] =
{
    BTN(0, vHidd_Controller_Label_A, "A"), BTN(1, vHidd_Controller_Label_B, "B"), BTN(2, vHidd_Controller_Label_None, "unused"), BTN(3, vHidd_Controller_Label_X, "X"), BTN(4, vHidd_Controller_Label_Y, "Y"),
    BTN(5, vHidd_Controller_Label_None, "unused"), BTN(6, vHidd_Controller_Label_LB, "LB"), BTN(7, vHidd_Controller_Label_RB, "RB"), BTN(8, vHidd_Controller_Label_None, "unused"),
    BTN(9, vHidd_Controller_Label_None, "unused"), BTN(10, vHidd_Controller_Label_None, "unused"), BTN(11, vHidd_Controller_Label_Menu, "Menu"), BTN(12, vHidd_Controller_Label_Guide, "Xbox"),
    BTN(13, vHidd_Controller_Label_L3, "Left Stick"), BTN(14, vHidd_Controller_Label_R3, "Right Stick"), BTN(15, vHidd_Controller_Label_View, "View"),
    AXIS(0, 0x01, 0x30, 0, 65535, 0, LOC_L, "Left X"),
    AXIS(1, 0x01, 0x31, 0, 65535, 0, LOC_L, "Left Y"),
    AXIS(2, 0x01, 0x32, 0, 65535, 0, LOC_R, "Right X"),
    AXIS(3, 0x01, 0x35, 0, 65535, 0, LOC_R, "Right Y"),
    AXIS(4, 0x02, 0xC4, 0, 1023, UNI, LOC_RT, "Accelerator"),
    AXIS(5, 0x02, 0xC5, 0, 1023, UNI, LOC_LT, "Brake"),
    HAT(0),
    CTL_END
};
static const struct Hidd_Controller_OutputDesc xone_outputs[] =
{
    OUT(vHidd_Controller_Out_RumbleLow,          0, LOC_L,  0, 0, "Left motor"),
    OUT(vHidd_Controller_Out_RumbleHigh,         0, LOC_R,  0, 0, "Right motor"),
    OUT(vHidd_Controller_Out_RumbleLeftTrigger,  0, LOC_LT, 0, 0, "Left trigger motor"),
    OUT(vHidd_Controller_Out_RumbleRightTrigger, 0, LOC_RT, 0, 0, "Right trigger motor"),
    OUT_END
};
static const struct Hidd_Controller_Binding xone_bindings[] =
{
    B_BTN(GP(South), 0), B_BTN(GP(East), 1), B_BTN(GP(West), 3), B_BTN(GP(North), 4),
    B_BTN(GP(LeftShoulder), 6), B_BTN(GP(RightShoulder), 7),
    B_BTN(GP(Start), 11), B_BTN(GP(Guide), 12), B_BTN(GP(LeftStick), 13), B_BTN(GP(RightStick), 14), B_BTN(GP(Back), 15),
    B_AXIS(GPA(LeftX), 0), B_AXIS(GPA(LeftY), 1), B_AXIS(GPA(RightX), 2), B_AXIS(GPA(RightY), 3),
    B_TRIG(GPA(RightTrigger), 4), B_TRIG(GPA(LeftTrigger), 5),
    DPAD_HAT(0),
    B_END
};

/*****************************************************************************************
    ds4 - Sony DualShock 4 v2 (CUH-ZCT2), USB
    Source: SDL3 src/joystick/hidapi/SDL_hidapi_ps4.c PS4StatePacket_t (sticks and
    triggers 0..255, hat nibble 0..7 / 8 centred, face buttons Square, Cross,
    Circle, Triangle in HID button order), Linux hid-playstation.c (touchpad
    1920x942 two fingers, IMU).
*****************************************************************************************/
static const struct Hidd_Controller_ControlDesc ds4_controls[] =
{
    BTN(0, vHidd_Controller_Label_Square, "Square"), BTN(1, vHidd_Controller_Label_Cross, "Cross"), BTN(2, vHidd_Controller_Label_Circle, "Circle"), BTN(3, vHidd_Controller_Label_Triangle, "Triangle"),
    BTN(4, vHidd_Controller_Label_L1, "L1"), BTN(5, vHidd_Controller_Label_R1, "R1"), BTN(6, vHidd_Controller_Label_L2, "L2"), BTN(7, vHidd_Controller_Label_R2, "R2"),
    BTN(8, vHidd_Controller_Label_Share, "Share"), BTN(9, vHidd_Controller_Label_Options, "Options"), BTN(10, vHidd_Controller_Label_L3, "L3"), BTN(11, vHidd_Controller_Label_R3, "R3"),
    BTN(12, vHidd_Controller_Label_Home, "PS"), BTN(13, vHidd_Controller_Label_Touchpad, "Touchpad"),
    AXIS(0, 0x01, 0x30, 0, 255, 0, LOC_L, "Left X"),
    AXIS(1, 0x01, 0x31, 0, 255, 0, LOC_L, "Left Y"),
    AXIS(2, 0x01, 0x32, 0, 255, 0, LOC_R, "Right X"),
    AXIS(3, 0x01, 0x35, 0, 255, 0, LOC_R, "Right Y"),
    AXIS(4, 0x01, 0x33, 0, 255, UNI, LOC_LT, "L2"),
    AXIS(5, 0x01, 0x34, 0, 255, UNI, LOC_RT, "R2"),
    HAT(0),
    TOUCH(0, "Touchpad"),
    SENSOR(0, vHidd_Controller_Sensor_Accel, 250, "Accelerometer"),
    SENSOR(1, vHidd_Controller_Sensor_Gyro, 250, "Gyroscope"),
    CTL_END
};
static const struct Hidd_Controller_OutputDesc ds4_outputs[] =
{
    OUT(vHidd_Controller_Out_RumbleLow,  0, LOC_L, 0, 0, "Large motor"),
    OUT(vHidd_Controller_Out_RumbleHigh, 0, LOC_R, 0, 0, "Small motor"),
    OUT(vHidd_Controller_Out_LEDRGB,     0, 0, 0, 0, "Light bar"),
    OUT_END
};
static const struct Hidd_Controller_Binding ds4_bindings[] =
{
    B_BTN(GP(West), 0), B_BTN(GP(South), 1), B_BTN(GP(East), 2), B_BTN(GP(North), 3),
    B_BTN(GP(LeftShoulder), 4), B_BTN(GP(RightShoulder), 5),
    B_BTN(GP(Back), 8), B_BTN(GP(Start), 9), B_BTN(GP(LeftStick), 10), B_BTN(GP(RightStick), 11),
    B_BTN(GP(Guide), 12), B_BTN(GP(Touchpad), 13),
    B_AXIS(GPA(LeftX), 0), B_AXIS(GPA(LeftY), 1), B_AXIS(GPA(RightX), 2), B_AXIS(GPA(RightY), 3),
    B_TRIG(GPA(LeftTrigger), 4), B_TRIG(GPA(RightTrigger), 5),
    DPAD_HAT(0),
    B_END
};

/*****************************************************************************************
    ds5 - Sony DualSense, USB
    Source: SDL3 src/joystick/hidapi/SDL_hidapi_ps5.c PS5StatePacket_t and button
    enum (touchpad 11, microphone 12), Linux hid-playstation.c (five player LEDs,
    lightbar, adaptive triggers, touchpad 1920x1080).
*****************************************************************************************/
static const struct Hidd_Controller_ControlDesc ds5_controls[] =
{
    BTN(0, vHidd_Controller_Label_Square, "Square"), BTN(1, vHidd_Controller_Label_Cross, "Cross"), BTN(2, vHidd_Controller_Label_Circle, "Circle"), BTN(3, vHidd_Controller_Label_Triangle, "Triangle"),
    BTN(4, vHidd_Controller_Label_L1, "L1"), BTN(5, vHidd_Controller_Label_R1, "R1"), BTN(6, vHidd_Controller_Label_L2, "L2"), BTN(7, vHidd_Controller_Label_R2, "R2"),
    BTN(8, vHidd_Controller_Label_Share, "Create"), BTN(9, vHidd_Controller_Label_Options, "Options"), BTN(10, vHidd_Controller_Label_L3, "L3"), BTN(11, vHidd_Controller_Label_R3, "R3"),
    BTN(12, vHidd_Controller_Label_Home, "PS"), BTN(13, vHidd_Controller_Label_Touchpad, "Touchpad"), BTN(14, vHidd_Controller_Label_Misc, "Mute"),
    AXIS(0, 0x01, 0x30, 0, 255, 0, LOC_L, "Left X"),
    AXIS(1, 0x01, 0x31, 0, 255, 0, LOC_L, "Left Y"),
    AXIS(2, 0x01, 0x32, 0, 255, 0, LOC_R, "Right X"),
    AXIS(3, 0x01, 0x35, 0, 255, 0, LOC_R, "Right Y"),
    AXIS(4, 0x01, 0x33, 0, 255, UNI, LOC_LT, "L2"),
    AXIS(5, 0x01, 0x34, 0, 255, UNI, LOC_RT, "R2"),
    HAT(0),
    TOUCH(0, "Touchpad"),
    SENSOR(0, vHidd_Controller_Sensor_Accel, 250, "Accelerometer"),
    SENSOR(1, vHidd_Controller_Sensor_Gyro, 250, "Gyroscope"),
    CTL_END
};
static const struct Hidd_Controller_OutputDesc ds5_outputs[] =
{
    OUT(vHidd_Controller_Out_RumbleLow,     0, LOC_L, 0, 0, "Left haptic"),
    OUT(vHidd_Controller_Out_RumbleHigh,    0, LOC_R, 0, 0, "Right haptic"),
    OUT(vHidd_Controller_Out_LEDRGB,        0, 0, 0, 0, "Light bar"),
    OUT(vHidd_Controller_Out_LEDPlayer,     0, 0, 0, 5, "Player LEDs"),
    OUT(vHidd_Controller_Out_TriggerEffect, 0, LOC_LT, 0, 0, "Left adaptive trigger"),
    OUT(vHidd_Controller_Out_TriggerEffect, 1, LOC_RT, 0, 0, "Right adaptive trigger"),
    OUT(vHidd_Controller_Out_RawEffect,     0, 0, 0, 0, "Output report"),
    OUT_END
};
static const struct Hidd_Controller_Binding ds5_bindings[] =
{
    B_BTN(GP(West), 0), B_BTN(GP(South), 1), B_BTN(GP(East), 2), B_BTN(GP(North), 3),
    B_BTN(GP(LeftShoulder), 4), B_BTN(GP(RightShoulder), 5),
    B_BTN(GP(Back), 8), B_BTN(GP(Start), 9), B_BTN(GP(LeftStick), 10), B_BTN(GP(RightStick), 11),
    B_BTN(GP(Guide), 12), B_BTN(GP(Touchpad), 13), B_BTN(GP(Misc1), 14),
    B_AXIS(GPA(LeftX), 0), B_AXIS(GPA(LeftY), 1), B_AXIS(GPA(RightX), 2), B_AXIS(GPA(RightY), 3),
    B_TRIG(GPA(LeftTrigger), 4), B_TRIG(GPA(RightTrigger), 5),
    DPAD_HAT(0),
    B_END
};

/*****************************************************************************************
    switchpro - Nintendo Switch Pro Controller, Bluetooth
    Source: dekuNukem/Nintendo_Switch_Reverse_Engineering bluetooth_hid_notes.md
    (input report 0x30: button bytes Y X B A SR SL R ZR / Minus Plus RStick LStick
    Home Capture / Down Up Right Left SR SL L ZL, 12-bit sticks, three IMU samples),
    SDL3 gamecontrollerdb hint SDL_GAMECONTROLLER_USE_BUTTON_LABELS for the
    positional A/B swap.
*****************************************************************************************/
static const struct Hidd_Controller_ControlDesc switchpro_controls[] =
{
    BTN(0, vHidd_Controller_Label_Y, "Y"), BTN(1, vHidd_Controller_Label_X, "X"), BTN(2, vHidd_Controller_Label_B, "B"), BTN(3, vHidd_Controller_Label_A, "A"),
    BTN(4, vHidd_Controller_Label_None, "SR"), BTN(5, vHidd_Controller_Label_None, "SL"), BTN(6, vHidd_Controller_Label_R1, "R"), BTN(7, vHidd_Controller_Label_R2, "ZR"),
    BTN(8, vHidd_Controller_Label_Minus, "Minus"), BTN(9, vHidd_Controller_Label_Plus, "Plus"), BTN(10, vHidd_Controller_Label_R3, "Right Stick"), BTN(11, vHidd_Controller_Label_L3, "Left Stick"),
    BTN(12, vHidd_Controller_Label_Home, "Home"), BTN(13, vHidd_Controller_Label_Capture, "Capture"), BTN(14, vHidd_Controller_Label_L1, "L"), BTN(15, vHidd_Controller_Label_L2, "ZL"),
    AXIS(0, 0x01, 0x30, 0, 4095, 0, LOC_L, "Left X"),
    AXIS(1, 0x01, 0x31, 0, 4095, vHidd_Controller_CF_Inverted, LOC_L, "Left Y"),   /* Switch: up = max */
    AXIS(2, 0x01, 0x32, 0, 4095, 0, LOC_R, "Right X"),
    AXIS(3, 0x01, 0x35, 0, 4095, vHidd_Controller_CF_Inverted, LOC_R, "Right Y"),
    HAT(0),
    SENSOR(0, vHidd_Controller_Sensor_Accel, 200, "Accelerometer"),
    SENSOR(1, vHidd_Controller_Sensor_Gyro, 200, "Gyroscope"),
    CTL_END
};
static const struct Hidd_Controller_OutputDesc switchpro_outputs[] =
{
    OUT(vHidd_Controller_Out_RumbleLow,  0, LOC_L, 0, 0, "Left HD rumble"),
    OUT(vHidd_Controller_Out_RumbleHigh, 0, LOC_R, 0, 0, "Right HD rumble"),
    OUT(vHidd_Controller_Out_LEDPlayer,  0, 0, 0, 4, "Player LEDs"),
    OUT(vHidd_Controller_Out_RawEffect,  0, 0, 0, 0, "Rumble/subcommand report"),
    OUT_END
};
static const struct Hidd_Controller_Binding switchpro_bindings[] =
{
    B_BTN(GP(South), 2), B_BTN(GP(East), 3), B_BTN(GP(West), 0), B_BTN(GP(North), 1),
    B_BTN(GP(RightShoulder), 6), B_BTN(GP(LeftShoulder), 14),
    { vHidd_Controller_Ctl_Button, 15, 0, 0, 0, 0, GPA(LeftTrigger), 0 },   /* ZL is digital: button -> trigger axis */
    { vHidd_Controller_Ctl_Button, 7, 0, 0, 0, 0, GPA(RightTrigger), 0 },
    B_BTN(GP(Back), 8), B_BTN(GP(Start), 9), B_BTN(GP(RightStick), 10), B_BTN(GP(LeftStick), 11),
    B_BTN(GP(Guide), 12), B_BTN(GP(Misc1), 13),
    B_AXIS(GPA(LeftX), 0), B_AXIS(GPA(LeftY), 1), B_AXIS(GPA(RightX), 2), B_AXIS(GPA(RightY), 3),
    DPAD_HAT(0),
    B_END
};

/*****************************************************************************************
    l3dpro - Logitech Extreme 3D Pro flight stick, USB
    Source: publicly captured HID report descriptor (usb.org style dumps used by
    Linux hid-lg): X/Y 10 bit, Rz (twist) 8 bit, Slider (throttle) 8 bit, hat,
    12 buttons. Not verified on hardware.
*****************************************************************************************/
static const struct Hidd_Controller_ControlDesc l3dpro_controls[] =
{
    BTN(0, vHidd_Controller_Label_Trigger, "Trigger"), BTN(1, vHidd_Controller_Label_Thumb, "Thumb"), BTN(2, vHidd_Controller_Label_Numbered + 3, "3"), BTN(3, vHidd_Controller_Label_Numbered + 4, "4"),
    BTN(4, vHidd_Controller_Label_Numbered + 5, "5"), BTN(5, vHidd_Controller_Label_Numbered + 6, "6"), BTN(6, vHidd_Controller_Label_Numbered + 7, "7"), BTN(7, vHidd_Controller_Label_Numbered + 8, "8"),
    BTN(8, vHidd_Controller_Label_Numbered + 9, "9"), BTN(9, vHidd_Controller_Label_Numbered + 10, "10"), BTN(10, vHidd_Controller_Label_Numbered + 11, "11"), BTN(11, vHidd_Controller_Label_Numbered + 12, "12"),
    AXIS(0, 0x01, 0x30, 0, 1023, 0, LOC_R, "X"),
    AXIS(1, 0x01, 0x31, 0, 1023, 0, LOC_R, "Y"),
    AXIS(2, 0x01, 0x35, 0, 255, 0, LOC_R, "Twist"),
    AXIS(3, 0x01, 0x36, 0, 255, UNI, LOC_L, "Throttle"),
    HAT(0),
    CTL_END
};
static const struct Hidd_Controller_OutputDesc l3dpro_outputs[] = { OUT_END };

/*****************************************************************************************
    g29 - Logitech G29 Driving Force racing wheel, USB (PS3 mode)
    Source: Linux hid-lg4ff.c (G29 product id c24f, force feedback effects:
    constant, spring, damper, friction, periodic, ramp, autocenter, gain) and
    community HID descriptor dumps: wheel 16 bit, pedals 8 bit, hat, 25 buttons.
    Button order not verified on hardware.
*****************************************************************************************/
static const struct Hidd_Controller_ControlDesc g29_controls[] =
{
    BTN(0, vHidd_Controller_Label_Cross, "Cross"), BTN(1, vHidd_Controller_Label_Square, "Square"), BTN(2, vHidd_Controller_Label_Circle, "Circle"), BTN(3, vHidd_Controller_Label_Triangle, "Triangle"),
    BTN(4, vHidd_Controller_Label_R1, "R1 paddle"), BTN(5, vHidd_Controller_Label_L1, "L1 paddle"), BTN(6, vHidd_Controller_Label_R2, "R2"), BTN(7, vHidd_Controller_Label_L2, "L2"),
    BTN(8, vHidd_Controller_Label_Share, "Share"), BTN(9, vHidd_Controller_Label_Options, "Options"), BTN(10, vHidd_Controller_Label_R3, "R3"), BTN(11, vHidd_Controller_Label_L3, "L3"),
    BTN(12, vHidd_Controller_Label_Numbered + 13, "Shifter 1"), BTN(13, vHidd_Controller_Label_Numbered + 14, "Shifter 2"), BTN(14, vHidd_Controller_Label_Numbered + 15, "Shifter 3"),
    BTN(15, vHidd_Controller_Label_Numbered + 16, "Shifter 4"), BTN(16, vHidd_Controller_Label_Numbered + 17, "Shifter 5"), BTN(17, vHidd_Controller_Label_Numbered + 18, "Shifter 6"),
    BTN(18, vHidd_Controller_Label_Numbered + 19, "Shifter R"), BTN(19, vHidd_Controller_Label_Plus, "Plus"), BTN(20, vHidd_Controller_Label_Minus, "Minus"),
    BTN(21, vHidd_Controller_Label_Numbered + 22, "Dial CW"), BTN(22, vHidd_Controller_Label_Numbered + 23, "Dial CCW"), BTN(23, vHidd_Controller_Label_Numbered + 24, "Enter"), BTN(24, vHidd_Controller_Label_Home, "PS"),
    AXIS(0, 0x01, 0x30, 0, 65535, 0, 0, "Wheel"),
    AXIS(1, 0x02, 0xC4, 0, 255, UNI | vHidd_Controller_CF_Inverted, 0, "Accelerator"),
    AXIS(2, 0x02, 0xC5, 0, 255, UNI | vHidd_Controller_CF_Inverted, 0, "Brake"),
    AXIS(3, 0x02, 0xC6, 0, 255, UNI | vHidd_Controller_CF_Inverted, 0, "Clutch"),
    HAT(0),
    CTL_END
};
static const struct Hidd_Controller_OutputDesc g29_outputs[] =
{
    OUT(vHidd_Controller_Out_FFMotor, 0, 0,
        vHidd_Controller_FF_Constant | vHidd_Controller_FF_Ramp | vHidd_Controller_FF_Square | vHidd_Controller_FF_Sine |
        vHidd_Controller_FF_Triangle | vHidd_Controller_FF_SawUp | vHidd_Controller_FF_SawDown | vHidd_Controller_FF_Spring |
        vHidd_Controller_FF_Damper | vHidd_Controller_FF_Friction | vHidd_Controller_FF_Gain | vHidd_Controller_FF_Autocenter |
        (16UL << 24), 0, "Wheel motor"),
    OUT(vHidd_Controller_Out_LEDPlayer, 0, 0, 0, 5, "Rev LEDs"),
    OUT_END
};
static const struct Hidd_Controller_Binding g29_bindings[] =
{
    B_BTN(GP(South), 0), B_BTN(GP(West), 1), B_BTN(GP(East), 2), B_BTN(GP(North), 3),
    B_BTN(GP(RightShoulder), 4), B_BTN(GP(LeftShoulder), 5),
    B_BTN(GP(Back), 8), B_BTN(GP(Start), 9), B_BTN(GP(RightStick), 10), B_BTN(GP(LeftStick), 11), B_BTN(GP(Guide), 24),
    B_AXIS(ARCH(Wheel), 0), B_TRIG(ARCH(Throttle), 1), B_TRIG(ARCH(Brake), 2), B_TRIG(ARCH(Clutch), 3),
    B_AXIS(GPA(LeftX), 0), B_TRIG(GPA(RightTrigger), 1), B_TRIG(GPA(LeftTrigger), 2),
    DPAD_HAT(0),
    B_END
};

/*****************************************************************************************
    cd32 - Commodore CD32 gamepad on an Amiga joyport
    Source: lowlevel.library ReadJoyPort() documentation (JPF_BUTTON_RED/BLUE/
    GREEN/YELLOW/FORWARD/REVERSE/PLAY, JPF_JOY_UP/DOWN/LEFT/RIGHT) and the
    arch/m68k-amiga lowlevel implementation.
*****************************************************************************************/
static const struct Hidd_Controller_ControlDesc cd32_controls[] =
{
    BTN(0, vHidd_Controller_Label_Red, "Red"), BTN(1, vHidd_Controller_Label_Blue, "Blue"), BTN(2, vHidd_Controller_Label_Green, "Green"), BTN(3, vHidd_Controller_Label_Yellow, "Yellow"),
    BTN(4, vHidd_Controller_Label_Forward, "Forward"), BTN(5, vHidd_Controller_Label_Reverse, "Reverse"), BTN(6, vHidd_Controller_Label_Play, "Play"),
    { vHidd_Controller_Ctl_Hat, 0, vHidd_Controller_CF_Hat8 | vHidd_Controller_CF_HatMask, 0, 0, 0, 15, 0, 0, 0, 0, 0, "Direction" },
    CTL_END
};
static const struct Hidd_Controller_OutputDesc cd32_outputs[] = { OUT_END };
static const struct Hidd_Controller_Binding cd32_bindings[] =
{
    B_BTN(GP(South), 0), B_BTN(GP(East), 1), B_BTN(GP(West), 2), B_BTN(GP(North), 3),
    B_BTN(GP(RightShoulder), 4), B_BTN(GP(LeftShoulder), 5), B_BTN(GP(Start), 6),
    DPAD_HAT(0),
    B_END
};

/*****************************************************************************************
    generic - no-name "USB Gamepad" (DragonRise 0079:0011), USB
    Source: SDL3 gamecontrollerdb entries 03000000790000001100... (Linux): two
    axes carrying the d-pad, no hat, ten buttons. No binding table: exercises
    the synthesised mapping.
*****************************************************************************************/
static const struct Hidd_Controller_ControlDesc generic_controls[] =
{
    BTN(0, vHidd_Controller_Label_Numbered + 1, "1"), BTN(1, vHidd_Controller_Label_Numbered + 2, "2"), BTN(2, vHidd_Controller_Label_Numbered + 3, "3"), BTN(3, vHidd_Controller_Label_Numbered + 4, "4"),
    BTN(4, vHidd_Controller_Label_Numbered + 5, "5"), BTN(5, vHidd_Controller_Label_Numbered + 6, "6"), BTN(6, vHidd_Controller_Label_Numbered + 7, "7"), BTN(7, vHidd_Controller_Label_Numbered + 8, "8"),
    BTN(8, vHidd_Controller_Label_Numbered + 9, "9"), BTN(9, vHidd_Controller_Label_Numbered + 10, "10"),
    AXIS(0, 0x01, 0x30, 0, 255, 0, 0, "X"),
    AXIS(1, 0x01, 0x31, 0, 255, 0, 0, "Y"),
    CTL_END
};
static const struct Hidd_Controller_OutputDesc generic_outputs[] = { OUT_END };

/*****************************************************************************************
    overflow - pathological device exceeding the fixed maxima (70 buttons, 20 axes, 5 hats)
*****************************************************************************************/
#define BTN10(b) BTN(b+0, 0, NULL), BTN(b+1, 0, NULL), BTN(b+2, 0, NULL), BTN(b+3, 0, NULL), BTN(b+4, 0, NULL), \
                 BTN(b+5, 0, NULL), BTN(b+6, 0, NULL), BTN(b+7, 0, NULL), BTN(b+8, 0, NULL), BTN(b+9, 0, NULL)
#define AX5(a)   AXIS(a+0, 0, 0, -100, 100, 0, 0, NULL), AXIS(a+1, 0, 0, -100, 100, 0, 0, NULL), AXIS(a+2, 0, 0, -100, 100, 0, 0, NULL), \
                 AXIS(a+3, 0, 0, -100, 100, 0, 0, NULL), AXIS(a+4, 0, 0, -100, 100, 0, 0, NULL)
static const struct Hidd_Controller_ControlDesc overflow_controls[] =
{
    BTN10(0), BTN10(10), BTN10(20), BTN10(30), BTN10(40), BTN10(50), BTN10(60),
    AX5(0), AX5(5), AX5(10), AX5(15),
    HAT(0), HAT(1), HAT(2), HAT(3), HAT(4),
    CTL_END
};
static const struct Hidd_Controller_OutputDesc overflow_outputs[] = { OUT_END };

/*****************************************************************************************
    profile directory
*****************************************************************************************/

const struct VirtualPad_Profile vpad_profiles[] =
{
    { "x360", "Xbox 360 Controller (XInput)", "Xbox 360 Controller", "Microsoft", NULL,
      0x045e, 0x028e, 0x0114, vHidd_Controller_Bus_USB, vHidd_Controller_Family_Proprietary,
      vHidd_Controller_Type_Gamepad, vHidd_Controller_Conn_Wired, x360_controls, x360_outputs, x360_bindings },

    { "xone", "Xbox Wireless Controller over Bluetooth HID", "Xbox Wireless Controller", "Microsoft", "7EED8A1B2C3D",
      0x045e, 0x02fd, 0x0903, vHidd_Controller_Bus_Bluetooth, vHidd_Controller_Family_HID,
      vHidd_Controller_Type_Gamepad, vHidd_Controller_Conn_Wireless, xone_controls, xone_outputs, xone_bindings },

    { "ds4", "Sony DualShock 4 v2 (USB HID)", "Wireless Controller", "Sony Interactive Entertainment", "1c:66:6d:00:00:01",
      0x054c, 0x09cc, 0x0100, vHidd_Controller_Bus_USB, vHidd_Controller_Family_HID,
      vHidd_Controller_Type_Gamepad, vHidd_Controller_Conn_Wired, ds4_controls, ds4_outputs, ds4_bindings },

    { "ds5", "Sony DualSense (USB HID)", "DualSense Wireless Controller", "Sony Interactive Entertainment", "88:03:4c:00:00:02",
      0x054c, 0x0ce6, 0x0100, vHidd_Controller_Bus_USB, vHidd_Controller_Family_HID,
      vHidd_Controller_Type_Gamepad, vHidd_Controller_Conn_Wired, ds5_controls, ds5_outputs, ds5_bindings },

    { "switchpro", "Nintendo Switch Pro Controller (Bluetooth)", "Pro Controller", "Nintendo Co., Ltd.", "98:b6:e9:00:00:03",
      0x057e, 0x2009, 0x0100, vHidd_Controller_Bus_Bluetooth, vHidd_Controller_Family_Proprietary,
      vHidd_Controller_Type_Gamepad, vHidd_Controller_Conn_Wireless, switchpro_controls, switchpro_outputs, switchpro_bindings },

    { "l3dpro", "Logitech Extreme 3D Pro flight stick", "Logitech Extreme 3D", "Logitech", NULL,
      0x046d, 0xc215, 0x0204, vHidd_Controller_Bus_USB, vHidd_Controller_Family_HID,
      vHidd_Controller_Type_FlightStick, vHidd_Controller_Conn_Wired, l3dpro_controls, l3dpro_outputs, NULL },

    { "g29", "Logitech G29 Driving Force racing wheel", "G29 Driving Force Racing Wheel", "Logitech", NULL,
      0x046d, 0xc24f, 0x8900, vHidd_Controller_Bus_USB, vHidd_Controller_Family_HID,
      vHidd_Controller_Type_Wheel, vHidd_Controller_Conn_Wired, g29_controls, g29_outputs, g29_bindings },

    { "cd32", "Commodore CD32 gamepad on an Amiga joyport", "CD32 Gamepad", "Commodore", NULL,
      0, 0, 0, vHidd_Controller_Bus_AmigaPort, vHidd_Controller_Family_Native,
      vHidd_Controller_Type_Gamepad, vHidd_Controller_Conn_Wired, cd32_controls, cd32_outputs, cd32_bindings },

    { "generic", "No-name USB gamepad (DragonRise 0079:0011)", "USB Gamepad", NULL, NULL,
      0x0079, 0x0011, 0x0110, vHidd_Controller_Bus_USB, vHidd_Controller_Family_HID,
      vHidd_Controller_Type_Gamepad, vHidd_Controller_Conn_Wired, generic_controls, generic_outputs, NULL },

    { "overflow", "Pathological device exceeding the fixed maxima", "Overflow Test Device", "AROS", NULL,
      0, 0, 0, vHidd_Controller_Bus_Virtual, vHidd_Controller_Family_Virtual,
      vHidd_Controller_Type_Other, vHidd_Controller_Conn_Unknown, overflow_controls, overflow_outputs, NULL },

    { NULL }
};

const struct VirtualPad_Profile *vpad_FindProfile(CONST_STRPTR name)
{
    const struct VirtualPad_Profile *p;

    if (!name)
        return NULL;
    for (p = vpad_profiles; p->name; p++)
    {
        const char *a = p->name, *b = name;
        while (*a && *b && *a == *b) { a++; b++; }
        if (!*a && !*b)
            return p;
    }
    return NULL;
}
