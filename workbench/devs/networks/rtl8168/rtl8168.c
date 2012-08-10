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

#include "rtl8168.h"

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

#include "unit.h"
#include LC_LIBDEFS_FILE

#undef LIBBASE
#define LIBBASE (unit->rtl8168u_device)

#define _R(NAME,MAC,RCR,MASK, JumFrameSz) \
    { .name = NAME, .mcfg = MAC, .RCR_Cfg = RCR, .RxConfigMask = MASK, .jumbo_frame_sz = JumFrameSz }

static const struct {
    const char *name;
    UBYTE      mcfg;
    ULONG      RCR_Cfg;
    ULONG      RxConfigMask;	/* Clears the bits supported by this chip */
    ULONG      jumbo_frame_sz;
} rtl_chip_info[] = {
	_R("RTL8168B/8111B",
	   CFG_METHOD_1,
	   (Reserved2_data << Reserved2_shift) | (RX_DMA_BURST << RxCfgDMAShift),
	   0xff7e1880,
	   Jumbo_Frame_4k),

	_R("RTL8168B/8111B",
	   CFG_METHOD_2,
	   (Reserved2_data << Reserved2_shift) | (RX_DMA_BURST << RxCfgDMAShift),
	   0xff7e1880,
	   Jumbo_Frame_4k),

	_R("RTL8168B/8111B",
	   CFG_METHOD_3,
	   (Reserved2_data << Reserved2_shift) | (RX_DMA_BURST << RxCfgDMAShift),
	   0xff7e1880,
	   Jumbo_Frame_4k),

	_R("RTL8168C/8111C",
	   CFG_METHOD_4, RxCfg_128_int_en | RxCfg_fet_multi_en | (RX_DMA_BURST << RxCfgDMAShift),
	   0xff7e1880,
	   Jumbo_Frame_6k),	

	_R("RTL8168C/8111C",
	   CFG_METHOD_5,
	   RxCfg_128_int_en | RxCfg_fet_multi_en | (RX_DMA_BURST << RxCfgDMAShift),
	   0xff7e1880,
	   Jumbo_Frame_6k),	

	_R("RTL8168C/8111C",
	   CFG_METHOD_6,
	   RxCfg_128_int_en | RxCfg_fet_multi_en | (RX_DMA_BURST << RxCfgDMAShift),
	   0xff7e1880,
	   Jumbo_Frame_6k),

	_R("RTL8168CP/8111CP",
	   CFG_METHOD_7,
	   RxCfg_128_int_en | RxCfg_fet_multi_en | (RX_DMA_BURST << RxCfgDMAShift),
	   0xff7e1880,
	   Jumbo_Frame_6k),

	_R("RTL8168CP/8111CP",
	   CFG_METHOD_8,
	   RxCfg_128_int_en | RxCfg_fet_multi_en | (RX_DMA_BURST << RxCfgDMAShift),
	   0xff7e1880,
	   Jumbo_Frame_6k),

	_R("RTL8168D/8111D",
	   CFG_METHOD_9,
	   RxCfg_128_int_en | (RX_DMA_BURST << RxCfgDMAShift),
	   0xff7e1880,
	   Jumbo_Frame_9k),

	_R("RTL8168D/8111D",
	   CFG_METHOD_10,
	   RxCfg_128_int_en | (RX_DMA_BURST << RxCfgDMAShift),
	   0xff7e1880,
	   Jumbo_Frame_9k),

	_R("RTL8168D/8111D",
	   CFG_METHOD_11,
	   RxCfg_128_int_en | (RX_DMA_BURST << RxCfgDMAShift),
	   0xff7e1880,
	   Jumbo_Frame_9k),
};
#undef _R

void rtl8168nic_USecDelay(struct net_device *unit, ULONG usec)
{
    if (unit != NULL)
    {
        unit->rtl8168u_DelayPort.mp_SigTask = FindTask(NULL);
        unit->rtl8168u_DelayReq.tr_node.io_Command = TR_ADDREQUEST;
        unit->rtl8168u_DelayReq.tr_time.tv_micro = usec % 1000000;
        unit->rtl8168u_DelayReq.tr_time.tv_secs = usec / 1000000;

        DoIO((struct IORequest *)&unit->rtl8168u_DelayReq);
    }
}

static inline struct rtl8168_priv *get_pcnpriv(struct net_device *unit)
{
    return unit->rtl8168u_priv;
}

static inline UBYTE *get_hwbase(struct net_device *unit)
{
    return (UBYTE *)unit->rtl8168u_BaseMem;
}

void MMIO_W8(APTR addr, UBYTE val8)
{
    *((volatile UBYTE *)(addr)) = (val8);

    RTL_R8(addr);
}

void MMIO_W16(APTR addr, UWORD val16)
{
    *((volatile UWORD *)(addr)) = (val16);

    RTL_R16(addr);
}

void MMIO_W32(APTR addr, ULONG val32)
{
    *((volatile ULONG *)(addr)) = (val32);

    RTL_R32(addr);
}

static void mdio_write(struct net_device *unit, int RegAddr, UWORD value)
{
    APTR		base = get_hwbase(unit);
    int i;

    RTL_W32(base + (PHYAR), PHYAR_Write | 
	(RegAddr & PHYAR_Reg_Mask) << PHYAR_Reg_shift | 
	(value & PHYAR_Data_Mask));

    for (i = 0; i < 10; i++) {
	/* Check if the RTL8168 has completed writing to the specified MII register */
	if (!(RTL_R32(base + (PHYAR)) & PHYAR_Flag)) 
	    break;

	udelay(100);
    }
}

static ULONG mdio_read(struct net_device *unit, int RegAddr)
{
    APTR		base = get_hwbase(unit);
    UWORD value = 0xffff;
    int i;

    RTL_W32(base + (PHYAR), PHYAR_Read | (RegAddr & PHYAR_Reg_Mask) << PHYAR_Reg_shift);

    for (i = 0; i < 10; i++) {
	/* Check if the RTL8168 has completed retrieving data from the specified MII register */
	if (RTL_R32(base + (PHYAR)) & PHYAR_Flag) {
	    value = (UWORD)(RTL_R32(base + (PHYAR)) & PHYAR_Data_Mask);
	    break;
	}
	udelay(100);
    }
    return value;
}

static void rtl8168nic_EPHYWrite(struct net_device *unit, int RegAddr, UWORD value)
{
    APTR		base = get_hwbase(unit);
    int i;

    RTL_W32(base + (EPHYAR), 
	    EPHYAR_Write | 
	    (RegAddr & EPHYAR_Reg_Mask) << EPHYAR_Reg_shift | 
	    (value & EPHYAR_Data_Mask));

    for (i = 0; i < 10; i++) {
	udelay(100);

	/* Check if the RTL8168 has completed EPHY write */
	if (!(RTL_R32(base + (EPHYAR)) & EPHYAR_Flag)) 
	    break;
    }

    udelay(20);
}

static UWORD rtl8168nic_EPHYRead(struct net_device *unit, int RegAddr)
{
    APTR		base = get_hwbase(unit);
    UWORD value = 0xffff;
    int i;

    RTL_W32(base + (EPHYAR), 
	    EPHYAR_Read | (RegAddr & EPHYAR_Reg_Mask) << EPHYAR_Reg_shift);

    for (i = 0; i < 10; i++) {
	udelay(100);

	/* Check if the RTL8168 has completed EPHY read */
	if (RTL_R32(base + (EPHYAR)) & EPHYAR_Flag) {
	    value = (UWORD)(RTL_R32(base + (EPHYAR)) & EPHYAR_Data_Mask);
	    break;
	}
    }

    udelay(20);

    return value;
}

static void rtl8168nic_CSIWrite(struct net_device *unit, int addr, ULONG value)
{
    APTR		base = get_hwbase(unit);
    int i;

    RTL_W32(base + (CSIDR), value);
    RTL_W32(base + (CSIAR), 
	    CSIAR_Write |
	    CSIAR_ByteEn << CSIAR_ByteEn_shift |
	    (addr & CSIAR_Addr_Mask));

    for (i = 0; i < 10; i++) {
	udelay(100);

	/* Check if the RTL8168 has completed CSI write */
	if (!(RTL_R32(base + (CSIAR)) & CSIAR_Flag)) 
	    break;
    }

    udelay(20);
}

static ULONG rtl8168nic_CSIRead(struct net_device *unit, int addr)
{
    APTR		base = get_hwbase(unit);
    ULONG value = 0xffffffff;
    int i;

    RTL_W32(base + (CSIAR), 
	    CSIAR_Read | 
	    CSIAR_ByteEn << CSIAR_ByteEn_shift |
	    (addr & CSIAR_Addr_Mask));

    for (i = 0; i < 10; i++) {
	udelay(100);

	/* Check if the RTL8168 has completed CSI read */
	if (RTL_R32(base + (CSIAR)) & CSIAR_Flag) {
	    value = RTL_R32(base + (CSIDR));
	    break;
	}
    }

    udelay(20);

    return value;
}

static void rtl8168nic_PHYPowerUP(struct net_device *unit)
{
    // APTR		base = get_hwbase(unit);

RTLD(bug("[%s] rtl8168nic_PHYPowerUP()\n", unit->rtl8168u_name))

    mdio_write(unit, 0x1F, 0x0000);
    mdio_write(unit, 0x0E, 0x0000);
}

static void rtl8168nic_GetMACVersion(struct net_device *unit)
{
    struct rtl8168_priv *np = get_pcnpriv(unit);
    APTR		base = get_hwbase(unit);
    ULONG               reg, val32;
    ULONG               ICVerID;

RTLD(bug("[%s] rtl8168nic_GetMACVersion()\n", unit->rtl8168u_name))

    val32 = RTL_R32(base + (TxConfig));
    reg = val32 & 0x7c800000;
    ICVerID = val32 & 0x00700000;

    switch(reg)
    {
    case 0x30000000:
	    np->mcfg = CFG_METHOD_1;
	    break;
    case 0x38000000:
	    if(ICVerID == 0x00000000) {
		    np->mcfg = CFG_METHOD_2;
	    } else if(ICVerID == 0x00500000) {
		    np->mcfg = CFG_METHOD_3;
	    } else {
		    np->mcfg = CFG_METHOD_3;
	    }
	    break;
    case 0x3C000000:
	    if(ICVerID == 0x00000000) {
		    np->mcfg = CFG_METHOD_4;
	    } else if(ICVerID == 0x00200000) {
		    np->mcfg = CFG_METHOD_5;
	    } else if(ICVerID == 0x00400000) {
		    np->mcfg = CFG_METHOD_6;
	    } else {
		    np->mcfg = CFG_METHOD_6;
	    }
	    break;
    case 0x3C800000:
	    if (ICVerID == 0x00100000){
		    np->mcfg = CFG_METHOD_7;
	    } else if (ICVerID == 0x00300000){
		    np->mcfg = CFG_METHOD_8;
	    } else {
		    np->mcfg = CFG_METHOD_8;
	    }
	    break;
    case 0x28000000:
	    if(ICVerID == 0x00100000) {
		    np->mcfg = CFG_METHOD_9;
	    } else if(ICVerID == 0x00200000) {
		    np->mcfg = CFG_METHOD_10;
	    } else if(ICVerID == 0x00300000) {
		    np->mcfg = CFG_METHOD_11;
	    } else {
		    np->mcfg = CFG_METHOD_11;
	    }
	    break;
    default:
	    np->mcfg = 0xFFFFFFFF;
	    RTLD(bug("[%s] rtl8168nic_GetMACVersion: unknown chip version (%x)\n", unit->rtl8168u_name, reg))
	    break;
    }
}

static void rtl8168nic_PrintMACVersion(struct net_device *unit)
{
    struct rtl8168_priv *np = get_pcnpriv(unit);
    int i;

RTLD(bug("[%s] rtl8168nic_PrintMACVersion()\n", unit->rtl8168u_name))
    
    for (i = (sizeof(rtl_chip_info) / sizeof(rtl_chip_info[0])) - 1; i >= 0; i--)
    {
	if (np->mcfg == rtl_chip_info[i].mcfg)
	{
	    RTLD(bug("[%s] rtl8168nic_PrintMACVersion: mcfg == %s (%04d)\n", unit->rtl8168u_name, 
									     rtl_chip_info[i].name,
									     rtl_chip_info[i].mcfg))		
	    return;
	}
    }

    RTLD(bug("[%s] rtl8168nic_PrintMACVersion: mac_version == Unknown\n", unit->rtl8168u_name))
}

