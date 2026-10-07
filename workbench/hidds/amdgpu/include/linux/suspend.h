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
typedef int suspend_state_t;

#define PM_SUSPEND_ON           ((suspend_state_t) 0)
#define PM_SUSPEND_TO_IDLE      ((suspend_state_t) 1)
#define PM_SUSPEND_STANDBY      ((suspend_state_t) 2)
#define PM_SUSPEND_MEM          ((suspend_state_t) 3)
#define PM_SUSPEND_MAX          ((suspend_state_t) 4)

#define pm_suspend_target_state PM_SUSPEND_ON

static inline bool pm_hibernate_is_recovering(void) { return false; }
static inline bool pm_hibernation_mode_is_suspend(void) { return false; }

#endif
