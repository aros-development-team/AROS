/* SPDX-License-Identifier: MIT
 *
 * Copyright (c) 2025, NVIDIA CORPORATION. All rights reserved.
 */
#include "wndw.h"
#include "atom.h"

#include <nvif/pushc97b.h>

#include <nvhw/class/clca7e.h>

#include <nouveau_bo.h>

static int
wndwca7e_image_clr(struct nv50_wndw *wndw)
{
	struct nvif_push *push = &wndw->wndw.push;
	int ret;

	ret = PUSH_WAIT(push, 6 + 2);
	if (ret)
		return ret;

	PUSH_MTHD(push, NVCA7E, SET_PRESENT_CONTROL,
		  NVVAL(NVCA7E, SET_PRESENT_CONTROL, MIN_PRESENT_INTERVAL, 0) |
		  NVDEF(NVCA7E, SET_PRESENT_CONTROL, BEGIN_MODE, NON_TEARING));

	/* surface slots are eye + plane * 2 (nvkms EyeAndPlaneToCtxDmaIdx): planes of the left eye sit in 0, 2, 4 */
	PUSH_MTHD(push, NVCA7E, SET_SURFACE_ADDRESS_LO_ISO(0),
		  NVDEF(NVCA7E, SET_SURFACE_ADDRESS_LO_ISO, ENABLE, DISABLE));
	PUSH_MTHD(push, NVCA7E, SET_SURFACE_ADDRESS_LO_ISO(2),
		  NVDEF(NVCA7E, SET_SURFACE_ADDRESS_LO_ISO, ENABLE, DISABLE));
	PUSH_MTHD(push, NVCA7E, SET_SURFACE_ADDRESS_LO_ISO(4),
		  NVDEF(NVCA7E, SET_SURFACE_ADDRESS_LO_ISO, ENABLE, DISABLE));

	return 0;
}

/*
 * FMT (input format conversion) matrices, 12 coefficients C00..C23, as nvkms
 * FMTMatrix[] (nvkms-evo3.c): identity for RGB sources; for the packed YUV
 * sources the 8 bpc limited-range BT.601 / BT.709 to full-range RGB
 * conversion, since CA7E has no SET_PARAMS COLOR_SPACE field (C37E..C67E
 * convert through it instead).
 */
static const u32
wndwca7e_fmt_matrix[3][12] = {
	/* NV507E_SURFACE_SET_PARAMS_COLOR_SPACE_RGB: identity */
	{ 0x10000,       0,       0,        0,        0, 0x10000,        0,      0,       0,       0, 0x10000,        0 },
	/* YUV_601: FMT_COEFF_TYPE_REC601_YUV_8BPC_LTD_TO_RGB_16BPC_FULL */
	{ 0x19A29, 0x12B3C,       0, 0x1F2038, 0x1F2F14, 0x12B3C, 0x1F9B52, 0x8819,       0, 0x12B3C, 0x20668, 0x1EEA18 },
	/* YUV_709: FMT_COEFF_TYPE_REC709_YUV_8BPC_LTD_TO_RGB_16BPC_FULL */
	{ 0x1CCB7, 0x12B3C,       0, 0x1F06F1, 0x1F770C, 0x12B3C, 0x1FC933, 0x4D2D,       0, 0x12B3C, 0x21EDD, 0x1EDDDE },
};