static void rtl8168nic_HWPHYConfig(struct net_device *unit)
{
    struct rtl8168_priv *np = get_pcnpriv(unit);
    // APTR		base = get_hwbase(unit);
    // int data;

RTLD(bug("[%s] rtl8168nic_HWPHYConfig()\n", unit->rtl8168u_name))

    if (np->mcfg == CFG_METHOD_1)
    {
	mdio_write(unit, 0x1F, 0x0001);
	mdio_write(unit, 0x0B, 0x94B0);

	mdio_write(unit, 0x1F, 0x0003);
	mdio_write(unit, 0x12, 0x6096);
	mdio_write(unit, 0x1F, 0x0000);

	mdio_write(unit, 0x0D, 0xF8A0);
    } else if (np->mcfg == CFG_METHOD_2) {
	mdio_write(unit, 0x1F, 0x0001);
	mdio_write(unit, 0x0B, 0x94B0);

	mdio_write(unit, 0x1F, 0x0003);
	mdio_write(unit, 0x12, 0x6096);

	mdio_write(unit, 0x1F, 0x0000);
    } else if (np->mcfg == CFG_METHOD_3) {
	mdio_write(unit, 0x1F, 0x0001);
	mdio_write(unit, 0x0B, 0x94B0);

	mdio_write(unit, 0x1F, 0x0003);
	mdio_write(unit, 0x12, 0x6096);

	mdio_write(unit, 0x1F, 0x0000);
    } else if (np->mcfg == CFG_METHOD_4) {
	mdio_write(unit, 0x1F, 0x0001);
	mdio_write(unit, 0x12, 0x2300);
	mdio_write(unit, 0x1F, 0x0000);
	mdio_write(unit, 0x1F, 0x0003);
	mdio_write(unit, 0x16, 0x000A);
	mdio_write(unit, 0x1F, 0x0000);

	mdio_write(unit, 0x1F, 0x0003);
	mdio_write(unit, 0x12, 0xC096);
	mdio_write(unit, 0x1F, 0x0000);

	mdio_write(unit, 0x1F, 0x0002);
	mdio_write(unit, 0x00, 0x88DE);
	mdio_write(unit, 0x01, 0x82B1);
	mdio_write(unit, 0x1F, 0x0000);

	mdio_write(unit, 0x1F, 0x0002);
	mdio_write(unit, 0x08, 0x9E30);
	mdio_write(unit, 0x09, 0x01F0);
	mdio_write(unit, 0x1F, 0x0000);

	mdio_write(unit, 0x1F, 0x0002);
	mdio_write(unit, 0x0A, 0x5500);
	mdio_write(unit, 0x1F, 0x0000);

	mdio_write(unit, 0x1F, 0x0002);
	mdio_write(unit, 0x03, 0x7002);
	mdio_write(unit, 0x1F, 0x0000);

	mdio_write(unit, 0x1F, 0x0002);
	mdio_write(unit, 0x0C, 0x00C8);
	mdio_write(unit, 0x1F, 0x0000);

	mdio_write(unit, 0x1F, 0x0000);
	mdio_write(unit, 0x14, mdio_read(unit, 0x14) | (1 << 5));
	mdio_write(unit, 0x0D, mdio_read(unit, 0x0D) | (1 << 5));
    } else if (np->mcfg == CFG_METHOD_5) {
	mdio_write(unit, 0x1F, 0x0001);
	mdio_write(unit, 0x12, 0x2300);
	mdio_write(unit, 0x1F, 0x0003);
	mdio_write(unit, 0x16, 0x0F0A);
	mdio_write(unit, 0x1F, 0x0000);

	mdio_write(unit, 0x1F, 0x0002);
	mdio_write(unit, 0x00, 0x88DE);
	mdio_write(unit, 0x01, 0x82B1);
	mdio_write(unit, 0x1F, 0x0000);

	mdio_write(unit, 0x1F, 0x0002);
	mdio_write(unit, 0x0C, 0x7EB8);
	mdio_write(unit, 0x1F, 0x0000);

	mdio_write(unit, 0x1F, 0x0002);
	mdio_write(unit, 0x06, 0x0761);
	mdio_write(unit, 0x1F, 0x0000);

	mdio_write(unit, 0x1F, 0x0001);
	mdio_write(unit, 0x03, 0x802F);
	mdio_write(unit, 0x02, 0x4F02);
	mdio_write(unit, 0x01, 0x0409);
	mdio_write(unit, 0x00, 0xF099);
	mdio_write(unit, 0x04, 0x9800);
	mdio_write(unit, 0x04, 0x9000);
	mdio_write(unit, 0x1F, 0x0000);

	mdio_write(unit, 0x1F, 0x0000);
	mdio_write(unit, 0x16, mdio_read(unit, 0x16) | (1 << 0));

	mdio_write(unit, 0x1F, 0x0000);
	mdio_write(unit, 0x14, mdio_read(unit, 0x14) | (1 << 5));
	mdio_write(unit, 0x0D, mdio_read(unit, 0x0D) | (1 << 5));

	mdio_write(unit, 0x1F, 0x0001);
	mdio_write(unit, 0x1D, 0x3D98);
	mdio_write(unit, 0x1F, 0x0000);

	mdio_write(unit, 0x1F, 0x0001);
	mdio_write(unit, 0x17, 0x0CC0);
	mdio_write(unit, 0x1F, 0x0000);
    } else if (np->mcfg == CFG_METHOD_6) {
	mdio_write(unit, 0x1F, 0x0001);
	mdio_write(unit, 0x12, 0x2300);
	mdio_write(unit, 0x1F, 0x0003);
	mdio_write(unit, 0x16, 0x0F0A);
	mdio_write(unit, 0x1F, 0x0000);

	mdio_write(unit, 0x1F, 0x0002);
	mdio_write(unit, 0x00, 0x88DE);
	mdio_write(unit, 0x01, 0x82B1);
	mdio_write(unit, 0x1F, 0x0000);

	mdio_write(unit, 0x1F, 0x0002);
	mdio_write(unit, 0x0C, 0x7EB8);
	mdio_write(unit, 0x1F, 0x0000);

	mdio_write(unit, 0x1F, 0x0002);
	mdio_write(unit, 0x06, 0x0761);
	mdio_write(unit, 0x1F, 0x0000);

	mdio_write(unit, 0x1F, 0x0000);
	mdio_write(unit, 0x16, mdio_read(unit, 0x16) | (1 << 0));

	mdio_write(unit, 0x1F, 0x0000);
	mdio_write(unit, 0x14, mdio_read(unit, 0x14) | (1 << 5));
	mdio_write(unit, 0x0D, mdio_read(unit, 0x0D) | (1 << 5));

	mdio_write(unit, 0x1F, 0x0001);
	mdio_write(unit, 0x1D, 0x3D98);
	mdio_write(unit, 0x1F, 0x0000);

	mdio_write(unit, 0x1f, 0x0001);
	mdio_write(unit, 0x17, 0x0CC0);
	mdio_write(unit, 0x1F, 0x0000);
    } else if (np->mcfg == CFG_METHOD_7) {
	mdio_write(unit, 0x1F, 0x0000);
	mdio_write(unit, 0x14, mdio_read(unit, 0x14) | (1 << 5));
	mdio_write(unit, 0x0D, mdio_read(unit, 0x0D) | (1 << 5));

	mdio_write(unit, 0x1F, 0x0001);
	mdio_write(unit, 0x1D, 0x3D98);

	mdio_write(unit, 0x1F, 0x0001);
	mdio_write(unit, 0x14, 0xCAA3);
	mdio_write(unit, 0x1C, 0x000A);
	mdio_write(unit, 0x18, 0x65D0);
	
	mdio_write(unit, 0x1F, 0x0003);
	mdio_write(unit, 0x17, 0xB580);
	mdio_write(unit, 0x18, 0xFF54);
	mdio_write(unit, 0x19, 0x3954);

	mdio_write(unit, 0x1F, 0x0002);
	mdio_write(unit, 0x0D, 0x310C);
	mdio_write(unit, 0x0E, 0x310C);
	mdio_write(unit, 0x0F, 0x311C);
	mdio_write(unit, 0x06, 0x0761);

	mdio_write(unit, 0x1F, 0x0003);
	mdio_write(unit, 0x18, 0xFF55);
	mdio_write(unit, 0x19, 0x3955);
	mdio_write(unit, 0x18, 0xFF54);
	mdio_write(unit, 0x19, 0x3954);

	mdio_write(unit, 0x1f, 0x0001);
	mdio_write(unit, 0x17, 0x0CC0);

	mdio_write(unit, 0x1F, 0x0000);
    } else if (np->mcfg == CFG_METHOD_8) {
	mdio_write(unit, 0x1F, 0x0000);
	mdio_write(unit, 0x0D, mdio_read(unit, 0x0D) | (1 << 5));

	mdio_write(unit, 0x1F, 0x0001);
	mdio_write(unit, 0x14, 0xCAA3);
	mdio_write(unit, 0x1C, 0x000A);
	mdio_write(unit, 0x18, 0x65D0);
	
	mdio_write(unit, 0x1F, 0x0003);
	mdio_write(unit, 0x17, 0xB580);
	mdio_write(unit, 0x18, 0xFF54);
	mdio_write(unit, 0x19, 0x3954);

	mdio_write(unit, 0x1F, 0x0002);
	mdio_write(unit, 0x0D, 0x310C);
	mdio_write(unit, 0x0E, 0x310C);
	mdio_write(unit, 0x0F, 0x311C);
	mdio_write(unit, 0x06, 0x0761);

	mdio_write(unit, 0x1F, 0x0003);
	mdio_write(unit, 0x18, 0xFF55);
	mdio_write(unit, 0x19, 0x3955);
	mdio_write(unit, 0x18, 0xFF54);
	mdio_write(unit, 0x19, 0x3954);

	mdio_write(unit, 0x1f, 0x0001);
	mdio_write(unit, 0x17, 0x0CC0);

	mdio_write(unit, 0x1F, 0x0000);
    } else if (np->mcfg == CFG_METHOD_9) {
	mdio_write(unit, 0x1F, 0x0001);
	mdio_write(unit, 0x06, 0x4064);
	mdio_write(unit, 0x07, 0x2863);
	mdio_write(unit, 0x08, 0x059C);
	mdio_write(unit, 0x09, 0x26B4);
	mdio_write(unit, 0x0A, 0x6A19);
	mdio_write(unit, 0x0B, 0xACC0);
	mdio_write(unit, 0x10, 0xF06D);
	mdio_write(unit, 0x14, 0x7F68);
	mdio_write(unit, 0x18, 0x7FD9);
	mdio_write(unit, 0x1C, 0xF0FF);
	mdio_write(unit, 0x1D, 0x3D9C);
	mdio_write(unit, 0x1F, 0x0003);
	mdio_write(unit, 0x12, 0xF49F);
	mdio_write(unit, 0x13, 0x070B);
	mdio_write(unit, 0x1A, 0x05AD);
	mdio_write(unit, 0x14, 0x94C0);

	mdio_write(unit, 0x1F, 0x0002);
	mdio_write(unit, 0x0B, 0x0B10);
	mdio_write(unit, 0x0C, 0xA2F7);

	mdio_write(unit, 0x1F, 0x0002);
	mdio_write(unit, 0x06, 0x5571);

	mdio_write(unit, 0x1F, 0x0002);
	mdio_write(unit, 0x02, 0xC107);
	mdio_write(unit, 0x03, 0x1002);

	mdio_write(unit, 0x1F, 0x0001);
	mdio_write(unit, 0x17, 0x0CC0);

	mdio_write(unit, 0x1F, 0x0005);
	mdio_write(unit, 0x05, 0x8200);
	mdio_write(unit, 0x06, 0xF8F9);
	mdio_write(unit, 0x06, 0xFAEF);
	mdio_write(unit, 0x06, 0x59EE);
	mdio_write(unit, 0x06, 0xF8EA);
	mdio_write(unit, 0x06, 0x00EE);
	mdio_write(unit, 0x06, 0xF8EB);
	mdio_write(unit, 0x06, 0x00E0);
	mdio_write(unit, 0x06, 0xF87C);
	mdio_write(unit, 0x06, 0xE1F8);
	mdio_write(unit, 0x06, 0x7D59);
	mdio_write(unit, 0x06, 0x0FEF);
	mdio_write(unit, 0x06, 0x0139);
	mdio_write(unit, 0x06, 0x029E);
	mdio_write(unit, 0x06, 0x06EF);
	mdio_write(unit, 0x06, 0x1039);
	mdio_write(unit, 0x06, 0x089F);
	mdio_write(unit, 0x06, 0x2AEE);
	mdio_write(unit, 0x06, 0xF8EA);
	mdio_write(unit, 0x06, 0x00EE);
	mdio_write(unit, 0x06, 0xF8EB);
	mdio_write(unit, 0x06, 0x01E0);
	mdio_write(unit, 0x06, 0xF87C);
	mdio_write(unit, 0x06, 0xE1F8);
	mdio_write(unit, 0x06, 0x7D58);
	mdio_write(unit, 0x06, 0x409E);
	mdio_write(unit, 0x06, 0x0F39);
	mdio_write(unit, 0x06, 0x46AA);
	mdio_write(unit, 0x06, 0x0BBF);
	mdio_write(unit, 0x06, 0x8251);
	mdio_write(unit, 0x06, 0xD682);
	mdio_write(unit, 0x06, 0x5902);
	mdio_write(unit, 0x06, 0x014F);
	mdio_write(unit, 0x06, 0xAE09);
	mdio_write(unit, 0x06, 0xBF82);
	mdio_write(unit, 0x06, 0x59D6);
	mdio_write(unit, 0x06, 0x8261);
	mdio_write(unit, 0x06, 0x0201);
	mdio_write(unit, 0x06, 0x4FEF);
	mdio_write(unit, 0x06, 0x95FE);
	mdio_write(unit, 0x06, 0xFDFC);
	mdio_write(unit, 0x06, 0x054D);
	mdio_write(unit, 0x06, 0x2000);
	mdio_write(unit, 0x06, 0x024E);
	mdio_write(unit, 0x06, 0x2200);
	mdio_write(unit, 0x06, 0x024D);
	mdio_write(unit, 0x06, 0xDFFF);
	mdio_write(unit, 0x06, 0x014E);
	mdio_write(unit, 0x06, 0xDDFF);
	mdio_write(unit, 0x06, 0x0100);
	mdio_write(unit, 0x06, 0x6010);
	mdio_write(unit, 0x05, 0xFFF6);
	mdio_write(unit, 0x06, 0x00EC);
	mdio_write(unit, 0x05, 0x83D4);
	mdio_write(unit, 0x06, 0x8200);
	mdio_write(unit, 0x1F, 0x0000);
    } else if (np->mcfg == CFG_METHOD_10) {
	mdio_write(unit, 0x1F, 0x0001);
	mdio_write(unit, 0x06, 0x4064);
	mdio_write(unit, 0x07, 0x2863);
	mdio_write(unit, 0x08, 0x059C);
	mdio_write(unit, 0x09, 0x26B4);
	mdio_write(unit, 0x0A, 0x6A19);
	mdio_write(unit, 0x0B, 0xACC0);
	mdio_write(unit, 0x10, 0xF06D);
	mdio_write(unit, 0x14, 0x7F68);
	mdio_write(unit, 0x18, 0x7FD9);
	mdio_write(unit, 0x1C, 0xF0FF);
	mdio_write(unit, 0x1D, 0x3D9C);
	mdio_write(unit, 0x1F, 0x0003);
	mdio_write(unit, 0x12, 0xF49F);
	mdio_write(unit, 0x13, 0x070B);
	mdio_write(unit, 0x1A, 0x05AD);
	mdio_write(unit, 0x14, 0x94C0);
	
	mdio_write(unit, 0x1F, 0x0002);
	mdio_write(unit, 0x06, 0x5571);

	mdio_write(unit, 0x1F, 0x0002);
	mdio_write(unit, 0x05, 0x2642);

	mdio_write(unit, 0x1F, 0x0002);
	mdio_write(unit, 0x02, 0xC107);
	mdio_write(unit, 0x03, 0x1002);

	mdio_write(unit, 0x1F, 0x0001);
	mdio_write(unit, 0x17, 0x0CC0);

	mdio_write(unit, 0x1F, 0x0002);
	mdio_write(unit, 0x0F, 0x0017);

	mdio_write(unit, 0x1F, 0x0005);
	mdio_write(unit, 0x05, 0x8200);
	mdio_write(unit, 0x06, 0xF8F9);
	mdio_write(unit, 0x06, 0xFAEF);
	mdio_write(unit, 0x06, 0x59EE);
	mdio_write(unit, 0x06, 0xF8EA);
	mdio_write(unit, 0x06, 0x00EE);
	mdio_write(unit, 0x06, 0xF8EB);
	mdio_write(unit, 0x06, 0x00E0);
	mdio_write(unit, 0x06, 0xF87C);
	mdio_write(unit, 0x06, 0xE1F8);
	mdio_write(unit, 0x06, 0x7D59);
	mdio_write(unit, 0x06, 0x0FEF);
	mdio_write(unit, 0x06, 0x0139);
	mdio_write(unit, 0x06, 0x029E);
	mdio_write(unit, 0x06, 0x06EF);
	mdio_write(unit, 0x06, 0x1039);
	mdio_write(unit, 0x06, 0x089F);
	mdio_write(unit, 0x06, 0x2AEE);
	mdio_write(unit, 0x06, 0xF8EA);
	mdio_write(unit, 0x06, 0x00EE);
	mdio_write(unit, 0x06, 0xF8EB);
	mdio_write(unit, 0x06, 0x01E0);
	mdio_write(unit, 0x06, 0xF87C);
	mdio_write(unit, 0x06, 0xE1F8);
	mdio_write(unit, 0x06, 0x7D58);
	mdio_write(unit, 0x06, 0x409E);
	mdio_write(unit, 0x06, 0x0F39);
	mdio_write(unit, 0x06, 0x46AA);
	mdio_write(unit, 0x06, 0x0BBF);
	mdio_write(unit, 0x06, 0x8251);
	mdio_write(unit, 0x06, 0xD682);
	mdio_write(unit, 0x06, 0x5902);
	mdio_write(unit, 0x06, 0x014F);
	mdio_write(unit, 0x06, 0xAE09);
	mdio_write(unit, 0x06, 0xBF82);
	mdio_write(unit, 0x06, 0x59D6);
	mdio_write(unit, 0x06, 0x8261);
	mdio_write(unit, 0x06, 0x0201);
	mdio_write(unit, 0x06, 0x4FEF);
	mdio_write(unit, 0x06, 0x95FE);
	mdio_write(unit, 0x06, 0xFDFC);
	mdio_write(unit, 0x06, 0x054D);
	mdio_write(unit, 0x06, 0x2000);
	mdio_write(unit, 0x06, 0x024E);
	mdio_write(unit, 0x06, 0x2200);
	mdio_write(unit, 0x06, 0x024D);
	mdio_write(unit, 0x06, 0xDFFF);
	mdio_write(unit, 0x06, 0x014E);
	mdio_write(unit, 0x06, 0xDDFF);
	mdio_write(unit, 0x06, 0x0100);
	mdio_write(unit, 0x02, 0x6010);
	mdio_write(unit, 0x05, 0xFFF6);
	mdio_write(unit, 0x06, 0x00EC);
	mdio_write(unit, 0x05, 0x83D4);
	mdio_write(unit, 0x06, 0x8200);
	mdio_write(unit, 0x1F, 0x0000);
    } else if (np->mcfg == CFG_METHOD_11) {
	mdio_write(unit, 0x1F, 0x0001);
	mdio_write(unit, 0x06, 0x4064);
	mdio_write(unit, 0x07, 0x2863);
	mdio_write(unit, 0x08, 0x059C);
	mdio_write(unit, 0x09, 0x26B4);
	mdio_write(unit, 0x0A, 0x6A19);
	mdio_write(unit, 0x0B, 0xACC0);
	mdio_write(unit, 0x10, 0xF06D);
	mdio_write(unit, 0x14, 0x7F68);
	mdio_write(unit, 0x18, 0x7FD9);
	mdio_write(unit, 0x1C, 0xF0FF);
	mdio_write(unit, 0x1D, 0x3D9C);
	mdio_write(unit, 0x1F, 0x0003);
	mdio_write(unit, 0x12, 0xF49F);
	mdio_write(unit, 0x13, 0x070B);
	mdio_write(unit, 0x1A, 0x05AD);
	mdio_write(unit, 0x14, 0x94C0);
	
	mdio_write(unit, 0x1F, 0x0002);
	mdio_write(unit, 0x06, 0x5571);

	mdio_write(unit, 0x1F, 0x0002);
	mdio_write(unit, 0x05, 0x2642);

	mdio_write(unit, 0x1F, 0x0002);
	mdio_write(unit, 0x02, 0xC107);
	mdio_write(unit, 0x03, 0x1002);

	mdio_write(unit, 0x1F, 0x0001);
	mdio_write(unit, 0x17, 0x0CC0);

	mdio_write(unit, 0x1F, 0x0002);
	mdio_write(unit, 0x0F, 0x0017);
    }
}

