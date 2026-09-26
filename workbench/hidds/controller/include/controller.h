#ifndef HIDD_CONTROLLER_H
#define HIDD_CONTROLLER_H

/*
    Copyright (C) 2025-2026, The AROS Development Team.
    All rights reserved.

    Desc: Public include for the game controller input subsystem (controller.hidd).
    Lang: English.
*/

#ifndef EXEC_TYPES_H
#   include <exec/types.h>
#endif
#ifndef EXEC_PORTS_H
#   include <exec/ports.h>
#endif
#ifndef OOP_OOP_H
#   include <oop/oop.h>
#endif
#ifndef PROTO_OOP_H
#   include <proto/oop.h>
#endif
#ifndef HIDD_INPUT_H
#   include <hidd/input.h>
#endif

/*
 * Class IDs. The subsystem singleton is CLID_HW_Controller; every registered
 * controller device is an object of a driver class derived from
 * CLID_Hidd_Controller. Consumers create plain CLID_Hidd_Controller objects.
 */
#define CLID_Hidd_Controller "hidd.input.controller"
#define CLID_HW_Controller   "hw.input.controller"

/* API version reported by aHW_Controller_Version */
#define HIDD_CONTROLLER_API_VERSION 2

/* Fixed maxima, so that readings are copyable in interrupt context */
#define HIDD_CONTROLLER_MAX_BUTTONS   64
#define HIDD_CONTROLLER_MAX_AXES      16
#define HIDD_CONTROLLER_MAX_HATS       4
#define HIDD_CONTROLLER_MAX_TOUCHPADS  2
#define HIDD_CONTROLLER_MAX_FINGERS    2
#define HIDD_CONTROLLER_MAX_SENSORS    4
#define HIDD_CONTROLLER_MAX_OUTPUTS   16
#define HIDD_CONTROLLER_SENSOR_RING    8

#define HIDD_CONTROLLER_AXIS_MIN   (-32768)
#define HIDD_CONTROLLER_AXIS_MAX    32767
#define HIDD_CONTROLLER_TRIGGER_MAX 32767

/*****************************************************************************************
    Identity values
*****************************************************************************************/

/* aHidd_Controller_Bus */
enum {
    vHidd_Controller_Bus_Unknown = 0,
    vHidd_Controller_Bus_USB,
    vHidd_Controller_Bus_Bluetooth,
    vHidd_Controller_Bus_AmigaPort,
    vHidd_Controller_Bus_GPIO,
    vHidd_Controller_Bus_Virtual,
    vHidd_Controller_Bus_Hosted,
    vHidd_Controller_Bus_Internal
};

/* aHidd_Controller_Type (archetype hint) */
enum {
    vHidd_Controller_Type_Unknown = 0,
    vHidd_Controller_Type_Gamepad,
    vHidd_Controller_Type_Joystick,
    vHidd_Controller_Type_Wheel,
    vHidd_Controller_Type_ArcadeStick,
    vHidd_Controller_Type_FlightStick,
    vHidd_Controller_Type_DancePad,
    vHidd_Controller_Type_Guitar,
    vHidd_Controller_Type_DrumKit,
    vHidd_Controller_Type_Throttle,
    vHidd_Controller_Type_Other
};

/* aHidd_Controller_Style (button label style) */
enum {
    vHidd_Controller_Style_Generic = 0,
    vHidd_Controller_Style_Xbox,
    vHidd_Controller_Style_PlayStation,
    vHidd_Controller_Style_Nintendo,
    vHidd_Controller_Style_Amiga
};

/* aHidd_Controller_Family (protocol family) */
enum {
    vHidd_Controller_Family_Unknown = 0,
    vHidd_Controller_Family_HID,
    vHidd_Controller_Family_XInput,
    vHidd_Controller_Family_PlayStation,
    vHidd_Controller_Family_Nintendo,
    vHidd_Controller_Family_Amiga,
    vHidd_Controller_Family_Virtual,
    vHidd_Controller_Family_Hosted
};

/* aHidd_Controller_Connection */
enum {
    vHidd_Controller_Conn_Unknown = 0,
    vHidd_Controller_Conn_Wired,
    vHidd_Controller_Conn_Wireless
};

/*****************************************************************************************
    Control and output descriptors (supplied by drivers, queried by consumers)
*****************************************************************************************/

