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

#include "rtl8169.h"

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

#include "rtl8169_hw.h"
#include "rtl8168_hw.h"
#include "rtl8101_hw.h"

#undef LIBBASE
#define LIBBASE (unit->rtl8169u_device)

#define _R(NAME, MAC, MASK) \
    { .name = NAME, .mcfg = MAC, .RxConfigMask = MASK }

static const struct
{
    const char *name;
    UBYTE      mcfg;
    ULONG      RxConfigMask;    /* Clears the bits supported by this chip */
} rtl_chip_info[] =
{
        _R("RTL8169",
           RTL_GIGA_MAC_VER_01,
           0xff7e1880),

        _R("RTL8169s",
           RTL_GIGA_MAC_VER_02,
           0xff7e1880),

        _R("RTL8110s",
       RTL_GIGA_MAC_VER_03,
           0xff7e1880),

        _R("RTL8169sb/8110sb",
       RTL_GIGA_MAC_VER_04,
           0xff7e1880),

        _R("RTL8169sc/8110sc",
       RTL_GIGA_MAC_VER_05,
           0xff7e1880),

        _R("RTL8169sc/8110sc",
       RTL_GIGA_MAC_VER_06,
           0xff7e1880),

        _R("RTL8168b/8111b",
       RTL_GIGA_MAC_VER_11,
           0xff7e1880),

        _R("RTL8168b/8111b",
       RTL_GIGA_MAC_VER_12,
           0xff7e1880),

        _R("RTL8101e",
       RTL_GIGA_MAC_VER_13,
           0xff7e1880),

        _R("RTL8100e",
       RTL_GIGA_MAC_VER_14,
           0xff7e1880),

        _R("RTL8100e",
       RTL_GIGA_MAC_VER_15,
           0xff7e1880),

        _R("RTL8168b/8111b",
       RTL_GIGA_MAC_VER_17,
           0xff7e1880),

        _R("RTL8101e",
       RTL_GIGA_MAC_VER_16,
           0xff7e1880),

        _R("RTL8168cp/8111cp",
       RTL_GIGA_MAC_VER_18,
           0xff7e1880),

        _R("RTL8168c/8111c",
       RTL_GIGA_MAC_VER_19,
           0xff7e1880),

        _R("RTL8168c/8111c",
       RTL_GIGA_MAC_VER_20,
           0xff7e1880),

/*
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
           CFG_METHOD_4,
           RxCfg_128_int_en | RxCfg_fet_multi_en | (RX_DMA_BURST << RxCfgDMAShift),
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
           Jumbo_Frame_9k),*/
};
#undef _R

static const unsigned int rtl8169_rx_config =
        (RX_FIFO_THRESH << RxCfgFIFOShift) | (RX_DMA_BURST << RxCfgDMAShift);

void rtl8169_USecDelay(struct net_device *unit, ULONG usec)
{
    if (unit != NULL)
    {
        unit->rtl8169u_DelayPort.mp_SigTask = FindTask(NULL);
        unit->rtl8169u_DelayReq.tr_node.io_Command = TR_ADDREQUEST;
        unit->rtl8169u_DelayReq.tr_time.tv_micro = usec % 1000000;
        unit->rtl8169u_DelayReq.tr_time.tv_secs = usec / 1000000;

        DoIO((struct IORequest *) &unit->rtl8169u_DelayReq);
    }
}

struct rtl8169_priv *get_pcnpriv(struct net_device *unit)
{
    return unit->rtl8169u_priv;
}

UBYTE *get_hwbase(struct net_device *unit)
{
    return (UBYTE *) unit->rtl8169u_BaseMem;
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
            /* Check if the RTL8169 has completed writing to the specified MDIO register */
            if (!(RTL_R32(base + (PHYAR)) & PHYAR_Flag))
            {
                break;
                }
            udelay(25);
    }
}

ULONG mdio_read(struct net_device *unit, int RegAddr)
{
    APTR                base = get_hwbase(unit);
    UWORD value = 0xffff;
    int i;

    RTL_W32(base + (PHYAR), PHYAR_Read | (RegAddr & PHYAR_Reg_Mask) << PHYAR_Reg_shift);

    for (i = 20; i > 0; i--)
    {
            /* Check if the RTL8169 has completed retrieving data from the specified MDIO register */
            if (RTL_R32(base + (PHYAR)) & PHYAR_Flag)
            {
                value = (UWORD)(RTL_R32(base + (PHYAR)) & PHYAR_Data_Mask);
                break;
            }
            udelay(25);
    }
    return value;
}

void rtl_phy_write(struct net_device *unit, struct phy_reg *regs, int len)
{
        while (len-- > 0)
        {
                mdio_write(unit, regs->reg, regs->val);
                regs++;
        }
}

#if 0
static void rtl8169_PHYPowerUP(struct net_device *unit)
{
        RTLD(bug("[%s] rtl8169_PHYPowerUP()\n", unit->rtl8169u_name))

    mdio_write(unit, 0x1F, 0x0000);
    mdio_write(unit, 0x0E, 0x0000);
}
#endif

void rtl8169_write_gmii_reg_bit(struct net_device *unit, int reg,
                                                       int bitnum, int bitval)
{
        int val;

        val = mdio_read(unit, reg);
        val = (bitval == 1) ? val | (bitval << bitnum) :  val & ~(0x0001 << bitnum);
        mdio_write(unit, reg, val & 0xffff);
}

