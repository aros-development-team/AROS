#ifndef _RTL816X_HW_H_
#define _RTL816X_HW_H_

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
#include "unit.h"

/* 8169/8110 family */
unsigned int rtl8169_tbi_link_ok(struct net_device *unit);
unsigned int rtl8169_xmii_link_ok(struct net_device *unit);
void rtl8169_tbi_reset_enable(struct net_device *unit);
void rtl8169_xmii_reset_enable(struct net_device *unit);
unsigned int rtl8169_tbi_reset_pending(struct net_device *unit);
unsigned int rtl8169_xmii_reset_pending(struct net_device *unit);
void rtl8169s_hw_phy_config(struct net_device *unit);
void rtl8169sb_hw_phy_config(struct net_device *unit);
void rtl_hw_start_8169(struct net_device *unit);

/* 8168/8111 family */
void rtl8168_hw_phy_config(struct net_device *unit);
void rtl_hw_start_8168(struct net_device *unit);

/* 8101/8102 family */
void rtl_hw_start_8101(struct net_device *unit);

#endif
