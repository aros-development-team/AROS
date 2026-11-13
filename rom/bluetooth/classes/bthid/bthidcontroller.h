#ifndef BTHIDCONTROLLER_H
#define BTHIDCONTROLLER_H

/*
 *----------------------------------------------------------------------------
 *         Game controller devices for the Bluetooth HID class
 *----------------------------------------------------------------------------
 *
 * Registers one controller.hidd device per bound HID service that carries a
 * Joystick, Game Pad or Multi-axis application collection, and pushes the
 * decoded input reports to it. The mapping itself lives in bthidctrl_map.c.
 */

#include "bthid.h"
#include "bthidctrl_map.h"

struct BtHidCtrl
{
    APTR                    hc_Device;      /* controller.hidd device object            */
    struct BtHidCtrlTable  *hc_Table;       /* control table and map                    */
    struct BtHidCtrlItem   *hc_Items;       /* flat view of the parsed input items      */
    ULONG                   hc_ItemCount;
    APTR                    hc_Outputs;     /* struct Hidd_Controller_OutputDesc[]      */
    APTR                    hc_Raw;         /* struct pHidd_Controller_RawReport        */
};

BOOL bCtrlInit(struct BTHidBase *nh);
void bCtrlExit(struct BTHidBase *nh);
void bCtrlAttach(struct BTHidBinding *nhb);
void bCtrlDetach(struct BTHidBinding *nhb);
void bCtrlHandleReport(struct BTHidBinding *nhb, struct BtHidReport *nhr, UBYTE *buf, ULONG buflen);

#endif /* BTHIDCONTROLLER_H */
