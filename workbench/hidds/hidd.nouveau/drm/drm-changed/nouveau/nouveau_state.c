/*
 * Copyright 2005 Stephane Marchesin
 * Copyright 2008 Stuart Bennett
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * PRECISION INSIGHT AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

//FIXME: #include <linux/swab.h>
#include "drmP.h"
#include "drm.h"
#include "drm_sarea.h"
//FIXME: #include "drm_crtc_helper.h"
//FIXME: #include <linux/vgaarb.h>

#include "nouveau_drv.h"
#include "nouveau_drm.h"
#include "nv50_display.h"

static int nouveau_stub_init(struct drm_device *dev) { return 0; }
static void nouveau_stub_takedown(struct drm_device *dev) {}

static int nouveau_init_engine_ptrs(struct drm_device *dev)
{
	struct drm_nouveau_private *dev_priv = dev->dev_private;
	struct nouveau_engine *engine = &dev_priv->engine;

	switch (dev_priv->chipset & 0xf0) {
	case 0x00:
		engine->instmem.init		= nv04_instmem_init;
		engine->instmem.takedown	= nv04_instmem_takedown;
		engine->instmem.suspend		= nv04_instmem_suspend;
		engine->instmem.resume		= nv04_instmem_resume;
		engine->instmem.populate	= nv04_instmem_populate;
		engine->instmem.clear		= nv04_instmem_clear;
		engine->instmem.bind		= nv04_instmem_bind;
		engine->instmem.unbind		= nv04_instmem_unbind;
		engine->instmem.prepare_access	= nv04_instmem_prepare_access;
		engine->instmem.finish_access	= nv04_instmem_finish_access;
		engine->mc.init			= nv04_mc_init;
		engine->mc.takedown		= nv04_mc_takedown;
		engine->timer.init		= nv04_timer_init;
		engine->timer.read		= nv04_timer_read;
		engine->timer.takedown		= nv04_timer_takedown;
		engine->fb.init			= nv04_fb_init;
		engine->fb.takedown		= nv04_fb_takedown;
		engine->graph.grclass		= nv04_graph_grclass;
		engine->graph.init		= nv04_graph_init;
		engine->graph.takedown		= nv04_graph_takedown;
		engine->graph.fifo_access	= nv04_graph_fifo_access;
		engine->graph.channel		= nv04_graph_channel;
		engine->graph.create_context	= nv04_graph_create_context;
		engine->graph.destroy_context	= nv04_graph_destroy_context;
		engine->graph.load_context	= nv04_graph_load_context;
		engine->graph.unload_context	= nv04_graph_unload_context;
		engine->fifo.channels		= 16;
		engine->fifo.init		= nv04_fifo_init;
		engine->fifo.takedown		= nouveau_stub_takedown;
		engine->fifo.disable		= nv04_fifo_disable;
		engine->fifo.enable		= nv04_fifo_enable;
		engine->fifo.reassign		= nv04_fifo_reassign;
		engine->fifo.cache_flush	= nv04_fifo_cache_flush;
		engine->fifo.cache_pull		= nv04_fifo_cache_pull;
		engine->fifo.channel_id		= nv04_fifo_channel_id;
		engine->fifo.create_context	= nv04_fifo_create_context;
		engine->fifo.destroy_context	= nv04_fifo_destroy_context;
		engine->fifo.load_context	= nv04_fifo_load_context;
		engine->fifo.unload_context	= nv04_fifo_unload_context;
		break;
	case 0x10:
		engine->instmem.init		= nv04_instmem_init;
		engine->instmem.takedown	= nv04_instmem_takedown;
		engine->instmem.suspend		= nv04_instmem_suspend;
		engine->instmem.resume		= nv04_instmem_resume;
		engine->instmem.populate	= nv04_instmem_populate;
		engine->instmem.clear		= nv04_instmem_clear;
		engine->instmem.bind		= nv04_instmem_bind;
		engine->instmem.unbind		= nv04_instmem_unbind;
		engine->instmem.prepare_access	= nv04_instmem_prepare_access;
		engine->instmem.finish_access	= nv04_instmem_finish_access;
		engine->mc.init			= nv04_mc_init;
		engine->mc.takedown		= nv04_mc_takedown;
		engine->timer.init		= nv04_timer_init;
		engine->timer.read		= nv04_timer_read;
		engine->timer.takedown		= nv04_timer_takedown;
		engine->fb.init			= nv10_fb_init;
		engine->fb.takedown		= nv10_fb_takedown;
		engine->fb.set_region_tiling	= nv10_fb_set_region_tiling;
		engine->graph.grclass		= nv10_graph_grclass;
		engine->graph.init		= nv10_graph_init;
		engine->graph.takedown		= nv10_graph_takedown;
		engine->graph.channel		= nv10_graph_channel;
		engine->graph.create_context	= nv10_graph_create_context;
		engine->graph.destroy_context	= nv10_graph_destroy_context;
		engine->graph.fifo_access	= nv04_graph_fifo_access;
		engine->graph.load_context	= nv10_graph_load_context;
		engine->graph.unload_context	= nv10_graph_unload_context;
		engine->graph.set_region_tiling	= nv10_graph_set_region_tiling;
		engine->fifo.channels		= 32;
		engine->fifo.init		= nv10_fifo_init;
		engine->fifo.takedown		= nouveau_stub_takedown;
		engine->fifo.disable		= nv04_fifo_disable;
		engine->fifo.enable		= nv04_fifo_enable;
		engine->fifo.reassign		= nv04_fifo_reassign;
		engine->fifo.cache_flush	= nv04_fifo_cache_flush;
		engine->fifo.cache_pull		= nv04_fifo_cache_pull;
		engine->fifo.channel_id		= nv10_fifo_channel_id;
		engine->fifo.create_context	= nv10_fifo_create_context;
		engine->fifo.destroy_context	= nv10_fifo_destroy_context;
		engine->fifo.load_context	= nv10_fifo_load_context;
		engine->fifo.unload_context	= nv10_fifo_unload_context;
		break;
	case 0x20:
		engine->instmem.init		= nv04_instmem_init;
		engine->instmem.takedown	= nv04_instmem_takedown;
		engine->instmem.suspend		= nv04_instmem_suspend;
		engine->instmem.resume		= nv04_instmem_resume;
		engine->instmem.populate	= nv04_instmem_populate;
		engine->instmem.clear		= nv04_instmem_clear;
		engine->instmem.bind		= nv04_instmem_bind;
		engine->instmem.unbind		= nv04_instmem_unbind;
		engine->instmem.prepare_access	= nv04_instmem_prepare_access;
		engine->instmem.finish_access	= nv04_instmem_finish_access;
		engine->mc.init			= nv04_mc_init;
		engine->mc.takedown		= nv04_mc_takedown;
		engine->timer.init		= nv04_timer_init;
		engine->timer.read		= nv04_timer_read;
		engine->timer.takedown		= nv04_timer_takedown;
		engine->fb.init			= nv10_fb_init;
		engine->fb.takedown		= nv10_fb_takedown;
		engine->fb.set_region_tiling	= nv10_fb_set_region_tiling;
		engine->graph.grclass		= nv20_graph_grclass;
		engine->graph.init		= nv20_graph_init;
		engine->graph.takedown		= nv20_graph_takedown;
		engine->graph.channel		= nv10_graph_channel;
		engine->graph.create_context	= nv20_graph_create_context;
		engine->graph.destroy_context	= nv20_graph_destroy_context;
		engine->graph.fifo_access	= nv04_graph_fifo_access;
		engine->graph.load_context	= nv20_graph_load_context;
		engine->graph.unload_context	= nv20_graph_unload_context;
		engine->graph.set_region_tiling	= nv20_graph_set_region_tiling;
		engine->fifo.channels		= 32;
		engine->fifo.init		= nv10_fifo_init;
		engine->fifo.takedown		= nouveau_stub_takedown;
		engine->fifo.disable		= nv04_fifo_disable;
		engine->fifo.enable		= nv04_fifo_enable;
		engine->fifo.reassign		= nv04_fifo_reassign;
		engine->fifo.cache_flush	= nv04_fifo_cache_flush;
		engine->fifo.cache_pull		= nv04_fifo_cache_pull;
		engine->fifo.channel_id		= nv10_fifo_channel_id;
		engine->fifo.create_context	= nv10_fifo_create_context;
		engine->fifo.destroy_context	= nv10_fifo_destroy_context;
		engine->fifo.load_context	= nv10_fifo_load_context;
		engine->fifo.unload_context	= nv10_fifo_unload_context;
		break;
	case 0x30:
		engine->instmem.init		= nv04_instmem_init;
		engine->instmem.takedown	= nv04_instmem_takedown;
		engine->instmem.suspend		= nv04_instmem_suspend;
		engine->instmem.resume		= nv04_instmem_resume;
		engine->instmem.populate	= nv04_instmem_populate;
		engine->instmem.clear		= nv04_instmem_clear;
		engine->instmem.bind		= nv04_instmem_bind;
		engine->instmem.unbind		= nv04_instmem_unbind;
		engine->instmem.prepare_access	= nv04_instmem_prepare_access;
		engine->instmem.finish_access	= nv04_instmem_finish_access;
		engine->mc.init			= nv04_mc_init;
		engine->mc.takedown		= nv04_mc_takedown;
		engine->timer.init		= nv04_timer_init;
		engine->timer.read		= nv04_timer_read;
		engine->timer.takedown		= nv04_timer_takedown;
		engine->fb.init			= nv10_fb_init;
		engine->fb.takedown		= nv10_fb_takedown;
		engine->fb.set_region_tiling	= nv10_fb_set_region_tiling;
		engine->graph.grclass		= nv30_graph_grclass;
		engine->graph.init		= nv30_graph_init;
		engine->graph.takedown		= nv20_graph_takedown;
		engine->graph.fifo_access	= nv04_graph_fifo_access;
		engine->graph.channel		= nv10_graph_channel;
		engine->graph.create_context	= nv20_graph_create_context;
		engine->graph.destroy_context	= nv20_graph_destroy_context;
		engine->graph.load_context	= nv20_graph_load_context;
		engine->graph.unload_context	= nv20_graph_unload_context;
		engine->graph.set_region_tiling	= nv20_graph_set_region_tiling;
		engine->fifo.channels		= 32;
		engine->fifo.init		= nv10_fifo_init;
		engine->fifo.takedown		= nouveau_stub_takedown;
		engine->fifo.disable		= nv04_fifo_disable;
		engine->fifo.enable		= nv04_fifo_enable;
		engine->fifo.reassign		= nv04_fifo_reassign;
		engine->fifo.cache_flush	= nv04_fifo_cache_flush;
		engine->fifo.cache_pull		= nv04_fifo_cache_pull;
		engine->fifo.channel_id		= nv10_fifo_channel_id;
		engine->fifo.create_context	= nv10_fifo_create_context;
		engine->fifo.destroy_context	= nv10_fifo_destroy_context;
		engine->fifo.load_context	= nv10_fifo_load_context;
		engine->fifo.unload_context	= nv10_fifo_unload_context;
		break;
	case 0x40:
	case 0x60:
		engine->instmem.init		= nv04_instmem_init;
		engine->instmem.takedown	= nv04_instmem_takedown;
		engine->instmem.suspend		= nv04_instmem_suspend;
		engine->instmem.resume		= nv04_instmem_resume;
		engine->instmem.populate	= nv04_instmem_populate;
		engine->instmem.clear		= nv04_instmem_clear;
		engine->instmem.bind		= nv04_instmem_bind;
		engine->instmem.unbind		= nv04_instmem_unbind;
		engine->instmem.prepare_access	= nv04_instmem_prepare_access;
		engine->instmem.finish_access	= nv04_instmem_finish_access;
		engine->mc.init			= nv40_mc_init;
		engine->mc.takedown		= nv40_mc_takedown;
		engine->timer.init		= nv04_timer_init;
		engine->timer.read		= nv04_timer_read;
		engine->timer.takedown		= nv04_timer_takedown;
		engine->fb.init			= nv40_fb_init;
		engine->fb.takedown		= nv40_fb_takedown;
		engine->fb.set_region_tiling	= nv40_fb_set_region_tiling;
		engine->graph.grclass		= nv40_graph_grclass;
		engine->graph.init		= nv40_graph_init;
		engine->graph.takedown		= nv40_graph_takedown;
		engine->graph.fifo_access	= nv04_graph_fifo_access;
		engine->graph.channel		= nv40_graph_channel;
		engine->graph.create_context	= nv40_graph_create_context;
		engine->graph.destroy_context	= nv40_graph_destroy_context;
		engine->graph.load_context	= nv40_graph_load_context;
		engine->graph.unload_context	= nv40_graph_unload_context;
		engine->graph.set_region_tiling	= nv40_graph_set_region_tiling;
		engine->fifo.channels		= 32;
		engine->fifo.init		= nv40_fifo_init;
		engine->fifo.takedown		= nouveau_stub_takedown;
		engine->fifo.disable		= nv04_fifo_disable;
		engine->fifo.enable		= nv04_fifo_enable;
		engine->fifo.reassign		= nv04_fifo_reassign;
		engine->fifo.cache_flush	= nv04_fifo_cache_flush;
		engine->fifo.cache_pull		= nv04_fifo_cache_pull;
		engine->fifo.channel_id		= nv10_fifo_channel_id;
		engine->fifo.create_context	= nv40_fifo_create_context;
		engine->fifo.destroy_context	= nv40_fifo_destroy_context;
		engine->fifo.load_context	= nv40_fifo_load_context;
		engine->fifo.unload_context	= nv40_fifo_unload_context;
		break;
	case 0x50:
	case 0x80: /* gotta love NVIDIA's consistency.. */
	case 0x90:
	case 0xA0:
		engine->instmem.init		= nv50_instmem_init;
		engine->instmem.takedown	= nv50_instmem_takedown;
		engine->instmem.suspend		= nv50_instmem_suspend;
		engine->instmem.resume		= nv50_instmem_resume;
		engine->instmem.populate	= nv50_instmem_populate;
		engine->instmem.clear		= nv50_instmem_clear;
		engine->instmem.bind		= nv50_instmem_bind;
		engine->instmem.unbind		= nv50_instmem_unbind;
		engine->instmem.prepare_access	= nv50_instmem_prepare_access;
		engine->instmem.finish_access	= nv50_instmem_finish_access;
		engine->mc.init			= nv50_mc_init;
		engine->mc.takedown		= nv50_mc_takedown;
		engine->timer.init		= nv04_timer_init;
		engine->timer.read		= nv04_timer_read;
		engine->timer.takedown		= nv04_timer_takedown;
		engine->fb.init			= nouveau_stub_init;
		engine->fb.takedown		= nouveau_stub_takedown;
		engine->graph.grclass		= nv50_graph_grclass;
		engine->graph.init		= nv50_graph_init;
		engine->graph.takedown		= nv50_graph_takedown;
		engine->graph.fifo_access	= nv50_graph_fifo_access;
		engine->graph.channel		= nv50_graph_channel;
		engine->graph.create_context	= nv50_graph_create_context;
		engine->graph.destroy_context	= nv50_graph_destroy_context;
		engine->graph.load_context	= nv50_graph_load_context;
		engine->graph.unload_context	= nv50_graph_unload_context;
		engine->fifo.channels		= 128;
		engine->fifo.init		= nv50_fifo_init;
		engine->fifo.takedown		= nv50_fifo_takedown;
		engine->fifo.disable		= nv04_fifo_disable;
		engine->fifo.enable		= nv04_fifo_enable;
		engine->fifo.reassign		= nv04_fifo_reassign;
		engine->fifo.channel_id		= nv50_fifo_channel_id;
		engine->fifo.create_context	= nv50_fifo_create_context;
		engine->fifo.destroy_context	= nv50_fifo_destroy_context;
		engine->fifo.load_context	= nv50_fifo_load_context;
		engine->fifo.unload_context	= nv50_fifo_unload_context;
		break;
	default:
		NV_ERROR(dev, "NV%02x unsupported\n", dev_priv->chipset);
		return 1;
	}

	return 0;
}