static void rtl8168nic_DSM(struct net_device *unit, int dev_state)
{
    struct rtl8168_priv *np = get_pcnpriv(unit);
    APTR		base = get_hwbase(unit);

    switch (dev_state) {
    case DSM_MAC_INIT:
	if ((np->mcfg == CFG_METHOD_5) || (np->mcfg == CFG_METHOD_6)) {
	    if (RTL_R8(base + (MACDBG)) & 0x80) {
		RTL_W8(base + (GPIO), RTL_R8(base + (GPIO)) | GPIO_en);
	    } else {
		RTL_W8(base + (GPIO), RTL_R8(base + (GPIO)) & ~GPIO_en);
	    }
	}

	break;
    case DSM_NIC_GOTO_D3:
    case DSM_IF_DOWN:
	if ((np->mcfg == CFG_METHOD_5) || (np->mcfg == CFG_METHOD_6))
	    if (RTL_R8(base + (MACDBG)) & 0x80)
		RTL_W8(base + (GPIO), RTL_R8(base + (GPIO)) & ~GPIO_en);

	break;
    case DSM_NIC_RESUME_D3:
    case DSM_IF_UP:
	if ((np->mcfg == CFG_METHOD_5) || (np->mcfg == CFG_METHOD_6))
	    if (RTL_R8(base + (MACDBG)) & 0x80)
		RTL_W8(base + (GPIO), RTL_R8(base + (GPIO)) | GPIO_en);

	break;
    }
}

static int rtl8168nic_SetSpeedXMII(struct net_device *unit,
		       UBYTE autoneg, 
		       UWORD speed, 
		       UBYTE duplex)
{
    struct rtl8168_priv *np = get_pcnpriv(unit);
    // APTR		base = get_hwbase(unit);
    int auto_nego = 0;
    int giga_ctrl = 0;
    int bmcr_true_force = 0;
    // unsigned long flags;

RTLD(bug("[%s] rtl8168nic_SetSpeedXMII()\n", unit->rtl8168u_name))

    if ((speed != SPEED_1000) && 
      (speed != SPEED_100) && 
      (speed != SPEED_10)) {
	speed = SPEED_1000;
	duplex = DUPLEX_FULL;
    }

    if ((autoneg == AUTONEG_ENABLE) || (speed == SPEED_1000)) {
	/*n-way force*/
	if ((speed == SPEED_10) && (duplex == DUPLEX_HALF)) {
		auto_nego |= ADVERTISE_10HALF;
	} else if ((speed == SPEED_10) && (duplex == DUPLEX_FULL)) {
		auto_nego |= ADVERTISE_10HALF |
			     ADVERTISE_10FULL;
	} else if ((speed == SPEED_100) && (duplex == DUPLEX_HALF)) {
		auto_nego |= ADVERTISE_100HALF | 
			     ADVERTISE_10HALF | 
			     ADVERTISE_10FULL;
	} else if ((speed == SPEED_100) && (duplex == DUPLEX_FULL)) {
		auto_nego |= ADVERTISE_100HALF | 
			     ADVERTISE_100FULL |
			     ADVERTISE_10HALF | 
			     ADVERTISE_10FULL;
	} else if (speed == SPEED_1000) {
		giga_ctrl |= ADVERTISE_1000HALF | 
			     ADVERTISE_1000FULL;

		auto_nego |= ADVERTISE_100HALF | 
			     ADVERTISE_100FULL |
			     ADVERTISE_10HALF | 
			     ADVERTISE_10FULL;
	}

	//disable flow contorol
	auto_nego &= ~ADVERTISE_PAUSE_CAP;
	auto_nego &= ~ADVERTISE_PAUSE_ASYM;

	np->phy_auto_nego_reg = auto_nego;
	np->phy_1000_ctrl_reg = giga_ctrl;

	np->autoneg = autoneg;
	np->speed = speed; 
	np->duplex = duplex; 

	rtl8168nic_PHYPowerUP(unit);

	mdio_write(unit, 0x1f, 0x0000);
	mdio_write(unit, MII_ADVERTISE, auto_nego);
	mdio_write(unit, MII_CTRL1000, giga_ctrl);
	mdio_write(unit, MII_BMCR, BMCR_RESET | BMCR_ANENABLE | BMCR_ANRESTART);
    } else {
	    /*true force*/
#ifndef BMCR_SPEED100
#define BMCR_SPEED100	0x0040
#endif

#ifndef BMCR_SPEED10
#define BMCR_SPEED10	0x0000
#endif
	if ((speed == SPEED_10) && (duplex == DUPLEX_HALF)) {
		bmcr_true_force = BMCR_SPEED10;
	} else if ((speed == SPEED_10) && (duplex == DUPLEX_FULL)) {
		bmcr_true_force = BMCR_SPEED10 | BMCR_FULLDPLX;
	} else if ((speed == SPEED_100) && (duplex == DUPLEX_HALF)) {
		bmcr_true_force = BMCR_SPEED100;
	} else if ((speed == SPEED_100) && (duplex == DUPLEX_FULL)) {
		bmcr_true_force = BMCR_SPEED100 | BMCR_FULLDPLX;
	}

	mdio_write(unit, 0x1f, 0x0000);
	mdio_write(unit, MII_BMCR, bmcr_true_force);
    }

    return 0;
}

#if 0
static void rtl8168nic_start_rx(struct net_device *unit)
{
    // struct rtl8168_priv *np = get_pcnpriv(unit);
    // APTR base = get_hwbase(unit);

RTLD(bug("[%s] rtl8168nic_start_rx\n", unit->rtl8168u_name))
    // Already running? Stop it.
/* TODO: Handle starting/stopping Rx */
}

static void rtl8168nic_stop_rx(struct net_device *unit)
{
    // APTR base = get_hwbase(unit);

RTLD(bug("[%s] rtl8168nic_stop_rx\n", unit->rtl8168u_name))
/* TODO: Handle starting/stopping Rx */
}

static void rtl8168nic_start_tx(struct net_device *unit)
{
    // APTR base = get_hwbase(unit);

RTLD(bug("[%s] rtl8168nic_start_tx()\n", unit->rtl8168u_name))
/* TODO: Handle starting/stopping Tx */
}

static void rtl8168nic_stop_tx(struct net_device *unit)
{
    // APTR base = get_hwbase(unit);

RTLD(bug("[%s] rtl8168nic_stop_tx()\n", unit->rtl8168u_name))
/* TODO: Handle starting/stopping Tx */
}

static void rtl8168nic_txrx_reset(struct net_device *unit)
{
    // struct rtl8168_priv *np = get_pcnpriv(unit);
    // APTR base = get_hwbase(unit);

RTLD(bug("[%s] rtl8168nic_txrx_reset()\n", unit->rtl8168u_name))
}
#endif

/*
 * rtl8168nic_SetMulticast: unit->set_multicast function
 * Called with unit->xmit_lock held.
 */
static void rtl8168nic_SetMulticast(struct net_device *unit)
{
    // struct rtl8168_priv *np = get_pcnpriv(unit);
    // APTR base = get_hwbase(unit);
    ULONG addr[2];
    ULONG mask[2];
    // ULONG pff;

RTLD(bug("[%s] rtl8168nic_SetMulticast()\n", unit->rtl8168u_name))

    memset(addr, 0, sizeof(addr));
    memset(mask, 0, sizeof(mask));
}

