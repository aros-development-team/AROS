/*
 * $Id$
 */

/*
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

#include <exec/types.h>
#include <exec/resident.h>
#include <exec/io.h>
#include <exec/ports.h>

#include <aros/libcall.h>
#include <aros/macros.h>
#include <aros/io.h>

#include <oop/oop.h>

#include <devices/sana2.h>
#include <devices/sana2specialstats.h>

#include <utility/utility.h>
#include <utility/tagitem.h>
#include <utility/hooks.h>

#include <hidd/pci.h>

#include <proto/oop.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/battclock.h>

#include <hardware/intbits.h>

#include <stdlib.h>

#include "sis900.h"
#include "unit.h"
#include LC_LIBDEFS_FILE

/* A bit fixed linux stuff here :) */

#undef LIBBASE
#define LIBBASE (unit->sis900u_device)

#define net_device SiS900Unit

static struct mii_chip_info {
	const char * name;
	UWORD phy_id0;
	UWORD phy_id1;
	UBYTE phy_types;
#define	HOME 	0x0001
#define LAN	0x0002
#define MIX	0x0003
#define UNKNOWN	0x0
} mii_chip_table[] = {
	{ "SiS 900 Internal MII PHY", 		0x001d, 0x8000, LAN },
	{ "SiS 7014 Physical Layer Solution", 	0x0016, 0xf830, LAN },
	{ "Altimata AC101LF PHY",               0x0022, 0x5520, LAN },
	{ "AMD 79C901 10BASE-T PHY",  		0x0000, 0x6B70, LAN },
	{ "AMD 79C901 HomePNA PHY",		0x0000, 0x6B90, HOME},
	{ "ICS LAN PHY",			0x0015, 0xF440, LAN },
	{ "NS 83851 PHY",			0x2000, 0x5C20, MIX },
	{ "NS 83847 PHY",                       0x2000, 0x5C30, MIX },
	{ "Realtek RTL8201 PHY",		0x0000, 0x8200, LAN },
	{ "VIA 6103 PHY",			0x0101, 0x8f20, LAN },
	{NULL,},
};

#define TIMER_RPROK 3599597124UL

const UBYTE byte_rev_table[256] = {
        0x00, 0x80, 0x40, 0xc0, 0x20, 0xa0, 0x60, 0xe0,
        0x10, 0x90, 0x50, 0xd0, 0x30, 0xb0, 0x70, 0xf0,
        0x08, 0x88, 0x48, 0xc8, 0x28, 0xa8, 0x68, 0xe8,
        0x18, 0x98, 0x58, 0xd8, 0x38, 0xb8, 0x78, 0xf8,
        0x04, 0x84, 0x44, 0xc4, 0x24, 0xa4, 0x64, 0xe4,
        0x14, 0x94, 0x54, 0xd4, 0x34, 0xb4, 0x74, 0xf4,
        0x0c, 0x8c, 0x4c, 0xcc, 0x2c, 0xac, 0x6c, 0xec,
        0x1c, 0x9c, 0x5c, 0xdc, 0x3c, 0xbc, 0x7c, 0xfc,
        0x02, 0x82, 0x42, 0xc2, 0x22, 0xa2, 0x62, 0xe2,
        0x12, 0x92, 0x52, 0xd2, 0x32, 0xb2, 0x72, 0xf2,
        0x0a, 0x8a, 0x4a, 0xca, 0x2a, 0xaa, 0x6a, 0xea,
        0x1a, 0x9a, 0x5a, 0xda, 0x3a, 0xba, 0x7a, 0xfa,
        0x06, 0x86, 0x46, 0xc6, 0x26, 0xa6, 0x66, 0xe6,
        0x16, 0x96, 0x56, 0xd6, 0x36, 0xb6, 0x76, 0xf6,
        0x0e, 0x8e, 0x4e, 0xce, 0x2e, 0xae, 0x6e, 0xee,
        0x1e, 0x9e, 0x5e, 0xde, 0x3e, 0xbe, 0x7e, 0xfe,
        0x01, 0x81, 0x41, 0xc1, 0x21, 0xa1, 0x61, 0xe1,
        0x11, 0x91, 0x51, 0xd1, 0x31, 0xb1, 0x71, 0xf1,
        0x09, 0x89, 0x49, 0xc9, 0x29, 0xa9, 0x69, 0xe9,
        0x19, 0x99, 0x59, 0xd9, 0x39, 0xb9, 0x79, 0xf9,
        0x05, 0x85, 0x45, 0xc5, 0x25, 0xa5, 0x65, 0xe5,
        0x15, 0x95, 0x55, 0xd5, 0x35, 0xb5, 0x75, 0xf5,
        0x0d, 0x8d, 0x4d, 0xcd, 0x2d, 0xad, 0x6d, 0xed,
        0x1d, 0x9d, 0x5d, 0xdd, 0x3d, 0xbd, 0x7d, 0xfd,
        0x03, 0x83, 0x43, 0xc3, 0x23, 0xa3, 0x63, 0xe3,
        0x13, 0x93, 0x53, 0xd3, 0x33, 0xb3, 0x73, 0xf3,
        0x0b, 0x8b, 0x4b, 0xcb, 0x2b, 0xab, 0x6b, 0xeb,
        0x1b, 0x9b, 0x5b, 0xdb, 0x3b, 0xbb, 0x7b, 0xfb,
        0x07, 0x87, 0x47, 0xc7, 0x27, 0xa7, 0x67, 0xe7,
        0x17, 0x97, 0x57, 0xd7, 0x37, 0xb7, 0x77, 0xf7,
        0x0f, 0x8f, 0x4f, 0xcf, 0x2f, 0xaf, 0x6f, 0xef,
        0x1f, 0x9f, 0x5f, 0xdf, 0x3f, 0xbf, 0x7f, 0xff,
};

#define CRCPOLY_LE 0xedb88320

ULONG crc32_le(ULONG crc, unsigned char const *p, int len)
{
    int i;
    while (len--) {
        crc ^= *p++;
        for (i = 0; i < 8; i++)
            crc = (crc >> 1) ^ ((crc & 1) ? CRCPOLY_LE : 0);
    }
    return crc;
}

static inline UBYTE bitrev8(UBYTE byte)
{
        return byte_rev_table[byte];
}

UWORD bitrev16(UWORD x)
{
        return (bitrev8(x & 0xff) << 8) | bitrev8(x >> 8);
}

/**
 * bitrev32 - reverse the order of bits in a u32 value
 * @x: value to be bit-reversed
 */
ULONG bitrev32(ULONG x)
{
        return (bitrev16(x & 0xffff) << 16) | bitrev16(x >> 16);
}

#define ether_crc(length, data)    bitrev32(crc32_le(~0, data, length))


static ULONG usec2tick(ULONG usec)
{
    ULONG ret, timer_rpr = TIMER_RPROK;
    asm volatile("movl $0,%%eax; divl %2":"=a"(ret):"d"(usec),"m"(timer_rpr));
    return ret;
}

