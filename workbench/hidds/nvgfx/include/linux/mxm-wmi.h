/*
    Copyright 2026, The AROS Development Team. All rights reserved.
*/
#ifndef _LINUX_MXM_WMI_H_
#define _LINUX_MXM_WMI_H_

/* no ACPI WMI: MXM data is not offered through it */
static inline bool mxm_wmi_supported(void) { return false; }
static inline int mxm_wmi_call_mxmx(int adapter) { return -ENODEV; }
static inline int mxm_wmi_call_mxds(int adapter) { return -ENODEV; }

#endif /* _LINUX_MXM_WMI_H_ */