static void rtl8169_GetMACVersion(struct net_device *unit)
{
    struct rtl8169_priv *np = get_pcnpriv(unit);
    APTR base = get_hwbase(unit);

        /*
         * The driver currently handles the 8168Bf and the 8168Be identically
         * but they can be identified more specifically through the test below
         * if needed:
         *
         * (RTL_R32(TxConfig) & 0x700000) == 0x500000 ? 8168Bf : 8168Be
         *
         * Same thing for the 8101Eb and the 8101Ec:
         *
         * (RTL_R32(TxConfig) & 0x700000) == 0x200000 ? 8101Eb : 8101Ec
         */
        const struct
        {
                ULONG mask;
                ULONG val;
                int mac_version;
        } mac_info[] =
        {
                /* 8168B family. */
                { 0x7c800000, 0x3c800000,       RTL_GIGA_MAC_VER_18 },
                { 0x7cf00000, 0x3c000000,       RTL_GIGA_MAC_VER_19 },
                { 0x7cf00000, 0x3c200000,       RTL_GIGA_MAC_VER_20 },
                { 0x7c800000, 0x3c000000,       RTL_GIGA_MAC_VER_20 },

                /* 8168B family. */
                { 0x7cf00000, 0x38000000,       RTL_GIGA_MAC_VER_12 },
                { 0x7cf00000, 0x38500000,       RTL_GIGA_MAC_VER_17 },
                { 0x7c800000, 0x38000000,       RTL_GIGA_MAC_VER_17 },
                { 0x7c800000, 0x30000000,       RTL_GIGA_MAC_VER_11 },

                /* 8101 family. */
                { 0x7cf00000, 0x34000000,       RTL_GIGA_MAC_VER_13 },
                { 0x7cf00000, 0x34200000,       RTL_GIGA_MAC_VER_16 },
                { 0x7c800000, 0x34000000,       RTL_GIGA_MAC_VER_16 },
                /* FIXME: where did these entries come from ? -- FR */
                { 0xfc800000, 0x38800000,       RTL_GIGA_MAC_VER_15 },
                { 0xfc800000, 0x30800000,       RTL_GIGA_MAC_VER_14 },

                /* 8110 family. */
                { 0xfc800000, 0x98000000,       RTL_GIGA_MAC_VER_06 },
                { 0xfc800000, 0x18000000,       RTL_GIGA_MAC_VER_05 },
                { 0xfc800000, 0x10000000,       RTL_GIGA_MAC_VER_04 },
                { 0xfc800000, 0x04000000,       RTL_GIGA_MAC_VER_03 },
                { 0xfc800000, 0x00800000,       RTL_GIGA_MAC_VER_02 },
                { 0xfc800000, 0x00000000,       RTL_GIGA_MAC_VER_01 },

                { 0x00000000, 0x00000000,       RTL_GIGA_MAC_VER_01 }   /* Catch-all */
        }, *p = mac_info;
        ULONG reg;

        reg = RTL_R32(base + (TxConfig));
        while ((reg & p->mask) != p->val)
        {
                p++;
    }
        np->mcfg = p->mac_version;

        if (p->mask == 0x00000000)
        {
        RTLD(bug("[%s] unknown MAC (%08x)\n", unit->rtl8169u_name, reg))
        }
}

static void rtl8169_PrintMACVersion(struct net_device *unit)
{
    struct rtl8169_priv *np = get_pcnpriv(unit);
    int i;

    RTLD(bug("[%s] rtl8169_PrintMACVersion()\n", unit->rtl8169u_name))
    
    for (i = (sizeof(rtl_chip_info) / sizeof(rtl_chip_info[0])) - 1; i >= 0; i--)
    {
        if (np->mcfg == rtl_chip_info[i].mcfg)
        {
            RTLD(bug("[%s] rtl8169_PrintMACVersion: mcfg == %s (%04d)\n", unit->rtl8169u_name, 
                                                                             rtl_chip_info[i].name,
                                                                             rtl_chip_info[i].mcfg))            
            return;
        }
    }

    RTLD(bug("[%s] rtl8169_PrintMACVersion: mac_version == Unknown\n", unit->rtl8169u_name))
}

static void rtl_hw_phy_config(struct net_device *unit)
{
    struct rtl8169_priv *np = get_pcnpriv(unit);

        rtl8169_PrintMACVersion(unit);

        switch (np->mcfg)
        {
            case RTL_GIGA_MAC_VER_01:
                    break;
            case RTL_GIGA_MAC_VER_02:
            case RTL_GIGA_MAC_VER_03:
                    rtl8169s_hw_phy_config(unit);
                    break;
            case RTL_GIGA_MAC_VER_04:
                    rtl8169sb_hw_phy_config(unit);
                    break;
            case RTL_GIGA_MAC_VER_18:
                    rtl8168cp_hw_phy_config(unit);
                    break;
            case RTL_GIGA_MAC_VER_19:
                    rtl8168c_hw_phy_config(unit);
                    break;
            case RTL_GIGA_MAC_VER_20:
                    rtl8168cx_hw_phy_config(unit);
                    break;
            default:
                    break;
        }
}

/*static void rtl8169_DSM(struct net_device *unit, int dev_state)
{
    struct rtl8169_priv *np = get_pcnpriv(unit);
    APTR base = get_hwbase(unit);

    switch (dev_state)
    {
        case DSM_MAC_INIT:
            if ((np->mcfg == CFG_METHOD_5) || (np->mcfg == CFG_METHOD_6))
            {
                if (RTL_R8(base + (MACDBG)) & 0x80)
                {
                        RTL_W8(base + (GPIO), RTL_R8(base + (GPIO)) | GPIO_en);
                }
                else
                {
                        RTL_W8(base + (GPIO), RTL_R8(base + (GPIO)) & ~GPIO_en);
                }
            }
                break;
    
        case DSM_NIC_GOTO_D3:
        case DSM_IF_DOWN:
                if ((np->mcfg == CFG_METHOD_5) || (np->mcfg == CFG_METHOD_6))
                {
                    if (RTL_R8(base + (MACDBG)) & 0x80)
                    {
                            RTL_W8(base + (GPIO), RTL_R8(base + (GPIO)) & ~GPIO_en);
                                }
                        }
                break;
    
        case DSM_NIC_RESUME_D3:
        case DSM_IF_UP:
                if ((np->mcfg == CFG_METHOD_5) || (np->mcfg == CFG_METHOD_6))
                        {
                    if (RTL_R8(base + (MACDBG)) & 0x80)
                    {
                            RTL_W8(base + (GPIO), RTL_R8(base + (GPIO)) | GPIO_en);
                                }
                        }
                break;
    }
}
*/

