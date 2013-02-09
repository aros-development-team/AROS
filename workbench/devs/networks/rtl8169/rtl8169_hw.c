#include "rtl8169_hw.h"

UBYTE *get_hwbase(struct net_device *unit);
struct rtl8169_priv *get_pcnpriv(struct net_device *unit);

void rtl8169_set_magic_reg(struct net_device *unit, unsigned mac_version)
{
    APTR base = get_hwbase(unit);
	struct
	{
		ULONG mac_version;
		ULONG clk;
		ULONG val;
	} cfg2_info [] =
	{
		{ RTL_GIGA_MAC_VER_05, PCI_Clock_33MHz, 0x000fff00 }, // 8110SCd
		{ RTL_GIGA_MAC_VER_05, PCI_Clock_66MHz, 0x000fffff },
		{ RTL_GIGA_MAC_VER_06, PCI_Clock_33MHz, 0x00ffff00 }, // 8110SCe
		{ RTL_GIGA_MAC_VER_06, PCI_Clock_66MHz, 0x00ffffff }
	}, *p = cfg2_info;
	unsigned int i;
	ULONG clk;

	clk = RTL_R8(base + Config2) & PCI_Clock_66MHz;
	for (i = 0; i < ARRAY_SIZE(cfg2_info); i++, p++)
    {
		if ((p->mac_version == mac_version) && (p->clk == clk))
		{
			RTL_W32(base + 0x7c, p->val);
			break;
		}
	}
}

unsigned int rtl8169_tbi_link_ok(struct net_device *unit)
{
    APTR base = get_hwbase(unit);

	return RTL_R32(base + TBICSR) & TBILinkOk;
}

unsigned int rtl8169_xmii_link_ok(struct net_device *unit)
{
    APTR base = get_hwbase(unit);

	return RTL_R8(base + PHYstatus) & LinkStatus;
}

void rtl8169_tbi_reset_enable(struct net_device *unit)
{
    APTR base = get_hwbase(unit);

	RTL_W32(base + TBICSR, RTL_R32(base + TBICSR) | TBIReset);
}

void rtl8169_xmii_reset_enable(struct net_device *unit)
{
	unsigned int val;

	val = mdio_read(unit, MII_BMCR) | BMCR_RESET;
	mdio_write(unit, MII_BMCR, val & 0xffff);
}

unsigned int rtl8169_tbi_reset_pending(struct net_device *unit)
{
    APTR base = get_hwbase(unit);

	return RTL_R32(base + TBICSR) & TBIReset;
}

unsigned int rtl8169_xmii_reset_pending(struct net_device *unit)
{
	return mdio_read(unit, MII_BMCR) & BMCR_RESET;
}

void rtl8169s_hw_phy_config(struct net_device *unit)
{
	struct
	{
		UWORD regs[5]; /* Beware of bit-sign propagation */
	} phy_magic[5] =
	{ {
		{ 0x0000,	//w 4 15 12 0
		  0x00a1,	//w 3 15 0 00a1
		  0x0008,	//w 2 15 0 0008
		  0x1020,	//w 1 15 0 1020
		  0x1000 } },{	//w 0 15 0 1000
		{ 0x7000,	//w 4 15 12 7
		  0xff41,	//w 3 15 0 ff41
		  0xde60,	//w 2 15 0 de60
		  0x0140,	//w 1 15 0 0140
		  0x0077 } },{	//w 0 15 0 0077
		{ 0xa000,	//w 4 15 12 a
		  0xdf01,	//w 3 15 0 df01
		  0xdf20,	//w 2 15 0 df20
		  0xff95,	//w 1 15 0 ff95
		  0xfa00 } },{	//w 0 15 0 fa00
		{ 0xb000,	//w 4 15 12 b
		  0xff41,	//w 3 15 0 ff41
		  0xde20,	//w 2 15 0 de20
		  0x0140,	//w 1 15 0 0140
		  0x00bb } },{	//w 0 15 0 00bb
		{ 0xf000,	//w 4 15 12 f
		  0xdf01,	//w 3 15 0 df01
		  0xdf20,	//w 2 15 0 df20
		  0xff95,	//w 1 15 0 ff95
		  0xbf00 }	//w 0 15 0 bf00
		}
	}, *p = phy_magic;
	unsigned int i;

	mdio_write(unit, 0x1f, 0x0001);		//w 31 2 0 1
	mdio_write(unit, 0x15, 0x1000);		//w 21 15 0 1000
	mdio_write(unit, 0x18, 0x65c7);		//w 24 15 0 65c7
	rtl8169_write_gmii_reg_bit(unit, 4, 11, 0);	//w 4 11 11 0

	for (i = 0; i < ARRAY_SIZE(phy_magic); i++, p++)
	{
		int val, pos = 4;

		val = (mdio_read(unit, pos) & 0x0fff) | (p->regs[0] & 0xffff);
		mdio_write(unit, pos, val);
		while (--pos >= 0)
		{
			mdio_write(unit, pos, p->regs[4 - pos] & 0xffff);
        }
		rtl8169_write_gmii_reg_bit(unit, 4, 11, 1); //w 4 11 11 1
		rtl8169_write_gmii_reg_bit(unit, 4, 11, 0); //w 4 11 11 0
	}
	mdio_write(unit, 0x1f, 0x0000); //w 31 2 0 0
}

