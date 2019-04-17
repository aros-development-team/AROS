/*
    Copyright © 2011-2019, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef INTELGMA_WINSYS_H_
#define INTELGMA_WINSYS_H_

#include <exec/types.h>
#include <exec/lists.h>
#include <exec/nodes.h>

#include "i915/i915_winsys.h"
#include "i915/i915_batchbuffer.h"
#include "i915/i915_reg.h"
#include "i915/i915_public.h"
#include "i915/i915_resource.h"
#include "util/u_memory.h"
#include "util/u_atomic.h"
#include "util/u_inlines.h"
#include "i915/i915_debug.h"

#define MAGIC                   0x12345678
#define RELOC_MAGIC             0x87654321
#define MAGIC_WARNING(b)        if(b->magic != MAGIC ){ bug("[GMA winsys] %s: Bad MAGIC in buffer %p\n",__func__,b);};
#define IF_BAD_MAGIC(b)         MAGIC_WARNING(b);if(b->magic != MAGIC )

#define MAX_RELOCS              1024
#define BATCH_RESERVED          16

#define LOCK_BB                 { ObtainSemaphore(&BatchBufferLock); }
#define UNLOCK_BB               { ReleaseSemaphore(&BatchBufferLock); }

#define BASEADDRESS(p)          ((IPTR)(p) - (IPTR)sd->Card.Framebuffer)

#define MI_BATCH_NON_SECURE     (1)

#ifdef GALLIUM_SIMULATION
# undef LOCK_HW
# undef START_RING
# undef OUT_RING
# undef ADVANCE_RING
# undef WAIT_IDLE
# undef DO_FLUSH
# undef UNLOCK_HW
# define LOCK_HW
# define START_RING(x)
# define OUT_RING(x)
# define ADVANCE_RING()
# define WAIT_IDLE()
# define DO_FLUSH()
# define UNLOCK_HW
# warning GALLIUM_SIMULATION MODE!
#endif

struct status
{
    BOOL                                reserved;
    ULONG                               flush_num;
};

struct reloc
{
    struct i915_winsys_buffer           *buf;
    enum i915_winsys_buffer_usage       usage;
    ULONG                               offset;
    ULONG                               *ptr;
};

struct IntelGMABatchBuffer
{
    struct i915_winsys_batchbuffer      base;
    struct i915_winsys                  *iws;
    size_t                              actual_size;
    APTR                                allocated_map;
    ULONG                               allocated_size;
    struct reloc                        relocs[MAX_RELOCS];
    APTR                                gfxmap;
};

struct i915_winsys_buffer {
    struct Node                         bnode;
    ULONG                               magic;
    APTR                                map;
    ULONG                               size;
    void                                *ptr;
    ULONG                               stride;
    APTR                                allocated_map;
    ULONG                               allocated_size;
    ULONG                               status_index;
    ULONG                               flush_num;
};

#define INTELGMA_BB(batch) ((struct IntelGMABatchBuffer *)(batch))

#endif