static int rtl8169_set_speed_tbi(struct net_device *unit,
                                 UBYTE autoneg, UWORD speed, UBYTE duplex)
{
    // struct rtl8169_priv *np = get_pcnpriv(unit);
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
        RTLD(bug("[%s] incorrect speed setting refused in TBI mode\n", unit->rtl8169u_name))
        }

        return 0;
}

static int rtl8169_set_speed_xmii(struct net_device *unit,
                                  UBYTE autoneg, UWORD speed, UBYTE duplex)
{
    struct rtl8169_priv *np = get_pcnpriv(unit);
    // APTR base = get_hwbase(unit);
    int auto_nego = 0;
    int giga_ctrl = 0;
    // int bmcr_true_force = 0;
    // unsigned long flags;

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
                {
                        auto_nego |= ADVERTISE_10HALF | ADVERTISE_10FULL;
        }
                else if (speed == SPEED_100)
                {
                        auto_nego |= ADVERTISE_100HALF | ADVERTISE_100FULL;
        }
                else if (speed == SPEED_1000)
                {
                        giga_ctrl |= ADVERTISE_1000FULL | ADVERTISE_1000HALF;
        }
                if (duplex == DUPLEX_HALF)
                {
                        auto_nego &= ~(ADVERTISE_10FULL | ADVERTISE_100FULL);
        }
                if (duplex == DUPLEX_FULL)
                {
                        auto_nego &= ~(ADVERTISE_10HALF | ADVERTISE_100HALF);
        }

                /* This tweak comes straight from Realtek's driver. */
                if ((speed == SPEED_100) && (duplex == DUPLEX_HALF) &&
                   ((np->mcfg == RTL_GIGA_MAC_VER_13) ||
                (np->mcfg == RTL_GIGA_MAC_VER_16)))
                {
                        auto_nego = ADVERTISE_100HALF | ADVERTISE_CSMA;
                }
        }

        /* The 8100e/8101e do Fast Ethernet only. */
        if ((np->mcfg == RTL_GIGA_MAC_VER_13) ||
            (np->mcfg == RTL_GIGA_MAC_VER_14) ||
            (np->mcfg == RTL_GIGA_MAC_VER_15) ||
            (np->mcfg == RTL_GIGA_MAC_VER_16))
        {
                if ((giga_ctrl & (ADVERTISE_1000FULL | ADVERTISE_1000HALF)))
                {
            RTLD(bug("[%s] PHY does not support 1000Mbps.\n", unit->rtl8169u_name))
                }
                giga_ctrl &= ~(ADVERTISE_1000FULL | ADVERTISE_1000HALF);
        }

        auto_nego |= ADVERTISE_PAUSE_CAP | ADVERTISE_PAUSE_ASYM;

        if ((np->mcfg == RTL_GIGA_MAC_VER_12) ||
            (np->mcfg == RTL_GIGA_MAC_VER_17))
        {
                /* Vendor specific (0x1f) and reserved (0x0e) MII registers. */
                mdio_write(unit, 0x1f, 0x0000);
                mdio_write(unit, 0x0e, 0x0000);
        }

        np->phy_auto_nego_reg = auto_nego;
        np->phy_1000_ctrl_reg = giga_ctrl;

        mdio_write(unit, MII_ADVERTISE, auto_nego);
        mdio_write(unit, MII_CTRL1000, giga_ctrl);
        mdio_write(unit, MII_BMCR, BMCR_ANENABLE | BMCR_ANRESTART);
        return 0;
}

#if 0
static void rtl8169_start_rx(struct net_device *unit)
{
    RTLD(bug("[%s] rtl8169_start_rx\n", unit->rtl8169u_name))
    // Already running? Stop it.
    /* TODO: Handle starting/stopping Rx */
}

static void rtl8169_stop_rx(struct net_device *unit)
{
    RTLD(bug("[%s] rtl8169_stop_rx\n", unit->rtl8169u_name))
    /* TODO: Handle starting/stopping Rx */
}

static void rtl8169_start_tx(struct net_device *unit)
{
    RTLD(bug("[%s] rtl8169_start_tx()\n", unit->rtl8169u_name))
    /* TODO: Handle starting/stopping Tx */
}

static void rtl8169_stop_tx(struct net_device *unit)
{
    RTLD(bug("[%s] rtl8169_stop_tx()\n", unit->rtl8169u_name))
    /* TODO: Handle starting/stopping Tx */
}

static void rtl8169_txrx_reset(struct net_device *unit)
{
    RTLD(bug("[%s] rtl8169_txrx_reset()\n", unit->rtl8169u_name))
}
#endif

/*
 * rtl8169_SetMulticast: unit->set_multicast function
 * Called with unit->xmit_lock held.
 */
static void rtl8169_SetMulticast(struct net_device *unit)
{
    ULONG addr[2];
    ULONG mask[2];

    RTLD(bug("[%s] rtl8169_SetMulticast()\n", unit->rtl8169u_name))

    memset(addr, 0, sizeof(addr));
    memset(mask, 0, sizeof(mask));
}

