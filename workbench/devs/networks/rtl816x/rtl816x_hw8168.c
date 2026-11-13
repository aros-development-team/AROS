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

#include <aros/macros.h>

#include "rtl816x_hw.h"

static void rtl_patchphy_set(struct net_device *unit, int reg, int bit)
{
    mdio_write(unit, reg, mdio_read(unit, reg) | (1 << bit));
}

static void rtl8168b_1_phy_config(struct net_device *unit)
{
    static const struct phy_reg phy_reg_init[] =
    {
        { 0x1f, 0x0001 },
        { 0x0b, 0x94b0 },
        { 0x1f, 0x0003 },
        { 0x12, 0x6096 },
        { 0x1f, 0x0000 },
        { 0x0d, 0xf8a0 }
    };

    rtl_phy_write(unit, phy_reg_init, ARRAY_SIZE(phy_reg_init));
}

static void rtl8168b_2_phy_config(struct net_device *unit)
{
    static const struct phy_reg phy_reg_init[] =
    {
        { 0x1f, 0x0001 },
        { 0x0b, 0x94b0 },
        { 0x1f, 0x0003 },
        { 0x12, 0x6096 },
        { 0x1f, 0x0000 }
    };

    rtl_phy_write(unit, phy_reg_init, ARRAY_SIZE(phy_reg_init));
}

static void rtl8168c_1_phy_config(struct net_device *unit)
{
    static const struct phy_reg phy_reg_init[] =
    {
        { 0x1f, 0x0001 },
        { 0x12, 0x2300 },
        { 0x1f, 0x0000 },
        { 0x1f, 0x0003 },
        { 0x16, 0x000a },
        { 0x1f, 0x0000 },
        { 0x1f, 0x0003 },
        { 0x12, 0xc096 },
        { 0x1f, 0x0000 },
        { 0x1f, 0x0002 },
        { 0x00, 0x88de },
        { 0x01, 0x82b1 },
        { 0x1f, 0x0000 },
        { 0x1f, 0x0002 },
        { 0x08, 0x9e30 },
        { 0x09, 0x01f0 },
        { 0x1f, 0x0000 },
        { 0x1f, 0x0002 },
        { 0x0a, 0x5500 },
        { 0x1f, 0x0000 },
        { 0x1f, 0x0002 },
        { 0x03, 0x7002 },
        { 0x1f, 0x0000 },
        { 0x1f, 0x0002 },
        { 0x0c, 0x00c8 },
        { 0x1f, 0x0000 }
    };

    rtl_phy_write(unit, phy_reg_init, ARRAY_SIZE(phy_reg_init));

    rtl_patchphy_set(unit, 0x14, 5);
    rtl_patchphy_set(unit, 0x0d, 5);
}

static void rtl8168c_2_phy_config(struct net_device *unit)
{
    static const struct phy_reg phy_reg_init_0[] =
    {
        { 0x1f, 0x0001 },
        { 0x12, 0x2300 },
        { 0x1f, 0x0003 },
        { 0x16, 0x0f0a },
        { 0x1f, 0x0000 },
        { 0x1f, 0x0002 },
        { 0x00, 0x88de },
        { 0x01, 0x82b1 },
        { 0x1f, 0x0000 },
        { 0x1f, 0x0002 },
        { 0x0c, 0x7eb8 },
        { 0x1f, 0x0000 },
        { 0x1f, 0x0002 },
        { 0x06, 0x0761 },
        { 0x1f, 0x0000 },
        { 0x1f, 0x0001 },
        { 0x03, 0x802f },
        { 0x02, 0x4f02 },
        { 0x01, 0x0409 },
        { 0x00, 0xf099 },
        { 0x04, 0x9800 },
        { 0x04, 0x9000 },
        { 0x1f, 0x0000 }
    };
    static const struct phy_reg phy_reg_init_1[] =
    {
        { 0x1f, 0x0001 },
        { 0x1d, 0x3d98 },
        { 0x1f, 0x0000 },
        { 0x1f, 0x0001 },
        { 0x17, 0x0cc0 },
        { 0x1f, 0x0000 }
    };

    rtl_phy_write(unit, phy_reg_init_0, ARRAY_SIZE(phy_reg_init_0));

    mdio_write(unit, 0x1f, 0x0000);
    rtl_patchphy_set(unit, 0x16, 0);
    rtl_patchphy_set(unit, 0x14, 5);
    rtl_patchphy_set(unit, 0x0d, 5);

    rtl_phy_write(unit, phy_reg_init_1, ARRAY_SIZE(phy_reg_init_1));
}

