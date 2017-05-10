/*

Copyright (C) 2004-2011 Neil Cafferkey

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

#ifndef PRISM2_H
#define PRISM2_H


/* General */
/* ======= */

#define P2_HEADERSIZE      0x2E
#define P2_MAXMCASTENTRIES 16
#define P2_AIROWEPRECLEN   0x1c
#define P2_ALTWEPRECLEN    0x21
#define P2_APRECLEN        0x32 /* ALT... ? */

#define P2_MSGTYPE_RFC1042 1
#define P2_MSGTYPE_TUNNEL  2
#define P2_MSGTYPE_WMPMSG  3
#define P2_MSGTYPE_MGMT    4


/* Registers */
/* ========= */

#define P2_REG_COMMAND   0x0
#define P2_REG_PARAM0    0x2
#define P2_REG_PARAM1    0x4
#define P2_REG_PARAM2    0x6
#define P2_REG_STATUS    0x8
#define P2_REG_RESP0     0xA
#define P2_REG_RESP1     0xC
#define P2_REG_RESP2     0xE
#define P2_REG_INFOFID   0x10
#define P2_REG_CONTROL   0x14
#define P2_REG_SELECT0   0x18
#define P2_REG_SELECT1   0x1A
#define P2_REG_OFFSET0   0x1C
#define P2_REG_OFFSET1   0x1E
#define P2_REG_RXFID     0x20
#define P2_REG_ALLOCFID  0x22
#define P2_REG_TXCMPFID  0x24
#define P2_REG_PCICOR    0x26
#define P2_REG_EVENTS    0x30
#define P2_REG_INTMASK   0x32
#define P2_REG_ACKEVENTS 0x34
#define P2_REG_DATA0     0x36
#define P2_REG_DATA1     0x38
#define P2_REG_AUXPAGE   0x3A
#define P2_REG_AUXOFFSET 0x3C
#define P2_REG_AUXDATA   0x3E


/* Events */
/* ======= */

#define P2_EVENTB_TICK     15
#define P2_EVENTB_RES      14
#define P2_EVENTB_INFODROP 13
#define P2_EVENTB_NOCARD   11
#define P2_EVENTB_DUIFRX   10
#define P2_EVENTB_INFO      7
#define P2_EVENTB_CMD       4
#define P2_EVENTB_ALLOCMEM  3
#define P2_EVENTB_TXFAIL    2
#define P2_EVENTB_TX        1
#define P2_EVENTB_RX        0

#define P2_EVENTF_TICK     (1 << P2_EVENTB_TICK)
#define P2_EVENTF_RES      (1 << P2_EVENTB_RES)
#define P2_EVENTF_INFODROP (1 << P2_EVENTB_INFODROP)
#define P2_EVENTF_NOCARD   (1 << P2_EVENTB_NOCARD)
#define P2_EVENTF_DUIFRX   (1 << P2_EVENTB_DUIFRX)
#define P2_EVENTF_INFO     (1 << P2_EVENTB_INFO)
#define P2_EVENTF_CMD      (1 << P2_EVENTB_CMD)
#define P2_EVENTF_ALLOCMEM (1 << P2_EVENTB_ALLOCMEM)
#define P2_EVENTF_TXFAIL   (1 << P2_EVENTB_TXFAIL)
#define P2_EVENTF_TX       (1 << P2_EVENTB_TX)
#define P2_EVENTF_RX       (1 << P2_EVENTB_RX)


/* Commands */
/* ======== */

#define P2_CMD_INIT     0x0000
#define P2_CMD_ENABLE   0x0001
#define P2_CMD_DISABLE  0x0002
#define P2_CMD_DIAG     0x0003
#define P2_CMD_EXECUTE  0x0004
#define P2_CMD_ALLOCMEM 0x000A
#define P2_CMD_TX       0x000B
#define P2_CMD_NOTIFY   0x0010
#define P2_CMD_INQUIRE  0x0011
#define P2_CMD_ACCESS   0x0021
#define P2_CMD_PROGRAM  0x0022
#define P2_CMD_READEE   0x0030

/* Command flags */

#define P2_CMDB_RECLAIM 8
#define P2_CMDB_WRITE   8

