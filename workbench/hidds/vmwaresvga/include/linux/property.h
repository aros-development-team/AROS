/*
    Copyright 2009-2026, The AROS Development Team. All rights reserved.
*/

#ifndef _LINUX_PROPERTY_H_
#define _LINUX_PROPERTY_H_

#include <linux/of.h>
#include <linux/args.h>
#include <linux/array_size.h>
#include <linux/bits.h>
#include <linux/fwnode.h>
#include <linux/stddef.h>
#include <linux/types.h>
struct fwnode_handle { struct fwnode_handle *secondary; };
static inline void fwnode_handle_put(struct fwnode_handle *fwnode) { }
static inline struct fwnode_handle *fwnode_handle_get(struct fwnode_handle *fwnode) { return fwnode; }

#endif /* _LINUX_PROPERTY_H_ */