void udelay(LONG usec)
{
    int oldtick, tick;
    usec = usec2tick(usec);

    BYTEOUT(0x43, 0x80);
    oldtick = BYTEIN(0x42);
    oldtick += BYTEIN(0x42) << 8;

    while (usec > 0)
    {
        BYTEOUT(0x43, 0x80);
        tick = BYTEIN(0x42);
        tick += BYTEIN(0x42) << 8;

        usec -= (oldtick - tick);
        if (tick > oldtick) usec -= 0x10000;
        oldtick = tick;
    }
}

/* sis900_mcast_bitnr - compute hashtable index 
 *	@addr: multicast address
 *	@revision: revision id of chip
 *
 *	SiS 900 uses the most sigificant 7 bits to index a 128 bits multicast
 *	hash table, which makes this function a little bit different from other drivers
 *	SiS 900 B0 & 635 M/B uses the most significat 8 bits to index 256 bits
 *   	multicast hash table. 
 */
static inline UWORD sis900_mcast_bitnr(UBYTE *addr, UBYTE revision)
{

	ULONG crc = ether_crc(6, addr);

	/* leave 8 or 7 most siginifant bits */
	if ((revision >= SIS635A_900_REV) || (revision == SIS900B_900_REV))
		return ((int)(crc >> 24));
	else
		return ((int)(crc >> 25));
}

/* Delay between EEPROM clock transitions. */
#define eeprom_delay()  LONGIN(ee_addr)

static UWORD read_eeprom(long ioaddr, int location)
{
	int i;
	UWORD retval = 0;
	IPTR ee_addr = ioaddr + mear;
	ULONG read_cmd = location | EEread;

	LONGOUT(ee_addr, 0);
	eeprom_delay();
	LONGOUT(ee_addr, EECS);
	eeprom_delay();

	/* Shift the read command (9) bits out. */
	for (i = 8; i >= 0; i--) {
		ULONG dataval = (read_cmd & (1 << i)) ? EEDI | EECS : EECS;
		LONGOUT(ee_addr, dataval);
		eeprom_delay();
		LONGOUT(ee_addr, dataval | EECLK);
		eeprom_delay();
	}
	LONGOUT(ee_addr, EECS);
	eeprom_delay();

	/* read the 16-bits data in */
	for (i = 16; i > 0; i--) {
		LONGOUT(ee_addr, EECS);
		eeprom_delay();
		LONGOUT(ee_addr, EECS | EECLK);
		eeprom_delay();
		retval = (retval << 1) | ((LONGIN(ee_addr) & EEDO) ? 1 : 0);
		eeprom_delay();
	}

	/* Terminate the EEPROM access. */
	LONGOUT(ee_addr, 0);
	eeprom_delay();

	return (retval);
}

/* Read and write the MII management registers using software-generated
   serial MDIO protocol. Note that the command bits and data bits are
   send out separately */
#define mdio_delay()    LONGIN(mdio_addr)

static void mdio_idle(long mdio_addr)
{
	LONGOUT(mdio_addr, MDIO | MDDIR);
	mdio_delay();
	LONGOUT(mdio_addr, MDIO | MDDIR | MDC);
}

/* Syncronize the MII management interface by shifting 32 one bits out. */
static void mdio_reset(long mdio_addr)
{
	int i;

	for (i = 31; i >= 0; i--) {
		LONGOUT(mdio_addr, MDDIR | MDIO);
		mdio_delay();
		LONGOUT(mdio_addr, MDDIR | MDIO | MDC);
		mdio_delay();
	}
	return;
}

/**
 *	mdio_read - read MII PHY register
 *	@unit: the net device to read
 *	@phy_id: the phy address to read
 *	@location: the phy regiester id to read
 *
 *	Read MII registers through MDIO and MDC
 *	using MDIO management frame structure and protocol(defined by ISO/IEC).
 *	Please see SiS7014 or ICS spec
 */
int mdio_read(struct net_device *unit, int phy_id, int location)
{
	long mdio_addr = unit->sis900u_BaseMem + mear;
	int mii_cmd = MIIread | (phy_id << MIIpmdShift) | (location << MIIregShift);
	UWORD retval = 0;
	int i;

	mdio_reset(mdio_addr);
	mdio_idle(mdio_addr);

	for (i = 15; i >= 0; i--) {
		int dataval = (mii_cmd & (1 << i)) ? MDDIR | MDIO : MDDIR;
		LONGOUT(mdio_addr, dataval);
		mdio_delay();
		LONGOUT(mdio_addr, dataval | MDC);
		mdio_delay();
	}

	/* Read the 16 data bits. */
	for (i = 16; i > 0; i--) {
		LONGOUT(mdio_addr, 0);
		mdio_delay();
		retval = (retval << 1) | ((LONGIN(mdio_addr) & MDIO) ? 1 : 0);
		LONGOUT(mdio_addr, MDC);
		mdio_delay();
	}
	LONGOUT(mdio_addr, 0x00);

	return retval;
}

/**
 *	mdio_write - write MII PHY register
 *	@unit: the net device to write
 *	@phy_id: the phy address to write
 *	@location: the phy regiester id to write
 *	@value: the register value to write with
 *
 *	Write MII registers with @value through MDIO and MDC
 *	using MDIO management frame structure and protocol(defined by ISO/IEC)
 *	please see SiS7014 or ICS spec
 */
static void mdio_write(struct net_device *unit, int phy_id, int location, int value)
{
	long mdio_addr = unit->sis900u_BaseMem + mear;
	int mii_cmd = MIIwrite | (phy_id << MIIpmdShift) | (location << MIIregShift);
	int i;

	mdio_reset(mdio_addr);
	mdio_idle(mdio_addr);

	/* Shift the command bits out. */
	for (i = 15; i >= 0; i--) {
		int dataval = (mii_cmd & (1 << i)) ? MDDIR | MDIO : MDDIR;
		BYTEOUT(mdio_addr, dataval);
		mdio_delay();
		BYTEOUT(mdio_addr, dataval | MDC);
		mdio_delay();
	}
	mdio_delay();

	/* Shift the value bits out. */
	for (i = 15; i >= 0; i--) {
		int dataval = (value & (1 << i)) ? MDDIR | MDIO : MDDIR;
		LONGOUT(mdio_addr, dataval);
		mdio_delay();
		LONGOUT(mdio_addr, dataval | MDC);
		mdio_delay();
	}
	mdio_delay();

	/* Clear out extra bits. */
	for (i = 2; i > 0; i--) {
		BYTEOUT(mdio_addr, 0);
		mdio_delay();
		BYTEOUT(mdio_addr, MDC);
		mdio_delay();
	}
	LONGOUT(mdio_addr, 0x00);

	return;
}

/**
 * 	sis900_set_capability - set the media capability of network adapter.
 *	@unit : the net device to probe for
 *	@phy : default PHY
 *
 *	Set the media capability of network adapter according to
 *	mii status register. It's necessary before auto-negotiate.
 */