/* Control kinds */
enum {
    vHidd_Controller_Ctl_Button = 0,
    vHidd_Controller_Ctl_Axis,
    vHidd_Controller_Ctl_Hat,
    vHidd_Controller_Ctl_Touchpad,
    vHidd_Controller_Ctl_Sensor,
    vHidd_Controller_Ctl_End = 0xFF     /* terminates a control table */
};

/* Control descriptor flags */
#define vHidd_Controller_CF_Relative    0x0001  /* axis reports deltas                     */
#define vHidd_Controller_CF_Unipolar    0x0002  /* trigger-like, normalised to 0..32767    */
#define vHidd_Controller_CF_Inverted    0x0004  /* core negates the normalised value       */
#define vHidd_Controller_CF_Analog      0x0008  /* button has a companion pressure axis    */
#define vHidd_Controller_CF_Touch       0x0010  /* capacitive touch is reported            */
#define vHidd_Controller_CF_Hat8        0x0020  /* hat is 8-way (default 4-way)            */
#define vHidd_Controller_CF_NoDeadzone  0x0040  /* do not apply flat                       */
#define vHidd_Controller_CF_HatMask     0x0080  /* driver pushes hats as direction bitmask */

/* Locality */
enum {
    vHidd_Controller_Loc_None = 0,
    vHidd_Controller_Loc_Left,
    vHidd_Controller_Loc_Right,
    vHidd_Controller_Loc_LeftTrigger,
    vHidd_Controller_Loc_RightTrigger,
    vHidd_Controller_Loc_Body
};

/* Sensor usages (ControlDesc.usage for vHidd_Controller_Ctl_Sensor) */
enum {
    vHidd_Controller_Sensor_Accel = 1,
    vHidd_Controller_Sensor_Gyro,
    vHidd_Controller_Sensor_AccelRight,
    vHidd_Controller_Sensor_GyroRight
};

struct Hidd_Controller_ControlDesc
{
    UBYTE        kind;          /* vHidd_Controller_Ctl_*                                   */
    UBYTE        index;         /* raw index within the kind, dense from 0                  */
    UWORD        flags;         /* vHidd_Controller_CF_*                                    */
    UWORD        usage_page;    /* HID usage page or 0                                      */
    UWORD        usage;         /* HID usage id, sensor usage, or 0                         */
    LONG         min;           /* logical range as reported by the driver                  */
    LONG         max;
    ULONG        flat;          /* dead zone in raw units, 0 = default (max-min)>>4         */
    ULONG        fuzz;          /* noise filter in raw units, 0 = default (max-min)>>8      */
    UWORD        label;         /* vHidd_Controller_Label_*                                 */
    UBYTE        locality;      /* vHidd_Controller_Loc_*                                   */
    UBYTE        companion;     /* CF_Analog: axis index carrying the pressure              */
    CONST_STRPTR name;          /* optional human readable name                             */
};

/* Output kinds */
enum {
    vHidd_Controller_Out_RumbleLow = 0,
    vHidd_Controller_Out_RumbleHigh,
    vHidd_Controller_Out_RumbleLeftTrigger,
    vHidd_Controller_Out_RumbleRightTrigger,
    vHidd_Controller_Out_LEDRGB,
    vHidd_Controller_Out_LEDPlayer,
    vHidd_Controller_Out_LEDMono,
    vHidd_Controller_Out_FFMotor,
    vHidd_Controller_Out_RawEffect,
    vHidd_Controller_Out_TriggerEffect,
    vHidd_Controller_Out_End = 0xFF
};

/* Effect kinds supported by an FFMotor (OutputDesc.caps) and pHidd_Controller_Effect.type */
#define vHidd_Controller_FF_Constant   (1<<0)
#define vHidd_Controller_FF_Ramp       (1<<1)
#define vHidd_Controller_FF_Square     (1<<2)
#define vHidd_Controller_FF_Sine       (1<<3)
#define vHidd_Controller_FF_Triangle   (1<<4)
#define vHidd_Controller_FF_SawUp      (1<<5)
#define vHidd_Controller_FF_SawDown    (1<<6)
#define vHidd_Controller_FF_Spring     (1<<7)
#define vHidd_Controller_FF_Damper     (1<<8)
#define vHidd_Controller_FF_Inertia    (1<<9)
#define vHidd_Controller_FF_Friction   (1<<10)
#define vHidd_Controller_FF_Custom     (1<<11)
#define vHidd_Controller_FF_Gain       (1<<12)
#define vHidd_Controller_FF_Autocenter (1<<13)
#define vHidd_Controller_FF_Rumble     (1<<14)
#define vHidd_Controller_FF_MaxEffects(caps) ((caps) >> 24)

