/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: ControllerInfo - lists the game controllers known to controller.hidd

    Usage:
        ControllerInfo                  list all devices
        ControllerInfo ID=<n>           details of one device (controls, outputs, bindings)
        ControllerInfo ID=<n> LIVE      print readings until Ctrl-C
        ControllerInfo WATCH            print subsystem events (hotplug etc.) until Ctrl-C
*/

#include <exec/types.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include <oop/oop.h>
#include <hidd/hidd.h>
#include <hidd/controller.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/oop.h>
#include <aros/debug.h>
#include <exec/rawfmt.h>

const char version[] = "$VER: ControllerInfo 1.0 (26.9.2026)";

OOP_AttrBase HiddAttrBase;
OOP_AttrBase HWAttrBase;
OOP_AttrBase HiddControllerAB;
OOP_AttrBase HWControllerAB;

struct Library *OOPBase;

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
    { NULL,                 NULL                }
};

static const char *bus_names[]    = { "unknown", "USB", "Bluetooth", "Amiga port", "GPIO", "virtual", "hosted", "internal" };
static const char *type_names[]   = { "unknown", "gamepad", "joystick", "wheel", "arcade stick", "flight stick", "dance pad", "guitar", "drum kit", "throttle", "other" };
static const char *family_names[] = { "unknown", "HID", "proprietary", "native", "virtual", "hosted" };
static const char *kind_names[]   = { "button", "axis", "hat", "touchpad", "sensor" };
static const char *out_names[]    = { "rumble low", "rumble high", "rumble left trigger", "rumble right trigger", "LED RGB", "LED player", "LED mono", "FF motor", "raw effect", "trigger effect" };
static const char *gp_names[] =
{
    "south", "east", "west", "north", "back", "guide", "start", "leftstick", "rightstick",
    "leftshoulder", "rightshoulder", "dpup", "dpdown", "dpleft", "dpright", "misc1",
    "paddle1", "paddle2", "paddle3", "paddle4", "touchpad", "misc2", "misc3", "misc4", "misc5", "misc6"
};
static const char *gpa_names[]  = { "leftx", "lefty", "rightx", "righty", "lefttrigger", "righttrigger" };
static const char *arch_names[] = { "wheel", "throttle", "brake", "clutch", "handbrake", "yaw", "pitch", "roll" };
static const char *event_names[] =
{
    "press", "release", "axis", "hat", "frame", "added", "removed", "remapped", "power",
    "touchdown", "touchmotion", "touchup", "sensor", "connection", "port", "overflow"
};

