/*
    Copyright 2009-2026, The AROS Development Team. All rights reserved.
*/

#ifndef _LINUX_STACKDEPOT_H_
#define _LINUX_STACKDEPOT_H_

typedef u32 depot_stack_handle_t;
#define stack_depot_save(e, n, g)   (0)
#define stack_depot_print(h)        do { } while (0)
#define stack_depot_snprint(h, b, s, sp) (0)
#define stack_depot_init()          (0)
#define stack_trace_save(e, n, s)   (0)
#define STACK_DEPOT_MAX_FRAMES      64

#endif /* _LINUX_STACKDEPOT_H_ */
