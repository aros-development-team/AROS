#include <linux/types.h>
#include <linux/llist.h>
/*
    Copyright 2009-2026, The AROS Development Team. All rights reserved.
*/

#ifndef _LINUX_CGROUP_DMEM_H_
#define _LINUX_CGROUP_DMEM_H_

struct dmem_cgroup_pool_state;
struct dmem_cgroup_region;
static inline struct dmem_cgroup_region *dmem_cgroup_register_region(u64 size, const char *fmt, ...) { return NULL; }
static inline void dmem_cgroup_unregister_region(struct dmem_cgroup_region *r) { }
static inline int dmem_cgroup_try_charge(struct dmem_cgroup_region *r, u64 size, struct dmem_cgroup_pool_state **ret_pool, struct dmem_cgroup_pool_state **ret_limit_pool) { *ret_pool = NULL; if (ret_limit_pool) *ret_limit_pool = NULL; return 0; }
static inline void dmem_cgroup_uncharge(struct dmem_cgroup_pool_state *pool, u64 size) { }
static inline bool dmem_cgroup_state_evict_valuable(struct dmem_cgroup_pool_state *limit_pool, struct dmem_cgroup_pool_state *test_pool, bool ignore_low, bool *ret_hit_low) { return true; }
static inline void dmem_cgroup_pool_state_put(struct dmem_cgroup_pool_state *pool) { }

#endif /* _LINUX_CGROUP_DMEM_H_ */
