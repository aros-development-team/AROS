#include "rtl8168_hw.h"

void rtl8168cp_hw_phy_config(struct net_device *unit)
{
	struct phy_reg phy_reg_init[] =
	{
		{ 0x1f, 0x0000 },
		{ 0x1d, 0x0f00 },
		{ 0x1f, 0x0002 },
		{ 0x0c, 0x1ec8 },
		{ 0x1f, 0x0000 }
	};

	rtl_phy_write(unit, phy_reg_init, ARRAY_SIZE(phy_reg_init));
}

void rtl8168c_hw_phy_config(struct net_device *unit)
{
	struct phy_reg phy_reg_init[] =
	{
		{ 0x1f, 0x0001 },
		{ 0x12, 0x2300 },
		{ 0x1f, 0x0002 },
		{ 0x00, 0x88d4 },
		{ 0x01, 0x82b1 },
		{ 0x03, 0x7002 },
		{ 0x08, 0x9e30 },
		{ 0x09, 0x01f0 },
		{ 0x0a, 0x5500 },
		{ 0x0c, 0x00c8 },
		{ 0x1f, 0x0003 },
		{ 0x12, 0xc096 },
		{ 0x16, 0x000a },
		{ 0x1f, 0x0000 }
	};

	rtl_phy_write(unit, phy_reg_init, ARRAY_SIZE(phy_reg_init));
}

void rtl8168cx_hw_phy_config(struct net_device *unit)
{
	struct phy_reg phy_reg_init[] =
	{
		{ 0x1f, 0x0000 },
		{ 0x12, 0x2300 },
		{ 0x1f, 0x0003 },
		{ 0x16, 0x0f0a },
		{ 0x1f, 0x0000 },
		{ 0x1f, 0x0002 },
		{ 0x0c, 0x7eb8 },
		{ 0x1f, 0x0000 }
	};

	rtl_phy_write(unit, phy_reg_init, ARRAY_SIZE(phy_reg_init));
}

void rtl_hw_start_8168(struct net_device *unit)
{
    struct rtl8169_priv *np = get_pcnpriv(unit);
    APTR base = get_hwbase(unit);
	UBYTE ctl;

	RTL_W8(base + Cfg9346, Cfg9346_Unlock);

	RTL_W8(base + EarlyTxThres, EarlyTxThld);

	rtl_set_rx_max_size(unit);

	rtl_set_rx_tx_config_registers(unit);

	np->cp_cmd |= RTL_R16(base + CPlusCmd) | PktCntrDisable | INTT_1;

	RTL_W16(base + CPlusCmd, np->cp_cmd);

	/* Tx performance tweak. */
    ctl = HIDD_PCIDevice_ReadConfigByte(unit->rtl8169u_PCIDevice, 0x69);
	ctl = (ctl & ~0x70) | 0x50;
    HIDD_PCIDevice_WriteConfigByte(unit->rtl8169u_PCIDevice, 0x69, ctl);

	RTL_W16(base + IntrMitigate, 0x5151);

	rtl_set_rx_tx_desc_registers(unit);

	RTL_W8(base + Cfg9346, Cfg9346_Lock);

	RTL_R8(base + IntrMask);

	RTL_W32(base + RxMissed, 0);

	rtl_set_rx_mode(unit);

	RTL_W8(base + ChipCmd, CmdTxEnb | CmdRxEnb);

	RTL_W16(base + MultiIntr, RTL_R16(base + MultiIntr) & 0xF000);

    /* Work around for RxFIFO overflow. */
	if (np->mcfg == RTL_GIGA_MAC_VER_11)
	{
		np->intr_event |= RxFIFOOver | PCSTimeout;
		np->intr_event &= ~RxOverflow;
	}
    RTL_W16(base + IntrMask, np->intr_event);
}