static void sis900_set_capability(struct net_device *unit, struct mii_phy *phy)
{
	UWORD cap;

D(bug("[%s]  sis900_set_capability(phy:%d)\n", unit->sis900u_name, phy->phy_addr));

	mdio_read(unit, phy->phy_addr, MII_STATUS);
	mdio_read(unit, phy->phy_addr, MII_STATUS);
	
	cap = MII_NWAY_CSMA_CD |
		((phy->status & MII_STAT_CAN_TX_FDX)? MII_NWAY_TX_FDX:0) |
		((phy->status & MII_STAT_CAN_TX)    ? MII_NWAY_TX:0) |
		((phy->status & MII_STAT_CAN_T_FDX) ? MII_NWAY_T_FDX:0)|
		((phy->status & MII_STAT_CAN_T)     ? MII_NWAY_T:0);

	mdio_write(unit, phy->phy_addr, MII_ANADV, cap);
}

/**
 *	sis900_auto_negotiate - Set the Auto-Negotiation Enable/Reset bit.
 *	@unit: the net device to read mode for
 *	@phy_addr: mii phy address
 *
 *	If the adapter is link-on, set the auto-negotiate enable/reset bit.
 *	autong_complete should be set to 0 when starting auto-negotiation.
 *	autong_complete should be set to 1 if we didn't start auto-negotiation.
 *	sis900_timer will wait for link on again if autong_complete = 0.
 */
static void sis900_auto_negotiate(struct net_device *unit, int phy_addr)
{
	int i = 0;
	ULONG status;

D(bug("[%s]  sis900_auto_negotiate(phy:%d)\n", unit->sis900u_name, phy_addr));
	
	while (i++ < 2)
		status = mdio_read(unit, phy_addr, MII_STATUS);

	if (!(status & MII_STAT_LINK)){
		//if(netif_msg_link(sis_priv))
D(bug("[%s]: sis900_auto_negotiate: Media Link Off\n", unit->sis900u_name));
		unit->autong_complete = 1;
		netif_carrier_off(unit);
		return;
	}

	/* (Re)start AutoNegotiate */
	mdio_write(unit, phy_addr, MII_CONTROL, MII_CNTL_AUTO | MII_CNTL_RST_AUTO);
	unit->autong_complete = 0;
}

/**
 *	sis900_default_phy - Select default PHY for sis900 mac.
 *	@unit: the net device to probe for
 *
 *	Select first detected PHY with link as default.
 *	If no one is link on, select PHY whose types is HOME as default.
 *	If HOME doesn't exist, select LAN.
 */
static UWORD sis900_default_phy(struct net_device *unit)
{
 	struct mii_phy *phy = NULL, *phy_home = NULL, *default_phy = NULL, *phy_lan = NULL;
	UWORD status;

D(bug("[%s]  sis900_default_phy()\n", unit->sis900u_name));

    for (phy = unit->first_mii; phy; phy = phy->next)
    {
        status = mdio_read(unit, phy->phy_addr, MII_STATUS);
		status = mdio_read(unit, phy->phy_addr, MII_STATUS);

		/* Link ON & Not select default PHY & not ghost PHY */
		if ((status & MII_STAT_LINK) && !default_phy && (phy->phy_types != UNKNOWN))
		 	default_phy = phy;
		else {
			status = mdio_read(unit, phy->phy_addr, MII_CONTROL);
			mdio_write(unit, phy->phy_addr, MII_CONTROL, status | MII_CNTL_AUTO | MII_CNTL_ISOLATE);
			if (phy->phy_types == HOME)
				phy_home = phy;
			else if(phy->phy_types == LAN)
				phy_lan = phy;
        }
	}

	if (!default_phy && phy_home)
		default_phy = phy_home;
	else if (!default_phy && phy_lan)
		default_phy = phy_lan;
	else if (!default_phy)
		default_phy = unit->first_mii;

	if (unit->mii != default_phy) {
		unit->mii = default_phy;
		unit->cur_phy = default_phy->phy_addr;
D(bug("[%s]: sis900_default_phy: Using transceiver found at address %d as default\n", unit->sis900u_name, unit->cur_phy));
	}

//	unit->mii_info.phy_id = unit->cur_phy;

	status = mdio_read(unit, unit->cur_phy, MII_CONTROL);
	status &= (~MII_CNTL_ISOLATE);

	mdio_write(unit, unit->cur_phy, MII_CONTROL, status);	
	status = mdio_read(unit, unit->cur_phy, MII_STATUS);
	status = mdio_read(unit, unit->cur_phy, MII_STATUS);

	return status;
}

/**
 *	sis900_reset_phy - reset sis900 mii phy.
 *	@unit: the net device to write
 *	@phy_addr: default phy address
 *
 *	Some specific phy can't work properly without reset.
 *	This function will be called during initialization and
 *	link status change from ON to DOWN.
 */
static UWORD sis900_reset_phy(struct net_device *unit, int phy_addr)
{
	int i = 0;
	UWORD status;

D(bug("[%s]  sis900_reset_phy(phy:%d)\n", unit->sis900u_name, phy_addr));

	while (i++ < 2)
		status = mdio_read(unit, phy_addr, MII_STATUS);

	mdio_write(unit, phy_addr, MII_CONTROL, MII_CNTL_RESET);

	return status;
}

/**
 *	sis900_mii_probe - Probe MII PHY for sis900
 *	@unit: the net device to probe for
 *	
 *	Search for total of 32 possible mii phy addresses.
 *	Identify and set current phy if found one,
 *	return error if it failed to found.
 */
