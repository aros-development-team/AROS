/*
    Copyright 2009-2026, The AROS Development Team. All rights reserved.
*/

#ifndef _LINUX_GFP_H_
#define _LINUX_GFP_H_

#include <linux/types.h>

#define __GFP_DMA               0x01u
#define __GFP_HIGHMEM           0x02u
#define __GFP_DMA32             0x04u
#define __GFP_MOVABLE           0x08u
#define __GFP_RECLAIMABLE       0x10u
#define __GFP_HIGH              0x20u
#define __GFP_IO                0x40u
#define __GFP_FS                0x80u
#define __GFP_ZERO              0x100u
#define __GFP_ATOMIC            0x200u
#define __GFP_DIRECT_RECLAIM    0x400u
#define __GFP_KSWAPD_RECLAIM    0x800u
#define __GFP_NOWARN            0x2000u
#define __GFP_RETRY_MAYFAIL     0x4000u
#define __GFP_NOFAIL            0x8000u
#define __GFP_NORETRY           0x10000u
#define __GFP_MEMALLOC          0x20000u
#define __GFP_COMP              0x40000u
#define __GFP_NOMEMALLOC        0x80000u
#define __GFP_HARDWALL          0x100000u
#define __GFP_THISNODE          0x200000u
#define __GFP_ACCOUNT           0x400000u
#define __GFP_ZEROTAGS          0x800000u
#define __GFP_SKIP_ZERO         0x1000000u
#define __GFP_NOLOCKDEP         0x2000000u
#define __GFP_RECLAIM           (__GFP_DIRECT_RECLAIM | __GFP_KSWAPD_RECLAIM)
#define GFP_ATOMIC              (__GFP_HIGH | __GFP_ATOMIC | __GFP_KSWAPD_RECLAIM)
#define GFP_KERNEL              (__GFP_RECLAIM | __GFP_IO | __GFP_FS)
#define GFP_KERNEL_ACCOUNT      (GFP_KERNEL | __GFP_ACCOUNT)
#define GFP_NOWAIT              (__GFP_KSWAPD_RECLAIM)
#define GFP_NOIO                (__GFP_RECLAIM)
#define GFP_NOFS                (__GFP_RECLAIM | __GFP_IO)
#define GFP_USER                (__GFP_RECLAIM | __GFP_IO | __GFP_FS | __GFP_HARDWALL)
#define GFP_DMA                 __GFP_DMA
#define GFP_DMA32               __GFP_DMA32
#define GFP_HIGHUSER            (GFP_USER | __GFP_HIGHMEM)
#define GFP_HIGHUSER_MOVABLE    (GFP_HIGHUSER | __GFP_MOVABLE)
#define GFP_TRANSHUGE_LIGHT     GFP_KERNEL
#define GFP_TRANSHUGE           GFP_KERNEL
#define __GFP_BITS_SHIFT        26

static inline bool gfpflags_allow_blocking(const gfp_t gfp_flags)
{
    return !!(gfp_flags & __GFP_DIRECT_RECLAIM);
}
#define gfp_zone(g)             0
#define memalloc_noreclaim_save()   0
#define memalloc_noreclaim_restore(x) do { (void)(x); } while (0)
#define memalloc_nofs_save()    0
#define memalloc_nofs_restore(x) do { (void)(x); } while (0)
#define memalloc_noio_save()    0
#define memalloc_noio_restore(x) do { (void)(x); } while (0)
#define fs_reclaim_acquire(g)   do { } while (0)
#define fs_reclaim_release(g)   do { } while (0)
#define might_alloc(g)          do { } while (0)

#endif /* _LINUX_GFP_H_ */
