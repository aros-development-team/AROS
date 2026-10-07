#include <linux/errno.h>
#include <linux/types.h>
/*
    Copyright 2009-2026, The AROS Development Team. All rights reserved.
*/

#ifndef _LINUX_KMSG_DUMP_H_
#define _LINUX_KMSG_DUMP_H_


struct kmsg_dumper { void (*dump)(struct kmsg_dumper *dumper, int reason); bool registered; };
static inline int kmsg_dump_register(struct kmsg_dumper *dumper) { return 0; }
static inline int kmsg_dump_unregister(struct kmsg_dumper *dumper) { return 0; }
#endif /* _LINUX_KMSG_DUMP_H_ */
