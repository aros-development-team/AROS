/* SPDX-License-Identifier: MIT */
#ifndef __NOUVEAU_OS_H__
#define __NOUVEAU_OS_H__

#if !defined(__AROS__)
#include <linux/types.h>
#include <linux/slab.h>
#include <linux/mutex.h>
#include <linux/pci.h>
#include <linux/platform_device.h>
#include <linux/printk.h>
#include <linux/bitops.h>
#include <linux/firmware.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/i2c-algo-bit.h>
#include <linux/delay.h>
#include <linux/io-mapping.h>
#include <linux/acpi.h>
#include <linux/vmalloc.h>
#include <linux/dmi.h>
#include <linux/reboot.h>
#include <linux/interrupt.h>
#include <linux/log2.h>
#include <linux/pm_runtime.h>
#include <linux/power_supply.h>
#include <linux/clk.h>
#include <linux/regulator/consumer.h>
#include <linux/agp_backend.h>
#include <linux/reset.h>
#include <linux/iommu.h>
#include <linux/of_device.h>

#include <asm/unaligned.h>

#include <soc/tegra/fuse.h>
#include <soc/tegra/pmc.h>
#else
#include <drm-compat/drm_compat_types.h>
#include <drm-compat/drm_compat_funcs.h>
#include <drm-compat/drm_compat_dma.h>
#include <drm-compat/drm_compat_pci.h>

#include <linux/overflow.h>
#include <linux/rbtree.h>
#include <linux/bitmap.h>
#include <linux/ktime.h>
#include <linux/refcount.h>
#include <linux/list.h>

#define CONFIG_ARM_ENABLED 0
// #define MOCK_HARDWARE
#endif
#endif
