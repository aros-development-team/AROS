/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: ControllerSim - registers simulated controllers (virtualpad.hidd) with
          controller.hidd, feeds them scripted input and checks expectations.

    Usage:
        ControllerSim LISTPROFILES
        ControllerSim LIST
        ControllerSim PROFILE=<name> [SLOT=<n>] [SCRIPT=<file>] [VERBOSE]
        ControllerSim ID=<device id> SCRIPT=<file> [VERBOSE]
        ControllerSim REMOVE ID=<device id>

    Script lines (one command per line, '#' starts a comment):
        wait <ms>
        btn <index> <0|1>
        axis <index> <raw value>
        hat <index> <0..7 | 8 centred>
        touch <pad> <finger> <down> <x> <y>
        sensor <index> <x> <y> <z>
        power <state 0..4> <percent>
        connected <0|1>
        rumble <low> <high> [<lt> <rt>]        (calls the output method, as an application would)
        led <r> <g> <b>
        player <index>
        expect reading <field> <value>         field: button N | axis N | hat N | gp <name|N> | gpaxis N | arch N | sequence | mapped
        expect event <type> [<code> [<value>]] type: press release axis hat frame added removed remapped power
                                                     touchdown touchmotion touchup sensor connection port overflow
        expect noevent
        expect output <type> <a> [<b> [<c> [<d>]]]  type: rumble led player effect sensors upload play stop remove
        expect port <n | -1>
        expect jp <port|dev> <token>...        ReadJoyPort() through lowlevel.library; tokens: notavail gamectlr
                                               joystk mouse analogue unknown up down left right red blue green
                                               yellow forward reverse play, or a 0x literal; all OR-ed together
        expect jpa <port|dev> <x> <y>          analogue readout (JP_ANALOGUE_PORT_MAGIC): X/Y as 0..255
        sja <port|dev> <type|reinit|slow|fast|off> [<value>]   SetJoyPortAttrs(); type: auto gamectlr mouse joystk analogue
        dump reading | dump events | dump outputs | dump info
*/

#include <exec/types.h>
#include <exec/memory.h>
#include <exec/ports.h>
#include <dos/dos.h>
#include <dos/rdargs.h>
#include <oop/oop.h>
#include <hidd/hidd.h>
#include <hidd/controller.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/oop.h>
#include <aros/debug.h>
#include <exec/rawfmt.h>
#include <libraries/lowlevel.h>
#include <libraries/lowlevel_ext.h>
#include <proto/lowlevel.h>

#include "virtualpad.h"

const char version[] = "$VER: ControllerSim 1.0 (26.9.2026)";

OOP_AttrBase HiddAttrBase;
OOP_AttrBase HWAttrBase;
OOP_AttrBase HiddControllerAB;
OOP_AttrBase HWControllerAB;
OOP_AttrBase HiddVirtualPadAB;
OOP_AttrBase HiddInputAB;

struct Library *OOPBase;
struct Library *LowLevelBase;

/* Output goes to the shell and, with SERIAL, also to the kernel debug channel */
static BOOL g_serial = FALSE;
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

static const struct OOP_ABDescr attrbases[] =
{
    { IID_Hidd,             &HiddAttrBase       },
    { IID_HW,               &HWAttrBase         },
    { IID_Hidd_Controller,  &HiddControllerAB   },
    { IID_HW_Controller,    &HWControllerAB     },
    { IID_Hidd_VirtualPad,  &HiddVirtualPadAB   },
    { IID_Hidd_Input,       &HiddInputAB        },
    { NULL,                 NULL                }
};

static const char *profile_names[] =
{
    "x360", "xone", "ds4", "ds5", "switchpro", "l3dpro", "g29", "cd32", "generic", "overflow", NULL
};

static const char *event_names[] =
{
    "press", "release", "axis", "hat", "frame", "added", "removed", "remapped", "power",
    "touchdown", "touchmotion", "touchup", "sensor", "connection", "port", "overflow", NULL
};

static const char *gp_names[] =
{
    "south", "east", "west", "north", "back", "guide", "start", "leftstick", "rightstick",
    "leftshoulder", "rightshoulder", "dpup", "dpdown", "dpleft", "dpright", "misc1",
    "paddle1", "paddle2", "paddle3", "paddle4", "touchpad", "misc2", "misc3", "misc4", "misc5", "misc6", NULL
};

static const char *output_names[] =
{
    NULL, "rumble", "led", "player", "effect", "sensors", "upload", "play", "stop", "remove", NULL
};

#define MODE_SIGNAL   0
#define MODE_PORT     1
#define MODE_CALLBACK 2

struct sim
{
    OOP_Object  *hw;
    OOP_Object  *dev;
    OOP_Object  *consumer;
    UWORD       device_id;
    BYTE        sigbit;
    UBYTE       mode;
    BOOL        verbose;
    ULONG       failures;
    ULONG       checks;
    ULONG       line;
    struct MsgPort *port;
};