static void rtl8168c_3_phy_config(struct net_device *unit)
{
    static const struct phy_reg phy_reg_init_0[] =
    {
        { 0x1f, 0x0001 },
        { 0x12, 0x2300 },
        { 0x1f, 0x0003 },
        { 0x16, 0x0f0a },
        { 0x1f, 0x0000 },
        { 0x1f, 0x0002 },
        { 0x00, 0x88de },
        { 0x01, 0x82b1 },
        { 0x1f, 0x0000 },
        { 0x1f, 0x0002 },
        { 0x0c, 0x7eb8 },
        { 0x1f, 0x0000 },
        { 0x1f, 0x0002 },
        { 0x06, 0x0761 },
        { 0x1f, 0x0000 }
    };
    static const struct phy_reg phy_reg_init_1[] =
    {
        { 0x1f, 0x0001 },
        { 0x1d, 0x3d98 },
        { 0x1f, 0x0000 },
        { 0x1f, 0x0001 },
        { 0x17, 0x0cc0 },
        { 0x1f, 0x0000 }
    };

    rtl_phy_write(unit, phy_reg_init_0, ARRAY_SIZE(phy_reg_init_0));

    mdio_write(unit, 0x1f, 0x0000);
    rtl_patchphy_set(unit, 0x16, 0);
    rtl_patchphy_set(unit, 0x14, 5);
    rtl_patchphy_set(unit, 0x0d, 5);

    rtl_phy_write(unit, phy_reg_init_1, ARRAY_SIZE(phy_reg_init_1));
}

static void rtl8168cp_1_phy_config(struct net_device *unit)
{
    static const struct phy_reg phy_reg_init[] =
    {
        { 0x1f, 0x0001 },
        { 0x1d, 0x3d98 },
        { 0x1f, 0x0001 },
        { 0x14, 0xcaa3 },
        { 0x1c, 0x000a },
        { 0x18, 0x65d0 },
        { 0x1f, 0x0003 },
        { 0x17, 0xb580 },
        { 0x18, 0xff54 },
        { 0x19, 0x3954 },
        { 0x1f, 0x0002 },
        { 0x0d, 0x310c },
        { 0x0e, 0x310c },
        { 0x0f, 0x311c },
        { 0x06, 0x0761 },
        { 0x1f, 0x0003 },
        { 0x18, 0xff55 },
        { 0x19, 0x3955 },
        { 0x18, 0xff54 },
        { 0x19, 0x3954 },
        { 0x1f, 0x0001 },
        { 0x17, 0x0cc0 },
        { 0x1f, 0x0000 }
    };

    mdio_write(unit, 0x1f, 0x0000);
    rtl_patchphy_set(unit, 0x14, 5);
    rtl_patchphy_set(unit, 0x0d, 5);

    rtl_phy_write(unit, phy_reg_init, ARRAY_SIZE(phy_reg_init));
}

static void rtl8168cp_2_phy_config(struct net_device *unit)
{
    static const struct phy_reg phy_reg_init[] =
    {
        { 0x1f, 0x0001 },
        { 0x14, 0xcaa3 },
        { 0x1c, 0x000a },
        { 0x18, 0x65d0 },
        { 0x1f, 0x0003 },
        { 0x17, 0xb580 },
        { 0x18, 0xff54 },
        { 0x19, 0x3954 },
        { 0x1f, 0x0002 },
        { 0x0d, 0x310c },
        { 0x0e, 0x310c },
        { 0x0f, 0x311c },
        { 0x06, 0x0761 },
        { 0x1f, 0x0003 },
        { 0x18, 0xff55 },
        { 0x19, 0x3955 },
        { 0x18, 0xff54 },
        { 0x19, 0x3954 },
        { 0x1f, 0x0001 },
        { 0x17, 0x0cc0 },
        { 0x1f, 0x0000 }
    };

    mdio_write(unit, 0x1f, 0x0000);
    rtl_patchphy_set(unit, 0x0d, 5);

    rtl_phy_write(unit, phy_reg_init, ARRAY_SIZE(phy_reg_init));
}

