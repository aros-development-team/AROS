/* SPDX-License-Identifier: MIT
 *
 * Copyright (c) 2025, NVIDIA CORPORATION. All rights reserved.
 */
#include "core.h"
#include "disp.h"
#include "head.h"
#include "wndw.h"

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
 * Window ownership: never taken away from a head that is currently driving
 * an output. The firmware lights head 0 with window 0 only (two phywins), so
 * window 1 starts unowned. An unowned window is given to its live natural
 * head together with as many free physical windows as the head has tiles,
 * in the same update - the vendor binds all windows at init, live head or
 * not (nvkms-evo3.c EvoInitWindowMapping3), and every window of a head
 * needs phywins to match its tiles (nvkms-evo4.c EvoSetMultiTileConfigCA).
 * The owner alone failed the update (Xid 56, 0x12). Default on (confirmed
 * on the Titan 2026-10-24); NOUVEAU_WNDW_LIVE_OWNER=0 restores the old rule.
 */
int nouveau_wndw_live_owner = 1;
/*
 * NOUVEAU_WNDW_YUV (default 1): the windows this driver sets up may scan out
 * 4:2:0 video (planar I420 and semi-planar NV12, what nvkms uses) - the
 * format usage bounds carry YUV_PLANAR420 and YUV_SEMI_PLANAR420; YUV_PACKED422 only with
 * NOUVEAU_WNDW_PACKED422 (a packed window raised an UPDATE exception on the
 * GB207, 2026-10-25). 0 = RGB only.
 */
int nouveau_wndw_yuv = 1;
extern int nouveau_wndw_packed422;

static int
coreca7d_wndw_owner(struct nv50_core *core)
{
	struct nvif_push *push = &core->chan.push;
	const u32 windows = 8;
	u32 active, tiles, phywins;
	int ret, i;

	coreca7d_owned(core, &active, &tiles, &phywins);

	if ((ret = PUSH_WAIT(push, windows * 4)))
		return ret;

	for (i = 0; i < windows; i++) {
		u32 owner = NVVAL_GET(coreca7d_armed(core, NVCA7D_WINDOW_SET_CONTROL(i)),
				      NVCA7D, WINDOW_SET_CONTROL, OWNER);
		u32 head = i >> 1;

		if (owner == head)
			continue;
		if (owner < 8 && (active & BIT(owner)))
			continue;
		if (active & BIT(head)) {
			/*
			 * Every window of a head needs as many physical windows
			 * as the head has tiles (nvkms-evo4.c EvoSetMultiTileConfigCA
			 * asserts it); the firmware runs 1080p on two tiles.
			 */
			u32 htiles = coreca7d_armed(core, NVCA7D_HEAD_SET_TILE_MASK(head)) & 0xff;
			u32 need = hweight32(htiles) ? hweight32(htiles) : 1;
			u32 pw = coreca7d_armed(core, NVCA7D_WINDOW_SET_PHYSICAL(i)) & 0xff, t;

			if (!nouveau_wndw_live_owner)
				continue;
			if (hweight32(pw) != need) {
				pw = 0;
				for (t = 0; t < 8 && hweight32(pw) < need; t++) {
					if (!(phywins & BIT(t)))
						pw |= BIT(t);
				}
				if (hweight32(pw) != need)
					continue;
				phywins |= pw;
			}
			printk(KERN_NOTICE "[nouveau] wndw %d: owner %x -> live head %d (tiles %02x), phywins %02x\n",
			       i, owner, head, htiles, pw);
			PUSH_MTHD(push, NVCA7D, WINDOW_SET_PHYSICAL(i), pw);
		}
		PUSH_MTHD(push, NVCA7D, WINDOW_SET_CONTROL(i),
			  NVDEF(NVCA7D, WINDOW_SET_CONTROL, OWNER, HEAD(head)));
	}

	return 0;
}

static u32
coreca7d_assembly(struct nv50_core *core, u32 mthd)
{
	return nvif_rd32(&core->chan.base.user, mthd);
}

/* What the core channel holds for a window and its head (diagnostics) */
void
coreca7d_wndw_log(struct nv50_core *core, int id, const char *when)
{
	u32 ctrl = coreca7d_armed(core, NVCA7D_WINDOW_SET_CONTROL(id));
	u32 head = NVVAL_GET(ctrl, NVCA7D, WINDOW_SET_CONTROL, OWNER);

	printk(KERN_NOTICE "[nouveau] wndw %d %s: armed control %08x physical %08x format-bounds %08x (assembly %08x) usage-bounds %08x; head %u tiles %08x usage-bounds %08x\n",
	       id, when, ctrl, coreca7d_armed(core, NVCA7D_WINDOW_SET_PHYSICAL(id)),
	       coreca7d_armed(core, NVCA7D_WINDOW_SET_WINDOW_FORMAT_USAGE_BOUNDS(id)),
	       coreca7d_assembly(core, NVCA7D_WINDOW_SET_WINDOW_FORMAT_USAGE_BOUNDS(id)),
	       coreca7d_armed(core, NVCA7D_WINDOW_SET_WINDOW_USAGE_BOUNDS(id)),
	       head, head < 4 ? coreca7d_armed(core, NVCA7D_HEAD_SET_TILE_MASK(head)) : 0,
	       head < 4 ? coreca7d_armed(core, NVCA7D_HEAD_SET_HEAD_USAGE_BOUNDS(head)) : 0);
}

