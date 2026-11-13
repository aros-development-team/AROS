/*

Copyright (C) 2023 Neil Cafferkey

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

#ifndef NVIDIANET_H
#define NVIDIANET_H


/* General */
/* ======= */

#define NV_DESCSIZE 16


/* Registers */
/* ========= */

#define NV_REG_INTSTATUS         0x0
#define NV_REG_INTMASK           0x4
#define NV_REG_POLLTIME          0xc
#define NV_REG_MACCTRL          0x3c
#define NV_REG_TXCTRL           0x84
#define NV_REG_TXSTATUS         0x88
#define NV_REG_RXCONFIG         0x8c
#define NV_REG_OFFLOADCONFIG    0x90
#define NV_REG_RXCTRL           0x94
#define NV_REG_RXSTATUS         0x98
#define NV_REG_RANDOMSEED       0x9c
#define NV_REG_TXDEFER          0xa0
#define NV_REG_RXDEFER          0xa4
#define NV_REG_ADDRESSLOW       0xa8
#define NV_REG_ADDRESSHIGH      0xac
#define NV_REG_MCASTLOW         0xb0
#define NV_REG_MCASTHIGH        0xb4
#define NV_REG_MCASTMASKLOW     0xb8
#define NV_REG_MCASTMASKHIGH    0xbc
#define NV_REG_PHYCTRL          0xc0
#define NV_REG_TXDMALISTLOW    0x100
#define NV_REG_RXDMALISTLOW    0x104
#define NV_REG_DMALISTSIZES    0x108
#define NV_REG_TXPOLL          0x10c
#define NV_REG_LINKSPEED       0x110
#define NV_REG_XCVRCTRL        0x144
#define NV_REG_TXDMALISTHIGH   0x148
#define NV_REG_RXDMALISTHIGH   0x14c
#define NV_REG_MIISTATUS       0x180
#define NV_REG_MIIMASK         0x184
#define NV_REG_TXPAUSE         0x170
#define NV_REG_ADAPTERCTRL     0x188
#define NV_REG_MIISPEED        0x18c
#define NV_REG_MIICTRL         0x190
#define NV_REG_MIIDATA         0x194
#define NV_REG_POWERCTRL       0x26c
#define NV_REG_VLANCTRL        0x300


/* Interrupts */
/* ========== */

#define NV_INTB_LINK      6
#define NV_INTB_TIMER     5
#define NV_INTB_TXDONE    4
#define NV_INTB_TXERR     3
#define NV_INTB_NORXDESC  2
#define NV_INTB_RXDONE    1
#define NV_INTB_RXERR     0

#define NV_INTF_LINK     (1 << NV_INTB_LINK)
#define NV_INTF_TIMER    (1 << NV_INTB_TIMER)
#define NV_INTF_TXDONE   (1 << NV_INTB_TXDONE)
#define NV_INTF_TXERR    (1 << NV_INTB_TXERR)
#define NV_INTF_NORXDESC (1 << NV_INTB_NORXDESC)
#define NV_INTF_RXDONE   (1 << NV_INTB_RXDONE)
#define NV_INTF_RXERR    (1 << NV_INTB_RXERR)


/* MII Interrupts */
/* ============== */

#define NV_MIIINTB_LINK  3
#define NV_MIIINTB_ERROR 0

#define NV_MIIINTF_LINK  (1 << NV_MIIINTB_LINK)
#define NV_MIIINTF_ERROR (1 << NV_MIIINTB_ERROR)


/* Link Speeds */
/* =========== */

#define NV_RATE_10   0x3e8
#define NV_RATE_100   0x64
#define NV_RATE_1000  0x32


/* Register Details */
/* ================ */

/* Transmit Control Register */

#define NV_REG_TXCTRLB_ON    24
#define NV_REG_TXCTRLB_START  0

#define NV_REG_TXCTRLF_ON    (1 << NV_REG_TXCTRLB_ON)
#define NV_REG_TXCTRLF_START (1 << NV_REG_TXCTRLB_START)

/* RX Configuration Register */

#define NV_REG_RXCONFIGB_PROM   7
#define NV_REG_RXCONFIGB_UCAST  5

#define NV_REG_RXCONFIGF_PROM  (1 << NV_REG_RXCONFIGB_PROM)
#define NV_REG_RXCONFIGF_UCAST (1 << NV_REG_RXCONFIGB_UCAST)

/* RX Control Register */

#define NV_REG_RXCTRLB_ON    24
#define NV_REG_RXCTRLB_START  0

#define NV_REG_RXCTRLF_ON    (1 << NV_REG_RXCTRLB_ON)
#define NV_REG_RXCTRLF_START (1 << NV_REG_RXCTRLB_START)

/* PHY Control Register */

#define NV_REG_PHYCTRLB_RGMII    31
#define NV_REG_PHYCTRLB_HDUPLEX   8
#define NV_REG_PHYCTRLB_1000MBPS  1
#define NV_REG_PHYCTRLB_100MBPS   0

#define NV_REG_PHYCTRLF_RGMII    (1 << NV_REG_PHYCTRLB_RGMII)
#define NV_REG_PHYCTRLF_HDUPLEX  (1 << NV_REG_PHYCTRLB_HDUPLEX)
#define NV_REG_PHYCTRLF_1000MBPS (1 << NV_REG_PHYCTRLB_1000MBPS)
#define NV_REG_PHYCTRLF_100MBPS  (1 << NV_REG_PHYCTRLB_100MBPS)