static void rtl8168nic_DeInit(struct net_device *unit)
{
/* TODO: Clean up / DeInit hardware */
}

static void rtl8168nic_GetMACAddr(struct net_device *unit, char *addr, BOOL fromROM)
{
    APTR base = get_hwbase(unit);
    int i;

RTLD(bug("[%s] rtl8168nic_GetMACAddr()\n",unit->rtl8168u_name))
    /* Get MAC address.  FIXME: read EEPROM */
    for (i = 0; i < MAC_ADDR_LEN; i++)
	addr[i] = RTL_R8(base + MAC0 + i);
}

static void rtl8168nic_SetMACAddr(struct net_device *unit)
{
    APTR base = get_hwbase(unit);
    // int i,j;

RTLD(bug("[%s] rtl8168nic_SetMACAddr()\n",unit->rtl8168u_name))

    RTL_W8(base + (Cfg9346), Cfg9346_Unlock);

    RTL_W32(base + (MAC0),
	    unit->rtl8168u_dev_addr[0] |
	    (unit->rtl8168u_dev_addr[1] << 8) |
	    (unit->rtl8168u_dev_addr[2] << 16) |
	    (unit->rtl8168u_dev_addr[3] << 24));
    RTL_W32(base + (MAC4),
	    unit->rtl8168u_dev_addr[4] |
	    (unit->rtl8168u_dev_addr[5] << 8));

    RTL_W8(base + (Cfg9346), Cfg9346_Lock);

    RTLD(
	/* Read it back to be certain! */
	TEXT	newmac[6];
	rtl8168nic_GetMACAddr(unit, newmac, FALSE);

	bug("[%s] rtl8168nic_SetMACAddr: New MAC Address %02x:%02x:%02x:%02x:%02x:%02x\n", unit->rtl8168u_name,
			newmac[0], newmac[1], newmac[2],
			newmac[3], newmac[4], newmac[5])
    )
}

static void rtl8168nic_LinkOption(struct net_device *unit, UBYTE *aut, UWORD *spd, UBYTE *dup)
{
    unsigned char opt_speed;
    unsigned char opt_duplex;
    unsigned char opt_autoneg;

    opt_speed = ((unit->rtl8168u_UnitNum < MAX_UNITS) && (unit->rtl8168u_UnitNum >= 0)) ? unit->rtl8168u_device->speed[unit->rtl8168u_UnitNum] : 0xff;
    opt_duplex = ((unit->rtl8168u_UnitNum < MAX_UNITS) && (unit->rtl8168u_UnitNum >= 0)) ? unit->rtl8168u_device->duplex[unit->rtl8168u_UnitNum] : 0xff;
    opt_autoneg = ((unit->rtl8168u_UnitNum < MAX_UNITS) && (unit->rtl8168u_UnitNum >= 0)) ? unit->rtl8168u_device->autoneg[unit->rtl8168u_UnitNum] : 0xff;

    if ((opt_speed == 0xff) |
      (opt_duplex == 0xff) |
      (opt_autoneg == 0xff)) {
	*spd = SPEED_1000;
	*dup = DUPLEX_FULL;
	*aut = AUTONEG_ENABLE;
    } else {
	*spd = unit->rtl8168u_device->speed[unit->rtl8168u_UnitNum];
	*dup = unit->rtl8168u_device->duplex[unit->rtl8168u_UnitNum];
	*aut = unit->rtl8168u_device->autoneg[unit->rtl8168u_UnitNum];	
    }
}

static void rtl8168nic_Init(struct net_device *unit)
{
    struct pHidd_PCIDevice_WriteConfigByte pcibyte;

    struct rtl8168_priv *np = get_pcnpriv(unit);
    APTR base = get_hwbase(unit);
    UBYTE autoneg, duplex;
    UWORD speed;
    int i;
    // int config1;

RTLD(bug("[%s] rtl8168nic_Init(unit @ %p)\n", unit->rtl8168u_name, unit))

    /* Identify chip attached to board */
    rtl8168nic_GetMACVersion(unit);
    rtl8168nic_PrintMACVersion(unit);

    for (i = (sizeof(rtl_chip_info) / sizeof(rtl_chip_info[0])) - 1; i >= 0; i--) {
	if (np->mcfg == rtl_chip_info[i].mcfg)
	    break;
    }

    if (i < 0) {
	/* Unknown chip: assume array element #0, original RTL-8168 */
RTLD(bug("[%s] rtl8168nic_Init: Unknown Realtek chip version, assuming %s\n", unit->rtl8168u_name, rtl_chip_info[0].name))
	i++;
    }

    np->chipset = i;
    unit->rtl8168u_rtl_chipname = rtl_chip_info[np->chipset].name;

    RTL_W8(base + (Cfg9346), Cfg9346_Unlock);
    RTL_W8(base + (Config1), RTL_R8(base + Config1) | PMEnable);
    RTL_W8(base + (Config5), RTL_R8(base + Config5) & PMEStatus);
    RTL_W8(base + (Cfg9346), Cfg9346_Lock);

RTLD(bug("[%s] rtl8168nic_Init: Power Management enabled\n", unit->rtl8168u_name))


/*    RTL_W8(base + (Cfg9346), Cfg9346_Unlock);
    np->features |= rtl8168_try_msi(pdev, base);
    RTL_W8(base + (Cfg9346), Cfg9346_Lock); */


    rtl8168nic_GetMACAddr(unit, &np->orig_mac[0], TRUE);

    unit->rtl8168u_dev_addr[0] = unit->rtl8168u_org_addr[0] = np->orig_mac[0];
    unit->rtl8168u_dev_addr[1] = unit->rtl8168u_org_addr[1] = np->orig_mac[1];
    unit->rtl8168u_dev_addr[2] = unit->rtl8168u_org_addr[2] = np->orig_mac[2];
    unit->rtl8168u_dev_addr[3] = unit->rtl8168u_org_addr[3] = np->orig_mac[3];
    unit->rtl8168u_dev_addr[4] = unit->rtl8168u_org_addr[4] = np->orig_mac[4];
    unit->rtl8168u_dev_addr[5] = unit->rtl8168u_org_addr[5] = np->orig_mac[5];

RTLD(bug("[%s] rtl8168nic_Init: MAC Address %02x:%02x:%02x:%02x:%02x:%02x\n", unit->rtl8168u_name,
		    unit->rtl8168u_dev_addr[0], unit->rtl8168u_dev_addr[1], unit->rtl8168u_dev_addr[2],
		    unit->rtl8168u_dev_addr[3], unit->rtl8168u_dev_addr[4], unit->rtl8168u_dev_addr[5]))

    bug("[%s] rtl8168nic_Init: This product is covered by one or more of the following patents:\n", unit->rtl8168u_name);
    bug("[%s] rtl8168nic_Init: US5,307,459, US5,434,872, US5,732,094, US6,570,884, US6,115,776, and US6,327,625.\n", unit->rtl8168u_name);

/*
    unit->features |= NETIF_F_IP_CSUM;
*/
    np->cp_cmd |= RxChkSum;
    np->cp_cmd |= RTL_R16(base + (CPlusCmd));

    np->intr_mask = unit->rtl8168u_intr_mask;

    rtl8168nic_HWPHYConfig(unit);

RTLD(bug("[%s] rtl8168nic_Init: PHY Configured\n", unit->rtl8168u_name))

    pcibyte.mID = OOP_GetMethodID(CLID_Hidd_PCIDevice, moHidd_PCIDevice_WriteConfigByte);
    pcibyte.reg = PCI_LATENCY_TIMER;
    pcibyte.val = 0x40;
    OOP_DoMethod(unit->rtl8168u_PCIDevice, (OOP_Msg)&pcibyte);

    rtl8168nic_LinkOption(unit, &autoneg, &speed, &duplex);

    rtl8168nic_SetSpeedXMII(unit, autoneg, speed, duplex);
    
RTLD(bug("[%s] rtl8168nic_Init: Link Speed %dbps %s duplex %s\n", unit->rtl8168u_name, speed, (duplex == DUPLEX_HALF) ? "half" :"full", (autoneg == AUTONEG_ENABLE) ? "(autoneg)" :""))
}

static void rtl8168nic_drain_tx(struct net_device *unit)
{
    // struct rtl8168_priv *np = get_pcnpriv(unit);
    // int i;
//    for (i = 0; i < NUM_TX_DESC; i++) {
/* TODO: rtl8168nic_drain_tx does nothing atm. */
//    }
}

static void rtl8168nic_drain_rx(struct net_device *unit)
{
    // struct rtl8168_priv *np = get_pcnpriv(unit);
    // int i;
//    for (i = 0; i < RX_RING_SIZE; i++) {
/* TODO: rtl8168nic_drain_rx does nothing atm. */
//    }
}


static void drain_ring(struct net_device *unit)
{
    rtl8168nic_drain_tx(unit);
    rtl8168nic_drain_rx(unit);
}

static int request_irq(struct net_device *unit)
{

RTLD(bug("[%s] request_irq()\n", unit->rtl8168u_name))

    AddIntServer(INTB_KERNEL + unit->rtl8168u_IRQ, &unit->rtl8168u_irqhandler);
    AddIntServer(INTB_VERTB, &unit->rtl8168u_touthandler);

RTLD(bug("[%s] request_irq: IRQ Handlers configured\n", unit->rtl8168u_name))
    return 1;
}

static void free_irq(struct net_device *unit)
{
    RemIntServer(INTB_KERNEL + unit->rtl8168u_IRQ, &unit->rtl8168u_irqhandler);
    RemIntServer(INTB_VERTB, &unit->rtl8168u_touthandler);
}

static void rtl8168nic_SetRXBufSize(struct net_device *unit)
{
    struct rtl8168_priv *np = get_pcnpriv(unit);
    APTR base = get_hwbase(unit);
    unsigned int mtu = unit->rtl8168u_mtu;

    RTLD(bug("[%s] rtl8168nic_SetRXBufSize()\n", unit->rtl8168u_name))

    np->rx_buf_sz = (mtu > ETH_DATA_LEN) ? mtu + ETH_HLEN + 8 : RX_BUF_SIZE;

    RTL_W16(base + (RxMaxSize), np->rx_buf_sz + 1);
}

static void rtl8168nic_NICReset(struct net_device *unit)
{
    struct rtl8168_priv *np = get_pcnpriv(unit);
    APTR base = get_hwbase(unit);
    int i;

    RTLD(bug("[%s] rtl8168nic_NICReset()\n", unit->rtl8168u_name))

    if ((np->mcfg != CFG_METHOD_1) && 
	(np->mcfg != CFG_METHOD_2) &&
	(np->mcfg != CFG_METHOD_3)) {
	    RTL_W8(base + (ChipCmd), StopReq | CmdRxEnb | CmdTxEnb);
	    udelay(100);
    }

    /* Soft reset the chip. */
    RTL_W8(base + (ChipCmd), CmdReset);

    /* Check that the chip has finished the reset. */
    for (i = 1000; i > 0; i--) {
	if ((RTL_R8(base + (ChipCmd)) & CmdReset) == 0)
	    break;
	udelay(100);
    }
}

static void rtl8168nic_SetRxMode(struct net_device *unit)
{
    struct rtl8168_priv *np = get_pcnpriv(unit);
    APTR base = get_hwbase(unit);
    // unsigned long flags;
    ULONG mc_filter[2];	/* Multicast hash filter */
    int i, j, k, rx_mode;
    ULONG tmp = 0;

    RTLD(bug("[%s] rtl8168nic_SetRxMode()\n", unit->rtl8168u_name))

    if (unit->rtl8168u_flags & IFF_PROMISC) {
	/* Unconditionally log net taps. */
	RTLD(bug("[%s] rtl8168nic_SetRxMode: Promiscuous mode enabled\n", unit->rtl8168u_name))
	rx_mode =
	    AcceptBroadcast | AcceptMulticast | AcceptMyPhys |
	    AcceptAllPhys;
	mc_filter[1] = mc_filter[0] = 0xffffffff;
    } else if ((unit->rtl8168u_mc_count > unit->rtl8168u_device->rtl8168b_MulticastFilterLimit)
	   || (unit->rtl8168u_flags & IFF_ALLMULTI)) {
	/* Too many to filter perfectly -- accept all multicasts. */
	rx_mode = AcceptBroadcast | AcceptMulticast | AcceptMyPhys;
	mc_filter[1] = mc_filter[0] = 0xffffffff;
    } else {
	struct dev_mc_list *mclist;
	rx_mode = AcceptBroadcast | AcceptMyPhys;
	mc_filter[1] = mc_filter[0] = 0;
	for (i = 0, mclist = unit->rtl8168u_mc_list; mclist && i < unit->rtl8168u_mc_count;
	     i++, mclist = mclist->next) {
//		int bit_nr = ether_crc(ETH_ALEN, mclist->dmi_addr) >> 26;
//		mc_filter[bit_nr >> 5] |= 1 << (bit_nr & 31);
		rx_mode |= AcceptMulticast;
	}
    }

    np->rtl8168_rx_config = rtl_chip_info[np->chipset].RCR_Cfg;
    tmp = np->rtl8168_rx_config | rx_mode | (RTL_R32(base + (RxConfig)) & rtl_chip_info[np->chipset].RxConfigMask);

    for (j = 0; j < 2; j++) {
	ULONG mask = 0x000000ff;
	ULONG tmp1 = 0;
	ULONG tmp2 = 0;
	int x = 0;
	int y = 0;

	for (k = 0; k < 4; k++) {
	    tmp1 = mc_filter[j] & mask;
	    x = 32 - (8 + 16 * k);
	    y = x - 2 * x;
	    
	    if (x > 0)
		    tmp2 = tmp2 | (tmp1 << x);
	    else
		    tmp2 = tmp2 | (tmp1 >> y);

	    mask = mask << 8;
	}
	mc_filter[j] = tmp2;
    }

    RTL_W32(base + (RxConfig), tmp);
    RTL_W32(base + (MAR0 + 0), mc_filter[1]);
    RTL_W32(base + (MAR0 + 4), mc_filter[0]);
}

