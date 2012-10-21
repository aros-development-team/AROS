/**************************************************************************
 * 
 * Copyright 2003 Tungsten Graphics, Inc., Cedar Park, Texas.
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


#include "i915_reg.h"
#include "i915_context.h"
#include "i915_batch.h"
#include "i915_debug.h"
#include "i915_resource.h"

#include "pipe/p_context.h"
#include "pipe/p_defines.h"
#include "pipe/p_format.h"

#include "util/u_format.h"
#include "util/u_math.h"
#include "util/u_memory.h"

struct i915_tracked_hw_state {
   const char *name;
   void (*validate)(struct i915_context *, unsigned *batch_space);
   void (*emit)(struct i915_context *);
   unsigned dirty, batch_space;
};


static void
validate_flush(struct i915_context *i915, unsigned *batch_space)
{
   *batch_space = i915->flush_dirty ? 1 : 0;
}

static void
emit_flush(struct i915_context *i915)
{
   /* Cache handling is very cheap atm. State handling can request to flushes:
    * - I915_FLUSH_CACHE which is a flush everything request and
    * - I915_PIPELINE_FLUSH which is specifically for the draw_offset flush.
    * Because the cache handling is so dumb, no explicit "invalidate map cache".
    * Also, the first is a strict superset of the latter, so the following logic
    * works. */
   if (i915->flush_dirty & I915_FLUSH_CACHE)
      OUT_BATCH(MI_FLUSH | FLUSH_MAP_CACHE);
   else if (i915->flush_dirty & I915_PIPELINE_FLUSH)
      OUT_BATCH(MI_FLUSH | INHIBIT_FLUSH_RENDER_CACHE);
}

uint32_t invariant_state[] = {
   _3DSTATE_AA_CMD | AA_LINE_ECAAR_WIDTH_ENABLE | AA_LINE_ECAAR_WIDTH_1_0 |
             AA_LINE_REGION_WIDTH_ENABLE | AA_LINE_REGION_WIDTH_1_0,

   _3DSTATE_DFLT_DIFFUSE_CMD, 0,

   _3DSTATE_DFLT_SPEC_CMD, 0,

   _3DSTATE_DFLT_Z_CMD, 0,

   _3DSTATE_COORD_SET_BINDINGS |
             CSB_TCB(0, 0) |
             CSB_TCB(1, 1) |
             CSB_TCB(2, 2) |
             CSB_TCB(3, 3) |
             CSB_TCB(4, 4) |
             CSB_TCB(5, 5) |
             CSB_TCB(6, 6) |
             CSB_TCB(7, 7),

   _3DSTATE_RASTER_RULES_CMD |
             ENABLE_POINT_RASTER_RULE |
             OGL_POINT_RASTER_RULE |
             ENABLE_LINE_STRIP_PROVOKE_VRTX |
             ENABLE_TRI_FAN_PROVOKE_VRTX |
             LINE_STRIP_PROVOKE_VRTX(1) |
             TRI_FAN_PROVOKE_VRTX(2) |
             ENABLE_TEXKILL_3D_4D |
             TEXKILL_4D,

   _3DSTATE_DEPTH_SUBRECT_DISABLE,

   /* disable indirect state for now
    */
   _3DSTATE_LOAD_INDIRECT | 0, 0};

static void
emit_invariant(struct i915_context *i915)
{
   i915_winsys_batchbuffer_write(i915->batch, invariant_state,
                                 Elements(invariant_state)*sizeof(uint32_t));
}

static void
validate_immediate(struct i915_context *i915, unsigned *batch_space)
{
   unsigned dirty = (1 << I915_IMMEDIATE_S0 | 1 << I915_IMMEDIATE_S1 |
                     1 << I915_IMMEDIATE_S2 | 1 << I915_IMMEDIATE_S3 |
                     1 << I915_IMMEDIATE_S3 | 1 << I915_IMMEDIATE_S4 |
                     1 << I915_IMMEDIATE_S5 | 1 << I915_IMMEDIATE_S6) &
                    i915->immediate_dirty;

   if (i915->immediate_dirty & (1 << I915_IMMEDIATE_S0) && i915->vbo)
      i915->validation_buffers[i915->num_validation_buffers++] = i915->vbo;

   *batch_space = 1 + util_bitcount(dirty);
}

