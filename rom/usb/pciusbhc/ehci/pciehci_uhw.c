/*
    Copyright © 2011-2012, The AROS Development Team. All rights reserved.
    $Id$
*/

#include LC_LIBDEFS_FILE

#include "pciehci_uhw.h"

BOOL uhwOpenTimer(struct ehu_unit *ehu, LIBBASETYPEPTR LIBBASE) {
    return FALSE;
}

void uhwDelayMS(ULONG milli, struct ehu_unit *ehu) {
}

void uhwDelayMicro(ULONG micro, struct ehu_unit *ehu) {
}

void uhwCloseTimer(struct ehu_unit *ehu, LIBBASETYPEPTR LIBBASE) {
//  LIBBASETYPE *ehd = (LIBBASETYPE *) LIBBASE;
}

UWORD uhwGetUsbState(struct IOUsbHWReq *ioreq, struct ehu_unit *ehu, LIBBASETYPEPTR LIBBASE) {
//  LIBBASETYPE *ehd = (LIBBASETYPE *) LIBBASE;
    return(ioreq->iouh_State = UHSF_OPERATIONAL);
}

void uhwCheckRootHubChanges(struct ehu_unit *ehu) {
}

void uhwCheckSpecialCtrlTransfers(struct ehc_controller *ehc, struct IOUsbHWReq *ioreq) {
}

AROS_UFH1(void, uhwNakTimeoutInt, AROS_UFHA(struct ehu_unit *,  unit, A1)) {
    AROS_USERFUNC_INIT

    AROS_USERFUNC_EXIT
}

