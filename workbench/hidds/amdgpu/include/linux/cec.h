#include <linux/types.h>
#include <linux/device.h>
/*
    Copyright 2009-2026, The AROS Development Team. All rights reserved.
*/

#ifndef _LINUX_CEC_H_
#define _LINUX_CEC_H_

struct cec_adapter;
struct cec_connector_info;
#define cec_notifier_set_phys_addr_from_edid(n, e) do { } while (0)
#define cec_notifier_phys_addr_invalidate(n) do { } while (0)
#define cec_notifier_conn_register(d, p, c) NULL
#define cec_notifier_conn_unregister(n) do { } while (0)
#define cec_fill_conn_info_from_drm(c, con) do { } while (0)
#define CEC_PHYS_ADDR_INVALID 0xffff

#endif /* _LINUX_CEC_H_ */