static void rtl8168d_common_phy_config(struct net_device *unit)
{
    static const struct phy_reg phy_reg_init[] =
    {
        { 0x1f, 0x0001 },
        { 0x06, 0x4064 },
        { 0x07, 0x2863 },
        { 0x08, 0x059c },
        { 0x09, 0x26b4 },
        { 0x0a, 0x6a19 },
        { 0x0b, 0xacc0 },
        { 0x10, 0xf06d },
        { 0x14, 0x7f68 },
        { 0x18, 0x7fd9 },
        { 0x1c, 0xf0ff },
        { 0x1d, 0x3d9c },
        { 0x1f, 0x0003 },
        { 0x12, 0xf49f },
        { 0x13, 0x070b },
        { 0x1a, 0x05ad },
        { 0x14, 0x94c0 }
    };

    rtl_phy_write(unit, phy_reg_init, ARRAY_SIZE(phy_reg_init));
}

/* Shared PHY microcode stream written through page 5 registers */
static void rtl8168d_phy_stream(struct net_device *unit)
{
    static const struct phy_reg phy_reg_init[] =
    {
        { 0x1f, 0x0005 },
        { 0x05, 0x8200 },
        { 0x06, 0xf8f9 },
        { 0x06, 0xfaef },
        { 0x06, 0x59ee },
        { 0x06, 0xf8ea },
        { 0x06, 0x00ee },
        { 0x06, 0xf8eb },
        { 0x06, 0x00e0 },
        { 0x06, 0xf87c },
        { 0x06, 0xe1f8 },
        { 0x06, 0x7d59 },
        { 0x06, 0x0fef },
        { 0x06, 0x0139 },
        { 0x06, 0x029e },
        { 0x06, 0x06ef },
        { 0x06, 0x1039 },
        { 0x06, 0x089f },
        { 0x06, 0x2aee },
        { 0x06, 0xf8ea },
        { 0x06, 0x00ee },
        { 0x06, 0xf8eb },
        { 0x06, 0x01e0 },
        { 0x06, 0xf87c },
        { 0x06, 0xe1f8 },
        { 0x06, 0x7d58 },
        { 0x06, 0x409e },
        { 0x06, 0x0f39 },
        { 0x06, 0x46aa },
        { 0x06, 0x0bbf },
        { 0x06, 0x8251 },
        { 0x06, 0xd682 },
        { 0x06, 0x5902 },
        { 0x06, 0x014f },
        { 0x06, 0xae09 },
        { 0x06, 0xbf82 },
        { 0x06, 0x59d6 },
        { 0x06, 0x8261 },
        { 0x06, 0x0201 },
        { 0x06, 0x4fef },
        { 0x06, 0x95fe },
        { 0x06, 0xfdfc },
        { 0x06, 0x054d },
        { 0x06, 0x2000 },
        { 0x06, 0x024e },
        { 0x06, 0x2200 },
        { 0x06, 0x024d },
        { 0x06, 0xdfff },
        { 0x06, 0x014e },
        { 0x06, 0xddff },
        { 0x06, 0x0100 }
    };

    rtl_phy_write(unit, phy_reg_init, ARRAY_SIZE(phy_reg_init));
}