#define P2_CMDF_RECLAIM (1 << P2_CMDB_RECLAIM)
#define P2_CMDF_WRITE   (1 << P2_CMDB_WRITE)


/* LTV records */
/* =========== */

#define P2_REC_PORTTYPE    0xFC00
#define P2_REC_ADDRESS     0xFC01
#define P2_REC_DESIREDSSID 0xFC02
#define P2_REC_OWNCHNL     0xFC03
#define P2_REC_OWNSSID     0xFC04
#define P2_REC_SYSTEMSCALE 0xFC06
#define P2_REC_MAXDATALEN  0xFC07
#define P2_REC_ROAMINGMODE 0xFC2D   /* ??? */
#define P2_REC_MCASTLIST   0xFC80
#define P2_REC_CREATEIBSS  0xFC81
#define P2_REC_RTSTHRESH   0xFC83
#define P2_REC_TXRATE      0xFC84
#define P2_REC_PROMISC     0xFC85
#define P2_REC_PRIIDENTITY 0xFD02
#define P2_REC_NICIDENTITY 0xFD0B
#define P2_REC_STAIDENTITY 0xFD20
#define P2_REC_LINKQUALITY 0xFD43
#define P2_REC_CURTXRATE   0xFD44   /* ??? */
#define P2_REC_HASWEP      0xFD4F

/* Intersil/Symbol-specific records */

#define P2_REC_TXCRYPTKEY     0xFC23
#define P2_REC_CRYPTKEY0      0xFC24
#define P2_REC_CRYPTKEY1      0xFC25
#define P2_REC_CRYPTKEY2      0xFC26
#define P2_REC_CRYPTKEY3      0xFC27
#define P2_REC_ENCRYPTION     0xFC28
#define P2_REC_AUTHTYPE       0xFC2A
#define P2_REC_ROAMINGMODE    0xFC2D
#define P2_REC_DBMOFFSET      0xFC46
#define P2_REC_WPAIE          0xFC48
#define P2_REC_ALTHOSTSCAN    0xFCAB /* ??? */
#define P2_REC_RXMGMTFRAMES   0xFCBB
#define P2_REC_JOIN           0xFCE2
#define P2_REC_HOSTSCAN       0xFCE5
/*#define P2_REC_PRIMARYID      0xFD02*/ /* ??? */

/* Lucent-specific records */

#define P2_REC_ALTENCRYPTION  0xFC20
#define P2_REC_ALTAUTHTYPE    0xFC21
#define P2_REC_DEFLTCRYPTKEYS 0xFCB0
#define P2_REC_ALTTXCRYPTKEY  0xFCB1
#define P2_REC_SCANSSID       0xFCB2
#define P2_REC_ADDDEFAULTTKIPKEY  0xFCB4
#define P2_REC_KEYMGMTSUITE   0xFCB5
#define P2_REC_REMDEFAULTTKIPKEY  0xFCB6
#define P2_REC_ADDMAPPEDTKIPKEY  0xFCB7
#define P2_REC_REMMAPPEDTKIPKEY  0xFCB8
#define P2_REC_SCANCHANNELS   0xFCC2

/* Symbol-specific records */

/*#define P2_REC_SYMBOLIDENTITY 0xFD24*/

/* Aironet-specific records */