/* callback mode: the handler runs in the producer's context and just queues */
#define CB_RING 256
static struct pHidd_Controller_Event cb_ring[CB_RING];
static volatile ULONG cb_head, cb_tail;

static void sim_callback(APTR data, InputIrqData_t iedata)
{
    const struct pHidd_Controller_Event *ev = (const struct pHidd_Controller_Event *)iedata;
    ULONG next = (cb_head + 1) % CB_RING;

    if (next != cb_tail)
    {
        cb_ring[cb_head] = *ev;
        cb_head = next;
    }
}

static BOOL next_event(struct sim *s, struct pHidd_Controller_Event *ev)
{
    switch (s->mode)
    {
    case MODE_SIGNAL:
        return HIDD_Controller_GetEvent(s->consumer, ev);
    case MODE_PORT:
    {
        struct Hidd_Controller_EventMsg *m = (struct Hidd_Controller_EventMsg *)GetMsg(s->port);
        if (!m)
            return FALSE;
        *ev = m->ev;
        ReplyMsg(&m->msg);
        return TRUE;
    }
    case MODE_CALLBACK:
        if (cb_tail == cb_head)
            return FALSE;
        *ev = cb_ring[cb_tail];
        cb_tail = (cb_tail + 1) % CB_RING;
        return TRUE;
    }
    return FALSE;
}

/* --- small helpers ---------------------------------------------------------- */

static int c_strlen(const char *s) { int n = 0; while (s && s[n]) n++; return n; }

static BOOL c_streq(const char *a, const char *b)
{
    if (!a || !b) return FALSE;
    while (*a && *b)
    {
        char ca = *a, cb = *b;
        if (ca >= 'A' && ca <= 'Z') ca += 32;
        if (cb >= 'A' && cb <= 'Z') cb += 32;
        if (ca != cb) return FALSE;
        a++; b++;
    }
    return *a == *b;
}

static LONG c_atol(const char *s, BOOL *ok)
{
    LONG v = 0;
    BOOL neg = FALSE;
    int digits = 0;

    if (ok) *ok = FALSE;
    if (!s) return 0;
    if (*s == '-') { neg = TRUE; s++; }
    else if (*s == '+') s++;
    if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X'))
    {
        s += 2;
        while (*s)
        {
            int d;
            if (*s >= '0' && *s <= '9') d = *s - '0';
            else if (*s >= 'a' && *s <= 'f') d = *s - 'a' + 10;
            else if (*s >= 'A' && *s <= 'F') d = *s - 'A' + 10;
            else break;
            v = v * 16 + d; s++; digits++;
        }
    }
    else
    {
        while (*s >= '0' && *s <= '9') { v = v * 10 + (*s - '0'); s++; digits++; }
    }
    if (ok) *ok = (digits > 0 && *s == 0);
    return neg ? -v : v;
}

static int name_index(const char **names, const char *s)
{
    int i;
    BOOL ok;
    LONG v = c_atol(s, &ok);

    if (ok) return (int)v;
    for (i = 0; names[i] || i == 0; i++)
        if (names[i] && c_streq(names[i], s))
            return i;
    return -1;
}

static void guid_string(const UBYTE *g, char *out)
{
    static const char hex[] = "0123456789abcdef";
    int i;
    for (i = 0; i < 16; i++)
    {
        out[i * 2] = hex[g[i] >> 4];
        out[i * 2 + 1] = hex[g[i] & 15];
    }
    out[32] = 0;
}

/* --- device helpers ----------------------------------------------------------- */

static void print_device(OOP_Object *dev)
{
    IPTR id = 0, vendor = 0, product = 0, port = 0, caps = 0, name = 0, hwname = 0;
    IPTR nb = 0, na = 0, nh = 0;
    IPTR guid = 0;
    char g[33] = "";

    OOP_GetAttr(dev, aHidd_Controller_DeviceID, &id);
    OOP_GetAttr(dev, aHidd_Name, &name);
    OOP_GetAttr(dev, aHidd_HardwareName, &hwname);
    OOP_GetAttr(dev, aHidd_Controller_VendorID, &vendor);
    OOP_GetAttr(dev, aHidd_Controller_ProductID, &product);
    OOP_GetAttr(dev, aHidd_Controller_LegacyPort, &port);
    OOP_GetAttr(dev, aHidd_Controller_Capabilities, &caps);
    OOP_GetAttr(dev, aHidd_Controller_ButtonCount, &nb);
    OOP_GetAttr(dev, aHidd_Controller_AxisCount, &na);
    OOP_GetAttr(dev, aHidd_Controller_HatCount, &nh);
    OOP_GetAttr(dev, aHidd_Controller_GUID, &guid);
    if (guid) guid_string((const UBYTE *)guid, g);

    Printf("  id %3ld  %-28s %04lx:%04lx  port %2ld  %2ld btn %2ld axes %ld hats  caps %08lx  %s\n",
           id, hwname ? (CONST_STRPTR)hwname : (name ? (CONST_STRPTR)name : (CONST_STRPTR)"?"),
           vendor, product, (LONG)(WORD)port, nb, na, nh, caps, g);
}

