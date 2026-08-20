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

void rtl_hw_start_8101(struct net_device *unit)
{
    struct rtl816x_priv *np = get_pcnpriv(unit);
    APTR base = get_hwbase(unit);

    RTLD(bug("[%s] rtl_hw_start_8101()\n", unit->rtl816xu_name))

    if ((np->mcfg == RTL_GIGA_MAC_VER_13) ||
        (np->mcfg == RTL_GIGA_MAC_VER_16))
    {
        HIDD_PCIDevice_WriteConfigWord(unit->rtl816xu_PCIDevice, 0x68, 0x00);
        HIDD_PCIDevice_WriteConfigWord(unit->rtl816xu_PCIDevice, 0x69, 0x09);
    }

    RTL_W8(base + Cfg9346, Cfg9346_Unlock);

    RTL_W8(base + EarlyTxThres, EarlyTxThld);

    rtl_set_rx_max_size(unit);

    np->cp_cmd |= RTL_R16(base + CPlusCmd) | PCIMulRW;

    RTL_W16(base + CPlusCmd, np->cp_cmd);

    RTL_W16(base + IntrMitigate, 0x0000);

    rtl_set_rx_tx_desc_registers(unit);

    RTL_W8(base + ChipCmd, CmdTxEnb | CmdRxEnb);
    rtl_set_rx_tx_config_registers(unit);

    RTL_W8(base + Cfg9346, Cfg9346_Lock);

    /* PCI commit */
    RTL_R8(base + IntrMask);

    rtl_set_rx_mode(unit);

    RTL_W8(base + ChipCmd, CmdTxEnb | CmdRxEnb);

    RTL_W16(base + MultiIntr, RTL_R16(base + MultiIntr) & 0xf000);

    RTL_W16(base + IntrMask, np->intr_event);
}