#define P2_REC_CAPABILITIES      0xFF00
#define P2_REC_APINFO            0xFF01
#define P2_REC_RADIOINFO         0xFF02
#define P2_REC_RSSI              0xFF04
#define P2_REC_CONFIG            0xFF10
#define P2_REC_SSID              0xFF11
#define P2_REC_APLIST            0xFF12
#define P2_REC_DRVNAME           0xFF13
#define P2_REC_ETHERENCAP        0xFF14
#define P2_REC_WEPTEMP           0xFF15
#define P2_REC_WEPPERM           0xFF16
#define P2_REC_MODULATION        0xFF17
#define P2_REC_OPTIONS           0xFF18
#define P2_REC_ACTUALCONFIG      0xFF20
#define P2_REC_FACTORYCONFIG     0xFF21
#define P2_REC_LEAPUSERNAME      0xFF23
#define P2_REC_LEAPPASSWORD      0xFF24
#define P2_REC_STATUS            0xFF50
#define P2_REC_BEACON_HST        0xFF51
#define P2_REC_BUSYHST           0xFF52
#define P2_REC_RETRIES_HST       0xFF53
#define P2_REC_MIC               0xFF57
#define P2_REC_STATS16           0xFF60
#define P2_REC_STATS16DELTA      0xFF61
#define P2_REC_STATS16DELTACLEAR 0xFF62
#define P2_REC_STATS             0xFF68
#define P2_REC_STATSDELTA        0xFF69
#define P2_REC_STATSDELTACLEAR   0xFF6A
#define P2_REC_ECHOTESTRID       0xFF70
#define P2_REC_ECHOTESTRESULTS   0xFF71
#define P2_REC_BSSLISTFIRST      0xFF72
#define P2_REC_BSSLISTNEXT       0xFF73


/* Information frames */
/* ================== */

#define P2_INFO_NOTIFY          0xF000
#define P2_INFO_COUNTERS        0xF100
#define P2_INFO_SCANRESULTS     0xF101
#define P2_INFO_SCANRESULT      0xF102
#define P2_INFO_HOSTSCANRESULTS 0xF103
/*#define P2_INFO_ALTHOSTSCANRESULTS 0xF104*/
#define P2_INFO_LINKSTATUS      0xF200
/*#define P2_INFO_ASSOCSTATUS     0xF201*/ /* AP only? */


/* Register Details */
/* ================ */

/* Command Register */

#define P2_REG_COMMANDB_BUSY 15

#define P2_REG_COMMANDF_BUSY (1 << P2_REG_COMMANDB_BUSY)

/* Offset Register */

#define P2_REG_OFFSETB_BUSY  15
#define P2_REG_OFFSETB_ERROR 14

#define P2_REG_OFFSETF_BUSY  (1 << P2_REG_OFFSETB_BUSY)
#define P2_REG_OFFSETF_ERROR (1 << P2_REG_OFFSETB_ERROR)

/* Control Register */

#define P2_REG_CONTROLB_AUX 14

#define P2_REG_CONTROLF_AUX (0x3 << P2_REG_CONTROLB_AUX)

#define P2_REG_CONTROL_AUXDISABLED (0 << P2_REG_CONTROLB_AUX)
#define P2_REG_CONTROL_DISABLEAUX  (1 << P2_REG_CONTROLB_AUX)
#define P2_REG_CONTROL_ENABLEAUX   (2 << P2_REG_CONTROLB_AUX)
#define P2_REG_CONTROL_AUXENABLED  (3 << P2_REG_CONTROLB_AUX)


/* Record Details */
/* ============== */

#if 0 /* Contrary to docs in Agere code, these should be bit numbers */
#define P2_REC_ALTAUTHTYPE_OPEN   0
#define P2_REC_ALTAUTHTYPE_SHARED 1
#define P2_REC_ALTAUTHTYPE_LEAP   2
#endif

#define P2_REC_ENCRYPTIONB_ENABLE      0
#define P2_REC_ENCRYPTIONB_NOPLAINTEXT 1
#define P2_REC_ENCRYPTIONB_HOSTENCRYPT 4
#define P2_REC_ENCRYPTIONB_HOSTDECRYPT 7

#define P2_REC_ENCRYPTIONF_ENABLE      (1 << P2_REC_ENCRYPTIONB_ENABLE)
#define P2_REC_ENCRYPTIONF_NOPLAINTEXT (1 << P2_REC_ENCRYPTIONB_NOPLAINTEXT)
#define P2_REC_ENCRYPTIONF_HOSTENCRYPT (1 << P2_REC_ENCRYPTIONB_HOSTENCRYPT)
#define P2_REC_ENCRYPTIONF_HOSTDECRYPT (1 << P2_REC_ENCRYPTIONB_HOSTDECRYPT)

#define P2_REC_AUTHTYPEB_OPEN   0
#define P2_REC_AUTHTYPEB_SHARED 1