#if !defined(__AROS__)
static unsigned int
nouveau_vga_set_decode(void *priv, bool state)
{
	struct drm_device *dev = priv;
	struct drm_nouveau_private *dev_priv = dev->dev_private;

	if (dev_priv->chipset >= 0x40)
		nv_wr32(dev, 0x88054, state);
	else
		nv_wr32(dev, 0x1854, state);

	if (state)
		return VGA_RSRC_LEGACY_IO | VGA_RSRC_LEGACY_MEM |
		       VGA_RSRC_NORMAL_IO | VGA_RSRC_NORMAL_MEM;
	else
		return VGA_RSRC_NORMAL_IO | VGA_RSRC_NORMAL_MEM;
}
#endif

static int
nouveau_card_init_channel(struct drm_device *dev)
{
	struct drm_nouveau_private *dev_priv = dev->dev_private;
	struct nouveau_gpuobj *gpuobj;
	int ret;

	ret = nouveau_channel_alloc(dev, &dev_priv->channel,
				    (struct drm_file *)-2,
				    NvDmaFB, NvDmaTT);
	if (ret)
		return ret;

	gpuobj = NULL;
	ret = nouveau_gpuobj_dma_new(dev_priv->channel, NV_CLASS_DMA_IN_MEMORY,
				     0, nouveau_mem_fb_amount(dev),
				     NV_DMA_ACCESS_RW, NV_DMA_TARGET_VIDMEM,
				     &gpuobj);
	if (ret)
		goto out_err;

	ret = nouveau_gpuobj_ref_add(dev, dev_priv->channel, NvDmaVRAM,
				     gpuobj, NULL);
	if (ret)
		goto out_err;

	gpuobj = NULL;
	ret = nouveau_gpuobj_gart_dma_new(dev_priv->channel, 0,
					  dev_priv->gart_info.aper_size,
					  NV_DMA_ACCESS_RW, &gpuobj, NULL);
	if (ret)
		goto out_err;

	ret = nouveau_gpuobj_ref_add(dev, dev_priv->channel, NvDmaGART,
				     gpuobj, NULL);
	if (ret)
		goto out_err;

	return 0;
out_err:
	nouveau_gpuobj_del(dev, &gpuobj);
	nouveau_channel_free(dev_priv->channel);
	dev_priv->channel = NULL;
	return ret;
}