struct Hidd_Controller_OutputDesc
{
    UBYTE        kind;          /* vHidd_Controller_Out_*                                   */
    UBYTE        index;         /* dense per kind                                           */
    UBYTE        locality;      /* vHidd_Controller_Loc_*                                   */
    UBYTE        pad;
    ULONG        caps;          /* FFMotor: vHidd_Controller_FF_* | max effects << 24       */
    LONG         max;           /* LEDPlayer: number of LEDs                                */
    CONST_STRPTR name;
};

/* aHidd_Controller_Capabilities bits */
#define vHidd_Controller_Cap_Buttons          (1<<0)
#define vHidd_Controller_Cap_Axes             (1<<1)
#define vHidd_Controller_Cap_Hats             (1<<2)
#define vHidd_Controller_Cap_Triggers         (1<<3)
#define vHidd_Controller_Cap_Touchpad         (1<<4)
#define vHidd_Controller_Cap_Sensors          (1<<5)
#define vHidd_Controller_Cap_Battery          (1<<6)
#define vHidd_Controller_Cap_Rumble           (1<<7)
#define vHidd_Controller_Cap_TriggerRumble    (1<<8)
#define vHidd_Controller_Cap_LEDRGB           (1<<9)
#define vHidd_Controller_Cap_LEDPlayer        (1<<10)
#define vHidd_Controller_Cap_LEDMono          (1<<11)
#define vHidd_Controller_Cap_Effects          (1<<12)
#define vHidd_Controller_Cap_RawEffect        (1<<13)
#define vHidd_Controller_Cap_StandardMapping  (1<<14)
#define vHidd_Controller_Cap_MappingSynthesised (1<<15)
#define vHidd_Controller_Cap_Truncated        (1<<16)

/*****************************************************************************************
    Readings and events
*****************************************************************************************/

/* Hat directions (bitmask; diagonals set two bits) */
#define vHidd_Controller_Hat_Center 0
#define vHidd_Controller_Hat_Up     (1<<0)
#define vHidd_Controller_Hat_Right  (1<<1)
#define vHidd_Controller_Hat_Down   (1<<2)
#define vHidd_Controller_Hat_Left   (1<<3)

/* Standard (positional) gamepad buttons: bit (1 << id) in gp_buttons */
enum {
    vHidd_Controller_GP_South = 0,
    vHidd_Controller_GP_East,
    vHidd_Controller_GP_West,
    vHidd_Controller_GP_North,
    vHidd_Controller_GP_Back,
    vHidd_Controller_GP_Guide,
    vHidd_Controller_GP_Start,
    vHidd_Controller_GP_LeftStick,
    vHidd_Controller_GP_RightStick,
    vHidd_Controller_GP_LeftShoulder,
    vHidd_Controller_GP_RightShoulder,
    vHidd_Controller_GP_DpadUp,
    vHidd_Controller_GP_DpadDown,
    vHidd_Controller_GP_DpadLeft,
    vHidd_Controller_GP_DpadRight,
    vHidd_Controller_GP_Misc1,
    vHidd_Controller_GP_RightPaddle1,
    vHidd_Controller_GP_LeftPaddle1,
    vHidd_Controller_GP_RightPaddle2,
    vHidd_Controller_GP_LeftPaddle2,
    vHidd_Controller_GP_Touchpad,
    vHidd_Controller_GP_Misc2,
    vHidd_Controller_GP_Misc3,
    vHidd_Controller_GP_Misc4,
    vHidd_Controller_GP_Misc5,
    vHidd_Controller_GP_Misc6,
    vHidd_Controller_GP_ButtonCount
};
#define vHidd_Controller_GPF(id) (1UL << (id))

/* Standard gamepad axes (gp_axes[]) */
enum {
    vHidd_Controller_GPA_LeftX = 0,
    vHidd_Controller_GPA_LeftY,
    vHidd_Controller_GPA_RightX,
    vHidd_Controller_GPA_RightY,
    vHidd_Controller_GPA_LeftTrigger,
    vHidd_Controller_GPA_RightTrigger,
    vHidd_Controller_GPA_AxisCount
};

/* Archetype axes (arch_axes[]) */
enum {
    vHidd_Controller_ARCH_Wheel = 0,
    vHidd_Controller_ARCH_Throttle,
    vHidd_Controller_ARCH_Brake,
    vHidd_Controller_ARCH_Clutch,
    vHidd_Controller_ARCH_Handbrake,
    vHidd_Controller_ARCH_Yaw,
    vHidd_Controller_ARCH_Pitch,
    vHidd_Controller_ARCH_Roll,
    vHidd_Controller_ARCH_AxisCount
};