static void
emit_immediate(struct i915_context *i915)
{
   /* remove unwatned bits and S7 */
   unsigned dirty = (1 << I915_IMMEDIATE_S0 | 1 << I915_IMMEDIATE_S1 |
                     1 << I915_IMMEDIATE_S2 | 1 << I915_IMMEDIATE_S3 |
                     1 << I915_IMMEDIATE_S3 | 1 << I915_IMMEDIATE_S4 |
                     1 << I915_IMMEDIATE_S5 | 1 << I915_IMMEDIATE_S6) &
                    i915->immediate_dirty;
   int i, num = util_bitcount(dirty);
   assert(num && num <= I915_MAX_IMMEDIATE);

   OUT_BATCH(_3DSTATE_LOAD_STATE_IMMEDIATE_1 |
             dirty << 4 | (num - 1));

   if (i915->immediate_dirty & (1 << I915_IMMEDIATE_S0)) {
      if (i915->vbo)
         OUT_RELOC(i915->vbo, I915_USAGE_VERTEX,
                   i915->current.immediate[I915_IMMEDIATE_S0]);
      else
         OUT_BATCH(0);
   }

   for (i = 1; i < I915_MAX_IMMEDIATE; i++) {
      if (dirty & (1 << i))
         OUT_BATCH(i915->current.immediate[i]);
   }
}

static void
validate_dynamic(struct i915_context *i915, unsigned *batch_space)
{
   *batch_space = util_bitcount(i915->dynamic_dirty & ((1 << I915_MAX_DYNAMIC) - 1));
}

static void
emit_dynamic(struct i915_context *i915)
{
   int i;
   for (i = 0; i < I915_MAX_DYNAMIC; i++) {
      if (i915->dynamic_dirty & (1 << i))
         OUT_BATCH(i915->current.dynamic[i]);
   }
}

static void
validate_static(struct i915_context *i915, unsigned *batch_space)
{
   *batch_space = 0;

   if (i915->current.cbuf_bo && (i915->static_dirty & I915_DST_BUF_COLOR)) {
      i915->validation_buffers[i915->num_validation_buffers++]
         = i915->current.cbuf_bo;
      *batch_space += 3;
   }

   if (i915->current.depth_bo && (i915->static_dirty & I915_DST_BUF_DEPTH)) {
      i915->validation_buffers[i915->num_validation_buffers++]
         = i915->current.depth_bo;
      *batch_space += 3;
   }

   if (i915->static_dirty & I915_DST_VARS)
      *batch_space += 2;

   if (i915->static_dirty & I915_DST_RECT)
      *batch_space += 5;
}

static void
emit_static(struct i915_context *i915)
{
   if (i915->current.cbuf_bo && (i915->static_dirty & I915_DST_BUF_COLOR)) {
      OUT_BATCH(_3DSTATE_BUF_INFO_CMD);
      OUT_BATCH(i915->current.cbuf_flags);
      OUT_RELOC(i915->current.cbuf_bo,
                I915_USAGE_RENDER,
                0);
   }

   /* What happens if no zbuf??
    */
   if (i915->current.depth_bo && (i915->static_dirty & I915_DST_BUF_DEPTH)) {
      OUT_BATCH(_3DSTATE_BUF_INFO_CMD);
      OUT_BATCH(i915->current.depth_flags);
      OUT_RELOC(i915->current.depth_bo,
                I915_USAGE_RENDER,
                0);
   }

   if (i915->static_dirty & I915_DST_VARS) {
      OUT_BATCH(_3DSTATE_DST_BUF_VARS_CMD);
      OUT_BATCH(i915->current.dst_buf_vars);
   }
}

static void
validate_map(struct i915_context *i915, unsigned *batch_space)
{
   const uint enabled = i915->current.sampler_enable_flags;
   uint unit;
   struct i915_texture *tex;

   *batch_space = i915->current.sampler_enable_nr ?
     2 + 3*i915->current.sampler_enable_nr : 0;

   for (unit = 0; unit < I915_TEX_UNITS; unit++) {
      if (enabled & (1 << unit)) {
         tex = i915_texture(i915->fragment_sampler_views[unit]->texture);
         i915->validation_buffers[i915->num_validation_buffers++] = tex->buffer;
      }
   }
}

static void
emit_map(struct i915_context *i915)
{
   const uint nr = i915->current.sampler_enable_nr;
   if (nr) {
      const uint enabled = i915->current.sampler_enable_flags;
      uint unit;
      uint count = 0;
      OUT_BATCH(_3DSTATE_MAP_STATE | (3 * nr));
      OUT_BATCH(enabled);
      for (unit = 0; unit < I915_TEX_UNITS; unit++) {
         if (enabled & (1 << unit)) {
            struct i915_texture *texture = i915_texture(i915->fragment_sampler_views[unit]->texture);
            struct i915_winsys_buffer *buf = texture->buffer;
            assert(buf);

            count++;

            OUT_RELOC(buf, I915_USAGE_SAMPLER, 0);
            OUT_BATCH(i915->current.texbuffer[unit][0]); /* MS3 */
            OUT_BATCH(i915->current.texbuffer[unit][1]); /* MS4 */
         }
      }
      assert(count == nr);
   }
}

