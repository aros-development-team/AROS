#ifndef __NV50_SCREEN_H__
#define __NV50_SCREEN_H__

#define NOUVEAU_NVC0
#include "nouveau/nouveau_screen.h"
#include "nouveau/nouveau_fence.h"
#include "nouveau/nouveau_mm.h"
#undef NOUVEAU_NVC0
#include "nv50_winsys.h"
#include "nv50_stateobj.h"

#define NV50_TIC_MAX_ENTRIES 2048
#define NV50_TSC_MAX_ENTRIES 2048

struct nv50_context;

#define NV50_CODE_BO_SIZE_LOG2 19

#define NV50_SCRATCH_SIZE (2 << 20)
#define NV50_SCRATCH_NR_BUFFERS 2

#define NV50_SCREEN_RESIDENT_BO_COUNT 5

struct nv50_screen {
   struct nouveau_screen base;
   struct nouveau_winsys *nvws;

   struct nv50_context *cur_ctx;

   struct nouveau_bo *code;
   struct nouveau_bo *uniforms;
   struct nouveau_bo *txc; /* TIC (offset 0) and TSC (65536) */
   struct nouveau_bo *stack_bo;
   struct nouveau_bo *tls_bo;

   uint64_t tls_size;

   struct nouveau_resource *vp_code_heap;
   struct nouveau_resource *gp_code_heap;
   struct nouveau_resource *fp_code_heap;

   struct {
      void **entries;
      int next;
      uint32_t lock[NV50_TIC_MAX_ENTRIES / 32];
   } tic;
   
   struct {
      void **entries;
      int next;
      uint32_t lock[NV50_TSC_MAX_ENTRIES / 32];
   } tsc;

   struct {
      uint32_t *map;
      struct nouveau_bo *bo;
   } fence;

   struct nouveau_notifier *sync;

   struct nouveau_mman *mm_VRAM_fe0;

   struct nouveau_grobj *tesla;
   struct nouveau_grobj *eng2d;
   struct nouveau_grobj *m2mf;
};

static INLINE struct nv50_screen *
nv50_screen(struct pipe_screen *screen)
{
   return (struct nv50_screen *)screen;
}

void nv50_screen_make_buffers_resident(struct nv50_screen *);

int nv50_screen_tic_alloc(struct nv50_screen *, void *);
int nv50_screen_tsc_alloc(struct nv50_screen *, void *);

static INLINE void
nv50_resource_fence(struct nv04_resource *res, uint32_t flags)
{
   struct nv50_screen *screen = nv50_screen(res->base.screen);

   if (res->mm) {
      nouveau_fence_ref(screen->base.fence.current, &res->fence);

      if (flags & NOUVEAU_BO_WR)
         nouveau_fence_ref(screen->base.fence.current, &res->fence_wr);
   }
}

static INLINE void
nv50_resource_validate(struct nv04_resource *res, uint32_t flags)
{
   struct nv50_screen *screen = nv50_screen(res->base.screen);

   if (likely(res->bo)) {
      nouveau_bo_validate(screen->base.channel, res->bo, flags);

      if (flags & NOUVEAU_BO_WR)
         res->status |= NOUVEAU_BUFFER_STATUS_GPU_WRITING;
      if (flags & NOUVEAU_BO_RD)
         res->status |= NOUVEAU_BUFFER_STATUS_GPU_READING;

      nv50_resource_fence(res, flags);
   }
}

struct nv50_format {
   uint32_t rt;
   uint32_t tic;
   uint32_t vtx;
   uint32_t usage;
};

extern const struct nv50_format nv50_format_table[];

static INLINE void
nv50_screen_tic_unlock(struct nv50_screen *screen, struct nv50_tic_entry *tic)
{
   if (tic->id >= 0)
      screen->tic.lock[tic->id / 32] &= ~(1 << (tic->id % 32));
}

static INLINE void
nv50_screen_tsc_unlock(struct nv50_screen *screen, struct nv50_tsc_entry *tsc)
{
   if (tsc->id >= 0)
      screen->tsc.lock[tsc->id / 32] &= ~(1 << (tsc->id % 32));
}

static INLINE void
nv50_screen_tic_free(struct nv50_screen *screen, struct nv50_tic_entry *tic)
{
   if (tic->id >= 0) {
      screen->tic.entries[tic->id] = NULL;
      screen->tic.lock[tic->id / 32] &= ~(1 << (tic->id % 32));
   }
}

static INLINE void
nv50_screen_tsc_free(struct nv50_screen *screen, struct nv50_tsc_entry *tsc)
{
   if (tsc->id >= 0) {
      screen->tsc.entries[tsc->id] = NULL;
      screen->tsc.lock[tsc->id / 32] &= ~(1 << (tsc->id % 32));
   }
}

#endif