static void rtl8168nic_HWStart(struct net_device *unit)
{
    struct rtl8168_priv *np = get_pcnpriv(unit);
    APTR base = get_hwbase(unit);

    struct pHidd_PCIDevice_WriteConfigByte pcibyte;

    UBYTE device_control;
    UWORD ephy_data;
    ULONG csi_tmp;

    RTLD(bug("[%s] rtl8168nic_HWStart()\n", unit->rtl8168u_name))

    rtl8168nic_NICReset(unit);

    RTL_W8(base + (Cfg9346), Cfg9346_Unlock);

    RTL_W8(base + (Reserved1), Reserved1_data);

    np->cp_cmd |= PktCntrDisable | INTT_1;
    RTL_W16(base + (CPlusCmd), np->cp_cmd);

    RTL_W16(base + (IntrMitigate), 0x5151);

    //Work around for RxFIFO overflow
    if (np->mcfg == CFG_METHOD_1) {
	unit->rtl8168u_intr_mask |= RxFIFOOver | PCSTimeout;
	unit->rtl8168u_intr_mask &= ~RxDescUnavail;
    }

    RTL_W32(base + (TxDescStartAddrLow), ((UQUAD) (IPTR)np->TxPhyAddr & DMA_32BIT_MASK));
    RTL_W32(base + (TxDescStartAddrHigh), ((UQUAD) (IPTR)np->TxPhyAddr >> 32));
    RTL_W32(base + (RxDescAddrLow), ((UQUAD) (IPTR)np->RxPhyAddr & DMA_32BIT_MASK));
    RTL_W32(base + (RxDescAddrHigh), ((UQUAD) (IPTR)np->RxPhyAddr >> 32));

    /* Set Rx Config register */
    rtl8168nic_SetRxMode(unit);

    /* Set DMA burst size and Interframe Gap Time */
    if (np->mcfg == CFG_METHOD_1) {
	RTL_W32(base + (TxConfig), (TX_DMA_BURST_512 << TxDMAShift) | 
	    (InterFrameGap << TxInterFrameGapShift));
    } else {
	RTL_W32(base + (TxConfig), (TX_DMA_BURST_unlimited << TxDMAShift) | 
	    (InterFrameGap << TxInterFrameGapShift));
    }

    /* Clear the interrupt status register. */
    RTL_W16(base + (IntrStatus), 0xFFFF);

    if (np->rx_fifo_overflow == 0) {
	/* Enable all known interrupts by setting the interrupt mask. */
	RTL_W16(base + (IntrMask), unit->rtl8168u_intr_mask);
	netif_start_queue(unit);
    }

    if (np->mcfg == CFG_METHOD_4) {
	/*set PCI configuration space offset 0x70F to 0x27*/
	/*When the register offset of PCI configuration space larger than 0xff, use CSI to access it.*/
	csi_tmp = rtl8168nic_CSIRead(unit, 0x70c) & 0x00ffffff;
	rtl8168nic_CSIWrite(unit, 0x70c, csi_tmp | 0x27000000);

	RTL_W8(base + (DBG_reg), (0x0E << 4) | Fix_Nak_1 | Fix_Nak_2);

	/*Set EPHY registers	begin*/
	/*Set EPHY register offset 0x02 bit 11 to 0 and bit 12 to 1*/
	ephy_data = rtl8168nic_EPHYRead(unit, 0x02);
	ephy_data &= ~(1 << 11);
	ephy_data |= (1 << 12);
	rtl8168nic_EPHYWrite(unit, 0x02, ephy_data);

	/*Set EPHY register offset 0x03 bit 1 to 1*/
	ephy_data = rtl8168nic_EPHYRead(unit, 0x03);
	ephy_data |= (1 << 1);
	rtl8168nic_EPHYWrite(unit, 0x03, ephy_data);

	/*Set EPHY register offset 0x06 bit 7 to 0*/
	ephy_data = rtl8168nic_EPHYRead(unit, 0x06);
	ephy_data &= ~(1 << 7);
	rtl8168nic_EPHYWrite(unit, 0x06, ephy_data);
	/*Set EPHY registers	end*/

	RTL_W8(base + (Config3), RTL_R8(base + (Config3)) & ~Beacon_en);

	//disable clock request.
	pcibyte.mID = OOP_GetMethodID(CLID_Hidd_PCIDevice, moHidd_PCIDevice_WriteConfigByte);
	pcibyte.reg = 0x81;
	pcibyte.val = 0x00;
	OOP_DoMethod(unit->rtl8168u_PCIDevice, (OOP_Msg)&pcibyte);

	RTL_W16(base + (CPlusCmd), RTL_R16(base + (CPlusCmd)) & 
	    ~(EnableBist | Macdbgo_oe | Force_halfdup | Force_rxflow_en | Force_txflow_en | 
	      Cxpl_dbg_sel | ASF | PktCntrDisable | Macdbgo_sel));

	if (unit->rtl8168u_mtu > ETH_DATA_LEN) {
	    RTL_W8(base + (Reserved1), Reserved1_data);
	    RTL_W8(base + (Config3), RTL_R8(base + (Config3)) | Jumbo_En0);
	    RTL_W8(base + (Config4), RTL_R8(base + (Config4)) | Jumbo_En1);

	    //Set PCI configuration space offset 0x79 to 0x20
	    /*Increase the Tx performance*/
	    pcibyte.mID = OOP_GetMethodID(CLID_Hidd_PCIDevice, moHidd_PCIDevice_ReadConfigByte);
	    pcibyte.reg = 0x79;
	    device_control = (UBYTE)OOP_DoMethod(unit->rtl8168u_PCIDevice, (OOP_Msg)&pcibyte);
	    device_control &= ~0x70;
	    device_control |= 0x20;
	    pcibyte.mID = OOP_GetMethodID(CLID_Hidd_PCIDevice, moHidd_PCIDevice_WriteConfigByte);
	    pcibyte.reg = 0x79;
	    pcibyte.val = device_control;
	    OOP_DoMethod(unit->rtl8168u_PCIDevice, (OOP_Msg)&pcibyte);

	    //tx checksum offload disable
//		    unit->features &= ~NETIF_F_IP_CSUM;

	    //rx checksum offload disable
	    np->cp_cmd &= ~RxChkSum;
	    RTL_W16(base + (CPlusCmd), np->cp_cmd);
	} else {
	    RTL_W8(base + (Reserved1), Reserved1_data);
	    RTL_W8(base + (Config3), RTL_R8(base + (Config3)) & ~Jumbo_En0);
	    RTL_W8(base + (Config4), RTL_R8(base + (Config4)) & ~Jumbo_En1);

	    //Set PCI configuration space offset 0x79 to 0x50
	    /*Increase the Tx performance*/
	    pcibyte.mID = OOP_GetMethodID(CLID_Hidd_PCIDevice, moHidd_PCIDevice_ReadConfigByte);
	    pcibyte.reg = 0x79;
	    device_control = (UBYTE)OOP_DoMethod(unit->rtl8168u_PCIDevice, (OOP_Msg)&pcibyte);
	    device_control &= ~0x70;
	    device_control |= 0x50;
	    pcibyte.mID = OOP_GetMethodID(CLID_Hidd_PCIDevice, moHidd_PCIDevice_WriteConfigByte);
	    pcibyte.reg = 0x79;
	    pcibyte.val = device_control;
	    OOP_DoMethod(unit->rtl8168u_PCIDevice, (OOP_Msg)&pcibyte);

	    //tx checksum offload enable
//		    unit->features |= NETIF_F_IP_CSUM;

	    //rx checksum offload enable
	    np->cp_cmd |= RxChkSum;
	    RTL_W16(base + (CPlusCmd), np->cp_cmd);
	}
    } else if (np->mcfg == CFG_METHOD_5) {
	/*set PCI configuration space offset 0x70F to 0x27*/
	/*When the register offset of PCI configuration space larger than 0xff, use CSI to access it.*/
	csi_tmp = rtl8168nic_CSIRead(unit, 0x70c) & 0x00ffffff;
	rtl8168nic_CSIWrite(unit, 0x70c, csi_tmp | 0x27000000);

	/******set EPHY registers for RTL8168CP	begin******/
	//Set EPHY register offset 0x01 bit 0 to 1.
	ephy_data = rtl8168nic_EPHYRead(unit, 0x01);
	ephy_data |= (1 << 0);
	rtl8168nic_EPHYWrite(unit, 0x01, ephy_data);

	//Set EPHY register offset 0x03 bit 10 to 0, bit 9 to 1 and bit 5 to 1.
	ephy_data = rtl8168nic_EPHYRead(unit, 0x03);
	ephy_data &= ~(1 << 10);
	ephy_data |= (1 << 9);
	ephy_data |= (1 << 5);
	rtl8168nic_EPHYWrite(unit, 0x03, ephy_data);
	/******set EPHY registers for RTL8168CP	end******/

	RTL_W8(base + (Config3), RTL_R8(base + (Config3)) & ~Beacon_en);

	//disable clock request.
	pcibyte.mID = OOP_GetMethodID(CLID_Hidd_PCIDevice, moHidd_PCIDevice_WriteConfigByte);
	pcibyte.reg = 0x81;
	pcibyte.val = 0x00;
	OOP_DoMethod(unit->rtl8168u_PCIDevice, (OOP_Msg)&pcibyte);

	RTL_W16(base + (CPlusCmd), RTL_R16(base + (CPlusCmd)) & 
	    ~(EnableBist | Macdbgo_oe | Force_halfdup | Force_rxflow_en | Force_txflow_en | 
	      Cxpl_dbg_sel | ASF | PktCntrDisable | Macdbgo_sel));

	if (unit->rtl8168u_mtu > ETH_DATA_LEN) {
	    RTL_W8(base + (Reserved1), Reserved1_data);
	    RTL_W8(base + (Config3), RTL_R8(base + (Config3)) | Jumbo_En0);
	    RTL_W8(base + (Config4), RTL_R8(base + (Config4)) | Jumbo_En1);

	    //Set PCI configuration space offset 0x79 to 0x20
	    /*Increase the Tx performance*/
	    pcibyte.mID = OOP_GetMethodID(CLID_Hidd_PCIDevice, moHidd_PCIDevice_ReadConfigByte);
	    pcibyte.reg = 0x79;
	    device_control = (UBYTE)OOP_DoMethod(unit->rtl8168u_PCIDevice, (OOP_Msg)&pcibyte);
	    device_control &= ~0x70;
	    device_control |= 0x20;
	    pcibyte.mID = OOP_GetMethodID(CLID_Hidd_PCIDevice, moHidd_PCIDevice_WriteConfigByte);
	    pcibyte.reg = 0x79;
	    pcibyte.val = device_control;
	    OOP_DoMethod(unit->rtl8168u_PCIDevice, (OOP_Msg)&pcibyte);

	    //tx checksum offload disable
//		    unit->features &= ~NETIF_F_IP_CSUM;

	    //rx checksum offload disable
	    np->cp_cmd &= ~RxChkSum;
	    RTL_W16(base + (CPlusCmd), np->cp_cmd);
	} else {
	    RTL_W8(base + (Reserved1), Reserved1_data);
	    RTL_W8(base + (Config3), RTL_R8(base + (Config3)) & ~Jumbo_En0);
	    RTL_W8(base + (Config4), RTL_R8(base + (Config4)) & ~Jumbo_En1);

	    //Set PCI configuration space offset 0x79 to 0x50
	    /*Increase the Tx performance*/
	    pcibyte.mID = OOP_GetMethodID(CLID_Hidd_PCIDevice, moHidd_PCIDevice_ReadConfigByte);
	    pcibyte.reg = 0x79;
	    device_control = (UBYTE)OOP_DoMethod(unit->rtl8168u_PCIDevice, (OOP_Msg)&pcibyte);
	    device_control &= ~0x70;
	    device_control |= 0x50;
	    pcibyte.mID = OOP_GetMethodID(CLID_Hidd_PCIDevice, moHidd_PCIDevice_WriteConfigByte);
	    pcibyte.reg = 0x79;
	    pcibyte.val = device_control;
	    OOP_DoMethod(unit->rtl8168u_PCIDevice, (OOP_Msg)&pcibyte);

	    //tx checksum offload enable
//		    unit->features |= NETIF_F_IP_CSUM;

	    //rx checksum offload enable
	    np->cp_cmd |= RxChkSum;
	    RTL_W16(base + (CPlusCmd), np->cp_cmd);
	}
    } else if (np->mcfg == CFG_METHOD_6) {
	/*set PCI configuration space offset 0x70F to 0x27*/
	/*When the register offset of PCI configuration space larger than 0xff, use CSI to access it.*/
	csi_tmp = rtl8168nic_CSIRead(unit, 0x70c) & 0x00ffffff;
	rtl8168nic_CSIWrite(unit, 0x70c, csi_tmp | 0x27000000);

	RTL_W8(base + (Config3), RTL_R8(base + (Config3)) & ~Beacon_en);

	RTL_W16(base + (CPlusCmd), RTL_R16(base + (CPlusCmd)) & 
	    ~(EnableBist | Macdbgo_oe | Force_halfdup | Force_rxflow_en | Force_txflow_en | 
	      Cxpl_dbg_sel | ASF | PktCntrDisable | Macdbgo_sel));

	if (unit->rtl8168u_mtu > ETH_DATA_LEN) {
	    RTL_W8(base + (Reserved1), Reserved1_data);
	    RTL_W8(base + (Config3), RTL_R8(base + (Config3)) | Jumbo_En0);
	    RTL_W8(base + (Config4), RTL_R8(base + (Config4)) | Jumbo_En1);

	    //Set PCI configuration space offset 0x79 to 0x20
	    /*Increase the Tx performance*/
	    pcibyte.mID = OOP_GetMethodID(CLID_Hidd_PCIDevice, moHidd_PCIDevice_ReadConfigByte);
	    pcibyte.reg = 0x79;
	    device_control = (UBYTE)OOP_DoMethod(unit->rtl8168u_PCIDevice, (OOP_Msg)&pcibyte);
	    device_control &= ~0x70;
	    device_control |= 0x20;
	    pcibyte.mID = OOP_GetMethodID(CLID_Hidd_PCIDevice, moHidd_PCIDevice_WriteConfigByte);
	    pcibyte.reg = 0x79;
	    pcibyte.val = device_control;
	    OOP_DoMethod(unit->rtl8168u_PCIDevice, (OOP_Msg)&pcibyte);

	    //tx checksum offload disable
//		    unit->features &= ~NETIF_F_IP_CSUM;

	    //rx checksum offload disable
	    np->cp_cmd &= ~RxChkSum;
	    RTL_W16(base + (CPlusCmd), np->cp_cmd);
	} else {
	    RTL_W8(base + (Reserved1), Reserved1_data);
	    RTL_W8(base + (Config3), RTL_R8(base + (Config3)) & ~Jumbo_En0);
	    RTL_W8(base + (Config4), RTL_R8(base + (Config4)) & ~Jumbo_En1);

	    //Set PCI configuration space offset 0x79 to 0x50
	    /*Increase the Tx performance*/
	    pcibyte.mID = OOP_GetMethodID(CLID_Hidd_PCIDevice, moHidd_PCIDevice_ReadConfigByte);
	    pcibyte.reg = 0x79;
	    device_control = (UBYTE)OOP_DoMethod(unit->rtl8168u_PCIDevice, (OOP_Msg)&pcibyte);
	    device_control &= ~0x70;
	    device_control |= 0x50;
	    pcibyte.mID = OOP_GetMethodID(CLID_Hidd_PCIDevice, moHidd_PCIDevice_WriteConfigByte);
	    pcibyte.reg = 0x79;
	    pcibyte.val = device_control;
	    OOP_DoMethod(unit->rtl8168u_PCIDevice, (OOP_Msg)&pcibyte);

	    //tx checksum offload enable
//		    unit->features |= NETIF_F_IP_CSUM;

	    //rx checksum offload enable
	    np->cp_cmd |= RxChkSum;
	    RTL_W16(base + (CPlusCmd), np->cp_cmd);
	}
    } else if (np->mcfg == CFG_METHOD_7) {
	/*set PCI configuration space offset 0x70F to 0x27*/
	/*When the register offset of PCI configuration space larger than 0xff, use CSI to access it.*/
	csi_tmp = rtl8168nic_CSIRead(unit, 0x70c) & 0x00ffffff;
	rtl8168nic_CSIWrite(unit, 0x70c, csi_tmp | 0x27000000);

	RTL_W16(base + (CPlusCmd), RTL_R16(base + (CPlusCmd)) & 
	    ~(EnableBist | Macdbgo_oe | Force_halfdup | Force_rxflow_en | Force_txflow_en | 
	      Cxpl_dbg_sel | ASF | PktCntrDisable | Macdbgo_sel));

	RTL_W8(base + (Config3), RTL_R8(base + (Config3)) & ~Beacon_en);

	if (unit->rtl8168u_mtu > ETH_DATA_LEN) {
	    RTL_W8(base + (Reserved1), Reserved1_data);
	    RTL_W8(base + (Config3), RTL_R8(base + (Config3)) | Jumbo_En0);
	    RTL_W8(base + (Config4), RTL_R8(base + (Config4)) | Jumbo_En1);

	    //Set PCI configuration space offset 0x79 to 0x20
	    /*Increase the Tx performance*/
	    pcibyte.mID = OOP_GetMethodID(CLID_Hidd_PCIDevice, moHidd_PCIDevice_ReadConfigByte);
	    pcibyte.reg = 0x79;
	    device_control = (UBYTE)OOP_DoMethod(unit->rtl8168u_PCIDevice, (OOP_Msg)&pcibyte);
	    device_control &= ~0x70;
	    device_control |= 0x20;
	    pcibyte.mID = OOP_GetMethodID(CLID_Hidd_PCIDevice, moHidd_PCIDevice_WriteConfigByte);
	    pcibyte.reg = 0x79;
	    pcibyte.val = device_control;
	    OOP_DoMethod(unit->rtl8168u_PCIDevice, (OOP_Msg)&pcibyte);

	    //tx checksum offload disable
//		    unit->features &= ~NETIF_F_IP_CSUM;

	    //rx checksum offload disable
	    np->cp_cmd &= ~RxChkSum;
	    RTL_W16(base + (CPlusCmd), np->cp_cmd);
	} else {
	    RTL_W8(base + (Reserved1), Reserved1_data);
	    RTL_W8(base + (Config3), RTL_R8(base + (Config3)) & ~Jumbo_En0);
	    RTL_W8(base + (Config4), RTL_R8(base + (Config4)) & ~Jumbo_En1);

	    //Set PCI configuration space offset 0x79 to 0x50
	    /*Increase the Tx performance*/
	    pcibyte.mID = OOP_GetMethodID(CLID_Hidd_PCIDevice, moHidd_PCIDevice_ReadConfigByte);
	    pcibyte.reg = 0x79;
	    device_control = (UBYTE)OOP_DoMethod(unit->rtl8168u_PCIDevice, (OOP_Msg)&pcibyte);
	    device_control &= ~0x70;
	    device_control |= 0x50;
	    pcibyte.mID = OOP_GetMethodID(CLID_Hidd_PCIDevice, moHidd_PCIDevice_WriteConfigByte);
	    pcibyte.reg = 0x79;
	    pcibyte.val = device_control;
	    OOP_DoMethod(unit->rtl8168u_PCIDevice, (OOP_Msg)&pcibyte);

	    //tx checksum offload enable
//		    unit->features |= NETIF_F_IP_CSUM;

	    //rx checksum offload enable
	    np->cp_cmd |= RxChkSum;
	    RTL_W16(base + (CPlusCmd), np->cp_cmd);
	}
    } else if (np->mcfg == CFG_METHOD_8) {
	/*set PCI configuration space offset 0x70F to 0x27*/
	/*When the register offset of PCI configuration space larger than 0xff, use CSI to access it.*/
	csi_tmp = rtl8168nic_CSIRead(unit, 0x70c) & 0x00ffffff;
	rtl8168nic_CSIWrite(unit, 0x70c, csi_tmp | 0x27000000);

	RTL_W16(base + (CPlusCmd), RTL_R16(base + (CPlusCmd)) & 
	    ~(EnableBist | Macdbgo_oe | Force_halfdup | Force_rxflow_en | Force_txflow_en | 
	      Cxpl_dbg_sel | ASF | PktCntrDisable | Macdbgo_sel));

	RTL_W8(base + (Config3), RTL_R8(base + (Config3)) & ~Beacon_en);

	RTL_W8(base + (0xD1), 0x20);

	if (unit->rtl8168u_mtu > ETH_DATA_LEN) {
	    RTL_W8(base + (Reserved1), Reserved1_data);
	    RTL_W8(base + (Config3), RTL_R8(base + (Config3)) | Jumbo_En0);
	    RTL_W8(base + (Config4), RTL_R8(base + (Config4)) | Jumbo_En1);

	    //Set PCI configuration space offset 0x79 to 0x20
	    /*Increase the Tx performance*/
	    pcibyte.mID = OOP_GetMethodID(CLID_Hidd_PCIDevice, moHidd_PCIDevice_ReadConfigByte);
	    pcibyte.reg = 0x79;
	    device_control = (UBYTE)OOP_DoMethod(unit->rtl8168u_PCIDevice, (OOP_Msg)&pcibyte);
	    device_control &= ~0x70;
	    device_control |= 0x20;
	    pcibyte.mID = OOP_GetMethodID(CLID_Hidd_PCIDevice, moHidd_PCIDevice_WriteConfigByte);
	    pcibyte.reg = 0x79;
	    pcibyte.val = device_control;
	    OOP_DoMethod(unit->rtl8168u_PCIDevice, (OOP_Msg)&pcibyte);

	    //tx checksum offload disable
//		    unit->features &= ~NETIF_F_IP_CSUM;

	    //rx checksum offload disable
	    np->cp_cmd &= ~RxChkSum;
	    RTL_W16(base + (CPlusCmd), np->cp_cmd);
	} else {
	    RTL_W8(base + (Reserved1), Reserved1_data);
	    RTL_W8(base + (Config3), RTL_R8(base + (Config3)) & ~Jumbo_En0);
	    RTL_W8(base + (Config4), RTL_R8(base + (Config4)) & ~Jumbo_En1);

	    //Set PCI configuration space offset 0x79 to 0x50
	    /*Increase the Tx performance*/
	    pcibyte.mID = OOP_GetMethodID(CLID_Hidd_PCIDevice, moHidd_PCIDevice_ReadConfigByte);
	    pcibyte.reg = 0x79;
	    device_control = (UBYTE)OOP_DoMethod(unit->rtl8168u_PCIDevice, (OOP_Msg)&pcibyte);
	    device_control &= ~0x70;
	    device_control |= 0x50;
	    pcibyte.mID = OOP_GetMethodID(CLID_Hidd_PCIDevice, moHidd_PCIDevice_WriteConfigByte);
	    pcibyte.reg = 0x79;
	    pcibyte.val = device_control;
	    OOP_DoMethod(unit->rtl8168u_PCIDevice, (OOP_Msg)&pcibyte);

	    //tx checksum offload enable
//		    unit->features |= NETIF_F_IP_CSUM;

	    //rx checksum offload enable
	    np->cp_cmd |= RxChkSum;
	    RTL_W16(base + (CPlusCmd), np->cp_cmd);
	}

    } else if (np->mcfg == CFG_METHOD_9) {
	/*set PCI configuration space offset 0x70F to 0x27*/
	/*When the register offset of PCI configuration space larger than 0xff, use CSI to access it.*/
	csi_tmp = rtl8168nic_CSIRead(unit, 0x70c) & 0x00ffffff;
	rtl8168nic_CSIWrite(unit, 0x70c, csi_tmp | 0x27000000);

	//disable clock request.
	pcibyte.mID = OOP_GetMethodID(CLID_Hidd_PCIDevice, moHidd_PCIDevice_WriteConfigByte);
	pcibyte.reg = 0x81;
	pcibyte.val = 0x00;
	OOP_DoMethod(unit->rtl8168u_PCIDevice, (OOP_Msg)&pcibyte);

	RTL_W8(base + (Config1), 0xCF);
	RTL_W8(base + (Config2), 0x9C);
	RTL_W8(base + (Config3), 0x62);

	if (unit->rtl8168u_mtu > ETH_DATA_LEN) {
	    RTL_W8(base + (Reserved1), Reserved1_data);
	    RTL_W8(base + (Config3), RTL_R8(base + (Config3)) | Jumbo_En0);
	    RTL_W8(base + (Config4), RTL_R8(base + (Config4)) | Jumbo_En1);

	    //Set PCI configuration space offset 0x79 to 0x20
	    pcibyte.mID = OOP_GetMethodID(CLID_Hidd_PCIDevice, moHidd_PCIDevice_ReadConfigByte);
	    pcibyte.reg = 0x79;
	    device_control = (UBYTE)OOP_DoMethod(unit->rtl8168u_PCIDevice, (OOP_Msg)&pcibyte);
	    device_control &= ~0x70;
	    device_control |= 0x20;
	    pcibyte.mID = OOP_GetMethodID(CLID_Hidd_PCIDevice, moHidd_PCIDevice_WriteConfigByte);
	    pcibyte.reg = 0x79;
	    pcibyte.val = device_control;
	    OOP_DoMethod(unit->rtl8168u_PCIDevice, (OOP_Msg)&pcibyte);

	    //tx checksum offload disable
//		    unit->features &= ~NETIF_F_IP_CSUM;

	    //rx checksum offload disable
	    np->cp_cmd &= ~RxChkSum;
	    RTL_W16(base + (CPlusCmd), np->cp_cmd);
	} else {
	    RTL_W8(base + (Reserved1), Reserved1_data);
	    RTL_W8(base + (Config3), RTL_R8(base + (Config3)) & ~Jumbo_En0);
	    RTL_W8(base + (Config4), RTL_R8(base + (Config4)) & ~Jumbo_En1);

	    //Set PCI configuration space offset 0x79 to 0x50
	    pcibyte.mID = OOP_GetMethodID(CLID_Hidd_PCIDevice, moHidd_PCIDevice_ReadConfigByte);
	    pcibyte.reg = 0x79;
	    device_control = (UBYTE)OOP_DoMethod(unit->rtl8168u_PCIDevice, (OOP_Msg)&pcibyte);
	    device_control &= ~0x70;
	    device_control |= 0x50;
	    pcibyte.mID = OOP_GetMethodID(CLID_Hidd_PCIDevice, moHidd_PCIDevice_WriteConfigByte);
	    pcibyte.reg = 0x79;
	    pcibyte.val = device_control;
	    OOP_DoMethod(unit->rtl8168u_PCIDevice, (OOP_Msg)&pcibyte);

	    //tx checksum offload enable
//		    unit->features |= NETIF_F_IP_CSUM;

	    //rx checksum offload enable
	    np->cp_cmd |= RxChkSum;
	    RTL_W16(base + (CPlusCmd), np->cp_cmd);
	}

	/******set EPHY registers	begin******/
	rtl8168nic_EPHYWrite(unit, 0x01, 0x7C7D);
	rtl8168nic_EPHYWrite(unit, 0x02, 0x091F);
	rtl8168nic_EPHYWrite(unit, 0x06, 0xB271);
	rtl8168nic_EPHYWrite(unit, 0x07, 0xCE00);
	/******set EPHY registers	end******/

    } else if (np->mcfg == CFG_METHOD_10) {
	/*set PCI configuration space offset 0x70F to 0x27*/
	/*When the register offset of PCI configuration space larger than 0xff, use CSI to access it.*/
	csi_tmp = rtl8168nic_CSIRead(unit, 0x70c) & 0x00ffffff;
	rtl8168nic_CSIWrite(unit, 0x70c, csi_tmp | 0x27000000);

	if (unit->rtl8168u_mtu > ETH_DATA_LEN) {
	    RTL_W8(base + (Reserved1), Reserved1_data);
	    RTL_W8(base + (Config3), RTL_R8(base + (Config3)) | Jumbo_En0);
	    RTL_W8(base + (Config4), RTL_R8(base + (Config4)) | Jumbo_En1);

	    //Set PCI configuration space offset 0x79 to 0x20
	    pcibyte.mID = OOP_GetMethodID(CLID_Hidd_PCIDevice, moHidd_PCIDevice_ReadConfigByte);
	    pcibyte.reg = 0x79;
	    device_control = (UBYTE)OOP_DoMethod(unit->rtl8168u_PCIDevice, (OOP_Msg)&pcibyte);
	    device_control &= ~0x70;
	    device_control |= 0x20;
	    pcibyte.mID = OOP_GetMethodID(CLID_Hidd_PCIDevice, moHidd_PCIDevice_WriteConfigByte);
	    pcibyte.reg = 0x79;
	    pcibyte.val = device_control;
	    OOP_DoMethod(unit->rtl8168u_PCIDevice, (OOP_Msg)&pcibyte);

	    //tx checksum offload disable
//		    unit->features &= ~NETIF_F_IP_CSUM;

	    //rx checksum offload disable
	    np->cp_cmd &= ~RxChkSum;
	    RTL_W16(base + (CPlusCmd), np->cp_cmd);
	} else {
	    RTL_W8(base + (Reserved1), Reserved1_data);
	    RTL_W8(base + (Config3), RTL_R8(base + (Config3)) & ~Jumbo_En0);
	    RTL_W8(base + (Config4), RTL_R8(base + (Config4)) & ~Jumbo_En1);

	    //Set PCI configuration space offset 0x79 to 0x50
	    pcibyte.mID = OOP_GetMethodID(CLID_Hidd_PCIDevice, moHidd_PCIDevice_ReadConfigByte);
	    pcibyte.reg = 0x79;
	    device_control = (UBYTE)OOP_DoMethod(unit->rtl8168u_PCIDevice, (OOP_Msg)&pcibyte);
	    device_control &= ~0x70;
	    device_control |= 0x50;
	    pcibyte.mID = OOP_GetMethodID(CLID_Hidd_PCIDevice, moHidd_PCIDevice_WriteConfigByte);
	    pcibyte.reg = 0x79;
	    pcibyte.val = device_control;
	    OOP_DoMethod(unit->rtl8168u_PCIDevice, (OOP_Msg)&pcibyte);

	    //tx checksum offload enable
//		    unit->features |= NETIF_F_IP_CSUM;

	    //rx checksum offload enable
	    np->cp_cmd |= RxChkSum;
	    RTL_W16(base + (CPlusCmd), np->cp_cmd);
	}

	RTL_W8(base + (Config1), 0xDF);

	/******set EPHY registers	begin******/
	rtl8168nic_EPHYWrite(unit, 0x01, 0x7C7D);
	rtl8168nic_EPHYWrite(unit, 0x02, 0x091F);
	rtl8168nic_EPHYWrite(unit, 0x03, 0xC5BA);
	rtl8168nic_EPHYWrite(unit, 0x06, 0xB279);
	rtl8168nic_EPHYWrite(unit, 0x07, 0xAF00);
	rtl8168nic_EPHYWrite(unit, 0x1E, 0xB8EB);
	/******set EPHY registers	end******/

	//disable clock request.
	pcibyte.mID = OOP_GetMethodID(CLID_Hidd_PCIDevice, moHidd_PCIDevice_WriteConfigByte);
	pcibyte.reg = 0x81;
	pcibyte.val = 0x00;
	OOP_DoMethod(unit->rtl8168u_PCIDevice, (OOP_Msg)&pcibyte);

    } else if (np->mcfg == CFG_METHOD_11) {
	/*set PCI configuration space offset 0x70F to 0x27*/
	/*When the register offset of PCI configuration space larger than 0xff, use CSI to access it.*/
	csi_tmp = rtl8168nic_CSIRead(unit, 0x70c) & 0x00ffffff;
	rtl8168nic_CSIWrite(unit, 0x70c, csi_tmp | 0x27000000);

	if (unit->rtl8168u_mtu > ETH_DATA_LEN) {
	    RTL_W8(base + (Reserved1), Reserved1_data);
	    RTL_W8(base + (Config3), RTL_R8(base + (Config3)) | Jumbo_En0);
	    RTL_W8(base + (Config4), RTL_R8(base + (Config4)) | Jumbo_En1);

	    //Set PCI configuration space offset 0x79 to 0x20
	    pcibyte.mID = OOP_GetMethodID(CLID_Hidd_PCIDevice, moHidd_PCIDevice_ReadConfigByte);
	    pcibyte.reg = 0x79;
	    device_control = (UBYTE)OOP_DoMethod(unit->rtl8168u_PCIDevice, (OOP_Msg)&pcibyte);
	    device_control &= ~0x70;
	    device_control |= 0x20;
	    pcibyte.mID = OOP_GetMethodID(CLID_Hidd_PCIDevice, moHidd_PCIDevice_WriteConfigByte);
	    pcibyte.reg = 0x79;
	    pcibyte.val = device_control;
	    OOP_DoMethod(unit->rtl8168u_PCIDevice, (OOP_Msg)&pcibyte);

	    //tx checksum offload disable
//		    unit->features &= ~NETIF_F_IP_CSUM;

	    //rx checksum offload disable
	    np->cp_cmd &= ~RxChkSum;
	    RTL_W16(base + (CPlusCmd), np->cp_cmd);
	} else {
	    RTL_W8(base + (Reserved1), Reserved1_data);
	    RTL_W8(base + (Config3), RTL_R8(base + (Config3)) & ~Jumbo_En0);
	    RTL_W8(base + (Config4), RTL_R8(base + (Config4)) & ~Jumbo_En1);

	    //Set PCI configuration space offset 0x79 to 0x50
	    pcibyte.mID = OOP_GetMethodID(CLID_Hidd_PCIDevice, moHidd_PCIDevice_ReadConfigByte);
	    pcibyte.reg = 0x79;
	    device_control = (UBYTE)OOP_DoMethod(unit->rtl8168u_PCIDevice, (OOP_Msg)&pcibyte);
	    device_control &= ~0x70;
	    device_control |= 0x50;
	    pcibyte.mID = OOP_GetMethodID(CLID_Hidd_PCIDevice, moHidd_PCIDevice_WriteConfigByte);
	    pcibyte.reg = 0x79;
	    pcibyte.val = device_control;
	    OOP_DoMethod(unit->rtl8168u_PCIDevice, (OOP_Msg)&pcibyte);

	    //tx checksum offload enable
//		    unit->features |= NETIF_F_IP_CSUM;

	    //rx checksum offload enable
	    np->cp_cmd |= RxChkSum;
	    RTL_W16(base + (CPlusCmd), np->cp_cmd);
	}

	RTL_W8(base + (Config1), 0xDF);

	/******set EPHY registers	begin******/
	rtl8168nic_EPHYWrite(unit, 0x01, 0x6C7F);
	rtl8168nic_EPHYWrite(unit, 0x02, 0x011F);
	rtl8168nic_EPHYWrite(unit, 0x03, 0xC1B2);
	rtl8168nic_EPHYWrite(unit, 0x1A, 0x0546);
	rtl8168nic_EPHYWrite(unit, 0x1C, 0x80C4);
	rtl8168nic_EPHYWrite(unit, 0x1D, 0x78E4);
	/******set EPHY registers	end******/

	//disable clock request.
	pcibyte.mID = OOP_GetMethodID(CLID_Hidd_PCIDevice, moHidd_PCIDevice_WriteConfigByte);
	pcibyte.reg = 0x81;
	pcibyte.val = 0x00;
	OOP_DoMethod(unit->rtl8168u_PCIDevice, (OOP_Msg)&pcibyte);

	RTL_W8(base + (0xF3), RTL_R8(base + (0xF3)) | (1 << 2));

    } else if (np->mcfg == CFG_METHOD_1) {
	RTL_W8(base + (Config3), RTL_R8(base + (Config3)) & ~Beacon_en);

	RTL_W16(base + (CPlusCmd), RTL_R16(base + (CPlusCmd)) & 
	    ~(EnableBist | Macdbgo_oe | Force_halfdup | Force_rxflow_en | Force_txflow_en | 
	      Cxpl_dbg_sel | ASF | PktCntrDisable | Macdbgo_sel));

	if (unit->rtl8168u_mtu > ETH_DATA_LEN) {
	    pcibyte.mID = OOP_GetMethodID(CLID_Hidd_PCIDevice, moHidd_PCIDevice_ReadConfigByte);
	    pcibyte.reg = 0x69;
	    device_control = (UBYTE)OOP_DoMethod(unit->rtl8168u_PCIDevice, (OOP_Msg)&pcibyte);
	    device_control &= ~0x70;
	    device_control |= 0x28;
	    pcibyte.mID = OOP_GetMethodID(CLID_Hidd_PCIDevice, moHidd_PCIDevice_WriteConfigByte);
	    pcibyte.reg = 0x69;
	    pcibyte.val = device_control;
	    OOP_DoMethod(unit->rtl8168u_PCIDevice, (OOP_Msg)&pcibyte);
	} else {
	    pcibyte.mID = OOP_GetMethodID(CLID_Hidd_PCIDevice, moHidd_PCIDevice_ReadConfigByte);
	    pcibyte.reg = 0x69;
	    device_control = (UBYTE)OOP_DoMethod(unit->rtl8168u_PCIDevice, (OOP_Msg)&pcibyte);
	    device_control &= ~0x70;
	    device_control |= 0x58;
	    pcibyte.mID = OOP_GetMethodID(CLID_Hidd_PCIDevice, moHidd_PCIDevice_WriteConfigByte);
	    pcibyte.reg = 0x69;
	    pcibyte.val = device_control;
	    OOP_DoMethod(unit->rtl8168u_PCIDevice, (OOP_Msg)&pcibyte);
	}
    } else if (np->mcfg == CFG_METHOD_2) {
	RTL_W8(base + (Config3), RTL_R8(base + (Config3)) & ~Beacon_en);

	RTL_W16(base + (CPlusCmd), RTL_R16(base + (CPlusCmd)) & 
	    ~(EnableBist | Macdbgo_oe | Force_halfdup | Force_rxflow_en | Force_txflow_en | 
	      Cxpl_dbg_sel | ASF | PktCntrDisable | Macdbgo_sel));

	if (unit->rtl8168u_mtu > ETH_DATA_LEN) {
	    pcibyte.mID = OOP_GetMethodID(CLID_Hidd_PCIDevice, moHidd_PCIDevice_ReadConfigByte);
	    pcibyte.reg = 0x69;
	    device_control = (UBYTE)OOP_DoMethod(unit->rtl8168u_PCIDevice, (OOP_Msg)&pcibyte);
	    device_control &= ~0x70;
	    device_control |= 0x28;
	    pcibyte.mID = OOP_GetMethodID(CLID_Hidd_PCIDevice, moHidd_PCIDevice_WriteConfigByte);
	    pcibyte.reg = 0x69;
	    pcibyte.val = device_control;
	    OOP_DoMethod(unit->rtl8168u_PCIDevice, (OOP_Msg)&pcibyte);

	    RTL_W8(base + (Reserved1), Reserved1_data);
	    RTL_W8(base + (Config4), RTL_R8(base + (Config4)) | (1 << 0));
	} else {
	    pcibyte.mID = OOP_GetMethodID(CLID_Hidd_PCIDevice, moHidd_PCIDevice_ReadConfigByte);
	    pcibyte.reg = 0x69;
	    device_control = (UBYTE)OOP_DoMethod(unit->rtl8168u_PCIDevice, (OOP_Msg)&pcibyte);
	    device_control &= ~0x70;
	    device_control |= 0x58;
	    pcibyte.mID = OOP_GetMethodID(CLID_Hidd_PCIDevice, moHidd_PCIDevice_WriteConfigByte);
	    pcibyte.reg = 0x69;
	    pcibyte.val = device_control;
	    OOP_DoMethod(unit->rtl8168u_PCIDevice, (OOP_Msg)&pcibyte);

	    RTL_W8(base + (Reserved1), Reserved1_data);
	    RTL_W8(base + (Config4), RTL_R8(base + (Config4)) & ~(1 << 0));
	}
    } else if (np->mcfg == CFG_METHOD_3) {
	RTL_W8(base + (Config3), RTL_R8(base + (Config3)) & ~Beacon_en);

	RTL_W16(base + (CPlusCmd), RTL_R16(base + (CPlusCmd)) & 
	    ~(EnableBist | Macdbgo_oe | Force_halfdup | Force_rxflow_en | Force_txflow_en | 
	      Cxpl_dbg_sel | ASF | PktCntrDisable | Macdbgo_sel));

	if (unit->rtl8168u_mtu > ETH_DATA_LEN) {
	    pcibyte.mID = OOP_GetMethodID(CLID_Hidd_PCIDevice, moHidd_PCIDevice_ReadConfigByte);
	    pcibyte.reg = 0x69;
	    device_control = (UBYTE)OOP_DoMethod(unit->rtl8168u_PCIDevice, (OOP_Msg)&pcibyte);
	    device_control &= ~0x70;
	    device_control |= 0x28;
	    pcibyte.mID = OOP_GetMethodID(CLID_Hidd_PCIDevice, moHidd_PCIDevice_WriteConfigByte);
	    pcibyte.reg = 0x69;
	    pcibyte.val = device_control;
	    OOP_DoMethod(unit->rtl8168u_PCIDevice, (OOP_Msg)&pcibyte);

	    RTL_W8(base + (Reserved1), Reserved1_data);
	    RTL_W8(base + (Config4), RTL_R8(base + (Config4)) | (1 << 0));
	} else {
	    pcibyte.mID = OOP_GetMethodID(CLID_Hidd_PCIDevice, moHidd_PCIDevice_ReadConfigByte);
	    pcibyte.reg = 0x69;
	    device_control = (UBYTE)OOP_DoMethod(unit->rtl8168u_PCIDevice, (OOP_Msg)&pcibyte);
	    device_control &= ~0x70;
	    device_control |= 0x58;
	    pcibyte.mID = OOP_GetMethodID(CLID_Hidd_PCIDevice, moHidd_PCIDevice_WriteConfigByte);
	    pcibyte.reg = 0x69;
	    pcibyte.val = device_control;
	    OOP_DoMethod(unit->rtl8168u_PCIDevice, (OOP_Msg)&pcibyte);

	    RTL_W8(base + (Reserved1), Reserved1_data);
	    RTL_W8(base + (Config4), RTL_R8(base + (Config4)) & ~(1 << 0));
	}
    }

    if ((np->mcfg == CFG_METHOD_1) || (np->mcfg == CFG_METHOD_2) || (np->mcfg == CFG_METHOD_3)) {
	/* csum offload command for RTL8168B/8111B */
	np->tx_tcp_csum_cmd = TxIPCS | TxTCPCS;
	np->tx_udp_csum_cmd = TxIPCS | TxUDPCS;
	np->tx_ip_csum_cmd = TxIPCS;
    } else {
	/* csum offload command for RTL8168C/8111C and RTL8168CP/8111CP */
	np->tx_tcp_csum_cmd = TxIPCS_C | TxTCPCS_C;
	np->tx_udp_csum_cmd = TxIPCS_C | TxUDPCS_C;
	np->tx_ip_csum_cmd = TxIPCS_C;
    }

    RTL_W8(base + (ChipCmd), CmdTxEnb | CmdRxEnb);

    RTL_W8(base + (Cfg9346), Cfg9346_Lock);

    if (!np->pci_cfg_is_read) {
	struct pHidd_PCIDevice_ReadConfigWord readpciword;

	pcibyte.mID = OOP_GetMethodID(CLID_Hidd_PCIDevice, moHidd_PCIDevice_ReadConfigByte);
	pcibyte.reg = PCI_COMMAND;
	np->pci_cfg_space.cmd = (UBYTE)OOP_DoMethod(unit->rtl8168u_PCIDevice, (OOP_Msg)&pcibyte);

	pcibyte.reg = PCI_CACHE_LINE_SIZE;
	np->pci_cfg_space.cls = (UBYTE)OOP_DoMethod(unit->rtl8168u_PCIDevice, (OOP_Msg)&pcibyte);

	readpciword.mID = OOP_GetMethodID(CLID_Hidd_PCIDevice, moHidd_PCIDevice_ReadConfigWord);
	readpciword.reg = PCI_BASE_ADDRESS_0;
	np->pci_cfg_space.io_base_l = (UWORD)OOP_DoMethod(unit->rtl8168u_PCIDevice, (OOP_Msg)&readpciword);
	readpciword.reg = PCI_BASE_ADDRESS_0 + 2;
	np->pci_cfg_space.io_base_h = (UWORD)OOP_DoMethod(unit->rtl8168u_PCIDevice, (OOP_Msg)&readpciword);
	readpciword.reg = PCI_BASE_ADDRESS_2;
	np->pci_cfg_space.mem_base_l = (UWORD)OOP_DoMethod(unit->rtl8168u_PCIDevice, (OOP_Msg)&readpciword);
	readpciword.reg = PCI_BASE_ADDRESS_2 + 2;
	np->pci_cfg_space.mem_base_h = (UWORD)OOP_DoMethod(unit->rtl8168u_PCIDevice, (OOP_Msg)&readpciword);

	pcibyte.reg = PCI_INTERRUPT_LINE;
	np->pci_cfg_space.ilr = (UBYTE)OOP_DoMethod(unit->rtl8168u_PCIDevice, (OOP_Msg)&pcibyte);

	readpciword.reg = PCI_BASE_ADDRESS_4;
	np->pci_cfg_space.resv_0x20_l = (UWORD)OOP_DoMethod(unit->rtl8168u_PCIDevice, (OOP_Msg)&readpciword);
	readpciword.reg = PCI_BASE_ADDRESS_4 + 2;
	np->pci_cfg_space.resv_0x20_h = (UWORD)OOP_DoMethod(unit->rtl8168u_PCIDevice, (OOP_Msg)&readpciword);
	readpciword.reg = PCI_BASE_ADDRESS_5;
	np->pci_cfg_space.resv_0x24_l = (UWORD)OOP_DoMethod(unit->rtl8168u_PCIDevice, (OOP_Msg)&readpciword);
	readpciword.reg = PCI_BASE_ADDRESS_5 + 2;
	np->pci_cfg_space.resv_0x24_h = (UWORD)OOP_DoMethod(unit->rtl8168u_PCIDevice, (OOP_Msg)&readpciword);

	np->pci_cfg_is_read = 1;
    }

    rtl8168nic_DSM(unit, DSM_MAC_INIT);

    udelay(10);
}