#define P2_REC_AUTHTYPEF_OPEN   (1 << P2_REC_AUTHTYPEB_OPEN)
#define P2_REC_AUTHTYPEF_SHARED (1 << P2_REC_AUTHTYPEB_SHARED)

#define P2_REC_CAPABILITIES_OEMADDR 0x40

#define P2_REC_CONFIG_OPMODE    0x2
#define P2_REC_CONFIG_MACADDR   0xA
#define P2_REC_CONFIG_DSCHANNEL 0x66


/* Information Frame Details */
/* ========================= */

#define P2_APREC_CHANNEL 0x0
#define P2_APREC_NOISE 0x2
#define P2_APREC_SIGNAL 0x4
#define P2_APREC_BSSID 0x6
#define P2_APREC_INTERVAL 0xc
#define P2_APREC_CAPABILITIES 0xe
#define P2_APREC_NAMELEN 0x10
#define P2_APREC_NAME 0x12


/* Frame descriptors */
/* ================= */

/* Standard frame */

#define P2_FRM_STATUS       0x0
#define P2_FRM_NOISE        0x6
#define P2_FRM_SIGNAL       0x7
#define P2_FRM_TXCONTROL    0xc
#define P2_FRM_HEADER       0xe
#define P2_FRM_DATALEN      0x2c
#define P2_FRM_ALTTXCONTROL 0x2c
#define P2_FRM_ETHFRAME     0x2e
#define P2_FRM_DATA         0x3c

/* Hermes-II frame */

#define P2_H2FRM_TXCONTROL 0x36
#define P2_H2FRM_DATALEN   0x38
#define P2_H2FRM_ETHFRAME  0x3a
#define P2_H2FRM_DATA      0x48

#define P2_AIROFRM_STATUS  0x4
#define P2_AIROFRM_FRAME  0x14

/* Status field */

#define P2_FRM_STATUSB_BADCRC      0
#define P2_FRM_STATUSB_RETERR      0
#define P2_FRM_STATUSB_BADCRYPT    1
#define P2_FRM_STATUSB_AGEDERR     1
#define P2_FRM_STATUSB_DISCONNECT  2
#define P2_FRM_STATUSB_FORMERR     3
#define P2_FRM_STATUSB_MACPORT     8
#define P2_FRM_STATUSB_PCF        12
#define P2_FRM_STATUSB_MSGTYPE    13

#define P2_FRM_STATUSF_BADCRC     (1 << P2_FRM_STATUSB_BADCRC)
#define P2_FRM_STATUSF_RETERR     (1 << P2_FRM_STATUSB_RETERR)
#define P2_FRM_STATUSF_BADCRYPT   (1 << P2_FRM_STATUSB_BADCRYPT)
#define P2_FRM_STATUSF_AGEDERR    (1 << P2_FRM_STATUSB_AGEDERR)
#define P2_FRM_STATUSF_DISCONNECT (1 << P2_FRM_STATUSB_DISCONNECT)
#define P2_FRM_STATUSF_FORMERR    (1 << P2_FRM_STATUSB_FORMERR)
#define P2_FRM_STATUSF_MACPORT    (7 << P2_FRM_STATUSB_MACPORT)
#define P2_FRM_STATUSF_PCF        (1 << P2_FRM_STATUSB_PCF)
#define P2_FRM_STATUSF_MSGTYPE    (7 << P2_FRM_STATUSB_MSGTYPE)

/* TX Control field */

#define P2_FRM_TXCONTROLB_NATIVE    3
#define P2_FRM_TXCONTROLB_MIC       4 /* HERMES only */
#define P2_FRM_TXCONTROLB_NOENC     7
#define P2_FRM_TXCONTROLB_MICKEYID 11 /* HERMES only */

#define P2_FRM_TXCONTROLF_NATIVE   (1 << P2_FRM_TXCONTROLB_NATIVE)
#define P2_FRM_TXCONTROLF_MIC      (1 << P2_FRM_TXCONTROLB_MIC)
#define P2_FRM_TXCONTROLF_NOENC    (1 << P2_FRM_TXCONTROLB_NOENC)
#define P2_FRM_TXCONTROLF_MICKEYID (3 << P2_FRM_TXCONTROLB_MICKEYID)

#endif