static int
wndwca7e_image_set(struct nv50_wndw *wndw, struct nv50_wndw_atom *asyw)
{
	const u32 iso0_hi = upper_32_bits(asyw->image.offset[0]);
	const u32 iso0_lo = lower_32_bits(asyw->image.offset[0]);
	const u32 iso1_hi = upper_32_bits(asyw->image.offset[1]);
	const u32 iso1_lo = lower_32_bits(asyw->image.offset[1]);
	const u32 iso2_hi = upper_32_bits(asyw->image.offset[2]);
	const u32 iso2_lo = lower_32_bits(asyw->image.offset[2]);
	const bool planar = asyw->image.handle[1] != 0;		/* NV12 and I420 */
	const bool planar3 = asyw->image.handle[2] != 0;	/* I420 */
	const u32 *fmt = wndwca7e_fmt_matrix[asyw->image.colorspace < 3 ? asyw->image.colorspace : 0];
	struct nvif_push *push = &wndw->wndw.push;
	int ret, kind;

	if (asyw->image.kind)
		kind = NVCA7E_SET_SURFACE_ADDRESS_LO_ISO_KIND_BLOCKLINEAR;
	else
		kind = NVCA7E_SET_SURFACE_ADDRESS_LO_ISO_KIND_PITCH;

	ret = PUSH_WAIT(push, 17 + 13 + 12 + 8 + 2 + 4);
	if (ret)
		return ret;

	/*
	 * RGB colour pipeline defaults, as nvkms sets them on every flip
	 * (nvkms-evo3.c EvoSetFMTMatrixC5, ConfigureCsc0/Csc1C5, TMO LUT):
	 * identity FMT matrix, CSC00/01/10/11 and both CSC LUTs off, no TMO
	 * LUT, so a window the firmware never set up does not depend on its
	 * reset state. (The wrong overlay colours on the GB207 were blending,
	 * see nv50_wndw_atomic_check_acquire.)
	 */
	PUSH_MTHD(push, NVCA7E, SET_FMT_COEFFICIENT_C00, fmt[0],
				SET_FMT_COEFFICIENT_C01, fmt[1],
				SET_FMT_COEFFICIENT_C02, fmt[2],
				SET_FMT_COEFFICIENT_C03, fmt[3],
				SET_FMT_COEFFICIENT_C10, fmt[4],
				SET_FMT_COEFFICIENT_C11, fmt[5]);
	PUSH_MTHD(push, NVCA7E, SET_FMT_COEFFICIENT_C12, fmt[6],
				SET_FMT_COEFFICIENT_C13, fmt[7],
				SET_FMT_COEFFICIENT_C20, fmt[8],
				SET_FMT_COEFFICIENT_C21, fmt[9],
				SET_FMT_COEFFICIENT_C22, fmt[10],
				SET_FMT_COEFFICIENT_C23, fmt[11]);
	PUSH_MTHD(push, NVCA7E, SET_CSC00CONTROL, 0);
	PUSH_MTHD(push, NVCA7E, SET_CSC0LUT_CONTROL, 0);
	PUSH_MTHD(push, NVCA7E, SET_CSC01CONTROL, 0);
	PUSH_MTHD(push, NVCA7E, SET_CSC10CONTROL, 0);
	PUSH_MTHD(push, NVCA7E, SET_CSC1LUT_CONTROL, 0);
	PUSH_MTHD(push, NVCA7E, SET_CSC11CONTROL, 0);
	PUSH_MTHD(push, NVCA7E, SET_SURFACE_ADDRESS_LO_TMO_LUT, 0);

	/*
	 * Surface slots are eye + plane * 2 (nvkms EyeAndPlaneToCtxDmaIdx):
	 * the left eye's planes 0, 1, 2 live in slots 0, 2, 4; slots 1, 3, 5
	 * are the right eye. The chroma planes (semi-planar: 1, planar: 1 and 2)
	 * are the same buffer at their offsets, else off.
	 */
	PUSH_MTHD(push, NVCA7E, SET_SURFACE_ADDRESS_HI_ISO(0), iso0_hi);
	PUSH_MTHD(push, NVCA7E, SET_SURFACE_ADDRESS_HI_ISO(2), iso1_hi);
	PUSH_MTHD(push, NVCA7E, SET_SURFACE_ADDRESS_HI_ISO(4), iso2_hi);

	PUSH_MTHD(push, NVCA7E, SET_SURFACE_ADDRESS_LO_ISO(0),
		  NVVAL(NVCA7E, SET_SURFACE_ADDRESS_LO_ISO, ADDRESS_LO, iso0_lo >> 4) |
		  NVDEF(NVCA7E, SET_SURFACE_ADDRESS_LO_ISO, TARGET, PHYSICAL_NVM) |
		  NVVAL(NVCA7E, SET_SURFACE_ADDRESS_LO_ISO, KIND, kind) |
		  NVDEF(NVCA7E, SET_SURFACE_ADDRESS_LO_ISO, ENABLE, ENABLE));

	PUSH_MTHD(push, NVCA7E, SET_SURFACE_ADDRESS_LO_ISO(2),
		  NVVAL(NVCA7E, SET_SURFACE_ADDRESS_LO_ISO, ADDRESS_LO, planar ? iso1_lo >> 4 : 0) |
		  NVDEF(NVCA7E, SET_SURFACE_ADDRESS_LO_ISO, TARGET, PHYSICAL_NVM) |
		  NVVAL(NVCA7E, SET_SURFACE_ADDRESS_LO_ISO, KIND, kind) |
		  NVVAL(NVCA7E, SET_SURFACE_ADDRESS_LO_ISO, ENABLE, planar ? 1 : 0));

	PUSH_MTHD(push, NVCA7E, SET_SURFACE_ADDRESS_LO_ISO(4),
		  NVVAL(NVCA7E, SET_SURFACE_ADDRESS_LO_ISO, ADDRESS_LO, planar3 ? iso2_lo >> 4 : 0) |
		  NVDEF(NVCA7E, SET_SURFACE_ADDRESS_LO_ISO, TARGET, PHYSICAL_NVM) |
		  NVVAL(NVCA7E, SET_SURFACE_ADDRESS_LO_ISO, KIND, kind) |
		  NVVAL(NVCA7E, SET_SURFACE_ADDRESS_LO_ISO, ENABLE, planar3 ? 1 : 0));

	PUSH_MTHD(push, NVCA7E, SET_PRESENT_CONTROL,
		  NVVAL(NVCA7E, SET_PRESENT_CONTROL, MIN_PRESENT_INTERVAL, asyw->image.interval) |
		  NVVAL(NVCA7E, SET_PRESENT_CONTROL, BEGIN_MODE, asyw->image.mode) |
		  NVDEF(NVCA7E, SET_PRESENT_CONTROL, TIMESTAMP_MODE, DISABLE));

	PUSH_MTHD(push, NVCA7E, SET_SIZE,
		  NVVAL(NVCA7E, SET_SIZE, WIDTH, asyw->image.w) |
		  NVVAL(NVCA7E, SET_SIZE, HEIGHT, asyw->image.h),

				SET_STORAGE,
		  NVVAL(NVCA7E, SET_STORAGE, BLOCK_HEIGHT, asyw->image.blockh),

				SET_PARAMS,
		  NVVAL(NVCA7E, SET_PARAMS, FORMAT, asyw->image.format) |
		  NVDEF(NVCA7E, SET_PARAMS, CLAMP_BEFORE_BLEND, DISABLE) |
		  NVVAL(NVCA7E, SET_PARAMS, SWAP_UV, asyw->image.swapuv) |
		  NVDEF(NVCA7E, SET_PARAMS, FMT_ROUNDING_MODE, ROUND_TO_NEAREST),

				SET_PLANAR_STORAGE(0),
		  NVVAL(NVCA7E, SET_PLANAR_STORAGE, PITCH, asyw->image.blocks[0]) |
		  NVVAL(NVCA7E, SET_PLANAR_STORAGE, PITCH, asyw->image.pitch[0] >> 6),

				SET_PLANAR_STORAGE(1),
		  NVVAL(NVCA7E, SET_PLANAR_STORAGE, PITCH, asyw->image.blocks[1]) |
		  NVVAL(NVCA7E, SET_PLANAR_STORAGE, PITCH, asyw->image.pitch[1] >> 6),

				SET_PLANAR_STORAGE(2),
		  NVVAL(NVCA7E, SET_PLANAR_STORAGE, PITCH, asyw->image.blocks[2]) |
		  NVVAL(NVCA7E, SET_PLANAR_STORAGE, PITCH, asyw->image.pitch[2] >> 6));

	PUSH_MTHD(push, NVCA7E, SET_POINT_IN(0),
		  NVVAL(NVCA7E, SET_POINT_IN, X, asyw->state.src_x >> 16) |
		  NVVAL(NVCA7E, SET_POINT_IN, Y, asyw->state.src_y >> 16));

	PUSH_MTHD(push, NVCA7E, SET_SIZE_IN,
		  NVVAL(NVCA7E, SET_SIZE_IN, WIDTH, asyw->state.src_w >> 16) |
		  NVVAL(NVCA7E, SET_SIZE_IN, HEIGHT, asyw->state.src_h >> 16));

	PUSH_MTHD(push, NVCA7E, SET_SIZE_OUT,
		  NVVAL(NVCA7E, SET_SIZE_OUT, WIDTH, asyw->state.crtc_w) |
		  NVVAL(NVCA7E, SET_SIZE_OUT, HEIGHT, asyw->state.crtc_h));

	/*
	 * nvkms programs the input scaler taps on every flip (at least 2
	 * taps, EvoFlipC5Common). RGB at 1:1 works with the reset value, but
	 * 4:2:0 chroma is upsampled through this block, so leave nothing
	 * unset here.
	 */
	PUSH_MTHD(push, NVCA7E, SET_CONTROL_INPUT_SCALER,
		  NVDEF(NVCA7E, SET_CONTROL_INPUT_SCALER, VERTICAL_TAPS, TAPS_2) |
		  NVDEF(NVCA7E, SET_CONTROL_INPUT_SCALER, HORIZONTAL_TAPS, TAPS_2));

	return 0;
}

