/*
    Copyright 2009-2026, The AROS Development Team. All rights reserved.
*/

#ifndef _LINUX_STRING_HELPERS_H_
#define _LINUX_STRING_HELPERS_H_

#include <linux/string.h>
#define str_yes_no(v)           ((v) ? "yes" : "no")
#define str_on_off(v)           ((v) ? "on" : "off")
#define str_enable_disable(v)   ((v) ? "enable" : "disable")
#define str_enabled_disabled(v) ((v) ? "enabled" : "disabled")
#define str_true_false(v)       ((v) ? "true" : "false")
#define str_up_down(v)          ((v) ? "up" : "down")
#define str_read_write(v)       ((v) ? "read" : "write")
#define str_plural(n)           ((n) == 1 ? "" : "s")

#endif /* _LINUX_STRING_HELPERS_H_ */
