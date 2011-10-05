/*
 * Copyright 2008 Ben Skeggs
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "nvc0_context.h"
#include "nvc0_resource.h"
#include "nv50/nv50_texture.xml.h"

#include "util/u_format.h"

#define NV50_TIC_0_SWIZZLE__MASK                      \
   (NV50_TIC_0_MAPA__MASK | NV50_TIC_0_MAPB__MASK |   \
    NV50_TIC_0_MAPG__MASK | NV50_TIC_0_MAPR__MASK)

static INLINE uint32_t
nv50_tic_swizzle(uint32_t tc, unsigned swz, boolean tex_int)
{
   switch (swz) {
   case PIPE_SWIZZLE_RED:
      return (tc & NV50_TIC_0_MAPR__MASK) >> NV50_TIC_0_MAPR__SHIFT;
   case PIPE_SWIZZLE_GREEN:
      return (tc & NV50_TIC_0_MAPG__MASK) >> NV50_TIC_0_MAPG__SHIFT;
   case PIPE_SWIZZLE_BLUE:
      return (tc & NV50_TIC_0_MAPB__MASK) >> NV50_TIC_0_MAPB__SHIFT;
   case PIPE_SWIZZLE_ALPHA:
      return (tc & NV50_TIC_0_MAPA__MASK) >> NV50_TIC_0_MAPA__SHIFT;
   case PIPE_SWIZZLE_ONE:
      return tex_int ? NV50_TIC_MAP_ONE_INT : NV50_TIC_MAP_ONE_FLOAT;
   case PIPE_SWIZZLE_ZERO:
   default:
      return NV50_TIC_MAP_ZERO;
   }
}

struct pipe_sampler_view *
nvc0_create_sampler_view(struct pipe_context *pipe,
                         struct pipe_resource *texture,
                         const struct pipe_sampler_view *templ)
{
   const struct util_format_description *desc;
   uint32_t *tic;
   uint32_t swz[4];
   uint32_t depth;
   struct nv50_tic_entry *view;
   struct nvc0_miptree *mt = nvc0_miptree(texture);
   boolean tex_int;

   view = MALLOC_STRUCT(nv50_tic_entry);
   if (!view)
      return NULL;

   view->pipe = *templ;
   view->pipe.reference.count = 1;
   view->pipe.texture = NULL;
   view->pipe.context = pipe;

   view->id = -1;

   pipe_resource_reference(&view->pipe.texture, texture);

   tic = &view->tic[0];

   desc = util_format_description(view->pipe.format);

   /* TIC[0] */

   tic[0] = nvc0_format_table[view->pipe.format].tic;

   tex_int = FALSE; /* XXX: integer textures */

   swz[0] = nv50_tic_swizzle(tic[0], view->pipe.swizzle_r, tex_int);
   swz[1] = nv50_tic_swizzle(tic[0], view->pipe.swizzle_g, tex_int);
   swz[2] = nv50_tic_swizzle(tic[0], view->pipe.swizzle_b, tex_int);
   swz[3] = nv50_tic_swizzle(tic[0], view->pipe.swizzle_a, tex_int);
   tic[0] = (tic[0] & ~NV50_TIC_0_SWIZZLE__MASK) |
      (swz[0] << NV50_TIC_0_MAPR__SHIFT) |
      (swz[1] << NV50_TIC_0_MAPG__SHIFT) |
      (swz[2] << NV50_TIC_0_MAPB__SHIFT) |
      (swz[3] << NV50_TIC_0_MAPA__SHIFT);

   tic[1] = /* mt->base.bo->offset; */ 0;
   tic[2] = /* mt->base.bo->offset >> 32 */ 0;

   tic[2] |= 0x10001000 | NV50_TIC_2_NO_BORDER;

   if (desc->colorspace == UTIL_FORMAT_COLORSPACE_SRGB)
      tic[2] |= NV50_TIC_2_COLORSPACE_SRGB;

   if (mt->base.base.target != PIPE_TEXTURE_RECT)
      tic[2] |= NV50_TIC_2_NORMALIZED_COORDS;

   tic[2] |=
      ((mt->base.bo->tile_mode & 0x0f0) << (22 - 4)) |
      ((mt->base.bo->tile_mode & 0xf00) << (25 - 8));

   depth = MAX2(mt->base.base.array_size, mt->base.base.depth0);

   if (mt->base.base.target == PIPE_TEXTURE_1D_ARRAY ||
       mt->base.base.target == PIPE_TEXTURE_2D_ARRAY) {
      /* there doesn't seem to be a base layer field in TIC */
      tic[1] = view->pipe.u.tex.first_layer * mt->layer_stride;
      depth = view->pipe.u.tex.last_layer - view->pipe.u.tex.first_layer + 1;
   }

   switch (mt->base.base.target) {
   case PIPE_TEXTURE_1D:
      tic[2] |= NV50_TIC_2_TARGET_1D;
      break;
   case PIPE_TEXTURE_2D:
      tic[2] |= NV50_TIC_2_TARGET_2D;
      break;
   case PIPE_TEXTURE_RECT:
      tic[2] |= NV50_TIC_2_TARGET_RECT;
      break;
   case PIPE_TEXTURE_3D:
      tic[2] |= NV50_TIC_2_TARGET_3D;
      break;
   case PIPE_TEXTURE_CUBE:
      depth /= 6;
      if (depth > 1)
         tic[2] |= NV50_TIC_2_TARGET_CUBE_ARRAY;
      else
         tic[2] |= NV50_TIC_2_TARGET_CUBE;
      break;
   case PIPE_TEXTURE_1D_ARRAY:
      tic[2] |= NV50_TIC_2_TARGET_1D_ARRAY;
      break;
   case PIPE_TEXTURE_2D_ARRAY:
      tic[2] |= NV50_TIC_2_TARGET_2D_ARRAY;
      break;
   case PIPE_BUFFER:
      tic[2] |= NV50_TIC_2_TARGET_BUFFER | NV50_TIC_2_LINEAR;
      break;
   default:
      NOUVEAU_ERR("invalid texture target: %d\n", mt->base.base.target);
      return FALSE;
   }

   if (mt->base.base.target == PIPE_BUFFER)
      tic[3] = mt->base.base.width0;
   else
      tic[3] = 0x00300000;

   tic[4] = (1 << 31) | mt->base.base.width0;

   tic[5] = mt->base.base.height0 & 0xffff;
   tic[5] |= depth << 16;
   tic[5] |= mt->base.base.last_level << 28;

   tic[6] = 0x03000000;

   tic[7] = (view->pipe.u.tex.last_level << 4) | view->pipe.u.tex.first_level;

   return &view->pipe;
}

