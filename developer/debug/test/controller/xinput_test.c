/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: xinput_test - unit test for the arosx.class XInput report decoding
          unit (rom/usb/classes/arosx/arosxctrl_map.c), which is compiled in.
          Runs without Poseidon or any USB device. The messages are the ones
          documented in arosx.class.c (captured from Logitech F310/F710 pads)
          and the XINPUT_GAMEPAD layout.

    Usage: xinput_test [SERIAL]
*/

#include <exec/types.h>
#include <dos/dos.h>
#include <exec/rawfmt.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <aros/debug.h>
#include <string.h>

#include "arosxctrl_map.c"

const char version[] = "$VER: xinput_test 1.0 (27.9.2026)";

static BOOL g_serial = FALSE;
static ULONG g_checks = 0, g_failures = 0;

static void sim_vout(CONST_STRPTR fmt, RAWARG args)
{
    VPrintf(fmt, args);
    if (g_serial)
    {
        char buf[512];
        RawDoFmt(fmt, args, RAWFMTFUNC_STRING, buf);
        kprintf("%s", buf);
    }
}
static void sim_out(CONST_STRPTR fmt, ...)
{
    AROS_SLOWSTACKFORMAT_PRE(fmt);
    sim_vout(fmt, AROS_SLOWSTACKFORMAT_ARG(fmt));
    AROS_SLOWSTACKFORMAT_POST(fmt);
}
#undef Printf
#define Printf sim_out

static void check(const char *what, LONG got, LONG expected)
{
    g_checks++;
    if (got != expected)
    {
        g_failures++;
        Printf("FAIL %s: expected %ld, got %ld\n", what, expected, got);
    }
}