static void
validate_sampler(struct i915_context *i915, unsigned *batch_space)
{
   *batch_space = i915->current.sampler_enable_nr ?
     2 + 3*i915->current.sampler_enable_nr : 0;
}

static void
emit_sampler(struct i915_context *i915)
{
   if (i915->current.sampler_enable_nr) {
      int i;

      OUT_BATCH( _3DSTATE_SAMPLER_STATE |
                 (3 * i915->current.sampler_enable_nr) );

      OUT_BATCH( i915->current.sampler_enable_flags );

      for (i = 0; i < I915_TEX_UNITS; i++) {
         if (i915->current.sampler_enable_flags & (1<<i)) {
            OUT_BATCH( i915->current.sampler[i][0] );
            OUT_BATCH( i915->current.sampler[i][1] );
            OUT_BATCH( i915->current.sampler[i][2] );
         }
      }
   }
}

static void
validate_constants(struct i915_context *i915, unsigned *batch_space)
{
   *batch_space = i915->fs->num_constants ?
      2 + 4*i915->fs->num_constants : 0;
}

static void
emit_constants(struct i915_context *i915)
{
   /* Collate the user-defined constants with the fragment shader's
    * immediates according to the constant_flags[] array.
    */
   const uint nr = i915->fs->num_constants;
   if (nr) {
      uint i;

      OUT_BATCH( _3DSTATE_PIXEL_SHADER_CONSTANTS | (nr * 4) );
      OUT_BATCH((1 << nr) - 1);

      for (i = 0; i < nr; i++) {
         const uint *c;
         if (i915->fs->constant_flags[i] == I915_CONSTFLAG_USER) {
            /* grab user-defined constant */
            c = (uint *) i915_buffer(i915->constants[PIPE_SHADER_FRAGMENT])->data;
            c += 4 * i;
         }
         else {
            /* emit program constant */
            c = (uint *) i915->fs->constants[i];
         }
#if 0 /* debug */
         {
            float *f = (float *) c;
            printf("Const %2d: %f %f %f %f %s\n", i, f[0], f[1], f[2], f[3],
                   (i915->fs->constant_flags[i] == I915_CONSTFLAG_USER
                    ? "user" : "immediate"));
         }
#endif
         OUT_BATCH(*c++);
         OUT_BATCH(*c++);
         OUT_BATCH(*c++);
         OUT_BATCH(*c++);
      }
   }
}

static const struct
{
   enum pipe_format format;
   uint hw_swizzle;
} fixup_formats[] = {
   { PIPE_FORMAT_R8G8B8A8_UNORM, 0x21030000 /* BGRA */},
   { PIPE_FORMAT_L8_UNORM,       0x00030000 /* RRRA */},
   { PIPE_FORMAT_I8_UNORM,       0x00030000 /* RRRA */},
   { PIPE_FORMAT_A8_UNORM,       0x33330000 /* AAAA */},
   { PIPE_FORMAT_NONE,           0x00000000},
};

static uint need_target_fixup(struct pipe_surface* p)
{
   enum pipe_format f;
   /* if we don't have a surface bound yet, we don't need to fixup the shader */
   if (!p)
      return 0;

   f = p->format;
   for(int i=0; fixup_formats[i].format != PIPE_FORMAT_NONE; i++)
      if (fixup_formats[i].format == f)
         return 1;

   return 0;
}

static uint fixup_swizzle(enum pipe_format f)
{
   for(int i=0; fixup_formats[i].format != PIPE_FORMAT_NONE; i++)
      if (fixup_formats[i].format == f)
         return fixup_formats[i].hw_swizzle;

   return 0;
}

static void
validate_program(struct i915_context *i915, unsigned *batch_space)
{
   struct pipe_surface *cbuf_surface = i915->framebuffer.cbufs[0];
   uint additional_size = need_target_fixup(cbuf_surface);

   /* we need more batch space if we want to emulate rgba framebuffers */
   *batch_space = i915->fs->program_len + 3 * additional_size;
}

