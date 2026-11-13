/*
 *----------------------------------------------------------------------------
 *        HID items -> controller control table, report bytes -> values
 *----------------------------------------------------------------------------
 *
 * Mirrors what hid.class understands for joysticks and game pads today:
 * Button page items, Generic Desktop X/Y/Z/Rx/Ry/Rz/Slider/Dial/Wheel axes,
 * the Hat Switch, and the Simulation page driving axes. Items that belong to
 * mouse or keyboard collections of a composite interface are left alone.
 */

#include "hidctrl_map.h"

static BOOL hidctrl_IsControllerColl(ULONG usage)
{
    return usage == HIDCTRL_COLL_JOYSTICK || usage == HIDCTRL_COLL_GAMEPAD || usage == HIDCTRL_COLL_MULTIAXIS;
}

ULONG hidctrl_FindRoot(const struct HidCtrlItem *items, ULONG count)
{
    ULONG i;

    for (i = 0; i < count; i++)
        if (hidctrl_IsControllerColl(items[i].hci_RootUsage))
            return items[i].hci_RootUsage;
    return 0;
}

/* Classify an input item; returns the control kind or 0xFF when it is not a controller control */
static UBYTE hidctrl_Classify(const struct HidCtrlItem *item, UWORD *flags)
{
    ULONG page = item->hci_Usage >> 16;
    ULONG id = item->hci_Usage & 0xFFFF;

    *flags = 0;
    if (item->hci_Type != REPORT_MAIN_INPUT)
        return 0xFF;
    if (!(item->hci_Flags & RPF_MAIN_VARIABLE) || (item->hci_Flags & RPF_MAIN_CONST))
        return 0xFF;
    if (item->hci_RootUsage == HIDCTRL_COLL_MOUSE || item->hci_RootUsage == HIDCTRL_COLL_KEYBOARD ||
        item->hci_RootUsage == HIDCTRL_COLL_KEYPAD)
        return 0xFF;
    if (item->hci_Flags & RPF_MAIN_RELATIVE)
        *flags |= vHidd_Controller_CF_Relative;

    switch (page)
    {
    case 0x09:  /* Button */
        if (id >= 1 && id <= HIDD_CONTROLLER_MAX_BUTTONS)
            return vHidd_Controller_Ctl_Button;
        break;

    case 0x01:  /* Generic Desktop */
        switch (id)
        {
        case 0x30: case 0x31: case 0x32: case 0x33: case 0x34: case 0x35:  /* X Y Z Rx Ry Rz */
        case 0x36: case 0x37: case 0x38:                                    /* Slider Dial Wheel */
            return vHidd_Controller_Ctl_Axis;
        case 0x39:                                                          /* Hat switch */
            return vHidd_Controller_Ctl_Hat;
        }
        break;

    case 0x02:  /* Simulation */
        switch (id)
        {
        case 0xBB: case 0xC4: case 0xC5: case 0xC6:  /* Throttle Accelerator Brake Clutch */
            *flags |= vHidd_Controller_CF_Unipolar;
            return vHidd_Controller_Ctl_Axis;
        case 0xBA: case 0xC8:                        /* Rudder Steering */
            return vHidd_Controller_Ctl_Axis;
        }
        break;
    }
    return 0xFF;
}