static void rtl8169_irq_mask_and_ack(struct net_device *unit)
{
    APTR base = get_hwbase(unit);
        RTL_W16(base + IntrMask, 0x0000);
        RTL_W16(base + IntrStatus, 0xffff);
}

static void rtl8169_asic_down(struct net_device *unit)
{
    APTR base = get_hwbase(unit);
        RTL_W8(base + ChipCmd, 0x00);
        rtl8169_irq_mask_and_ack(unit);
        RTL_R16(base + CPlusCmd);
}

static void rtl8169_DeInit(struct net_device *unit)
{
    APTR base = get_hwbase(unit);
        rtl8169_asic_down(unit);

        /* Update the error counts. */
        RTL_W32(base + RxMissed, 0);
}

static void rtl8169_GetMACAddr(struct net_device *unit, char *addr, BOOL fromROM)
{
    APTR base = get_hwbase(unit);
    int i;

    RTLD(bug("[%s] rtl8169_GetMACAddr()\n",unit->rtl8169u_name))
    /* Get MAC address.  FIXME: read EEPROM */
    for (i = 0; i < MAC_ADDR_LEN; i++)
    {
        addr[i] = RTL_R8(base + MAC0 + i);
    }
}

static void rtl8169_SetMACAddr(struct net_device *unit)
{
    APTR base = get_hwbase(unit);

    RTLD(bug("[%s] rtl8169_SetMACAddr()\n", unit->rtl8169u_name))

    RTL_W8(base + (Cfg9346), Cfg9346_Unlock);

    RTL_W32(base + (MAC0),
            unit->rtl8169u_dev_addr[0] |
            (unit->rtl8169u_dev_addr[1] << 8) |
            (unit->rtl8169u_dev_addr[2] << 16) |
            (unit->rtl8169u_dev_addr[3] << 24));
    RTL_W32(base + (MAC4),
            unit->rtl8169u_dev_addr[4] |
            (unit->rtl8169u_dev_addr[5] << 8));

    RTL_W8(base + (Cfg9346), Cfg9346_Lock);

    RTLD(
            /* Read it back to be certain! */
            TEXT        newmac[6];
            rtl8169_GetMACAddr(unit, newmac, FALSE);

            bug("[%s] rtl8169_SetMACAddr: New MAC Address %02x:%02x:%02x:%02x:%02x:%02x\n", unit->rtl8169u_name,
                        newmac[0], newmac[1], newmac[2],
                        newmac[3], newmac[4], newmac[5])
    )
}

static void rtl8169_LinkOption(struct net_device *unit, UBYTE *aut, UWORD *spd, UBYTE *dup)
{
    unsigned char opt_speed;
    unsigned char opt_duplex;
    unsigned char opt_autoneg;

    opt_speed =   ((unit->rtl8169u_UnitNum < MAX_UNITS) && (unit->rtl8169u_UnitNum >= 0)) ? unit->rtl8169u_device->speed[unit->rtl8169u_UnitNum] : 0xff;
    opt_duplex =  ((unit->rtl8169u_UnitNum < MAX_UNITS) && (unit->rtl8169u_UnitNum >= 0)) ? unit->rtl8169u_device->duplex[unit->rtl8169u_UnitNum] : 0xff;
    opt_autoneg = ((unit->rtl8169u_UnitNum < MAX_UNITS) && (unit->rtl8169u_UnitNum >= 0)) ? unit->rtl8169u_device->autoneg[unit->rtl8169u_UnitNum] : 0xff;

    if ((opt_speed == 0xff) |
      (opt_duplex == 0xff) |
      (opt_autoneg == 0xff))
    {
            *spd = SPEED_1000;
            *dup = DUPLEX_FULL;
            *aut = AUTONEG_ENABLE;
    }
    else
    {
            *spd = unit->rtl8169u_device->speed[unit->rtl8169u_UnitNum];
            *dup = unit->rtl8169u_device->duplex[unit->rtl8169u_UnitNum];
            *aut = unit->rtl8169u_device->autoneg[unit->rtl8169u_UnitNum];      
    }
}

static void rtl8169_phy_reset(struct net_device *unit)
{
    struct rtl8169_priv *np = get_pcnpriv(unit);
    APTR base = get_hwbase(unit);
        unsigned int i;

        if ((np->mcfg <= RTL_GIGA_MAC_VER_06) &&
            (RTL_R8(base + PHYstatus) & TBI_Enable))
        {
        rtl8169_tbi_reset_enable(unit);
    }
    else
    {
        rtl8169_xmii_reset_enable(unit);
    }

        if ((np->mcfg <= RTL_GIGA_MAC_VER_06) &&
            (RTL_R8(base + PHYstatus) & TBI_Enable))
        {
        for (i = 0; i < 100; i++)
        {
                if (!rtl8169_tbi_reset_pending(unit))
                {
                        return;
            }
                rtl8169_USecDelay(unit, 100);
        }
    }
    else
    {
        for (i = 0; i < 100; i++)
        {
                if (!rtl8169_xmii_reset_pending(unit))
                {
                        return;
            }
                rtl8169_USecDelay(unit, 100);
        }
    }
}