struct nouveau_bo * screen_bo = NULL;

int
nouveau_card_init(struct drm_device *dev)
{
	struct drm_nouveau_private *dev_priv = dev->dev_private;
	struct nouveau_engine *engine;
	int ret;

	NV_DEBUG(dev, "prev state = %d\n", dev_priv->init_state);

	if (dev_priv->init_state == NOUVEAU_CARD_INIT_DONE)
		return 0;

#if !defined(__AROS__)
	vga_client_register(dev->pdev, dev, NULL, nouveau_vga_set_decode);
#else
DRM_IMPL("Calling vga_client_register\n");
#endif

	/* Initialise internal driver API hooks */
	ret = nouveau_init_engine_ptrs(dev);
	if (ret)
		goto out;
	engine = &dev_priv->engine;
	dev_priv->init_state = NOUVEAU_CARD_INIT_FAILED;
	spin_lock_init(&dev_priv->context_switch_lock);

#if !defined(__AROS__)
	/* Parse BIOS tables / Run init tables if card not POSTed */
	if (drm_core_check_feature(dev, DRIVER_MODESET)) {
		ret = nouveau_bios_init(dev);
		if (ret)
			goto out;
	}
#else
DRM_IMPL("Calling nouveau_bios_init\n");
#endif

	ret = nouveau_gpuobj_early_init(dev);
	if (ret)
		goto out_bios;

	/* Initialise instance memory, must happen before mem_init so we
	 * know exactly how much VRAM we're able to use for "normal"
	 * purposes.
	 */
	ret = engine->instmem.init(dev);
	if (ret)
		goto out_gpuobj_early;

	/* Setup the memory manager */
	ret = nouveau_mem_init(dev);
	if (ret)
		goto out_instmem;

	ret = nouveau_gpuobj_init(dev);
	if (ret)
		goto out_mem;

	/* PMC */
	ret = engine->mc.init(dev);
	if (ret)
		goto out_gpuobj;

	/* PTIMER */
	ret = engine->timer.init(dev);
	if (ret)
		goto out_mc;

	/* PFB */
	ret = engine->fb.init(dev);
	if (ret)
		goto out_timer;

	if (nouveau_noaccel)
		engine->graph.accel_blocked = true;
	else {
		/* PGRAPH */
		ret = engine->graph.init(dev);
		if (ret)
			goto out_fb;

		/* PFIFO */
		ret = engine->fifo.init(dev);
		if (ret)
			goto out_graph;
	}

	/* this call irq_preinstall, register irq handler and
	 * call irq_postinstall
	 */
	ret = drm_irq_install(dev);
	if (ret)
		goto out_fifo;

#if !defined(__AROS__)
	ret = drm_vblank_init(dev, 0);
	if (ret)
		goto out_irq;
#else
DRM_IMPL("Calling drm_vblank_init\n");
#endif

#if defined(__AROS__)
    {
        /* Create buffer object to protect visible screen. */
        /* FIXME: Allocate FullHD buffer size for now, should read real resolution later */
        nouveau_gem_new(dev, NULL, 1920 * 1080 * 4, 0, TTM_PL_FLAG_VRAM, 0, 0, false, false, &screen_bo);

        /* Create global name == 1 */
        idr_pre_get(&dev->object_name_idr, GFP_KERNEL);
        spin_lock(&dev->object_name_lock);
        idr_get_new_above(&dev->object_name_idr, screen_bo->gem, 1, &screen_bo->gem->name);
        spin_unlock(&dev->object_name_lock);
    }
#endif

	/* what about PVIDEO/PCRTC/PRAMDAC etc? */

	if (!engine->graph.accel_blocked) {
		ret = nouveau_card_init_channel(dev);
		if (ret)
			goto out_irq;
	}

#if !defined(__AROS__)
	if (drm_core_check_feature(dev, DRIVER_MODESET)) {
		if (dev_priv->card_type >= NV_50)
			ret = nv50_display_create(dev);
		else
			ret = nv04_display_create(dev);
		if (ret)
			goto out_irq;
	}
#else
DRM_IMPL("Calling *_display_create\n");
#endif

	ret = nouveau_backlight_init(dev);
	if (ret)
		NV_ERROR(dev, "Error %d registering backlight\n", ret);

	dev_priv->init_state = NOUVEAU_CARD_INIT_DONE;

#if !defined(__AROS__)
	if (drm_core_check_feature(dev, DRIVER_MODESET))
		drm_helper_initial_config(dev);
#else
DRM_IMPL("Calling drm_helper_initial_config\n");
#endif

	return 0;

out_irq:
	drm_irq_uninstall(dev);
out_fifo:
	if (!nouveau_noaccel)
		engine->fifo.takedown(dev);
out_graph:
	if (!nouveau_noaccel)
		engine->graph.takedown(dev);
out_fb:
	engine->fb.takedown(dev);
out_timer:
	engine->timer.takedown(dev);
out_mc:
	engine->mc.takedown(dev);
out_gpuobj:
	nouveau_gpuobj_takedown(dev);
out_mem:
	nouveau_mem_close(dev);
out_instmem:
	engine->instmem.takedown(dev);
out_gpuobj_early:
	nouveau_gpuobj_late_takedown(dev);
out_bios:
#if !defined(__AROS__)
	nouveau_bios_takedown(dev);
#else
IMPLEMENT("Calling nouveau_bios_takedown\n");
#endif	
out:
#if !defined(__AROS__)
	vga_client_register(dev->pdev, NULL, NULL, NULL);
#else
DRM_IMPL("Calling vga_client_register\n");
#endif
	return ret;
}