static unsigned int rtl8168nic_XMIILinkOK(struct net_device *unit)
{
    // struct rtl8168_priv *np = get_pcnpriv(unit);
    APTR base = get_hwbase(unit);

    mdio_write(unit, 0x1f, 0x0000);

    return RTL_R8(base + (PHYstatus)) & LinkStatus;
}

void rtl8168_CheckLinkStatus(struct net_device *unit)
{
    // struct rtl8168_priv *np = get_pcnpriv(unit);
    // APTR base = get_hwbase(unit);
    // unsigned long flags;

    if (rtl8168nic_XMIILinkOK(unit)) {
	netif_carrier_on(unit);
	RTLD(bug("[%s] rtl8168_CheckLinkStatus: Link Up\n", unit->rtl8168u_name))
    } else {
	RTLD(bug("[%s] rtl8168_CheckLinkStatus: Link Down\n", unit->rtl8168u_name))
	netif_carrier_off(unit);
    }
}

static void rtl8168nic_InitRingIndexes(struct rtl8168_priv *np)
{
    np->dirty_tx = 0;
    np->dirty_rx = 0;
    np->cur_tx = 0;
    np->cur_rx = 0;
}

static void rtl8168nic_TxDescInit(struct rtl8168_priv *np)
{
    int i = 0;

//    memset(np->TxDescArray, 0x0, NUM_TX_DESC * sizeof(struct TxDesc));

    for (i = 0; i < NUM_TX_DESC; i++)
	if(i == (NUM_TX_DESC - 1))
	    np->TxDescArray[i].opts1 = AROS_LONG2LE(RingEnd);
}

