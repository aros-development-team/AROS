/*
 * Copyright (C) 2013, The AROS Development Team
 * All right reserved.
 * Author: Jason S. McMullan <jason.mcmullan@gmail.com>
 *
 * Licensed under the AROS PUBLIC LICENSE (APL) Version 1.1
 */

#ifndef CD32_H
#define CD32_H

/* CD32 Akiko registers */

#define AKIKO_ID        0xb80002        /* u16 */
#define AKIKO_ID_MAGIC  0xCAFE

#define AKIKO_CDINTREQ  0xb80004        /* u32 */
#define AKIKO_CDINTENA  0xb80008        /* u32 */
#define AKIKO_CDINT_SUBCODE     (1 << 31)
#define AKIKO_CDINT_DRIVEXMIT   (1 << 30)
#define AKIKO_CDINT_DRIVERECV   (1 << 29)
#define AKIKO_CDINT_RXDMA       (1 << 28)
#define AKIKO_CDINT_TXDMA       (1 << 27)
#define AKIKO_CDINT_PBX         (1 << 26)
#define AKIKO_CDINT_OVERFLOW    (1 << 25)

#define AKIKO_CDADRDATA 0xb80010        /* u32 */
        /* Address must be on a 4K boundary, in the first 16M of address
         * Sector data is aligned on 4K boundaries, regardless of
         * the actual CD sector size.
         * Address +0x000..0xbff Sector DMA index (u32), followed by sector data
         *         +0xc00..0xfff Sector error status
         */
#define AKIKO_CDADRMISC 0xb80014        /* u32 Address for misc data */
        /* Address must be on a 1K boundary, in the first 16M of address
         * Address +0x000..0x0ff is Result buffer
         *         +0x100..0x1ff is Subcode data
         *         +0x200..0x2ff is Command buffer
         */
#define AKIKO_CDSUBINX  0xb80018        /* u8 Bytes of subcode recieved */
#define AKIKO_CDTXINX   0xb80019        /* u8 Bytes of Command processed */
#define AKIKO_CDRXINX   0xb8001a        /* u8 Bytes of Result processed */
#define AKIKO_CDTXCMP   0xb8001d        /* u8 Bytes of Command expected */
#define AKIKO_CDRXCMP   0xb8001f        /* u8 Bytes of Result expected */
#define AKIKO_CDPBX     0xb80020        /* u16 Sector mask */

#define AKIKO_CDFLAG    0xb80024        /* u32 */
#define AKIKO_CDFLAG_SUBCODE    (1 << 31)
#define AKIKO_CDFLAG_TXD        (1 << 30)
#define AKIKO_CDFLAG_RXD        (1 << 29)
#define AKIKO_CDFLAG_CAS        (1 << 28)
#define AKIKO_CDFLAG_PBX        (1 << 27)
#define AKIKO_CDFLAG_ENABLE     (1 << 26)
#define AKIKO_CDFLAG_RAW        (1 << 25)
#define AKIKO_CDFLAG_MSB        (1 << 24)
#define AKIKO_CDRESET   0xb80025        /* u8  0x80 = reset, 0x00 = normal */

#define AKIKO_NVRAM     0xb80030        /* u32 */
#define AKIKO_C2P       0xb80038        /* u32 */


#endif /* CD32_H */
