/*
    Copyright © 2011, The AROS Development Team. All rights reserved
    $Id: pciehci_uhw.h 40015 2011-07-11 23:48:37Z DizzyOfCRN $
*/

UWORD cmdQueryDevice(struct IOUsbHWReq *ioreq, struct ehu_unit *ehu, LIBBASETYPEPTR LIBBASE);
UWORD uhwGetUsbState(struct IOUsbHWReq *ioreq, struct ehu_unit *ehu, LIBBASETYPEPTR LIBBASE);
