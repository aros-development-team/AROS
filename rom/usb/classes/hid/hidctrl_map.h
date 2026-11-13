#ifndef HIDCTRL_MAP_H
#define HIDCTRL_MAP_H

/*
 *----------------------------------------------------------------------------
 *        HID items -> controller control table, report bytes -> values
 *----------------------------------------------------------------------------
 *
 * This unit depends on nothing but the HID report constants and the
 * controller.hidd public header, so it can be unit tested without Poseidon.
 * hidcontroller.c feeds it a flat array of the parsed input items.
 */

#include <exec/types.h>
#include <devices/usb_hid.h>
#include <hidd/controller.h>

#define HIDCTRL_MAX_ITEMS  256
#define HIDCTRL_MAX_MAP    (HIDD_CONTROLLER_MAX_BUTTONS + HIDD_CONTROLLER_MAX_AXES + HIDD_CONTROLLER_MAX_HATS)

#define HIDCTRL_COLL_MOUSE      0x010002
#define HIDCTRL_COLL_JOYSTICK   0x010004
#define HIDCTRL_COLL_GAMEPAD    0x010005
#define HIDCTRL_COLL_KEYBOARD   0x010006
#define HIDCTRL_COLL_KEYPAD     0x010007
#define HIDCTRL_COLL_MULTIAXIS  0x010008

/* What the mapper needs to know about one parsed HID main item */
struct HidCtrlItem
{
    ULONG   hci_Usage;      /* page << 16 | usage id                          */
    ULONG   hci_Flags;      /* RPF_MAIN_*                                     */
    UWORD   hci_Type;       /* REPORT_MAIN_INPUT / OUTPUT / FEATURE           */
    UWORD   hci_Offset;     /* bit offset inside the report (without id byte) */
    UWORD   hci_Size;       /* bits                                           */
    UWORD   hci_ReportID;
    LONG    hci_Min;        /* logical range                                  */
    LONG    hci_Max;
    BOOL    hci_IsSigned;
    ULONG   hci_RootUsage;  /* usage of the top level collection              */
    APTR    hci_Ref;        /* opaque, for the caller                         */
};

/* One mapped control */
struct HidCtrlMapEntry
{
    UWORD   hcm_Item;       /* index into the item array */
    UBYTE   hcm_Kind;       /* vHidd_Controller_Ctl_*    */
    UBYTE   hcm_Index;
};

struct HidCtrlTable
{
    ULONG                              hct_RootUsage;   /* first controller collection found */
    ULONG                              hct_MapCount;
    struct HidCtrlMapEntry             hct_Map[HIDCTRL_MAX_MAP];
    UWORD                              hct_Buttons, hct_Axes, hct_Hats;
    ULONG                              hct_ControlCount;
    struct Hidd_Controller_ControlDesc hct_Controls[HIDCTRL_MAX_MAP + HIDD_CONTROLLER_MAX_BUTTONS + 1];
};

/* Usage of the first top level Joystick/Game Pad/Multi-axis collection among the items, or 0 */
ULONG hidctrl_FindRoot(const struct HidCtrlItem *items, ULONG count);

/* Build the control table; returns the number of mapped controls (0 = nothing to register) */
ULONG hidctrl_Build(const struct HidCtrlItem *items, ULONG count, struct HidCtrlTable *t);

/* Extract an item's value from a report buffer (sign extended when signed); returns FALSE if out of the buffer */
BOOL hidctrl_Extract(const struct HidCtrlItem *item, const UBYTE *buf, ULONG buflen, LONG *value);

/* Update a raw report from the items of one input report; returns TRUE if anything belonged to it */
BOOL hidctrl_Decode(const struct HidCtrlTable *t, const struct HidCtrlItem *items, UWORD reportid,
                    const UBYTE *buf, ULONG buflen, struct pHidd_Controller_RawReport *r);

#endif /* HIDCTRL_MAP_H */