static void list_devices(struct sim *s)
{
    UWORD ids[64];
    ULONG n = HW_Controller_GetDeviceIDs(s->hw, ids, 64), i;

    Printf("%lu controller(s) registered\n", n);
    for (i = 0; i < n && i < 64; i++)
    {
        OOP_Object *dev = HW_Controller_FindDevice(s->hw, ids[i]);
        if (dev)
            print_device(dev);
    }
}

static BOOL open_consumer(struct sim *s)
{
    struct TagItem tags_signal[] =
    {
        { aHidd_Controller_Device,       (IPTR)s->dev          },
        { aHidd_Controller_NotifyTask,   (IPTR)FindTask(NULL)  },
        { aHidd_Controller_NotifySignal, s->sigbit             },
        { aHidd_Controller_QueueDepth,   256                   },
        { TAG_DONE,                      0                     }
    };
    struct TagItem tags_port[] =
    {
        { aHidd_Controller_Device,       (IPTR)s->dev          },
        { aHidd_Controller_NotifyPort,   (IPTR)s->port         },
        { aHidd_Controller_QueueDepth,   256                   },
        { TAG_DONE,                      0                     }
    };
    struct TagItem tags_cb[] =
    {
        { aHidd_Controller_Device,       (IPTR)s->dev          },
        { aHidd_Input_IrqHandler,        (IPTR)sim_callback    },
        { aHidd_Input_IrqHandlerData,    (IPTR)s               },
        { TAG_DONE,                      0                     }
    };
    struct TagItem *tags = tags_signal;

    if (s->mode == MODE_PORT)
    {
        s->port = CreateMsgPort();
        if (!s->port)
            return FALSE;
        tags_port[1].ti_Data = (IPTR)s->port;     /* the initialiser ran before the port existed */
        tags = tags_port;
    }
    else if (s->mode == MODE_CALLBACK)
    {
        cb_head = cb_tail = 0;
        tags = tags_cb;
    }

    s->consumer = OOP_NewObject(NULL, CLID_Hidd_Controller, tags);
    return s->consumer != NULL;
}

/* --- script execution ---------------------------------------------------------- */

static void fail(struct sim *s, const char *what, LONG expected, LONG got)
{
    s->failures++;
    Printf("line %lu: FAIL %s: expected %ld, got %ld\n", s->line, what, expected, got);
}

static void pass(struct sim *s, const char *what)
{
    s->checks++;
    if (s->verbose)
        Printf("line %lu: ok %s\n", s->line, what);
}

static void dump_reading(struct sim *s, const struct pHidd_Controller_Reading *rd)
{
    UWORD i;

    Printf("reading: dev %lu seq %lu flags %04lx ts %lu buttons %08lx%08lx\n",
           (ULONG)rd->device_id, rd->sequence, (ULONG)rd->flags, (ULONG)rd->timestamp,
           rd->buttons[1], rd->buttons[0]);
    Printf("  axes:");
    for (i = 0; i < rd->axis_count; i++) Printf(" %ld", (LONG)rd->axes[i]);
    Printf("\n  hats:");
    for (i = 0; i < rd->hat_count; i++) Printf(" %lu", (ULONG)rd->hats[i]);
    Printf("\n  gp buttons %08lx  gp axes", rd->gp_buttons);
    for (i = 0; i < vHidd_Controller_GPA_AxisCount; i++) Printf(" %ld", (LONG)rd->gp_axes[i]);
    Printf("  arch");
    for (i = 0; i < vHidd_Controller_ARCH_AxisCount; i++) Printf(" %ld", (LONG)rd->arch_axes[i]);
    Printf("\n");
}

static void print_event(const struct pHidd_Controller_Event *ev)
{
    const char *n = (ev->type < vHidd_Controller_EventTypeCount) ? event_names[ev->type] : "?";
    Printf("  event %-11s dev %lu code %lu std 0x%03lx flags %lx value %ld seq %lu\n",
           n, (ULONG)ev->device_id, (ULONG)ev->code, (ULONG)ev->std, (ULONG)ev->flags, ev->value, ev->sequence);
}