/*
 * Standard element ids used by GetBinding()/GetLabel() and the std field of events:
 *   0x000 + n   standard button n (vHidd_Controller_GP_*)
 *   0x100 + n   standard axis n   (vHidd_Controller_GPA_*)
 *   0x200 + n   archetype axis n  (vHidd_Controller_ARCH_*)
 *   0xFFFF      none
 */
#define vHidd_Controller_Std_Button(n)  (0x000 + (n))
#define vHidd_Controller_Std_Axis(n)    (0x100 + (n))
#define vHidd_Controller_Std_Arch(n)    (0x200 + (n))
#define vHidd_Controller_Std_None       0xFFFF
#define vHidd_Controller_Std_IsButton(s) ((s) < 0x100)
#define vHidd_Controller_Std_IsAxis(s)   ((s) >= 0x100 && (s) < 0x200)
#define vHidd_Controller_Std_IsArch(s)   ((s) >= 0x200 && (s) < 0x300)

/* Labels (glyph hints) */
enum {
    vHidd_Controller_Label_None = 0,
    vHidd_Controller_Label_A, vHidd_Controller_Label_B, vHidd_Controller_Label_X, vHidd_Controller_Label_Y,
    vHidd_Controller_Label_Cross, vHidd_Controller_Label_Circle, vHidd_Controller_Label_Square, vHidd_Controller_Label_Triangle,
    vHidd_Controller_Label_Red, vHidd_Controller_Label_Blue, vHidd_Controller_Label_Green, vHidd_Controller_Label_Yellow,
    vHidd_Controller_Label_Play, vHidd_Controller_Label_Forward, vHidd_Controller_Label_Reverse,
    vHidd_Controller_Label_LB, vHidd_Controller_Label_RB, vHidd_Controller_Label_LT, vHidd_Controller_Label_RT,
    vHidd_Controller_Label_L1, vHidd_Controller_Label_R1, vHidd_Controller_Label_L2, vHidd_Controller_Label_R2,
    vHidd_Controller_Label_L3, vHidd_Controller_Label_R3,
    vHidd_Controller_Label_Start, vHidd_Controller_Label_Select, vHidd_Controller_Label_Back,
    vHidd_Controller_Label_Menu, vHidd_Controller_Label_View, vHidd_Controller_Label_Options,
    vHidd_Controller_Label_Share, vHidd_Controller_Label_Home, vHidd_Controller_Label_Guide,
    vHidd_Controller_Label_Plus, vHidd_Controller_Label_Minus, vHidd_Controller_Label_Capture,
    vHidd_Controller_Label_Paddle1, vHidd_Controller_Label_Paddle2, vHidd_Controller_Label_Paddle3, vHidd_Controller_Label_Paddle4,
    vHidd_Controller_Label_Fire, vHidd_Controller_Label_Thumb, vHidd_Controller_Label_Top, vHidd_Controller_Label_Pinkie,
    vHidd_Controller_Label_Trigger,
    vHidd_Controller_Label_DpadUp, vHidd_Controller_Label_DpadDown, vHidd_Controller_Label_DpadLeft, vHidd_Controller_Label_DpadRight,
    vHidd_Controller_Label_LeftStick, vHidd_Controller_Label_RightStick, vHidd_Controller_Label_Touchpad, vHidd_Controller_Label_Misc,
    vHidd_Controller_Label_Numbered = 0x100     /* label = Numbered + n */
};

/* Reading flags */
#define vHidd_Controller_RF_Mapped     0x0001   /* standard view valid                   */
#define vHidd_Controller_RF_Connected  0x0002
#define vHidd_Controller_RF_Truncated  0x0004

struct pHidd_Controller_Reading
{
    ULONG   size;                    /* caller sets sizeof(struct pHidd_Controller_Reading) */
    UWORD   device_id;
    UWORD   flags;                   /* vHidd_Controller_RF_* */
    ULONG   sequence;
    UQUAD   timestamp;               /* EClock ticks, see aHW_Controller_ClockFrequency */

    /* raw view */
    UWORD   button_count;
    UWORD   axis_count;
    UWORD   hat_count;
    UWORD   reserved;
    ULONG   buttons[2];              /* bit n = raw button n pressed */
    WORD    axes[HIDD_CONTROLLER_MAX_AXES];
    UBYTE   hats[HIDD_CONTROLLER_MAX_HATS];

