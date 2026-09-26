/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: GUIDs, the standard-layout mapping database (SDL gamecontrollerdb
          format), binding evaluation and button labels
*/

#define DEBUG 0
#include <aros/debug.h>

#include <exec/memory.h>
#include <proto/exec.h>
#include <hidd/controller.h>

#include "controller_intern.h"

#define SysBase ((struct ExecBase *)(CSD(cl)->cs_SysBase))

/*****************************************************************************************
    small string helpers (no C library in this module)
*****************************************************************************************/

static ULONG c_strlen(const char *s)
{
    ULONG n = 0;
    while (s && s[n]) n++;
    return n;
}

static STRPTR c_strdup(OOP_Class *cl, const char *s, ULONG n)
{
    STRPTR d = AllocVec(n + 1, MEMF_PUBLIC);
    if (d)
    {
        CopyMem((APTR)s, d, n);
        d[n] = 0;
    }
    return d;
}

static int c_hexval(char c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

/*****************************************************************************************
    GUID (SDL compatible: bus, crc16(name), vendor, 0, product, 0, version, 0; little endian)
*****************************************************************************************/

/* CRC-16 with the reflected 0x8005 polynomial (0xA001), init 0, as used by SDL_crc16() */
static UWORD c_crc16(UWORD crc, const char *data, ULONG len)
{
    ULONG i;
    int b;

    for (i = 0; i < len; i++)
    {
        crc ^= (UBYTE)data[i];
        for (b = 0; b < 8; b++)
            crc = (crc & 1) ? (crc >> 1) ^ 0xA001 : (crc >> 1);
    }
    return crc;
}

static void c_put16le(UBYTE *p, UWORD v)
{
    p[0] = v & 0xFF;
    p[1] = v >> 8;
}

void ctrl_ComputeGUID(struct ControllerDevice *dev)
{
    UWORD bus, crc = 0;
    UWORD i;

    for (i = 0; i < 16; i++)
        dev->guid[i] = 0;

    switch (dev->bus)
    {
    case vHidd_Controller_Bus_USB:       bus = 0x03; break;
    case vHidd_Controller_Bus_Bluetooth: bus = 0x05; break;
    case vHidd_Controller_Bus_Virtual:   bus = 0x06; break;
    default:                             bus = 0x00; break;
    }

    if (dev->manufacturer && dev->manufacturer[0] && dev->hwname && dev->hwname[0])
    {
        crc = c_crc16(crc, dev->manufacturer, c_strlen(dev->manufacturer));
        crc = c_crc16(crc, " ", 1);
        crc = c_crc16(crc, dev->hwname, c_strlen(dev->hwname));
    }
    else if (dev->hwname)
    {
        crc = c_crc16(crc, dev->hwname, c_strlen(dev->hwname));
    }

    c_put16le(&dev->guid[0], bus);
    c_put16le(&dev->guid[2], crc);
    if (dev->vendor)
    {
        c_put16le(&dev->guid[4], dev->vendor);
        c_put16le(&dev->guid[8], dev->product);
        c_put16le(&dev->guid[12], dev->version);
    }
    else if (dev->hwname)
    {
        ULONG n = c_strlen(dev->hwname), k;
        if (n > 11) n = 11;
        for (k = 0; k < n; k++)
            dev->guid[4 + k] = (UBYTE)dev->hwname[k];
    }
}

BOOL ctrl_GUIDFromString(const char *s, UBYTE *guid)
{
    UWORD i;

    for (i = 0; i < 16; i++)
    {
        int hi = c_hexval(s[i * 2]), lo = c_hexval(s[i * 2 + 1]);
        if (hi < 0 || lo < 0)
            return FALSE;
        guid[i] = (UBYTE)((hi << 4) | lo);
    }
    return TRUE;
}

void ctrl_GUIDToString(const UBYTE *guid, char *s)
{
    static const char hex[] = "0123456789abcdef";
    UWORD i;

    for (i = 0; i < 16; i++)
    {
        s[i * 2]     = hex[guid[i] >> 4];
        s[i * 2 + 1] = hex[guid[i] & 15];
    }
    s[32] = 0;
}

/*****************************************************************************************
    bindings
*****************************************************************************************/

/* std id -> slot in dev->bind[] */
static LONG ctrl_StdSlot(UWORD std)
{
    if (vHidd_Controller_Std_IsButton(std) && std < vHidd_Controller_GP_ButtonCount)
        return std;
    if (vHidd_Controller_Std_IsAxis(std) && (std & 0xFF) < vHidd_Controller_GPA_AxisCount)
        return vHidd_Controller_GP_ButtonCount + (std & 0xFF);
    if (vHidd_Controller_Std_IsArch(std) && (std & 0xFF) < vHidd_Controller_ARCH_AxisCount)
        return vHidd_Controller_GP_ButtonCount + vHidd_Controller_GPA_AxisCount + (std & 0xFF);
    return -1;
}

static UWORD ctrl_SlotStd(LONG slot)
{
    if (slot < vHidd_Controller_GP_ButtonCount)
        return vHidd_Controller_Std_Button(slot);
    slot -= vHidd_Controller_GP_ButtonCount;
    if (slot < vHidd_Controller_GPA_AxisCount)
        return vHidd_Controller_Std_Axis(slot);
    slot -= vHidd_Controller_GPA_AxisCount;
    return vHidd_Controller_Std_Arch(slot);
}

static BOOL ctrl_StdIsUnipolar(UWORD std)
{
    if (vHidd_Controller_Std_IsAxis(std))
        return (std & 0xFF) >= vHidd_Controller_GPA_LeftTrigger;
    if (vHidd_Controller_Std_IsArch(std))
    {
        UWORD a = std & 0xFF;
        return a == vHidd_Controller_ARCH_Throttle || a == vHidd_Controller_ARCH_Brake ||
               a == vHidd_Controller_ARCH_Clutch || a == vHidd_Controller_ARCH_Handbrake;
    }
    return FALSE;
}

static ULONG ctrl_BindingCount(const struct Hidd_Controller_Binding *b)
{
    ULONG n = 0;
    while (b && b[n].out != vHidd_Controller_Std_None)
        n++;
    return n;
}

/* Fill dev->bind[] and the reverse lookups from a terminated binding array */
static void ctrl_ApplyBindings(struct ControllerDevice *dev, const struct Hidd_Controller_Binding *table)
{
    LONG i;
    UWORD h, d;

    for (i = 0; i < CTRL_NUM_STD; i++)
    {
        dev->bind[i].flags = 0;
        dev->bind[i].out = ctrl_SlotStd(i);
    }
    for (i = 0; i < HIDD_CONTROLLER_MAX_BUTTONS; i++) dev->raw_btn_std[i] = vHidd_Controller_Std_None;
    for (i = 0; i < HIDD_CONTROLLER_MAX_AXES; i++)    dev->raw_axis_std[i] = vHidd_Controller_Std_None;
    for (h = 0; h < HIDD_CONTROLLER_MAX_HATS; h++)
        for (d = 0; d < 4; d++)
            dev->raw_hat_std[h][d] = vHidd_Controller_Std_None;

    for (; table && table->out != vHidd_Controller_Std_None; table++)
    {
        LONG slot = ctrl_StdSlot(table->out);
        if (slot >= 0)
        {
            dev->bind[slot] = *table;
            dev->bind[slot].flags |= vHidd_Controller_BF_Valid;
            if (dev->bind[slot].in_kind == vHidd_Controller_Ctl_Axis)
            {
                struct Hidd_Controller_Binding *nb = &dev->bind[slot];
                BOOL unipolar = nb->in_index < dev->naxes &&
                                (dev->axes[nb->in_index].flags & vHidd_Controller_CF_Unipolar);

                if (nb->in_min == 0 && nb->in_max == 0)
                {
                    nb->in_min = HIDD_CONTROLLER_AXIS_MIN;
                    nb->in_max = HIDD_CONTROLLER_AXIS_MAX;
                }
                /*
                 * A unipolar raw axis is normalised to 0..32767. A binding that
                 * still names the full bipolar range (the usual notation for
                 * triggers in mapping records) is narrowed to the real range.
                 */
                if (unipolar && nb->in_min == HIDD_CONTROLLER_AXIS_MIN && nb->in_max == HIDD_CONTROLLER_AXIS_MAX)
                    nb->in_min = 0;
                else if (unipolar && nb->in_min == HIDD_CONTROLLER_AXIS_MAX && nb->in_max == HIDD_CONTROLLER_AXIS_MIN)
                    nb->in_max = 0;
            }
        }
    }

    /* reverse lookups: first std element fed by each raw control */
    for (i = 0; i < CTRL_NUM_STD; i++)
    {
        struct Hidd_Controller_Binding *b = &dev->bind[i];
        if (!(b->flags & vHidd_Controller_BF_Valid))
            continue;
        switch (b->in_kind)
        {
        case vHidd_Controller_Ctl_Button:
            if (b->in_index < HIDD_CONTROLLER_MAX_BUTTONS && dev->raw_btn_std[b->in_index] == vHidd_Controller_Std_None)
                dev->raw_btn_std[b->in_index] = b->out;
            break;
        case vHidd_Controller_Ctl_Axis:
            if (b->in_index < HIDD_CONTROLLER_MAX_AXES && dev->raw_axis_std[b->in_index] == vHidd_Controller_Std_None)
                dev->raw_axis_std[b->in_index] = b->out;
            break;
        case vHidd_Controller_Ctl_Hat:
            if (b->in_index < HIDD_CONTROLLER_MAX_HATS)
                for (d = 0; d < 4; d++)
                    if ((b->hat_mask & (1 << d)) && dev->raw_hat_std[b->in_index][d] == vHidd_Controller_Std_None)
                        dev->raw_hat_std[b->in_index][d] = b->out;
            break;
        }
    }
}

/*****************************************************************************************
    standard view evaluation (called with the device seqlock held for writing)
*****************************************************************************************/

static BOOL ctrl_InputPressed(const struct ControllerDevice *dev, const struct Hidd_Controller_Binding *b)
{
    switch (b->in_kind)
    {
    case vHidd_Controller_Ctl_Button:
        if (b->in_index >= dev->nbuttons) return FALSE;
        return (dev->rd.buttons[b->in_index >> 5] >> (b->in_index & 31)) & 1;
    case vHidd_Controller_Ctl_Hat:
        if (b->in_index >= dev->nhats) return FALSE;
        return (dev->rd.hats[b->in_index] & b->hat_mask) != 0;
    case vHidd_Controller_Ctl_Axis:
    {
        LONG v, lo, hi, mid;
        if (b->in_index >= dev->naxes) return FALSE;
        v = dev->rd.axes[b->in_index];
        if (b->in_min <= b->in_max) { lo = b->in_min; hi = b->in_max; }
        else                        { lo = b->in_max; hi = b->in_min; }
        if (v < lo || v > hi)
            return FALSE;
        mid = b->in_min + (b->in_max - b->in_min) / 2;
        return (b->in_min <= b->in_max) ? (v >= mid) : (v <= mid);
    }
    }
    return FALSE;
}

static WORD ctrl_InputAxis(const struct ControllerDevice *dev, const struct Hidd_Controller_Binding *b,
                           WORD out_min, WORD out_max)
{
    LONG v;

    switch (b->in_kind)
    {
    case vHidd_Controller_Ctl_Button:
    case vHidd_Controller_Ctl_Hat:
        return ctrl_InputPressed(dev, b) ? out_max : out_min;

    case vHidd_Controller_Ctl_Axis:
    {
        LONG lo, hi, in_range;
        if (b->in_index >= dev->naxes) return out_min;
        v = dev->rd.axes[b->in_index];
        if (b->in_min <= b->in_max) { lo = b->in_min; hi = b->in_max; }
        else                        { lo = b->in_max; hi = b->in_min; }
        if (v < lo) v = lo;
        if (v > hi) v = hi;
        in_range = (LONG)b->in_max - (LONG)b->in_min;
        if (in_range == 0)
            return out_min;
        /* 64 bit intermediate: the products exceed 32 bits for full range bindings */
        v = (LONG)((QUAD)out_min + (((QUAD)out_max - (QUAD)out_min) * (QUAD)(v - (LONG)b->in_min)) / (QUAD)in_range);
        if (v < HIDD_CONTROLLER_AXIS_MIN) v = HIDD_CONTROLLER_AXIS_MIN;
        if (v > HIDD_CONTROLLER_AXIS_MAX) v = HIDD_CONTROLLER_AXIS_MAX;
        return (WORD)v;
    }
    }
    return out_min;
}

void ctrl_ComputeStandardView(struct ControllerDevice *dev)
{
    LONG i;

    if (!(dev->flags & CTRL_DF_MAPPED))
        return;

    dev->rd.gp_buttons = 0;
    for (i = 0; i < vHidd_Controller_GP_ButtonCount; i++)
    {
        const struct Hidd_Controller_Binding *b = &dev->bind[i];
        if ((b->flags & vHidd_Controller_BF_Valid) && ctrl_InputPressed(dev, b))
            dev->rd.gp_buttons |= vHidd_Controller_GPF(i);
    }

    for (i = vHidd_Controller_GP_ButtonCount; i < CTRL_NUM_STD; i++)
    {
        const struct Hidd_Controller_Binding *b = &dev->bind[i];
        UWORD std = ctrl_SlotStd(i);
        WORD out_min, out_max, v;

        if (ctrl_StdIsUnipolar(std)) { out_min = 0; out_max = HIDD_CONTROLLER_TRIGGER_MAX; }
        else                         { out_min = HIDD_CONTROLLER_AXIS_MIN; out_max = HIDD_CONTROLLER_AXIS_MAX; }
        if (b->flags & vHidd_Controller_BF_OutPos) { out_min = 0; out_max = HIDD_CONTROLLER_AXIS_MAX; }
        if (b->flags & vHidd_Controller_BF_OutNeg) { out_min = 0; out_max = HIDD_CONTROLLER_AXIS_MIN; }

        v = (b->flags & vHidd_Controller_BF_Valid) ? ctrl_InputAxis(dev, b, out_min, out_max) : 0;
        if (vHidd_Controller_Std_IsAxis(std))
            dev->rd.gp_axes[std & 0xFF] = v;
        else
            dev->rd.arch_axes[std & 0xFF] = v;
    }
}

/*****************************************************************************************
    mapping database (structured records)
*****************************************************************************************/

#define B_BTN(std, i)         { vHidd_Controller_Ctl_Button, (i), 0, 0, 0, 0, (std), 0 }
#define B_HAT(std, i, m)      { vHidd_Controller_Ctl_Hat, (i), (m), 0, 0, 0, (std), 0 }
#define B_AXIS(std, i)        { vHidd_Controller_Ctl_Axis, (i), 0, 0, HIDD_CONTROLLER_AXIS_MIN, HIDD_CONTROLLER_AXIS_MAX, (std), 0 }
#define B_END                 { 0, 0, 0, 0, 0, 0, vHidd_Controller_Std_None, 0 }
#define GP(n)   vHidd_Controller_Std_Button(vHidd_Controller_GP_##n)
#define GPA(n)  vHidd_Controller_Std_Axis(vHidd_Controller_GPA_##n)

/* XInput family devices share one layout regardless of GUID (XINPUT_GAMEPAD order) */
static const struct Hidd_Controller_Binding ctrl_xinput_bindings[] =
{
    B_BTN(GP(South), 0), B_BTN(GP(East), 1), B_BTN(GP(West), 2), B_BTN(GP(North), 3),
    B_BTN(GP(LeftShoulder), 4), B_BTN(GP(RightShoulder), 5),
    B_BTN(GP(Back), 6), B_BTN(GP(Start), 7),
    B_BTN(GP(LeftStick), 8), B_BTN(GP(RightStick), 9), B_BTN(GP(Guide), 10),
    B_BTN(GP(DpadUp), 11), B_BTN(GP(DpadDown), 12), B_BTN(GP(DpadLeft), 13), B_BTN(GP(DpadRight), 14),
    B_AXIS(GPA(LeftX), 0), B_AXIS(GPA(LeftY), 1), B_AXIS(GPA(RightX), 2), B_AXIS(GPA(RightY), 3),
    B_AXIS(GPA(LeftTrigger), 4), B_AXIS(GPA(RightTrigger), 5),
    B_END
};

/* Built-in records, matched by GUID */
static const struct Hidd_Controller_MappingDesc ctrl_builtin_mappings[] =
{
    { { 0 }, NULL, NULL }
};

static void ctrl_MappingFree(OOP_Class *cl, struct ControllerMapping *m)
{
    if (!(m->flags & CTRL_MF_STATIC))
    {
        if (m->name) FreeVec(m->name);
        if (m->bindings) FreeVec(m->bindings);
    }
    FreeVec(m);
}

static struct ControllerMapping *ctrl_MappingCopy(OOP_Class *cl, const struct Hidd_Controller_MappingDesc *desc, UBYTE source)
{
    struct ControllerMapping *m;
    ULONG n;

    if (!desc || !desc->bindings)
        return NULL;
    m = AllocVec(sizeof(*m), MEMF_PUBLIC | MEMF_CLEAR);
    if (!m)
        return NULL;
    CopyMem((APTR)desc->guid, m->guid, 16);
    m->source = source;
    n = ctrl_BindingCount(desc->bindings);
    m->bindings = AllocVec((n + 1) * sizeof(struct Hidd_Controller_Binding), MEMF_PUBLIC);
    if (!m->bindings)
    {
        FreeVec(m);
        return NULL;
    }
    CopyMem((APTR)desc->bindings, m->bindings, (n + 1) * sizeof(struct Hidd_Controller_Binding));
    m->nbindings = n;
    if (desc->name)
        m->name = c_strdup(cl, desc->name, c_strlen(desc->name));
    return m;
}

void ctrl_MappingInitDB(OOP_Class *cl, struct ControllerHWData *hw)
{
    const struct Hidd_Controller_MappingDesc *t;

    NEWLIST(&hw->mappings);
    InitSemaphore(&hw->maplock);
    for (t = ctrl_builtin_mappings; t->bindings; t++)
        ctrl_MappingAdd(cl, hw, t, vHidd_Controller_MapSrc_BuiltIn);
}

void ctrl_MappingFreeDB(OOP_Class *cl, struct ControllerHWData *hw)
{
    struct ControllerMapping *m;

    ObtainSemaphore(&hw->maplock);
    while ((m = (struct ControllerMapping *)REMHEAD(&hw->mappings)))
        ctrl_MappingFree(cl, m);
    ReleaseSemaphore(&hw->maplock);
}

static BOOL ctrl_GUIDEqual(const UBYTE *a, const UBYTE *b)
{
    UWORD i;
    for (i = 0; i < 16; i++)
        if (a[i] != b[i]) return FALSE;
    return TRUE;
}

BOOL ctrl_MappingAdd(OOP_Class *cl, struct ControllerHWData *hw, const struct Hidd_Controller_MappingDesc *desc, UBYTE source)
{
    struct ControllerMapping *m = ctrl_MappingCopy(cl, desc, source);
    struct ControllerMapping *old, *tmp;

    if (!m)
        return FALSE;

    ObtainSemaphore(&hw->maplock);
    /* replace an existing record of the same GUID and source */
    ForeachNodeSafe(&hw->mappings, old, tmp)
    {
        if (ctrl_GUIDEqual(old->guid, m->guid) && old->source == m->source)
        {
            REMOVE(&old->node);
            ctrl_MappingFree(cl, old);
        }
    }
    ADDHEAD(&hw->mappings, &m->node);
    ReleaseSemaphore(&hw->maplock);
    return TRUE;
}

BOOL ctrl_MappingRemove(OOP_Class *cl, struct ControllerHWData *hw, const UBYTE *guid, UBYTE source)
{
    struct ControllerMapping *m, *tmp;
    BOOL removed = FALSE;

    ObtainSemaphore(&hw->maplock);
    ForeachNodeSafe(&hw->mappings, m, tmp)
    {
        if (ctrl_GUIDEqual(m->guid, guid) && (source == 0 || m->source == source) &&
            m->source != vHidd_Controller_MapSrc_BuiltIn)
        {
            REMOVE(&m->node);
            ctrl_MappingFree(cl, m);
            removed = TRUE;
        }
    }
    ReleaseSemaphore(&hw->maplock);
    return removed;
}

/*
 * Match quality: 3 exact, 2 ignoring the name hash, 1 ignoring hash and
 * version, 0 none. Zero hash/version bytes in a record act as wildcards.
 */
static int ctrl_MappingMatch(const struct ControllerMapping *m, const UBYTE *guid)
{
    UWORD i;
    BOOL exact = TRUE, nohash = TRUE, nover = TRUE;
    BOOL rec_hash = m->guid[2] || m->guid[3];

    for (i = 0; i < 16; i++)
    {
        if (m->guid[i] == guid[i]) continue;
        exact = FALSE;
        if (i == 2 || i == 3)
        {
            if (rec_hash) return 0;      /* record demands this name hash */
            continue;
        }
        nohash = FALSE;
        if (i == 12 || i == 13) continue;
        nover = FALSE;
    }
    if (exact) return 3;
    if (nohash) return 2;
    if (nover) return 1;
    return 0;
}

static struct ControllerMapping *ctrl_MappingLookup(struct ControllerHWData *hw, const UBYTE *guid, UBYTE minsource)
{
    struct ControllerMapping *m, *best = NULL;
    int bestq = 0;

    ForeachNode(&hw->mappings, m)
    {
        int q = ctrl_MappingMatch(m, guid);
        if (m->source < minsource || q == 0)
            continue;
        if (q > bestq || (q == bestq && best && m->source > best->source))
        {
            best = m;
            bestq = q;
        }
    }
    return best;
}

/* Synthesise bindings from HID usages, or from plain indices when there are none */
static ULONG ctrl_SynthesiseMapping(struct ControllerDevice *dev, struct Hidd_Controller_Binding *out, ULONG max)
{
    ULONG n = 0;
    UWORD i;
    BOOL have_rx = FALSE, have_ry = FALSE, have_usages = FALSE;
    static const UWORD btnstd[] = { GP(South), GP(East), GP(West), GP(North), GP(LeftShoulder), GP(RightShoulder),
                                    GP(Back), GP(Start), GP(LeftStick), GP(RightStick), GP(Guide) };
    static const UWORD axisstd[] = { GPA(LeftX), GPA(LeftY), GPA(RightX), GPA(RightY), GPA(LeftTrigger), GPA(RightTrigger) };

    for (i = 0; i < dev->naxes; i++)
    {
        if (dev->axes[i].usage_page == 0x01 && dev->axes[i].usage == 0x33) have_rx = TRUE;
        if (dev->axes[i].usage_page == 0x01 && dev->axes[i].usage == 0x34) have_ry = TRUE;
        if (dev->axes[i].usage_page) have_usages = TRUE;
    }

    for (i = 0; i < dev->naxes && n + 1 < max; i++)
    {
        const struct Hidd_Controller_ControlDesc *d = &dev->axes[i];
        UWORD std = vHidd_Controller_Std_None;

        if (have_usages)
        {
            if (d->usage_page == 0x01)
            {
                switch (d->usage)
                {
                case 0x30: std = GPA(LeftX); break;
                case 0x31: std = GPA(LeftY); break;
                case 0x33: std = GPA(RightX); break;
                case 0x34: std = GPA(RightY); break;
                case 0x32: std = (have_rx && have_ry) ? GPA(LeftTrigger)  : GPA(RightX); break;
                case 0x35:
                    if (dev->type == vHidd_Controller_Type_FlightStick)
                        std = vHidd_Controller_Std_Arch(vHidd_Controller_ARCH_Yaw);     /* twist */
                    else
                        std = (have_rx && have_ry) ? GPA(RightTrigger) : GPA(RightY);
                    break;
                case 0x36: std = vHidd_Controller_Std_Arch(vHidd_Controller_ARCH_Throttle); break;
                case 0x37:
                case 0x38: std = vHidd_Controller_Std_Arch(vHidd_Controller_ARCH_Wheel); break;
                }
            }
            else if (d->usage_page == 0x02)
            {
                switch (d->usage)
                {
                case 0xBA: std = vHidd_Controller_Std_Arch(vHidd_Controller_ARCH_Yaw); break;
                case 0xBB:
                case 0xC4: std = vHidd_Controller_Std_Arch(vHidd_Controller_ARCH_Throttle); break;
                case 0xC5: std = vHidd_Controller_Std_Arch(vHidd_Controller_ARCH_Brake); break;
                case 0xC6: std = vHidd_Controller_Std_Arch(vHidd_Controller_ARCH_Clutch); break;
                case 0xC8: std = vHidd_Controller_Std_Arch(vHidd_Controller_ARCH_Wheel); break;
                }
            }
        }
        else if (i < 6)
            std = axisstd[i];

        if (std == vHidd_Controller_Std_None)
            continue;
        out[n].in_kind = vHidd_Controller_Ctl_Axis;
        out[n].in_index = i;
        out[n].hat_mask = 0;
        out[n].flags = 0;
        if (d->flags & vHidd_Controller_CF_Unipolar)
        {
            out[n].in_min = 0;
            out[n].in_max = HIDD_CONTROLLER_TRIGGER_MAX;
        }
        else
        {
            out[n].in_min = HIDD_CONTROLLER_AXIS_MIN;
            out[n].in_max = HIDD_CONTROLLER_AXIS_MAX;
        }
        out[n].out = std;
        out[n].reserved = 0;
        n++;
    }

    if (dev->nhats && n + 4 < max)
    {
        static const UWORD dp[4] = { GP(DpadUp), GP(DpadRight), GP(DpadDown), GP(DpadLeft) };
        for (i = 0; i < 4; i++)
        {
            out[n].in_kind = vHidd_Controller_Ctl_Hat;
            out[n].in_index = 0;
            out[n].hat_mask = 1 << i;
            out[n].flags = 0;
            out[n].in_min = out[n].in_max = 0;
            out[n].out = dp[i];
            out[n].reserved = 0;
            n++;
        }
    }

    for (i = 0; i < dev->nbuttons && i < 11 && n + 1 < max; i++)
    {
        out[n].in_kind = vHidd_Controller_Ctl_Button;
        out[n].in_index = i;
        out[n].hat_mask = 0;
        out[n].flags = 0;
        out[n].in_min = out[n].in_max = 0;
        out[n].out = btnstd[i];
        out[n].reserved = 0;
        n++;
    }

    out[n].out = vHidd_Controller_Std_None;
    out[n].flags = 0;
    return n;
}

void ctrl_FreeMapping(OOP_Class *cl, struct ControllerDevice *dev)
{
    dev->flags &= ~CTRL_DF_MAPPED;
    dev->mapsource = vHidd_Controller_MapSrc_None;
}

/*
 * Choose and apply the effective mapping for a device. Priority:
 * user/API records in the database > driver supplied table > built-in records
 * > XInput family rule > synthesised.
 */
void ctrl_ApplyMapping(OOP_Class *cl, struct ControllerDevice *dev)
{
    struct ControllerHWData *hw = dev->hw;
    struct ControllerMapping *m;
    struct Hidd_Controller_Binding synth[CTRL_NUM_STD + 1];
    const struct Hidd_Controller_Binding *table = NULL;
    UBYTE source = vHidd_Controller_MapSrc_None;

    ObtainSemaphore(&hw->maplock);
    m = ctrl_MappingLookup(hw, dev->guid, vHidd_Controller_MapSrc_API);
    if (!m && !dev->drv_bindings)
        m = ctrl_MappingLookup(hw, dev->guid, vHidd_Controller_MapSrc_BuiltIn);
    if (m)
    {
        table = m->bindings;
        source = m->source;
    }
    else if (dev->drv_bindings)
    {
        table = dev->drv_bindings;
        source = vHidd_Controller_MapSrc_Driver;
    }
    else if (dev->family == vHidd_Controller_Family_XInput)
    {
        table = ctrl_xinput_bindings;
        source = vHidd_Controller_MapSrc_BuiltIn;
    }
    else if (ctrl_SynthesiseMapping(dev, synth, CTRL_NUM_STD + 1))
    {
        table = synth;
        source = vHidd_Controller_MapSrc_Synthesised;
    }

    dev->caps &= ~(vHidd_Controller_Cap_StandardMapping | vHidd_Controller_Cap_MappingSynthesised);
    Disable();
    dev->seq++;                 /* seqlock write side, see controller_reading.c */
    __sync_synchronize();
    if (table)
    {
        ctrl_ApplyBindings(dev, table);
        dev->flags |= CTRL_DF_MAPPED;
        dev->rd.flags |= vHidd_Controller_RF_Mapped;
        ctrl_ComputeStandardView(dev);
    }
    else
    {
        dev->flags &= ~CTRL_DF_MAPPED;
        dev->rd.flags &= ~vHidd_Controller_RF_Mapped;
    }
    dev->mapsource = source;
    __sync_synchronize();
    dev->seq++;
    Enable();
    ReleaseSemaphore(&hw->maplock);

    if (table)
    {
        dev->caps |= vHidd_Controller_Cap_StandardMapping;
        if (source == vHidd_Controller_MapSrc_Synthesised)
            dev->caps |= vHidd_Controller_Cap_MappingSynthesised;
    }
    D(bug("[Controller] %s: device %u mapping source %u\n", __func__, dev->id, source));
}

/* Copy the valid bindings of a device, Std_None terminated when there is room */
ULONG ctrl_CopyBindings(struct ControllerDevice *dev, struct Hidd_Controller_Binding *buf, ULONG max)
{
    ULONG n = 0;
    LONG i;

    for (i = 0; i < CTRL_NUM_STD && n < max; i++)
    {
        if (dev->bind[i].flags & vHidd_Controller_BF_Valid)
            buf[n++] = dev->bind[i];
    }
    if (n < max)
    {
        buf[n].out = vHidd_Controller_Std_None;
        buf[n].flags = 0;
    }
    return n;
}

/*****************************************************************************************
    labels
*****************************************************************************************/

static const UWORD ctrl_labels[5][vHidd_Controller_GP_ButtonCount] =
{
    /* Generic */
    { vHidd_Controller_Label_A, vHidd_Controller_Label_B, vHidd_Controller_Label_X, vHidd_Controller_Label_Y,
      vHidd_Controller_Label_Back, vHidd_Controller_Label_Guide, vHidd_Controller_Label_Start,
      vHidd_Controller_Label_L3, vHidd_Controller_Label_R3, vHidd_Controller_Label_L1, vHidd_Controller_Label_R1,
      vHidd_Controller_Label_DpadUp, vHidd_Controller_Label_DpadDown, vHidd_Controller_Label_DpadLeft, vHidd_Controller_Label_DpadRight,
      vHidd_Controller_Label_Misc, vHidd_Controller_Label_Paddle1, vHidd_Controller_Label_Paddle2,
      vHidd_Controller_Label_Paddle3, vHidd_Controller_Label_Paddle4, vHidd_Controller_Label_Touchpad,
      vHidd_Controller_Label_Misc, vHidd_Controller_Label_Misc, vHidd_Controller_Label_Misc, vHidd_Controller_Label_Misc, vHidd_Controller_Label_Misc },
    /* Xbox */
    { vHidd_Controller_Label_A, vHidd_Controller_Label_B, vHidd_Controller_Label_X, vHidd_Controller_Label_Y,
      vHidd_Controller_Label_View, vHidd_Controller_Label_Guide, vHidd_Controller_Label_Menu,
      vHidd_Controller_Label_L3, vHidd_Controller_Label_R3, vHidd_Controller_Label_LB, vHidd_Controller_Label_RB,
      vHidd_Controller_Label_DpadUp, vHidd_Controller_Label_DpadDown, vHidd_Controller_Label_DpadLeft, vHidd_Controller_Label_DpadRight,
      vHidd_Controller_Label_Share, vHidd_Controller_Label_Paddle1, vHidd_Controller_Label_Paddle2,
      vHidd_Controller_Label_Paddle3, vHidd_Controller_Label_Paddle4, vHidd_Controller_Label_Touchpad,
      vHidd_Controller_Label_Misc, vHidd_Controller_Label_Misc, vHidd_Controller_Label_Misc, vHidd_Controller_Label_Misc, vHidd_Controller_Label_Misc },
    /* PlayStation */
    { vHidd_Controller_Label_Cross, vHidd_Controller_Label_Circle, vHidd_Controller_Label_Square, vHidd_Controller_Label_Triangle,
      vHidd_Controller_Label_Share, vHidd_Controller_Label_Home, vHidd_Controller_Label_Options,
      vHidd_Controller_Label_L3, vHidd_Controller_Label_R3, vHidd_Controller_Label_L1, vHidd_Controller_Label_R1,
      vHidd_Controller_Label_DpadUp, vHidd_Controller_Label_DpadDown, vHidd_Controller_Label_DpadLeft, vHidd_Controller_Label_DpadRight,
      vHidd_Controller_Label_Misc, vHidd_Controller_Label_Paddle1, vHidd_Controller_Label_Paddle2,
      vHidd_Controller_Label_Paddle3, vHidd_Controller_Label_Paddle4, vHidd_Controller_Label_Touchpad,
      vHidd_Controller_Label_Misc, vHidd_Controller_Label_Misc, vHidd_Controller_Label_Misc, vHidd_Controller_Label_Misc, vHidd_Controller_Label_Misc },
    /* Nintendo (positional: south = B, east = A, west = Y, north = X) */
    { vHidd_Controller_Label_B, vHidd_Controller_Label_A, vHidd_Controller_Label_Y, vHidd_Controller_Label_X,
      vHidd_Controller_Label_Minus, vHidd_Controller_Label_Home, vHidd_Controller_Label_Plus,
      vHidd_Controller_Label_L3, vHidd_Controller_Label_R3, vHidd_Controller_Label_L1, vHidd_Controller_Label_R1,
      vHidd_Controller_Label_DpadUp, vHidd_Controller_Label_DpadDown, vHidd_Controller_Label_DpadLeft, vHidd_Controller_Label_DpadRight,
      vHidd_Controller_Label_Capture, vHidd_Controller_Label_Paddle1, vHidd_Controller_Label_Paddle2,
      vHidd_Controller_Label_Paddle3, vHidd_Controller_Label_Paddle4, vHidd_Controller_Label_Touchpad,
      vHidd_Controller_Label_Misc, vHidd_Controller_Label_Misc, vHidd_Controller_Label_Misc, vHidd_Controller_Label_Misc, vHidd_Controller_Label_Misc },
    /* Amiga / CD32 */
    { vHidd_Controller_Label_Red, vHidd_Controller_Label_Blue, vHidd_Controller_Label_Green, vHidd_Controller_Label_Yellow,
      vHidd_Controller_Label_None, vHidd_Controller_Label_None, vHidd_Controller_Label_Play,
      vHidd_Controller_Label_None, vHidd_Controller_Label_None, vHidd_Controller_Label_Reverse, vHidd_Controller_Label_Forward,
      vHidd_Controller_Label_DpadUp, vHidd_Controller_Label_DpadDown, vHidd_Controller_Label_DpadLeft, vHidd_Controller_Label_DpadRight,
      vHidd_Controller_Label_None, vHidd_Controller_Label_None, vHidd_Controller_Label_None,
      vHidd_Controller_Label_None, vHidd_Controller_Label_None, vHidd_Controller_Label_None,
      vHidd_Controller_Label_None, vHidd_Controller_Label_None, vHidd_Controller_Label_None, vHidd_Controller_Label_None, vHidd_Controller_Label_None },
};

UWORD ctrl_LabelFor(const struct ControllerDevice *dev, UWORD stdid)
{
    UBYTE style = dev->style;

    if (style > vHidd_Controller_Style_Amiga)
        style = vHidd_Controller_Style_Generic;

    if (vHidd_Controller_Std_IsButton(stdid) && stdid < vHidd_Controller_GP_ButtonCount)
        return ctrl_labels[style][stdid];

    if (vHidd_Controller_Std_IsAxis(stdid))
    {
        switch (stdid & 0xFF)
        {
        case vHidd_Controller_GPA_LeftX:
        case vHidd_Controller_GPA_LeftY:
            return vHidd_Controller_Label_LeftStick;
        case vHidd_Controller_GPA_RightX:
        case vHidd_Controller_GPA_RightY:
            return vHidd_Controller_Label_RightStick;
        case vHidd_Controller_GPA_LeftTrigger:
            return (style == vHidd_Controller_Style_Xbox) ? vHidd_Controller_Label_LT :
                   (style == vHidd_Controller_Style_Amiga) ? vHidd_Controller_Label_None : vHidd_Controller_Label_L2;
        case vHidd_Controller_GPA_RightTrigger:
            return (style == vHidd_Controller_Style_Xbox) ? vHidd_Controller_Label_RT :
                   (style == vHidd_Controller_Style_Amiga) ? vHidd_Controller_Label_None : vHidd_Controller_Label_R2;
        }
    }
    return vHidd_Controller_Label_None;
}