static void rtl8168d_1_phy_config(struct net_device *unit)
{
    static const struct phy_reg phy_reg_init[] =
    {
        { 0x1f, 0x0002 },
        { 0x0b, 0x0b10 },
        { 0x0c, 0xa2f7 },
        { 0x1f, 0x0002 },
        { 0x06, 0x5571 },
        { 0x1f, 0x0002 },
        { 0x02, 0xc107 },
        { 0x03, 0x1002 },
        { 0x1f, 0x0001 },
        { 0x17, 0x0cc0 }
    };
    static const struct phy_reg phy_reg_tail[] =
    {
        { 0x06, 0x6010 },
        { 0x05, 0xfff6 },
        { 0x06, 0x00ec },
        { 0x05, 0x83d4 },
        { 0x06, 0x8200 },
        { 0x1f, 0x0000 }
    };

    rtl8168d_common_phy_config(unit);
    rtl_phy_write(unit, phy_reg_init, ARRAY_SIZE(phy_reg_init));
    rtl8168d_phy_stream(unit);
    rtl_phy_write(unit, phy_reg_tail, ARRAY_SIZE(phy_reg_tail));
}

static void rtl8168d_2_phy_config(struct net_device *unit)
{
    static const struct phy_reg phy_reg_init[] =
    {
        { 0x1f, 0x0002 },
        { 0x06, 0x5571 },
        { 0x1f, 0x0002 },
        { 0x05, 0x2642 },
        { 0x1f, 0x0002 },
        { 0x02, 0xc107 },
        { 0x03, 0x1002 },
        { 0x1f, 0x0001 },
        { 0x17, 0x0cc0 },
        { 0x1f, 0x0002 },
        { 0x0f, 0x0017 }
    };
    static const struct phy_reg phy_reg_tail[] =
    {
        { 0x02, 0x6010 },
        { 0x05, 0xfff6 },
        { 0x06, 0x00ec },
        { 0x05, 0x83d4 },
        { 0x06, 0x8200 },
        { 0x1f, 0x0000 }
    };

    rtl8168d_common_phy_config(unit);
    rtl_phy_write(unit, phy_reg_init, ARRAY_SIZE(phy_reg_init));
    rtl8168d_phy_stream(unit);
    rtl_phy_write(unit, phy_reg_tail, ARRAY_SIZE(phy_reg_tail));
}

static void rtl8168d_3_phy_config(struct net_device *unit)
{
    static const struct phy_reg phy_reg_init[] =
    {
        { 0x1f, 0x0002 },
        { 0x06, 0x5571 },
        { 0x1f, 0x0002 },
        { 0x05, 0x2642 },
        { 0x1f, 0x0002 },
        { 0x02, 0xc107 },
        { 0x03, 0x1002 },
        { 0x1f, 0x0001 },
        { 0x17, 0x0cc0 },
        { 0x1f, 0x0002 },
        { 0x0f, 0x0017 },
        { 0x1f, 0x0000 }
    };

    rtl8168d_common_phy_config(unit);
    rtl_phy_write(unit, phy_reg_init, ARRAY_SIZE(phy_reg_init));
}

void rtl8168_hw_phy_config(struct net_device *unit)
{
    struct rtl816x_priv *np = get_pcnpriv(unit);

    switch (np->mcfg)
    {
    case RTL_GIGA_MAC_VER_11:
        rtl8168b_1_phy_config(unit);
        break;
    case RTL_GIGA_MAC_VER_12:
    case RTL_GIGA_MAC_VER_17:
        rtl8168b_2_phy_config(unit);
        break;
    case RTL_GIGA_MAC_VER_18:
        rtl8168cp_1_phy_config(unit);
        break;
    case RTL_GIGA_MAC_VER_19:
        rtl8168c_1_phy_config(unit);
        break;
    case RTL_GIGA_MAC_VER_20:
        rtl8168c_2_phy_config(unit);
        break;
    case RTL_GIGA_MAC_VER_21:
        rtl8168c_3_phy_config(unit);
        break;
    case RTL_GIGA_MAC_VER_24:
        rtl8168cp_2_phy_config(unit);
        break;
    case RTL_GIGA_MAC_VER_25:
        rtl8168d_1_phy_config(unit);
        break;
    case RTL_GIGA_MAC_VER_26:
        rtl8168d_2_phy_config(unit);
        break;
    case RTL_GIGA_MAC_VER_27:
        rtl8168d_3_phy_config(unit);
        break;
    default:
        /* Later generations bring their PHY defaults up from internal
           configuration - standard autonegotiation is sufficient */
        break;
    }
}