static int
wndwca7e_ilut_clr(struct nv50_wndw *wndw)
{
	struct nvif_push *push = &wndw->wndw.push;
	int ret;

	ret = PUSH_WAIT(push, 2);
	if (ret)
		return ret;

	PUSH_MTHD(push, NVCA7E, SET_SURFACE_ADDRESS_LO_ILUT,
		  NVDEF(NVCA7E, SET_SURFACE_ADDRESS_LO_ILUT, ENABLE, DISABLE));

	return 0;
}

static int
wndwca7e_ilut_set(struct nv50_wndw *wndw, struct nv50_wndw_atom *asyw)
{
	const u32 ilut_hi = upper_32_bits(asyw->xlut.i.offset);
	const u32 ilut_lo = lower_32_bits(asyw->xlut.i.offset);
	struct nvif_push *push = &wndw->wndw.push;
	int ret;

	ret = PUSH_WAIT(push, 5);
	if (ret)
		return ret;

	PUSH_MTHD(push, NVCA7E, SET_SURFACE_ADDRESS_HI_ILUT, ilut_hi,

				SET_SURFACE_ADDRESS_LO_ILUT,
		  NVVAL(NVCA7E, SET_SURFACE_ADDRESS_LO_ILUT, ADDRESS_LO, ilut_lo >> 4) |
		  NVDEF(NVCA7E, SET_SURFACE_ADDRESS_LO_ILUT, TARGET, PHYSICAL_NVM) |
		  NVDEF(NVCA7E, SET_SURFACE_ADDRESS_LO_ILUT, ENABLE, ENABLE));

	PUSH_MTHD(push, NVCA7E, SET_ILUT_CONTROL,
		  NVVAL(NVCA7E, SET_ILUT_CONTROL, SIZE, asyw->xlut.i.size) |
		  NVVAL(NVCA7E, SET_ILUT_CONTROL, MODE, asyw->xlut.i.mode) |
		  NVVAL(NVCA7E, SET_ILUT_CONTROL, INTERPOLATE, asyw->xlut.i.output_mode));

	return 0;
}

