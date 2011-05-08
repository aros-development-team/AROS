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

#if !defined(__AROS__)
void
NV50SyncToVBlank(PixmapPtr ppix, BoxPtr box)
{
	ScrnInfoPtr pScrn = xf86Screens[ppix->drawable.pScreen->myNum];
	NVPtr pNv = NVPTR(pScrn);
	struct nouveau_channel *chan = pNv->chan;
	struct nouveau_grobj *nvsw = pNv->NvSW;
	int crtcs;

	if (!nouveau_exa_pixmap_is_onscreen(ppix))
		return;

	crtcs = nv_window_belongs_to_crtc(pScrn, box->x1, box->y1,
					  box->x2 - box->x1,
					  box->y2 - box->y1);
	if (!crtcs)
		return;

	BEGIN_RING(chan, nvsw, 0x0060, 2);
	OUT_RING  (chan, pNv->vblank_sem->handle);
	OUT_RING  (chan, 0);
	BEGIN_RING(chan, nvsw, 0x006c, 1);
	OUT_RING  (chan, 0x22222222);
	BEGIN_RING(chan, nvsw, 0x0404, 2);
	OUT_RING  (chan, 0x11111111);
	OUT_RING  (chan, ffs(crtcs) - 1);
	BEGIN_RING(chan, nvsw, 0x0068, 1);
	OUT_RING  (chan, 0x11111111);
}
#endif

