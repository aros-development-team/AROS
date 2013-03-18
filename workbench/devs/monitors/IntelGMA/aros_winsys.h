/*
    Copyright © 2011-2013, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef INTEL_AROS_WINSYS_H
#define INTEL_AROS_WINSYS_H

#include <proto/alib.h>
#include <proto/exec.h>
#include <exec/types.h>
#include <exec/lists.h>
#include <exec/nodes.h>

#include "gallium/i915_batchbuffer.h"
#include "hidd/gallium.h"

#define MAGIC 0x12345678
#define RELOC_MAGIC 0x87654321
#define MAGIC_WARNING(b) if(b->magic != MAGIC ){ bug("[GMA winsys] %s: Bad MAGIC in buffer %p\n",__func__,b);};
#define IF_BAD_MAGIC(b) MAGIC_WARNING(b);if(b->magic != MAGIC )

#define MAX_RELOCS 1024

struct aros_winsys
{
    struct i915_winsys base;
    struct pipe_screen *pscreen;
    size_t max_batch_size;
};

static INLINE struct aros_winsys *
aros_winsys(struct i915_winsys *iws)
{
   return (struct aros_winsys *)iws;
}

struct reloc
{
    struct i915_winsys_buffer *buf;
    enum i915_winsys_buffer_usage usage;
    ULONG offset;
    uint32_t *ptr;
};

struct aros_batchbuffer
{
   struct i915_winsys_batchbuffer base;
   size_t actual_size;
   APTR allocated_map;
   ULONG allocated_size;
   struct reloc relocs[MAX_RELOCS];
   APTR gfxmap;
};

static INLINE struct aros_batchbuffer *
aros_batchbuffer(struct i915_winsys_batchbuffer *batch)
{
   return (struct aros_batchbuffer *)batch;
};

struct i915_winsys_buffer {
   struct Node bnode;
   ULONG magic;
   APTR map;
   ULONG size;
   void *ptr;
   ULONG stride;
   APTR allocated_map;
   ULONG allocated_size;
   ULONG status_index;
   ULONG flush_num;
};

struct aros_winsys *winsys_create();
boolean buffer_is_busy(struct i915_winsys *iws,struct i915_winsys_buffer *buf);
void destroy_unused_buffers();
#endif
