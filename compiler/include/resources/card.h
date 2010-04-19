/*
    Copyright © 2010, The AROS Development Team. All rights reserved.
    $Id:$
*/

#ifndef RESOURCES_CARD_H
#define RESOURCES_CARD_H

#include <exec/nodes.h>
#include <exec/interrupts.h>

#define CARDRESNAME  "card.resource"

struct CardHandle
{
	struct Node       cah_CardNode;
	struct Interrupt *cah_CardRemoved;
	struct Interrupt *cah_CardInserted;
	struct Interrupt *cah_CardStatus;
	UBYTE             cah_CardFlags;
};

struct DeviceTData
{
	ULONG dtd_DTsize;
	ULONG dtd_DTspeed;
	UBYTE dtd_DTtype;
	UBYTE dtd_DTflags;
};

struct CardMemoryMap
{
	UBYTE *cmm_CommonMemory;
	UBYTE *cmm_AttributeMemory;
	UBYTE *cmm_IOMemory;

	/*** V39 ***/
	ULONG cmm_CommonMemSize;
	ULONG cmm_AttributeMemSize;
	ULONG cmm_IOMemSize;
};

/* OwnCard() CardHandle.cah_CardFlags flags */

#define CARDB_RESETREMOVE     0
#define CARDF_RESETREMOVE     (1<<CARDB_RESETREMOVE)

#define CARDB_IFAVAILABLE     1
#define CARDF_IFAVAILABLE     (1<<CARDB_IFAVAILABLE)

#define CARDB_DELAYOWNERSHIP  2
#define CARDF_DELAYOWNERSHIP  (1<<CARDB_DELAYOWNERSHIP)

#define CARDB_POSTSTATUS      3
#define CARDF_POSTSTATUS      (1<<CARDB_POSTSTATUS)

/* ReleaseCreditCard() flags */

#define CARDB_REMOVEHANDLE    0
#define CARDF_REMOVEHANDLE    (1<<CARDB_REMOVEHANDLE)

/* ReadStatus() return flags */

#define CARD_STATUSB_CCDET    6
#define CARD_STATUSF_CCDET    (1<<CARD_STATUSB_CCDET)

#define CARD_STATUSB_BVD1     5
#define CARD_STATUSF_BVD1     (1<<CARD_STATUSB_BVD1)

#define CARD_STATUSB_SC       5
#define CARD_STATUSF_SC       (1<<CARD_STATUSB_SC)

#define CARD_STATUSB_BVD2     4
#define CARD_STATUSF_BVD2     (1<<CARD_STATUSB_BVD2)

#define CARD_STATUSB_DA       4
#define CARD_STATUSF_DA       (1<<CARD_STATUSB_DA)

#define CARD_STATUSB_WR       3
#define CARD_STATUSF_WR       (1<<CARD_STATUSB_WR)

#define CARD_STATUSB_BSY      2
#define CARD_STATUSF_BSY      (1<<CARD_STATUSB_BSY)

#define CARD_STATUSB_IRQ      2
#define CARD_STATUSF_IRQ      (1<<CARD_STATUSB_IRQ)


/* CardProgramVoltage() defines */

#define CARD_VOLTAGE_0V   0
#define CARD_VOLTAGE_5V   1
#define CARD_VOLTAGE_12V  2

/* CardMiscControl() defines */

#define CARD_ENABLEB_DIGAUDIO  1
#define CARD_ENABLEF_DIGAUDIO  (1<<CARD_ENABLEB_DIGAUDIO)

#define CARD_DISABLEB_WP       3
#define CARD_DISABLEF_WP       (1<<CARD_DISABLEB_WP)


/* New CardMiscControl() bits for V39 card.resource */

#define CARD_INTB_SETCLR  7
#define CARD_INTF_SETCLR  (1<<CARD_INTB_SETCLR)

#define CARD_INTB_BVD1    5
#define CARD_INTF_BVD1    (1<<CARD_INTB_BVD1)

#define CARD_INTB_SC      5
#define CARD_INTF_SC      (1<<CARD_INTB_SC)

#define CARD_INTB_BVD2    4
#define CARD_INTF_BVD2    (1<<CARD_INTB_BVD2)

#define CARD_INTB_DA      4
#define CARD_INTF_DA      (1<<CARD_INTB_DA)

#define CARD_INTB_BSY     2
#define CARD_INTF_BSY     (1<<CARD_INTB_BSY)

#define CARD_INTB_IRQ     2
#define CARD_INTF_IRQ     (1<<CARD_INTB_IRQ)


/* CardInterface() defines */

#define CARD_INTERFACE_AMIGA_0  0


#define CISTPL_AMIGAXIP  0x91

struct TP_AmigaXIP
{
	UBYTE TPL_CODE;
	UBYTE TPL_LINK;
	UBYTE TP_XIPLOC[4];
	UBYTE TP_XIPFLAGS;
	UBYTE TP_XIPRESRV;
};

#define XIPFLAGSB_AUTORUN  0
#define XIPFLAGSF_AUTORUN  (1<<XIPFLAGSB_AUTORUN)

#endif
