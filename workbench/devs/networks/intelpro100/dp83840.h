/*

Copyright (C) 2005 Neil Cafferkey

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston,
MA 02111-1307, USA.

*/

#ifndef DP83840_H
#define DP83840_H


/* Registers */
/* ========= */

#define MII_PCR 23


/* Register Details */
/* ================ */

/* PCS Configuration Register */

#define MII_PCRB_NOMLT3           11
#define MII_PCRB_NOCLK25M          7
#define MII_PCRB_NOFORCE100BASETX  6
#define MII_PCRB_NOLINKMON         5
#define MII_PCRB_NOTX              4
#define MII_PCRB_LED1LINK          2
#define MII_PCRB_LED4DUPLEX        1

#define MII_PCRF_NOMLT3           (1 << MII_PCRB_NOMLT3)
#define MII_PCRF_NOCLK25M         (1 << MII_PCRB_NOCLK25M)
#define MII_PCRF_NOFORCE100BASETX (1 << MII_PCRB_NOFORCE100BASETX)
#define MII_PCRF_NOLINKMON        (1 << MII_PCRB_NOLINKMON)
#define MII_PCRF_NOTX             (1 << MII_PCRB_NOTX)
#define MII_PCRF_LED1LINK         (1 << MII_PCRB_LED1LINK)
#define MII_PCRF_LED4DUPLEX       (1 << MII_PCRB_LED4DUPLEX)

#endif