static void rtl8169_Init(struct net_device *unit)
{
    struct pHidd_PCIDevice_WriteConfigByte pcibyte;

    struct rtl8169_priv *np = get_pcnpriv(unit);
    APTR base = get_hwbase(unit);
    UBYTE autoneg, duplex;
    UWORD speed;
    int i;
    // int config1;

    RTLD(bug("[%s] rtl8169_Init(unit @ %p)\n", unit->rtl8169u_name, unit))

        np->intr_mask = 0xffff;
        np->intr_event = unit->rtl8169u_intr_event;
        np->napi_event = unit->rtl8169u_napi_event;

        /* Unneeded ? Don't mess with Mrs. Murphy. */
        rtl8169_irq_mask_and_ack(unit);

        /* Soft reset the chip. */
        RTL_W8(base + ChipCmd, CmdReset);

        /* Check that the chip has finished the reset. */
        for (i = 0; i < 100; i++)
    {
                if ((RTL_R8(base + ChipCmd) & CmdReset) == 0)
                {
                        break;
        }
                rtl8169_USecDelay(unit, 100);
        }

    /* Identify chip attached to board */
    rtl8169_GetMACVersion(unit);
    rtl8169_PrintMACVersion(unit);

    for (i = 0; i < ARRAY_SIZE(rtl_chip_info); i++)
    {
                if (np->mcfg == rtl_chip_info[i].mcfg)
                {
                        break;
        }
        }

        if (i == ARRAY_SIZE(rtl_chip_info))
        {
                i = 0;
        }
    np->chipset = i;

    unit->rtl8169u_rtl_chipname = rtl_chip_info[np->chipset].name;

    RTL_W8(base + (Cfg9346), Cfg9346_Unlock);
    RTL_W8(base + (Config1), RTL_R8(base + Config1) | PMEnable);
    RTL_W8(base + (Config5), RTL_R8(base + Config5) & PMEStatus);

    RTLD(bug("[%s] rtl8169_Init: Power Management enabled\n", unit->rtl8169u_name))

/*    RTL_W8(base + (Cfg9346), Cfg9346_Unlock);
    np->features |= rtl8169_try_msi(pdev, base);
    RTL_W8(base + (Cfg9346), Cfg9346_Lock); 
    */

    RTL_W8(base + (Cfg9346), Cfg9346_Lock);

    rtl8169_GetMACAddr(unit, &np->orig_mac[0], TRUE);

    unit->rtl8169u_dev_addr[0] = unit->rtl8169u_org_addr[0] = np->orig_mac[0];
    unit->rtl8169u_dev_addr[1] = unit->rtl8169u_org_addr[1] = np->orig_mac[1];
    unit->rtl8169u_dev_addr[2] = unit->rtl8169u_org_addr[2] = np->orig_mac[2];
    unit->rtl8169u_dev_addr[3] = unit->rtl8169u_org_addr[3] = np->orig_mac[3];
    unit->rtl8169u_dev_addr[4] = unit->rtl8169u_org_addr[4] = np->orig_mac[4];
    unit->rtl8169u_dev_addr[5] = unit->rtl8169u_org_addr[5] = np->orig_mac[5];

    RTLD(bug("[%s] rtl8169_Init: MAC Address %02x:%02x:%02x:%02x:%02x:%02x\n", unit->rtl8169u_name,
                     unit->rtl8169u_dev_addr[0], unit->rtl8169u_dev_addr[1], unit->rtl8169u_dev_addr[2],
                     unit->rtl8169u_dev_addr[3], unit->rtl8169u_dev_addr[4], unit->rtl8169u_dev_addr[5]))

    bug("[%s] rtl8169_Init: This product is covered by one or more of the following patents:\n", unit->rtl8169u_name);
    bug("[%s] rtl8169_Init: US5,307,459, US5,434,872, US5,732,094, US6,570,884, US6,115,776, and US6,327,625.\n", unit->rtl8169u_name);

/*
    unit->features |= NETIF_F_IP_CSUM;
*/
    np->cp_cmd |= RxChkSum;
    np->cp_cmd |= RTL_R16(base + (CPlusCmd));

    //np->intr_mask = unit->rtl8169u_intr_mask;

    // rtl8169_init_phy
    rtl_hw_phy_config(unit);

        if (np->mcfg <= RTL_GIGA_MAC_VER_06)
    {
                RTL_W8(base +  0x82, 0x01);
        }

    RTLD(bug("[%s] rtl8169_Init: PHY Configured\n", unit->rtl8169u_name))

    pcibyte.mID = OOP_GetMethodID(CLID_Hidd_PCIDevice, moHidd_PCIDevice_WriteConfigByte);
    pcibyte.reg = PCI_LATENCY_TIMER;
    pcibyte.val = 0x40;
    OOP_DoMethod(unit->rtl8169u_PCIDevice, (OOP_Msg) &pcibyte);

        if (np->mcfg <= RTL_GIGA_MAC_VER_06)
        {
        pcibyte.mID = OOP_GetMethodID(CLID_Hidd_PCIDevice, moHidd_PCIDevice_WriteConfigByte);
        pcibyte.reg = PCI_CACHE_LINE_SIZE;
        pcibyte.val = 0x08;
        OOP_DoMethod(unit->rtl8169u_PCIDevice, (OOP_Msg) &pcibyte);
    }

        if (np->mcfg == RTL_GIGA_MAC_VER_02)
        {
                RTL_W8(base + 0x82, 0x01);
                mdio_write(unit, 0x0b, 0x0000); //w 0x0b 15 0 0
        }

        rtl8169_phy_reset(unit);

    rtl8169_LinkOption(unit, &autoneg, &speed, &duplex);

        if ((np->mcfg <= RTL_GIGA_MAC_VER_06) &&
            (RTL_R8(base + PHYstatus) & TBI_Enable))
        {
        rtl8169_set_speed_tbi(unit, autoneg, speed, duplex);
        }
        else
        {
        rtl8169_set_speed_xmii(unit, autoneg, speed, duplex);
        }
    
    RTLD(bug("[%s] rtl8169_Init: Link Speed %dbps %s duplex %s\n", unit->rtl8169u_name, speed, (duplex == DUPLEX_HALF) ? "half" :"full", (autoneg == AUTONEG_ENABLE) ? "(autoneg)" :""))
}