static BOOL expect_reading(struct sim *s, char **argv, int argc)
{
    struct pHidd_Controller_Reading rd;
    LONG got = 0, expected;
    BOOL ok;
    char what[64];

    if (argc < 2) { Printf("line %lu: expect reading needs a field and a value\n", s->line); s->failures++; return FALSE; }

    rd.size = sizeof(rd);
    HIDD_Controller_GetReading(s->dev, &rd);

    expected = c_atol(argv[argc - 1], &ok);
    if (!ok) { Printf("line %lu: bad value '%s'\n", s->line, argv[argc - 1]); s->failures++; return FALSE; }

    if (c_streq(argv[0], "button") && argc == 3)
    {
        LONG i = c_atol(argv[1], NULL);
        got = (rd.buttons[i >> 5] >> (i & 31)) & 1;
    }
    else if (c_streq(argv[0], "axis") && argc == 3)
        got = rd.axes[c_atol(argv[1], NULL) & 15];
    else if (c_streq(argv[0], "hat") && argc == 3)
        got = rd.hats[c_atol(argv[1], NULL) & 3];
    else if (c_streq(argv[0], "gp") && argc == 3)
    {
        int b = name_index(gp_names, argv[1]);
        if (b < 0) { Printf("line %lu: unknown gp button '%s'\n", s->line, argv[1]); s->failures++; return FALSE; }
        got = (rd.gp_buttons >> b) & 1;
    }
    else if (c_streq(argv[0], "gpaxis") && argc == 3)
        got = rd.gp_axes[c_atol(argv[1], NULL) % vHidd_Controller_GPA_AxisCount];
    else if (c_streq(argv[0], "arch") && argc == 3)
        got = rd.arch_axes[c_atol(argv[1], NULL) % vHidd_Controller_ARCH_AxisCount];
    else if (c_streq(argv[0], "sequence") && argc == 2)
        got = rd.sequence;
    else if (c_streq(argv[0], "mapped") && argc == 2)
        got = (rd.flags & vHidd_Controller_RF_Mapped) ? 1 : 0;
    else if (c_streq(argv[0], "connected") && argc == 2)
        got = (rd.flags & vHidd_Controller_RF_Connected) ? 1 : 0;
    else
    {
        Printf("line %lu: unknown reading field '%s'\n", s->line, argv[0]);
        s->failures++;
        return FALSE;
    }

    {
        int n = 0, i;
        for (i = 0; i < argc - 1 && n < 60; i++)
        {
            const char *p = argv[i];
            if (n) what[n++] = ' ';
            while (*p && n < 62) what[n++] = *p++;
        }
        what[n] = 0;
    }
    if (got != expected) { fail(s, what, expected, got); return FALSE; }
    pass(s, what);
    return TRUE;
}

static BOOL expect_event(struct sim *s, char **argv, int argc)
{
    struct pHidd_Controller_Event ev;
    int type;
    LONG code = -1, value = 0;
    BOOL want_value = FALSE;

    if (argc < 1) { Printf("line %lu: expect event needs a type\n", s->line); s->failures++; return FALSE; }
    type = name_index(event_names, argv[0]);
    if (type < 0) { Printf("line %lu: unknown event type '%s'\n", s->line, argv[0]); s->failures++; return FALSE; }
    if (argc >= 2) code = c_atol(argv[1], NULL);
    if (argc >= 3) { value = c_atol(argv[2], NULL); want_value = TRUE; }

    while (next_event(s, &ev))
    {
        if (s->verbose)
            print_event(&ev);
        if (ev.type == type && (code < 0 || ev.code == code) && (!want_value || ev.value == value))
        {
            pass(s, argv[0]);
            return TRUE;
        }
    }
    s->failures++;
    Printf("line %lu: FAIL expect event %s%s%s: not found\n", s->line, argv[0], argc >= 2 ? " code " : "", argc >= 2 ? argv[1] : "");
    return FALSE;
}

static BOOL expect_noevent(struct sim *s)
{
    struct pHidd_Controller_Event ev;
    ULONG n = 0;

    while (next_event(s, &ev))
    {
        if (s->verbose || n == 0)
            print_event(&ev);
        n++;
    }
    if (n) { fail(s, "noevent (queued events)", 0, n); return FALSE; }
    pass(s, "noevent");
    return TRUE;
}

static BOOL expect_output(struct sim *s, char **argv, int argc)
{
    struct VirtualPad_Output log[VPAD_LOG_MAX_QUERY];
    ULONG n, i;
    int type;
    LONG want[4];
    int nwant = 0;

    if (argc < 2) { Printf("line %lu: expect output needs a type and a value\n", s->line); s->failures++; return FALSE; }
    type = name_index(output_names, argv[0]);
    if (type <= 0) { Printf("line %lu: unknown output type '%s'\n", s->line, argv[0]); s->failures++; return FALSE; }
    for (i = 1; i < (ULONG)argc && nwant < 4; i++)
        want[nwant++] = c_atol(argv[i], NULL);

    n = HIDD_VirtualPad_GetOutputLog(s->dev, log, VPAD_LOG_MAX_QUERY);
    for (i = 0; i < n; i++)
    {
        LONG v[4] = { log[i].a, log[i].b, log[i].c, log[i].d };
        int k;
        if (s->verbose)
            Printf("  output %s %ld %ld %ld %ld\n", output_names[log[i].type], v[0], v[1], v[2], v[3]);
        if (log[i].type != type) continue;
        for (k = 0; k < nwant; k++)
            if (v[k] != want[k]) break;
        if (k == nwant) { pass(s, argv[0]); return TRUE; }
    }
    s->failures++;
    Printf("line %lu: FAIL expect output %s: not found (%lu logged)\n", s->line, argv[0], n);
    return FALSE;
}