static int sis900_mii_probe(struct net_device *unit)
{
	UWORD poll_bit = MII_STAT_LINK, status = 0;
/* TODO: Replace jiffies */
#define jiffies 10
	// unsigned long timeout = jiffies + 5 * HZ;
	int phy_addr;

D(bug("[%s]  sis900_mii_probe()\n", unit->sis900u_name));

	unit->mii = NULL;

	/* search for total of 32 possible mii phy addresses */
	for (phy_addr = 0; phy_addr < 32; phy_addr++) {	
		struct mii_phy * mii_phy = NULL;
		UWORD mii_status;
		int i;

		mii_phy = NULL;
		for(i = 0; i < 2; i++)
			mii_status = mdio_read(unit, phy_addr, MII_STATUS);

		if (mii_status == 0xffff || mii_status == 0x0000) {
D(bug("[%s]: sis900_mii_probe: MII at address %d not accessible\n", unit->sis900u_name, phy_addr));
			continue;
		}

		if ((mii_phy = AllocMem(sizeof(struct mii_phy), MEMF_PUBLIC | MEMF_CLEAR)) == NULL) {
D(bug("[%s]: sis900_mii_probe: MII %d: Cannot allocate mem for struct mii_phy\n", unit->sis900u_name, phy_addr));
			mii_phy = unit->first_mii;
			while (mii_phy) {
				struct mii_phy *phy;
				phy = mii_phy;
				mii_phy = mii_phy->next;
				FreeMem(phy, sizeof(struct mii_phy));
			}
			return 0;
		}
		
		mii_phy->phy_id0 = mdio_read(unit, phy_addr, MII_PHY_ID0);
		mii_phy->phy_id1 = mdio_read(unit, phy_addr, MII_PHY_ID1);		
		mii_phy->phy_addr = phy_addr;
		mii_phy->status = mii_status;
		mii_phy->next = unit->mii;
		unit->mii = mii_phy;
		unit->first_mii = mii_phy;

		for (i = 0; mii_chip_table[i].phy_id1; i++)
			if ((mii_phy->phy_id0 == mii_chip_table[i].phy_id0 ) &&
			    ((mii_phy->phy_id1 & 0xFFF0) == mii_chip_table[i].phy_id1)){
				mii_phy->phy_types = mii_chip_table[i].phy_types;
				if (mii_chip_table[i].phy_types == MIX)
					mii_phy->phy_types =
					    (mii_status & (MII_STAT_CAN_TX_FDX | MII_STAT_CAN_TX)) ? LAN : HOME;
D(bug("[%s]: sis900_mii_probe: %s transceiver found at address %d.\n", unit->sis900u_name, mii_chip_table[i].name, phy_addr));
				break;
			}
			
		if( !mii_chip_table[i].phy_id1 ) {
D(bug("[%s]: sis900_mii_probe: Unknown PHY transceiver found at address %d.\n", unit->sis900u_name, phy_addr));
			mii_phy->phy_types = UNKNOWN;
		}
	}
	
	if (unit->mii == NULL) {
D(bug("[%s]: sis900_mii_probe: No MII transceivers found!\n", unit->sis900u_name));
		return 0;
	}

	/* select default PHY for mac */
	unit->mii = NULL;
	sis900_default_phy(unit);

	/* Reset phy if default phy is internal sis900 */
    if ((unit->mii->phy_id0 == 0x001D) &&
    ((unit->mii->phy_id1&0xFFF0) == 0x8000))
        status = sis900_reset_phy(unit, unit->cur_phy);

    /* workaround for ICS1893 PHY */
    if ((unit->mii->phy_id0 == 0x0015) &&
        ((unit->mii->phy_id1&0xFFF0) == 0xF440))
            mdio_write(unit, unit->cur_phy, 0x0018, 0xD200);

	if(status & MII_STAT_LINK){
		while (poll_bit) {
//			yield();

			poll_bit ^= (mdio_read(unit, unit->cur_phy, MII_STATUS) & poll_bit);
//			if (time_after_eq(jiffies, timeout)) {
//D(bug("[%s]: sis900_mii_probe: reset phy and link down now\n", unit->sis900u_name));
/* TODO: Return -ETIME! */
				//return -ETIME;
//                return 0;
//			}
		}
	}

	if (unit->sis900u_RevisionID == SIS630E_900_REV) {
		/* SiS 630E has some bugs on default value of PHY registers */
		mdio_write(unit, unit->cur_phy, MII_ANADV, 0x05e1);
		mdio_write(unit, unit->cur_phy, MII_CONFIG1, 0x22);
		mdio_write(unit, unit->cur_phy, MII_CONFIG2, 0xff00);
		mdio_write(unit, unit->cur_phy, MII_MASK, 0xffc0);
		//mdio_write(unit, unit->cur_phy, MII_CONTROL, 0x1000);	
	}

	if (unit->mii->status & MII_STAT_LINK)
		netif_carrier_on(unit);
	else
		netif_carrier_off(unit);

	return 1;
}

#if 0
static void sis900func_start_rx(struct net_device *unit)
{
D(bug("[%s]: sis900func_start_rx\n", unit->sis900u_name));
    // Already running? Stop it.
/* TODO: Handle starting/stopping Rx */
}

static void sis900func_stop_rx(struct net_device *unit)
{
D(bug("[%s]: sis900func_stop_rx\n", unit->sis900u_name));
/* TODO: Handle starting/stopping Rx */
}

static void sis900func_start_tx(struct net_device *unit)
{
D(bug("[%s]: sis900func_start_tx()\n", unit->sis900u_name));
/* TODO: Handle starting/stopping Tx */
}

static void sis900func_stop_tx(struct net_device *unit)
{
D(bug("[%s]: sis900func_stop_tx()\n", unit->sis900u_name));
/* TODO: Handle starting/stopping Tx */
}

static void sis900func_txrx_reset(struct net_device *unit)
{
D(bug("[%s]: sis900func_txrx_reset()\n", unit->sis900u_name));
}
#endif

/**
 *	sis900_get_mac_addr - Get MAC address for stand alone SiS900 model
 *	@unit: the net device to get address for 
 *
 *	Older SiS900 and friends, use EEPROM to store MAC address.
 *	MAC address is read from read_eeprom() into @net_dev->dev_addr.
 */
static int sis900_get_mac_addr(struct net_device *unit)
{
	UWORD signature;
	int i;

D(bug("[%s]  sis900_get_mac_addr()\n", unit->sis900u_name));
	
	/* check to see if we have sane EEPROM */
	signature = (UWORD) read_eeprom(unit->sis900u_BaseMem, EEPROMSignature);    
	if (signature == 0xffff || signature == 0x0000) {
D(bug("[%s]: sis900_get_mac_addr:  Error EERPOM read %x\n", unit->sis900u_name, signature));
		return 0;
	}

	/* get MAC address from EEPROM */
	for (i = 0; i < 3; i++)
    {
	        unit->sis900u_org_addr[i] = (UWORD)read_eeprom(unit->sis900u_BaseMem, i + EEPROMMACAddr);
    }

	return 1;
}

/**
 *	sis630e_get_mac_addr - Get MAC address for SiS630E model
 *	@unit: the net device to get address for 
 *
 *	SiS630E model, use APC CMOS RAM to store MAC address.
 *	APC CMOS RAM is accessed through ISA bridge.
 *	MAC address is read into @net_dev->dev_addr.
 */
static int sis630e_get_mac_addr(struct net_device *unit)
{
//	struct pci_dev *isa_bridge = NULL;
	// UBYTE reg;
	int i;

D(bug("[%s]  sis630e_get_mac_addr()\n", unit->sis900u_name));
	
//	isa_bridge = pci_get_device(PCI_VENDOR_ID_SI, 0x0008, isa_bridge);
//	if (!isa_bridge)
//		isa_bridge = pci_get_device(PCI_VENDOR_ID_SI, 0x0018, isa_bridge);
//	if (!isa_bridge) {
//		printk(KERN_WARNING "%s: Can not find ISA bridge\n",
//		       pci_name(pci_dev));
//		return 0;
//	}
//	pci_read_config_byte(isa_bridge, 0x48, &reg);
//	pci_write_config_byte(isa_bridge, 0x48, reg | 0x40);

	for (i = 0; i < 3; i++) {
		BYTEOUT(0x70, 0x09 + (i * 2));
		unit->sis900u_org_addr[i] = 0x0000 + (BYTEIN(0x71) << 8); 
		BYTEOUT(0x70, 0x09 + (i * 2) + 1);
		unit->sis900u_org_addr[i] |= BYTEIN(0x71); 
	}
//	pci_write_config_byte(isa_bridge, 0x48, reg & ~0x40);
//	pci_dev_put(isa_bridge);

	return 1;
}

/**
 *	sis635_get_mac_addr - Get MAC address for SIS635 model
 *	@unit: the net device to get address for 
 *
 *	SiS635 model, set MAC Reload Bit to load Mac address from APC
 *	to rfdr. rfdr is accessed through rfcr. MAC address is read into 
 *	@net_dev->dev_addr.
 */
