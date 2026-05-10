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
            drm/ttm/ttm_bo \
            drm/ttm/ttm_tt \
            drm/ttm/ttm_bo_util \
            drm/ttm/ttm_memory \
            drm-compat/drm_compat_funcs \
            drm-compat/drm_compat_stubs \

AROS_DRM_NVIDIA_SOURCES = \
            drm/nouveau/nvkm/engine/device/engine_device \
            drm/nouveau/nvkm/engine/gr/engine_gr \
            drm/nouveau/nvkm/engine/gr/ctxnv40 \
            drm/nouveau/nvkm/engine/bsp/engine_bsp \
            drm/nouveau/nvkm/engine/ce/engine_ce \
            drm/nouveau/nvkm/engine/cipher/engine_cipher \
            drm/nouveau/nvkm/engine/disp/engine_disp \
            drm/nouveau/nvkm/engine/disp/rootnv04 \
            drm/nouveau/nvkm/engine/disp/rootnv50 \
            drm/nouveau/nvkm/engine/dma/engine_dma \
            drm/nouveau/nvkm/engine/falcon \
            drm/nouveau/nvkm/engine/fifo/engine_fifo \
            drm/nouveau/nvkm/engine/mpeg/engine_mpeg \
            drm/nouveau/nvkm/engine/mspdec/engine_mspdec \
            drm/nouveau/nvkm/engine/msppp/engine_msppp \
            drm/nouveau/nvkm/engine/msvld/engine_msvld \
            drm/nouveau/nvkm/engine/nvdec/engine_nvdec \
            drm/nouveau/nvkm/engine/pm/engine_pm \
            drm/nouveau/nvkm/engine/sec/engine_sec \
            drm/nouveau/nvkm/engine/sec2/engine_sec2 \
            drm/nouveau/nvkm/engine/sw/engine_sw \
            drm/nouveau/nvkm/engine/vp/engine_vp \
            drm/nouveau/nvkm/engine/xtensa \
            drm/nouveau/nvkm/subdev/bar/subdev_bar \
            drm/nouveau/nvkm/subdev/bios/subdev_bios \
            drm/nouveau/nvkm/subdev/bios/shadowramin \
            drm/nouveau/nvkm/subdev/bus/subdev_bus \
            drm/nouveau/nvkm/subdev/clk/subdev_clk_1 \
            drm/nouveau/nvkm/subdev/clk/subdev_clk_2 \
            drm/nouveau/nvkm/subdev/clk/subdev_clk_3 \
            drm/nouveau/nvkm/subdev/clk/subdev_clk_4 \
            drm/nouveau/nvkm/subdev/clk/subdev_clk_5 \
            drm/nouveau/nvkm/subdev/clk/subdev_clk_6 \
            drm/nouveau/nvkm/subdev/devinit/subdev_devinit \
            drm/nouveau/nvkm/subdev/fault/subdev_fault \
            drm/nouveau/nvkm/subdev/fb/subdev_fb \
            drm/nouveau/nvkm/subdev/fb/sddr2 \
            drm/nouveau/nvkm/subdev/fb/sddr3 \
            drm/nouveau/nvkm/subdev/fb/ramnv50 \
            drm/nouveau/nvkm/subdev/fb/ramgt215 \
            drm/nouveau/nvkm/subdev/fuse/subdev_fuse \
            drm/nouveau/nvkm/subdev/gpio/subdev_gpio \
            drm/nouveau/nvkm/subdev/gsp/subdev_gsp \
            drm/nouveau/nvkm/subdev/i2c/subdev_i2c \
            drm/nouveau/nvkm/subdev/ibus/subdev_ibus \
            drm/nouveau/nvkm/subdev/iccsense/subdev_iccsense \
            drm/nouveau/nvkm/subdev/instmem/subdev_instmem \
            drm/nouveau/nvkm/subdev/ltc/subdev_ltc \
            drm/nouveau/nvkm/subdev/mc/subdev_mc \
            drm/nouveau/nvkm/subdev/mmu/subdev_mmu \
            drm/nouveau/nvkm/subdev/mxm/subdev_mxm \
            drm/nouveau/nvkm/subdev/pci/subdev_pci \
            drm/nouveau/nvkm/subdev/pmu/subdev_pmu_1 \
            drm/nouveau/nvkm/subdev/pmu/subdev_pmu_2 \
            drm/nouveau/nvkm/subdev/secboot/subdev_secboot \
            drm/nouveau/nvkm/subdev/therm/subdev_therm_1 \
            drm/nouveau/nvkm/subdev/therm/subdev_therm_2 \
            drm/nouveau/nvkm/subdev/timer/subdev_timer \
            drm/nouveau/nvkm/subdev/top/subdev_top \
            drm/nouveau/nvkm/subdev/volt/subdev_volt \
            drm/nouveau/nvkm/core/core_root \
            drm/nouveau/nvkm/falcon/falcon_root \
            drm/nouveau/nvkm/falcon/msgqueue_0137c63d \
            drm/nouveau/nvkm/falcon/msgqueue_0148cdec \
            drm/nouveau/nvif/nvif_root \
            drm/nouveau/nouveau_bo \
            drm/nouveau/nouveau_fence \
            drm/nouveau/nouveau_vmm \
            drm/nouveau/nouveau_dma \
            drm/nouveau/nouveau_ttm \
            drm/nouveau/nouveau_mem \
            drm/nouveau/nouveau_sgdma \