static BOOL expect_port(struct sim *s, char **argv, int argc)
{
    IPTR port = 0;
    LONG expected;

    if (argc < 1) { s->failures++; return FALSE; }
    expected = c_atol(argv[0], NULL);
    OOP_GetAttr(s->dev, aHidd_Controller_LegacyPort, &port);
    if ((LONG)(WORD)port != expected) { fail(s, "port", expected, (LONG)(WORD)port); return FALSE; }
    pass(s, "port");
    return TRUE;
}

/* --- lowlevel.library bridge ---------------------------------------------------- */

static const char *jp_names[] = { "notavail", "gamectlr", "joystk", "mouse", "analogue", "unknown",
                                  "up", "down", "left", "right",
                                  "red", "blue", "green", "yellow", "forward", "reverse", "play", NULL };
static const ULONG jp_values[] = { JP_TYPE_NOTAVAIL, JP_TYPE_GAMECTLR, JP_TYPE_JOYSTK, JP_TYPE_MOUSE, JP_TYPE_ANALOGUE, JP_TYPE_UNKNOWN,
                                   JPF_JOY_UP, JPF_JOY_DOWN, JPF_JOY_LEFT, JPF_JOY_RIGHT,
                                   JPF_BUTTON_RED, JPF_BUTTON_BLUE, JPF_BUTTON_GREEN, JPF_BUTTON_YELLOW,
                                   JPF_BUTTON_FORWARD, JPF_BUTTON_REVERSE, JPF_BUTTON_PLAY };

/* "dev" = the legacy port of the device under test, otherwise a port number */
static LONG jp_port(struct sim *s, const char *arg)
{
    IPTR port = 0;

    if (!c_streq(arg, "dev"))
        return c_atol(arg, NULL);
    OOP_GetAttr(s->dev, aHidd_Controller_LegacyPort, &port);
    return (LONG)(WORD)port;
}

static BOOL expect_jp(struct sim *s, char **argv, int argc)
{
    ULONG want = 0, got;
    LONG port;
    int i;

    if (argc < 2) { Printf("line %lu: expect jp needs a port and tokens\n", s->line); s->failures++; return FALSE; }
    if (!LowLevelBase) { Printf("line %lu: lowlevel.library not open\n", s->line); s->failures++; return FALSE; }
    port = jp_port(s, argv[0]);
    if (port < 0) { Printf("line %lu: FAIL expect jp: device has no legacy port\n", s->line); s->failures++; return FALSE; }
    for (i = 1; i < argc; i++)
    {
        int k;
        for (k = 0; jp_names[k]; k++)
            if (c_streq(jp_names[k], argv[i])) break;
        if (jp_names[k]) want |= jp_values[k];
        else             want |= (ULONG)c_atol(argv[i], NULL);
    }
    got = ReadJoyPort(port);
    if (s->verbose)
        Printf("  ReadJoyPort(%ld) = %08lx\n", port, got);
    if (got != want)
    {
        s->failures++;
        Printf("line %lu: FAIL expect jp port %ld: expected %08lx, got %08lx\n", s->line, port, want, got);
        return FALSE;
    }
    pass(s, "jp");
    return TRUE;
}

static BOOL expect_jpa(struct sim *s, char **argv, int argc)
{
    ULONG got;
    LONG port, x, y;

    if (argc < 3) { Printf("line %lu: expect jpa needs a port, x and y\n", s->line); s->failures++; return FALSE; }
    if (!LowLevelBase) { Printf("line %lu: lowlevel.library not open\n", s->line); s->failures++; return FALSE; }
    port = jp_port(s, argv[0]);
    if (port < 0) { Printf("line %lu: FAIL expect jpa: device has no legacy port\n", s->line); s->failures++; return FALSE; }
    x = c_atol(argv[1], NULL);
    y = c_atol(argv[2], NULL);
    got = ReadJoyPort(JP_ANALOGUE_PORT_MAGIC | port);
    if (s->verbose)
        Printf("  ReadJoyPort(analogue %ld) = %08lx\n", port, got);
    if ((got & JP_TYPE_MASK) != JP_TYPE_ANALOGUE) { fail(s, "jpa type", JP_TYPE_ANALOGUE >> 28, got >> 28); return FALSE; }
    if ((LONG)(got & JP_XAXIS_MASK) != x) { fail(s, "jpa x", x, got & JP_XAXIS_MASK); return FALSE; }
    if ((LONG)((got & JP_YAXIS_MASK) >> 8) != y) { fail(s, "jpa y", y, (got & JP_YAXIS_MASK) >> 8); return FALSE; }
    pass(s, "jpa");
    return TRUE;
}

