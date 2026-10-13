#ifndef BTBTHIDCTRL_MAP_H
#define BTBTHIDCTRL_MAP_H

/*
 *----------------------------------------------------------------------------
 *        HID items -> controller control table, report bytes -> values
 *----------------------------------------------------------------------------
 *
 * This unit depends on nothing but the HID report constants and the
 * controller.hidd public header, so it can be unit tested without bluetooth.library. Own copy for the
 * Bluetooth HID class (not shared with hid.class); bthidcontroller.c feeds it
 * a flat array of the parsed input items.
 */

#include <exec/types.h>
#include <devices/usb_hid.h>
#include <hidd/controller.h>

#define BTHIDCTRL_MAX_ITEMS  256
#define BTHIDCTRL_MAX_MAP    (HIDD_CONTROLLER_MAX_BUTTONS + HIDD_CONTROLLER_MAX_AXES + HIDD_CONTROLLER_MAX_HATS)

#define BTHIDCTRL_COLL_MOUSE      0x010002
#define BTHIDCTRL_COLL_JOYSTICK   0x010004
#define BTHIDCTRL_COLL_GAMEPAD    0x010005
#define BTHIDCTRL_COLL_KEYBOARD   0x010006
#define BTHIDCTRL_COLL_KEYPAD     0x010007
#define BTHIDCTRL_COLL_MULTIAXIS  0x010008

/* What the mapper needs to know about one parsed HID main item */
struct BtHidCtrlItem
{
    ULONG   bhci_Usage;      /* page << 16 | usage id                          */
    ULONG   bhci_Flags;      /* RPF_MAIN_*                                     */
    UWORD   bhci_Type;       /* REPORT_MAIN_INPUT / OUTPUT / FEATURE           */
    UWORD   bhci_Offset;     /* bit offset inside the report (without id byte) */
    UWORD   bhci_Size;       /* bits                                           */
    UWORD   bhci_ReportID;
    LONG    bhci_Min;        /* logical range                                  */
    LONG    bhci_Max;
    BOOL    bhci_IsSigned;
    ULONG   bhci_RootUsage;  /* usage of the top level collection              */
    APTR    bhci_Ref;        /* opaque, for the caller                         */
};

/* One mapped control */
struct BtHidCtrlMapEntry
{
    UWORD   bhcm_Item;       /* index into the item array */
    UBYTE   bhcm_Kind;       /* vHidd_Controller_Ctl_*    */
    UBYTE   bhcm_Index;
};

struct BtHidCtrlTable
{
    ULONG                              bhct_RootUsage;   /* first controller collection found */
    ULONG                              bhct_MapCount;
    struct BtHidCtrlMapEntry             bhct_Map[BTHIDCTRL_MAX_MAP];
    UWORD                              bhct_Buttons, bhct_Axes, bhct_Hats;
    ULONG                              bhct_ControlCount;
    struct Hidd_Controller_ControlDesc bhct_Controls[BTHIDCTRL_MAX_MAP + HIDD_CONTROLLER_MAX_BUTTONS + 1];
};

/* Usage of the first top level Joystick/Game Pad/Multi-axis collection among the items, or 0 */
ULONG bthidctrl_FindRoot(const struct BtHidCtrlItem *items, ULONG count);

/* Build the control table; returns the number of mapped controls (0 = nothing to register) */
ULONG bthidctrl_Build(const struct BtHidCtrlItem *items, ULONG count, struct BtHidCtrlTable *t);

/* Extract an item's value from a report buffer (sign extended when signed); returns FALSE if out of the buffer */
BOOL bthidctrl_Extract(const struct BtHidCtrlItem *item, const UBYTE *buf, ULONG buflen, LONG *value);

/* Update a raw report from the items of one input report; returns TRUE if anything belonged to it */
BOOL bthidctrl_Decode(const struct BtHidCtrlTable *t, const struct BtHidCtrlItem *items, UWORD reportid,
                    const UBYTE *buf, ULONG buflen, struct pHidd_Controller_RawReport *r);

#endif /* BTBTHIDCTRL_MAP_H */
