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

#include "rtl816x.h"

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

#include <hardware/intbits.h>

#include <stdlib.h>
#include <string.h>

#include "unit.h"
#include LC_LIBDEFS_FILE

#include "rtl816x_hw.h"

#undef LIBBASE
#define LIBBASE (unit->rtl816xu_device)

void rtl816x_USecDelay(struct net_device *unit, ULONG usec)
{
    if (unit != NULL)
    {
        unit->rtl816xu_DelayPort.mp_SigTask = FindTask(NULL);
        unit->rtl816xu_DelayReq.tr_node.io_Command = TR_ADDREQUEST;
        unit->rtl816xu_DelayReq.tr_time.tv_micro = usec % 1000000;
        unit->rtl816xu_DelayReq.tr_time.tv_secs = usec / 1000000;

        DoIO((struct IORequest *) &unit->rtl816xu_DelayReq);
    }
}

struct rtl816x_priv *get_pcnpriv(struct net_device *unit)
{
    return unit->rtl816xu_priv;
}

UBYTE *get_hwbase(struct net_device *unit)
{
    return (UBYTE *) unit->rtl816xu_BaseMem;
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

void mdio_write(struct net_device *unit, int RegAddr, UWORD value)
{
    APTR base = get_hwbase(unit);
    int i;

    RTL_W32(base + (PHYAR), PHYAR_Write |
                (RegAddr & PHYAR_Reg_Mask) << PHYAR_Reg_shift |
                (value & PHYAR_Data_Mask));

    for (i = 20; i > 0; i--)
    {
        if (!(RTL_R32(base + (PHYAR)) & PHYAR_Flag))
            break;
        udelay(25);
    }
}

ULONG mdio_read(struct net_device *unit, int RegAddr)
{
    APTR base = get_hwbase(unit);
    UWORD value = 0xffff;
    int i;

    RTL_W32(base + (PHYAR), PHYAR_Read | (RegAddr & PHYAR_Reg_Mask) << PHYAR_Reg_shift);

    for (i = 20; i > 0; i--)
    {
        if (RTL_R32(base + (PHYAR)) & PHYAR_Flag)
        {
            value = (UWORD)(RTL_R32(base + (PHYAR)) & PHYAR_Data_Mask);
            break;
        }
        udelay(25);
    }
    return value;
}

void rtl_phy_write(struct net_device *unit, const struct phy_reg *regs, int len)
{
    while (len-- > 0)
    {
        mdio_write(unit, regs->reg, regs->val);
        regs++;
    }
}

void rtl8169_write_gmii_reg_bit(struct net_device *unit, int reg,
                                int bitnum, int bitval)
{
    int val;

    val = mdio_read(unit, reg);
    val = (bitval == 1) ? val | (bitval << bitnum) : val & ~(0x0001 << bitnum);
    mdio_write(unit, reg, val & 0xffff);
}

void rtl_ephy_write(struct net_device *unit, int RegAddr, UWORD value)
{
    APTR base = get_hwbase(unit);
    int i;

    RTL_W32(base + (EPHYAR),
            EPHYAR_Write |
            (RegAddr & EPHYAR_Reg_Mask) << EPHYAR_Reg_shift |
            (value & EPHYAR_Data_Mask));

    for (i = 0; i < 10; i++)
    {
        udelay(100);
        if (!(RTL_R32(base + (EPHYAR)) & EPHYAR_Flag))
            break;
    }

    udelay(20);
}

UWORD rtl_ephy_read(struct net_device *unit, int RegAddr)
{
    APTR base = get_hwbase(unit);
    UWORD value = 0xffff;
    int i;

    RTL_W32(base + (EPHYAR),
            EPHYAR_Read | (RegAddr & EPHYAR_Reg_Mask) << EPHYAR_Reg_shift);

    for (i = 0; i < 10; i++)
    {
        udelay(100);
        if (RTL_R32(base + (EPHYAR)) & EPHYAR_Flag)
        {
            value = (UWORD)(RTL_R32(base + (EPHYAR)) & EPHYAR_Data_Mask);
            break;
        }
    }

    udelay(20);

    return value;
}

void rtl_csi_write(struct net_device *unit, int addr, ULONG value)
{
    APTR base = get_hwbase(unit);
    int i;

    RTL_W32(base + (CSIDR), value);
    RTL_W32(base + (CSIAR),
            CSIAR_Write |
            CSIAR_ByteEn << CSIAR_ByteEn_shift |
            (addr & CSIAR_Addr_Mask));

    for (i = 0; i < 10; i++)
    {
        udelay(100);
        if (!(RTL_R32(base + (CSIAR)) & CSIAR_Flag))
            break;
    }

    udelay(20);
}

ULONG rtl_csi_read(struct net_device *unit, int addr)
{
    APTR base = get_hwbase(unit);
    ULONG value = 0xffffffff;
    int i;

    RTL_W32(base + (CSIAR),
            CSIAR_Read |
            CSIAR_ByteEn << CSIAR_ByteEn_shift |
            (addr & CSIAR_Addr_Mask));

    for (i = 0; i < 10; i++)
    {
        udelay(100);
        if (RTL_R32(base + (CSIAR)) & CSIAR_Flag)
        {
            value = RTL_R32(base + (CSIDR));
            break;
        }
    }

    udelay(20);

    return value;
}

void rtl_pcie_maxread_tweak(struct net_device *unit, int cfgoff, UBYTE val)
{
    UBYTE ctl;

    ctl = HIDD_PCIDevice_ReadConfigByte(unit->rtl816xu_PCIDevice, cfgoff);
    ctl = (ctl & ~0x70) | val;
    HIDD_PCIDevice_WriteConfigByte(unit->rtl816xu_PCIDevice, cfgoff, ctl);
}

static void rtl816x_GetMACVersion(struct net_device *unit)
{
    struct rtl816x_priv *np = get_pcnpriv(unit);
    APTR base = get_hwbase(unit);

    /* MAC version is identified from TxConfig bits 30-20. Entries are
       matched in order, so the specific masks must precede the family
       catch-alls. */
    static const struct
    {
        ULONG mask;
        ULONG val;
        int mac_version;
        const char *name;
    } mac_info[] =
    {
        /* 8168EP family - generic support only */
        { 0x7cf00000, 0x50200000, RTL_GIGA_MAC_VER_49, "RTL8168ep/8111ep" },
        { 0x7c800000, 0x50000000, RTL_GIGA_MAC_VER_49, "RTL8168ep/8111ep" },

        /* 8168H family */
        { 0x7cf00000, 0x54100000, RTL_GIGA_MAC_VER_45, "RTL8168h/8111h" },
        { 0x7c800000, 0x54000000, RTL_GIGA_MAC_VER_46, "RTL8168h/8111h" },

        /* 8411B */
        { 0x7cf00000, 0x5c800000, RTL_GIGA_MAC_VER_44, "RTL8411b" },

        /* 8168GU */
        { 0x7cf00000, 0x50900000, RTL_GIGA_MAC_VER_42, "RTL8168gu/8111gu" },
        { 0x7cf00000, 0x50800000, RTL_GIGA_MAC_VER_42, "RTL8168gu/8111gu" },

        /* 8168G family */
        { 0x7cf00000, 0x4c000000, RTL_GIGA_MAC_VER_40, "RTL8168g/8111g" },
        { 0x7cf00000, 0x4c100000, RTL_GIGA_MAC_VER_41, "RTL8168g/8111g" },
        { 0x7c800000, 0x4c000000, RTL_GIGA_MAC_VER_41, "RTL8168g/8111g" },

        /* 8411 */
        { 0x7cf00000, 0x44200000, RTL_GIGA_MAC_VER_38, "RTL8411" },

        /* 8168F family */
        { 0x7cf00000, 0x48000000, RTL_GIGA_MAC_VER_35, "RTL8168f/8111f" },
        { 0x7cf00000, 0x48100000, RTL_GIGA_MAC_VER_36, "RTL8168f/8111f" },
        { 0x7c800000, 0x48000000, RTL_GIGA_MAC_VER_36, "RTL8168f/8111f" },

        /* 8168E family */
        { 0x7c800000, 0x2c800000, RTL_GIGA_MAC_VER_34, "RTL8168evl/8111evl" },
        { 0x7cf00000, 0x2c100000, RTL_GIGA_MAC_VER_32, "RTL8168e/8111e" },
        { 0x7cf00000, 0x2c200000, RTL_GIGA_MAC_VER_33, "RTL8168e/8111e" },
        { 0x7c800000, 0x2c000000, RTL_GIGA_MAC_VER_33, "RTL8168e/8111e" },

        /* 8168DP family - generic support only */
        { 0x7cf00000, 0x28800000, RTL_GIGA_MAC_VER_28, "RTL8168dp/8111dp" },
        { 0x7cf00000, 0x28a00000, RTL_GIGA_MAC_VER_28, "RTL8168dp/8111dp" },

        /* 8168D family */
        { 0x7cf00000, 0x28100000, RTL_GIGA_MAC_VER_25, "RTL8168d/8111d" },
        { 0x7cf00000, 0x28200000, RTL_GIGA_MAC_VER_26, "RTL8168d/8111d" },
        { 0x7c800000, 0x28000000, RTL_GIGA_MAC_VER_27, "RTL8168d/8111d" },

        /* 8168C/CP family */
        { 0x7cf00000, 0x3c900000, RTL_GIGA_MAC_VER_18, "RTL8168cp/8111cp" },
        { 0x7cf00000, 0x3cb00000, RTL_GIGA_MAC_VER_24, "RTL8168cp/8111cp" },
        { 0x7c800000, 0x3c800000, RTL_GIGA_MAC_VER_24, "RTL8168cp/8111cp" },
        { 0x7cf00000, 0x3c000000, RTL_GIGA_MAC_VER_19, "RTL8168c/8111c" },
        { 0x7cf00000, 0x3c200000, RTL_GIGA_MAC_VER_20, "RTL8168c/8111c" },
        { 0x7cf00000, 0x3c400000, RTL_GIGA_MAC_VER_21, "RTL8168c/8111c" },
        { 0x7c800000, 0x3c000000, RTL_GIGA_MAC_VER_21, "RTL8168c/8111c" },

        /* 8168B family */
        { 0x7cf00000, 0x38000000, RTL_GIGA_MAC_VER_12, "RTL8168b/8111b" },
        { 0x7cf00000, 0x38500000, RTL_GIGA_MAC_VER_17, "RTL8168b/8111b" },
        { 0x7c800000, 0x38000000, RTL_GIGA_MAC_VER_17, "RTL8168b/8111b" },
        { 0x7c800000, 0x30000000, RTL_GIGA_MAC_VER_11, "RTL8168b/8111b" },

        /* 8101 family */
        { 0x7cf00000, 0x34000000, RTL_GIGA_MAC_VER_13, "RTL8101e" },
        { 0x7cf00000, 0x34200000, RTL_GIGA_MAC_VER_16, "RTL8101e" },
        { 0x7c800000, 0x34000000, RTL_GIGA_MAC_VER_16, "RTL8101e" },
        { 0xfc800000, 0x38800000, RTL_GIGA_MAC_VER_15, "RTL8100e" },
        { 0xfc800000, 0x30800000, RTL_GIGA_MAC_VER_14, "RTL8100e" },

        /* 8169/8110 family */
        { 0xfc800000, 0x98000000, RTL_GIGA_MAC_VER_06, "RTL8169sc/8110sc" },
        { 0xfc800000, 0x18000000, RTL_GIGA_MAC_VER_05, "RTL8169sc/8110sc" },
        { 0xfc800000, 0x10000000, RTL_GIGA_MAC_VER_04, "RTL8169sb/8110sb" },
        { 0xfc800000, 0x04000000, RTL_GIGA_MAC_VER_03, "RTL8110s" },
        { 0xfc800000, 0x00800000, RTL_GIGA_MAC_VER_02, "RTL8169s" },
        { 0xfc800000, 0x00000000, RTL_GIGA_MAC_VER_01, "RTL8169" },

        { 0x00000000, 0x00000000, RTL_GIGA_MAC_NONE,   "unknown" }
    }, *p = mac_info;
    ULONG reg;

    reg = RTL_R32(base + (TxConfig));
    while ((reg & p->mask) != p->val)
        p++;

    np->mcfg = p->mac_version;
    unit->rtl816xu_rtl_chipname = p->name;

    if (np->mcfg == RTL_GIGA_MAC_NONE)
    {
        /* Fall back to a family default so an unrecognised revision still
           gets a sensible generic setup */
        bug("[%s] unrecognised MAC version (TxConfig 0x%08x) - using family default\n",
            unit->rtl816xu_name, reg);
        switch (unit->rtl816xu_config)
        {
        case RTL_CFG_0:
            np->mcfg = RTL_GIGA_MAC_VER_01;
            break;
        case RTL_CFG_2:
            np->mcfg = RTL_GIGA_MAC_VER_16;
            break;
        default:
            np->mcfg = RTL_GIGA_MAC_VER_46;
            break;
        }
    }

    RTLD(bug("[%s] rtl816x_GetMACVersion: %s (mcfg %02x, TxConfig 0x%08x)\n",
             unit->rtl816xu_name, unit->rtl816xu_rtl_chipname, np->mcfg, reg))
}

static void rtl_hw_phy_config(struct net_device *unit)
{
    struct rtl816x_priv *np = get_pcnpriv(unit);

    switch (np->mcfg)
    {
    case RTL_GIGA_MAC_VER_02:
    case RTL_GIGA_MAC_VER_03:
        rtl8169s_hw_phy_config(unit);
        break;
    case RTL_GIGA_MAC_VER_04:
        rtl8169sb_hw_phy_config(unit);
        break;
    default:
        if (np->mcfg >= RTL_GIGA_MAC_VER_11)
            rtl8168_hw_phy_config(unit);
        break;
    }
}

static int rtl816x_set_speed_tbi(struct net_device *unit,
                                 UBYTE autoneg, UWORD speed, UBYTE duplex)
{
    APTR base = get_hwbase(unit);
    ULONG reg;

    reg = RTL_R32(base + TBICSR);
    if ((autoneg == AUTONEG_DISABLE) && (speed == SPEED_1000) &&
        (duplex == DUPLEX_FULL))
    {
        RTL_W32(base + TBICSR, reg & ~(TBINwEnable | TBINwRestart));
    }
    else if (autoneg == AUTONEG_ENABLE)
    {
        RTL_W32(base + TBICSR, reg | TBINwEnable | TBINwRestart);
    }
    else
    {
        RTLD(bug("[%s] incorrect speed setting refused in TBI mode\n", unit->rtl816xu_name))
    }

    return 0;
}

static int rtl816x_set_speed_xmii(struct net_device *unit,
                                  UBYTE autoneg, UWORD speed, UBYTE duplex)
{
    struct rtl816x_priv *np = get_pcnpriv(unit);
    int auto_nego = 0;
    int giga_ctrl = 0;

    auto_nego = mdio_read(unit, MII_ADVERTISE);
    auto_nego &= ~(ADVERTISE_10HALF | ADVERTISE_10FULL |
                   ADVERTISE_100HALF | ADVERTISE_100FULL);
    giga_ctrl = mdio_read(unit, MII_CTRL1000);
    giga_ctrl &= ~(ADVERTISE_1000FULL | ADVERTISE_1000HALF);

    if (autoneg == AUTONEG_ENABLE)
    {
        auto_nego |= (ADVERTISE_10HALF | ADVERTISE_10FULL |
                      ADVERTISE_100HALF | ADVERTISE_100FULL);
        giga_ctrl |= ADVERTISE_1000FULL | ADVERTISE_1000HALF;
    }
    else
    {
        if (speed == SPEED_10)
            auto_nego |= ADVERTISE_10HALF | ADVERTISE_10FULL;
        else if (speed == SPEED_100)
            auto_nego |= ADVERTISE_100HALF | ADVERTISE_100FULL;
        else if (speed == SPEED_1000)
            giga_ctrl |= ADVERTISE_1000FULL | ADVERTISE_1000HALF;

        if (duplex == DUPLEX_HALF)
            auto_nego &= ~(ADVERTISE_10FULL | ADVERTISE_100FULL);
        if (duplex == DUPLEX_FULL)
            auto_nego &= ~(ADVERTISE_10HALF | ADVERTISE_100HALF);

        /* This tweak comes straight from Realtek's driver. */
        if ((speed == SPEED_100) && (duplex == DUPLEX_HALF) &&
            ((np->mcfg == RTL_GIGA_MAC_VER_13) ||
             (np->mcfg == RTL_GIGA_MAC_VER_16)))
        {
            auto_nego = ADVERTISE_100HALF | ADVERTISE_CSMA;
        }
    }

    /* The 8100e/8101e do Fast Ethernet only. */
    if ((np->mcfg >= RTL_GIGA_MAC_VER_13) &&
        (np->mcfg <= RTL_GIGA_MAC_VER_16))
    {
        giga_ctrl &= ~(ADVERTISE_1000FULL | ADVERTISE_1000HALF);
    }

    auto_nego |= ADVERTISE_PAUSE_CAP | ADVERTISE_PAUSE_ASYM;

    if (np->mcfg >= RTL_GIGA_MAC_VER_11)
    {
        /* Vendor specific (0x1f) and reserved (0x0e) MII registers -
           takes the PHY out of power-down */
        mdio_write(unit, 0x1f, 0x0000);
        mdio_write(unit, 0x0e, 0x0000);
    }

    np->autoneg = autoneg;
    np->speed = speed;
    np->duplex = duplex;

    mdio_write(unit, MII_ADVERTISE, auto_nego);
    mdio_write(unit, MII_CTRL1000, giga_ctrl);
    mdio_write(unit, MII_BMCR, BMCR_ANENABLE | BMCR_ANRESTART);
    return 0;
}

static void rtl816x_phy_reset(struct net_device *unit)
{
    struct rtl816x_priv *np = get_pcnpriv(unit);
    APTR base = get_hwbase(unit);
    unsigned int i;
    BOOL tbi = (np->mcfg <= RTL_GIGA_MAC_VER_06) &&
               (RTL_R8(base + PHYstatus) & TBI_Enable);

    if (tbi)
        rtl8169_tbi_reset_enable(unit);
    else
        rtl8169_xmii_reset_enable(unit);

    for (i = 0; i < 100; i++)
    {
        if (tbi ? !rtl8169_tbi_reset_pending(unit)
                : !rtl8169_xmii_reset_pending(unit))
            return;
        rtl816x_USecDelay(unit, 100);
    }
}

void rtl816x_irq_mask_and_ack(struct net_device *unit)
{
    APTR base = get_hwbase(unit);

    RTL_W16(base + IntrMask, 0x0000);
    RTL_W16(base + IntrStatus, 0xffff);
}

static void rtl816x_asic_down(struct net_device *unit)
{
    APTR base = get_hwbase(unit);

    RTL_W8(base + ChipCmd, 0x00);
    rtl816x_irq_mask_and_ack(unit);
    RTL_R16(base + CPlusCmd);
}

static void rtl816x_DeInit(struct net_device *unit)
{
    struct rtl816x_priv *np = get_pcnpriv(unit);
    APTR base = get_hwbase(unit);

    rtl816x_asic_down(unit);

    if (np->mcfg <= RTL_GIGA_MAC_VER_06)
        RTL_W32(base + RxMissed, 0);
}

static void rtl816x_GetMACAddr(struct net_device *unit, char *addr)
{
    APTR base = get_hwbase(unit);
    int i;

    for (i = 0; i < MAC_ADDR_LEN; i++)
        addr[i] = RTL_R8(base + MAC0 + i);
}

static void rtl816x_SetMACAddr(struct net_device *unit)
{
    APTR base = get_hwbase(unit);

    RTLD(bug("[%s] rtl816x_SetMACAddr()\n", unit->rtl816xu_name))

    RTL_W8(base + (Cfg9346), Cfg9346_Unlock);

    RTL_W32(base + (MAC0),
            unit->rtl816xu_dev_addr[0] |
            (unit->rtl816xu_dev_addr[1] << 8) |
            (unit->rtl816xu_dev_addr[2] << 16) |
            (unit->rtl816xu_dev_addr[3] << 24));
    RTL_W32(base + (MAC4),
            unit->rtl816xu_dev_addr[4] |
            (unit->rtl816xu_dev_addr[5] << 8));

    RTL_W8(base + (Cfg9346), Cfg9346_Lock);
}

static void rtl816x_LinkOption(struct net_device *unit, UBYTE *aut, UWORD *spd, UBYTE *dup)
{
    int opt_speed = -1;
    int opt_duplex = -1;
    int opt_autoneg = -1;

    if (unit->rtl816xu_UnitNum < MAX_UNITS)
    {
        opt_speed = unit->rtl816xu_device->speed[unit->rtl816xu_UnitNum];
        opt_duplex = unit->rtl816xu_device->duplex[unit->rtl816xu_UnitNum];
        opt_autoneg = unit->rtl816xu_device->autoneg[unit->rtl816xu_UnitNum];
    }

    if ((opt_speed == -1) || (opt_duplex == -1) || (opt_autoneg == -1))
    {
        *spd = SPEED_1000;
        *dup = DUPLEX_FULL;
        *aut = AUTONEG_ENABLE;
    }
    else
    {
        *spd = opt_speed;
        *dup = opt_duplex;
        *aut = opt_autoneg;
    }
}

static void rtl816x_Init(struct net_device *unit)
{
    struct rtl816x_priv *np = get_pcnpriv(unit);
    APTR base = get_hwbase(unit);
    UBYTE autoneg, duplex;
    UWORD speed;
    int i;

    RTLD(bug("[%s] rtl816x_Init(unit @ %p)\n", unit->rtl816xu_name, unit))

    np->intr_event = unit->rtl816xu_intr_event;

    rtl816x_irq_mask_and_ack(unit);

    /* Soft reset the chip. */
    RTL_W8(base + ChipCmd, CmdReset);

    for (i = 0; i < 100; i++)
    {
        if ((RTL_R8(base + ChipCmd) & CmdReset) == 0)
            break;
        rtl816x_USecDelay(unit, 100);
    }

    /* Identify chip attached to board */
    rtl816x_GetMACVersion(unit);

    RTL_W8(base + (Cfg9346), Cfg9346_Unlock);
    RTL_W8(base + (Config1), RTL_R8(base + Config1) | PMEnable);
    RTL_W8(base + (Config5), RTL_R8(base + Config5) & PMEStatus);
    RTL_W8(base + (Cfg9346), Cfg9346_Lock);

    rtl816x_GetMACAddr(unit, (char *)&np->orig_mac[0]);

    for (i = 0; i < ETH_ADDRESSSIZE; i++)
        unit->rtl816xu_dev_addr[i] = unit->rtl816xu_org_addr[i] = np->orig_mac[i];

    RTLD(bug("[%s] rtl816x_Init: MAC Address %02x:%02x:%02x:%02x:%02x:%02x\n", unit->rtl816xu_name,
             unit->rtl816xu_dev_addr[0], unit->rtl816xu_dev_addr[1], unit->rtl816xu_dev_addr[2],
             unit->rtl816xu_dev_addr[3], unit->rtl816xu_dev_addr[4], unit->rtl816xu_dev_addr[5]))

    np->cp_cmd |= RxChkSum;
    np->cp_cmd |= RTL_R16(base + (CPlusCmd));

    rtl_hw_phy_config(unit);

    if (np->mcfg <= RTL_GIGA_MAC_VER_06)
        RTL_W8(base + 0x82, 0x01);

    HIDD_PCIDevice_WriteConfigByte(unit->rtl816xu_PCIDevice, PCI_LATENCY_TIMER, 0x40);

    if (np->mcfg <= RTL_GIGA_MAC_VER_06)
        HIDD_PCIDevice_WriteConfigByte(unit->rtl816xu_PCIDevice, PCI_CACHE_LINE_SIZE, 0x08);

    if (np->mcfg == RTL_GIGA_MAC_VER_02)
    {
        RTL_W8(base + 0x82, 0x01);
        mdio_write(unit, 0x0b, 0x0000);
    }

    rtl816x_phy_reset(unit);

    rtl816x_LinkOption(unit, &autoneg, &speed, &duplex);

    if ((np->mcfg <= RTL_GIGA_MAC_VER_06) &&
        (RTL_R8(base + PHYstatus) & TBI_Enable))
    {
        rtl816x_set_speed_tbi(unit, autoneg, speed, duplex);
    }
    else
    {
        rtl816x_set_speed_xmii(unit, autoneg, speed, duplex);
    }

    RTLD(bug("[%s] rtl816x_Init: Link %d Mbps %s duplex %s\n", unit->rtl816xu_name, speed,
             (duplex == DUPLEX_HALF) ? "half" : "full",
             (autoneg == AUTONEG_ENABLE) ? "(autoneg)" : ""))
}

static int request_irq(struct net_device *unit)
{
    RTLD(bug("[%s] request_irq()\n", unit->rtl816xu_name))

    if (!unit->rtl816xu_IntsAdded)
    {
        if (!HIDD_PCIDevice_AddInterrupt(unit->rtl816xu_PCIDevice, &unit->rtl816xu_irqhandler))
            return 1;
        unit->rtl816xu_IntsAdded = TRUE;
    }

    return 0;
}

static void free_irq(struct net_device *unit)
{
    if (unit->rtl816xu_IntsAdded)
    {
        HIDD_PCIDevice_RemoveInterrupt(unit->rtl816xu_PCIDevice, &unit->rtl816xu_irqhandler);
        unit->rtl816xu_IntsAdded = FALSE;
    }
}

void rtl_set_rx_max_size(struct net_device *unit)
{
    struct rtl816x_priv *np = get_pcnpriv(unit);
    APTR base = get_hwbase(unit);

    RTL_W16(base + (RxMaxSize), np->rx_buf_sz + 1);
}

void rtl_set_rx_tx_desc_registers(struct net_device *unit)
{
    struct rtl816x_priv *np = get_pcnpriv(unit);
    APTR base = get_hwbase(unit);

    /* High halves first: some hosts require TxDescAddrHigh to be
       written before TxDescAddrLow */
    RTL_W32(base + (TxDescStartAddrHigh), ((UQUAD) (IPTR)np->TxPhyAddr >> 32));
    RTL_W32(base + (TxDescStartAddrLow), ((UQUAD) (IPTR)np->TxPhyAddr & DMA_32BIT_MASK));
    RTL_W32(base + (RxDescAddrHigh), ((UQUAD) (IPTR)np->RxPhyAddr >> 32));
    RTL_W32(base + (RxDescAddrLow), ((UQUAD) (IPTR)np->RxPhyAddr & DMA_32BIT_MASK));
}

static ULONG rtl_rx_config(int mcfg)
{
    if (mcfg <= RTL_GIGA_MAC_VER_06)
        return (RX_FIFO_THRESH << RxCfgFIFOShift) | (RX_DMA_BURST_1024 << RxCfgDMAShift);

    switch (mcfg)
    {
    case RTL_GIGA_MAC_VER_13:
    case RTL_GIGA_MAC_VER_14:
    case RTL_GIGA_MAC_VER_15:
    case RTL_GIGA_MAC_VER_16:
        return (RX_FIFO_THRESH << RxCfgFIFOShift) | (RX_DMA_BURST_1024 << RxCfgDMAShift);

    case RTL_GIGA_MAC_VER_11:
    case RTL_GIGA_MAC_VER_12:
    case RTL_GIGA_MAC_VER_17:
        return (Reserved2_data << RxCfgFIFOShift) | (RX_DMA_BURST_unlimited << RxCfgDMAShift);

    case RTL_GIGA_MAC_VER_18:
    case RTL_GIGA_MAC_VER_19:
    case RTL_GIGA_MAC_VER_20:
    case RTL_GIGA_MAC_VER_21:
    case RTL_GIGA_MAC_VER_24:
        return RxCfg_128_int_en | RxCfg_fet_multi_en | (RX_DMA_BURST_unlimited << RxCfgDMAShift);

    case RTL_GIGA_MAC_VER_25:
    case RTL_GIGA_MAC_VER_26:
    case RTL_GIGA_MAC_VER_27:
    case RTL_GIGA_MAC_VER_28:
    case RTL_GIGA_MAC_VER_32:
    case RTL_GIGA_MAC_VER_33:
    case RTL_GIGA_MAC_VER_34:
    case RTL_GIGA_MAC_VER_35:
    case RTL_GIGA_MAC_VER_36:
    case RTL_GIGA_MAC_VER_38:
        return RxCfg_128_int_en | (RX_DMA_BURST_unlimited << RxCfgDMAShift);

    default:    /* 8168G and later */
        return RxCfg_128_int_en | RxCfg_fet_multi_en | RxCfg_early_off |
               (RX_DMA_BURST_unlimited << RxCfgDMAShift);
    }
}

static ULONG rtl_tx_config(int mcfg)
{
    int burst = TX_DMA_BURST_unlimited;

    if ((mcfg <= RTL_GIGA_MAC_VER_06) ||
        ((mcfg >= RTL_GIGA_MAC_VER_13) && (mcfg <= RTL_GIGA_MAC_VER_16)))
        burst = TX_DMA_BURST_1024;
    else if (mcfg == RTL_GIGA_MAC_VER_11)
        burst = TX_DMA_BURST_512;

    return (burst << TxDMAShift) | (InterFrameGap << TxInterFrameGapShift);
}

void rtl_set_rx_tx_config_registers(struct net_device *unit)
{
    struct rtl816x_priv *np = get_pcnpriv(unit);
    APTR base = get_hwbase(unit);
    ULONG cfg;

    cfg = rtl_rx_config(np->mcfg);
    cfg |= (RTL_R32(base + RxConfig) & RTL_RX_CONFIG_MASK);
    RTL_W32(base + RxConfig, cfg);

    RTL_W32(base + TxConfig, rtl_tx_config(np->mcfg));
}

void rtl_set_rx_mode(struct net_device *unit)
{
    struct rtl816x_priv *np = get_pcnpriv(unit);
    APTR base = get_hwbase(unit);
    ULONG mc_filter[2];     /* Multicast hash filter */
    int rx_mode;
    ULONG tmp;

    if (unit->rtl816xu_flags & IFF_PROMISC)
    {
        RTLD(bug("[%s] rtl_set_rx_mode: Promiscuous mode enabled\n", unit->rtl816xu_name))
        rx_mode = AcceptBroadcast |
                  AcceptMulticast |
                  AcceptMyPhys |
                  AcceptAllPhys;
        mc_filter[1] = mc_filter[0] = 0xffffffff;
    }
    else if ((unit->rtl816xu_flags & IFF_ALLMULTI) ||
             (unit->rtl816xu_range_count > 0))
    {
        /* Wanted multicast ranges are filtered in software - accept all
           multicasts at the hardware */
        rx_mode = AcceptBroadcast | AcceptMulticast | AcceptMyPhys;
        mc_filter[1] = mc_filter[0] = 0xffffffff;
    }
    else
    {
        rx_mode = AcceptBroadcast | AcceptMyPhys;
        mc_filter[1] = mc_filter[0] = 0;
    }

    tmp = rtl_rx_config(np->mcfg) | rx_mode |
          (RTL_R32(base + RxConfig) & RTL_RX_CONFIG_MASK);

    if (np->mcfg > RTL_GIGA_MAC_VER_06)
    {
        ULONG data = mc_filter[0];

        mc_filter[0] = swab32(mc_filter[1]);
        mc_filter[1] = swab32(data);
    }

    RTL_W32(base + MAR0 + 0, mc_filter[0]);
    RTL_W32(base + MAR0 + 4, mc_filter[1]);

    RTL_W32(base + RxConfig, tmp);
}

static void rtl816x_SetMulticast(struct net_device *unit)
{
    RTLD(bug("[%s] rtl816x_SetMulticast()\n", unit->rtl816xu_name))

    if (unit->rtl816xu_flags & IFF_UP)
        rtl_set_rx_mode(unit);
}

void rtl816x_CheckLinkStatus(struct net_device *unit)
{
    struct rtl816x_priv *np = get_pcnpriv(unit);
    APTR base = get_hwbase(unit);
    BOOL tbi = (np->mcfg <= RTL_GIGA_MAC_VER_06) &&
               (RTL_R8(base + PHYstatus) & TBI_Enable);
    int result;

    if (tbi)
        result = rtl8169_tbi_link_ok(unit);
    else
        result = rtl8169_xmii_link_ok(unit);

    if (result)
    {
        UBYTE phystatus = RTL_R8(base + PHYstatus);

        if (tbi || (phystatus & _1000bpsF))
            unit->rtl816xu_rtl_LinkSpeed = 1000000000;
        else if (phystatus & _100bps)
            unit->rtl816xu_rtl_LinkSpeed = 100000000;
        else
            unit->rtl816xu_rtl_LinkSpeed = 10000000;

        netif_carrier_on(unit);
        RTLD(bug("[%s] rtl816x_CheckLinkStatus: Link Up (%d Mbps)\n",
                 unit->rtl816xu_name, unit->rtl816xu_rtl_LinkSpeed / 1000000))
    }
    else
    {
        RTLD(bug("[%s] rtl816x_CheckLinkStatus: Link Down\n", unit->rtl816xu_name))
        unit->rtl816xu_rtl_LinkSpeed = 0;
        netif_carrier_off(unit);
    }
}

static void rtl816x_TxDescInit(struct rtl816x_priv *np)
{
    int i;

    for (i = 0; i < NUM_TX_DESC; i++)
    {
        np->TxDescArray[i].opts1 =
            AROS_LONG2LE((i == (NUM_TX_DESC - 1)) ? RingEnd : 0);
        np->TxDescArray[i].opts2 = 0;
    }
}

static void rtl816x_RxDescInit(struct rtl816x_priv *np)
{
    int i;

    for (i = 0; i < NUM_RX_DESC; i++)
    {
        np->RxDescArray[i].opts1 =
            AROS_LONG2LE(DescOwn | (ULONG)np->rx_buf_sz |
                         ((i == (NUM_RX_DESC - 1)) ? RingEnd : 0));
        np->RxDescArray[i].opts2 = 0;
    }
}

static ULONG rtl816x_TxFill(struct net_device *unit)
{
    struct rtl816x_priv *np = get_pcnpriv(unit);
    ULONG cur;

    for (cur = 0; cur < NUM_TX_DESC; cur++)
    {
        APTR buf;

        if (np->TxDescArray[cur].addr != 0)
            continue;

        if ((buf = HIDD_PCIDriver_AllocPCIMem(unit->rtl816xu_PCIDriver, TX_BUF_SIZE)) == NULL)
            break;

        np->TxDescArray[cur].addr = AROS_QUAD2LE((UQUAD)(IPTR)buf);
    }
    return cur;
}

static ULONG rtl816x_RxFill(struct net_device *unit)
{
    struct rtl816x_priv *np = get_pcnpriv(unit);
    ULONG cur;

    for (cur = 0; cur < NUM_RX_DESC; cur++)
    {
        APTR buf;

        if (np->RxDescArray[cur].addr != 0)
            continue;

        if ((buf = HIDD_PCIDriver_AllocPCIMem(unit->rtl816xu_PCIDriver, np->rx_buf_sz)) == NULL)
            break;

        np->RxDescArray[cur].addr = AROS_QUAD2LE((UQUAD)(IPTR)buf);
    }
    return cur;
}

static void rtl816x_FreeRings(struct net_device *unit)
{
    struct rtl816x_priv *np = get_pcnpriv(unit);
    int i;

    if (np->TxDescArray != NULL)
    {
        for (i = 0; i < NUM_TX_DESC; i++)
        {
            if (np->TxDescArray[i].addr != 0)
            {
                HIDD_PCIDriver_FreePCIMem(unit->rtl816xu_PCIDriver,
                    (APTR)(IPTR)AROS_LE2QUAD(np->TxDescArray[i].addr));
                np->TxDescArray[i].addr = 0;
            }
        }
        HIDD_PCIDriver_FreePCIMem(unit->rtl816xu_PCIDriver, np->TxDescArray);
        np->TxDescArray = NULL;
        np->TxPhyAddr = NULL;
    }

    if (np->RxDescArray != NULL)
    {
        for (i = 0; i < NUM_RX_DESC; i++)
        {
            if (np->RxDescArray[i].addr != 0)
            {
                HIDD_PCIDriver_FreePCIMem(unit->rtl816xu_PCIDriver,
                    (APTR)(IPTR)AROS_LE2QUAD(np->RxDescArray[i].addr));
                np->RxDescArray[i].addr = 0;
            }
        }
        HIDD_PCIDriver_FreePCIMem(unit->rtl816xu_PCIDriver, np->RxDescArray);
        np->RxDescArray = NULL;
        np->RxPhyAddr = NULL;
    }
}

static int rtl816x_InitRings(struct net_device *unit)
{
    struct rtl816x_priv *np = get_pcnpriv(unit);

    RTLD(bug("[%s] rtl816x_InitRings(unit @ %p)\n", unit->rtl816xu_name, unit))

    np->cur_rx = 0;
    np->cur_tx = 0;
    np->dirty_tx = 0;

    memset(np->TxDescArray, 0, R816X_TX_RING_BYTES);
    memset(np->RxDescArray, 0, R816X_RX_RING_BYTES);

    if (rtl816x_RxFill(unit) != NUM_RX_DESC)
        return -1;

    if (rtl816x_TxFill(unit) != NUM_TX_DESC)
        return -1;

    rtl816x_TxDescInit(np);
    rtl816x_RxDescInit(np);

    return 0;
}

static void rtl816x_HWStart(struct net_device *unit)
{
    APTR base = get_hwbase(unit);
    int i;

    /* Soft reset the chip. */
    RTL_W8(base + ChipCmd, CmdReset);

    for (i = 0; i < 100; i++)
    {
        if ((RTL_R8(base + ChipCmd) & CmdReset) == 0)
            break;
        udelay(100);
    }

    switch (unit->rtl816xu_config)
    {
    case RTL_CFG_0:
        rtl_hw_start_8169(unit);
        break;
    case RTL_CFG_2:
        rtl_hw_start_8101(unit);
        break;
    default:
        rtl_hw_start_8168(unit);
        break;
    }

    netif_start_queue(unit);
}

static int rtl816x_Open(struct net_device *unit)
{
    struct rtl816x_priv *np = get_pcnpriv(unit);

    RTLD(bug("[%s] rtl816x_Open(unit @ %p)\n", unit->rtl816xu_name, unit))

    np->rx_buf_sz = RX_BUF_SIZE;

    np->TxDescArray = HIDD_PCIDriver_AllocPCIMem(unit->rtl816xu_PCIDriver, R816X_TX_RING_BYTES);
    np->TxPhyAddr = np->TxDescArray;

    np->RxDescArray = HIDD_PCIDriver_AllocPCIMem(unit->rtl816xu_PCIDriver, R816X_RX_RING_BYTES);
    np->RxPhyAddr = np->RxDescArray;

    if ((np->TxDescArray == NULL) || (np->RxDescArray == NULL))
    {
        RTLD(bug("[%s] rtl816x_Open: Failed to allocate descriptor rings!\n", unit->rtl816xu_name))
        rtl816x_FreeRings(unit);
        return -1;
    }

    if (rtl816x_InitRings(unit) != 0)
    {
        RTLD(bug("[%s] rtl816x_Open: Failed to initialise descriptor rings!\n", unit->rtl816xu_name))
        rtl816x_FreeRings(unit);
        return -1;
    }

    if (request_irq(unit))
    {
        RTLD(bug("[%s] rtl816x_Open: Failed to install interrupt handler!\n", unit->rtl816xu_name))
        rtl816x_FreeRings(unit);
        return -1;
    }

    rtl816x_HWStart(unit);

    unit->rtl816xu_flags |= IFF_UP;

    rtl816x_CheckLinkStatus(unit);

    return 0;
}

static int rtl816x_Close(struct net_device *unit)
{
    struct rtl816x_priv *np = get_pcnpriv(unit);

    RTLD(bug("[%s] rtl816x_Close()\n", unit->rtl816xu_name))

    unit->rtl816xu_flags &= ~IFF_UP;

    netif_stop_queue(unit);

    ObtainSemaphore(&np->lock);
    rtl816x_DeInit(unit);
    ReleaseSemaphore(&np->lock);

    free_irq(unit);

    FlushUnit(LIBBASE, unit, EVENT_QUEUE, S2ERR_OUTOFSERVICE);

    rtl816x_FreeRings(unit);

    ReportEvents(LIBBASE, unit, S2EVENT_OFFLINE);

    return 0;
}

void rtl816x_get_functions(struct net_device *Unit)
{
    Unit->initialize = rtl816x_Init;
    Unit->deinitialize = rtl816x_DeInit;
    Unit->start = rtl816x_Open;
    Unit->stop = rtl816x_Close;
    Unit->set_mac_address = rtl816x_SetMACAddr;
    Unit->set_multicast = rtl816x_SetMulticast;
}