static int
wndwca7e_ntfy_clr(struct nv50_wndw *wndw)
{
	struct nvif_push *push = &wndw->wndw.push;
	int ret;

	ret = PUSH_WAIT(push, 2);
	if (ret)
		return ret;

	PUSH_MTHD(push, NVCA7E, SET_SURFACE_ADDRESS_LO_NOTIFIER,
		  NVDEF(NVCA7E, SET_SURFACE_ADDRESS_LO_NOTIFIER, ENABLE, DISABLE));

	return 0;
}

static int
wndwca7e_ntfy_set(struct nv50_wndw *wndw, struct nv50_wndw_atom *asyw)
{
	struct nv50_disp *disp = nv50_disp(wndw->plane.dev);
	u32 ntfy_target;
	const u64 ntfy_addr = nv50_disp_sync_addr(disp, asyw->ntfy.offset, &ntfy_target);
	const u32 ntfy_hi = upper_32_bits(ntfy_addr);
	const u32 ntfy_lo = lower_32_bits(ntfy_addr);
	struct nvif_push *push = &wndw->wndw.push;
	int ret;

	ret = PUSH_WAIT(push, 5);
	if (ret)
		return ret;

	PUSH_MTHD(push, NVCA7E, SET_SURFACE_ADDRESS_HI_NOTIFIER, ntfy_hi,

				SET_SURFACE_ADDRESS_LO_NOTIFIER,
		  NVVAL(NVCA7E, SET_SURFACE_ADDRESS_LO_NOTIFIER, ADDRESS_LO, ntfy_lo >> 4) |
		  NVVAL(NVCA7E, SET_SURFACE_ADDRESS_LO_NOTIFIER, TARGET, ntfy_target) |
		  NVDEF(NVCA7E, SET_SURFACE_ADDRESS_LO_NOTIFIER, ENABLE, ENABLE));

	PUSH_MTHD(push, NVCA7E, SET_NOTIFIER_CONTROL,
		  NVVAL(NVCA7E, SET_NOTIFIER_CONTROL, MODE, asyw->ntfy.awaken));

	return 0;
}

