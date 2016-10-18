#ifndef DEVICES_SANA2WIRELESS_H
#define DEVICES_SANA2WIRELESS_H

/*
    Copyright (C) 2011-2013 Neil Cafferkey
    $Id$

    Desc: Definitions for SANA-II wireless devices
    Lang: english
*/

#include <exec/types.h>
#include <utility/tagitem.h>


/* Constants */
/* ========= */

/* Tags to get and set information */

#define S2INFO_SSID           (TAG_USER + 0)
#define S2INFO_BSSID          (TAG_USER + 1)
#define S2INFO_AuthTypes      (TAG_USER + 2)
#define S2INFO_AssocID        (TAG_USER + 3)
#define S2INFO_Encryption     (TAG_USER + 4)
#define S2INFO_PortType       (TAG_USER + 5)
#define S2INFO_BeaconInterval (TAG_USER + 6)
#define S2INFO_Channel        (TAG_USER + 7)
#define S2INFO_Signal         (TAG_USER + 8)
#define S2INFO_Noise          (TAG_USER + 9)
#define S2INFO_Capabilities   (TAG_USER + 10)
#define S2INFO_InfoElements   (TAG_USER + 11)
#define S2INFO_WPAInfo        (TAG_USER + 12)
#define S2INFO_Band           (TAG_USER + 13)
#define S2INFO_DefaultKeyNo   (TAG_USER + 14)

/* Wireless Commands */

#define S2_GETSIGNALQUALITY 0xc010
#define S2_GETNETWORKS      0xc011
#define S2_SETOPTIONS       0xc012
#define S2_SETKEY           0xc013
#define S2_GETNETWORKINFO   0xc014
#define S2_READMGMT         0xc015
#define S2_WRITEMGMT        0xc016
#define S2_GETRADIOBANDS    0xc017
#define S2_GETCRYPTTYPES    0xc018

/* Encryption types */

#define S2ENC_NONE 0
#define S2ENC_WEP  1
#define S2ENC_TKIP 2
#define S2ENC_CCMP 3

/* Radio modes */

#define S2BAND_A 0
#define S2BAND_B 1
#define S2BAND_G 2
#define S2BAND_N 3

/* Network topologies */

#define S2PORT_MANAGED 7
#define S2PORT_ADHOC   8


/* Structures */
/* ========== */

/* Structure for returning signal quality */

struct Sana2SignalQuality
{
   LONG SignalLevel;   /* signal level in dBm */
   LONG NoiseLevel;   /* noise level in dBm */
};

#endif