Bool
NVAccelInitNV50TCL(ScrnInfoPtr pScrn)
{
	NVPtr pNv = NVPTR(pScrn);
	struct nouveau_channel *chan = pNv->chan;
	struct nouveau_grobj *tesla, *nvsw;
	unsigned class;
	int i;

	switch (pNv->dev->chipset & 0xf0) {
	case 0x50:
		class = NV50TCL;
		break;
	case 0x80:
	case 0x90:
		class = NV84TCL;
		break;
	case 0xa0:
		switch (pNv->dev->chipset) {
		case 0xa0:
		case 0xaa:
		case 0xac:
			class = NVA0TCL;
			break;
		case 0xaf:
			class = NVAFTCL;
			break;
		default:
			class = NVA8TCL;
			break;
		}
		break;
	default:
		return FALSE;
	}

	if (!pNv->Nv3D) {
		if (nouveau_grobj_alloc(chan, Nv3D, class, &pNv->Nv3D))
			return FALSE;

		if (nouveau_grobj_alloc(chan, NvSW, 0x506e, &pNv->NvSW)) {
			nouveau_grobj_free(&pNv->Nv3D);
			return FALSE;
		}

		if (nouveau_notifier_alloc(chan, NvVBlankSem, 1,
					   &pNv->vblank_sem)) {
			nouveau_grobj_free(&pNv->NvSW);
			nouveau_grobj_free(&pNv->Nv3D);
		}

		if (nouveau_bo_new(pNv->dev, NOUVEAU_BO_VRAM, 0, 65536,
				   &pNv->tesla_scratch)) {
			nouveau_notifier_free(&pNv->vblank_sem);
			nouveau_grobj_free(&pNv->NvSW);
			nouveau_grobj_free(&pNv->Nv3D);
			return FALSE;
		}
	}
	tesla = pNv->Nv3D;
	nvsw = pNv->NvSW;

	if (MARK_RING(chan, 512, 32))
		return FALSE;

	BEGIN_RING(chan, nvsw, 0x018c, 1);
	OUT_RING  (chan, pNv->vblank_sem->handle);
	BEGIN_RING(chan, nvsw, 0x0400, 1);
	OUT_RING  (chan, 0);

	BEGIN_RING(chan, tesla, NV50TCL_COND_MODE, 1);
	OUT_RING  (chan, NV50TCL_COND_MODE_ALWAYS);
	BEGIN_RING(chan, tesla, NV50TCL_DMA_NOTIFY, 1);
	OUT_RING  (chan, chan->nullobj->handle);
	BEGIN_RING(chan, tesla, NV50TCL_DMA_ZETA, 11);
	for (i = 0; i < 11; i++)
		OUT_RING  (chan, pNv->chan->vram->handle);
	BEGIN_RING(chan, tesla, NV50TCL_DMA_COLOR(0), NV50TCL_DMA_COLOR__SIZE);
	for (i = 0; i < NV50TCL_DMA_COLOR__SIZE; i++)
		OUT_RING  (chan, pNv->chan->vram->handle);
	BEGIN_RING(chan, tesla, NV50TCL_RT_CONTROL, 1);
	OUT_RING  (chan, 1);

	BEGIN_RING(chan, tesla, NV50TCL_VIEWPORT_TRANSFORM_EN, 1);
	OUT_RING  (chan, 0);
	BEGIN_RING(chan, tesla, 0x0f90, 1);
	OUT_RING  (chan, 1);

	BEGIN_RING(chan, tesla, NV50TCL_LINKED_TSC, 1);
	OUT_RING  (chan, 1);

	BEGIN_RING(chan, tesla, NV50TCL_TEX_LIMITS(2), 1);
	OUT_RING  (chan, 0x54);

	BEGIN_RING(chan, tesla, NV50TCL_CB_DEF_ADDRESS_HIGH, 3);
	if (OUT_RELOCh(chan, pNv->tesla_scratch, PVP_OFFSET,
		       NOUVEAU_BO_VRAM | NOUVEAU_BO_WR) ||
	    OUT_RELOCl(chan, pNv->tesla_scratch, PVP_OFFSET,
		       NOUVEAU_BO_VRAM | NOUVEAU_BO_WR)) {
		MARK_UNDO(chan);
		return FALSE;
	}
	OUT_RING  (chan, 0x00004000);
	BEGIN_RING(chan, tesla, NV50TCL_CB_ADDR, 1);
	OUT_RING  (chan, 0);
	BEGIN_RING_NI(chan, tesla, NV50TCL_CB_DATA(0), (3*2*2));
	OUT_RING  (chan, 0x10000001);
	OUT_RING  (chan, 0x0423c788);
	OUT_RING  (chan, 0x10000205);
	OUT_RING  (chan, 0x0423c788);
	OUT_RING  (chan, 0x10000409);
	OUT_RING  (chan, 0x0423c788);
	OUT_RING  (chan, 0x1000060d);
	OUT_RING  (chan, 0x0423c788);
	OUT_RING  (chan, 0x10000811);
	OUT_RING  (chan, 0x0423c788);
	OUT_RING  (chan, 0x10000a15);
	OUT_RING  (chan, 0x0423c789);

	/* fetch only VTX_ATTR[0,8,9].xy */
	BEGIN_RING(chan, tesla, NV50TCL_VP_ATTR_EN_0, 2);
	OUT_RING  (chan, 0x00000003);
	OUT_RING  (chan, 0x00000033);
	BEGIN_RING(chan, tesla, NV50TCL_VP_REG_ALLOC_RESULT, 1);
	OUT_RING  (chan, 6);
	if (tesla->grclass != 0x8597) {
		BEGIN_RING(chan, tesla, NV50TCL_VP_RESULT_MAP_SIZE, 2);
		OUT_RING  (chan, 8);
		OUT_RING  (chan, 0); /* NV50TCL_VP_REG_ALLOC_TEMP */
	} else {
		BEGIN_RING(chan, tesla, NV50TCL_VP_RESULT_MAP_SIZE, 1);
		OUT_RING  (chan, 8);
	}
	BEGIN_RING(chan, tesla, NV50TCL_VP_START_ID, 1);
	OUT_RING  (chan, 0);

	BEGIN_RING(chan, tesla, NV50TCL_CB_DEF_ADDRESS_HIGH, 3);
	if (OUT_RELOCh(chan, pNv->tesla_scratch,
		       PFP_OFFSET + PFP_S, NOUVEAU_BO_VRAM | NOUVEAU_BO_WR) ||
	    OUT_RELOCl(chan, pNv->tesla_scratch,
		       PFP_OFFSET + PFP_S, NOUVEAU_BO_VRAM | NOUVEAU_BO_WR)) {
		MARK_UNDO(chan);
		return FALSE;
	}
	OUT_RING  (chan, (0 << NV50TCL_CB_DEF_SET_BUFFER_SHIFT) | 0x4000);
	BEGIN_RING(chan, tesla, NV50TCL_CB_ADDR, 1);
	OUT_RING  (chan, 0);
	BEGIN_RING_NI(chan, tesla, NV50TCL_CB_DATA(0), 6);
	OUT_RING  (chan, 0x80000000);
	OUT_RING  (chan, 0x90000004);
	OUT_RING  (chan, 0x82010200);
	OUT_RING  (chan, 0x82020204);
	OUT_RING  (chan, 0xf6400001);
	OUT_RING  (chan, 0x0000c785);
	BEGIN_RING(chan, tesla, NV50TCL_CB_DEF_ADDRESS_HIGH, 3);
	if (OUT_RELOCh(chan, pNv->tesla_scratch,
		       PFP_OFFSET + PFP_C, NOUVEAU_BO_VRAM | NOUVEAU_BO_WR) ||
	    OUT_RELOCl(chan, pNv->tesla_scratch,
		       PFP_OFFSET + PFP_C, NOUVEAU_BO_VRAM | NOUVEAU_BO_WR)) {
		MARK_UNDO(chan);
		return FALSE;
	}
	OUT_RING  (chan, (0 << NV50TCL_CB_DEF_SET_BUFFER_SHIFT) | 0x4000);
	BEGIN_RING(chan, tesla, NV50TCL_CB_ADDR, 1);
	OUT_RING  (chan, 0);
	BEGIN_RING_NI(chan, tesla, NV50TCL_CB_DATA(0), 16);
	OUT_RING  (chan, 0x80000000);
	OUT_RING  (chan, 0x90000004);
	OUT_RING  (chan, 0x82030210);
	OUT_RING  (chan, 0x82040214);
	OUT_RING  (chan, 0x82010200);
	OUT_RING  (chan, 0x82020204);
	OUT_RING  (chan, 0xf6400001);
	OUT_RING  (chan, 0x0000c784);
	OUT_RING  (chan, 0xf0400211);
	OUT_RING  (chan, 0x00008784);
	OUT_RING  (chan, 0xc0040000);
	OUT_RING  (chan, 0xc0040204);
	OUT_RING  (chan, 0xc0040409);
	OUT_RING  (chan, 0x00000780);
	OUT_RING  (chan, 0xc004060d);
	OUT_RING  (chan, 0x00000781);
	BEGIN_RING(chan, tesla, NV50TCL_CB_DEF_ADDRESS_HIGH, 3);
	if (OUT_RELOCh(chan, pNv->tesla_scratch,
		       PFP_OFFSET + PFP_CCA, NOUVEAU_BO_VRAM | NOUVEAU_BO_WR) ||
	    OUT_RELOCl(chan, pNv->tesla_scratch,
		       PFP_OFFSET + PFP_CCA, NOUVEAU_BO_VRAM | NOUVEAU_BO_WR)) {
		MARK_UNDO(chan);
		return FALSE;
	}
	OUT_RING  (chan, (0 << NV50TCL_CB_DEF_SET_BUFFER_SHIFT) | 0x4000);
	BEGIN_RING(chan, tesla, NV50TCL_CB_ADDR, 1);
	OUT_RING  (chan, 0);
	BEGIN_RING_NI(chan, tesla, NV50TCL_CB_DATA(0), 16);
	OUT_RING  (chan, 0x80000000);
	OUT_RING  (chan, 0x90000004);
	OUT_RING  (chan, 0x82030210);
	OUT_RING  (chan, 0x82040214);
	OUT_RING  (chan, 0x82010200);
	OUT_RING  (chan, 0x82020204);
	OUT_RING  (chan, 0xf6400001);
	OUT_RING  (chan, 0x0000c784);
	OUT_RING  (chan, 0xf6400211);
	OUT_RING  (chan, 0x0000c784);
	OUT_RING  (chan, 0xc0040000);
	OUT_RING  (chan, 0xc0050204);
	OUT_RING  (chan, 0xc0060409);
	OUT_RING  (chan, 0x00000780);
	OUT_RING  (chan, 0xc007060d);
	OUT_RING  (chan, 0x00000781);
	BEGIN_RING(chan, tesla, NV50TCL_CB_DEF_ADDRESS_HIGH, 3);
	if (OUT_RELOCh(chan, pNv->tesla_scratch, PFP_OFFSET + PFP_CCASA,
		       NOUVEAU_BO_VRAM | NOUVEAU_BO_WR) ||
	    OUT_RELOCl(chan, pNv->tesla_scratch, PFP_OFFSET + PFP_CCASA,
		       NOUVEAU_BO_VRAM | NOUVEAU_BO_WR)) {
		MARK_UNDO(chan);
		return FALSE;
	}
	OUT_RING  (chan, (0 << NV50TCL_CB_DEF_SET_BUFFER_SHIFT) | 0x4000);
	BEGIN_RING(chan, tesla, NV50TCL_CB_ADDR, 1);
	OUT_RING  (chan, 0);
	BEGIN_RING_NI(chan, tesla, NV50TCL_CB_DATA(0), 16);
	OUT_RING  (chan, 0x80000000);
	OUT_RING  (chan, 0x90000004);
	OUT_RING  (chan, 0x82030200);
	OUT_RING  (chan, 0x82040204);
	OUT_RING  (chan, 0x82010210);
	OUT_RING  (chan, 0x82020214);
	OUT_RING  (chan, 0xf6400201);
	OUT_RING  (chan, 0x0000c784);
	OUT_RING  (chan, 0xf0400011);
	OUT_RING  (chan, 0x00008784);
	OUT_RING  (chan, 0xc0040000);
	OUT_RING  (chan, 0xc0040204);
	OUT_RING  (chan, 0xc0040409);
	OUT_RING  (chan, 0x00000780);
	OUT_RING  (chan, 0xc004060d);
	OUT_RING  (chan, 0x00000781);
	BEGIN_RING(chan, tesla, NV50TCL_CB_DEF_ADDRESS_HIGH, 3);
	if (OUT_RELOCh(chan, pNv->tesla_scratch, PFP_OFFSET + PFP_S_A8,
		       NOUVEAU_BO_VRAM | NOUVEAU_BO_WR) ||
	    OUT_RELOCl(chan, pNv->tesla_scratch, PFP_OFFSET + PFP_S_A8,
		       NOUVEAU_BO_VRAM | NOUVEAU_BO_WR)) {
		MARK_UNDO(chan);
		return FALSE;
	}
	OUT_RING  (chan, (0 << NV50TCL_CB_DEF_SET_BUFFER_SHIFT) | 0x4000);
	BEGIN_RING(chan, tesla, NV50TCL_CB_ADDR, 1);
	OUT_RING  (chan, 0);
	BEGIN_RING_NI(chan, tesla, NV50TCL_CB_DATA(0), 10);
	OUT_RING  (chan, 0x80000000);
	OUT_RING  (chan, 0x90000004);
	OUT_RING  (chan, 0x82010200);
	OUT_RING  (chan, 0x82020204);
	OUT_RING  (chan, 0xf0400001);
	OUT_RING  (chan, 0x00008784);
	OUT_RING  (chan, 0x10008004);
	OUT_RING  (chan, 0x10008008);
	OUT_RING  (chan, 0x1000000d);
	OUT_RING  (chan, 0x0403c781);
	BEGIN_RING(chan, tesla, NV50TCL_CB_DEF_ADDRESS_HIGH, 3);
	if (OUT_RELOCh(chan, pNv->tesla_scratch, PFP_OFFSET + PFP_C_A8,
		       NOUVEAU_BO_VRAM | NOUVEAU_BO_WR) ||
	    OUT_RELOCl(chan, pNv->tesla_scratch, PFP_OFFSET + PFP_C_A8,
		       NOUVEAU_BO_VRAM | NOUVEAU_BO_WR)) {
		MARK_UNDO(chan);
		return FALSE;
	}
	OUT_RING  (chan, (0 << NV50TCL_CB_DEF_SET_BUFFER_SHIFT) | 0x4000);
	BEGIN_RING(chan, tesla, NV50TCL_CB_ADDR, 1);
	OUT_RING  (chan, 0);
	BEGIN_RING_NI(chan, tesla, NV50TCL_CB_DATA(0), 16);
	OUT_RING  (chan, 0x80000000);
	OUT_RING  (chan, 0x90000004);
	OUT_RING  (chan, 0x82030208);
	OUT_RING  (chan, 0x8204020c);
	OUT_RING  (chan, 0x82010200);
	OUT_RING  (chan, 0x82020204);
	OUT_RING  (chan, 0xf0400001);
	OUT_RING  (chan, 0x00008784);
	OUT_RING  (chan, 0xf0400209);
	OUT_RING  (chan, 0x00008784);
	OUT_RING  (chan, 0xc002000d);
	OUT_RING  (chan, 0x00000780);
	OUT_RING  (chan, 0x10008600);
	OUT_RING  (chan, 0x10008604);
	OUT_RING  (chan, 0x10000609);
	OUT_RING  (chan, 0x0403c781);
	BEGIN_RING(chan, tesla, NV50TCL_CB_DEF_ADDRESS_HIGH, 3);
	if (OUT_RELOCh(chan, pNv->tesla_scratch, PFP_OFFSET + PFP_NV12,
		       NOUVEAU_BO_VRAM | NOUVEAU_BO_WR) ||
	    OUT_RELOCl(chan, pNv->tesla_scratch, PFP_OFFSET + PFP_NV12,
		       NOUVEAU_BO_VRAM | NOUVEAU_BO_WR)) {
		MARK_UNDO(chan);
		return FALSE;
	}
	OUT_RING  (chan, (0 << NV50TCL_CB_DEF_SET_BUFFER_SHIFT) | 0x4000);
	BEGIN_RING(chan, tesla, NV50TCL_CB_ADDR, 1);
	OUT_RING  (chan, 0);
	BEGIN_RING_NI(chan, tesla, NV50TCL_CB_DATA(0), 24);
	OUT_RING  (chan, 0x80000008);
	OUT_RING  (chan, 0x90000408);
	OUT_RING  (chan, 0x82010400);
	OUT_RING  (chan, 0x82020404);
	OUT_RING  (chan, 0xf0400001);
	OUT_RING  (chan, 0x00008784);
	OUT_RING  (chan, 0xc0800014);
	OUT_RING  (chan, 0xb0810a0c);
	OUT_RING  (chan, 0xb0820a10);
	OUT_RING  (chan, 0xb0830a14);
	OUT_RING  (chan, 0x82030400);
	OUT_RING  (chan, 0x82040404);
	OUT_RING  (chan, 0xf0400201);
	OUT_RING  (chan, 0x0000c784);
	OUT_RING  (chan, 0xe084000c);
	OUT_RING  (chan, 0xe0850010);
	OUT_RING  (chan, 0xe0860015);
	OUT_RING  (chan, 0x00014780);
	OUT_RING  (chan, 0xe0870201);
	OUT_RING  (chan, 0x0000c780);
	OUT_RING  (chan, 0xe0890209);
	OUT_RING  (chan, 0x00014780);
	OUT_RING  (chan, 0xe0880205);
	OUT_RING  (chan, 0x00010781);

	/* HPOS.xy = ($o0, $o1), HPOS.zw = (0.0, 1.0), then map $o2 - $o5 */
	BEGIN_RING(chan, tesla, NV50TCL_VP_RESULT_MAP(0), 2);
	OUT_RING  (chan, 0x41400100);
	OUT_RING  (chan, 0x05040302);
	BEGIN_RING(chan, tesla, NV50TCL_POINT_SPRITE_ENABLE, 1);
	OUT_RING  (chan, 0x00000000);
	BEGIN_RING(chan, tesla, NV50TCL_FP_INTERPOLANT_CTRL, 2);
	OUT_RING  (chan, 0x08040404);
	OUT_RING  (chan, 0x00000008); /* NV50TCL_FP_REG_ALLOC_TEMP */

	BEGIN_RING(chan, tesla, NV50TCL_SCISSOR_ENABLE(0), 1);
	OUT_RING  (chan, 1);

	BEGIN_RING(chan, tesla, NV50TCL_VIEWPORT_HORIZ(0), 2);
	OUT_RING  (chan, 8192 << NV50TCL_VIEWPORT_HORIZ_W_SHIFT);
	OUT_RING  (chan, 8192 << NV50TCL_VIEWPORT_VERT_H_SHIFT);
	/* NV50TCL_SCISSOR_VERT_T_SHIFT is wrong, because it was deducted with
	 * origin lying at the bottom left. This will be changed to _MIN_ and _MAX_
	 * later, because it is origin dependent.
	 */
	BEGIN_RING(chan, tesla, NV50TCL_SCISSOR_HORIZ(0), 2);
	OUT_RING  (chan, 8192 << NV50TCL_SCISSOR_HORIZ_MAX_SHIFT);
	OUT_RING  (chan, 8192 << NV50TCL_SCISSOR_VERT_MAX_SHIFT);
	BEGIN_RING(chan, tesla, NV50TCL_SCREEN_SCISSOR_HORIZ, 2);
	OUT_RING  (chan, 8192 << NV50TCL_SCREEN_SCISSOR_HORIZ_W_SHIFT);
	OUT_RING  (chan, 8192 << NV50TCL_SCREEN_SCISSOR_VERT_H_SHIFT);

	BEGIN_RING(chan, tesla, NV50TCL_SET_PROGRAM_CB, 1);
	OUT_RING  (chan, 0x00000031 | (CB_PFP << 12));

	return TRUE;
}

