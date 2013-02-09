#include "rtl8101_hw.h"

void rtl_hw_start_8101(struct net_device *unit)
{
    struct rtl8169_priv *np = get_pcnpriv(unit);
    APTR base = get_hwbase(unit);

	if ((np->mcfg == RTL_GIGA_MAC_VER_13) ||
	    (np->mcfg == RTL_GIGA_MAC_VER_16))
	{
	    HIDD_PCIDevice_WriteConfigWord(unit->rtl8169u_PCIDevice, 0x68, 0x00);
	    HIDD_PCIDevice_WriteConfigWord(unit->rtl8169u_PCIDevice, 0x69, 0x09);
	}

	RTL_W8(base + Cfg9346, Cfg9346_Unlock);

	RTL_W8(base + EarlyTxThres, EarlyTxThld);

	rtl_set_rx_max_size(unit);

	np->cp_cmd |= rtl_rw_cpluscmd(unit) | PCIMulRW;

	RTL_W16(base + CPlusCmd, np->cp_cmd);

	RTL_W16(base + IntrMitigate, 0x0000);

	rtl_set_rx_tx_desc_registers(unit);

	RTL_W8(base + ChipCmd, CmdTxEnb | CmdRxEnb);
	rtl_set_rx_tx_config_registers(unit);

	RTL_W8(base + Cfg9346, Cfg9346_Lock);

	RTL_R8(base + IntrMask);

	RTL_W32(base + RxMissed, 0);

	rtl_set_rx_mode(unit);

	RTL_W8(base + ChipCmd, CmdTxEnb | CmdRxEnb);

	RTL_W16(base + MultiIntr, RTL_R16(base + MultiIntr) & 0xf000);

	RTL_W16(base + IntrMask, np->intr_event);
}
