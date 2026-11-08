/*
    Copyright 2009-2026, The AROS Development Team. All rights reserved.
*/

#ifndef _LINUX_REBOOT_H_
#define _LINUX_REBOOT_H_

#include <linux/notifier.h>
#include <linux/types.h>
#define SYS_DOWN                0x0001
#define SYS_RESTART             SYS_DOWN
#define SYS_HALT                0x0002
#define SYS_POWER_OFF           0x0003
static inline int register_reboot_notifier(struct notifier_block *nb) { return 0; }
static inline int unregister_reboot_notifier(struct notifier_block *nb) { return 0; }

#endif /* _LINUX_REBOOT_H_ */
