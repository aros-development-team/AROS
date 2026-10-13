/*
    Copyright 2026, The AROS Development Team. All rights reserved.

    There is no system suspend or hibernation here; a PM notifier is
    accepted and never called.
*/
#ifndef _LINUX_SUSPEND_H_
#define _LINUX_SUSPEND_H_
#include <linux/notifier.h>
#include <linux/pm.h>

#define PM_HIBERNATION_PREPARE  0x0001
#define PM_POST_HIBERNATION     0x0002
#define PM_SUSPEND_PREPARE      0x0003
#define PM_POST_SUSPEND         0x0004
#define PM_RESTORE_PREPARE      0x0005
#define PM_POST_RESTORE         0x0006

static inline int register_pm_notifier(struct notifier_block *nb)   { return 0; }
static inline int unregister_pm_notifier(struct notifier_block *nb) { return 0; }
#endif