static int sis635_get_mac_addr(struct net_device *unit)
{
	long ioaddr = unit->sis900u_BaseMem;
	ULONG rfcrSave;
	ULONG i;

D(bug("[%s]  sis635_get_mac_addr()\n", unit->sis900u_name));
	
	rfcrSave = LONGIN(rfcr + ioaddr);

	LONGOUT(ioaddr + cr, rfcrSave | RELOAD);
	LONGOUT(ioaddr + cr, 0);

	/* disable packet filtering before setting filter */
	LONGOUT(rfcr + ioaddr, rfcrSave & ~RFEN);

	/* load MAC addr to filter data register */
	for (i = 0 ; i < 3 ; i++) {
		LONGOUT(ioaddr + rfcr, i << RFADDR_shift);
		unit->sis900u_org_addr[i] = WORDIN(ioaddr + rfdr);
	}

	/* enable packet filtering */
	LONGOUT(rfcr + ioaddr, rfcrSave | RFEN);

	return 1;
}

/**
 *	sis96x_get_mac_addr - Get MAC address for SiS962 or SiS963 model
 *	@unit: the net device to get address for 
 *
 *	SiS962 or SiS963 model, use EEPROM to store MAC address. And EEPROM 
 *	is shared by
 *	LAN and 1394. When access EEPROM, send EEREQ signal to hardware first 
 *	and wait for EEGNT. If EEGNT is ON, EEPROM is permitted to be access 
 *	by LAN, otherwise is not. After MAC address is read from EEPROM, send
 *	EEDONE signal to refuse EEPROM access by LAN. 
 *	The EEPROM map of SiS962 or SiS963 is different to SiS900. 
 *	The signature field in SiS962 or SiS963 spec is meaningless. 
 *	MAC address is read into @net_dev->dev_addr.
 */
static int sis96x_get_mac_addr(struct net_device *unit)
{
	long ioaddr = unit->sis900u_BaseMem;
	long ee_addr = ioaddr + mear;
	ULONG waittime = 0;
	int i;

D(bug("[%s]  sis96x_get_mac_addr()\n", unit->sis900u_name));

	LONGOUT(ee_addr, EEREQ);
	while(waittime < 2000) {
		if(LONGIN(ee_addr) & EEGNT) {

			/* get MAC address from EEPROM */
			for (i = 0; i < 3; i++)
			        unit->sis900u_org_addr[i] = read_eeprom(ioaddr, i+EEPROMMACAddr);

			LONGOUT(ee_addr, EEDONE);
			return 1;
		} else {
			udelay(1);	
			waittime ++;
		}
	}
	LONGOUT(ee_addr, EEDONE);
	return 0;
}

/*
 * sis900func_set_multicast: unit->set_multicast function
 * Called with unit->xmit_lock held.
 */
void sis900func_set_multicast(struct net_device *unit)
{
    ULONG addr[2];
    ULONG mask[2];
    // ULONG pff;

D(bug("[%s]: sis900func_set_multicast()\n", unit->sis900u_name));

    memset(addr, 0, sizeof(addr));
    memset(mask, 0, sizeof(mask));
}

void sis900func_deinitialize(struct net_device *unit)
{
D(bug("[%s]  sis900func_deinitialize()\n", unit->sis900u_name));
}

void sis900func_initialize(struct net_device *unit)
{
    int ret;
    // int i, config1;

D(bug("[%s]  sis900func_initialize()\n", unit->sis900u_name));
	
	ret = 0;
	if (unit->sis900u_RevisionID == SIS630E_900_REV)
		ret = sis630e_get_mac_addr(unit);
	else if ((unit->sis900u_RevisionID > 0x81) && (unit->sis900u_RevisionID <= 0x90) )
		ret = sis635_get_mac_addr(unit);
	else if (unit->sis900u_RevisionID == SIS96x_900_REV)
		ret = sis96x_get_mac_addr(unit);
	else
		ret = sis900_get_mac_addr(unit);

	if (ret == 0) {
D(bug("[%s]: Cannot read MAC address.\n", unit->sis900u_name));
		return;
	}
    
    unit->sis900u_dev_addr[0] = unit->sis900u_org_addr[0] & 0xff;
    unit->sis900u_dev_addr[1] = (unit->sis900u_org_addr[0] >> 8) & 0xff;

    unit->sis900u_dev_addr[2] = unit->sis900u_org_addr[1] & 0xff;
    unit->sis900u_dev_addr[3] = (unit->sis900u_org_addr[1] >> 8) & 0xff;

    unit->sis900u_dev_addr[4] = unit->sis900u_org_addr[2] & 0xff;
    unit->sis900u_dev_addr[5] = (unit->sis900u_org_addr[2] >> 8) & 0xff;

D(bug("[%s]: MAC Address %02x:%02x:%02x:%02x:%02x:%02x\n", unit->sis900u_name,
            unit->sis900u_dev_addr[0], unit->sis900u_dev_addr[1], unit->sis900u_dev_addr[2],
            unit->sis900u_dev_addr[3], unit->sis900u_dev_addr[4], unit->sis900u_dev_addr[5]));
	
	/* 630ET : set the mii access mode as software-mode */
	if (unit->sis900u_RevisionID == SIS630ET_900_REV)
	{
		LONGOUT(unit->sis900u_BaseMem + cr, ACCESSMODE | LONGIN(unit->sis900u_BaseMem + cr));
	}

	/* probe for mii transceiver */
	if (sis900_mii_probe(unit) == 0) {
D(bug("[%s]: Error probing MII device.\n", unit->sis900u_name));
		return;
	}
}

static void sis900func_drain_tx(struct net_device *unit)
{
    int i;
    for (i = 0; i < NUM_TX_DESC; i++) {
/* TODO: sis900func_drain_tx does nothing atm. */
    }
}

static void sis900func_drain_rx(struct net_device *unit)
{
    int i;
    for (i = 0; i < NUM_RX_DESC; i++) {
/* TODO: sis900func_drain_rx does nothing atm. */
    }
}

static void drain_ring(struct net_device *unit)
{
    sis900func_drain_tx(unit);
    sis900func_drain_rx(unit);
}

static int request_irq(struct net_device *unit)
{
D(bug("[%s]: request_irq()\n", unit->sis900u_name));

    if (!unit->sis900u_IntsAdded)
    {
        AddIntServer(INTB_KERNEL + unit->sis900u_IRQ,
            &unit->sis900u_irqhandler);
        AddIntServer(INTB_VERTB, &unit->sis900u_touthandler);
        unit->sis900u_IntsAdded = TRUE;
    }

    return 0;
}

static void free_irq(struct net_device *unit)
{
    if (unit->sis900u_IntsAdded)
    {
        RemIntServer(INTB_KERNEL + unit->sis900u_IRQ,
            &unit->sis900u_irqhandler);
        RemIntServer(INTB_VERTB, &unit->sis900u_touthandler);
        unit->sis900u_IntsAdded = FALSE;
    }
}

