/*

Copyright (C) 2005,2006 Neil Cafferkey

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

#ifndef WIRELESS_H
#define WIRELESS_H


#include <exec/types.h>
#include <devices/sana2.h>
#include <devices/sana2specialstats.h>


/* IEEE 802.11 definitions */

#define IEEE802_11_MAXIDLEN 32
#define IEEE802_11_WEP64LEN 5
#define IEEE802_11_WEP128LEN 13
#define IEEE802_11_WEPKEYCOUNT 4

#define IEEE802_11_FRM_CONTROL  0x00
#define IEEE802_11_FRM_DURATION 0x02
/*#define IEEE802_11_FRM_BODY     0x20*/
#define IEEE802_11_FRM_BODY     0x1e

#define IEEE802_11_FRM_CONTROLB_TYPE 2
#define IEEE802_11_FRM_CONTROLF_TYPE (0x3 << IEEE802_11_FRM_CONTROLB_TYPE)

#define IEEE802_11_FRMTYPE_DATA 2


/* Possible SANA 2 extensions */

#define S2DUPLEX_AUTO 0
#define S2DUPLEX_HALF 1
#define S2DUPLEX_FULL 2

#define S2ENC_NONE 0
#define S2ENC_WEP  1
#define S2ENC_WPA  2

#define S2PORT_AUTO    1   /* eg. device may look for an active link */
#define S2PORT_SAVED   2   /* eg. from ROM */
#define S2PORT_DEFAULT 2   /* eg. from ROM */
#define S2PORT_10BASE2 3
#define S2PORT_TP      4
#define S2PORT_MII     5
#define S2PORT_AUI     6
#define S2PORT_MANAGED 7
#define S2PORT_ADHOC   8

#define P2_GETSIGNALQUALITY 0x8000

struct Sana2SignalQuality
{
   LONG SignalLevel;   /* signal level in dBm */
   LONG NoiseLevel;   /* noise level in dBm */
};


/* Support for SetPrism2Defualts command */

struct WEPKey
{
   UBYTE key[13];
   UBYTE length;
};

#define P2OPT_SSID (TAG_USER + 0)
#define P2OPT_WEPKey (TAG_USER + 1)
#define P2OPT_Encryption (TAG_USER + 4)
#define P2OPT_PortType (TAG_USER + 5)
#define P2OPT_Channel (TAG_USER + 7)


#endif
