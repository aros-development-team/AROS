/*
    Copyright 2009-2026, The AROS Development Team. All rights reserved.
*/

#ifndef _LINUX_PM_H_
#define _LINUX_PM_H_

#include <linux/device.h>
typedef struct pm_message { int event; } pm_message_t;

#define DPM_FLAG_NO_DIRECT_COMPLETE BIT(0)
#define DPM_FLAG_SMART_PREPARE      BIT(1)
#define DPM_FLAG_SMART_SUSPEND      BIT(2)
#define DPM_FLAG_MAY_SKIP_RESUME    BIT(3)
#define dev_pm_set_driver_flags(dev, flags) do { } while (0)

#endif /* _LINUX_PM_H_ */
