/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: hidmap_test - unit test for the hid.class HID-to-controller mapping
          unit (rom/usb/classes/hid/hidctrl_map.c), which is compiled in.
          Runs without Poseidon or any USB device: the parsed item tables are
          built by hand from documented report layouts.

    Usage: hidmap_test [SERIAL]
*/

#include <exec/types.h>
#include <dos/dos.h>
#include <exec/rawfmt.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <aros/debug.h>
#include <string.h>

#include "hidctrl_map.c"

const char version[] = "$VER: hidmap_test 1.0 (27.9.2026)";

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

/* helpers to build item tables */
#define IN(usage, off, size, mn, mx, root) \
    { (usage), RPF_MAIN_VARIABLE, REPORT_MAIN_INPUT, (off), (size), 1, (mn), (mx), ((mn) < 0), (root), NULL }
#define INID(id, usage, off, size, mn, mx, root) \
    { (usage), RPF_MAIN_VARIABLE, REPORT_MAIN_INPUT, (off), (size), (id), (mn), (mx), ((mn) < 0), (root), NULL }
#define BTN(n, off, root) IN(0x090000 + (n), (off), 1, 0, 1, root)

/*
 * DualShock 4, USB input report 1 (layout as documented by the public HID
 * descriptor captures used by SDL's hidapi driver and Linux hid-playstation):
 * X Y Z Rz 8 bit, hat 4 bit, 14 buttons, 6 bit counter, Rx Ry 8 bit triggers.
 */
static const struct HidCtrlItem ds4_items[] =
{
    INID(1, 0x010030,  0, 8, 0, 255, HIDCTRL_COLL_GAMEPAD),
    INID(1, 0x010031,  8, 8, 0, 255, HIDCTRL_COLL_GAMEPAD),
    INID(1, 0x010032, 16, 8, 0, 255, HIDCTRL_COLL_GAMEPAD),
    INID(1, 0x010035, 24, 8, 0, 255, HIDCTRL_COLL_GAMEPAD),
    INID(1, 0x010039, 32, 4, 0, 7,   HIDCTRL_COLL_GAMEPAD),
    INID(1, 0x090001, 36, 1, 0, 1, HIDCTRL_COLL_GAMEPAD), INID(1, 0x090002, 37, 1, 0, 1, HIDCTRL_COLL_GAMEPAD),
    INID(1, 0x090003, 38, 1, 0, 1, HIDCTRL_COLL_GAMEPAD), INID(1, 0x090004, 39, 1, 0, 1, HIDCTRL_COLL_GAMEPAD),
    INID(1, 0x090005, 40, 1, 0, 1, HIDCTRL_COLL_GAMEPAD), INID(1, 0x090006, 41, 1, 0, 1, HIDCTRL_COLL_GAMEPAD),
    INID(1, 0x090007, 42, 1, 0, 1, HIDCTRL_COLL_GAMEPAD), INID(1, 0x090008, 43, 1, 0, 1, HIDCTRL_COLL_GAMEPAD),
    INID(1, 0x090009, 44, 1, 0, 1, HIDCTRL_COLL_GAMEPAD), INID(1, 0x09000A, 45, 1, 0, 1, HIDCTRL_COLL_GAMEPAD),
    INID(1, 0x09000B, 46, 1, 0, 1, HIDCTRL_COLL_GAMEPAD), INID(1, 0x09000C, 47, 1, 0, 1, HIDCTRL_COLL_GAMEPAD),
    INID(1, 0x09000D, 48, 1, 0, 1, HIDCTRL_COLL_GAMEPAD), INID(1, 0x09000E, 49, 1, 0, 1, HIDCTRL_COLL_GAMEPAD),
    INID(1, 0x010020, 50, 6, 0, 63,  HIDCTRL_COLL_GAMEPAD),   /* vendor counter, not a control */
    INID(1, 0x010033, 56, 8, 0, 255, HIDCTRL_COLL_GAMEPAD),
    INID(1, 0x010034, 64, 8, 0, 255, HIDCTRL_COLL_GAMEPAD),
};