static void nouveau_card_takedown(struct drm_device *dev)
{
	struct drm_nouveau_private *dev_priv = dev->dev_private;
	struct nouveau_engine *engine = &dev_priv->engine;

	NV_DEBUG(dev, "prev state = %d\n", dev_priv->init_state);

	if (dev_priv->init_state != NOUVEAU_CARD_INIT_DOWN) {
		nouveau_backlight_exit(dev);

		if (dev_priv->channel) {
			nouveau_channel_free(dev_priv->channel);
			dev_priv->channel = NULL;
		}
		
#if defined(__AROS__)
        /* Release screen buffer object */
        if (screen_bo != NULL)
            drm_gem_object_unreference(screen_bo->gem);
#endif        

		if (!nouveau_noaccel) {
			engine->fifo.takedown(dev);
			engine->graph.takedown(dev);
		}
		engine->fb.takedown(dev);
		engine->timer.takedown(dev);
		engine->mc.takedown(dev);

		mutex_lock(&dev->struct_mutex);
		ttm_bo_clean_mm(&dev_priv->ttm.bdev, TTM_PL_VRAM);
		ttm_bo_clean_mm(&dev_priv->ttm.bdev, TTM_PL_TT);
		mutex_unlock(&dev->struct_mutex);
		nouveau_sgdma_takedown(dev);

		nouveau_gpuobj_takedown(dev);
		nouveau_mem_close(dev);
		engine->instmem.takedown(dev);

		if (drm_core_check_feature(dev, DRIVER_MODESET))
			drm_irq_uninstall(dev);

		nouveau_gpuobj_late_takedown(dev);
#if !defined(__AROS__)        
		nouveau_bios_takedown(dev);
#else
IMPLEMENT("Calling nouveau_bios_takedown\n");
#endif

#if !defined(__AROS__)
		vga_client_register(dev->pdev, NULL, NULL, NULL);
#else
DRM_IMPL("Calling vga_client_register\n");
#endif

		dev_priv->init_state = NOUVEAU_CARD_INIT_DOWN;
	}
}

