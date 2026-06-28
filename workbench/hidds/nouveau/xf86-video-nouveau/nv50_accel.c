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
#include "nv50_accel.h"

#include "hwdefs/nv_object.xml.h"
#include "hwdefs/nv50_2d.xml.h"
#include "hwdefs/nv50_3d.xml.h"

#if !defined(__AROS__)
void
NV50SyncToVBlank(PixmapPtr ppix, BoxPtr box)
{
	ScrnInfoPtr pScrn = xf86ScreenToScrn(ppix->drawable.pScreen);
	NVPtr pNv = NVPTR(pScrn);
	struct nouveau_pushbuf *push = pNv->pushbuf;
	int head;
	xf86CrtcPtr crtc;

	if (!nouveau_exa_pixmap_is_onscreen(ppix))
		return;

	crtc = nouveau_pick_best_crtc(pScrn, FALSE, box->x1, box->y1,
                                  box->x2 - box->x1,
                                  box->y2 - box->y1);
	if (!crtc)
		return;

	if (!PUSH_SPACE(push, 10))
		return;

	head = drmmode_head(crtc);

	BEGIN_NV04(push, SUBC_NVSW(0x0060), 2);
	PUSH_DATA (push, pNv->vblank_sem->handle);
	PUSH_DATA (push, 0);
	BEGIN_NV04(push, SUBC_NVSW(0x006c), 1);
	PUSH_DATA (push, 0x22222222);
	BEGIN_NV04(push, SUBC_NVSW(0x0404), 2);
	PUSH_DATA (push, 0x11111111);
	PUSH_DATA (push, head);
	BEGIN_NV04(push, SUBC_NVSW(0x0068), 1);
	PUSH_DATA (push, 0x11111111);
}
#endif

Bool
NVAccelInitM2MF_NV50(ScrnInfoPtr pScrn)
{
	NVPtr pNv = NVPTR(pScrn);
	struct nouveau_pushbuf *push = pNv->pushbuf;
	struct nv04_fifo *fifo = pNv->channel->data;

	if (nouveau_object_new(pNv->channel, NvMemFormat, NV50_M2MF_CLASS,
			       NULL, 0, &pNv->NvMemFormat))
		return FALSE;

	if (!PUSH_SPACE(push, 8))
		return FALSE;

	BEGIN_NV04(push, NV01_SUBC(M2MF, OBJECT), 1);
	PUSH_DATA (push, pNv->NvMemFormat->handle);
	BEGIN_NV04(push, NV03_M2MF(DMA_NOTIFY), 1);
	PUSH_DATA (push, pNv->notify0->handle);
	BEGIN_NV04(push, NV03_M2MF(DMA_BUFFER_IN), 2);
	PUSH_DATA (push, fifo->vram);
	PUSH_DATA (push, fifo->vram);
	return TRUE;
}

Bool
NVAccelInit2D_NV50(ScrnInfoPtr pScrn)
{
	NVPtr pNv = NVPTR(pScrn);
	struct nouveau_pushbuf *push = pNv->pushbuf;
	struct nv04_fifo *fifo = pNv->channel->data;

	if (nouveau_object_new(pNv->channel, Nv2D, NV50_2D_CLASS,
			       NULL, 0, &pNv->Nv2D))
		return FALSE;

	if (!PUSH_SPACE(push, 64))
		return FALSE;

	BEGIN_NV04(push, NV01_SUBC(2D, OBJECT), 1);
	PUSH_DATA (push, pNv->Nv2D->handle);
	BEGIN_NV04(push, NV50_2D(DMA_NOTIFY), 3);
	PUSH_DATA (push, pNv->notify0->handle);
	PUSH_DATA (push, fifo->vram);
	PUSH_DATA (push, fifo->vram);

	/* Magics from nv, no clue what they do, but at least some
	 * of them are needed to avoid crashes.
	 */
	BEGIN_NV04(push, NV50_2D(UNK260), 1);
	PUSH_DATA (push, 1);
	BEGIN_NV04(push, NV50_2D(CLIP_ENABLE), 1);
	PUSH_DATA (push, 1);
	BEGIN_NV04(push, NV50_2D(COLOR_KEY_ENABLE), 1);
	PUSH_DATA (push, 0);
	BEGIN_NV04(push, NV50_2D(UNK58C), 1);
	PUSH_DATA (push, 0x111);

	pNv->currentRop = 0xfffffffa;
	return TRUE;
}