static const char *name_of(const char **names, ULONG count, IPTR v)
{
    return (v < count) ? names[v] : "?";
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

static void print_summary(OOP_Object *dev)
{
    IPTR id = 0, name = 0, hwname = 0, vendor = 0, product = 0, port = 0, caps = 0, nb = 0, na = 0, nh = 0, guid = 0;
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

    Printf("%3lu  %-30s %04lx:%04lx  port %2ld  %2lu buttons %2lu axes %lu hats  %s\n",
           id, hwname ? (CONST_STRPTR)hwname : (name ? (CONST_STRPTR)name : (CONST_STRPTR)"?"),
           vendor, product, (LONG)(WORD)port, nb, na, nh, g);
}

static void print_details(OOP_Object *dev)
{
    IPTR v = 0, manufacturer = 0, serial = 0, path = 0, caps = 0;
    IPTR nb = 0, na = 0, nh = 0, nt = 0, ns = 0, no = 0;
    ULONG i;
    struct Hidd_Controller_ControlDesc cd;
    struct Hidd_Controller_OutputDesc od;
    struct Hidd_Controller_Binding bind[64];
    ULONG nbind;

    print_summary(dev);
    OOP_GetAttr(dev, aHidd_Controller_Manufacturer, &manufacturer);
    OOP_GetAttr(dev, aHidd_Controller_Serial, &serial);
    OOP_GetAttr(dev, aHidd_Controller_Path, &path);
    Printf("     manufacturer '%s' serial '%s' path '%s'\n",
           manufacturer ? (CONST_STRPTR)manufacturer : (CONST_STRPTR)"", serial ? (CONST_STRPTR)serial : (CONST_STRPTR)"",
           path ? (CONST_STRPTR)path : (CONST_STRPTR)"");
    OOP_GetAttr(dev, aHidd_Controller_Bus, &v);        Printf("     bus %s", name_of(bus_names, 8, v));
    OOP_GetAttr(dev, aHidd_Controller_Family, &v);     Printf(", family %s", name_of(family_names, 6, v));
    OOP_GetAttr(dev, aHidd_Controller_Type, &v);       Printf(", type %s", name_of(type_names, 11, v));
    OOP_GetAttr(dev, aHidd_Controller_Version, &v);    Printf(", version %04lx", v);
    OOP_GetAttr(dev, aHidd_Controller_PlayerIndex, &v);Printf(", player %ld", (LONG)(WORD)v);
    OOP_GetAttr(dev, aHidd_Controller_Connected, &v);  Printf(", %s\n", v ? "connected" : "disconnected");
    OOP_GetAttr(dev, aHidd_Controller_Capabilities, &caps);
    Printf("     capabilities %08lx:%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s\n", caps,
           (caps & vHidd_Controller_Cap_Buttons) ? " buttons" : "",
           (caps & vHidd_Controller_Cap_Axes) ? " axes" : "",
           (caps & vHidd_Controller_Cap_Hats) ? " hats" : "",
           (caps & vHidd_Controller_Cap_Triggers) ? " triggers" : "",
           (caps & vHidd_Controller_Cap_Touchpad) ? " touchpad" : "",
           (caps & vHidd_Controller_Cap_Sensors) ? " sensors" : "",
           (caps & vHidd_Controller_Cap_Battery) ? " battery" : "",
           (caps & vHidd_Controller_Cap_Rumble) ? " rumble" : "",
           (caps & vHidd_Controller_Cap_TriggerRumble) ? " trigger-rumble" : "",
           (caps & vHidd_Controller_Cap_LEDRGB) ? " led-rgb" : "",
           (caps & vHidd_Controller_Cap_LEDPlayer) ? " led-player" : "",
           (caps & vHidd_Controller_Cap_LEDMono) ? " led-mono" : "",
           (caps & vHidd_Controller_Cap_Effects) ? " effects" : "",
           (caps & vHidd_Controller_Cap_RawEffect) ? " raw-effect" : "",
           (caps & vHidd_Controller_Cap_StandardMapping) ? " mapped" : "",
           (caps & vHidd_Controller_Cap_MappingSynthesised) ? " (synthesised)" : "",
           (caps & vHidd_Controller_Cap_Truncated) ? " TRUNCATED" : "");

    OOP_GetAttr(dev, aHidd_Controller_ButtonCount, &nb);
    OOP_GetAttr(dev, aHidd_Controller_AxisCount, &na);
    OOP_GetAttr(dev, aHidd_Controller_HatCount, &nh);
    OOP_GetAttr(dev, aHidd_Controller_TouchpadCount, &nt);
    OOP_GetAttr(dev, aHidd_Controller_SensorCount, &ns);
    OOP_GetAttr(dev, aHidd_Controller_OutputCount, &no);

    {
        struct { UBYTE kind; IPTR count; } kinds[] =
        {
            { vHidd_Controller_Ctl_Button, nb }, { vHidd_Controller_Ctl_Axis, na }, { vHidd_Controller_Ctl_Hat, nh },
            { vHidd_Controller_Ctl_Touchpad, nt }, { vHidd_Controller_Ctl_Sensor, ns }
        };
        ULONG k;
        for (k = 0; k < 5; k++)
        {
            for (i = 0; i < kinds[k].count; i++)
            {
                if (!HIDD_Controller_GetControlInfo(dev, kinds[k].kind, i, &cd))
                    continue;
                Printf("     %-8s %2lu  usage %02lx:%02lx  range %ld..%ld  flat %lu fuzz %lu  flags %04lx  label %lu  %s\n",
                       kind_names[kinds[k].kind], i, (ULONG)cd.usage_page, (ULONG)cd.usage, cd.min, cd.max,
                       cd.flat, cd.fuzz, (ULONG)cd.flags, (ULONG)cd.label, cd.name ? cd.name : (CONST_STRPTR)"");
            }
        }
    }
    for (i = 0; i < no; i++)
    {
        if (!HIDD_Controller_GetOutputInfo(dev, i, &od))
            continue;
        Printf("     output %2lu  %-20s index %lu caps %08lx max %ld  %s\n", i, name_of(out_names, 10, od.kind),
               (ULONG)od.index, od.caps, od.max, od.name ? od.name : (CONST_STRPTR)"");
    }

    nbind = HIDD_Controller_GetBindings(dev, bind, 64);
    if (nbind)
    {
        Printf("     bindings (%lu):\n", nbind);
        for (i = 0; i < nbind; i++)
        {
            const char *outname;
            UWORD std = bind[i].out;
            if (vHidd_Controller_Std_IsButton(std))     outname = name_of(gp_names, vHidd_Controller_GP_ButtonCount, std);
            else if (vHidd_Controller_Std_IsAxis(std))  outname = name_of(gpa_names, vHidd_Controller_GPA_AxisCount, std & 0xFF);
            else                                        outname = name_of(arch_names, vHidd_Controller_ARCH_AxisCount, std & 0xFF);
            Printf("       %-14s <- %s %lu", outname, kind_names[bind[i].in_kind], (ULONG)bind[i].in_index);
            if (bind[i].in_kind == vHidd_Controller_Ctl_Hat)  Printf(" mask %lu", (ULONG)bind[i].hat_mask);
            if (bind[i].in_kind == vHidd_Controller_Ctl_Axis) Printf(" [%ld..%ld]", (LONG)bind[i].in_min, (LONG)bind[i].in_max);
            Printf("  label %lu\n", (ULONG)HIDD_Controller_GetLabel(dev, std));
        }
    }
}

static void live(OOP_Object *dev)
{
    struct pHidd_Controller_Reading rd;
    ULONG last = 0xFFFFFFFF;

    Printf("polling, Ctrl-C to stop\n");
    for (;;)
    {
        UWORD i;

        if (SetSignal(0, 0) & SIGBREAKF_CTRL_C)
        {
            SetSignal(0, SIGBREAKF_CTRL_C);
            break;
        }
        rd.size = sizeof(rd);
        HIDD_Controller_GetReading(dev, &rd);
        if (rd.sequence != last)
        {
            last = rd.sequence;
            Printf("seq %6lu  btn %08lx%08lx  hats", rd.sequence, rd.buttons[1], rd.buttons[0]);
            for (i = 0; i < rd.hat_count; i++) Printf(" %lu", (ULONG)rd.hats[i]);
            Printf("  axes");
            for (i = 0; i < rd.axis_count; i++) Printf(" %6ld", (LONG)rd.axes[i]);
            if (rd.flags & vHidd_Controller_RF_Mapped)
            {
                Printf("  | gp %08lx", rd.gp_buttons);
                for (i = 0; i < vHidd_Controller_GPA_AxisCount; i++) Printf(" %6ld", (LONG)rd.gp_axes[i]);
            }
            Printf("\n");
        }
        Delay(2);
    }
}

static void watch(OOP_Object *hw)
{
    BYTE sigbit = AllocSignal(-1);
    OOP_Object *con;
    struct TagItem tags[] =
    {
        { aHidd_Controller_NotifyTask,   (IPTR)FindTask(NULL) },
        { aHidd_Controller_NotifySignal, sigbit               },
        { aHidd_Controller_EventMask,    vHidd_Controller_EventMask(vHidd_Controller_DeviceAdded) |
                                         vHidd_Controller_EventMask(vHidd_Controller_DeviceRemoved) |
                                         vHidd_Controller_EventMask(vHidd_Controller_Remapped) |
                                         vHidd_Controller_EventMask(vHidd_Controller_PortChanged) |
                                         vHidd_Controller_EventMask(vHidd_Controller_PowerChanged) |
                                         vHidd_Controller_EventMask(vHidd_Controller_ConnectionChanged) |
                                         vHidd_Controller_EventMask(vHidd_Controller_Press) |
                                         vHidd_Controller_EventMask(vHidd_Controller_Release) },
        { TAG_DONE, 0 }
    };

    if (sigbit < 0)
        return;
    con = OOP_NewObject(NULL, CLID_Hidd_Controller, tags);
    if (!con)
    {
        FreeSignal(sigbit);
        Printf("cannot create consumer\n");
        return;
    }
    Printf("watching subsystem events, Ctrl-C to stop\n");
    for (;;)
    {
        struct pHidd_Controller_Event ev;
        ULONG sigs = Wait((1UL << sigbit) | SIGBREAKF_CTRL_C);

        if (sigs & SIGBREAKF_CTRL_C)
            break;
        while (HIDD_Controller_GetEvent(con, &ev))
        {
            Printf("event %-11s device %lu code %lu std 0x%03lx value %ld seq %lu\n",
                   name_of(event_names, vHidd_Controller_EventTypeCount, ev.type),
                   (ULONG)ev.device_id, (ULONG)ev.code, (ULONG)ev.std, ev.value, ev.sequence);
        }
    }
    OOP_DisposeObject(con);
    FreeSignal(sigbit);
}

int main(void)
{
    IPTR args[4] = { 0 };
    struct RDArgs *rda;
    struct Library *ControllerHiddBase = NULL;
    OOP_Object *hw;
    int rc = RETURN_FAIL;

    rda = ReadArgs("ID/N,LIVE/S,WATCH/S,SERIAL/S", args, NULL);
    if (!rda)
    {
        PrintFault(IoErr(), "ControllerInfo");
        return RETURN_FAIL;
    }

    g_serial = args[3] ? TRUE : FALSE;
    OOPBase = OpenLibrary("oop.library", 0);
    ControllerHiddBase = OpenLibrary("controller.hidd", 0);
    if (!OOPBase || !ControllerHiddBase)
    {
        Printf("ControllerInfo: cannot open %s\n", OOPBase ? "controller.hidd" : "oop.library");
        goto done;
    }
    if (!OOP_ObtainAttrBases(attrbases))
        goto done;

    hw = OOP_NewObject(NULL, CLID_HW_Controller, NULL);
    if (!hw)
    {
        Printf("no controller subsystem\n");
        goto release;
    }

    {
        IPTR n = 0, freq = 0, ver = 0;
        OOP_GetAttr(hw, aHW_Controller_DeviceCount, &n);
        OOP_GetAttr(hw, aHW_Controller_ClockFrequency, &freq);
        OOP_GetAttr(hw, aHW_Controller_Version, &ver);
        Printf("controller subsystem API %lu, %lu device(s), clock %lu Hz\n", ver, n, freq);
    }

    if (args[2])
    {
        watch(hw);
        rc = RETURN_OK;
    }
    else if (args[0])
    {
        OOP_Object *dev = HW_Controller_FindDevice(hw, *(LONG *)args[0]);
        if (!dev)
        {
            Printf("no device with id %ld\n", *(LONG *)args[0]);
            goto release;
        }
        print_details(dev);
        if (args[1])
            live(dev);
        rc = RETURN_OK;
    }
    else
    {
        UWORD ids[64];
        ULONG n = HW_Controller_GetDeviceIDs(hw, ids, 64), i;
        for (i = 0; i < n && i < 64; i++)
        {
            OOP_Object *dev = HW_Controller_FindDevice(hw, ids[i]);
            if (dev)
                print_summary(dev);
        }
        rc = RETURN_OK;
    }

release:
    OOP_ReleaseAttrBases(attrbases);
done:
    if (ControllerHiddBase) CloseLibrary(ControllerHiddBase);
    if (OOPBase) CloseLibrary(OOPBase);
    FreeArgs(rda);
    return rc;
}