/* DMA List Sizes Register */

#define NV_REG_DMALISTSIZESB_RX 16
#define NV_REG_DMALISTSIZESB_TX  0

#define NV_REG_DMALISTSIZESF_RX (0xffff << NV_REG_DMALISTSIZESB_RX)
#define NV_REG_DMALISTSIZESF_TX (0xffff << NV_REG_DMALISTSIZESB_TX)

/* Link Speed Register */

#define NV_REG_LINKSPEEDB_FORCE 16
#define NV_REG_LINKSPEEDB_CODE   0

#define NV_REG_LINKSPEEDF_FORCE (1 << NV_REG_LINKSPEEDB_FORCE)
#define NV_REG_LINKSPEEDF_CODE  (0xfff << NV_REG_LINKSPEEDB_CODE)

/* Transceiver Control Register */

#define NV_REG_XCVRCTRLB_RESET   4
#define NV_REG_XCVRCTRLB_TXPOLL  0

#define NV_REG_XCVRCTRLF_RESET  (1 << NV_REG_XCVRCTRLB_RESET)
#define NV_REG_XCVRCTRLF_TXPOLL (1 << NV_REG_XCVRCTRLB_TXPOLL)

/* TX Pause Register */

#define NV_REG_TXPAUSEF_OFF 0x1ff0080
#define NV_REG_TXPAUSEF_ON  0x0c00030

/* Adapter Control Register */

#define NV_REG_ADAPTERCTRLB_PHYNO    24
#define NV_REG_ADAPTERCTRLB_RUNNING  20
#define NV_REG_ADAPTERCTRLB_PHYVALID 18
#define NV_REG_ADAPTERCTRLB_LINKUP    2
#define NV_REG_ADAPTERCTRLB_START     1

#define NV_REG_ADAPTERCTRLF_PHYNO    (0x1f << NV_REG_ADAPTERCTRLB_PHYNO)
#define NV_REG_ADAPTERCTRLF_RUNNING  (1 << NV_REG_ADAPTERCTRLB_RUNNING)
#define NV_REG_ADAPTERCTRLF_PHYVALID (1 << NV_REG_ADAPTERCTRLB_PHYVALID)
#define NV_REG_ADAPTERCTRLF_LINKUP   (1 << NV_REG_ADAPTERCTRLB_LINKUP)
#define NV_REG_ADAPTERCTRLF_START    (1 << NV_REG_ADAPTERCTRLB_START)

/* MII Control Register */

#define NV_REG_MIICTRLB_INUSE 15
#define NV_REG_MIICTRLB_WRITE 10
#define NV_REG_MIICTRLB_PHYNO  5
#define NV_REG_MIICTRLB_REGNO  0

#define NV_REG_MIICTRLF_INUSE (1 << NV_REG_MIICTRLB_INUSE)
#define NV_REG_MIICTRLF_WRITE (1 << NV_REG_MIICTRLB_WRITE)
#define NV_REG_MIICTRLF_PHYNO (0x1f << NV_REG_MIICTRLB_PHYNO)
#define NV_REG_MIICTRLF_REGNO (0x1f << NV_REG_MIICTRLB_REGNO)

/* Power Control Register */

#define NV_REG_POWERCTRLB_ON    15
#define NV_REG_POWERCTRLB_VALID  8
#define NV_REG_POWERCTRLB_STATE  0

#define NV_REG_POWERCTRLF_ON    (1 << NV_REG_POWERCTRLB_ON)
#define NV_REG_POWERCTRLF_VALID (1 << NV_REG_POWERCTRLB_VALID)
#define NV_REG_POWERCTRLF_STATE (0x3 << NV_REG_POWERCTRLB_STATE)


/* Frame descriptor */
/* ================ */

#define NV_DESC_BUFFERHIGH 0
#define NV_DESC_BUFFERLOW  1
#define NV_DESC_VLAN       2
#define NV_DESC_CONTROL    3

/* Control field */

#define NV_DESC_CONTROLB_TXVALID  31
#define NV_DESC_CONTROLB_INUSE    31
#define NV_DESC_CONTROLB_INT      30
#define NV_DESC_CONTROLB_ERROR    30
#define NV_DESC_CONTROLB_LASTFRAG 29
#define NV_DESC_CONTROLB_RXVALID  29
#define NV_DESC_CONTROLB_LENGTH    0

#define NV_DESC_CONTROLF_TXVALID  (1 << NV_DESC_CONTROLB_TXVALID)
#define NV_DESC_CONTROLF_INUSE    (1 << NV_DESC_CONTROLB_INUSE)
#define NV_DESC_CONTROLF_INT      (1 << NV_DESC_CONTROLB_INT)
#define NV_DESC_CONTROLF_ERROR    (1 << NV_DESC_CONTROLB_ERROR)
#define NV_DESC_CONTROLF_LASTFRAG (1 << NV_DESC_CONTROLB_LASTFRAG)
#define NV_DESC_CONTROLF_LENGTH   (0x3fff << NV_DESC_CONTROLB_LENGTH)

#endif