static void do_sja(struct sim *s, char **argv, int argc)
{
    struct TagItem tags[2] = { { TAG_DONE, 0 }, { TAG_DONE, 0 } };
    LONG port, value = argc >= 3 ? c_atol(argv[2], NULL) : 0;
    BOOL ok;

    if (argc < 2) { Printf("line %lu: sja needs a port and an attribute\n", s->line); s->failures++; return; }
    if (!LowLevelBase) { Printf("line %lu: lowlevel.library not open\n", s->line); s->failures++; return; }
    port = jp_port(s, argv[0]);
    if (port < 0) { Printf("line %lu: sja: device has no legacy port\n", s->line); s->failures++; return; }
    if (c_streq(argv[1], "type") && argc >= 3)
    {
        static const char *types[] = { "auto", "gamectlr", "mouse", "joystk", NULL };
        int t = name_index(types, argv[2]);
        tags[0].ti_Tag = SJA_Type;
        tags[0].ti_Data = c_streq(argv[2], "analogue") ? SJA_TYPE_ANALOGUE : (t >= 0 ? t : SJA_TYPE_AUTOSENSE);
    }
    else if (c_streq(argv[1], "reinit")) { tags[0].ti_Tag = SJA_Reinitialize; tags[0].ti_Data = TRUE; }
    else if (c_streq(argv[1], "slow"))   { tags[0].ti_Tag = SJA_RumbleSetSlowMotor; tags[0].ti_Data = value; }
    else if (c_streq(argv[1], "fast"))   { tags[0].ti_Tag = SJA_RumbleSetFastMotor; tags[0].ti_Data = value; }
    else if (c_streq(argv[1], "off"))    { tags[0].ti_Tag = SJA_RumbleOff; tags[0].ti_Data = TRUE; }
    else { Printf("line %lu: unknown sja attribute '%s'\n", s->line, argv[1]); s->failures++; return; }
    ok = SetJoyPortAttrsA(port, tags);
    if (s->verbose)
        Printf("  SetJoyPortAttrs(%ld, %s) = %ld\n", port, argv[1], (LONG)ok);
    if (!ok) { Printf("line %lu: FAIL sja %s: SetJoyPortAttrs returned FALSE\n", s->line, argv[1]); s->failures++; }
    else pass(s, "sja");
}

