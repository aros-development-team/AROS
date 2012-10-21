/**************************************************************************
 * 
 * Copyright 2007 Tungsten Graphics, Inc., Cedar Park, Texas.
 * All Rights Reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 **************************************************************************/

/* Author:
 *    Keith Whitwell <keith@tungstengraphics.com>
 */


#include "pipe/p_defines.h"
#include "draw/draw_context.h"
#include "i915_context.h"
#include "i915_reg.h"
#include "i915_batch.h"
#include "i915_debug.h"


static void i915_flush_pipe( struct pipe_context *pipe,
                             struct pipe_fence_handle **fence )
{
   struct i915_context *i915 = i915_context(pipe);

   draw_flush(i915->draw);

   if (i915->batch->map == i915->batch->ptr) {
      return;
   }

   /* If there are no flags, just flush pending commands to hardware:
    */
   FLUSH_BATCH(fence);

   I915_DBG(DBG_FLUSH, "%s: #####\n", __FUNCTION__);
}

void i915_init_flush_functions( struct i915_context *i915 )
{
   i915->base.flush = i915_flush_pipe;
}

/**
 * Here we handle all the notifications that needs to go out on a flush.
 * XXX might move above function to i915_pipe_flush.c and leave this here.
 */
void i915_flush(struct i915_context *i915, struct pipe_fence_handle **fence)
{
   struct i915_winsys_batchbuffer *batch = i915->batch;

   batch->iws->batchbuffer_flush(batch, fence);
   i915->vbo_flushed = 1;
   i915->hardware_dirty = ~0;
   i915->immediate_dirty = ~0;
   i915->dynamic_dirty = ~0;
   i915->static_dirty = ~0;
   /* kernel emits flushes in between batchbuffers */
   i915->flush_dirty = 0;
   i915->vertices_since_last_flush = 0;
}
