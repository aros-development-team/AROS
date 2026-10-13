#include <linux/types.h>
#include <linux/errno.h>
#include <linux/kobject.h>
/*
    Copyright 2009-2026, The AROS Development Team. All rights reserved.
*/

#ifndef _LINUX_OF_H_
#define _LINUX_OF_H_

struct device_node { const char *name; };
struct property;
#define of_property_read_bool(n, p)     false
#define of_property_read_u32(n, p, v)   (-ENODEV)
#define of_property_present(n, p)       false
#define of_find_property(n, p, l)       NULL
#define of_node_put(n)                  do { } while (0)
#define of_node_get(n)                  (n)
#define of_get_child_by_name(n, c)      NULL
#define of_find_node_by_name(f, n)      NULL
#define of_find_compatible_node(f, t, c) NULL
#define of_match_device(m, d)           NULL
#define of_device_is_compatible(n, c)   0
#define of_device_get_match_data(d)     NULL
#define of_get_property(n, p, l)        NULL
#define of_machine_is_compatible(c)     0
#define of_get_next_child(n, c)         NULL
#define for_each_child_of_node(p, c)    for (c = NULL; c; )
#define IS_OF_ENABLED                   0

#endif /* _LINUX_OF_H_ */
