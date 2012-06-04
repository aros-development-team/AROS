/*
    Copyright © 2011, The AROS Development Team. All rights reserved
    $Id$
*/

static const UWORD NSDSupported[] = {
    CMD_FLUSH, CMD_RESET,
    UHCMD_QUERYDEVICE,
    UHCMD_USBRESET,
    UHCMD_USBRESUME,
    UHCMD_USBSUSPEND,
    UHCMD_USBOPER,
    UHCMD_CONTROLXFER ,
    UHCMD_ISOXFER,
    UHCMD_INTXFER,
    UHCMD_BULKXFER,
    NSCMD_DEVICEQUERY,
    0
};

UWORD uhwGetUsbState(struct IOUsbHWReq *ioreq, struct ehu_unit *ehu, LIBBASETYPEPTR LIBBASE);
void uhwCheckSpecialCtrlTransfers(struct ehc_controller *ehc, struct IOUsbHWReq *ioreq);