static void rtl8168nic_RxDescInit(struct rtl8168_priv *np)
{
    int i = 0;

//    memset(np->RxDescArray, 0x0, NUM_RX_DESC * sizeof(struct RxDesc));

    for (i = 0; i < NUM_RX_DESC; i++) {
	if(i == (NUM_RX_DESC - 1))
	    np->RxDescArray[i].opts1 = AROS_LONG2LE((DescOwn | RingEnd | (ULONG)np->rx_buf_sz));
	else
	    np->RxDescArray[i].opts1 = AROS_LONG2LE(DescOwn | (ULONG)np->rx_buf_sz);
    }
}

static ULONG rtl8168nic_RxFill(struct net_device *unit, ULONG start, ULONG end)
{
    struct rtl8168_priv *np = get_pcnpriv(unit);
    ULONG cur;

    for (cur = start; end - cur > 0; cur++) {
	int i = cur % NUM_RX_DESC;

	if (np->RxDescArray[i].addr)
	    continue;

	if ((np->RxDescArray[i].addr = (IPTR)HIDD_PCIDriver_AllocPCIMem(unit->rtl8168u_PCIDriver, np->rx_buf_sz)) == 0)
	    break;

//	RTLD(bug("[%s] rtl8168nic_RxFill: Rx Buffer %d Allocated @ %p\n", unit->rtl8168u_name, i, np->RxDescArray[i].addr))
    }
    return cur - start;
}