void sis900func_set_mac(struct net_device *unit)
{
   // int i;

D(bug("[%s]: sis900func_set_mac()\n", unit->sis900u_name));

/*	BYTEOUT(base + RTLr_Cfg9346, 0xc0);
    LONGOUT(base + RTLr_MAC0 + 0, (
													( unit->sis900u_dev_addr[3] << 24 ) |
													( unit->sis900u_dev_addr[2] << 16 ) |
													( unit->sis900u_dev_addr[1] << 8 ) |
													  unit->sis900u_dev_addr[0]
												 ));

    LONGOUT(base + RTLr_MAC0 + 4, (
													( unit->sis900u_dev_addr[5] << 8 ) |
													  unit->sis900u_dev_addr[4]
												 ));
	BYTEOUT(base + RTLr_Cfg9346, 0x00);*/
}

/**
 *	sis900_reset - Reset sis900 MAC 
 *	@unit: the net device to reset
 *
 *	reset sis900 MAC and wait until finished
 *	reset through command register
 *	change backoff algorithm for 900B0 & 635 M/B
 */
static void sis900_reset(struct net_device *unit)
{
	long ioaddr = unit->sis900u_BaseMem;
	int i = 0;
	ULONG status = TxRCMP | RxRCMP;

D(bug("[%s]: sis900_reset()\n", unit->sis900u_name));

	LONGOUT(ioaddr + ier, 0);
	LONGOUT(ioaddr + imr, 0);
	LONGOUT(ioaddr + rfcr, 0);

	LONGOUT(ioaddr + cr, RxRESET | TxRESET | RESET | LONGIN(ioaddr + cr));
	
	/* Check that the chip has finished the reset. */
	while (status && (i++ < 1000)) {
		status ^= (LONGIN(isr + ioaddr) & status);
	}

	if ( (unit->sis900u_RevisionID >= SIS635A_900_REV) || (unit->sis900u_RevisionID == SIS900B_900_REV) )
		LONGOUT(ioaddr + cfg, PESEL | RND_CNT);
	else
		LONGOUT(ioaddr + cfg, PESEL);
}

/**
 *	sis630_set_eq - set phy equalizer value for 630 LAN
 *	@unit: the net device to set equalizer value
 *	@revision: 630 LAN revision number
 *
 *	630E equalizer workaround rule(Cyrus Huang 08/15)
 *	PHY register 14h(Test)
 *	Bit 14: 0 -- Automatically dectect (default)
 *		1 -- Manually set Equalizer filter
 *	Bit 13: 0 -- (Default)
 *		1 -- Speed up convergence of equalizer setting
 *	Bit 9 : 0 -- (Default)
 *		1 -- Disable Baseline Wander
 *	Bit 3~7   -- Equalizer filter setting
 *	Link ON: Set Bit 9, 13 to 1, Bit 14 to 0
 *	Then calculate equalizer value
 *	Then set equalizer value, and set Bit 14 to 1, Bit 9 to 0
 *	Link Off:Set Bit 13 to 1, Bit 14 to 0
 *	Calculate Equalizer value:
 *	When Link is ON and Bit 14 is 0, SIS900PHY will auto-dectect proper equalizer value.
 *	When the equalizer is stable, this value is not a fixed value. It will be within
 *	a small range(eg. 7~9). Then we get a minimum and a maximum value(eg. min=7, max=9)
 *	0 <= max <= 4  --> set equalizer to max
 *	5 <= max <= 14 --> set equalizer to max+1 or set equalizer to max+2 if max == min
 *	max >= 15      --> set equalizer to max+5 or set equalizer to max+6 if max == min
 */
static void sis630_set_eq(struct net_device *unit, UBYTE revision)
{
	UWORD reg14h, eq_value=0, max_value=0, min_value=0;
	int i, maxcount=10;

D(bug("[%s]: sis630_set_eq()\n", unit->sis900u_name));
	
	if ( !(revision == SIS630E_900_REV || revision == SIS630EA1_900_REV ||
	       revision == SIS630A_900_REV || revision ==  SIS630ET_900_REV) )
	{
D(bug("[%s]: sis630_set_eq: Skipping for revision %d chipset\n", unit->sis900u_name, revision));
		return;
	}

	if (netif_carrier_ok(unit)) {
		reg14h = mdio_read(unit, unit->cur_phy, MII_RESV);
		mdio_write(unit, unit->cur_phy, MII_RESV,
					(0x2200 | reg14h) & 0xBFFF);
		for (i=0; i < maxcount; i++) {
			eq_value = (0x00F8 & mdio_read(unit,
					unit->cur_phy, MII_RESV)) >> 3;
			if (i == 0)
				max_value=min_value=eq_value;
			max_value = (eq_value > max_value) ?
						eq_value : max_value;
			min_value = (eq_value < min_value) ?
						eq_value : min_value;
		}
		/* 630E rule to determine the equalizer value */
		if (revision == SIS630E_900_REV || revision == SIS630EA1_900_REV ||
		    revision == SIS630ET_900_REV) {
			if (max_value < 5)
				eq_value = max_value;
			else if (max_value >= 5 && max_value < 15)
				eq_value = (max_value == min_value) ?
						max_value+2 : max_value+1;
			else if (max_value >= 15)
				eq_value=(max_value == min_value) ?
						max_value+6 : max_value+5;
		}
		/* 630B0&B1 rule to determine the equalizer value */
		if (revision == SIS630A_900_REV && 
		    (unit->sis900u_HostRevisionID == SIS630B0 || 
		     unit->sis900u_HostRevisionID == SIS630B1)) {
			if (max_value == 0)
				eq_value = 3;
			else
				eq_value = (max_value + min_value + 1)/2;
		}
		/* write equalizer value and setting */
		reg14h = mdio_read(unit, unit->cur_phy, MII_RESV);
		reg14h = (reg14h & 0xFF07) | ((eq_value << 3) & 0x00F8);
		reg14h = (reg14h | 0x6000) & 0xFDFF;
		mdio_write(unit, unit->cur_phy, MII_RESV, reg14h);
	} else {
		reg14h = mdio_read(unit, unit->cur_phy, MII_RESV);
		if (revision == SIS630A_900_REV && 
		    (unit->sis900u_HostRevisionID == SIS630B0 || 
		     unit->sis900u_HostRevisionID == SIS630B1)) 
			mdio_write(unit, unit->cur_phy, MII_RESV,
						(reg14h | 0x2200) & 0xBFFF);
		else
			mdio_write(unit, unit->cur_phy, MII_RESV,
						(reg14h | 0x2000) & 0xBFFF);
	}
	return;
}

/**
 *	sis900_init_rxfilter - Initialize the Rx filter
 *	@unit: the net device to initialize for
 *
 *	Set receive filter address to our MAC address
 *	and enable packet filtering.
 */