/* here a client dies, release the stuff that was allocated for its
 * file_priv */
void nouveau_preclose(struct drm_device *dev, struct drm_file *file_priv)
{
	nouveau_channel_cleanup(dev, file_priv);
}

/* first module load, setup the mmio/fb mapping */
/* KMS: we need mmio at load time, not when the first drm client opens. */
int nouveau_firstopen(struct drm_device *dev)
{
	return 0;
}

/* if we have an OF card, copy vbios to RAMIN */
static void nouveau_OF_copy_vbios_to_ramin(struct drm_device *dev)
{
#if defined(__powerpc__)
	int size, i;
	const uint32_t *bios;
	struct device_node *dn = pci_device_to_OF_node(dev->pdev);
	if (!dn) {
		NV_INFO(dev, "Unable to get the OF node\n");
		return;
	}

	bios = of_get_property(dn, "NVDA,BMP", &size);
	if (bios) {
		for (i = 0; i < size; i += 4)
			nv_wi32(dev, i, bios[i/4]);
		NV_INFO(dev, "OF bios successfully copied (%d bytes)\n", size);
	} else {
		NV_INFO(dev, "Unable to get the OF bios\n");
	}
#endif
}

int nouveau_load(struct drm_device *dev, unsigned long flags)
{
	struct drm_nouveau_private *dev_priv;
	uint32_t reg0;
	resource_size_t mmio_start_offs;

	dev_priv = kzalloc(sizeof(*dev_priv), GFP_KERNEL);
	if (!dev_priv)
		return -ENOMEM;
	dev->dev_private = dev_priv;
	dev_priv->dev = dev;

	dev_priv->flags = flags & NOUVEAU_FLAGS;
	dev_priv->init_state = NOUVEAU_CARD_INIT_DOWN;

#if !defined(__AROS__)
	NV_DEBUG(dev, "vendor: 0x%X device: 0x%X class: 0x%X\n",
		 dev->pci_vendor, dev->pci_device, dev->pdev->class);
#else
	NV_DEBUG(dev, "vendor: 0x%X device: 0x%X\n",
		 dev->pci_vendor, dev->pci_device);
#endif

#if !defined(__AROS__)
	dev_priv->acpi_dsm = nouveau_dsm_probe(dev);

	if (dev_priv->acpi_dsm)
		nouveau_hybrid_setup(dev);
#else
DRM_IMPL("Calling nouveau_dsm_probe and nouveau_hybrid_setup\n");
#endif

#if !defined(__AROS__)
	dev_priv->wq = create_workqueue("nouveau");
	if (!dev_priv->wq)
		return -EINVAL;
#else
DRM_IMPL("Creating workqueue\n");
#endif

	/* resource 0 is mmio regs */
	/* resource 1 is linear FB */
	/* resource 2 is RAMIN (mmio regs + 0x1000000) */
	/* resource 6 is bios */

	/* map the mmio regs */
	mmio_start_offs = pci_resource_start(dev->pdev, 0);
	dev_priv->mmio = ioremap(mmio_start_offs, 0x00800000);
	if (!dev_priv->mmio) {
		NV_ERROR(dev, "Unable to initialize the mmio mapping. "
			 "Please report your setup to " DRIVER_EMAIL "\n");
		return -EINVAL;
	}
	NV_DEBUG(dev, "regs mapped ok at 0x%llx\n",
					(unsigned long long)mmio_start_offs);

#ifdef __BIG_ENDIAN
	/* Put the card in BE mode if it's not */
	if (nv_rd32(dev, NV03_PMC_BOOT_1))
		nv_wr32(dev, NV03_PMC_BOOT_1, 0x00000001);

	DRM_MEMORYBARRIER();
#endif

	/* Time to determine the card architecture */
	reg0 = nv_rd32(dev, NV03_PMC_BOOT_0);

	/* We're dealing with >=NV10 */
	if ((reg0 & 0x0f000000) > 0) {
		/* Bit 27-20 contain the architecture in hex */
		dev_priv->chipset = (reg0 & 0xff00000) >> 20;
	/* NV04 or NV05 */
	} else if ((reg0 & 0xff00fff0) == 0x20004000) {
		if (reg0 & 0x00f00000)
			dev_priv->chipset = 0x05;
		else
			dev_priv->chipset = 0x04;
	} else
		dev_priv->chipset = 0xff;

#if defined(HOSTED_BUILD)
    dev_priv->chipset = HOSTED_BUILD_CHIPSET;
#endif
 
	switch (dev_priv->chipset & 0xf0) {
	case 0x00:
	case 0x10:
	case 0x20:
	case 0x30:
		dev_priv->card_type = dev_priv->chipset & 0xf0;
		break;
	case 0x40:
	case 0x60:
		dev_priv->card_type = NV_40;
		break;
	case 0x50:
	case 0x80:
	case 0x90:
	case 0xa0:
		dev_priv->card_type = NV_50;
		break;
	default:
		NV_INFO(dev, "Unsupported chipset 0x%08x\n", reg0);
		return -EINVAL;
	}
  
	NV_INFO(dev, "Detected an NV%2x generation card (0x%08x)\n",
		dev_priv->card_type, reg0);

	/* map larger RAMIN aperture on NV40 cards */
	dev_priv->ramin  = NULL;
	if (dev_priv->card_type >= NV_40) {
		int ramin_bar = 2;
		if (pci_resource_len(dev->pdev, ramin_bar) == 0)
			ramin_bar = 3;

		dev_priv->ramin_size = pci_resource_len(dev->pdev, ramin_bar);
		dev_priv->ramin = ioremap(
				pci_resource_start(dev->pdev, ramin_bar),
				dev_priv->ramin_size);
		if (!dev_priv->ramin) {
			NV_ERROR(dev, "Failed to init RAMIN mapping, "
				      "limited instance memory available\n");
		}
	}

	/* On older cards (or if the above failed), create a map covering
	 * the BAR0 PRAMIN aperture */
	if (!dev_priv->ramin) {
		dev_priv->ramin_size = 1 * 1024 * 1024;
		dev_priv->ramin = ioremap(mmio_start_offs + NV_RAMIN,
							dev_priv->ramin_size);
		if (!dev_priv->ramin) {
			NV_ERROR(dev, "Failed to map BAR0 PRAMIN.\n");
			return -ENOMEM;
		}
	}

	nouveau_OF_copy_vbios_to_ramin(dev);

	/* Special flags */
	if (dev->pci_device == 0x01a0)
		dev_priv->flags |= NV_NFORCE;
	else if (dev->pci_device == 0x01f0)
		dev_priv->flags |= NV_NFORCE2;

	/* For kernel modesetting, init card now and bring up fbcon */
	if (drm_core_check_feature(dev, DRIVER_MODESET)) {
		int ret = nouveau_card_init(dev);
		if (ret)
			return ret;
	}

	return 0;
}

