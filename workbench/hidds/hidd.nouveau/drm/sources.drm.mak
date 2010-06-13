### Lists of source files, included by Makefiles

AROS_LIBDRM_CORE_SOURCES = \
            libdrm/arosdrm \
            libdrm/arosdrmmode \
            
AROS_LIBDRM_NVIDIA_SOURCES = \
            libdrm/nouveau/nouveau_device \
            libdrm/nouveau/nouveau_resource \
            libdrm/nouveau/nouveau_pushbuf \
            libdrm/nouveau/nouveau_notifier \
            libdrm/nouveau/nouveau_grobj \
            libdrm/nouveau/nouveau_channel \
            libdrm/nouveau/nouveau_bo \
            libdrm/nouveau/nouveau_reloc \
            libdrm/arosdrm_nouveau \

AROS_DRM_CORE_SOURCES = \
            drm-aros/drm_aros \
            drm-aros/drm_bufs \
            drm-aros/drm_irq \
            drm-aros/drm_pci \
            drm-aros/drm_drv \
            drm-aros/drm_compat_funcs \
            drm-changed/drm_mm \
            drm-changed/drm_gem \
            drm-changed/drm_cache \
            drm-changed/drm_crtc \
            drm-changed/drm_edid \
            drm-changed/drm_fb_helper \
            drm-changed/drm_memory \
            drm-changed/drm_modes \
            drm-changed/drm_agpsupport \
            drm-changed/ttm/ttm_global \
            drm-changed/ttm/ttm_bo \
            drm-changed/ttm/ttm_bo_util \
            drm-changed/ttm/ttm_tt \
            drm-changed/ttm/ttm_agp_backend \
            drm-unchanged/drm_crtc_helper \

AROS_DRM_NVIDIA_SOURCES = \
            drm-aros/nouveau/nouveau_drv \
            drm-changed/nouveau/nouveau_bios \
            drm-changed/nouveau/nouveau_dp \
            drm-changed/nouveau/nouveau_i2c \
            drm-changed/nouveau/nouveau_calc \
            drm-changed/nouveau/nouveau_connector \
            drm-changed/nouveau/nouveau_hw \
            drm-changed/nouveau/nouveau_state \
            drm-changed/nouveau/nouveau_mem \
            drm-changed/nouveau/nouveau_fbcon \
            drm-changed/nouveau/nouveau_fence \
            drm-changed/nouveau/nouveau_grctx \
            drm-changed/nouveau/nouveau_sgdma \
            drm-changed/nouveau/nouveau_ttm \
            drm-changed/nouveau/nouveau_irq \
            drm-changed/nouveau/nv04_display \
            drm-changed/nouveau/nv04_tv \
            drm-changed/nouveau/nv50_instmem \
            drm-changed/nouveau/nv50_display \
            drm-changed/nouveau/nouveau_bo_renamed \
            drm-unchanged/nouveau/nouveau_channel_renamed \
            drm-unchanged/nouveau/nouveau_display \
            drm-unchanged/nouveau/nouveau_dma \
            drm-unchanged/nouveau/nouveau_object \
            drm-unchanged/nouveau/nouveau_notifier_renamed \
            drm-unchanged/nouveau/nouveau_gem \
            drm-unchanged/nouveau/nv04_crtc \
            drm-unchanged/nouveau/nv04_cursor \
            drm-unchanged/nouveau/nv04_dac \
            drm-unchanged/nouveau/nv04_dfp \
            drm-unchanged/nouveau/nv04_fb \
            drm-unchanged/nouveau/nv04_fifo \
            drm-unchanged/nouveau/nv04_graph \
            drm-unchanged/nouveau/nv04_instmem \
            drm-unchanged/nouveau/nv04_mc \
            drm-unchanged/nouveau/nv04_timer \
            drm-unchanged/nouveau/nv10_fb \
            drm-unchanged/nouveau/nv10_fifo \
            drm-unchanged/nouveau/nv10_graph \
            drm-unchanged/nouveau/nv17_gpio \
            drm-unchanged/nouveau/nv17_tv \
            drm-unchanged/nouveau/nv17_tv_modes \
            drm-unchanged/nouveau/nv20_graph \
            drm-unchanged/nouveau/nv40_fb \
            drm-unchanged/nouveau/nv40_fifo \
            drm-unchanged/nouveau/nv40_graph \
            drm-unchanged/nouveau/nv40_grctx \
            drm-unchanged/nouveau/nv40_mc \
            drm-unchanged/nouveau/nv50_crtc \
            drm-unchanged/nouveau/nv50_cursor \
            drm-unchanged/nouveau/nv50_dac \
            drm-unchanged/nouveau/nv50_fifo \
            drm-unchanged/nouveau/nv50_mc \
            drm-unchanged/nouveau/nv50_gpio \
            drm-unchanged/nouveau/nv50_graph \
            drm-unchanged/nouveau/nv50_grctx \
            drm-unchanged/nouveau/nv50_sor \