    /* standard (mapped) view, valid when RF_Mapped */
    ULONG   gp_buttons;              /* vHidd_Controller_GPF(id) bits */
    WORD    gp_axes[vHidd_Controller_GPA_AxisCount];
    WORD    arch_axes[vHidd_Controller_ARCH_AxisCount];
};

/* Event types */
enum {
    vHidd_Controller_Press = 0,      /* code = raw button, std = standard button or Std_None  */
    vHidd_Controller_Release,
    vHidd_Controller_AxisMotion,     /* code = raw axis, value = normalised, std = std/arch axis */
    vHidd_Controller_HatMotion,      /* code = hat, value = direction bitmask                   */
    vHidd_Controller_Frame,          /* one per accepted report: reading is coherent now        */
    vHidd_Controller_DeviceAdded,    /* subsystem-wide consumers only                           */
    vHidd_Controller_DeviceRemoved,
    vHidd_Controller_Remapped,
    vHidd_Controller_PowerChanged,
    vHidd_Controller_TouchDown,      /* code = (pad << 8) | finger                              */
    vHidd_Controller_TouchMotion,
    vHidd_Controller_TouchUp,
    vHidd_Controller_SensorUpdate,   /* code = sensor index                                     */
    vHidd_Controller_ConnectionChanged,
    vHidd_Controller_PortChanged,    /* value = new legacy joyport or -1                        */
    vHidd_Controller_Overflow,       /* consumer queue dropped events since last drain          */
    vHidd_Controller_EventTypeCount
};
#define vHidd_Controller_EventMask(type) (1UL << (type))
#define vHidd_Controller_EventMask_All   0xFFFFFFFFUL

/* Event flags */
#define vHidd_Controller_EF_Standard  0x0001   /* std field is valid    */
#define vHidd_Controller_EF_Unipolar  0x0002

struct pHidd_Controller_Event
{
    UWORD   type;        /* vHidd_Controller_* event type */
    UWORD   device_id;
    UWORD   code;        /* raw index (or packed pad/finger, sensor index) */
    UWORD   std;         /* standard element id or vHidd_Controller_Std_None */
    UWORD   flags;       /* vHidd_Controller_EF_* */
    UWORD   reserved;
    LONG    value;       /* new value */
    ULONG   sequence;    /* device sequence after this change */
    UQUAD   timestamp;
};

/* Message delivered in port mode (aHidd_Controller_NotifyPort). Reply it with ReplyMsg(). */
struct Hidd_Controller_EventMsg
{
    struct Message               msg;
    struct pHidd_Controller_Event ev;
};

/* Which raw control feeds a standard element */
struct Hidd_Controller_Binding
{
    UBYTE   in_kind;     /* vHidd_Controller_Ctl_Button / Axis / Hat */
    UBYTE   in_index;
    UBYTE   hat_mask;    /* Ctl_Hat: direction bit                    */
    UBYTE   flags;       /* vHidd_Controller_BF_*                     */
    WORD    in_min;      /* Ctl_Axis: normalised input range mapped   */
    WORD    in_max;
    UWORD   out;         /* standard element id                       */
    UWORD   reserved;
};
#define vHidd_Controller_BF_Valid    0x01
#define vHidd_Controller_BF_Invert   0x02
#define vHidd_Controller_BF_HalfPos  0x04   /* +aN */
#define vHidd_Controller_BF_HalfNeg  0x08   /* -aN */
#define vHidd_Controller_BF_OutPos   0x10   /* output +axis */
#define vHidd_Controller_BF_OutNeg   0x20   /* output -axis */

/* Mapping sources (priority ascending) */
enum {
    vHidd_Controller_MapSrc_None = 0,
    vHidd_Controller_MapSrc_Synthesised,
    vHidd_Controller_MapSrc_BuiltIn,
    vHidd_Controller_MapSrc_Driver,
    vHidd_Controller_MapSrc_API,
    vHidd_Controller_MapSrc_User
};

/*
 * A mapping database record: which raw controls of devices with this GUID
 * feed the standard layout. Bindings are terminated by an entry with
 * out == vHidd_Controller_Std_None. Bytes 2-3 of the GUID (the name hash)
 * and 12-13 (version) are treated as wildcards when zero.
 */
struct Hidd_Controller_MappingDesc
{
    UBYTE                                guid[16];
    CONST_STRPTR                         name;       /* optional, informational */
    const struct Hidd_Controller_Binding *bindings;
};