static void rtl8169_drain_tx(struct net_device *unit)
{
    // struct rtl8169_priv *np = get_pcnpriv(unit);
    // int i;
//    for (i = 0; i < NUM_TX_DESC; i++) {
    /* TODO: rtl8169_drain_tx does nothing atm. */
//    }
}

static void rtl8169_drain_rx(struct net_device *unit)
{
    // struct rtl8169_priv *np = get_pcnpriv(unit);
    // int i;
//    for (i = 0; i < RX_RING_SIZE; i++) {
    /* TODO: rtl8169_drain_rx does nothing atm. */
//    }
}

static void drain_ring(struct net_device *unit)
{
    rtl8169_drain_tx(unit);
    rtl8169_drain_rx(unit);
}

static int request_irq(struct net_device *unit)
{
    RTLD(bug("[%s] request_irq()\n", unit->rtl8169u_name))

    if (!unit->rtl8169u_IntsAdded)
    {
        if (!HIDD_PCIDevice_AddInterrupt(unit->rtl8169u_PCIDevice, &unit->rtl8169u_irqhandler))
            return 1;
        AddIntServer(INTB_VERTB, &unit->rtl8169u_touthandler);
        unit->rtl8169u_IntsAdded = TRUE;
    }

    return 0;
}

static void free_irq(struct net_device *unit)
{
    if (unit->rtl8169u_IntsAdded)
    {
        HIDD_PCIDevice_RemoveInterrupt(unit->rtl8169u_PCIDevice, &unit->rtl8169u_irqhandler);
        RemIntServer(INTB_VERTB, &unit->rtl8169u_touthandler);
        unit->rtl8169u_IntsAdded = FALSE;
    }
}

void rtl_set_rx_max_size(struct net_device *unit)
{
    struct rtl8169_priv *np = get_pcnpriv(unit);
    APTR base = get_hwbase(unit);
    unsigned int mtu = unit->rtl8169u_mtu;

    RTLD(bug("[%s] rtl_set_rx_max_size()\n", unit->rtl8169u_name))

    np->rx_buf_sz = (mtu > ETH_DATA_LEN) ? mtu + ETH_HLEN + 8 : RX_BUF_SIZE;

    RTLD(bug("[%s] %d\n", unit->rtl8169u_name, np->rx_buf_sz + 1))

    RTL_W16(base + (RxMaxSize), np->rx_buf_sz + 1);
}

/*void rtl_set_tx_max_size(struct net_device *unit)
{
    struct rtl8169_priv *np = get_pcnpriv(unit);
    APTR base = get_hwbase(unit);
    unsigned int mtu = unit->rtl8169u_mtu;

    RTLD(bug("[%s] rtl_set_tx_max_size()\n", unit->rtl8169u_name))

    np->tx_buf_sz = (mtu > ETH_DATA_LEN) ? mtu + ETH_HLEN + 8 : TX_BUF_SIZE;

    RTL_W16(base + (TxMaxSize), np->tx_buf_sz + 1);
}*/


/*static void rtl8169_NICReset(struct net_device *unit)
{
    struct rtl8169_priv *np = get_pcnpriv(unit);
    APTR base = get_hwbase(unit);
    int i;

    RTLD(bug("[%s] rtl8169_NICReset()\n", unit->rtl8169u_name))

    if ((np->mcfg != CFG_METHOD_1) && 
        (np->mcfg != CFG_METHOD_2) &&
        (np->mcfg != CFG_METHOD_3))
    {
            RTL_W8(base + (ChipCmd), StopReq | CmdRxEnb | CmdTxEnb);
            udelay(100);
    }

    // Soft reset the chip.
    RTL_W8(base + (ChipCmd), CmdReset);

    // Check that the chip has finished the reset.
    for (i = 1000; i > 0; i--)
    {
            if ((RTL_R8(base + (ChipCmd)) & CmdReset) == 0)
            {
                break;
            }
            udelay(100);
    }
}*/

UWORD rtl_rw_cpluscmd(struct net_device *unit)
{
    APTR base = get_hwbase(unit);
        UWORD cmd;

        cmd = RTL_R16(base + CPlusCmd);
        RTL_W16(base + CPlusCmd, cmd);
        return cmd;
}

void rtl_set_rx_tx_desc_registers(struct net_device *unit)
{
    struct rtl8169_priv *np = get_pcnpriv(unit);
    APTR base = get_hwbase(unit);
        /*
         * Magic spell: some iop3xx ARM board needs the TxDescAddrHigh
         * register to be written before TxDescAddrLow to work.
         * Switching from MMIO to I/O access fixes the issue as well.
         */
    RTL_W32(base + (TxDescStartAddrHigh), ((UQUAD) (IPTR)np->TxPhyAddr >> 32));
    RTL_W32(base + (TxDescStartAddrLow), ((UQUAD) (IPTR)np->TxPhyAddr & DMA_32BIT_MASK));
    RTL_W32(base + (RxDescAddrHigh), ((UQUAD) (IPTR)np->RxPhyAddr >> 32));
    RTL_W32(base + (RxDescAddrLow), ((UQUAD) (IPTR)np->RxPhyAddr & DMA_32BIT_MASK));
}

void rtl_set_rx_tx_config_registers(struct net_device *unit)
{
    struct rtl8169_priv *np = get_pcnpriv(unit);
    APTR base = get_hwbase(unit);

        ULONG cfg = rtl8169_rx_config;

        cfg |= (RTL_R32(base + RxConfig) & rtl_chip_info[np->chipset].RxConfigMask);
        RTL_W32(base + RxConfig, cfg);

        /* Set DMA burst size and Interframe Gap Time */
        RTL_W32(base + TxConfig, (TX_DMA_BURST << TxDMAShift) |
                                     (InterFrameGap << TxInterFrameGapShift));
}

