#ifndef __NVC0_SCREEN_H__
#define __NVC0_SCREEN_H__

#define NOUVEAU_NVC0
#include "nouveau/nouveau_screen.h"
#include "nouveau/nouveau_mm.h"
#undef NOUVEAU_NVC0
#include "nvc0_winsys.h"
#include "nvc0_stateobj.h"

#define NVC0_TIC_MAX_ENTRIES 2048
#define NVC0_TSC_MAX_ENTRIES 2048

struct nvc0_context;

#define NVC0_SCRATCH_SIZE (2 << 20)
#define NVC0_SCRATCH_NR_BUFFERS 2

#define NVC0_SCREEN_RESIDENT_BO_COUNT 5

struct nvc0_screen {
   struct nouveau_screen base;
   struct nouveau_winsys *nvws;

   struct nvc0_context *cur_ctx;

   struct nouveau_bo *text;
   struct nouveau_bo *uniforms;
   struct nouveau_bo *tls;
   struct nouveau_bo *txc; /* TIC (offset 0) and TSC (65536) */
   struct nouveau_bo *vfetch_cache;

   uint64_t tls_size;

   struct nouveau_resource *text_heap;

   struct {
      struct nouveau_bo *bo[NVC0_SCRATCH_NR_BUFFERS];
      uint8_t *buf;
      int index;
      uint32_t offset;
   } scratch;

   struct {
      void **entries;
      int next;
      uint32_t lock[NVC0_TIC_MAX_ENTRIES / 32];
   } tic;
   
   struct {
      void **entries;
      int next;
      uint32_t lock[NVC0_TSC_MAX_ENTRIES / 32];
   } tsc;

   struct {
      struct nouveau_bo *bo;
      uint32_t *map;
   } fence;

   struct nouveau_mman *mm_VRAM_fe0;

   struct nouveau_grobj *fermi;
   struct nouveau_grobj *eng2d;
   struct nouveau_grobj *m2mf;
};

static INLINE struct nvc0_screen *
nvc0_screen(struct pipe_screen *screen)
{
   return (struct nvc0_screen *)screen;
}

void nvc0_screen_make_buffers_resident(struct nvc0_screen *);

int nvc0_screen_tic_alloc(struct nvc0_screen *, void *);
int nvc0_screen_tsc_alloc(struct nvc0_screen *, void *);

static INLINE void
nvc0_resource_fence(struct nv04_resource *res, uint32_t flags)
{
   struct nvc0_screen *screen = nvc0_screen(res->base.screen);

   if (res->mm) {
      nouveau_fence_ref(screen->base.fence.current, &res->fence);

      if (flags & NOUVEAU_BO_WR)
         nouveau_fence_ref(screen->base.fence.current, &res->fence_wr);
   }
}

static INLINE void
nvc0_resource_validate(struct nv04_resource *res, uint32_t flags)
{
   struct nvc0_screen *screen = nvc0_screen(res->base.screen);

   if (likely(res->bo)) {
      nouveau_bo_validate(screen->base.channel, res->bo, flags);

      if (flags & NOUVEAU_BO_WR)
         res->status |= NOUVEAU_BUFFER_STATUS_GPU_WRITING;
      if (flags & NOUVEAU_BO_RD)
         res->status |= NOUVEAU_BUFFER_STATUS_GPU_READING;

      nvc0_resource_fence(res, flags);
   }
}

struct nvc0_format {
   uint32_t rt;
   uint32_t tic;
   uint32_t vtx;
   uint32_t usage;
};

extern const struct nvc0_format nvc0_format_table[];

static INLINE void
nvc0_screen_tic_unlock(struct nvc0_screen *screen, struct nv50_tic_entry *tic)
{
   if (tic->id >= 0)
      screen->tic.lock[tic->id / 32] &= ~(1 << (tic->id % 32));
}

static INLINE void
nvc0_screen_tsc_unlock(struct nvc0_screen *screen, struct nv50_tsc_entry *tsc)
{
   if (tsc->id >= 0)
      screen->tsc.lock[tsc->id / 32] &= ~(1 << (tsc->id % 32));
}

static INLINE void
nvc0_screen_tic_free(struct nvc0_screen *screen, struct nv50_tic_entry *tic)
{
   if (tic->id >= 0) {
      screen->tic.entries[tic->id] = NULL;
      screen->tic.lock[tic->id / 32] &= ~(1 << (tic->id % 32));
   }
}

static INLINE void
nvc0_screen_tsc_free(struct nvc0_screen *screen, struct nv50_tsc_entry *tsc)
{
   if (tsc->id >= 0) {
      screen->tsc.entries[tsc->id] = NULL;
      screen->tsc.lock[tsc->id / 32] &= ~(1 << (tsc->id % 32));
   }
}

#endif
