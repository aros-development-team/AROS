#ifndef _RTL8169_HW_H_
#define _RTL8169_HW_H_

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

#include <stdlib.h>

#include "unit.h"
#include LC_LIBDEFS_FILE

unsigned int rtl8169_tbi_link_ok(struct net_device *unit);
unsigned int rtl8169_xmii_link_ok(struct net_device *unit);
void rtl8169_tbi_reset_enable(struct net_device *unit);
void rtl8169_xmii_reset_enable(struct net_device *unit);
unsigned int rtl8169_tbi_reset_pending(struct net_device *unit);
unsigned int rtl8169_xmii_reset_pending(struct net_device *unit);
void rtl8169s_hw_phy_config(struct net_device *unit);
void rtl8169sb_hw_phy_config(struct net_device *unit);
void rtl_hw_start_8169(struct net_device *unit);

#endif
