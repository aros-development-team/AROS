/* SPDX-License-Identifier: MIT
 *
 * Copyright (c) 2025, NVIDIA CORPORATION. All rights reserved.
 */
#include "core.h"
#include "disp.h"
#include "head.h"

#include <nvif/class.h>
#include <nvif/printf.h>
#include <nvif/pushc97b.h>

#include <nvhw/class/clca7d.h>

#include <nouveau_bo.h>
#include <nvkm/engine/disp.h>

static int
coreca7d_update(struct nv50_core *core, u32 *interlock, bool ntfy)
{
	u32 ntfy_target;
	const u64 ntfy_addr = nv50_disp_sync_addr(core->disp, NV50_DISP_CORE_NTFY, &ntfy_target);
	const u32 ntfy_hi = upper_32_bits(ntfy_addr);
	const u32 ntfy_lo = lower_32_bits(ntfy_addr);
	struct nvif_push *push = &core->chan.push;
	int ret;

	ret = PUSH_WAIT(push, 5 + (ntfy ? 5 + 2 : 0));
	if (ret)
		return ret;

	if (ntfy) {
		PUSH_MTHD(push, NVCA7D, SET_SURFACE_ADDRESS_HI_NOTIFIER, ntfy_hi,

					SET_SURFACE_ADDRESS_LO_NOTIFIER,
			  NVVAL(NVCA7D, SET_SURFACE_ADDRESS_LO_NOTIFIER, ADDRESS_LO, ntfy_lo >> 4) |
			  NVVAL(NVCA7D, SET_SURFACE_ADDRESS_LO_NOTIFIER, TARGET, ntfy_target) |
			  NVDEF(NVCA7D, SET_SURFACE_ADDRESS_LO_NOTIFIER, ENABLE, ENABLE));

		PUSH_MTHD(push, NVCA7D, SET_NOTIFIER_CONTROL,
			  NVDEF(NVCA7D, SET_NOTIFIER_CONTROL, MODE, WRITE) |
			  NVDEF(NVCA7D, SET_NOTIFIER_CONTROL, NOTIFY, ENABLE));
	}

	PUSH_MTHD(push, NVCA7D, SET_INTERLOCK_FLAGS, interlock[NV50_DISP_INTERLOCK_CURS],
				SET_WINDOW_INTERLOCK_FLAGS, interlock[NV50_DISP_INTERLOCK_WNDW]);

	PUSH_MTHD(push, NVCA7D, UPDATE,
		  NVDEF(NVCA7D, UPDATE, RELEASE_ELV, TRUE) |
		  NVDEF(NVCA7D, UPDATE, SPECIAL_HANDLING, NONE) |
		  NVDEF(NVCA7D, UPDATE, INHIBIT_INTERRUPTS, FALSE));

	if (ntfy) {
		PUSH_MTHD(push, NVCA7D, SET_NOTIFIER_CONTROL,
			  NVDEF(NVCA7D, SET_NOTIFIER_CONTROL, NOTIFY, DISABLE));
	}

	return PUSH_KICK(push);
}

/* The core channel's ARMed state cache follows the user area. */
#define CORECA7D_ARMED 0x8000

u32
coreca7d_armed(struct nv50_core *core, u32 mthd)
{
	return nvif_rd32(&core->chan.base.user, CORECA7D_ARMED + mthd);
}

/*
 * Tiles and physical windows are owned by heads/windows; the firmware may
 * have left a head scanning out with several of each, and reassigning an
 * owned tile or phywin, or giving one to two owners, fails the UPDATE with
 * an invalid-state exception. Everything not owned by a live head is
 * released here and handed out again when a head is enabled.
 */
void
coreca7d_owned(struct nv50_core *core, u32 *pactive, u32 *ptiles, u32 *pphywins)
{
	u32 active = 0, tiles = 0, phywins = 0;
	int i;

	for (i = 0; i < 4; i++)
		active |= NVVAL_GET(coreca7d_armed(core, NVCA7D_SOR_SET_CONTROL(i)),
				    NVCA7D, SOR_SET_CONTROL, OWNER_MASK);

	for (i = 0; i < 4; i++)
		tiles |= coreca7d_armed(core, NVCA7D_HEAD_SET_TILE_MASK(i)) & 0xff;

	for (i = 0; i < 8; i++)
		phywins |= coreca7d_armed(core, NVCA7D_WINDOW_SET_PHYSICAL(i)) & 0xff;

	*pactive = active;
	*ptiles = tiles;
	*pphywins = phywins;
}