static void sis900_init_rxfilter(struct net_device *unit)
{
	long ioaddr = unit->sis900u_BaseMem;
	ULONG rfcrSave;
	ULONG i;

D(bug("[%s]: sis900_init_rxfilter()\n", unit->sis900u_name));

	rfcrSave = LONGIN(rfcr + ioaddr);

	/* disable packet filtering before setting filter */
	LONGOUT(rfcr + ioaddr, rfcrSave & ~RFEN);

	/* load MAC addr to filter data register */
	for (i = 0 ; i < 3 ; i++) {
		ULONG w;

		w = (unit->sis900u_dev_addr[(i * 2)] << 8 ) + unit->sis900u_dev_addr[(i * 2) + 1];
		LONGOUT(ioaddr + rfcr, (i << RFADDR_shift));
		LONGOUT(ioaddr + rfdr, w);

		//if (netif_msg_hw(unit)) {
D(bug("[%s]: sis900_init_rxfilter: Receive Filter Addrss[%d]=%x\n",unit->sis900u_name, i, LONGIN(ioaddr + rfdr)));
		//}
	}

	/* enable packet filtering */
	LONGOUT(rfcr + ioaddr, rfcrSave | RFEN);
}

/**
 *	sis900_init_tx_ring - Initialize the Tx descriptor ring
 *	@unit: the net device to initialize for
 *
 *	Initialize the Tx descriptor ring, 
 */
static void sis900_init_tx_ring(struct net_device *unit)
{
	long ioaddr = unit->sis900u_BaseMem;
	int i, allocate = 1;

D(bug("[%s]: sis900_init_tx_ring()\n", unit->sis900u_name));

	unit->tx_full = 0;
	unit->dirty_tx = unit->cur_tx = 0;

	for (i = 0; i < NUM_TX_DESC; i++) {
		APTR framebuffer = NULL;

		if ((allocate) && ((framebuffer = AllocMem(TX_BUF_SIZE, MEMF_PUBLIC | MEMF_CLEAR)) == NULL)) {
			/* not enough memory for framebuffer this makes a "hole"
			   on the buffer ring, it is not clear how the
			   hardware will react to this kind of degenerated
			   buffer */
			allocate = 0;
		}

		unit->tx_ring[i].link = (IPTR)unit->tx_ring_dma +
			((i+1)%NUM_TX_DESC)*sizeof(BufferDesc);
		unit->tx_ring[i].cmdsts = 0;
		if (framebuffer)
		{
			unit->tx_buffers[i] = framebuffer;
			unit->tx_ring[i].bufptr = (IPTR)HIDD_PCIDriver_CPUtoPCI(unit->sis900u_PCIDriver, framebuffer);
		}
D(bug("[%s]: sis900_init_tx_ring: Buffer %d @ %p\n", unit->sis900u_name, i, framebuffer));
	}

	/* load Transmit Descriptor Register */
	LONGOUT(ioaddr + txdp, (IPTR)unit->tx_ring_dma);
//	if (netif_msg_hw(unit))
D(bug("[%s]: sis900_init_tx_ring: TX descriptor register loaded with: %8.8x\n",unit->sis900u_name, LONGIN(ioaddr + txdp)));
}

/**
 *	sis900_init_rx_ring - Initialize the Rx descriptor ring
 *	@unit: the net device to initialize for
 *
 *	Initialize the Rx descriptor ring, 
 *	and pre-allocate recevie buffers (socket buffer)
 */
static void sis900_init_rx_ring(struct net_device *unit)
{
	long ioaddr = unit->sis900u_BaseMem;
	int i, allocate = 1;

D(bug("[%s]: sis900_init_rx_ring()\n", unit->sis900u_name));

	unit->cur_rx = 0;
	unit->dirty_rx = 0;

	/*  init RX descriptor and allocate buffers */
	for (i = 0; i < NUM_RX_DESC; i++) {
		APTR framebuffer = NULL;

		if ((allocate) && ((framebuffer = AllocMem(RX_BUF_SIZE, MEMF_PUBLIC | MEMF_CLEAR)) == NULL)) {
			/* not enough memory for framebuffer this makes a "hole"
			   on the buffer ring, it is not clear how the
			   hardware will react to this kind of degenerated
			   buffer */
			allocate = 0;
		}
		unit->rx_ring[i].link = (IPTR)unit->rx_ring_dma +
			((i+1)%NUM_RX_DESC)*sizeof(BufferDesc);
		unit->rx_ring[i].cmdsts = RX_BUF_SIZE;
		if (framebuffer)
		{
			unit->rx_buffers[i] = framebuffer;
			unit->rx_ring[i].bufptr = (IPTR)HIDD_PCIDriver_CPUtoPCI(unit->sis900u_PCIDriver, framebuffer);
		}
D(bug("[%s]: sis900_init_rx_ring: Buffer %d @ %p\n", unit->sis900u_name, i, framebuffer));
	}
	unit->dirty_rx = (unsigned int) (i - NUM_RX_DESC);

	/* load Receive Descriptor Register */
	LONGOUT(ioaddr + rxdp, (IPTR)unit->rx_ring_dma);
//	if (netif_msg_hw(sis_priv))
D(bug("[%s]: sis900_init_rx_ring: RX descriptor register loaded with: %8.8x\n",unit->sis900u_name, LONGIN(ioaddr + rxdp)));
}


/**
 *	set_rx_mode - Set SiS900 receive mode 
 *	@unit: the net device to be set
 *
 *	Set SiS900 receive mode for promiscuous, multicast, or broadcast mode.
 *	And set the appropriate multicast filter.
 *	Multicast hash table changes from 128 to 256 bits for 635M/B & 900B0.
 */
static void set_rx_mode(struct net_device *unit)
{
	long ioaddr = unit->sis900u_BaseMem;
	UWORD mc_filter[16] = {0};	/* 256/128 bits multicast hash table */
	int i, table_entries;
	ULONG rx_mode;

D(bug("[%s]: set_rx_mode()\n", unit->sis900u_name));

	/* 635 Hash Table entires = 256(2^16) */
	if((unit->sis900u_RevisionID >= SIS635A_900_REV) ||
			(unit->sis900u_RevisionID == SIS900B_900_REV))
		table_entries = 16;
	else
		table_entries = 8;

/* TODO: Fix multicast settings */
	//if (unit->sis900u_ifflags & IFF_PROMISC) {
		// Accept any kinds of packets
		rx_mode = RFPromiscuous;
		for (i = 0; i < table_entries; i++)
			mc_filter[i] = 0xffff;
	/*} else if ((unit->mc_count > multicast_filter_limit) ||
		   (unit->sis900u_ifflags & IFF_ALLMULTI)) {
		// too many multicast addresses or accept all multicast packet
		rx_mode = RFAAB | RFAAM;
		for (i = 0; i < table_entries; i++)
			mc_filter[i] = 0xffff;
	} else {
		// Accept Broadcast packet, destination address matchs our
        // MAC address, use Receive Filter to reject unwanted MCAST
        // packets
		struct dev_mc_list *mclist;
		rx_mode = RFAAB;
		for (i = 0, mclist = unit->mc_list;
			mclist && i < unit->mc_count;
			i++, mclist = mclist->next) {
			unsigned int bit_nr =
				sis900_mcast_bitnr(mclist->dmi_addr, unit->sis900u_RevisionID);
			mc_filter[bit_nr >> 4] |= (1 << (bit_nr & 0xf));
		}
	}*/

	/* update Multicast Hash Table in Receive Filter */
	for (i = 0; i < table_entries; i++) {
                /* why plus 0x04 ??, That makes the correct value for hash table. */
		LONGOUT(ioaddr + rfcr, (ULONG)(0x00000004+i) << RFADDR_shift);
		LONGOUT(ioaddr + rfdr, mc_filter[i]);
	}

	LONGOUT(ioaddr + rfcr, RFEN | rx_mode);

	/* sis900 is capable of looping back packets at MAC level for
	 * debugging purpose */
	if (unit->sis900u_ifflags & IFF_LOOPBACK) {
		ULONG cr_saved;
		/* We must disable Tx/Rx before setting loopback mode */
		cr_saved = LONGIN(ioaddr + cr);
		LONGOUT(ioaddr + cr, cr_saved | TxDIS | RxDIS);
		/* enable loopback */
		LONGOUT(ioaddr + txcfg, LONGIN(ioaddr + txcfg) | TxMLB);
		LONGOUT(ioaddr + rxcfg, LONGIN(ioaddr + rxcfg) | RxATX);
		/* restore cr */
		LONGOUT(ioaddr + cr, cr_saved);
	}

	return;
}

