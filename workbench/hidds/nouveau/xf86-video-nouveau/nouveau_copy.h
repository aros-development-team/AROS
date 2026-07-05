#ifndef __NVDDX_COPY_H__
#define __NVDDX_COPY_H__

#include "nv_include.h"
#include "nouveau_local.h"

Bool nouveau_copy_init(ScreenPtr);
void nouveau_copy_fini(ScreenPtr);

Bool nouveau_copy85b5_init(NVPtr);
Bool nouveau_copy90b5_init(NVPtr);
Bool nouveau_copya0b5_init(NVPtr);
Bool nouveau_copya0b5_rect(struct nouveau_pushbuf *, struct nouveau_object *,
			   int, int, int, struct nouveau_bo *, uint32_t, int,
			   int, int, int, int, struct nouveau_bo *, uint32_t,
			   int, int, int, int, int);

#endif