static void test_ds4(void)
{
    struct HidCtrlTable *t = AllocVec(sizeof(struct HidCtrlTable), MEMF_CLEAR);
    struct pHidd_Controller_RawReport r;
    /* X centre, Y centre, Z full, Rz zero, hat right + Cross, R1, Rx 0x40, Ry 0xC0 */
    static const UBYTE report[] = { 0x80, 0x80, 0xFF, 0x00, 0x22, 0x01, 0x00, 0x40, 0xC0 };
    ULONG n;

    Printf("ds4:\n");
    n = hidctrl_Build(ds4_items, sizeof(ds4_items) / sizeof(ds4_items[0]), t);
    check("ds4 mapped controls", n, 21);
    check("ds4 root", t->hct_RootUsage, HIDCTRL_COLL_GAMEPAD);
    check("ds4 buttons", t->hct_Buttons, 14);
    check("ds4 axes", t->hct_Axes, 6);
    check("ds4 hats", t->hct_Hats, 1);
    check("ds4 table entries", t->hct_ControlCount, 21);
    check("ds4 axis 4 usage", t->hct_Controls[19].usage, 0x33);
    check("ds4 hat usage", t->hct_Controls[4].usage, 0x39);
    check("ds4 hat max", t->hct_Controls[4].max, 7);

    memset(&r, 0, sizeof(r));
    check("ds4 decode", hidctrl_Decode(t, ds4_items, 1, report, sizeof(report), &r), TRUE);
    check("ds4 X", r.axes[0], 0x80);
    check("ds4 Y", r.axes[1], 0x80);
    check("ds4 Z", r.axes[2], 0xFF);
    check("ds4 Rz", r.axes[3], 0);
    check("ds4 Rx", r.axes[4], 0x40);
    check("ds4 Ry", r.axes[5], 0xC0);
    check("ds4 hat", r.hats[0], 2);
    check("ds4 buttons", r.buttons[0], (1 << 1) | (1 << 4));
    check("ds4 valid", r.valid, vHidd_Controller_RR_Buttons | vHidd_Controller_RR_Axes | vHidd_Controller_RR_Hats);
    check("ds4 other report", hidctrl_Decode(t, ds4_items, 2, report, sizeof(report), &r), FALSE);

    /* hat null state (8) must reach the raw report unchanged */
    {
        static const UBYTE centred[] = { 0x80, 0x80, 0x80, 0x80, 0x08, 0x00, 0x00, 0x00, 0x00 };
        hidctrl_Decode(t, ds4_items, 1, centred, sizeof(centred), &r);
        check("ds4 hat null", r.hats[0], 8);
        check("ds4 buttons released", r.buttons[0], 0);
    }
    FreeVec(t);
}

/* No-name pad: X Y 8 bit, 10 buttons, no hat */
static const struct HidCtrlItem generic_items[] =
{
    IN(0x010030, 0, 8, 0, 255, HIDCTRL_COLL_JOYSTICK),
    IN(0x010031, 8, 8, 0, 255, HIDCTRL_COLL_JOYSTICK),
    BTN(1, 16, HIDCTRL_COLL_JOYSTICK), BTN(2, 17, HIDCTRL_COLL_JOYSTICK), BTN(3, 18, HIDCTRL_COLL_JOYSTICK),
    BTN(4, 19, HIDCTRL_COLL_JOYSTICK), BTN(5, 20, HIDCTRL_COLL_JOYSTICK), BTN(6, 21, HIDCTRL_COLL_JOYSTICK),
    BTN(7, 22, HIDCTRL_COLL_JOYSTICK), BTN(8, 23, HIDCTRL_COLL_JOYSTICK), BTN(9, 24, HIDCTRL_COLL_JOYSTICK),
    BTN(10, 25, HIDCTRL_COLL_JOYSTICK),
};

static void test_generic(void)
{
    struct HidCtrlTable *t = AllocVec(sizeof(struct HidCtrlTable), MEMF_CLEAR);
    struct pHidd_Controller_RawReport r;
    static const UBYTE report[] = { 0x7F, 0x7F, 0xFF, 0x03 };

    Printf("generic:\n");
    check("generic mapped", hidctrl_Build(generic_items, sizeof(generic_items) / sizeof(generic_items[0]), t), 12);
    check("generic root", t->hct_RootUsage, HIDCTRL_COLL_JOYSTICK);
    check("generic buttons", t->hct_Buttons, 10);
    check("generic axes", t->hct_Axes, 2);
    check("generic hats", t->hct_Hats, 0);
    memset(&r, 0, sizeof(r));
    hidctrl_Decode(t, generic_items, 1, report, sizeof(report), &r);
    check("generic X", r.axes[0], 0x7F);
    check("generic buttons", r.buttons[0], 0x3FF);
    FreeVec(t);
}

/* Composite interface: a mouse collection first, then a game pad; mouse items must be ignored */
static const struct HidCtrlItem composite_items[] =
{
    INID(1, 0x090001, 0, 1, 0, 1, HIDCTRL_COLL_MOUSE),
    INID(1, 0x090002, 1, 1, 0, 1, HIDCTRL_COLL_MOUSE),
    { 0x010030, RPF_MAIN_VARIABLE | RPF_MAIN_RELATIVE, REPORT_MAIN_INPUT, 8, 8, 1, -127, 127, TRUE, HIDCTRL_COLL_MOUSE, NULL },
    { 0x010031, RPF_MAIN_VARIABLE | RPF_MAIN_RELATIVE, REPORT_MAIN_INPUT, 16, 8, 1, -127, 127, TRUE, HIDCTRL_COLL_MOUSE, NULL },
    INID(2, 0x010030, 0, 16, -32768, 32767, HIDCTRL_COLL_GAMEPAD),
    INID(2, 0x010031, 16, 16, -32768, 32767, HIDCTRL_COLL_GAMEPAD),
    INID(2, 0x090001, 32, 1, 0, 1, HIDCTRL_COLL_GAMEPAD),
    INID(2, 0x090003, 33, 1, 0, 1, HIDCTRL_COLL_GAMEPAD),
    { 0x0200C4, RPF_MAIN_VARIABLE, REPORT_MAIN_INPUT, 40, 8, 2, 0, 255, FALSE, HIDCTRL_COLL_GAMEPAD, NULL },
    { 0x010030, RPF_MAIN_VARIABLE | RPF_MAIN_CONST, REPORT_MAIN_INPUT, 48, 8, 2, 0, 255, FALSE, HIDCTRL_COLL_GAMEPAD, NULL },
};