/**
 *	sis900_set_mode - Set the media mode of mac register.
 *	@ioaddr: the address of the device
 *	@speed : the transmit speed to be determined
 *	@duplex: the duplex mode to be determined
 *
 *	Set the media mode of mac register txcfg/rxcfg according to
 *	speed and duplex of phy. Bit EDB_MASTER_EN indicates the EDB
 *	bus is used instead of PCI bus. When this bit is set 1, the
 *	Max DMA Burst Size for TX/RX DMA should be no larger than 16
 *	double words.
 */
static void sis900_set_mode(long ioaddr, int speed, int duplex)
{
	ULONG tx_flags = 0, rx_flags = 0;

//D(bug("[%s]: sis900_set_mode()\n", unit->sis900u_name));

	if (LONGIN(ioaddr + cfg) & EDB_MASTER_EN) {
		tx_flags = TxATP | (DMA_BURST_64 << TxMXDMA_shift) |
					(TX_FILL_THRESH << TxFILLT_shift);
		rx_flags = DMA_BURST_64 << RxMXDMA_shift;
	} else {
		tx_flags = TxATP | (DMA_BURST_512 << TxMXDMA_shift) |
					(TX_FILL_THRESH << TxFILLT_shift);
		rx_flags = DMA_BURST_512 << RxMXDMA_shift;
	}

	if (speed == HW_SPEED_HOME || speed == HW_SPEED_10_MBPS) {
		rx_flags |= (RxDRNT_10 << RxDRNT_shift);
		tx_flags |= (TxDRNT_10 << TxDRNT_shift);
	} else {
		rx_flags |= (RxDRNT_100 << RxDRNT_shift);
		tx_flags |= (TxDRNT_100 << TxDRNT_shift);
	}

	if (duplex == FDX_CAPABLE_FULL_SELECTED) {
		tx_flags |= (TxCSI | TxHBI);
		rx_flags |= RxATX;
	}

	LONGOUT(ioaddr + txcfg, tx_flags);
	LONGOUT(ioaddr + rxcfg, rx_flags);
}

/**
 *	sis900_check_mode - check the media mode for sis900
 *	@unit: the net device to be checked
 *	@mii_phy: the mii phy
 *
 *	Older driver gets the media mode from mii status output
 *	register. Now we set our media capability and auto-negotiate
 *	to get the upper bound of speed and duplex between two ends.
 *	If the types of mii phy is HOME, it doesn't need to auto-negotiate
 *	and autong_complete should be set to 1.
 */
static void sis900_check_mode(struct net_device *unit, struct mii_phy *mii_phy)
{
	long ioaddr = unit->sis900u_BaseMem;
	int speed, duplex;

D(bug("[%s]: sis900_check_mode()\n", unit->sis900u_name));
	
	if (mii_phy->phy_types == LAN) {
		LONGOUT(ioaddr + cfg, ~EXD & LONGIN(ioaddr + cfg));
		sis900_set_capability(unit, mii_phy);
		sis900_auto_negotiate(unit, unit->cur_phy);
	} else {
		LONGOUT(ioaddr + cfg, EXD | LONGIN(ioaddr + cfg));
		speed = HW_SPEED_HOME;
		duplex = FDX_CAPABLE_HALF_SELECTED;
		sis900_set_mode(ioaddr, speed, duplex);
		unit->autong_complete = 1;
	}
}

int sis900func_open(struct net_device *unit)
{
    int ret;
    // int i, rx_buf_len_idx;

D(bug("[%s]: sis900func_open()\n", unit->sis900u_name));

	/* Soft reset the chip. */
	sis900_reset(unit);

	/* Equalizer workaround Rule */
	sis630_set_eq(unit, unit->sis900u_RevisionID);

	ret = request_irq(unit);
	if (ret)
		goto out_drain;

	sis900_init_rxfilter(unit);

	sis900_init_tx_ring(unit);
	sis900_init_rx_ring(unit);

	set_rx_mode(unit);

//	netif_start_queue(unit);

	/* Workaround for EDB */
	sis900_set_mode(unit->sis900u_BaseMem, HW_SPEED_10_MBPS, FDX_CAPABLE_HALF_SELECTED);

D(bug("[%s]: sis900func_open: Enabling NIC's interupts .. \n", unit->sis900u_name));
	/* Enable all known interrupts by setting the interrupt mask. */
	LONGOUT(unit->sis900u_BaseMem + imr, (RxSOVR|RxORN|RxERR|RxOK|TxURN|TxERR|TxIDLE));
	LONGOUT(unit->sis900u_BaseMem + cr, RxENA | LONGIN(unit->sis900u_BaseMem + cr));
	LONGOUT(unit->sis900u_BaseMem + ier, IE);

	sis900_check_mode(unit, unit->mii);

   unit->sis900u_ifflags |= IFF_UP;
   ReportEvents(LIBBASE, unit, S2EVENT_ONLINE);
D(bug("[%s]: sis900func_open: Device set as ONLINE\n",unit->sis900u_name));

   return 0;

out_drain:
    drain_ring(unit);
    return ret;
}

int sis900func_close(struct net_device *unit)
{
    // UBYTE *base;

D(bug("[%s]: sis900func_close()\n", unit->sis900u_name));
	
    unit->sis900u_ifflags &= ~IFF_UP;

//    ObtainSemaphore(&np->lock);
//   np->in_shutdown = 1;
//    ReleaseSemaphore(&np->lock);

//    unit->sis900u_toutNEED = FALSE;

    netif_stop_queue(unit);
//    ObtainSemaphore(&np->lock);
    
    sis900func_deinitialize(unit);    // Stop the chipset and set it in 16bit-mode

//    base = get_hwbase(unit);

//    ReleaseSemaphore(&np->lock);

    free_irq(unit);

    drain_ring(unit);

//    HIDD_PCIDriver_FreePCIMem(unit->sis900u_PCIDriver, np->rx_buffer);
//    HIDD_PCIDriver_FreePCIMem(unit->sis900u_PCIDriver, np->tx_buffer);

    ReportEvents(LIBBASE, unit, S2EVENT_OFFLINE);

    return 0;
}
