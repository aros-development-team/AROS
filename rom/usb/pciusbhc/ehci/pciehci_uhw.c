/*
    Copyright © 2011, The AROS Development Team. All rights reserved
    $Id$
*/

#include LC_LIBDEFS_FILE

#include "pciehci_uhw.h"

UWORD uhwGetUsbState(struct IOUsbHWReq *ioreq, struct ehu_unit *ehu, LIBBASETYPEPTR LIBBASE) {
//  LIBBASETYPE *ehd = (LIBBASETYPE *) LIBBASE;

    return(ioreq->iouh_State = UHSF_OPERATIONAL);
}

void uhwCheckSpecialCtrlTransfers(struct ehc_controller *ehc, struct IOUsbHWReq *ioreq) {
}