static boolean
nvc0_validate_tic(struct nvc0_context *nvc0, int s)
{
   struct nouveau_channel *chan = nvc0->screen->base.channel;
   struct nouveau_bo *txc = nvc0->screen->txc;
   unsigned i;
   boolean need_flush = FALSE;

   for (i = 0; i < nvc0->num_textures[s]; ++i) {
      struct nv50_tic_entry *tic = nv50_tic_entry(nvc0->textures[s][i]);
      struct nv04_resource *res;

      if (!tic) {
         BEGIN_RING(chan, RING_3D(BIND_TIC(s)), 1);
         OUT_RING  (chan, (i << 1) | 0);
         continue;
      }
      res = &nvc0_miptree(tic->pipe.texture)->base;

      if (tic->id < 0) {
         uint32_t offset = tic->tic[1];

         tic->id = nvc0_screen_tic_alloc(nvc0->screen, tic);

         MARK_RING (chan, 9 + 8, 4);
         BEGIN_RING(chan, RING_MF(OFFSET_OUT_HIGH), 2);
         OUT_RELOCh(chan, txc, tic->id * 32, NOUVEAU_BO_VRAM | NOUVEAU_BO_WR);
         OUT_RELOCl(chan, txc, tic->id * 32, NOUVEAU_BO_VRAM | NOUVEAU_BO_WR);
         BEGIN_RING(chan, RING_MF(LINE_LENGTH_IN), 2);
         OUT_RING  (chan, 32);
         OUT_RING  (chan, 1);
         BEGIN_RING(chan, RING_MF(EXEC), 1);
         OUT_RING  (chan, 0x100111);
         BEGIN_RING_NI(chan, RING_MF(DATA), 8);
         OUT_RING  (chan, tic->tic[0]);
         OUT_RELOCl(chan, res->bo, offset, NOUVEAU_BO_VRAM | NOUVEAU_BO_RD);
         OUT_RELOC (chan, res->bo, offset, NOUVEAU_BO_VRAM | NOUVEAU_BO_RD |
                    NOUVEAU_BO_HIGH | NOUVEAU_BO_OR, tic->tic[2], tic->tic[2]);
         OUT_RINGp (chan, &tic->tic[3], 5);

         need_flush = TRUE;
      } else
      if (res->status & NOUVEAU_BUFFER_STATUS_GPU_WRITING) {
         BEGIN_RING(chan, RING_3D(TEX_CACHE_CTL), 1);
         OUT_RING  (chan, (tic->id << 4) | 1);
      }
      nvc0->screen->tic.lock[tic->id / 32] |= 1 << (tic->id % 32);

      res->status &= ~NOUVEAU_BUFFER_STATUS_GPU_WRITING;
      res->status |=  NOUVEAU_BUFFER_STATUS_GPU_READING;

      nvc0_bufctx_add_resident(nvc0, NVC0_BUFCTX_TEXTURES, res,
                               NOUVEAU_BO_VRAM | NOUVEAU_BO_RD);

      BEGIN_RING(chan, RING_3D(BIND_TIC(s)), 1);
      OUT_RING  (chan, (tic->id << 9) | (i << 1) | 1);
   }
   for (; i < nvc0->state.num_textures[s]; ++i) {
      BEGIN_RING(chan, RING_3D(BIND_TIC(s)), 1);
      OUT_RING  (chan, (i << 1) | 0);
   }
   nvc0->state.num_textures[s] = nvc0->num_textures[s];

   return need_flush;
}