Bool
NVAccelInitNV50TCL(ScrnInfoPtr pScrn)
{
	NVPtr pNv = NVPTR(pScrn);
	struct nv04_fifo *fifo = pNv->channel->data;
	struct nouveau_pushbuf *push = pNv->pushbuf;
	struct nv04_notify ntfy = { .length = 32 };
	unsigned class;
	int i;

	switch (pNv->dev->chipset & 0xf0) {
	case 0x50:
		class = NV50_3D_CLASS;
		break;
	case 0x80:
	case 0x90:
		class = NV84_3D_CLASS;
		break;
	case 0xa0:
		switch (pNv->dev->chipset) {
		case 0xa0:
		case 0xaa:
		case 0xac:
			class = NVA0_3D_CLASS;
			break;
		case 0xaf:
			class = NVAF_3D_CLASS;
			break;
		default:
			class = NVA3_3D_CLASS;
			break;
		}
		break;
	default:
		return FALSE;
	}

	if (nouveau_object_new(pNv->channel, Nv3D, class, NULL, 0, &pNv->Nv3D))
		return FALSE;

	if (nouveau_object_new(pNv->channel, NvSW, 0x506e, NULL, 0, &pNv->NvSW)) {
		nouveau_object_del(&pNv->Nv3D);
		return FALSE;
	}

	if (nouveau_object_new(pNv->channel, NvVBlankSem, NOUVEAU_NOTIFIER_CLASS,
			       &ntfy, sizeof(ntfy), &pNv->vblank_sem)) {
		nouveau_object_del(&pNv->NvSW);
		nouveau_object_del(&pNv->Nv3D);
		return FALSE;
	}

	if (nouveau_pushbuf_space(push, 512, 0, 0) ||
	    nouveau_pushbuf_refn (push, &(struct nouveau_pushbuf_refn) {
					pNv->scratch, NOUVEAU_BO_VRAM |
					NOUVEAU_BO_WR }, 1))
		return FALSE;

	BEGIN_NV04(push, NV01_SUBC(NVSW, OBJECT), 1);
	PUSH_DATA (push, pNv->NvSW->handle);
	BEGIN_NV04(push, SUBC_NVSW(0x018c), 1);
	PUSH_DATA (push, pNv->vblank_sem->handle);
	BEGIN_NV04(push, SUBC_NVSW(0x0400), 1);
	PUSH_DATA (push, 0);

	BEGIN_NV04(push, NV01_SUBC(3D, OBJECT), 1);
	PUSH_DATA (push, pNv->Nv3D->handle);
	BEGIN_NV04(push, NV50_3D(COND_MODE), 1);
	PUSH_DATA (push, NV50_3D_COND_MODE_ALWAYS);
	BEGIN_NV04(push, NV50_3D(DMA_NOTIFY), 1);
	PUSH_DATA (push, pNv->NvNull->handle);
	BEGIN_NV04(push, NV50_3D(DMA_ZETA), 11);
	for (i = 0; i < 11; i++)
		PUSH_DATA (push, fifo->vram);
	BEGIN_NV04(push, NV50_3D(DMA_COLOR(0)), NV50_3D_DMA_COLOR__LEN);
	for (i = 0; i < NV50_3D_DMA_COLOR__LEN; i++)
		PUSH_DATA (push, fifo->vram);
	BEGIN_NV04(push, NV50_3D(RT_CONTROL), 1);
	PUSH_DATA (push, 1);

	BEGIN_NV04(push, NV50_3D(VIEWPORT_TRANSFORM_EN), 1);
	PUSH_DATA (push, 0);
	BEGIN_NV04(push, NV50_3D(COLOR_MASK_COMMON), 1);
	PUSH_DATA (push, 1);
	BEGIN_NV04(push, NV50_3D(ZETA_ENABLE), 1);
	PUSH_DATA (push, 0);

	BEGIN_NV04(push, NV50_3D(TIC_ADDRESS_HIGH), 3);
	PUSH_DATA (push, (pNv->scratch->offset + TIC_OFFSET) >> 32);
	PUSH_DATA (push, (pNv->scratch->offset + TIC_OFFSET));
	PUSH_DATA (push, 0x00000800);
	BEGIN_NV04(push, NV50_3D(TSC_ADDRESS_HIGH), 3);
	PUSH_DATA (push, (pNv->scratch->offset + TSC_OFFSET) >> 32);
	PUSH_DATA (push, (pNv->scratch->offset + TSC_OFFSET));
	PUSH_DATA (push, 0x00000000);
	BEGIN_NV04(push, NV50_3D(LINKED_TSC), 1);
	PUSH_DATA (push, 1);
	BEGIN_NV04(push, NV50_3D(TEX_LIMITS(2)), 1);
	PUSH_DATA (push, 0x54);

	PUSH_DATAu(push, pNv->scratch, PVP_OFFSET, 30 * 2);
	PUSH_DATA (push, 0x10000001);
	PUSH_DATA (push, 0x0423c788); /* mov b32 o[0x0] a[0x0] */
	PUSH_DATA (push, 0x10000205);
	PUSH_DATA (push, 0x0423c788); /* mov b32 o[0x4] a[0x4] */
	PUSH_DATA (push, 0xc0800401);
	PUSH_DATA (push, 0x00200780); /* mul rn f32 $r0 a[0x8] c0[0x0] */
	PUSH_DATA (push, 0xc0830405);
	PUSH_DATA (push, 0x00200780); /* mul rn f32 $r1 a[0x8] c0[0xc] */
	PUSH_DATA (push, 0xc0860409);
	PUSH_DATA (push, 0x00200780); /* mul rn f32 $r2 a[0x8] c0[0x18] */
	PUSH_DATA (push, 0xe0810601);
	PUSH_DATA (push, 0x00200780); /* add f32 $r0 (mul a[0xc] c0[0x4]) $r0 */
	PUSH_DATA (push, 0xe0840605);
	PUSH_DATA (push, 0x00204780); /* add f32 $r1 (mul a[0xc] c0[0x10]) $r1 */
	PUSH_DATA (push, 0xe0870609);
	PUSH_DATA (push, 0x00208780); /* add f32 $r2 (mul a[0xc] c0[0x1c]) $r2 */
	PUSH_DATA (push, 0xb1000001);
	PUSH_DATA (push, 0x00008780); /* add rn f32 $r0 $r0 c0[0x8] */
	PUSH_DATA (push, 0xb1000205);
	PUSH_DATA (push, 0x00014780); /* add rn f32 $r1 $r1 c0[0x14] */
	PUSH_DATA (push, 0xb1000409);
	PUSH_DATA (push, 0x00020780); /* add rn f32 $r2 $r2 c0[0x20] */
	PUSH_DATA (push, 0x90000409);
	PUSH_DATA (push, 0x00000780); /* rcp f32 $r2 $r2 */
	PUSH_DATA (push, 0xc0020001);
	PUSH_DATA (push, 0x00000780); /* mul rn f32 $r0 $r0 $r2 */
	PUSH_DATA (push, 0xc0020205);
	PUSH_DATA (push, 0x00000780); /* mul rn f32 $r1 $r1 $r2 */
	PUSH_DATA (push, 0xc0890009);
	PUSH_DATA (push, 0x00000788); /* mul rn f32 o[0x8] $r0 c0[0x24] */
	PUSH_DATA (push, 0xc08a020d);
	PUSH_DATA (push, 0x00000788); /* mul rn f32 o[0xc] $r1 c0[0x28] */
	PUSH_DATA (push, 0xc08b0801);
	PUSH_DATA (push, 0x00200780); /* mul rn f32 $r0 a[0x10] c0[0x2c] */
	PUSH_DATA (push, 0xc08e0805);
	PUSH_DATA (push, 0x00200780); /* mul rn f32 $r1 a[0x10] c0[0x38] */
	PUSH_DATA (push, 0xc0910809);
	PUSH_DATA (push, 0x00200780); /* mul rn f32 $r2 a[0x10] c0[0x44] */
	PUSH_DATA (push, 0xe08c0a01);
	PUSH_DATA (push, 0x00200780); /* add f32 $r0 (mul a[0x14] c0[0x30]) $r0 */
	PUSH_DATA (push, 0xe08f0a05);
	PUSH_DATA (push, 0x00204780); /* add f32 $r1 (mul a[0x14] c0[0x3c]) $r1 */
	PUSH_DATA (push, 0xe0920a09);
	PUSH_DATA (push, 0x00208780); /* add f32 $r2 (mul a[0x14] c0[0x48]) $r2 */
	PUSH_DATA (push, 0xb1000001);
	PUSH_DATA (push, 0x00034780); /* add rn f32 $r0 $r0 c0[0x34] */
	PUSH_DATA (push, 0xb1000205);
	PUSH_DATA (push, 0x00040780); /* add rn f32 $r1 $r1 c0[0x40] */
	PUSH_DATA (push, 0xb1000409);
	PUSH_DATA (push, 0x0004c780); /* add rn f32 $r2 $r2 c0[0x4c] */
	PUSH_DATA (push, 0x90000409);
	PUSH_DATA (push, 0x00000780); /* rcp f32 $r2 $r2 */
	PUSH_DATA (push, 0xc0020001);
	PUSH_DATA (push, 0x00000780); /* mul rn f32 $r0 $r0 $r2 */
	PUSH_DATA (push, 0xc0020205);
	PUSH_DATA (push, 0x00000780); /* mul rn f32 $r1 $r1 $r2 */
	PUSH_DATA (push, 0xc0940011);
	PUSH_DATA (push, 0x00000788); /* mul rn f32 o[0x10] $r0 c0[0x50] */
	PUSH_DATA (push, 0xc0950215);
	PUSH_DATA (push, 0x00000789); /* exit mul rn f32 o[0x14] $r1 c0[0x54] */

	/* fetch only VTX_ATTR[0,8,9].xy */
	BEGIN_NV04(push, NV50_3D(VP_ATTR_EN(0)), 2);
	PUSH_DATA (push, 0x00000003);
	PUSH_DATA (push, 0x00000033);
	BEGIN_NV04(push, NV50_3D(VP_REG_ALLOC_RESULT), 1);
	PUSH_DATA (push, 6);
	BEGIN_NV04(push, NV50_3D(VP_RESULT_MAP_SIZE), 2);
	PUSH_DATA (push, 8);
	PUSH_DATA (push, 4); /* NV50_3D_VP_REG_ALLOC_TEMP */
	BEGIN_NV04(push, NV50_3D(VP_ADDRESS_HIGH), 2);
	PUSH_DATA (push, (pNv->scratch->offset + PVP_OFFSET) >> 32);
	PUSH_DATA (push, (pNv->scratch->offset + PVP_OFFSET));
	BEGIN_NV04(push, NV50_3D(CB_DEF_ADDRESS_HIGH), 3);
	PUSH_DATA (push, (pNv->scratch->offset + PVP_DATA) >> 32);
	PUSH_DATA (push, (pNv->scratch->offset + PVP_DATA));
	PUSH_DATA (push, (CB_PVP << NV50_3D_CB_DEF_SET_BUFFER__SHIFT) | 256);
	BEGIN_NV04(push, NV50_3D(SET_PROGRAM_CB), 1);
	PUSH_DATA (push, 0x00000001 | (CB_PVP << 12));
	BEGIN_NV04(push, NV50_3D(VP_START_ID), 1);
	PUSH_DATA (push, 0);

	PUSH_DATAu(push, pNv->scratch, PFP_OFFSET + PFP_S, 6);
	PUSH_DATA (push, 0x80000000); /* interp $r0 v[0x0] */
	PUSH_DATA (push, 0x90000004); /* rcp f32 $r1 $r0 */
	PUSH_DATA (push, 0x82010200); /* interp $r0 v[0x4] $r1 */
	PUSH_DATA (push, 0x82020204); /* interp $r1 v[0x8] $r1 */
	PUSH_DATA (push, 0xf6400001);
	PUSH_DATA (push, 0x0000c785); /* exit texauto live $r0:$r1:$r2:$r3 $t0 $s0 $r0:$r1 0x0 0x0 0x0 */
	PUSH_DATAu(push, pNv->scratch, PFP_OFFSET + PFP_C, 16);
	PUSH_DATA (push, 0x80000000); /* interp $r0 v[0x0] */
	PUSH_DATA (push, 0x90000004); /* rcp f32 $r1 $r0 */
	PUSH_DATA (push, 0x82030210); /* interp $r4 v[0xc] $r1 */
	PUSH_DATA (push, 0x82040214); /* interp $r5 v[0x10] $r1 */
	PUSH_DATA (push, 0x82010200); /* interp $r0 v[0x4] $r1 */
	PUSH_DATA (push, 0x82020204); /* interp $r1 v[0x8] $r1 */
	PUSH_DATA (push, 0xf6400001);
	PUSH_DATA (push, 0x0000c784); /* texauto live $r0:$r1:$r2:$r3 $t0 $s0 $r0:$r1 0x0 0x0 0x0 */
	PUSH_DATA (push, 0xf0400211);
	PUSH_DATA (push, 0x00008784); /* texauto live #:#:#:$r4 $t1 $s0 $r4:$r5 0x0 0x0 0x0 */
	PUSH_DATA (push, 0xc0040000); /* mul f32 $r0 $r0 $r4 */
	PUSH_DATA (push, 0xc0040204); /* mul f32 $r1 $r1 $r4 */
	PUSH_DATA (push, 0xc0040409);
	PUSH_DATA (push, 0x00000780); /* mul rn f32 $r2 $r2 $r4 */
	PUSH_DATA (push, 0xc004060d);
	PUSH_DATA (push, 0x00000781); /* exit mul rn f32 $r3 $r3 $r4 */
	PUSH_DATAu(push, pNv->scratch, PFP_OFFSET + PFP_CCA, 16);
	PUSH_DATA (push, 0x80000000); /* interp $r0 v[0x0] */
	PUSH_DATA (push, 0x90000004); /* rcp f32 $r1 $r0 */
	PUSH_DATA (push, 0x82030210); /* interp $r4 v[0xc] $r1 */
	PUSH_DATA (push, 0x82040214); /* interp $r5 v[0x10] $r1 */
	PUSH_DATA (push, 0x82010200); /* interp $r0 v[0x4] $r1 */
	PUSH_DATA (push, 0x82020204); /* interp $r1 v[0x8] $r1 */
	PUSH_DATA (push, 0xf6400001);
	PUSH_DATA (push, 0x0000c784); /* texauto live $r0:$r1:$r2:$r3 $t0 $s0 $r0:$r1 0x0 0x0 0x0 */
	PUSH_DATA (push, 0xf6400211);
	PUSH_DATA (push, 0x0000c784); /* texauto live $r4:$r5:$r6:$r7 $t1 $s0 $r4:$r5 0x0 0x0 0x0 */
	PUSH_DATA (push, 0xc0040000); /* mul f32 $r0 $r0 $r4 */
	PUSH_DATA (push, 0xc0050204); /* mul f32 $r1 $r1 $r5 */
	PUSH_DATA (push, 0xc0060409);
	PUSH_DATA (push, 0x00000780); /* mul rn f32 $r2 $r2 $r6 */
	PUSH_DATA (push, 0xc007060d);
	PUSH_DATA (push, 0x00000781); /* exit mul rn f32 $r3 $r3 $r7 */
	PUSH_DATAu(push, pNv->scratch, PFP_OFFSET + PFP_CCASA, 16);
	PUSH_DATA (push, 0x80000000); /* interp $r0 v[0x0] */
	PUSH_DATA (push, 0x90000004); /* rcp f32 $r1 $r0 */
	PUSH_DATA (push, 0x82030200); /* interp $r0 v[0xc] $r1 */
	PUSH_DATA (push, 0x82040204); /* interp $r1 v[0x10] $r1 */
	PUSH_DATA (push, 0x82010210); /* interp $r4 v[0x4] $r1 */
	PUSH_DATA (push, 0x82020214); /* interp $r5 v[0x8] $r1 */
	PUSH_DATA (push, 0xf6400201);
	PUSH_DATA (push, 0x0000c784); /* texauto live $r0:$r1:$r2:$r3 $t1 $s0 $r0:$r1 0x0 0x0 0x0 */
	PUSH_DATA (push, 0xf0400011);
	PUSH_DATA (push, 0x00008784); /* texauto live #:#:#:$r4 $t0 $s0 $r4:$r5 0x0 0x0 0x0 */
	PUSH_DATA (push, 0xc0040000); /* mul f32 $r0 $r0 $r4 */
	PUSH_DATA (push, 0xc0040204); /* mul f32 $r1 $r1 $r4 */
	PUSH_DATA (push, 0xc0040409);
	PUSH_DATA (push, 0x00000780); /* mul rn f32 $r2 $r2 $r4 */
	PUSH_DATA (push, 0xc004060d);
	PUSH_DATA (push, 0x00000781); /* exit mul rn f32 $r3 $r3 $r4 */
	PUSH_DATAu(push, pNv->scratch, PFP_OFFSET + PFP_S_A8, 10);
	PUSH_DATA (push, 0x80000000); /* interp $r0 v[0x0] */
	PUSH_DATA (push, 0x90000004); /* rcp f32 $r1 $r0 */
	PUSH_DATA (push, 0x82010200); /* interp $r0 v[0x4] $r1 */
	PUSH_DATA (push, 0x82020204); /* interp $r1 v[0x8] $r1 */
	PUSH_DATA (push, 0xf0400001);
	PUSH_DATA (push, 0x00008784); /* texauto live #:#:#:$r0 $t0 $s0 $r0:$r1 0x0 0x0 0x0 */
	PUSH_DATA (push, 0x10008004); /* mov b32 $r1 $r0 */
	PUSH_DATA (push, 0x10008008); /* mov b32 $r2 $r0 */
	PUSH_DATA (push, 0x1000000d);
	PUSH_DATA (push, 0x0403c781); /* exit mov b32 $r3 $r0 */
	PUSH_DATAu(push, pNv->scratch, PFP_OFFSET + PFP_C_A8, 16);
	PUSH_DATA (push, 0x80000000); /* interp $r0 v[0x0] */
	PUSH_DATA (push, 0x90000004); /* rcp f32 $r1 $r0 */
	PUSH_DATA (push, 0x82030208); /* interp $r2 v[0xc] $r1 */
	PUSH_DATA (push, 0x8204020c); /* interp $r3 v[0x10] $r1 */
	PUSH_DATA (push, 0x82010200); /* interp $r0 v[0x4] $r1 */
	PUSH_DATA (push, 0x82020204); /* interp $r1 v[0x8] $r1 */
	PUSH_DATA (push, 0xf0400001);
	PUSH_DATA (push, 0x00008784); /* texauto live #:#:#:$r0 $t0 $s0 $r0:$r1 0x0 0x0 0x0 */
	PUSH_DATA (push, 0xf0400209);
	PUSH_DATA (push, 0x00008784); /* texauto live #:#:#:$r2 $t1 $s0 $r2:$r3 0x0 0x0 0x0 */
	PUSH_DATA (push, 0xc002000d);
	PUSH_DATA (push, 0x00000780); /* mul rn f32 $r3 $r0 $r2 */
	PUSH_DATA (push, 0x10008600); /* mov b32 $r0 $r3 */
	PUSH_DATA (push, 0x10008604); /* mov b32 $r1 $r3 */
	PUSH_DATA (push, 0x10000609);
	PUSH_DATA (push, 0x0403c781); /* exit mov b32 $r2 $r3 */
	PUSH_DATAu(push, pNv->scratch, PFP_OFFSET + PFP_NV12, 24);
	PUSH_DATA (push, 0x80000008); /* interp $r2 v[0x0] */
	PUSH_DATA (push, 0x90000408); /* rcp f32 $r2 $r2 */
	PUSH_DATA (push, 0x82010400); /* interp $r0 v[0x4] $r2 */
	PUSH_DATA (push, 0x82020404); /* interp $r1 v[0x8] $r2 */
	PUSH_DATA (push, 0xf0400001);
	PUSH_DATA (push, 0x00008784); /* texauto live #:#:#:$r0 $t0 $s0 $r0:$r1 0x0 0x0 0x0 */
	PUSH_DATA (push, 0xc0800014); /* mul f32 $r5 $r0 c0[0x0] */
	PUSH_DATA (push, 0xb0810a0c); /* add f32 $r3 $r5 c0[0x4] */
	PUSH_DATA (push, 0xb0820a10); /* add f32 $r4 $r5 c0[0x8] */
	PUSH_DATA (push, 0xb0830a14); /* add f32 $r5 $r5 c0[0xc] */
	PUSH_DATA (push, 0x82010400); /* interp $r0 v[0x4] $r2 */
	PUSH_DATA (push, 0x82020404); /* interp $r1 v[0x8] $r2 */
	PUSH_DATA (push, 0xf0400201);
	PUSH_DATA (push, 0x0000c784); /* texauto live #:#:$r0:$r1 $t1 $s0 $r0:$r1 0x0 0x0 0x0 */
	PUSH_DATA (push, 0xe084000c); /* add f32 $r3 (mul $r0 c0[0x10]) $r3 */
	PUSH_DATA (push, 0xe0850010); /* add f32 $r4 (mul $r0 c0[0x14]) $r4 */
	PUSH_DATA (push, 0xe0860015);
	PUSH_DATA (push, 0x00014780); /* add f32 $r5 (mul $r0 c0[0x18]) $r5 */
	PUSH_DATA (push, 0xe0870201);
	PUSH_DATA (push, 0x0000c780); /* add f32 $r0 (mul $r1 c0[0x1c]) $r3 */
	PUSH_DATA (push, 0xe0890209);
	PUSH_DATA (push, 0x00014780); /* add f32 $r2 (mul $r1 c0[0x24]) $r5 */
	PUSH_DATA (push, 0xe0880205);
	PUSH_DATA (push, 0x00010781); /* exit add f32 $r1 (mul $r1 c0[0x20]) $r4 */

	/* HPOS.xy = ($o0, $o1), HPOS.zw = (0.0, 1.0), then map $o2 - $o5 */
	BEGIN_NV04(push, NV50_3D(VP_RESULT_MAP(0)), 2);
	PUSH_DATA (push, 0x41400100);
	PUSH_DATA (push, 0x05040302);
	BEGIN_NV04(push, NV50_3D(POINT_SPRITE_ENABLE), 1);
	PUSH_DATA (push, 0x00000000);
	BEGIN_NV04(push, NV50_3D(FP_INTERPOLANT_CTRL), 2);
	PUSH_DATA (push, 0x08040404);
	PUSH_DATA (push, 0x00000008); /* NV50_3D_FP_REG_ALLOC_TEMP */
	BEGIN_NV04(push, NV50_3D(FP_ADDRESS_HIGH), 2);
	PUSH_DATA (push, (pNv->scratch->offset + PFP_OFFSET) >> 32);
	PUSH_DATA (push, (pNv->scratch->offset + PFP_OFFSET));
	BEGIN_NV04(push, NV50_3D(CB_DEF_ADDRESS_HIGH), 3);
	PUSH_DATA (push, (pNv->scratch->offset + PFP_DATA) >> 32);
	PUSH_DATA (push, (pNv->scratch->offset + PFP_DATA));
	PUSH_DATA (push, (CB_PFP << NV50_3D_CB_DEF_SET_BUFFER__SHIFT) | 256);
	BEGIN_NV04(push, NV50_3D(SET_PROGRAM_CB), 1);
	PUSH_DATA (push, 0x00000031 | (CB_PFP << 12));

	BEGIN_NV04(push, NV50_3D(SCISSOR_ENABLE(0)), 1);
	PUSH_DATA (push, 1);

	BEGIN_NV04(push, NV50_3D(VIEWPORT_HORIZ(0)), 2);
	PUSH_DATA (push, 8192 << NV50_3D_VIEWPORT_HORIZ_W__SHIFT);
	PUSH_DATA (push, 8192 << NV50_3D_VIEWPORT_VERT_H__SHIFT);
	/* NV50_3D_SCISSOR_VERT_T_SHIFT is wrong, because it was deducted with
	 * origin lying at the bottom left. This will be changed to _MIN_ and _MAX_
	 * later, because it is origin dependent.
	 */
	BEGIN_NV04(push, NV50_3D(SCISSOR_HORIZ(0)), 2);
	PUSH_DATA (push, 8192 << NV50_3D_SCISSOR_HORIZ_MAX__SHIFT);
	PUSH_DATA (push, 8192 << NV50_3D_SCISSOR_VERT_MAX__SHIFT);
	BEGIN_NV04(push, NV50_3D(SCREEN_SCISSOR_HORIZ), 2);
	PUSH_DATA (push, 8192 << NV50_3D_SCREEN_SCISSOR_HORIZ_W__SHIFT);
	PUSH_DATA (push, 8192 << NV50_3D_SCREEN_SCISSOR_VERT_H__SHIFT);

	return TRUE;
}