/****************************************************************
 *            Log2(block height) ----------------------------+  *
 *            Page Kind ----------------------------------+  |  *
 *            Gob Height/Page Kind Generation ------+     |  |  *
 *                          Sector layout -------+  |     |  |  *
 *                          Compression ------+  |  |     |  |  */
const u64 wndwca7e_modifiers[] = { /*         |  |  |     |  |  */
	/* 4cpp+ modifiers */
	DRM_FORMAT_MOD_NVIDIA_BLOCK_LINEAR_2D(0, 1, 2, 0x06, 0),
	DRM_FORMAT_MOD_NVIDIA_BLOCK_LINEAR_2D(0, 1, 2, 0x06, 1),
	DRM_FORMAT_MOD_NVIDIA_BLOCK_LINEAR_2D(0, 1, 2, 0x06, 2),
	DRM_FORMAT_MOD_NVIDIA_BLOCK_LINEAR_2D(0, 1, 2, 0x06, 3),
	DRM_FORMAT_MOD_NVIDIA_BLOCK_LINEAR_2D(0, 1, 2, 0x06, 4),
	DRM_FORMAT_MOD_NVIDIA_BLOCK_LINEAR_2D(0, 1, 2, 0x06, 5),
	/* 1cpp/8bpp modifiers */
	DRM_FORMAT_MOD_NVIDIA_BLOCK_LINEAR_2D(0, 2, 2, 0x06, 0),
	DRM_FORMAT_MOD_NVIDIA_BLOCK_LINEAR_2D(0, 2, 2, 0x06, 1),
	DRM_FORMAT_MOD_NVIDIA_BLOCK_LINEAR_2D(0, 2, 2, 0x06, 2),
	DRM_FORMAT_MOD_NVIDIA_BLOCK_LINEAR_2D(0, 2, 2, 0x06, 3),
	DRM_FORMAT_MOD_NVIDIA_BLOCK_LINEAR_2D(0, 2, 2, 0x06, 4),
	DRM_FORMAT_MOD_NVIDIA_BLOCK_LINEAR_2D(0, 2, 2, 0x06, 5),
	/* 2cpp/16bpp modifiers */
	DRM_FORMAT_MOD_NVIDIA_BLOCK_LINEAR_2D(0, 3, 2, 0x06, 0),
	DRM_FORMAT_MOD_NVIDIA_BLOCK_LINEAR_2D(0, 3, 2, 0x06, 1),
	DRM_FORMAT_MOD_NVIDIA_BLOCK_LINEAR_2D(0, 3, 2, 0x06, 2),
	DRM_FORMAT_MOD_NVIDIA_BLOCK_LINEAR_2D(0, 3, 2, 0x06, 3),
	DRM_FORMAT_MOD_NVIDIA_BLOCK_LINEAR_2D(0, 3, 2, 0x06, 4),
	DRM_FORMAT_MOD_NVIDIA_BLOCK_LINEAR_2D(0, 3, 2, 0x06, 5),
	/* All formats support linear */
	DRM_FORMAT_MOD_LINEAR,
	DRM_FORMAT_MOD_INVALID
};

static const struct nv50_wndw_func
wndwca7e = {
	.acquire = wndwc37e_acquire,
	.release = wndwc37e_release,
	.ntfy_set = wndwca7e_ntfy_set,
	.ntfy_clr = wndwca7e_ntfy_clr,
	.ntfy_reset = corec37d_ntfy_init,
	.ntfy_wait_begun = base507c_ntfy_wait_begun,
	.ilut = wndwc57e_ilut,
	.ilut_identity = true,
	.ilut_size = 1024,
	.xlut_set = wndwca7e_ilut_set,
	.xlut_clr = wndwca7e_ilut_clr,
	.csc = base907c_csc,
	.csc_set = wndwc57e_csc_set,
	.csc_clr = wndwc57e_csc_clr,
	.image_set = wndwca7e_image_set,
	.image_clr = wndwca7e_image_clr,
	.blend_set = wndwc37e_blend_set,
	.update = wndwc37e_update,
};

int
wndwca7e_new(struct nouveau_drm *drm, enum drm_plane_type type, int index,
	     s32 oclass, struct nv50_wndw **pwndw)
{
	return wndwc37e_new_(&wndwca7e, drm, type, index, oclass, BIT(index >> 1), pwndw);
}
