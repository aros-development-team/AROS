/*

Copyright (C) 2005-2011 Neil Cafferkey

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


/* IEEE 802.11 definitions */

#define WIFI_MAXIDLEN 32
#define WIFI_WEP64LEN 5
#define WIFI_WEP128LEN 13
#define WIFI_KEYCOUNT 4

#define WIFI_FRM_CONTROL    0x00
#define WIFI_FRM_DURATION   0x02
#define WIFI_FRM_ADDRESS1   0x04
#define WIFI_FRM_ADDRESS2   0x0a
#define WIFI_FRM_ADDRESS3   0x10
#define WIFI_FRM_SEQCONTROL 0x16
#define WIFI_FRM_ADDRESS4   0x18
#define WIFI_FRM_DATA       0x18

#define WIFI_FRM_CONTROLB_VERSION    0
#define WIFI_FRM_CONTROLB_TYPE       2
#define WIFI_FRM_CONTROLB_SUBTYPE    4
#define WIFI_FRM_CONTROLB_TODS       8
#define WIFI_FRM_CONTROLB_FROMDS     9
#define WIFI_FRM_CONTROLB_MOREFRAGS 10
#define WIFI_FRM_CONTROLB_WEP       14
#define WIFI_FRM_CONTROLB_ORDER     15

#define WIFI_FRM_CONTROLF_VERSION   (0x3 << WIFI_FRM_CONTROLB_VERSION)
#define WIFI_FRM_CONTROLF_TYPE      (0x3 << WIFI_FRM_CONTROLB_TYPE)
#define WIFI_FRM_CONTROLF_SUBTYPE   (0xf << WIFI_FRM_CONTROLB_SUBTYPE)
#define WIFI_FRM_CONTROLF_TODS      (0x1 << WIFI_FRM_CONTROLB_TODS)
#define WIFI_FRM_CONTROLF_FROMDS    (0x1 << WIFI_FRM_CONTROLB_FROMDS)
#define WIFI_FRM_CONTROLF_MOREFRAGS (0x1 << WIFI_FRM_CONTROLB_MOREFRAGS)
#define WIFI_FRM_CONTROLF_WEP       (0x1 << WIFI_FRM_CONTROLB_WEP)
#define WIFI_FRM_CONTROLF_ORDER     (0x1 << WIFI_FRM_CONTROLB_ORDER)

#define WIFI_FRMTYPE_MGMT 0
#define WIFI_FRMTYPE_DATA 2

#define WIFI_BEACON_TIMESTAMP    0x0
#define WIFI_BEACON_INTERVAL     0x8
#define WIFI_BEACON_CAPABILITIES 0xa
#define WIFI_BEACON_IES          0xc

#define WIFI_IE_SSID      0
#define WIFI_IE_CHANNEL   3
#define WIFI_IE_RSN      48
#define WIFI_IE_CUSTOM  221


#endif
