/*
 * Copyright (C) 2012, The AROS Development Team.  All rights reserved.
 * Author: Jason S. McMullan <jason.mcmullan@gmail.com>
 *
 * Licensed under the AROS PUBLIC LICENSE (APL) Version 1.1
 */

#ifndef PRINTERS_INTERN_H
#define PRINTERS_INTERN_H

#include <devices/prtbase.h>

/* .text tag that helps identify this as a printer drivers.
 * The ELF header already told us the architecture
 */
#define PRINTER_MAGIC   0x70004e75      /* m68k's moveq #0,%d0, rts */

struct PrinterMagicHeader {
    ULONG pmh_Magic;
    UWORD pmh_Version;
    UWORD pmh_Revision;
    struct PrinterExtendedData pmh_PED;
};

#define PRINTER_TAG(PED, version, revision, ped...) \
    __section(".tag.printer") __used static struct PrinterMagicHeader pmh = { \
        .pmh_Magic = PRINTER_MAGIC, \
        .pmh_Version = (version), \
        .pmh_Revision = (revision), \
        .pmh_PED = { ped } }; \
    struct PrinterExtendedData *PED = &pmh.pmh_PED;

/* 'status' codes sent to the ped_Render() routine
 */

/* PRS_INIT - Initialize page for printing
 *
 * ct - pointer to IODRPReq
 * x  - width in pixels
 * y  - height in pixels
 */
#define PRS_INIT                0
/* PRS_TRANSFER - Render page row
 *
 * ct - pointer to PrtInfo
 * x  - 0
 * y  - row # (0 to height-1)
 */
#define PRS_TRANSFER            1
/* PRS_FLUSH - Send data to the printer
 * ct - 0
 * x  - 0
 * y  - # of rows to send (1 to NumRows)
 */
#define PRS_FLUSH               2
/* PRS_CLEAR - Clear and Init buffer
 * ct - 0
 * x  - 0
 * y  - 0
 */
#define PRS_CLEAR               3
/* PRS_CLOSE - Close down
 * ct - Error code
 * x  - io_Special flag from IODRPReq
 * y  - 0
 */
#define PRS_CLOSE               4
/* PRS_PREINIT - Pre-Master initialization
 * ct - NULL or pointer to IODRPReq
 * x  - io_Special flag from IODRPReq
 * y  - 0
 */
#define PRS_PREINIT             5
/* PRS_CONVERT - Transfer colormap/BGR line to printer
 * ct - UBYTE * to an array of entries
 * x  - # of entries in the array
 * y  - 0 if ct is a union colorEntry *, 1 if ct is a BGR pixel line
 */
#define PRS_EJECT               7
/* PRS_EJECT - Finish page, advance to next page
 * ct - 0
 * x  - 0
 * y  - 0
 */
#define PRS_CONVERT             8
/* PRS_CORRECT - Color correct a colormap/BGR line
 * ct - UBYTE * to an array of entries
 * x  - # of entries in the array
 * y  - 0 if ct is a union colorEntry *, 1 if ct is a BGR pixel line
 */
#define PRS_CORRECT             9


#endif /* PRINTERS_INTERN_H */
