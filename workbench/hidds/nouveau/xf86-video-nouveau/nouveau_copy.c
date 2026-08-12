/*
 * Copyright 2014 Red Hat Inc.
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
 * THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * Authors: Ben Skeggs <bskeggs@redhat.com>
 */

#include "nouveau_copy.h"

void
nouveau_copy_fini(ScreenPtr pScreen)
{
	ScrnInfoPtr pScrn = xf86ScreenToScrn(pScreen);
	NVPtr pNv = NVPTR(pScrn);
	nouveau_object_del(&pNv->NvCopy);
	nouveau_pushbuf_del(&pNv->ce_pushbuf);
	nouveau_object_del(&pNv->ce_channel);
}

Bool
nouveau_copy_init(ScreenPtr pScreen)
{
	static const struct {
		CARD32 oclass;
		int engine;
		Bool (*init)(NVPtr);
	} methods[] = {
		{ 0xc1b5, 0, nouveau_copya0b5_init },
		{ 0xc0b5, 0, nouveau_copya0b5_init },
		{ 0xb0b5, 0, nouveau_copya0b5_init },
		{ 0xa0b5, 0, nouveau_copya0b5_init },
		{ 0x90b8, 5, nouveau_copy90b5_init },
		{ 0x90b5, 4, nouveau_copy90b5_init },
		{ 0x85b5, 0, nouveau_copy85b5_init },
		{}
	}, *method = methods;
	ScrnInfoPtr pScrn = xf86ScreenToScrn(pScreen);
	NVPtr pNv = NVPTR(pScrn);
	int ret;

#if !defined(__AROS__)
	if (pNv->AccelMethod == NONE) {
		xf86DrvMsg(pScrn->scrnIndex, X_CONFIG,
			   "[COPY] acceleration disabled\n");
		return FALSE;
	}
#endif

	switch (pNv->Architecture) {
	case NV_TESLA:
		if (pNv->dev->chipset < 0xa3 ||
		    pNv->dev->chipset == 0xaa ||
		    pNv->dev->chipset == 0xac)
			return FALSE;

		ret = nouveau_object_new(&pNv->dev->object, 0,
					 NOUVEAU_FIFO_CHANNEL_CLASS,
					 &(struct nv04_fifo) {
						.vram = NvDmaFB,
						.gart = NvDmaTT,
					 }, sizeof(struct nv04_fifo),
					 &pNv->ce_channel);
		break;
	case NV_FERMI:
		ret = nouveau_object_new(&pNv->dev->object, 0,
					 NOUVEAU_FIFO_CHANNEL_CLASS,
					 &(struct nvc0_fifo) {
					 }, sizeof(struct nvc0_fifo),
					 &pNv->ce_channel);
		break;
	case NV_KEPLER:
	case NV_MAXWELL:
	case NV_PASCAL:
		ret = nouveau_object_new(&pNv->dev->object, 0,
					 NOUVEAU_FIFO_CHANNEL_CLASS,
					 &(struct nve0_fifo) {
						.engine = NVE0_FIFO_ENGINE_CE0 |
							  NVE0_FIFO_ENGINE_CE1,
					 }, sizeof(struct nve0_fifo),
					 &pNv->ce_channel);
		break;
	default:
		return FALSE;
	}

	if (ret) {
		xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
			   "[COPY] error allocating channel: %d\n", ret);
		return FALSE;
	}

	ret = nouveau_pushbuf_new(pNv->client, pNv->ce_channel, 4,
				  32 * 1024, true, &pNv->ce_pushbuf);
	if (ret) {
		xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
			   "[COPY] error allocating pushbuf: %d\n", ret);
		nouveau_copy_fini(pScreen);
		return FALSE;
	}

	while (method->init) {
		ret = nouveau_object_new(pNv->ce_channel,
					 method->engine << 16 | method->oclass,
					 method->oclass, NULL, 0,
					 &pNv->NvCopy);
		if (ret == 0) {
			if (!method->init(pNv)) {
				xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
					   "[COPY] failed to initialise.\n");
				nouveau_copy_fini(pScreen);
				return FALSE;
			}
			break;
		}
		method++;
	}

	if (ret) {
		xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
			   "[COPY] failed to allocate class.\n");
		nouveau_copy_fini(pScreen);
		return FALSE;
	}

	xf86DrvMsg(pScrn->scrnIndex, X_INFO, "[COPY] async initialised.\n");
	return TRUE;
}
