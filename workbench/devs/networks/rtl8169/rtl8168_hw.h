#ifndef _RTL8168_HW_H_
#define _RTL8168_HW_H_

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

void rtl8168cp_hw_phy_config(struct net_device *unit);
void rtl8168c_hw_phy_config(struct net_device *unit);
void rtl8168cx_hw_phy_config(struct net_device *unit);
void rtl_hw_start_8168(struct net_device *unit);

#endif
