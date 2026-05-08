/* SPDX-License-Identifier: MIT */
#ifndef __NVKM_SEC2_H__
#define __NVKM_SEC2_H__
#include <core/engine.h>

struct nvkm_sec2 {
	struct nvkm_engine engine;
	u32 addr;

	struct nvkm_falcon *falcon;
	struct nvkm_msgqueue *queue;
#if !defined(__AROS__)
	struct work_struct work;
#endif
};

int gp102_sec2_new(struct nvkm_device *, int, struct nvkm_sec2 **);
int tu102_sec2_new(struct nvkm_device *, int, struct nvkm_sec2 **);
#endif