void rtl8169sb_hw_phy_config(struct net_device *unit)
{
	struct phy_reg phy_reg_init[] =
	{
		{ 0x1f, 0x0002 },
		{ 0x01, 0x90d0 },
		{ 0x1f, 0x0000 }
	};

	rtl_phy_write(unit, phy_reg_init, ARRAY_SIZE(phy_reg_init));
}

void rtl_hw_start_8169(struct net_device *unit)
{
    struct rtl8169_priv *np = get_pcnpriv(unit);
    APTR base = get_hwbase(unit);

    // UBYTE device_control;
    // UWORD ephy_data;
    // ULONG csi_tmp;

    RTLD(bug("[%s] rtl8169nic_HWStart_8169()\n", unit->rtl8169u_name))
	if (np->mcfg == RTL_GIGA_MAC_VER_05)
	{
		RTL_W16(base + CPlusCmd, RTL_R16(base + CPlusCmd) | PCIMulRW);
		HIDD_PCIDevice_WriteConfigByte(unit->rtl8169u_PCIDevice, PCI_CACHE_LINE_SIZE, 0x08);
	}

    RTL_W8(base + (Cfg9346), Cfg9346_Unlock);

	if ((np->mcfg == RTL_GIGA_MAC_VER_01) ||
	    (np->mcfg == RTL_GIGA_MAC_VER_02) ||
	    (np->mcfg == RTL_GIGA_MAC_VER_03) ||
	    (np->mcfg == RTL_GIGA_MAC_VER_04))
	{
		RTL_W8(base + (ChipCmd), CmdTxEnb | CmdRxEnb);
    }

    RTL_W8(base + (EarlyTxThres), EarlyTxThld);

    rtl_set_rx_max_size(unit);

	if ((np->mcfg == RTL_GIGA_MAC_VER_01) ||
	    (np->mcfg == RTL_GIGA_MAC_VER_02) ||
	    (np->mcfg == RTL_GIGA_MAC_VER_03) ||
	    (np->mcfg == RTL_GIGA_MAC_VER_04))
	{
		rtl_set_rx_tx_config_registers(unit);
    }

    np->cp_cmd |= rtl_rw_cpluscmd(unit) | PCIMulRW;

	if ((np->mcfg == RTL_GIGA_MAC_VER_02) ||
	    (np->mcfg == RTL_GIGA_MAC_VER_03))
    {
		np->cp_cmd |= (1 << 14);
	}

	RTL_W16(base + CPlusCmd, np->cp_cmd);

	rtl8169_set_magic_reg(unit, np->mcfg);

	/*
	 * Undocumented corner. Supposedly:
	 * (TxTimer << 12) | (TxPackets << 8) | (RxTimer << 4) | RxPackets
	 */
    RTL_W16(base + (IntrMitigate), 0);

	rtl_set_rx_tx_desc_registers(unit);

	if ((np->mcfg != RTL_GIGA_MAC_VER_01) &&
	    (np->mcfg != RTL_GIGA_MAC_VER_02) &&
	    (np->mcfg != RTL_GIGA_MAC_VER_03) &&
	    (np->mcfg != RTL_GIGA_MAC_VER_04))
	{
		RTL_W8(base + ChipCmd, CmdTxEnb | CmdRxEnb);
		rtl_set_rx_tx_config_registers(unit);
	}

	RTL_W8(base + Cfg9346, Cfg9346_Lock);

	/* Initially a 10 us delay. Turned it into a PCI commit. - FR */
	RTL_R8(base + IntrMask);

	RTL_W32(base + RxMissed, 0);

	rtl_set_rx_mode(unit);

	/* no early-rx interrupts */
	RTL_W16(base + MultiIntr, RTL_R16(base + MultiIntr) & 0xF000);

	/* Enable all known interrupts by setting the interrupt mask. */
	RTL_W16(base + IntrMask, np->intr_event);
}