static void nouveau_close(struct drm_device *dev)
{
	struct drm_nouveau_private *dev_priv = dev->dev_private;

	/* In the case of an error dev_priv may not be allocated yet */
	if (dev_priv)
		nouveau_card_takedown(dev);
}

/* KMS: we need mmio at load time, not when the first drm client opens. */
void nouveau_lastclose(struct drm_device *dev)
{
	if (drm_core_check_feature(dev, DRIVER_MODESET))
		return;

	nouveau_close(dev);
}

int nouveau_unload(struct drm_device *dev)
{
	struct drm_nouveau_private *dev_priv = dev->dev_private;

	if (drm_core_check_feature(dev, DRIVER_MODESET)) {
#if !defined(__AROS__)        
		if (dev_priv->card_type >= NV_50)
			nv50_display_destroy(dev);
		else
			nv04_display_destroy(dev);
#else
DRM_IMPL("Calling *_display_destroy\n");
#endif
		nouveau_close(dev);
	}

    if (dev_priv)
    {
	    iounmap(dev_priv->mmio);
	    iounmap(dev_priv->ramin);
        
	    kfree(dev_priv);
	    dev->dev_private = NULL;
	}
	return 0;
}

int nouveau_ioctl_getparam(struct drm_device *dev, void *data,
						struct drm_file *file_priv)
{
	struct drm_nouveau_private *dev_priv = dev->dev_private;
	struct drm_nouveau_getparam *getparam = data;