static void run_line(struct sim *s, char *line)
{
    char *argv[12];
    int argc = 0;
    char *p = line;

    while (*p && argc < 12)
    {
        while (*p == ' ' || *p == '\t') p++;
        if (!*p || *p == '#') break;
        argv[argc++] = p;
        while (*p && *p != ' ' && *p != '\t' && *p != '\n' && *p != '\r') p++;
        if (*p) *p++ = 0;
    }
    if (!argc)
        return;

    if (c_streq(argv[0], "wait") && argc >= 2)
    {
        LONG ms = c_atol(argv[1], NULL);
        LONG ticks = (ms * 50 + 999) / 1000;
        Delay(ticks > 0 ? ticks : 1);
    }
    else if (c_streq(argv[0], "btn") && argc >= 3)
        HIDD_VirtualPad_Feed(s->dev, vHidd_Controller_Ctl_Button, c_atol(argv[1], NULL), c_atol(argv[2], NULL));
    else if (c_streq(argv[0], "axis") && argc >= 3)
        HIDD_VirtualPad_Feed(s->dev, vHidd_Controller_Ctl_Axis, c_atol(argv[1], NULL), c_atol(argv[2], NULL));
    else if (c_streq(argv[0], "hat") && argc >= 3)
        HIDD_VirtualPad_Feed(s->dev, vHidd_Controller_Ctl_Hat, c_atol(argv[1], NULL), c_atol(argv[2], NULL));
    else if (c_streq(argv[0], "touch") && argc >= 6)
    {
        struct pHidd_Controller_Finger f;
        f.down = c_atol(argv[3], NULL) ? 1 : 0;
        f.id = c_atol(argv[2], NULL);
        f.x = c_atol(argv[4], NULL);
        f.y = c_atol(argv[5], NULL);
        f.pressure = f.down ? 65535 : 0;
        HIDD_VirtualPad_FeedTouch(s->dev, c_atol(argv[1], NULL), c_atol(argv[2], NULL), &f);
    }
    else if (c_streq(argv[0], "sensor") && argc >= 5)
        HIDD_VirtualPad_FeedSensor(s->dev, c_atol(argv[1], NULL), c_atol(argv[2], NULL), c_atol(argv[3], NULL), c_atol(argv[4], NULL));
    else if (c_streq(argv[0], "power") && argc >= 3)
        HIDD_VirtualPad_FeedPower(s->dev, c_atol(argv[1], NULL), c_atol(argv[2], NULL));
    else if (c_streq(argv[0], "connected") && argc >= 2)
        HIDD_VirtualPad_FeedConnected(s->dev, c_atol(argv[1], NULL) ? TRUE : FALSE);
    else if (c_streq(argv[0], "rumble") && argc >= 3)
    {
        struct pHidd_Controller_Rumble r;
        r.low = c_atol(argv[1], NULL);
        r.high = c_atol(argv[2], NULL);
        r.left_trigger = argc >= 4 ? c_atol(argv[3], NULL) : 0;
        r.right_trigger = argc >= 5 ? c_atol(argv[4], NULL) : 0;
        r.duration_ms = 0;
        if (!HIDD_Controller_SetRumble(s->dev, &r) && s->verbose)
            Printf("line %lu: SetRumble returned FALSE\n", s->line);
    }
    else if (c_streq(argv[0], "led") && argc >= 4)
        HIDD_Controller_SetLED(s->dev, c_atol(argv[1], NULL), c_atol(argv[2], NULL), c_atol(argv[3], NULL));
    else if (c_streq(argv[0], "player") && argc >= 2)
        HIDD_Controller_SetPlayerIndex(s->dev, c_atol(argv[1], NULL));
    else if (c_streq(argv[0], "sja") && argc >= 3)
        do_sja(s, argv + 1, argc - 1);
    else if (c_streq(argv[0], "expect") && argc >= 2)
    {
        if (c_streq(argv[1], "reading"))      expect_reading(s, argv + 2, argc - 2);
        else if (c_streq(argv[1], "event"))   expect_event(s, argv + 2, argc - 2);
        else if (c_streq(argv[1], "noevent")) expect_noevent(s);
        else if (c_streq(argv[1], "output"))  expect_output(s, argv + 2, argc - 2);
        else if (c_streq(argv[1], "port"))    expect_port(s, argv + 2, argc - 2);
        else if (c_streq(argv[1], "jp"))      expect_jp(s, argv + 2, argc - 2);
        else if (c_streq(argv[1], "jpa"))     expect_jpa(s, argv + 2, argc - 2);
        else { Printf("line %lu: unknown expectation '%s'\n", s->line, argv[1]); s->failures++; }
    }
    else if (c_streq(argv[0], "dump") && argc >= 2)
    {
        if (c_streq(argv[1], "reading"))
        {
            struct pHidd_Controller_Reading rd;
            rd.size = sizeof(rd);
            HIDD_Controller_GetReading(s->dev, &rd);
            dump_reading(s, &rd);
        }
        else if (c_streq(argv[1], "events"))
        {
            struct pHidd_Controller_Event ev;
            while (next_event(s, &ev))
                print_event(&ev);
        }
        else if (c_streq(argv[1], "outputs"))
        {
            struct VirtualPad_Output log[VPAD_LOG_MAX_QUERY];
            ULONG n = HIDD_VirtualPad_GetOutputLog(s->dev, log, VPAD_LOG_MAX_QUERY), i;
            for (i = 0; i < n; i++)
                Printf("  output %s %ld %ld %ld %ld\n", output_names[log[i].type], log[i].a, log[i].b, log[i].c, log[i].d);
        }
        else if (c_streq(argv[1], "info"))
            print_device(s->dev);
    }
    else
    {
        Printf("line %lu: unknown command '%s'\n", s->line, argv[0]);
        s->failures++;
    }
}

static BOOL run_script(struct sim *s, CONST_STRPTR path)
{
    BPTR fh = Open(path, MODE_OLDFILE);
    char *buf;

    if (!fh)
    {
        Printf("cannot open script '%s'\n", path);
        return FALSE;
    }
    buf = AllocVec(1024, MEMF_ANY);
    if (!buf)
    {
        Close(fh);
        return FALSE;
    }
    s->line = 0;
    while (FGets(fh, buf, 1024))
    {
        s->line++;
        run_line(s, buf);
    }
    FreeVec(buf);
    Close(fh);
    Printf("%lu check(s), %lu failure(s)\n", s->checks, s->failures);
    return s->failures == 0;
}

/* --- main --------------------------------------------------------------------- */

