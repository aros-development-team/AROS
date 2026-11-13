/*
 *----------------------------------------------------------------------------
 *        HID items -> controller control table, report bytes -> values
 *----------------------------------------------------------------------------
 *
 * Bluetooth HID class copy. Mirrors what the HID classes understand for joysticks and game pads today:
 * Button page items, Generic Desktop X/Y/Z/Rx/Ry/Rz/Slider/Dial/Wheel axes,
 * the Hat Switch, and the Simulation page driving axes. Items that belong to
 * mouse or keyboard collections of a composite interface are left alone.
 */

#include "bthidctrl_map.h"

static BOOL bthidctrl_IsControllerColl(ULONG usage)
{
    return usage == BTHIDCTRL_COLL_JOYSTICK || usage == BTHIDCTRL_COLL_GAMEPAD || usage == BTHIDCTRL_COLL_MULTIAXIS;
}

ULONG bthidctrl_FindRoot(const struct BtHidCtrlItem *items, ULONG count)
{
    ULONG i;

    for (i = 0; i < count; i++)
        if (bthidctrl_IsControllerColl(items[i].bhci_RootUsage))
            return items[i].bhci_RootUsage;
    return 0;
}

/* Classify an input item; returns the control kind or 0xFF when it is not a controller control */
static UBYTE bthidctrl_Classify(const struct BtHidCtrlItem *item, UWORD *flags)
{
    ULONG page = item->bhci_Usage >> 16;
    ULONG id = item->bhci_Usage & 0xFFFF;

    *flags = 0;
    if (item->bhci_Type != REPORT_MAIN_INPUT)
        return 0xFF;
    if (!(item->bhci_Flags & RPF_MAIN_VARIABLE) || (item->bhci_Flags & RPF_MAIN_CONST))
        return 0xFF;
    if (item->bhci_RootUsage == BTHIDCTRL_COLL_MOUSE || item->bhci_RootUsage == BTHIDCTRL_COLL_KEYBOARD ||
        item->bhci_RootUsage == BTHIDCTRL_COLL_KEYPAD)
        return 0xFF;
    if (item->bhci_Flags & RPF_MAIN_RELATIVE)
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

ULONG bthidctrl_Build(const struct BtHidCtrlItem *items, ULONG count, struct BtHidCtrlTable *t)
{
    BOOL used[HIDD_CONTROLLER_MAX_BUTTONS];
    struct Hidd_Controller_ControlDesc *cd;
    ULONG i;

    t->bhct_RootUsage = bthidctrl_FindRoot(items, count);
    t->bhct_MapCount = 0;
    t->bhct_Buttons = t->bhct_Axes = t->bhct_Hats = 0;
    t->bhct_ControlCount = 0;
    for (i = 0; i < HIDD_CONTROLLER_MAX_BUTTONS; i++)
        used[i] = FALSE;
    cd = t->bhct_Controls;
    cd->kind = vHidd_Controller_Ctl_End;

    if (!t->bhct_RootUsage)
        return 0;

    for (i = 0; i < count && t->bhct_MapCount < BTHIDCTRL_MAX_MAP; i++)
    {
        const struct BtHidCtrlItem *item = &items[i];
        UWORD flags;
        UBYTE kind = bthidctrl_Classify(item, &flags);
        UBYTE index;

        if (kind == 0xFF)
            continue;

        switch (kind)
        {
        case vHidd_Controller_Ctl_Button:
            index = (item->bhci_Usage & 0xFFFF) - 1;
            if (used[index])
                continue;
            used[index] = TRUE;
            if ((UWORD)(index + 1) > t->bhct_Buttons)
                t->bhct_Buttons = index + 1;
            break;
        case vHidd_Controller_Ctl_Axis:
            if (t->bhct_Axes >= HIDD_CONTROLLER_MAX_AXES)
                continue;
            index = t->bhct_Axes++;
            break;
        default:
            if (t->bhct_Hats >= HIDD_CONTROLLER_MAX_HATS)
                continue;
            index = t->bhct_Hats++;
            break;
        }

        t->bhct_Map[t->bhct_MapCount].bhcm_Item = i;
        t->bhct_Map[t->bhct_MapCount].bhcm_Kind = kind;
        t->bhct_Map[t->bhct_MapCount].bhcm_Index = index;
        t->bhct_MapCount++;

        cd->kind = kind;
        cd->index = index;
        cd->flags = flags;
        cd->usage_page = item->bhci_Usage >> 16;
        cd->usage = item->bhci_Usage & 0xFFFF;
        cd->min = item->bhci_Min;
        cd->max = item->bhci_Max;
        cd->flat = 0;
        cd->fuzz = 0;
        cd->label = (kind == vHidd_Controller_Ctl_Button) ? vHidd_Controller_Label_Numbered + index + 1
                                                          : vHidd_Controller_Label_None;
        cd->locality = vHidd_Controller_Loc_None;
        cd->companion = 0;
        cd->name = NULL;
        cd++;
        t->bhct_ControlCount++;
    }

    /* buttons occupy indices 0..Buttons-1 even when the device skips some usages */
    for (i = 0; i < t->bhct_Buttons; i++)
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
        t->bhct_ControlCount++;
    }
    cd->kind = vHidd_Controller_Ctl_End;

    return t->bhct_MapCount;
}

BOOL bthidctrl_Extract(const struct BtHidCtrlItem *item, const UBYTE *buf, ULONG buflen, LONG *value)
{
    ULONG offset = item->bhci_Offset;
    ULONG size = item->bhci_Size;
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
    if (item->bhci_IsSigned && size < 32 && (v & (1UL << (size - 1))))
        v |= ~((1UL << size) - 1);
    *value = (LONG)v;
    return TRUE;
}

BOOL bthidctrl_Decode(const struct BtHidCtrlTable *t, const struct BtHidCtrlItem *items, UWORD reportid,
                    const UBYTE *buf, ULONG buflen, struct pHidd_Controller_RawReport *r)
{
    BOOL any = FALSE;
    ULONG i;

    for (i = 0; i < t->bhct_MapCount; i++)
    {
        const struct BtHidCtrlMapEntry *m = &t->bhct_Map[i];
        const struct BtHidCtrlItem *item = &items[m->bhcm_Item];
        LONG value;

        if (item->bhci_ReportID != reportid)
            continue;
        if (!bthidctrl_Extract(item, buf, buflen, &value))
            continue;
        any = TRUE;

        switch (m->bhcm_Kind)
        {
        case vHidd_Controller_Ctl_Button:
            if (value) r->buttons[m->bhcm_Index >> 5] |=  (1UL << (m->bhcm_Index & 31));
            else       r->buttons[m->bhcm_Index >> 5] &= ~(1UL << (m->bhcm_Index & 31));
            break;
        case vHidd_Controller_Ctl_Axis:
            r->axes[m->bhcm_Index] = value;
            break;
        case vHidd_Controller_Ctl_Hat:
            /* raw value: the subsystem treats anything outside min..min+7 as the centred null state */
            r->hats[m->bhcm_Index] = (value < 0 || value > 255) ? 255 : (UBYTE)value;
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