	NOUVEAU_CHECK_INITIALISED_WITH_RETURN;

	switch (getparam->param) {
	case NOUVEAU_GETPARAM_CHIPSET_ID:
		getparam->value = dev_priv->chipset;
		break;
	case NOUVEAU_GETPARAM_PCI_VENDOR:
		getparam->value = dev->pci_vendor;
		break;
	case NOUVEAU_GETPARAM_PCI_DEVICE:
		getparam->value = dev->pci_device;
		break;
	case NOUVEAU_GETPARAM_BUS_TYPE:
		if (drm_device_is_agp(dev))
			getparam->value = NV_AGP;
		else if (drm_device_is_pcie(dev))
			getparam->value = NV_PCIE;
		else
			getparam->value = NV_PCI;
		break;
	case NOUVEAU_GETPARAM_FB_PHYSICAL:
		getparam->value = dev_priv->fb_phys;
		break;
	case NOUVEAU_GETPARAM_AGP_PHYSICAL:
		getparam->value = dev_priv->gart_info.aper_base;
		break;
	case NOUVEAU_GETPARAM_PCI_PHYSICAL:
#if !defined(__AROS__)
		if (dev->sg) {
			getparam->value = (unsigned long)dev->sg->virtual;
		} else {
#else
        {
#endif
			NV_ERROR(dev, "Requested PCIGART address, "
					"while no PCIGART was created\n");
			return -EINVAL;
		}
		break;
	case NOUVEAU_GETPARAM_FB_SIZE:
		getparam->value = dev_priv->fb_available_size;
		break;
	case NOUVEAU_GETPARAM_AGP_SIZE:
		getparam->value = dev_priv->gart_info.aper_size;
		break;
	case NOUVEAU_GETPARAM_VM_VRAM_BASE:
		getparam->value = dev_priv->vm_vram_base;
		break;
	case NOUVEAU_GETPARAM_GRAPH_UNITS:
		/* NV40 and NV50 versions are quite different, but register
		 * address is the same. User is supposed to know the card
		 * family anyway... */
		if (dev_priv->chipset >= 0x40) {
			getparam->value = nv_rd32(dev, NV40_PMC_GRAPH_UNITS);
			break;
		}
		/* FALLTHRU */
	default:
		NV_ERROR(dev, "unknown parameter %lld\n", getparam->param);
		return -EINVAL;
	}

	return 0;
}

int
nouveau_ioctl_setparam(struct drm_device *dev, void *data,
		       struct drm_file *file_priv)
{
	struct drm_nouveau_setparam *setparam = data;