static void test_composite(void)
{
    struct HidCtrlTable *t = AllocVec(sizeof(struct HidCtrlTable), MEMF_CLEAR);
    struct pHidd_Controller_RawReport r;
    static const UBYTE report2[] = { 0x00, 0x80, 0xFF, 0x7F, 0x02, 0xC0, 0x00 };
    ULONG i, ncontrols = 0;

    Printf("composite:\n");
    check("composite mapped", hidctrl_Build(composite_items, sizeof(composite_items) / sizeof(composite_items[0]), t), 5);
    check("composite buttons (gap filled)", t->hct_Buttons, 3);
    check("composite axes", t->hct_Axes, 3);
    for (i = 0; t->hct_Controls[i].kind != vHidd_Controller_Ctl_End; i++)
        ncontrols++;
    check("composite table entries", ncontrols, 6);
    check("composite accelerator unipolar", t->hct_Controls[4].flags & vHidd_Controller_CF_Unipolar, vHidd_Controller_CF_Unipolar);
    memset(&r, 0, sizeof(r));
    check("composite mouse report ignored", hidctrl_Decode(t, composite_items, 1, report2, sizeof(report2), &r), FALSE);
    check("composite pad report", hidctrl_Decode(t, composite_items, 2, report2, sizeof(report2), &r), TRUE);
    check("composite X signed", r.axes[0], -32768);
    check("composite Y signed", r.axes[1], 32767);
    check("composite buttons", r.buttons[0], (1 << 2));   /* usage 3 -> index 2 */
    check("composite accelerator", r.axes[2], 0xC0);
    FreeVec(t);
}

static const struct HidCtrlItem keyboard_items[] =
{
    IN(0x0700E0, 0, 1, 0, 1, HIDCTRL_COLL_KEYBOARD),
    IN(0x0700E1, 1, 1, 0, 1, HIDCTRL_COLL_KEYBOARD),
};

static void test_keyboard(void)
{
    struct HidCtrlTable *t = AllocVec(sizeof(struct HidCtrlTable), MEMF_CLEAR);

    Printf("keyboard:\n");
    check("keyboard root", hidctrl_FindRoot(keyboard_items, 2), 0);
    check("keyboard mapped", hidctrl_Build(keyboard_items, 2, t), 0);
    FreeVec(t);
}

static void test_extract(void)
{
    struct HidCtrlItem s16 = { 0x010030, RPF_MAIN_VARIABLE, REPORT_MAIN_INPUT, 8, 16, 1, -32768, 32767, TRUE, HIDCTRL_COLL_GAMEPAD, NULL };
    struct HidCtrlItem u12 = { 0x010030, RPF_MAIN_VARIABLE, REPORT_MAIN_INPUT, 4, 12, 1, 0, 4095, FALSE, HIDCTRL_COLL_GAMEPAD, NULL };
    struct HidCtrlItem s10 = { 0x010030, RPF_MAIN_VARIABLE, REPORT_MAIN_INPUT, 3, 10, 1, -512, 511, TRUE, HIDCTRL_COLL_GAMEPAD, NULL };
    static const UBYTE b1[] = { 0x00, 0x00, 0x80 };
    static const UBYTE b2[] = { 0xF0, 0xFF, 0x0F };
    static const UBYTE b3[] = { 0xF8, 0x1F, 0x00 };
    LONG v = 0;

    Printf("extract:\n");
    check("extract s16", hidctrl_Extract(&s16, b1, 3, &v), TRUE); check("extract s16 value", v, -32768);
    check("extract u12", hidctrl_Extract(&u12, b2, 3, &v), TRUE); check("extract u12 value", v, 4095);
    check("extract s10", hidctrl_Extract(&s10, b3, 3, &v), TRUE); check("extract s10 value", v, -1);
    check("extract short buffer", hidctrl_Extract(&s16, b1, 2, &v), FALSE);
}

int main(void)
{
    IPTR args[1] = { 0 };
    struct RDArgs *rda = ReadArgs("SERIAL/S", args, NULL);

    g_serial = args[0] ? TRUE : FALSE;

    test_ds4();
    test_generic();
    test_composite();
    test_keyboard();
    test_extract();

    Printf("hidmap_test: %lu check(s), %lu failure(s)\n", g_checks, g_failures);
    if (rda)
        FreeArgs(rda);
    return g_failures ? RETURN_ERROR : RETURN_OK;
}
