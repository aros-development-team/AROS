#ifndef __NV50_SCREEN_H__
#define __NV50_SCREEN_H__

#include "nouveau/nouveau_screen.h"

struct nv50_context;

struct nv50_screen {
	struct nouveau_screen base;

	struct nouveau_winsys *nvws;

	struct nv50_context *cur_ctx;

	struct nouveau_grobj *tesla;
	struct nouveau_grobj *eng2d;
	struct nouveau_grobj *m2mf;
	struct nouveau_notifier *sync;

	struct nouveau_bo *constbuf_misc[1];
	struct nouveau_bo *constbuf_parm[PIPE_SHADER_TYPES];

	struct nouveau_resource *immd_heap;

	struct pipe_resource *strm_vbuf[16];

	struct nouveau_bo *tic;
	struct nouveau_bo *tsc;

	boolean force_push;
};

static INLINE struct nv50_screen *
nv50_screen(struct pipe_screen *screen)
{
	return (struct nv50_screen *)screen;
}

extern void nv50_screen_relocs(struct nv50_screen *);

#endif