static void
emit_program(struct i915_context *i915)
{
   struct pipe_surface *cbuf_surface = i915->framebuffer.cbufs[0];
   uint target_fixup = need_target_fixup(cbuf_surface);
   uint i;

   /* we should always have, at least, a pass-through program */
   assert(i915->fs->program_len > 0);

   {
      /* first word has the size, we have to adjust that */
      uint size = (i915->fs->program[0]);
      size += target_fixup * 3;
      OUT_BATCH(size);
   }

   /* output the declarations of the program */
   for (i=1 ; i < i915->fs->program_len; i++) 
      OUT_BATCH(i915->fs->program[i]);

   /* we emit an additional mov with swizzle to fake RGBA framebuffers */
   if (target_fixup) {
      /* mov out_color, out_color.zyxw */
      OUT_BATCH(A0_MOV |
                (REG_TYPE_OC << A0_DEST_TYPE_SHIFT) |
                A0_DEST_CHANNEL_ALL |
                (REG_TYPE_OC << A0_SRC0_TYPE_SHIFT) |
                (T_DIFFUSE << A0_SRC0_NR_SHIFT));
      OUT_BATCH(fixup_swizzle(cbuf_surface->format));
      OUT_BATCH(0);
   }
}

static void
emit_draw_rect(struct i915_context *i915)
{
   if (i915->static_dirty & I915_DST_RECT) {
      OUT_BATCH(_3DSTATE_DRAW_RECT_CMD);
      OUT_BATCH(DRAW_RECT_DIS_DEPTH_OFS);
      OUT_BATCH(i915->current.draw_offset);
      OUT_BATCH(i915->current.draw_size);
      OUT_BATCH(i915->current.draw_offset);
   }
}

static boolean
i915_validate_state(struct i915_context *i915, unsigned *batch_space)
{
   unsigned tmp;

   i915->num_validation_buffers = 0;
   if (i915->hardware_dirty & I915_HW_INVARIANT)
      *batch_space = Elements(invariant_state);
   else
      *batch_space = 0;

#define VALIDATE_ATOM(atom, hw_dirty) \
   if (i915->hardware_dirty & hw_dirty) { \
      validate_##atom(i915, &tmp); \
      *batch_space += tmp; }
   VALIDATE_ATOM(flush, I915_HW_FLUSH);
   VALIDATE_ATOM(immediate, I915_HW_IMMEDIATE);
   VALIDATE_ATOM(dynamic, I915_HW_DYNAMIC);
   VALIDATE_ATOM(static, I915_HW_STATIC);
   VALIDATE_ATOM(map, I915_HW_MAP);
   VALIDATE_ATOM(sampler, I915_HW_SAMPLER);
   VALIDATE_ATOM(constants, I915_HW_CONSTANTS);
   VALIDATE_ATOM(program, I915_HW_PROGRAM);
#undef VALIDATE_ATOM

   if (i915->num_validation_buffers == 0)
      return TRUE;

   if (!i915_winsys_validate_buffers(i915->batch, i915->validation_buffers,
                                     i915->num_validation_buffers))
      return FALSE;

   return TRUE;
}

/* Push the state into the sarea and/or texture memory.
 */
void
i915_emit_hardware_state(struct i915_context *i915 )
{
   unsigned batch_space;
   uintptr_t save_ptr;

   assert(i915->dirty == 0);

   if (I915_DBG_ON(DBG_ATOMS))
      i915_dump_hardware_dirty(i915, __FUNCTION__);

   if (!i915_validate_state(i915, &batch_space)) {
      FLUSH_BATCH(NULL);
      assert(i915_validate_state(i915, &batch_space));
   }

   if(!BEGIN_BATCH(batch_space)) {
      FLUSH_BATCH(NULL);
      assert(i915_validate_state(i915, &batch_space));
      assert(BEGIN_BATCH(batch_space));
   }

   save_ptr = (uintptr_t)i915->batch->ptr;

#define EMIT_ATOM(atom, hw_dirty) \
   if (i915->hardware_dirty & hw_dirty) \
      emit_##atom(i915);
   EMIT_ATOM(flush, I915_HW_FLUSH);
   EMIT_ATOM(invariant, I915_HW_INVARIANT);
   EMIT_ATOM(immediate, I915_HW_IMMEDIATE);
   EMIT_ATOM(dynamic, I915_HW_DYNAMIC);
   EMIT_ATOM(static, I915_HW_STATIC);
   EMIT_ATOM(map, I915_HW_MAP);
   EMIT_ATOM(sampler, I915_HW_SAMPLER);
   EMIT_ATOM(constants, I915_HW_CONSTANTS);
   EMIT_ATOM(program, I915_HW_PROGRAM);
   EMIT_ATOM(draw_rect, I915_HW_STATIC);
#undef EMIT_ATOM

   I915_DBG(DBG_EMIT, "%s: used %d dwords, %d dwords reserved\n", __FUNCTION__,
            ((uintptr_t)i915->batch->ptr - save_ptr) / 4,
            batch_space);
   assert(((uintptr_t)i915->batch->ptr - save_ptr) / 4 == batch_space);

   i915->hardware_dirty = 0;
   i915->immediate_dirty = 0;
   i915->dynamic_dirty = 0;
   i915->static_dirty = 0;
   i915->flush_dirty = 0;
}