int main(void)
{
    struct sim s = { 0 };
    IPTR args[11] = { 0 };
    struct RDArgs *rda;
    struct Library *ControllerHiddBase = NULL, *VirtualPadBase = NULL;
    int rc = RETURN_FAIL;
    enum { A_PROFILE, A_ID, A_SLOT, A_SCRIPT, A_REMOVE, A_LIST, A_LISTPROFILES, A_VERBOSE, A_SERIAL, A_PORTMODE, A_CALLBACK };

    rda = ReadArgs("PROFILE,ID/N,SLOT/N,SCRIPT,REMOVE/S,LIST/S,LISTPROFILES/S,VERBOSE/S,SERIAL/S,PORTMODE/S,CALLBACK/S", args, NULL);
    if (!rda)
    {
        PrintFault(IoErr(), "ControllerSim");
        return RETURN_FAIL;
    }
    s.verbose = args[A_VERBOSE] ? TRUE : FALSE;
    g_serial = args[A_SERIAL] ? TRUE : FALSE;
    s.mode = args[A_PORTMODE] ? MODE_PORT : (args[A_CALLBACK] ? MODE_CALLBACK : MODE_SIGNAL);
    s.sigbit = -1;

    if (args[A_LISTPROFILES])
    {
        const char **p;
        for (p = profile_names; *p; p++)
            Printf("%s\n", *p);
        rc = RETURN_OK;
        goto done;
    }

    OOPBase = OpenLibrary("oop.library", 0);
    ControllerHiddBase = OpenLibrary("controller.hidd", 0);
    LowLevelBase = OpenLibrary("lowlevel.library", 40);    /* optional: only the jp/jpa/sja commands need it */
    VirtualPadBase = OpenLibrary("PROGDIR:virtualpad.hidd", 0);
    if (!VirtualPadBase)
        VirtualPadBase = OpenLibrary("virtualpad.hidd", 0);
    if (!OOPBase || !ControllerHiddBase || !VirtualPadBase)
    {
        Printf("ControllerSim: cannot open %s\n", !OOPBase ? "oop.library" : !ControllerHiddBase ? "controller.hidd" : "virtualpad.hidd");
        goto done;
    }
    if (!OOP_ObtainAttrBases(attrbases))
    {
        Printf("ControllerSim: cannot obtain attribute bases\n");
        goto done;
    }

    s.hw = OOP_NewObject(NULL, CLID_HW_Controller, NULL);
    if (!s.hw)
    {
        Printf("ControllerSim: no controller subsystem\n");
        goto release;
    }

    if (args[A_LIST])
    {
        list_devices(&s);
        rc = RETURN_OK;
        goto release;
    }

    if (args[A_REMOVE])
    {
        OOP_Object *dev;
        if (!args[A_ID]) { Printf("REMOVE needs ID\n"); goto release; }
        dev = HW_Controller_FindDevice(s.hw, *(LONG *)args[A_ID]);
        if (!dev) { Printf("no device with id %ld\n", *(LONG *)args[A_ID]); goto release; }
        if (HW_RemoveDriver(s.hw, dev))
        {
            Printf("device %ld removed\n", *(LONG *)args[A_ID]);
            rc = RETURN_OK;
        }
        goto release;
    }

    if (args[A_PROFILE])
    {
        OOP_Class *cls = OOP_FindClass(CLID_Hidd_VirtualPad);
        struct TagItem tags[] =
        {
            { aHidd_VirtualPad_Profile, args[A_PROFILE] },
            { TAG_DONE, 0 }
        };
        IPTR id = 0;

        if (!cls) { Printf("virtualpad class not found\n"); goto release; }
        s.dev = HW_AddDriver(s.hw, cls, tags);
        if (!s.dev) { Printf("failed to create profile '%s'\n", (CONST_STRPTR)args[A_PROFILE]); goto release; }
        OOP_GetAttr(s.dev, aHidd_Controller_DeviceID, &id);
        s.device_id = id;
        Printf("created '%s' as device %lu\n", (CONST_STRPTR)args[A_PROFILE], id);
        print_device(s.dev);
        if (args[A_SLOT])
            HW_Controller_AssignSlot(s.hw, *(LONG *)args[A_SLOT], id);
        rc = RETURN_OK;
    }
    else if (args[A_ID])
    {
        s.dev = HW_Controller_FindDevice(s.hw, *(LONG *)args[A_ID]);
        if (!s.dev) { Printf("no device with id %ld\n", *(LONG *)args[A_ID]); goto release; }
        s.device_id = *(LONG *)args[A_ID];
        if (args[A_SLOT])
            HW_Controller_AssignSlot(s.hw, *(LONG *)args[A_SLOT], s.device_id);
        rc = RETURN_OK;
    }

    if (args[A_SCRIPT] && s.dev)
    {
        s.sigbit = AllocSignal(-1);
        if (s.sigbit < 0 || !open_consumer(&s))
        {
            Printf("cannot create consumer\n");
            rc = RETURN_FAIL;
            goto release;
        }
        rc = run_script(&s, (CONST_STRPTR)args[A_SCRIPT]) ? RETURN_OK : RETURN_ERROR;
        OOP_DisposeObject(s.consumer);
        s.consumer = NULL;
        if (s.port)
        {
            struct Message *m;
            while ((m = GetMsg(s.port)))
                ReplyMsg(m);
            DeleteMsgPort(s.port);
            s.port = NULL;
        }
    }

release:
    if (s.sigbit >= 0) FreeSignal(s.sigbit);
    OOP_ReleaseAttrBases(attrbases);
done:
    if (VirtualPadBase) CloseLibrary(VirtualPadBase);
    if (ControllerHiddBase) CloseLibrary(ControllerHiddBase);
    if (LowLevelBase) CloseLibrary(LowLevelBase);
    if (OOPBase) CloseLibrary(OOPBase);
    FreeArgs(rda);
    return rc;
}
