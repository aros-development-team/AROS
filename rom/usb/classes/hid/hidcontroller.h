#ifndef HIDCONTROLLER_H
#define HIDCONTROLLER_H

/*
 *----------------------------------------------------------------------------
 *              Game controller devices for the HID class
 *----------------------------------------------------------------------------
 *
 * Registers one controller.hidd device per HID interface that carries a
 * Joystick, Game Pad or Multi-axis application collection, and pushes the
 * decoded input reports to it. The mapping itself lives in hidctrl_map.c.
 */

#include "hid.h"
#include "hidctrl_map.h"

struct NepHidCtrl
{
    APTR                    hc_Device;      /* controller.hidd device object            */
    struct HidCtrlTable    *hc_Table;       /* control table and map                    */
    struct HidCtrlItem     *hc_Items;       /* flat view of the parsed input items      */
    ULONG                   hc_ItemCount;
    APTR                    hc_Outputs;     /* struct Hidd_Controller_OutputDesc[]      */
    APTR                    hc_Raw;         /* struct pHidd_Controller_RawReport        */
};

BOOL nCtrlInit(struct NepHidBase *nh);
void nCtrlExit(struct NepHidBase *nh);
void nCtrlAttach(struct NepClassHid *nch);
void nCtrlDetach(struct NepClassHid *nch);
void nCtrlHandleReport(struct NepClassHid *nch, struct NepHidReport *nhr, UBYTE *buf, ULONG buflen);
void nCtrlDOSAvailable(struct NepHidBase *nh);
BOOL nCtrlDeferBinding(struct NepClassHid *nch);

#endif /* HIDCONTROLLER_H */