ULONG hidctrl_Build(const struct HidCtrlItem *items, ULONG count, struct HidCtrlTable *t)
{
    BOOL used[HIDD_CONTROLLER_MAX_BUTTONS];
    struct Hidd_Controller_ControlDesc *cd;
    ULONG i;

    t->hct_RootUsage = hidctrl_FindRoot(items, count);
    t->hct_MapCount = 0;
    t->hct_Buttons = t->hct_Axes = t->hct_Hats = 0;
    t->hct_ControlCount = 0;
    for (i = 0; i < HIDD_CONTROLLER_MAX_BUTTONS; i++)
        used[i] = FALSE;
    cd = t->hct_Controls;
    cd->kind = vHidd_Controller_Ctl_End;

    if (!t->hct_RootUsage)
        return 0;

    for (i = 0; i < count && t->hct_MapCount < HIDCTRL_MAX_MAP; i++)
    {
        const struct HidCtrlItem *item = &items[i];
        UWORD flags;
        UBYTE kind = hidctrl_Classify(item, &flags);
        UBYTE index;

        if (kind == 0xFF)
            continue;

        switch (kind)
        {
        case vHidd_Controller_Ctl_Button:
            index = (item->hci_Usage & 0xFFFF) - 1;
            if (used[index])
                continue;
            used[index] = TRUE;
            if ((UWORD)(index + 1) > t->hct_Buttons)
                t->hct_Buttons = index + 1;
            break;
        case vHidd_Controller_Ctl_Axis:
            if (t->hct_Axes >= HIDD_CONTROLLER_MAX_AXES)
                continue;
            index = t->hct_Axes++;
            break;
        default:
            if (t->hct_Hats >= HIDD_CONTROLLER_MAX_HATS)
                continue;
            index = t->hct_Hats++;
            break;
        }

        t->hct_Map[t->hct_MapCount].hcm_Item = i;
        t->hct_Map[t->hct_MapCount].hcm_Kind = kind;
        t->hct_Map[t->hct_MapCount].hcm_Index = index;
        t->hct_MapCount++;

        cd->kind = kind;
        cd->index = index;
        cd->flags = flags;
        cd->usage_page = item->hci_Usage >> 16;
        cd->usage = item->hci_Usage & 0xFFFF;
        cd->min = item->hci_Min;
        cd->max = item->hci_Max;
        cd->flat = 0;
        cd->fuzz = 0;
        cd->label = (kind == vHidd_Controller_Ctl_Button) ? vHidd_Controller_Label_Numbered + index + 1
                                                          : vHidd_Controller_Label_None;
        cd->locality = vHidd_Controller_Loc_None;
        cd->companion = 0;
        cd->name = NULL;
        cd++;
        t->hct_ControlCount++;
    }

    /* buttons occupy indices 0..Buttons-1 even when the device skips some usages */
    for (i = 0; i < t->hct_Buttons; i++)
    {
        if (used[i])
            continue;
        cd->kind = vHidd_Controller_Ctl_Button;
        cd->index = i;
        cd->flags = 0;
        cd->usage_page = 0x09;
        cd->usage = i + 1;
        cd->min = 0;
        cd->max = 1;
        cd->flat = cd->fuzz = 0;
        cd->label = vHidd_Controller_Label_Numbered + i + 1;
        cd->locality = 0;
        cd->companion = 0;
        cd->name = NULL;
        cd++;
        t->hct_ControlCount++;
    }
    cd->kind = vHidd_Controller_Ctl_End;

    return t->hct_MapCount;
}

BOOL hidctrl_Extract(const struct HidCtrlItem *item, const UBYTE *buf, ULONG buflen, LONG *value)
{
    ULONG offset = item->hci_Offset;
    ULONG size = item->hci_Size;
    ULONG v = 0;
    ULONG i;

    if (size == 0 || size > 32 || ((offset + size + 7) >> 3) > buflen)
        return FALSE;

    for (i = 0; i < size; i++)
    {
        ULONG bit = offset + i;
        if ((buf[bit >> 3] >> (bit & 7)) & 1)
            v |= 1UL << i;
    }
    if (item->hci_IsSigned && size < 32 && (v & (1UL << (size - 1))))
        v |= ~((1UL << size) - 1);
    *value = (LONG)v;
    return TRUE;
}

BOOL hidctrl_Decode(const struct HidCtrlTable *t, const struct HidCtrlItem *items, UWORD reportid,
                    const UBYTE *buf, ULONG buflen, struct pHidd_Controller_RawReport *r)
{
    BOOL any = FALSE;
    ULONG i;

    for (i = 0; i < t->hct_MapCount; i++)
    {
        const struct HidCtrlMapEntry *m = &t->hct_Map[i];
        const struct HidCtrlItem *item = &items[m->hcm_Item];
        LONG value;

        if (item->hci_ReportID != reportid)
            continue;
        if (!hidctrl_Extract(item, buf, buflen, &value))
            continue;
        any = TRUE;

        switch (m->hcm_Kind)
        {
        case vHidd_Controller_Ctl_Button:
            if (value) r->buttons[m->hcm_Index >> 5] |=  (1UL << (m->hcm_Index & 31));
            else       r->buttons[m->hcm_Index >> 5] &= ~(1UL << (m->hcm_Index & 31));
            break;
        case vHidd_Controller_Ctl_Axis:
            r->axes[m->hcm_Index] = value;
            break;
        case vHidd_Controller_Ctl_Hat:
            /* raw value: the subsystem treats anything outside min..min+7 as the centred null state */
            r->hats[m->hcm_Index] = (value < 0 || value > 255) ? 255 : (UBYTE)value;
            break;
        }
    }
    if (any)
    {
        r->timestamp = 0;
        r->valid = vHidd_Controller_RR_Buttons | vHidd_Controller_RR_Axes | vHidd_Controller_RR_Hats;
    }
    return any;
}