static void rtl_disable_clock_request(struct net_device *unit)
{
    HIDD_PCIDevice_WriteConfigByte(unit->rtl816xu_PCIDevice, 0x81, 0x00);
}

static void rtl_csi_access_enable(struct net_device *unit)
{
    ULONG csi;

    csi = rtl_csi_read(unit, 0x70c) & 0x00ffffff;
    rtl_csi_write(unit, 0x70c, csi | 0x27000000);
}

static void rtl_hw_start_8168bb(struct net_device *unit)
{
    APTR base = get_hwbase(unit);

    RTL_W8(base + Config3, RTL_R8(base + Config3) & ~Beacon_en);

    RTL_W16(base + CPlusCmd, RTL_R16(base + CPlusCmd) &
        ~(EnableBist | Macdbgo_oe | Force_halfdup | Force_rxflow_en | Force_txflow_en |
          Cxpl_dbg_sel | ASF | PktCntrDisable | Macdbgo_sel));

    /* PCIe capability sits at 0x60 on 8168B: max read request in cfg 0x69 */
    rtl_pcie_maxread_tweak(unit, 0x69, 0x50);
}

static void rtl_hw_start_8168c(struct net_device *unit)
{
    struct rtl816x_priv *np = get_pcnpriv(unit);
    APTR base = get_hwbase(unit);
    UWORD ephy_data;

    rtl_csi_access_enable(unit);

    switch (np->mcfg)
    {
    case RTL_GIGA_MAC_VER_19:
        RTL_W8(base + DBG_reg, (0x0E << 4) | Fix_Nak_1 | Fix_Nak_2);

        ephy_data = rtl_ephy_read(unit, 0x02);
        ephy_data &= ~(1 << 11);
        ephy_data |= (1 << 12);
        rtl_ephy_write(unit, 0x02, ephy_data);

        ephy_data = rtl_ephy_read(unit, 0x03);
        ephy_data |= (1 << 1);
        rtl_ephy_write(unit, 0x03, ephy_data);

        ephy_data = rtl_ephy_read(unit, 0x06);
        ephy_data &= ~(1 << 7);
        rtl_ephy_write(unit, 0x06, ephy_data);
        break;

    case RTL_GIGA_MAC_VER_20:
        ephy_data = rtl_ephy_read(unit, 0x01);
        ephy_data |= (1 << 0);
        rtl_ephy_write(unit, 0x01, ephy_data);

        ephy_data = rtl_ephy_read(unit, 0x03);
        ephy_data &= ~(1 << 10);
        ephy_data |= (1 << 9) | (1 << 5);
        rtl_ephy_write(unit, 0x03, ephy_data);
        break;

    case RTL_GIGA_MAC_VER_24:
        RTL_W8(base + DBG_reg, 0x20);
        break;
    }

    RTL_W8(base + Config3, RTL_R8(base + Config3) & ~Beacon_en);

    if ((np->mcfg == RTL_GIGA_MAC_VER_19) ||
        (np->mcfg == RTL_GIGA_MAC_VER_20))
        rtl_disable_clock_request(unit);

    RTL_W16(base + CPlusCmd, RTL_R16(base + CPlusCmd) &
        ~(EnableBist | Macdbgo_oe | Force_halfdup | Force_rxflow_en | Force_txflow_en |
          Cxpl_dbg_sel | ASF | PktCntrDisable | Macdbgo_sel));

    /* PCIe capability sits at 0x70 from 8168C on: cfg 0x79 */
    rtl_pcie_maxread_tweak(unit, 0x79, 0x50);
}