/*****************************************************************************************
    Output, power, sensors, touch
*****************************************************************************************/

struct pHidd_Controller_Rumble
{
    UWORD   low;            /* 0..65535 */
    UWORD   high;
    UWORD   left_trigger;
    UWORD   right_trigger;
    ULONG   duration_ms;    /* 0 = until changed */
};

struct pHidd_Controller_Envelope
{
    UWORD   attack_length_ms;
    UWORD   attack_level;
    UWORD   fade_length_ms;
    UWORD   fade_level;
};

struct pHidd_Controller_Effect
{
    UWORD   type;           /* single vHidd_Controller_FF_* bit */
    UWORD   motor;          /* FFMotor index */
    UWORD   direction;      /* 0..65535 = 0..360 degrees, 0x4000 = east */
    UWORD   gain;           /* 0..65535 */
    ULONG   length_ms;
    ULONG   delay_ms;
    UWORD   trigger_button;
    UWORD   trigger_interval_ms;
    struct pHidd_Controller_Envelope envelope;
    union {
        struct { WORD level; } constant;
        struct { WORD start_level, end_level; } ramp;
        struct { UWORD period_ms; WORD magnitude, offset; UWORD phase; } periodic;
        struct { UWORD right_saturation, left_saturation; WORD right_coeff, left_coeff; UWORD deadband; WORD center; } condition[2];
        struct { UWORD strong, weak; } rumble;
    } u;
};

/* Power state */
enum {
    vHidd_Controller_Power_Unknown = 0,
    vHidd_Controller_Power_NoBattery,
    vHidd_Controller_Power_Discharging,
    vHidd_Controller_Power_Charging,
    vHidd_Controller_Power_Charged
};
enum {
    vHidd_Controller_Level_Unknown = 0,
    vHidd_Controller_Level_Empty,
    vHidd_Controller_Level_Low,
    vHidd_Controller_Level_Medium,
    vHidd_Controller_Level_Full
};

struct pHidd_Controller_PowerInfo
{
    UBYTE   state;      /* vHidd_Controller_Power_* */
    BYTE    percent;    /* -1 unknown */
    UBYTE   level;      /* vHidd_Controller_Level_* */
    UBYTE   reserved;
};

struct pHidd_Controller_SensorSample
{
    UQUAD   device_timestamp;   /* sensor clock, driver units */
    UQUAD   timestamp;          /* EClock ticks */
    LONG    v[3];               /* accel: milli-g, gyro: milli-degrees/s */
};

struct pHidd_Controller_Finger
{
    UBYTE   down;
    UBYTE   id;
    UWORD   x;          /* 0..65535 */
    UWORD   y;
    UWORD   pressure;
};

/*****************************************************************************************
    Driver side: raw report pushed with HIDD_Controller_PushReport()
*****************************************************************************************/

#define vHidd_Controller_RR_Buttons  0x0001
#define vHidd_Controller_RR_Axes     0x0002
#define vHidd_Controller_RR_Hats     0x0004
#define vHidd_Controller_RR_HatsMask 0x0008    /* hats[] already hold direction bitmasks */

struct pHidd_Controller_RawReport
{
    UQUAD   timestamp;                      /* 0 = now */
    ULONG   valid;                          /* vHidd_Controller_RR_* */
    ULONG   buttons[2];
    LONG    axes[HIDD_CONTROLLER_MAX_AXES]; /* in descriptor [min,max] units */
    UBYTE   hats[HIDD_CONTROLLER_MAX_HATS]; /* 0..7 clockwise from up, other = centred; or bitmask with CF_HatMask */
};

/* aHW_Controller_SlotPolicy */
enum {
    vHidd_Controller_SlotPolicy_Auto = 0,
    vHidd_Controller_SlotPolicy_Pinned,
    vHidd_Controller_SlotPolicy_Manual
};
#define HIDD_CONTROLLER_LEGACY_PORTS 4

/* Generated interface headers: attributes, method ids, message structs and stubs */
#include <interface/Hidd_Controller.h>
#include <interface/HW_Controller.h>

#define IS_HIDDCONTROLLER_ATTR(attr, idx) \
    IS_IF_ATTR(attr, idx, HiddControllerAB, num_Hidd_Controller_Attrs)
#define IS_HWCONTROLLER_ATTR(attr, idx) \
    IS_IF_ATTR(attr, idx, HWControllerAB, num_HW_Controller_Attrs)

#endif /* HIDD_CONTROLLER_H */