/* Logitech F310 (wired), "A" pressed, sticks at rest */
static const UBYTE f310_a[20]    = { 0x00, 0x14, 0x00, 0x10, 0x00, 0x00, 0x80, 0x00, 0x80, 0x00, 0x80, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
/* Logitech F710 (wireless), idle and awake */
static const UBYTE f710_idle[20] = { 0x00, 0x14, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x80, 0x00, 0x80, 0x00, 0x80, 0x00, 0xb4, 0x00, 0x55, 0x00, 0x00, 0x00 };
/* Logitech F710 asleep */
static const UBYTE f710_sleep[20] = { 0x00, 0x14, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x80, 0x00, 0x80, 0x00, 0x80, 0x00, 0x84, 0x00, 0x00, 0x00, 0x00, 0x00 };
/* capability answers to vendor request 1 (byte 18 bit 0 = wireless) */
static const UBYTE caps_f710[20] = { 0x00, 0x14, 0xff, 0xf7, 0xff, 0xff, 0xc0, 0xff, 0xc0, 0xff, 0xc0, 0xff, 0xc0, 0xff, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00 };
static const UBYTE caps_f310[20] = { 0x00, 0x14, 0xff, 0xf7, 0xff, 0xff, 0xc0, 0xff, 0xc0, 0xff, 0xc0, 0xff, 0xc0, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

static ULONG count_controls(UBYTE kind)
{
    const struct Hidd_Controller_ControlDesc *cd;
    ULONG n = 0;

    for (cd = arosxctrl_Controls; cd->kind != vHidd_Controller_Ctl_End; cd++)
        if (cd->kind == kind)
            n++;
    return n;
}

static const struct Hidd_Controller_ControlDesc *find_control(UBYTE kind, UBYTE index)
{
    const struct Hidd_Controller_ControlDesc *cd;

    for (cd = arosxctrl_Controls; cd->kind != vHidd_Controller_Ctl_End; cd++)
        if (cd->kind == kind && cd->index == index)
            return cd;
    return NULL;
}

static const struct Hidd_Controller_Binding *find_binding(UWORD out)
{
    const struct Hidd_Controller_Binding *b;

    for (b = arosxctrl_Bindings; b->out != vHidd_Controller_Std_None; b++)
        if (b->out == out)
            return b;
    return NULL;
}

static void test_tables(void)
{
    const struct Hidd_Controller_Binding *b;
    const struct Hidd_Controller_OutputDesc *od;
    ULONG nbind = 0, nout = 0, i;
    UWORD all = 0;

    Printf("tables:\n");
    check("button controls", count_controls(vHidd_Controller_Ctl_Button), XINPUT_BUTTON_COUNT);
    check("axis controls", count_controls(vHidd_Controller_Ctl_Axis), XINPUT_AXIS_COUNT);
    check("hat controls", count_controls(vHidd_Controller_Ctl_Hat), 0);

    /* every raw button has a distinct wire bit */
    for (i = 0; i < XINPUT_BUTTON_COUNT; i++)
    {
        UWORD bit = arosxctrl_ButtonBit(i);
        check("button bit set", bit != 0, TRUE);
        check("button bit unique", (all & bit) == 0, TRUE);
        all |= bit;
    }
    check("button bit out of range", arosxctrl_ButtonBit(XINPUT_BUTTON_COUNT), 0);
    check("button bits cover the pad", all, 0xF7FF);

    /* every binding refers to a control that exists; the standard layout is complete */
    for (b = arosxctrl_Bindings; b->out != vHidd_Controller_Std_None; b++)
    {
        check("binding control exists", find_control(b->in_kind, b->in_index) != NULL, TRUE);
        nbind++;
    }
    check("binding count", nbind, XINPUT_BUTTON_COUNT + XINPUT_AXIS_COUNT);
    check("south is A", find_binding(vHidd_Controller_Std_Button(vHidd_Controller_GP_South))->in_index, XINPUT_IDX_A);
    check("north is Y", find_binding(vHidd_Controller_Std_Button(vHidd_Controller_GP_North))->in_index, XINPUT_IDX_Y);
    check("dpad up", find_binding(vHidd_Controller_Std_Button(vHidd_Controller_GP_DpadUp))->in_index, XINPUT_IDX_DPAD_UP);
    check("left trigger", find_binding(vHidd_Controller_Std_Axis(vHidd_Controller_GPA_LeftTrigger))->in_index, XINPUT_AXIS_LT);
    check("left trigger range", find_binding(vHidd_Controller_Std_Axis(vHidd_Controller_GPA_LeftTrigger))->in_max, HIDD_CONTROLLER_TRIGGER_MAX);
    check("right y", find_binding(vHidd_Controller_Std_Axis(vHidd_Controller_GPA_RightY))->in_index, XINPUT_AXIS_RY);

    /* +Y up on the wire: the Y axes are marked inverted, the triggers unipolar */
    check("LY inverted", find_control(vHidd_Controller_Ctl_Axis, XINPUT_AXIS_LY)->flags & vHidd_Controller_CF_Inverted, vHidd_Controller_CF_Inverted);
    check("RY inverted", find_control(vHidd_Controller_Ctl_Axis, XINPUT_AXIS_RY)->flags & vHidd_Controller_CF_Inverted, vHidd_Controller_CF_Inverted);
    check("LX not inverted", find_control(vHidd_Controller_Ctl_Axis, XINPUT_AXIS_LX)->flags & vHidd_Controller_CF_Inverted, 0);
    check("LT unipolar", find_control(vHidd_Controller_Ctl_Axis, XINPUT_AXIS_LT)->flags & vHidd_Controller_CF_Unipolar, vHidd_Controller_CF_Unipolar);
    check("LT max", find_control(vHidd_Controller_Ctl_Axis, XINPUT_AXIS_LT)->max, 255);
    check("A label", find_control(vHidd_Controller_Ctl_Button, XINPUT_IDX_A)->label, vHidd_Controller_Label_A);
    check("Guide label", find_control(vHidd_Controller_Ctl_Button, XINPUT_IDX_GUIDE)->label, vHidd_Controller_Label_Guide);

    for (od = arosxctrl_Outputs; od->kind != vHidd_Controller_Out_End; od++)
        nout++;
    check("output count", nout, 3);
    check("output 0 low rumble", arosxctrl_Outputs[0].kind, vHidd_Controller_Out_RumbleLow);
    check("output 1 high rumble", arosxctrl_Outputs[1].kind, vHidd_Controller_Out_RumbleHigh);
    check("output 2 player LED", arosxctrl_Outputs[2].kind, vHidd_Controller_Out_LEDPlayer);
    check("player LED count", arosxctrl_Outputs[2].max, 4);
}

static void test_decode(void)
{
    struct pHidd_Controller_RawReport r;
    UBYTE msg[20];
    ULONG i;

    Printf("decode:\n");
    memset(&r, 0, sizeof(r));
    check("f310 A accepted", arosxctrl_Decode(f310_a, 20, &r), TRUE);
    check("f310 A button", r.buttons[0], 1UL << XINPUT_IDX_A);
    check("f310 A upper buttons", r.buttons[1], 0);
    check("f310 LX", r.axes[XINPUT_AXIS_LX], 0x0080);
    check("f310 LY", r.axes[XINPUT_AXIS_LY], 0x0080);
    check("f310 LT", r.axes[XINPUT_AXIS_LT], 0);
    check("f310 valid", r.valid, vHidd_Controller_RR_Buttons | vHidd_Controller_RR_Axes);
    check("f310 signal", arosxctrl_SignalLost(f310_a, 20), TRUE);   /* wired pads never set bit 4 */

    check("f710 idle accepted", arosxctrl_Decode(f710_idle, 20, &r), TRUE);
    check("f710 idle buttons", r.buttons[0], 0);
    check("f710 awake", arosxctrl_SignalLost(f710_idle, 20), FALSE);
    check("f710 asleep", arosxctrl_SignalLost(f710_sleep, 20), TRUE);

    check("f710 wireless", arosxctrl_Wireless(caps_f710, 20), TRUE);
    check("f310 wired", arosxctrl_Wireless(caps_f310, 20), FALSE);
    check("short caps", arosxctrl_Wireless(caps_f710, 10), FALSE);

    /* each wire bit lands on its raw index */
    for (i = 0; i < XINPUT_BUTTON_COUNT; i++)
    {
        UWORD bit = arosxctrl_ButtonBit(i);

        memset(msg, 0, sizeof(msg));
        msg[1] = XINPUT_INPUT_LEN;
        msg[2] = bit & 0xFF;
        msg[3] = bit >> 8;
        check("bit decode accepted", arosxctrl_Decode(msg, 20, &r), TRUE);
        check("bit decode index", r.buttons[0], 1UL << i);
    }
    memset(msg, 0, sizeof(msg));
    msg[1] = XINPUT_INPUT_LEN;
    msg[2] = 0xFF;
    msg[3] = 0xFF;
    check("all buttons", arosxctrl_Decode(msg, 20, &r), TRUE);
    check("all buttons mask", r.buttons[0], (1UL << XINPUT_BUTTON_COUNT) - 1);

    /* documented bit assignments */
    memset(msg, 0, sizeof(msg)); msg[1] = XINPUT_INPUT_LEN; msg[2] = XINPUT_BTN_START;
    arosxctrl_Decode(msg, 20, &r); check("start", r.buttons[0], 1UL << XINPUT_IDX_START);
    memset(msg, 0, sizeof(msg)); msg[1] = XINPUT_INPUT_LEN; msg[2] = XINPUT_BTN_DPAD_RIGHT;
    arosxctrl_Decode(msg, 20, &r); check("dpad right", r.buttons[0], 1UL << XINPUT_IDX_DPAD_RIGHT);
    memset(msg, 0, sizeof(msg)); msg[1] = XINPUT_INPUT_LEN; msg[3] = XINPUT_BTN_GUIDE >> 8;
    arosxctrl_Decode(msg, 20, &r); check("guide", r.buttons[0], 1UL << XINPUT_IDX_GUIDE);
    memset(msg, 0, sizeof(msg)); msg[1] = XINPUT_INPUT_LEN; msg[3] = XINPUT_BTN_Y >> 8;
    arosxctrl_Decode(msg, 20, &r); check("y", r.buttons[0], 1UL << XINPUT_IDX_Y);

    /* signed little endian sticks, unsigned triggers */
    memset(msg, 0, sizeof(msg));
    msg[1] = XINPUT_INPUT_LEN;
    msg[4] = 0xFF;                  /* LT */
    msg[5] = 0x80;                  /* RT */
    msg[6] = 0x00; msg[7] = 0x80;   /* LX -32768 */
    msg[8] = 0xFF; msg[9] = 0x7F;   /* LY  32767 */
    msg[10] = 0xFF; msg[11] = 0xFF; /* RX -1 */
    msg[12] = 0x01; msg[13] = 0x00; /* RY  1 */
    check("axes accepted", arosxctrl_Decode(msg, 20, &r), TRUE);
    check("LT 255", r.axes[XINPUT_AXIS_LT], 255);
    check("RT 128", r.axes[XINPUT_AXIS_RT], 128);
    check("LX min", r.axes[XINPUT_AXIS_LX], -32768);
    check("LY max", r.axes[XINPUT_AXIS_LY], 32767);
    check("RX -1", r.axes[XINPUT_AXIS_RX], -1);
    check("RY 1", r.axes[XINPUT_AXIS_RY], 1);

    /* rejected messages */
    check("short message", arosxctrl_Decode(f310_a, 13, &r), FALSE);
    check("minimum length", arosxctrl_Decode(f310_a, XINPUT_REPORT_MINLEN, &r), TRUE);
    memcpy(msg, f310_a, 20); msg[0] = 0x01; msg[1] = 0x03; msg[2] = 0x02;   /* LED status */
    check("led status rejected", arosxctrl_IsInputReport(msg, 20), FALSE);
    check("led status not decoded", arosxctrl_Decode(msg, 20, &r), FALSE);
    memcpy(msg, f310_a, 20); msg[1] = 0x13;
    check("wrong length rejected", arosxctrl_IsInputReport(msg, 20), FALSE);
    check("null buffer", arosxctrl_IsInputReport(NULL, 20), FALSE);
}

static void test_commands(void)
{
    UBYTE out[XINPUT_LED_CMD_LEN];
    ULONG n;

    Printf("commands:\n");
    memset(out, 0xAA, sizeof(out));
    n = arosxctrl_BuildRumble(out, 0xFFFF, 0x8000);
    check("rumble length", n, XINPUT_RUMBLE_CMD_LEN);
    check("rumble byte 0", out[0], 0x00);
    check("rumble byte 1", out[1], 0x08);
    check("rumble byte 2", out[2], 0x00);
    check("rumble left", out[3], 0xFF);
    check("rumble right", out[4], 0x80);
    check("rumble byte 5", out[5], 0x00);
    check("rumble byte 7", out[7], 0x00);
    n = arosxctrl_BuildRumble(out, 0, 0);
    check("rumble off left", out[3], 0);
    check("rumble off right", out[4], 0);

    n = arosxctrl_BuildLED(out, 0);
    check("led length", n, XINPUT_LED_CMD_LEN);
    check("led byte 0", out[0], 0x01);
    check("led byte 1", out[1], 0x03);
    check("led player 1", out[2], 0x02);
    check("led padding", out[11], 0x00);
    arosxctrl_BuildLED(out, 3);
    check("led player 4", out[2], 0x05);
    arosxctrl_BuildLED(out, -1);
    check("led off", out[2], 0x00);
    arosxctrl_BuildLED(out, 4);
    check("led player 5 wraps", out[2], 0x02);
}

int main(void)
{
    IPTR args[1] = { 0 };
    struct RDArgs *rda = ReadArgs("SERIAL/S", args, NULL);

    g_serial = args[0] ? TRUE : FALSE;

    test_tables();
    test_decode();
    test_commands();

    Printf("xinput_test: %lu check(s), %lu failure(s)\n", g_checks, g_failures);
    if (rda)
        FreeArgs(rda);
    return g_failures ? RETURN_ERROR : RETURN_OK;
}
