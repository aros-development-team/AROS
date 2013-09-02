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

#include "nv_include.h"
#include "nvc0_accel.h"

#define NOUVEAU_BO(a, b, m) (NOUVEAU_BO_##a | NOUVEAU_BO_##b | NOUVEAU_BO_##m)

Bool
NVAccelInitM2MF_NVC0(ScrnInfoPtr pScrn)
{
	NVPtr pNv = NVPTR(pScrn);
	struct nouveau_channel *chan = pNv->chan;
	int ret;

	ret = nouveau_grobj_alloc(chan, 0x9039, 0x9039, &pNv->NvMemFormat);
	if (ret)
		return FALSE;

	return TRUE;
}

Bool
NVAccelInit2D_NVC0(ScrnInfoPtr pScrn)
{
	NVPtr pNv = NVPTR(pScrn);
	struct nouveau_channel *chan = pNv->chan;
	int ret;

	ret = nouveau_grobj_alloc(chan, 0x902d, 0x902d, &pNv->Nv2D);
	if (ret)
		return FALSE;

	BEGIN_RING(chan, pNv->Nv2D, NV50_2D_CLIP_ENABLE, 1);
	OUT_RING  (chan, 1);
	BEGIN_RING(chan, pNv->Nv2D, NV50_2D_COLOR_KEY_ENABLE, 1);
	OUT_RING  (chan, 0);
	BEGIN_RING(chan, pNv->Nv2D, 0x0884, 1);
	OUT_RING  (chan, 0x3f);
	BEGIN_RING(chan, pNv->Nv2D, 0x0888, 1);
	OUT_RING  (chan, 1);
	BEGIN_RING(chan, pNv->Nv2D, NV50_2D_ROP, 1);
	OUT_RING  (chan, 0x55);
	BEGIN_RING(chan, pNv->Nv2D, NV50_2D_OPERATION, 1);
	OUT_RING  (chan, NV50_2D_OPERATION_SRCCOPY);

	BEGIN_RING(chan, pNv->Nv2D, NV50_2D_BLIT_DU_DX_FRACT, 4);
	OUT_RING  (chan, 0);
	OUT_RING  (chan, 1);
	OUT_RING  (chan, 0);
	OUT_RING  (chan, 1);
	BEGIN_RING(chan, pNv->Nv2D, NV50_2D_DRAW_SHAPE, 2);
	OUT_RING  (chan, 4);
	OUT_RING  (chan, NV50_SURFACE_FORMAT_R5G6B5_UNORM);
	BEGIN_RING(chan, pNv->Nv2D, NV50_2D_PATTERN_FORMAT, 2);
	OUT_RING  (chan, 2);
	OUT_RING  (chan, 1);
	FIRE_RING (chan);

#if !defined(__AROS__)
	pNv->currentRop = 0xfffffffa;
#endif
	return TRUE;
}