static inline void rtl8168nic_MarkAsLastDescriptor(struct RxDesc *desc)
{
	desc->opts1 |= AROS_LONG2LE(RingEnd);
}

static int rtl8168nic_InitRings(struct net_device *unit)
{
    struct rtl8168_priv *np = get_pcnpriv(unit);

    RTLD(bug("[%s] rtl8168nic_InitRings(unit @ %p)\n", unit->rtl8168u_name, unit))

    rtl8168nic_InitRingIndexes(np);

    rtl8168nic_TxDescInit(np);
    rtl8168nic_RxDescInit(np);

    if (rtl8168nic_RxFill(unit, 0, NUM_RX_DESC) != NUM_RX_DESC)
	    goto err_out;

    rtl8168nic_MarkAsLastDescriptor(np->RxDescArray + NUM_RX_DESC - 1);

    return 0;

err_out:
//    rtl8168_rx_clear(np);
    return -1;
}

static int rtl8168nic_Open(struct net_device *unit)
{
    struct rtl8168_priv *np = get_pcnpriv(unit);
    // APTR base = get_hwbase(unit);
    int ret;

    RTLD(bug("[%s] rtl8168nic_Open(unit @ %p)\n", unit->rtl8168u_name, unit))

    rtl8168nic_SetRXBufSize(unit);

    ret = request_irq(unit);
    if (ret)
	goto out_drain;

    np->TxDescArray = HIDD_PCIDriver_AllocPCIMem(unit->rtl8168u_PCIDriver, R8168_TX_RING_BYTES);
    np->TxPhyAddr = np->TxDescArray;

    np->RxDescArray = HIDD_PCIDriver_AllocPCIMem(unit->rtl8168u_PCIDriver, R8168_RX_RING_BYTES);
    np->RxPhyAddr = np->RxDescArray;

    if ((np->TxDescArray) && (np->RxDescArray))
    {
RTLD(bug("[%s] rtl8168nic_Open: Allocated Descriptor Arrays - Tx @ %p (%d bytes), Rx @ %p (%d bytes)\n", unit->rtl8168u_name,
						np->TxDescArray, R8168_TX_RING_BYTES,
						np->RxDescArray, R8168_RX_RING_BYTES))
	if (rtl8168nic_InitRings(unit) == 0)
	{
	    rtl8168nic_HWStart(unit);

//	    if (np->esd_flag == 0) {
//		rtl8168_request_esd_timer(unit);
//	    }

//	    rtl8168_request_link_timer(unit);

	    rtl8168nic_DSM(unit, DSM_IF_UP);

	    rtl8168_CheckLinkStatus(unit);
	}
	else
	{
RTLD(bug("[%s] rtl8168nic_Open: Failed to initialise Descriptor Arrays!\n",unit->rtl8168u_name))
	}
    }
    else
    {
RTLD(bug("[%s] rtl8168nic_Open: Failed to Allocate Descriptor Arrays!\n",unit->rtl8168u_name))	
    }
    return 0;

out_drain:
    drain_ring(unit);
    return ret;
}

static int rtl8168nic_Close(struct net_device *unit)
{
    struct rtl8168_priv *np = get_pcnpriv(unit);

    RTLD(bug("[%s] rtl8168nic_Close()\n", unit->rtl8168u_name))
    
    unit->rtl8168u_flags &= ~IFF_UP;

    ObtainSemaphore(&np->lock);
//    np->in_shutdown = 1;
    ReleaseSemaphore(&np->lock);

    unit->rtl8168u_toutNEED = FALSE;

    netif_stop_queue(unit);
    ObtainSemaphore(&np->lock);

    rtl8168nic_DeInit(unit);

    ReleaseSemaphore(&np->lock);

    free_irq(unit);

    drain_ring(unit);

//    HIDD_PCIDriver_FreePCIMem(unit->rtl8168u_PCIDriver, np->rx_buffer);
//    HIDD_PCIDriver_FreePCIMem(unit->rtl8168u_PCIDriver, np->tx_buffer);

    ReportEvents(LIBBASE, unit, S2EVENT_OFFLINE);

    return 0;
}


void rtl8168nic_get_functions(struct net_device *Unit)
{
    Unit->initialize = rtl8168nic_Init;
    Unit->deinitialize = rtl8168nic_DeInit;
    Unit->start = rtl8168nic_Open;
    Unit->stop = rtl8168nic_Close;
    Unit->set_mac_address = rtl8168nic_SetMACAddr;
    Unit->set_multicast = rtl8168nic_SetMulticast;
}