void nvc0_validate_textures(struct nvc0_context *nvc0)
{
   boolean need_flush;

   need_flush  = nvc0_validate_tic(nvc0, 0);
   need_flush |= nvc0_validate_tic(nvc0, 4);

   if (need_flush) {
      BEGIN_RING(nvc0->screen->base.channel, RING_3D(TIC_FLUSH), 1);
      OUT_RING  (nvc0->screen->base.channel, 0);
   }
}

static boolean
nvc0_validate_tsc(struct nvc0_context *nvc0, int s)
{
   struct nouveau_channel *chan = nvc0->screen->base.channel;
   unsigned i;
   boolean need_flush = FALSE;

   for (i = 0; i < nvc0->num_samplers[s]; ++i) {
      struct nv50_tsc_entry *tsc = nv50_tsc_entry(nvc0->samplers[s][i]);

      if (!tsc) {
         BEGIN_RING(chan, RING_3D(BIND_TSC(s)), 1);
         OUT_RING  (chan, (i << 4) | 0);
         continue;
      }
      if (tsc->id < 0) {
         tsc->id = nvc0_screen_tsc_alloc(nvc0->screen, tsc);

         nvc0_m2mf_push_linear(&nvc0->base, nvc0->screen->txc,
                               65536 + tsc->id * 32, NOUVEAU_BO_VRAM,
                               32, tsc->tsc);
         need_flush = TRUE;
      }
      nvc0->screen->tsc.lock[tsc->id / 32] |= 1 << (tsc->id % 32);

      BEGIN_RING(chan, RING_3D(BIND_TSC(s)), 1);
      OUT_RING  (chan, (tsc->id << 12) | (i << 4) | 1);
   }
   for (; i < nvc0->state.num_samplers[s]; ++i) {
      BEGIN_RING(chan, RING_3D(BIND_TSC(s)), 1);
      OUT_RING  (chan, (i << 4) | 0);
   }
   nvc0->state.num_samplers[s] = nvc0->num_samplers[s];

   return need_flush;
}

void nvc0_validate_samplers(struct nvc0_context *nvc0)
{
   boolean need_flush;

   need_flush  = nvc0_validate_tsc(nvc0, 0);
   need_flush |= nvc0_validate_tsc(nvc0, 4);

   if (need_flush) {
      BEGIN_RING(nvc0->screen->base.channel, RING_3D(TSC_FLUSH), 1);
      OUT_RING  (nvc0->screen->base.channel, 0);
   }
}