/* The usage bounds this driver gives a window (5 words) */
static void
coreca7d_wndw_bounds_push(struct nvif_push *push, int i, u32 yuv)
{
	PUSH_MTHD(push, NVCA7D, WINDOW_SET_WINDOW_FORMAT_USAGE_BOUNDS(i),
		  NVDEF(NVCA7D, WINDOW_SET_WINDOW_FORMAT_USAGE_BOUNDS, RGB_PACKED1BPP, TRUE) |
		  NVDEF(NVCA7D, WINDOW_SET_WINDOW_FORMAT_USAGE_BOUNDS, RGB_PACKED2BPP, TRUE) |
		  NVDEF(NVCA7D, WINDOW_SET_WINDOW_FORMAT_USAGE_BOUNDS, RGB_PACKED4BPP, TRUE) |
		  NVDEF(NVCA7D, WINDOW_SET_WINDOW_FORMAT_USAGE_BOUNDS, RGB_PACKED8BPP, TRUE) |
		  ((yuv & NOUVEAU_WNDW_YUV_I420) ?
		   NVDEF(NVCA7D, WINDOW_SET_WINDOW_FORMAT_USAGE_BOUNDS, YUV_PLANAR420, TRUE) : 0) |
		  ((yuv & NOUVEAU_WNDW_YUV_NV12) ?
		   NVDEF(NVCA7D, WINDOW_SET_WINDOW_FORMAT_USAGE_BOUNDS, YUV_SEMI_PLANAR420, TRUE) : 0) |
		  ((yuv & NOUVEAU_WNDW_YUV_PACKED422) ?
		   NVDEF(NVCA7D, WINDOW_SET_WINDOW_FORMAT_USAGE_BOUNDS, YUV_PACKED422, TRUE) : 0),

				WINDOW_SET_WINDOW_ROTATED_FORMAT_USAGE_BOUNDS(i), 0x00000000);

	PUSH_MTHD(push, NVCA7D, WINDOW_SET_WINDOW_USAGE_BOUNDS(i),
		  NVVAL(NVCA7D, WINDOW_SET_WINDOW_USAGE_BOUNDS, MAX_PIXELS_FETCHED_PER_LINE, 0x7fff) |
		  NVDEF(NVCA7D, WINDOW_SET_WINDOW_USAGE_BOUNDS, ILUT_ALLOWED, TRUE) |
		  NVDEF(NVCA7D, WINDOW_SET_WINDOW_USAGE_BOUNDS, INPUT_SCALER_TAPS, TAPS_2) |
		  NVDEF(NVCA7D, WINDOW_SET_WINDOW_USAGE_BOUNDS, UPSCALING_ALLOWED, FALSE));
}

/*
 * Re-state a window's usage bounds in the core channel (nvkms sends the
 * bounds in the core update of every modeset and of a flip that needs
 * more; the init-time ones did not arm on the GB207, see 2026-10-25).
 */
int
coreca7d_wndw_bounds(struct nv50_core *core, int id)
{
	struct nvif_push *push = &core->chan.push;
	int ret;

	if ((ret = PUSH_WAIT(push, 5)))
		return ret;
	coreca7d_wndw_bounds_push(push, id, nouveau_wndw_yuv_mask());
	return 0;
}

static int
coreca7d_init(struct nv50_core *core)
{
	struct nvif_push *push = &core->chan.push;
	const u32 windows = 8, heads = 4;
	u32 active, tiles, phywins, yuv;
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
	/* Only known after the IMP query above read the capabilities (2026-10-25: computing
	 * this at function entry sent RGB-only bounds, 0x0f, and every YUV flip failed). */
	yuv = nouveau_wndw_yuv_mask();
	for (i = 0; i < windows; i++)
		printk(KERN_NOTICE "[nouveau] wndw %d: control %08x phywin %08x\n", i,
		       coreca7d_armed(core, NVCA7D_WINDOW_SET_CONTROL(i)),
		       coreca7d_armed(core, NVCA7D_WINDOW_SET_PHYSICAL(i)));
	for (i = 0; i < heads; i++)
		printk(KERN_NOTICE "[nouveau] head %d: tiles %08x live %d\n", i,
		       coreca7d_armed(core, NVCA7D_HEAD_SET_TILE_MASK(i)), !!(active & BIT(i)));

	ret = PUSH_WAIT(push, windows * 6 + heads * 4);
	if (ret)
		return ret;

	for (i = 0; i < windows; i++) {
		u32 owner = NVVAL_GET(coreca7d_armed(core, NVCA7D_WINDOW_SET_CONTROL(i)),
				      NVCA7D, WINDOW_SET_CONTROL, OWNER);

		/* A window in use by a live head keeps its firmware bounds for now. */
		if (owner < 4 && (active & BIT(owner)))
			continue;

		coreca7d_wndw_bounds_push(push, i, yuv);

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