Bool
NVAccelInit3D_NVC0(ScrnInfoPtr pScrn)
{
	NVPtr pNv = NVPTR(pScrn);
	struct nouveau_channel *chan = pNv->chan;
	struct nouveau_grobj *fermi, *m2mf;
	struct nouveau_bo *bo;
	int ret, i;

	if (!pNv->Nv3D) {
		ret = nouveau_grobj_alloc(chan, 0x9097, 0x9097, &pNv->Nv3D);
		if (ret)
			return FALSE;

		ret = nouveau_bo_new(pNv->dev, NOUVEAU_BO_VRAM,
				     (128 << 10), 0x20000,
				     &pNv->tesla_scratch);
		if (ret) {
			nouveau_grobj_free(&pNv->Nv3D);
			return FALSE;
		}
	}
	bo = pNv->tesla_scratch;
	m2mf = pNv->NvMemFormat;
	fermi = pNv->Nv3D;

	if (MARK_RING(chan, 512, 32))
		return FALSE;

	BEGIN_RING(chan, m2mf, NVC0_M2MF_NOTIFY_ADDRESS_HIGH, 3);
	OUT_RELOCh(chan, bo, NTFY_OFFSET, NOUVEAU_BO_VRAM | NOUVEAU_BO_RDWR);
	OUT_RELOCl(chan, bo, NTFY_OFFSET, NOUVEAU_BO_VRAM | NOUVEAU_BO_RDWR);
	OUT_RING  (chan, 0);

	BEGIN_RING(chan, fermi, NVC0_GRAPH_NOTIFY_ADDRESS_HIGH, 3);
	OUT_RELOCh(chan, bo, NTFY_OFFSET, NOUVEAU_BO_VRAM | NOUVEAU_BO_RDWR);
	OUT_RELOCl(chan, bo, NTFY_OFFSET, NOUVEAU_BO_VRAM | NOUVEAU_BO_RDWR);
	OUT_RING  (chan, 0);

	BEGIN_RING(chan, fermi, NVC0_3D_MULTISAMPLE_COLOR_ENABLE, 1);
	OUT_RING  (chan, 0);
	BEGIN_RING(chan, fermi, NVC0_3D_MULTISAMPLE_ZETA_ENABLE, 1);
	OUT_RING  (chan, 0);
	BEGIN_RING(chan, fermi, NVC0_3D_MULTISAMPLE_MODE, 1);
	OUT_RING  (chan, NVC0_3D_MULTISAMPLE_MODE_1X);

	BEGIN_RING(chan, fermi, NVC0_3D_COND_MODE, 1);
	OUT_RING  (chan, NVC0_3D_COND_MODE_ALWAYS);
	BEGIN_RING(chan, fermi, NVC0_3D_RT_CONTROL, 1);
	OUT_RING  (chan, 1);
	BEGIN_RING(chan, fermi, NVC0_3D_ZETA_ENABLE, 1);
	OUT_RING  (chan, 0);
	BEGIN_RING(chan, fermi, NVC0_3D_VIEWPORT_CLIP_RECTS_EN, 2);
	OUT_RING  (chan, 0);
	OUT_RING  (chan, 0);
	BEGIN_RING(chan, fermi, NVC0_3D_CLIPID_ENABLE, 1);
	OUT_RING  (chan, 0);
	BEGIN_RING(chan, fermi, NVC0_3D_VERTEX_TWO_SIDE_ENABLE, 1);
	OUT_RING  (chan, 0);
	BEGIN_RING(chan, fermi, 0x0fac, 1);
	OUT_RING  (chan, 0);
	BEGIN_RING(chan, fermi, NVC0_3D_COLOR_MASK(0), 8);
	OUT_RING  (chan, 0x1111);
	for (i = 1; i < 8; ++i)
		OUT_RING(chan, 0);
	FIRE_RING (chan);

	BEGIN_RING(chan, fermi, NVC0_3D_SCREEN_SCISSOR_HORIZ, 2);
	OUT_RING  (chan, (8192 << 16) | 0);
	OUT_RING  (chan, (8192 << 16) | 0);
	BEGIN_RING(chan, fermi, NVC0_3D_Y_ORIGIN_BOTTOM, 1);
	OUT_RING  (chan, 0);
	BEGIN_RING(chan, fermi, NVC0_3D_WINDOW_OFFSET_X, 2);
	OUT_RING  (chan, 0);
	OUT_RING  (chan, 0);
	BEGIN_RING(chan, fermi, 0x1590, 1);
	OUT_RING  (chan, 0);

	BEGIN_RING(chan, fermi, NVC0_3D_LINKED_TSC, 1);
	OUT_RING  (chan, 1);

	BEGIN_RING(chan, fermi, NVC0_3D_VIEWPORT_TRANSFORM_EN, 1);
	OUT_RING  (chan, 0);
	BEGIN_RING(chan, fermi, NVC0_3D_VIEW_VOLUME_CLIP_CTRL, 1);
	OUT_RING  (chan, 0);
	BEGIN_RING(chan, fermi, NVC0_3D_DEPTH_RANGE_NEAR(0), 2);
	OUT_RINGf (chan, 0.0f);
	OUT_RINGf (chan, 1.0f);

	BEGIN_RING(chan, fermi, NVC0_3D_TEX_LIMITS(4), 1);
	OUT_RING  (chan, 0x54);

	BEGIN_RING(chan, fermi, NVC0_3D_BLEND_ENABLE(0), 8);
	OUT_RING  (chan, 1);
	for (i = 1; i < 8; ++i)
		OUT_RING(chan, 0);
	BEGIN_RING(chan, fermi, NVC0_3D_BLEND_INDEPENDENT, 1);
	OUT_RING  (chan, 0);

	BEGIN_RING(chan, fermi, 0x17bc, 3);
	OUT_RELOCh(chan, bo, MISC_OFFSET, NOUVEAU_BO_VRAM | NOUVEAU_BO_RDWR);
	OUT_RELOCl(chan, bo, MISC_OFFSET, NOUVEAU_BO_VRAM | NOUVEAU_BO_RDWR);
	OUT_RING  (chan, 1);
	FIRE_RING (chan);

	BEGIN_RING(chan, fermi, NVC0_3D_CODE_ADDRESS_HIGH, 2);
	OUT_RELOCh(chan, bo, CODE_OFFSET, NOUVEAU_BO_VRAM | NOUVEAU_BO_RD);
	OUT_RELOCl(chan, bo, CODE_OFFSET, NOUVEAU_BO_VRAM | NOUVEAU_BO_RD);

	BEGIN_RING(chan, m2mf, NVC0_M2MF_OFFSET_OUT_HIGH, 2);
	if (OUT_RELOCh(chan, bo, PVP_PASS, NOUVEAU_BO(VRAM, VRAM, WR)) ||
	    OUT_RELOCl(chan, bo, PVP_PASS, NOUVEAU_BO(VRAM, VRAM, WR))) {
		MARK_UNDO(chan);
		return FALSE;
	}
	BEGIN_RING(chan, m2mf, NVC0_M2MF_LINE_LENGTH_IN, 2);
	OUT_RING  (chan, 7 * 8 + 20 * 4);
	OUT_RING  (chan, 1);
	BEGIN_RING(chan, m2mf, NVC0_M2MF_EXEC, 1);
	OUT_RING  (chan, 0x100111);
	BEGIN_RING_NI(chan, m2mf, NVC0_M2MF_DATA, 7 * 2 + 20);
	OUT_RING  (chan, 0x00020461);
	OUT_RING  (chan, 0);
	OUT_RING  (chan, 0);
	OUT_RING  (chan, 0);
	OUT_RING  (chan, 0xff000);
	OUT_RING  (chan, 0x00000000); /* VP_ATTR_EN[0x000] */
	OUT_RING  (chan, 0x0001033f); /* VP_ATTR_EN[0x080] */
	OUT_RING  (chan, 0x00000000); /* VP_ATTR_EN[0x100] */
	OUT_RING  (chan, 0x00000000);
	OUT_RING  (chan, 0x00000000); /* VP_ATTR_EN[0x200] */
	OUT_RING  (chan, 0x00000000);
	OUT_RING  (chan, 0x00000000); /* VP_ATTR_EN[0x300] */
	OUT_RING  (chan, 0x00000000);
	OUT_RING  (chan, 0x0033f000); /* VP_EXPORT_EN[0x040] */
	OUT_RING  (chan, 0x00000000); /* VP_EXPORT_EN[0x0c0] */
	OUT_RING  (chan, 0x00000000);
	OUT_RING  (chan, 0x00000000);
	OUT_RING  (chan, 0x00000000);
	OUT_RING  (chan, 0x00000000); /* VP_EXPORT_EN[0x2c0] */
	OUT_RING  (chan, 0x00000000);
	OUT_RING  (chan, 0xfff01c66);
	OUT_RING  (chan, 0x06000080); /* vfetch { $r0,1,2,3 } b128 a[0x80] */
	OUT_RING  (chan, 0xfff11c26);
	OUT_RING  (chan, 0x06000090); /* vfetch { $r4,5 } b64 a[0x90] */
	OUT_RING  (chan, 0xfff19c26);
	OUT_RING  (chan, 0x060000a0); /* vfetch { $r6,7 } b64 a[0xa0] */
	OUT_RING  (chan, 0x03f01c66);
	OUT_RING  (chan, 0x0a7e0070); /* export v[0x70] { $r0 $r1 $r2 $r3 } */
	OUT_RING  (chan, 0x13f01c26);
	OUT_RING  (chan, 0x0a7e0080); /* export v[0x80] { $r4 $r5 } */
	OUT_RING  (chan, 0x1bf01c26);
	OUT_RING  (chan, 0x0a7e0090); /* export v[0x90] { $r6 $r7 } */
	OUT_RING  (chan, 0x00001de7);
	OUT_RING  (chan, 0x80000000); /* exit */

	BEGIN_RING(chan, fermi, NVC0_3D_SP_SELECT(1), 2);
	OUT_RING  (chan, 0x11);
	OUT_RING  (chan, PVP_PASS);
	BEGIN_RING(chan, fermi, NVC0_3D_SP_GPR_ALLOC(1), 1);
	OUT_RING  (chan, 8);
	BEGIN_RING(chan, fermi, 0x163c, 1);
	OUT_RING  (chan, 0);
	BEGIN_RING(chan, fermi, 0x2600, 1);
	OUT_RING  (chan, 1);
	FIRE_RING (chan);

	BEGIN_RING(chan, m2mf, NVC0_M2MF_OFFSET_OUT_HIGH, 2);
	if (OUT_RELOCh(chan, bo, PFP_S, NOUVEAU_BO(VRAM, VRAM, WR)) ||
	    OUT_RELOCl(chan, bo, PFP_S, NOUVEAU_BO(VRAM, VRAM, WR))) {
		MARK_UNDO(chan);
		return FALSE;
	}
	BEGIN_RING(chan, m2mf, NVC0_M2MF_LINE_LENGTH_IN, 2);
	OUT_RING  (chan, 6 * 8 + 20 * 4);
	OUT_RING  (chan, 1);
	BEGIN_RING(chan, m2mf, NVC0_M2MF_EXEC, 1);
	OUT_RING  (chan, 0x100111);
	BEGIN_RING_NI(chan, m2mf, NVC0_M2MF_DATA, 6 * 2 + 20);
	OUT_RING  (chan, 0x00021462);
	OUT_RING  (chan, 0x00000000);
	OUT_RING  (chan, 0x00000000);
	OUT_RING  (chan, 0x00000000);
	OUT_RING  (chan, 0x00000000);
	OUT_RING  (chan, 0x80000000);
	OUT_RING  (chan, 0x0000000a);
	OUT_RING  (chan, 0x00000000);
	OUT_RING  (chan, 0x00000000);
	OUT_RING  (chan, 0x00000000);
	OUT_RING  (chan, 0x00000000);
	OUT_RING  (chan, 0x00000000);
	OUT_RING  (chan, 0x00000000);
	OUT_RING  (chan, 0x00000000);
	OUT_RING  (chan, 0x00000000);
	OUT_RING  (chan, 0x00000000);
	OUT_RING  (chan, 0x00000000);
	OUT_RING  (chan, 0x00000000);
	OUT_RING  (chan, 0x0000000f);
	OUT_RING  (chan, 0x00000000);
	OUT_RING  (chan, 0xfff01c00);
	OUT_RING  (chan, 0xc07e007c); /* linterp f32 $r0 v[$r63+0x7c] */
	OUT_RING  (chan, 0x10001c00);
	OUT_RING  (chan, 0xc8000000); /* rcp f32 $r0 $r0 */
	OUT_RING  (chan, 0x03f05c40);
	OUT_RING  (chan, 0xc07e0084); /* pinterp f32 $r1 $r0 v[$r63+0x84] */
	OUT_RING  (chan, 0x03f01c40);
	OUT_RING  (chan, 0xc07e0080); /* pinterp f32 $r0 $r0 v[$r63+0x80] */
	OUT_RING  (chan, 0xfc001e86);
	OUT_RING  (chan, 0x8013c000); /* tex { $r0,1,2,3 } $t0 { $r0,1 } */
	OUT_RING  (chan, 0x00001de7);
	OUT_RING  (chan, 0x80000000); /* exit */

	BEGIN_RING(chan, m2mf, NVC0_M2MF_OFFSET_OUT_HIGH, 2);
	if (OUT_RELOCh(chan, bo, PFP_C, NOUVEAU_BO(VRAM, VRAM, WR)) ||
	    OUT_RELOCl(chan, bo, PFP_C, NOUVEAU_BO(VRAM, VRAM, WR))) {
		MARK_UNDO(chan);
		return FALSE;
	}
	BEGIN_RING(chan, m2mf, NVC0_M2MF_LINE_LENGTH_IN, 2);
	OUT_RING  (chan, 13 * 8 + 20 * 4);
	OUT_RING  (chan, 1);
	BEGIN_RING(chan, m2mf, NVC0_M2MF_EXEC, 1);
	OUT_RING  (chan, 0x100111);
	BEGIN_RING_NI(chan, m2mf, NVC0_M2MF_DATA, 13 * 2 + 20);
	OUT_RING  (chan, 0x00021462);
	OUT_RING  (chan, 0x00000000);
	OUT_RING  (chan, 0x00000000);
	OUT_RING  (chan, 0x00000000);
	OUT_RING  (chan, 0x00000000);
	OUT_RING  (chan, 0x80000000);
	OUT_RING  (chan, 0x00000a0a);
	OUT_RING  (chan, 0x00000000);
	OUT_RING  (chan, 0x00000000);
	OUT_RING  (chan, 0x00000000);
	OUT_RING  (chan, 0x00000000);
	OUT_RING  (chan, 0x00000000);
	OUT_RING  (chan, 0x00000000);
	OUT_RING  (chan, 0x00000000);
	OUT_RING  (chan, 0x00000000);
	OUT_RING  (chan, 0x00000000);
	OUT_RING  (chan, 0x00000000);
	OUT_RING  (chan, 0x00000000);
	OUT_RING  (chan, 0x0000000f);
	OUT_RING  (chan, 0x00000000);
	OUT_RING  (chan, 0xfff01c00);
	OUT_RING  (chan, 0xc07e007c); /* linterp f32 $r0 v[$r63+0x7c] */
	OUT_RING  (chan, 0x10001c00);
	OUT_RING  (chan, 0xc8000000); /* rcp f32 $r0 $r0 */
	OUT_RING  (chan, 0x03f0dc40);
	OUT_RING  (chan, 0xc07e0094); /* pinterp f32 $r3 $r0 v[$r63+0x94] */
	OUT_RING  (chan, 0x03f09c40);
	OUT_RING  (chan, 0xc07e0090); /* pinterp f32 $r2 $r0 v[$r63+0x90] */
	OUT_RING  (chan, 0xfc211e86);
	OUT_RING  (chan, 0x80120001); /* tex { _,_,_,$r4 } $t1 { $r2,3 } */
	OUT_RING  (chan, 0x03f05c40);
	OUT_RING  (chan, 0xc07e0084); /* pinterp f32 $r1 $r0 v[$r63+0x84] */
	OUT_RING  (chan, 0x03f01c40);
	OUT_RING  (chan, 0xc07e0080); /* pinterp f32 $r0 $r0 v[$r63+0x80] */
	OUT_RING  (chan, 0xfc001e86);
	OUT_RING  (chan, 0x8013c000); /* tex { $r0,1,2,3 } $t0 { $r0,1 } */
	OUT_RING  (chan, 0x1030dc40);
	OUT_RING  (chan, 0x58000000); /* mul ftz rn f32 $r3 $r3 $r4 */
	OUT_RING  (chan, 0x10209c40);
	OUT_RING  (chan, 0x58000000); /* mul ftz rn f32 $r2 $r2 $r4 */
	OUT_RING  (chan, 0x10105c40);
	OUT_RING  (chan, 0x58000000); /* mul ftz rn f32 $r1 $r1 $r4 */
	OUT_RING  (chan, 0x10001c40);
	OUT_RING  (chan, 0x58000000); /* mul ftz rn f32 $r0 $r0 $r4 */
	OUT_RING  (chan, 0x00001de7);
	OUT_RING  (chan, 0x80000000); /* exit */

	BEGIN_RING(chan, m2mf, NVC0_M2MF_OFFSET_OUT_HIGH, 2);
	if (OUT_RELOCh(chan, bo, PFP_CCA, NOUVEAU_BO(VRAM, VRAM, WR)) ||
	    OUT_RELOCl(chan, bo, PFP_CCA, NOUVEAU_BO(VRAM, VRAM, WR))) {
		MARK_UNDO(chan);
		return FALSE;
	}
	BEGIN_RING(chan, m2mf, NVC0_M2MF_LINE_LENGTH_IN, 2);
	OUT_RING  (chan, 13 * 8 + 20 * 4);
	OUT_RING  (chan, 1);
	BEGIN_RING(chan, m2mf, NVC0_M2MF_EXEC, 1);
	OUT_RING  (chan, 0x100111);
	BEGIN_RING_NI(chan, m2mf, NVC0_M2MF_DATA, 13 * 2 + 20);
	OUT_RING  (chan, 0x00021462); /* 0x0000c000 = USES_KIL, MULTI_COLORS */
	OUT_RING  (chan, 0x00000000);
	OUT_RING  (chan, 0x00000000);
	OUT_RING  (chan, 0x00000000);
	OUT_RING  (chan, 0x00000000);
	OUT_RING  (chan, 0x80000000); /* FRAG_COORD_UMASK = 0x8 */
	OUT_RING  (chan, 0x00000a0a); /* FP_INTERP[0x080], 0022 0022 */
	OUT_RING  (chan, 0x00000000); /* FP_INTERP[0x0c0], 0 = OFF */
	OUT_RING  (chan, 0x00000000); /* FP_INTERP[0x100], 1 = FLAT */
	OUT_RING  (chan, 0x00000000); /* FP_INTERP[0x140], 2 = PERSPECTIVE */
	OUT_RING  (chan, 0x00000000); /* FP_INTERP[0x180], 3 = LINEAR */
	OUT_RING  (chan, 0x00000000); /* FP_INTERP[0x1c0] */
	OUT_RING  (chan, 0x00000000); /* FP_INTERP[0x200] */
	OUT_RING  (chan, 0x00000000); /* FP_INTERP[0x240] */
	OUT_RING  (chan, 0x00000000); /* FP_INTERP[0x280] */
	OUT_RING  (chan, 0x00000000); /* FP_INTERP[0x2c0] */
	OUT_RING  (chan, 0x00000000); /* FP_INTERP[0x300] */
	OUT_RING  (chan, 0x00000000);
	OUT_RING  (chan, 0x0000000f); /* FP_RESULT_MASK (0x8000 Face ?) */
	OUT_RING  (chan, 0x00000000); /* 0x2 = FragDepth, 0x1 = SampleMask */
	OUT_RING  (chan, 0xfff01c00);
	OUT_RING  (chan, 0xc07e007c); /* linterp f32 $r0 v[$r63+0x7c] */
	OUT_RING  (chan, 0x10001c00);
	OUT_RING  (chan, 0xc8000000); /* rcp f32 $r0 $r0 */
	OUT_RING  (chan, 0x03f0dc40);
	OUT_RING  (chan, 0xc07e0094); /* pinterp f32 $r3 $r0 v[$r63+0x94] */
	OUT_RING  (chan, 0x03f09c40);
	OUT_RING  (chan, 0xc07e0090); /* pinterp f32 $r2 $r0 v[$r63+0x90] */
	OUT_RING  (chan, 0xfc211e86);
	OUT_RING  (chan, 0x8013c001); /* tex { $r4,5,6,7 } $t1 { $r2,3 } */
	OUT_RING  (chan, 0x03f05c40);
	OUT_RING  (chan, 0xc07e0084); /* pinterp f32 $r1 $r0 v[$r63+0x84] */
	OUT_RING  (chan, 0x03f01c40);
	OUT_RING  (chan, 0xc07e0080); /* pinterp f32 $r0 $r0 v[$r63+0x80] */
	OUT_RING  (chan, 0xfc001e86);
	OUT_RING  (chan, 0x8013c000); /* tex { $r0,1,2,3 } $t0 { $r0,1 } */
	OUT_RING  (chan, 0x1c30dc40);
	OUT_RING  (chan, 0x58000000); /* mul ftz rn f32 $r3 $r3 $r7 */
	OUT_RING  (chan, 0x18209c40);
	OUT_RING  (chan, 0x58000000); /* mul ftz rn f32 $r2 $r2 $r6 */
	OUT_RING  (chan, 0x14105c40);
	OUT_RING  (chan, 0x58000000); /* mul ftz rn f32 $r1 $r1 $r5 */
	OUT_RING  (chan, 0x10001c40);
	OUT_RING  (chan, 0x58000000); /* mul ftz rn f32 $r0 $r0 $r4 */
	OUT_RING  (chan, 0x00001de7);
	OUT_RING  (chan, 0x80000000); /* exit */

	BEGIN_RING(chan, m2mf, NVC0_M2MF_OFFSET_OUT_HIGH, 2);
	if (OUT_RELOCh(chan, bo, PFP_CCASA, NOUVEAU_BO(VRAM, VRAM, WR)) ||
	    OUT_RELOCl(chan, bo, PFP_CCASA, NOUVEAU_BO(VRAM, VRAM, WR))) {
		MARK_UNDO(chan);
		return FALSE;
	}
	BEGIN_RING(chan, m2mf, NVC0_M2MF_LINE_LENGTH_IN, 2);
	OUT_RING  (chan, 13 * 8 + 20 * 4);
	OUT_RING  (chan, 1);
	BEGIN_RING(chan, m2mf, NVC0_M2MF_EXEC, 1);
	OUT_RING  (chan, 0x100111);
	BEGIN_RING_NI(chan, m2mf, NVC0_M2MF_DATA, 13 * 2 + 20);
	OUT_RING  (chan, 0x00021462);
	OUT_RING  (chan, 0x00000000);
	OUT_RING  (chan, 0x00000000);
	OUT_RING  (chan, 0x00000000);
	OUT_RING  (chan, 0x00000000);
	OUT_RING  (chan, 0x80000000);
	OUT_RING  (chan, 0x00000a0a);
	OUT_RING  (chan, 0x00000000);
	OUT_RING  (chan, 0x00000000);
	OUT_RING  (chan, 0x00000000);
	OUT_RING  (chan, 0x00000000);
	OUT_RING  (chan, 0x00000000);
	OUT_RING  (chan, 0x00000000);
	OUT_RING  (chan, 0x00000000);
	OUT_RING  (chan, 0x00000000);
	OUT_RING  (chan, 0x00000000);
	OUT_RING  (chan, 0x00000000);
	OUT_RING  (chan, 0x00000000);
	OUT_RING  (chan, 0x0000000f);
	OUT_RING  (chan, 0x00000000);
	OUT_RING  (chan, 0xfff01c00);
	OUT_RING  (chan, 0xc07e007c); /* linterp f32 $r0 v[$r63+0x7c] */
	OUT_RING  (chan, 0x10001c00);
	OUT_RING  (chan, 0xc8000000); /* rcp f32 $r0 $r0 */
	OUT_RING  (chan, 0x03f0dc40);
	OUT_RING  (chan, 0xc07e0084); /* pinterp f32 $r3 $r0 v[$r63+0x84] */
	OUT_RING  (chan, 0x03f09c40);
	OUT_RING  (chan, 0xc07e0080); /* pinterp f32 $r2 $r0 v[$r63+0x80] */
	OUT_RING  (chan, 0xfc211e86);
	OUT_RING  (chan, 0x80120000); /* tex { _,_,_,$r4 } $t0 { $r2,3 } */
	OUT_RING  (chan, 0x03f05c40);
	OUT_RING  (chan, 0xc07e0094); /* pinterp f32 $r1 $r0 v[$r63+0x94] */
	OUT_RING  (chan, 0x03f01c40);
	OUT_RING  (chan, 0xc07e0090); /* pinterp f32 $r0 $r0 v[$r63+0x90] */
	OUT_RING  (chan, 0xfc001e86);
	OUT_RING  (chan, 0x8013c001); /* tex { $r0,1,2,3 } $t1 { $r0,1 } */
	OUT_RING  (chan, 0x1030dc40);
	OUT_RING  (chan, 0x58000000); /* mul ftz rn f32 $r3 $r3 $r4 */
	OUT_RING  (chan, 0x10209c40);
	OUT_RING  (chan, 0x58000000); /* mul ftz rn f32 $r2 $r2 $r4 */
	OUT_RING  (chan, 0x10105c40);
	OUT_RING  (chan, 0x58000000); /* mul ftz rn f32 $r1 $r1 $r4 */
	OUT_RING  (chan, 0x10001c40);
	OUT_RING  (chan, 0x58000000); /* mul ftz rn f32 $r0 $r0 $r4 */
	OUT_RING  (chan, 0x00001de7);
	OUT_RING  (chan, 0x80000000); /* exit */

	BEGIN_RING(chan, m2mf, NVC0_M2MF_OFFSET_OUT_HIGH, 2);
	if (OUT_RELOCh(chan, bo, PFP_S_A8, NOUVEAU_BO(VRAM, VRAM, WR)) ||
	    OUT_RELOCl(chan, bo, PFP_S_A8, NOUVEAU_BO(VRAM, VRAM, WR))) {
		MARK_UNDO(chan);
		return FALSE;
	}
	BEGIN_RING(chan, m2mf, NVC0_M2MF_LINE_LENGTH_IN, 2);
	OUT_RING  (chan, 9 * 8 + 20 * 4);
	OUT_RING  (chan, 1);
	BEGIN_RING(chan, m2mf, NVC0_M2MF_EXEC, 1);
	OUT_RING  (chan, 0x100111);
	BEGIN_RING_NI(chan, m2mf, NVC0_M2MF_DATA, 9 * 2 + 20);
	OUT_RING  (chan, 0x00021462);
	OUT_RING  (chan, 0x00000000);
	OUT_RING  (chan, 0x00000000);
	OUT_RING  (chan, 0x00000000);
	OUT_RING  (chan, 0x00000000);
	OUT_RING  (chan, 0x80000000);
	OUT_RING  (chan, 0x0000000a);
	OUT_RING  (chan, 0x00000000);
	OUT_RING  (chan, 0x00000000);
	OUT_RING  (chan, 0x00000000);
	OUT_RING  (chan, 0x00000000);
	OUT_RING  (chan, 0x00000000);
	OUT_RING  (chan, 0x00000000);
	OUT_RING  (chan, 0x00000000);
	OUT_RING  (chan, 0x00000000);
	OUT_RING  (chan, 0x00000000);
	OUT_RING  (chan, 0x00000000);
	OUT_RING  (chan, 0x00000000);
	OUT_RING  (chan, 0x0000000f);
	OUT_RING  (chan, 0x00000000);
	OUT_RING  (chan, 0xfff01c00);
	OUT_RING  (chan, 0xc07e007c); /* linterp f32 $r0 v[$r63+0x7c] */
	OUT_RING  (chan, 0x10001c00);
	OUT_RING  (chan, 0xc8000000); /* rcp f32 $r0 $r0 */
	OUT_RING  (chan, 0x03f05c40);
	OUT_RING  (chan, 0xc07e0084); /* pinterp f32 $r1 $r0 v[$r63+0x84] */
	OUT_RING  (chan, 0x03f01c40);
	OUT_RING  (chan, 0xc07e0080); /* pinterp f32 $r0 $r0 v[$r63+0x80] */
	OUT_RING  (chan, 0xfc001e86);
	OUT_RING  (chan, 0x80120000); /* tex { _ _ _ $r0 } $t0 { $r0 $r1 } */
	OUT_RING  (chan, 0x0000dde4);
	OUT_RING  (chan, 0x28000000); /* mov b32 $r3 $r0 */
	OUT_RING  (chan, 0x00009de4);
	OUT_RING  (chan, 0x28000000); /* mov b32 $r2 $r0 */
	OUT_RING  (chan, 0x00005de4);
	OUT_RING  (chan, 0x28000000); /* mov b32 $r1 $r0 */
	OUT_RING  (chan, 0x00001de7);
	OUT_RING  (chan, 0x80000000); /* exit */

	BEGIN_RING(chan, m2mf, NVC0_M2MF_OFFSET_OUT_HIGH, 2);
	if (OUT_RELOCh(chan, bo, PFP_C_A8, NOUVEAU_BO(VRAM, VRAM, WR)) ||
	    OUT_RELOCl(chan, bo, PFP_C_A8, NOUVEAU_BO(VRAM, VRAM, WR))) {
		MARK_UNDO(chan);
		return FALSE;
	}
	BEGIN_RING(chan, m2mf, NVC0_M2MF_LINE_LENGTH_IN, 2);
	OUT_RING  (chan, 13 * 8 + 20 * 4);
	OUT_RING  (chan, 1);
	BEGIN_RING(chan, m2mf, NVC0_M2MF_EXEC, 1);
	OUT_RING  (chan, 0x100111);
	BEGIN_RING_NI(chan, m2mf, NVC0_M2MF_DATA, 13 * 2 + 20);
	OUT_RING  (chan, 0x00021462);
	OUT_RING  (chan, 0x00000000);
	OUT_RING  (chan, 0x00000000);
	OUT_RING  (chan, 0x00000000);
	OUT_RING  (chan, 0x00000000);
	OUT_RING  (chan, 0x80000000);
	OUT_RING  (chan, 0x00000a0a);
	OUT_RING  (chan, 0x00000000);
	OUT_RING  (chan, 0x00000000);
	OUT_RING  (chan, 0x00000000);
	OUT_RING  (chan, 0x00000000);
	OUT_RING  (chan, 0x00000000);
	OUT_RING  (chan, 0x00000000);
	OUT_RING  (chan, 0x00000000);
	OUT_RING  (chan, 0x00000000);
	OUT_RING  (chan, 0x00000000);
	OUT_RING  (chan, 0x00000000);
	OUT_RING  (chan, 0x00000000);
	OUT_RING  (chan, 0x0000000f);
	OUT_RING  (chan, 0x00000000);
	OUT_RING  (chan, 0xfff01c00);
	OUT_RING  (chan, 0xc07e007c); /* linterp f32 $r0 v[$r63+0x7c] */
	OUT_RING  (chan, 0x10001c00);
	OUT_RING  (chan, 0xc8000000); /* rcp f32 $r0 $r0 */
	OUT_RING  (chan, 0x03f0dc40);
	OUT_RING  (chan, 0xc07e0094); /* pinterp f32 $r3 $r0 v[$r63+0x94] */
	OUT_RING  (chan, 0x03f09c40);
	OUT_RING  (chan, 0xc07e0090); /* pinterp f32 $r2 $r0 v[$r63+0x90] */
	OUT_RING  (chan, 0xfc205e86);
	OUT_RING  (chan, 0x80120001); /* tex { _ _ _ $r1 } $t1 { $r2 $r3 } */
	OUT_RING  (chan, 0x03f0dc40);
	OUT_RING  (chan, 0xc07e0084); /* pinterp f32 $r3 $r0 v[$r63+0x84] */
	OUT_RING  (chan, 0x03f09c40);
	OUT_RING  (chan, 0xc07e0080); /* pinterp f32 $r2 $r0 v[$r63+0x80] */
	OUT_RING  (chan, 0xfc201e86);
	OUT_RING  (chan, 0x80120000); /* tex { _ _ _ $r0 } $t0 { $r2 $r3 } */
	OUT_RING  (chan, 0x0400dc40);
	OUT_RING  (chan, 0x58000000); /* mul ftz rn f32 $r3 $r0 $r1 */
	OUT_RING  (chan, 0x0c009de4);
	OUT_RING  (chan, 0x28000000); /* mov b32 $r2 $r3 */
	OUT_RING  (chan, 0x0c005de4);
	OUT_RING  (chan, 0x28000000); /* mov b32 $r1 $r3 */
	OUT_RING  (chan, 0x0c001de4);
	OUT_RING  (chan, 0x28000000); /* mov b32 $r0 $r3 */
	OUT_RING  (chan, 0x00001de7);
	OUT_RING  (chan, 0x80000000); /* exit */
	FIRE_RING (chan);

	BEGIN_RING(chan, m2mf, NVC0_M2MF_OFFSET_OUT_HIGH, 2);
	if (OUT_RELOCh(chan, bo, PFP_NV12, NOUVEAU_BO(VRAM, VRAM, WR)) ||
	    OUT_RELOCl(chan, bo, PFP_NV12, NOUVEAU_BO(VRAM, VRAM, WR))) {
		MARK_UNDO(chan);
		return FALSE;
	}
	BEGIN_RING(chan, m2mf, NVC0_M2MF_LINE_LENGTH_IN, 2);
	OUT_RING  (chan, 19 * 8 + 20 * 4);
	OUT_RING  (chan, 1);
	BEGIN_RING(chan, m2mf, NVC0_M2MF_EXEC, 1);
	OUT_RING  (chan, 0x100111);
	BEGIN_RING_NI(chan, m2mf, NVC0_M2MF_DATA, 19 * 2 + 20);
	OUT_RING  (chan, 0x00021462);
	OUT_RING  (chan, 0x00000000);
	OUT_RING  (chan, 0x00000000);
	OUT_RING  (chan, 0x00000000);
	OUT_RING  (chan, 0x00000000);
	OUT_RING  (chan, 0x80000000);
	OUT_RING  (chan, 0x00000a0a);
	OUT_RING  (chan, 0x00000000);
	OUT_RING  (chan, 0x00000000);
	OUT_RING  (chan, 0x00000000);
	OUT_RING  (chan, 0x00000000);
	OUT_RING  (chan, 0x00000000);
	OUT_RING  (chan, 0x00000000);
	OUT_RING  (chan, 0x00000000);
	OUT_RING  (chan, 0x00000000);
	OUT_RING  (chan, 0x00000000);
	OUT_RING  (chan, 0x00000000);
	OUT_RING  (chan, 0x00000000);
	OUT_RING  (chan, 0x0000000f);
	OUT_RING  (chan, 0x00000000);
	OUT_RING  (chan, 0xfff09c00);
	OUT_RING  (chan, 0xc07e007c);
	OUT_RING  (chan, 0x10209c00);
	OUT_RING  (chan, 0xc8000000);
	OUT_RING  (chan, 0x0bf01c40);
	OUT_RING  (chan, 0xc07e0080);
	OUT_RING  (chan, 0x0bf05c40);
	OUT_RING  (chan, 0xc07e0084);
	OUT_RING  (chan, 0xfc001e86);
	OUT_RING  (chan, 0x80120000);
	OUT_RING  (chan, 0x00015c40);
	OUT_RING  (chan, 0x58004000);
	OUT_RING  (chan, 0x1050dc20);
	OUT_RING  (chan, 0x50004000);
	OUT_RING  (chan, 0x20511c20);
	OUT_RING  (chan, 0x50004000);
	OUT_RING  (chan, 0x30515c20);
	OUT_RING  (chan, 0x50004000);
	OUT_RING  (chan, 0x0bf01c40);
	OUT_RING  (chan, 0xc07e0090);
	OUT_RING  (chan, 0x0bf05c40);
	OUT_RING  (chan, 0xc07e0094);
	OUT_RING  (chan, 0xfc001e86);
	OUT_RING  (chan, 0x80130001);
	OUT_RING  (chan, 0x4000dc40);
	OUT_RING  (chan, 0x30064000);
	OUT_RING  (chan, 0x50011c40);
	OUT_RING  (chan, 0x30084000);
	OUT_RING  (chan, 0x60015c40);
	OUT_RING  (chan, 0x300a4000);
	OUT_RING  (chan, 0x70101c40);
	OUT_RING  (chan, 0x30064000);
	OUT_RING  (chan, 0x90109c40);
	OUT_RING  (chan, 0x300a4000);
	OUT_RING  (chan, 0x80105c40);
	OUT_RING  (chan, 0x30084000);
	OUT_RING  (chan, 0x00001de7);
	OUT_RING  (chan, 0x80000000);

	BEGIN_RING(chan, fermi, 0x021c, 1); /* CODE_FLUSH ? */
	OUT_RING  (chan, 0x1111);
	FIRE_RING (chan);

	BEGIN_RING(chan, fermi, NVC0_3D_SP_SELECT(5), 2);
	OUT_RING  (chan, 0x51);
	OUT_RING  (chan, PFP_S);
	BEGIN_RING(chan, fermi, NVC0_3D_SP_GPR_ALLOC(5), 1);
	OUT_RING  (chan, 8);

	BEGIN_RING(chan, fermi, NVC0_3D_CB_SIZE, 3);
	OUT_RING  (chan, 256);
	if (OUT_RELOCh(chan, bo, CB_OFFSET, NOUVEAU_BO_VRAM | NOUVEAU_BO_WR) ||
	    OUT_RELOCl(chan, bo, CB_OFFSET, NOUVEAU_BO_VRAM | NOUVEAU_BO_WR)) {
		MARK_UNDO(chan);
		return FALSE;
	}
	BEGIN_RING(chan, fermi, NVC0_3D_CB_BIND(4), 1);
	OUT_RING  (chan, 0x01);

	BEGIN_RING(chan, fermi, NVC0_3D_EARLY_FRAGMENT_TESTS, 1);
	OUT_RING  (chan, 0);
	BEGIN_RING(chan, fermi, 0x0360, 2);
	OUT_RING  (chan, 0x20164010);
	OUT_RING  (chan, 0x20);
	BEGIN_RING(chan, fermi, 0x196c, 1);
	OUT_RING  (chan, 0);
	BEGIN_RING(chan, fermi, 0x1664, 1);
	OUT_RING  (chan, 0);
	BEGIN_RING(chan, fermi, NVC0_3D_FRAG_COLOR_CLAMP_EN, 1);
	OUT_RING  (chan, 0x11111111);

	BEGIN_RING(chan, fermi, NVC0_3D_DEPTH_TEST_ENABLE, 1);
	OUT_RING  (chan, 0);

	BEGIN_RING(chan, fermi, NVC0_3D_RASTERIZE_ENABLE, 1);
	OUT_RING  (chan, 1);
	BEGIN_RING(chan, fermi, NVC0_3D_SP_SELECT(4), 1);
	OUT_RING  (chan, 0x40);
	BEGIN_RING(chan, fermi, NVC0_3D_GP_BUILTIN_RESULT_EN, 1);
	OUT_RING  (chan, 0);
	BEGIN_RING(chan, fermi, NVC0_3D_SP_SELECT(3), 1);
	OUT_RING  (chan, 0x30);
	BEGIN_RING(chan, fermi, NVC0_3D_SP_SELECT(2), 1);
	OUT_RING  (chan, 0x20);
	BEGIN_RING(chan, fermi, NVC0_3D_SP_SELECT(0), 1);
	OUT_RING  (chan, 0x00);

	BEGIN_RING(chan, fermi, 0x1604, 1);
	OUT_RING  (chan, 4);
	BEGIN_RING(chan, fermi, NVC0_3D_POINT_SPRITE_ENABLE, 1);
	OUT_RING  (chan, 0);
	BEGIN_RING(chan, fermi, NVC0_3D_SCISSOR_ENABLE(0), 1);
	OUT_RING  (chan, 1);

	BEGIN_RING(chan, fermi, NVC0_3D_VIEWPORT_HORIZ(0), 2);
	OUT_RING  (chan, (8192 << 16) | 0);
	OUT_RING  (chan, (8192 << 16) | 0);
	BEGIN_RING(chan, fermi, NVC0_3D_SCISSOR_HORIZ(0), 2);
	OUT_RING  (chan, (8192 << 16) | 0);
	OUT_RING  (chan, (8192 << 16) | 0);
	FIRE_RING (chan);

	return TRUE;
}