static void rtl_hw_start_8168d(struct net_device *unit)
{
    struct rtl816x_priv *np = get_pcnpriv(unit);
    APTR base = get_hwbase(unit);

    rtl_csi_access_enable(unit);

    rtl_disable_clock_request(unit);

    switch (np->mcfg)
    {
    case RTL_GIGA_MAC_VER_25:
        RTL_W8(base + Config1, 0xCF);
        RTL_W8(base + Config2, 0x9C);
        RTL_W8(base + Config3, 0x62);

        rtl_ephy_write(unit, 0x01, 0x7C7D);
        rtl_ephy_write(unit, 0x02, 0x091F);
        rtl_ephy_write(unit, 0x06, 0xB271);
        rtl_ephy_write(unit, 0x07, 0xCE00);
        break;

    case RTL_GIGA_MAC_VER_26:
        RTL_W8(base + Config1, 0xDF);

        rtl_ephy_write(unit, 0x01, 0x7C7D);
        rtl_ephy_write(unit, 0x02, 0x091F);
        rtl_ephy_write(unit, 0x03, 0xC5BA);
        rtl_ephy_write(unit, 0x06, 0xB279);
        rtl_ephy_write(unit, 0x07, 0xAF00);
        rtl_ephy_write(unit, 0x1E, 0xB8EB);
        break;

    case RTL_GIGA_MAC_VER_27:
        RTL_W8(base + Config1, 0xDF);

        rtl_ephy_write(unit, 0x01, 0x6C7F);
        rtl_ephy_write(unit, 0x02, 0x011F);
        rtl_ephy_write(unit, 0x03, 0xC1B2);
        rtl_ephy_write(unit, 0x1A, 0x0546);
        rtl_ephy_write(unit, 0x1C, 0x80C4);
        rtl_ephy_write(unit, 0x1D, 0x78E4);

        RTL_W8(base + 0xF3, RTL_R8(base + 0xF3) | (1 << 2));
        break;
    }

    rtl_pcie_maxread_tweak(unit, 0x79, 0x50);
}

void rtl_hw_start_8168(struct net_device *unit)
{
    struct rtl816x_priv *np = get_pcnpriv(unit);
    APTR base = get_hwbase(unit);

    RTLD(bug("[%s] rtl_hw_start_8168()\n", unit->rtl816xu_name))

    RTL_W8(base + Cfg9346, Cfg9346_Unlock);

    RTL_W8(base + EarlyTxThres, EarlyTxThld);

    rtl_set_rx_max_size(unit);

    np->cp_cmd |= RTL_R16(base + CPlusCmd) | PktCntrDisable | INTT_1;
    RTL_W16(base + CPlusCmd, np->cp_cmd);

    RTL_W16(base + IntrMitigate, 0x5151);

    switch (np->mcfg)
    {
    case RTL_GIGA_MAC_VER_11:
    case RTL_GIGA_MAC_VER_12:
    case RTL_GIGA_MAC_VER_17:
        rtl_hw_start_8168bb(unit);
        break;

    case RTL_GIGA_MAC_VER_18:
    case RTL_GIGA_MAC_VER_19:
    case RTL_GIGA_MAC_VER_20:
    case RTL_GIGA_MAC_VER_21:
    case RTL_GIGA_MAC_VER_24:
        rtl_hw_start_8168c(unit);
        break;

    case RTL_GIGA_MAC_VER_25:
    case RTL_GIGA_MAC_VER_26:
    case RTL_GIGA_MAC_VER_27:
        rtl_hw_start_8168d(unit);
        break;

    default:
        /* 8168DP and 8168E onwards run with the generic setup */
        break;
    }

    rtl_set_rx_tx_desc_registers(unit);

    RTL_W8(base + Cfg9346, Cfg9346_Lock);

    /* PCI commit */
    RTL_R8(base + IntrMask);

    RTL_W8(base + ChipCmd, CmdTxEnb | CmdRxEnb);

    rtl_set_rx_tx_config_registers(unit);

    rtl_set_rx_mode(unit);

    RTL_W16(base + MultiIntr, RTL_R16(base + MultiIntr) & 0xF000);

    /* Work around for RxFIFO overflow on 8168Bb */
    if (np->mcfg == RTL_GIGA_MAC_VER_11)
    {
        np->intr_event |= RxFIFOOver | PCSTimeout;
        np->intr_event &= ~RxOverflow;
    }

    RTL_W16(base + IntrMask, np->intr_event);
}