/*
 * Window ownership follows the same rule as tiles: never moved to or from
 * a head that is currently driving an output.
 */
static int
coreca7d_wndw_owner(struct nv50_core *core)
{
	struct nvif_push *push = &core->chan.push;
	const u32 windows = 8;
	u32 active, tiles, phywins;
	int ret, i;

	coreca7d_owned(core, &active, &tiles, &phywins);

	if ((ret = PUSH_WAIT(push, windows * 2)))
		return ret;

	for (i = 0; i < windows; i++) {
		u32 owner = NVVAL_GET(coreca7d_armed(core, NVCA7D_WINDOW_SET_CONTROL(i)),
				      NVCA7D, WINDOW_SET_CONTROL, OWNER);
		u32 head = i >> 1;

		if (owner == head)
			continue;
		if ((owner < 4 && (active & BIT(owner))) || (active & BIT(head)))
			continue;
		PUSH_MTHD(push, NVCA7D, WINDOW_SET_CONTROL(i),
			  NVDEF(NVCA7D, WINDOW_SET_CONTROL, OWNER, HEAD(head)));
	}

	return 0;
}

static int
coreca7d_init(struct nv50_core *core)
{
	struct nvif_push *push = &core->chan.push;
	const u32 windows = 8, heads = 4;
	u32 active, tiles, phywins;
	int ret, i;

	coreca7d_owned(core, &active, &tiles, &phywins);
	for (i = 0; i < heads; i++)
		NVIF_DEBUG(&core->chan.base.user, "head %d: tiles %08x sor-active %d", i,
			   coreca7d_armed(core, NVCA7D_HEAD_SET_TILE_MASK(i)), !!(active & BIT(i)));

	/*
	 * RM wants an IMP query for whatever is being displayed before it
	 * will drive updates; describe the head(s) the firmware lit.
	 */
	{
		struct nvkm_disp_imp_head ih[4];
		struct nvkm_disp_imp_result res;
		int n = 0;

		for (i = 0; i < heads; i++) {
			u32 rs, bs, be, vb2, vp;

			if (!(active & BIT(i)))
				continue;
			rs = coreca7d_armed(core, NVCA7D_HEAD_SET_RASTER_SIZE(i));
			bs = coreca7d_armed(core, NVCA7D_HEAD_SET_RASTER_BLANK_START(i));
			be = coreca7d_armed(core, NVCA7D_HEAD_SET_RASTER_BLANK_END(i));
			vb2 = coreca7d_armed(core, 0x2074 + i * 0x800);
			vp = coreca7d_armed(core, NVCA7D_HEAD_SET_VIEWPORT_SIZE_OUT(i));
			ih[n].head = i;
			ih[n].pclk_khz = coreca7d_armed(core, NVCA7D_HEAD_SET_PIXEL_CLOCK_FREQUENCY(i)) / 1000;
			ih[n].raster_w = rs & 0xffff;
			ih[n].raster_h = rs >> 16;
			ih[n].blank_start_x = bs & 0x7fff;
			ih[n].blank_start_y = (bs >> 16) & 0x7fff;
			ih[n].blank_end_x = be & 0x7fff;
			ih[n].blank_end_y = (be >> 16) & 0x7fff;
			ih[n].vblank2_start = vb2 & 0x7fff;
			ih[n].vblank2_end = (vb2 >> 16) & 0x7fff;
			ih[n].view_w = vp & 0xffff;
			ih[n].view_h = vp >> 16;
			ih[n].tile_mask = coreca7d_armed(core, NVCA7D_HEAD_SET_TILE_MASK(i)) & 0xff;
			n++;
		}
		if (n)
			nv50_disp_imp(core->disp, ih, n, &res);
	}
	for (i = 0; i < windows; i++)
		NVIF_DEBUG(&core->chan.base.user, "wndw %d: owner %08x phywin %08x", i,
			   coreca7d_armed(core, NVCA7D_WINDOW_SET_CONTROL(i)),
			   coreca7d_armed(core, NVCA7D_WINDOW_SET_PHYSICAL(i)));

	ret = PUSH_WAIT(push, windows * 6 + heads * 4);
	if (ret)
		return ret;

	for (i = 0; i < windows; i++) {
		u32 owner = NVVAL_GET(coreca7d_armed(core, NVCA7D_WINDOW_SET_CONTROL(i)),
				      NVCA7D, WINDOW_SET_CONTROL, OWNER);

		/* A window in use by a live head keeps its firmware bounds for now. */
		if (owner < 4 && (active & BIT(owner)))
			continue;

		PUSH_MTHD(push, NVCA7D, WINDOW_SET_WINDOW_FORMAT_USAGE_BOUNDS(i),
			  NVDEF(NVCA7D, WINDOW_SET_WINDOW_FORMAT_USAGE_BOUNDS, RGB_PACKED1BPP, TRUE) |
			  NVDEF(NVCA7D, WINDOW_SET_WINDOW_FORMAT_USAGE_BOUNDS, RGB_PACKED2BPP, TRUE) |
			  NVDEF(NVCA7D, WINDOW_SET_WINDOW_FORMAT_USAGE_BOUNDS, RGB_PACKED4BPP, TRUE) |
			  NVDEF(NVCA7D, WINDOW_SET_WINDOW_FORMAT_USAGE_BOUNDS, RGB_PACKED8BPP, TRUE),

					WINDOW_SET_WINDOW_ROTATED_FORMAT_USAGE_BOUNDS(i), 0x00000000);

		PUSH_MTHD(push, NVCA7D, WINDOW_SET_WINDOW_USAGE_BOUNDS(i),
			  NVVAL(NVCA7D, WINDOW_SET_WINDOW_USAGE_BOUNDS, MAX_PIXELS_FETCHED_PER_LINE, 0x7fff) |
			  NVDEF(NVCA7D, WINDOW_SET_WINDOW_USAGE_BOUNDS, ILUT_ALLOWED, TRUE) |
			  NVDEF(NVCA7D, WINDOW_SET_WINDOW_USAGE_BOUNDS, INPUT_SCALER_TAPS, TAPS_2) |
			  NVDEF(NVCA7D, WINDOW_SET_WINDOW_USAGE_BOUNDS, UPSCALING_ALLOWED, FALSE));

		PUSH_MTHD(push, NVCA7D, WINDOW_SET_PHYSICAL(i), 0);
	}

	for (i = 0; i < heads; i++) {
		if (active & BIT(i))
			continue;

		PUSH_MTHD(push, NVCA7D, HEAD_SET_HEAD_USAGE_BOUNDS(i),
			  NVDEF(NVCA7D, HEAD_SET_HEAD_USAGE_BOUNDS, CURSOR, USAGE_W256_H256) |
			  NVDEF(NVCA7D, HEAD_SET_HEAD_USAGE_BOUNDS, OLUT_ALLOWED, TRUE) |
			  NVDEF(NVCA7D, HEAD_SET_HEAD_USAGE_BOUNDS, OUTPUT_SCALER_TAPS, TAPS_2) |
			  NVDEF(NVCA7D, HEAD_SET_HEAD_USAGE_BOUNDS, UPSCALING_ALLOWED, TRUE));

		PUSH_MTHD(push, NVCA7D, HEAD_SET_TILE_MASK(i), 0);
	}

	core->assign_windows = true;
	return PUSH_KICK(push);
}

static const struct nv50_core_func
coreca7d = {
	.init = coreca7d_init,
	.ntfy_init = corec37d_ntfy_init,
	.caps_init = corec37d_caps_init,
	.caps_class = GB202_DISP_CAPS,
	.ntfy_wait_done = corec37d_ntfy_wait_done,
	.update = coreca7d_update,
	.wndw.owner = coreca7d_wndw_owner,
	.head = &headca7d,
	.sor = &sorc37d,
#if IS_ENABLED(CONFIG_DEBUG_FS)
	.crc = &crcca7d,
#endif
};

int
coreca7d_new(struct nouveau_drm *drm, s32 oclass, struct nv50_core **pcore)
{
	return core507d_new_(&coreca7d, drm, oclass, pcore);
}