	NOUVEAU_CHECK_INITIALISED_WITH_RETURN;

	switch (setparam->param) {
	default:
		NV_ERROR(dev, "unknown parameter %lld\n", setparam->param);
		return -EINVAL;
	}

	return 0;
}

/* Wait until (value(reg) & mask) == val, up until timeout has hit */
bool nouveau_wait_until(struct drm_device *dev, uint64_t timeout,
			uint32_t reg, uint32_t mask, uint32_t val)
{
#if !defined(HOSTED_BUILD)
	struct drm_nouveau_private *dev_priv = dev->dev_private;
	struct nouveau_timer_engine *ptimer = &dev_priv->engine.timer;
	uint64_t start = ptimer->read(dev);

	do {
		if ((nv_rd32(dev, reg) & mask) == val)
			return true;
	} while (ptimer->read(dev) - start < timeout);

	return false;
#else
    return true;
#endif
}

/* Waits for PGRAPH to go completely idle */
bool nouveau_wait_for_idle(struct drm_device *dev)
{
	if (!nv_wait(NV04_PGRAPH_STATUS, 0xffffffff, 0x00000000)) {
		NV_ERROR(dev, "PGRAPH idle timed out with status 0x%08x\n",
			 nv_rd32(dev, NV04_PGRAPH_STATUS));
		return false;
	}

	return true;
}