void rtl_set_rx_mode(struct net_device *unit)
{
    struct rtl8169_priv *np = get_pcnpriv(unit);
    APTR base = get_hwbase(unit);

        // unsigned long flags;
        ULONG mc_filter[2];     /* Multicast hash filter */
        int rx_mode;
        ULONG tmp = 0;

    if (unit->rtl8169u_flags & IFF_PROMISC)
    {
            /* Unconditionally log net taps. */
            RTLD(bug("[%s] rtl8169_SetRxMode: Promiscuous mode enabled\n", unit->rtl8169u_name))
                rx_mode = AcceptBroadcast |
                          AcceptMulticast |
                          AcceptMyPhys |
                          AcceptAllPhys;
                mc_filter[1] = mc_filter[0] = 0xffffffff;
        }
        else if ((unit->rtl8169u_mc_count > unit->rtl8169u_device->rtl8169b_MulticastFilterLimit) ||
                     (unit->rtl8169u_flags))
    {
                /* Too many to filter perfectly -- accept all multicasts. */
                rx_mode = AcceptBroadcast | AcceptMulticast | AcceptMyPhys;
                mc_filter[1] = mc_filter[0] = 0xffffffff;
        }
        else
        {
                struct dev_mc_list *mclist;
                unsigned int i;

                rx_mode = AcceptBroadcast | AcceptMyPhys;
                mc_filter[1] = mc_filter[0] = 0;
                for (i = 0, mclist = unit->rtl8169u_mc_list; mclist && i < unit->rtl8169u_mc_count;
                     i++, mclist = mclist->next)
                {
//                      int bit_nr = ether_crc(ETH_ALEN, mclist->dmi_addr) >> 26;
//                      mc_filter[bit_nr >> 5] |= 1 << (bit_nr & 31);
                        rx_mode |= AcceptMulticast;
                }
        }

        tmp = rtl8169_rx_config | rx_mode |
              (RTL_R32(base + RxConfig) & rtl_chip_info[np->chipset].RxConfigMask);

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

#if 0
static unsigned int rtl8169_XMIILinkOK(struct net_device *unit)
{
    // struct rtl8169_priv *np = get_pcnpriv(unit);
    APTR base = get_hwbase(unit);

    mdio_write(unit, 0x1f, 0x0000);

    return RTL_R8(base + (PHYstatus)) & LinkStatus;
}
#endif

void rtl8169_CheckLinkStatus(struct net_device *unit)
{
    struct rtl8169_priv *np = get_pcnpriv(unit);
    APTR base = get_hwbase(unit);
    // unsigned long flags;
    int result;

        if ((np->mcfg <= RTL_GIGA_MAC_VER_06) &&
            (RTL_R8(base + PHYstatus) & TBI_Enable))
        {
        result = rtl8169_tbi_link_ok(unit);
    }
    else
    {
        result = rtl8169_xmii_link_ok(unit);    
    }

    if (result)
    {
            netif_carrier_on(unit);
            RTLD(bug("[%s] rtl8169_CheckLinkStatus: Link Up\n", unit->rtl8169u_name))
    }
    else
    {
            RTLD(bug("[%s] rtl8169_CheckLinkStatus: Link Down\n", unit->rtl8169u_name))
            netif_carrier_off(unit);
    }
}

static void rtl8169_InitRingIndexes(struct rtl8169_priv *np)
{
    np->dirty_tx = 0;
    np->dirty_rx = 0;
    np->cur_tx = 0;
    np->cur_rx = 0;
}

static void rtl8169_TxDescInit(struct rtl8169_priv *np)
{
    int i = 0;

    memset(np->TxDescArray, 0x0, NUM_TX_DESC * sizeof(struct TxDesc));

    for (i = 0; i < NUM_TX_DESC; i++)
    {
                if(i == (NUM_TX_DESC - 1))
                {
                    np->TxDescArray[i].opts1 = AROS_LONG2LE(RingEnd);
            }
        }
}

static ULONG rtl8169_TxFill(struct net_device *unit, ULONG start, ULONG end)
{
    struct rtl8169_priv *np = get_pcnpriv(unit);
    ULONG cur;

    for (cur = start; end - cur > 0; cur++)
    {
        int i = cur % NUM_TX_DESC;
    
        if (np->TxDescArray[i].addr)
        {
            continue;
        }
    
        if ((np->TxDescArray[i].addr = (IPTR)HIDD_PCIDriver_AllocPCIMem(unit->rtl8169u_PCIDriver, TX_BUF_SIZE)) == 0)
        {
            break;
        }  
    }
    return cur - start;
}

static void rtl8169_RxDescInit(struct rtl8169_priv *np)
{
    int i = 0;

    memset(np->RxDescArray, 0x0, NUM_RX_DESC * sizeof(struct RxDesc));

    for (i = 0; i < NUM_RX_DESC; i++)
    {
        if(i == (NUM_RX_DESC - 1))
        {
            np->RxDescArray[i].opts1 = AROS_LONG2LE((DescOwn | RingEnd | (ULONG) np->rx_buf_sz));
        }
        else
        {
            np->RxDescArray[i].opts1 = AROS_LONG2LE(DescOwn | (ULONG) np->rx_buf_sz);
        }
    }
}

static ULONG rtl8169_RxFill(struct net_device *unit, ULONG start, ULONG end)
{
    struct rtl8169_priv *np = get_pcnpriv(unit);
    ULONG cur;

    for (cur = start; end - cur > 0; cur++)
    {
        int i = cur % NUM_RX_DESC;
    
        if (np->RxDescArray[i].addr)
        {
            continue;
        }
    
        if ((np->RxDescArray[i].addr = (IPTR)HIDD_PCIDriver_AllocPCIMem(unit->rtl8169u_PCIDriver, np->rx_buf_sz)) == 0)
        {
            break;
        }
    }
    return cur - start;
}

static inline void rtl8169_MarkAsLastDescriptor(struct RxDesc *desc)
{
        desc->opts1 |= AROS_LONG2LE(RingEnd);
}

static int rtl8169_InitRings(struct net_device *unit)
{
    struct rtl8169_priv *np = get_pcnpriv(unit);

    RTLD(bug("[%s] rtl8169_InitRings(unit @ %p)\n", unit->rtl8169u_name, unit))

    rtl8169_InitRingIndexes(np);

    rtl8169_TxDescInit(np);
    rtl8169_RxDescInit(np);

    if (rtl8169_RxFill(unit, 0, NUM_RX_DESC) != NUM_RX_DESC)
    {
            goto err_out;
    }

    if (rtl8169_TxFill(unit, 0, NUM_TX_DESC) != NUM_TX_DESC)
    {
            goto err_out;
    }

//    rtl8169_MarkAsLastDescriptor(np->RxDescArray + (NUM_RX_DESC - 1));

    return 0;

err_out:
//    rtl8169_rx_clear(np);
    return -1;
}

static void rtl8169_HWStart(struct net_device *unit)
{
    int i;
    APTR base = get_hwbase(unit);
    
        /* Soft reset the chip. */
    RTL_W8(base + ChipCmd, CmdReset);

        /* Check that the chip has finished the reset. */
        for (i = 0; i < 100; i++)
        {
                if ((RTL_R8(base + ChipCmd) & CmdReset) == 0)
                {
                        break;
        }
            udelay(100);
        }

    switch(unit->rtl8169u_config)
    {
        case RTL_CFG_0:
            rtl_hw_start_8169(unit);
            break;
        case RTL_CFG_1:
                case UNKNOWN_CFG:
            rtl_hw_start_8168(unit);
            break;
        case RTL_CFG_2:
            rtl_hw_start_8101(unit);
            break;
    }

        netif_start_queue(unit);
}

static int rtl8169_Open(struct net_device *unit)
{
    struct rtl8169_priv *np = get_pcnpriv(unit);
    // APTR base = get_hwbase(unit);
    int ret;

    RTLD(bug("[%s] rtl8169_Open(unit @ %p)\n", unit->rtl8169u_name, unit))

    rtl_set_rx_max_size(unit);

    ret = request_irq(unit);
    if (ret)
    {
        goto out_drain;
    }

    np->TxDescArray = HIDD_PCIDriver_AllocPCIMem(unit->rtl8169u_PCIDriver, R8169_TX_RING_BYTES);
    np->TxPhyAddr = np->TxDescArray;

    np->RxDescArray = HIDD_PCIDriver_AllocPCIMem(unit->rtl8169u_PCIDriver, R8169_RX_RING_BYTES);
    np->RxPhyAddr = np->RxDescArray;

    if ((np->TxDescArray) && (np->RxDescArray))
    {
        RTLD(bug("[%s] rtl8169_Open: Allocated Descriptor Arrays - Tx @ %p (%d bytes), Rx @ %p (%d bytes)\n", unit->rtl8169u_name,
                                 np->TxDescArray, R8169_TX_RING_BYTES,
                                 np->RxDescArray, R8169_RX_RING_BYTES))
                if (rtl8169_InitRings(unit) == 0)
                {
                    rtl8169_HWStart(unit);

//                  if (np->esd_flag == 0)
//                      {
//                              rtl8169_request_esd_timer(unit);
//                  }

//                  rtl8169_request_link_timer(unit);

                    //rtl8169_DSM(unit, DSM_IF_UP);

                    rtl8169_CheckLinkStatus(unit);
                }
                else
                {
                        RTLD(bug("[%s] rtl8169_Open: Failed to initialise Descriptor Arrays!\n",unit->rtl8169u_name))
                }
    }
    else
    {
        RTLD(bug("[%s] rtl8169_Open: Failed to Allocate Descriptor Arrays!\n",unit->rtl8169u_name))     
    }

        unit->rtl8169u_flags |= IFF_UP;
    return 0;

out_drain:
    drain_ring(unit);
    return ret;
}

static int rtl8169_Close(struct net_device *unit)
{
    struct rtl8169_priv *np = get_pcnpriv(unit);
    // UBYTE *base;

    RTLD(bug("[%s] rtl8169_Close()\n", unit->rtl8169u_name))
    
    unit->rtl8169u_flags &= ~IFF_UP;

    ObtainSemaphore(&np->lock);
//    np->in_shutdown = 1;
    ReleaseSemaphore(&np->lock);

    unit->rtl8169u_toutNEED = FALSE;

    netif_stop_queue(unit);
    ObtainSemaphore(&np->lock);

    rtl8169_DeInit(unit);

    ReleaseSemaphore(&np->lock);

    free_irq(unit);

    drain_ring(unit);

//    HIDD_PCIDriver_FreePCIMem(unit->rtl8169u_PCIDriver, np->rx_buffer);
//    HIDD_PCIDriver_FreePCIMem(unit->rtl8169u_PCIDriver, np->tx_buffer);

    ReportEvents(LIBBASE, unit, S2EVENT_OFFLINE);

    return 0;
}

void rtl8169_get_functions(struct net_device *Unit)
{
    Unit->initialize = rtl8169_Init;
    Unit->deinitialize = rtl8169_DeInit;
    Unit->start = rtl8169_Open;
    Unit->stop = rtl8169_Close;
    Unit->set_mac_address = rtl8169_SetMACAddr;
    Unit->set_multicast = rtl8169_SetMulticast;
}
