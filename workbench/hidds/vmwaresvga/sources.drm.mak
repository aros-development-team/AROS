### Lists of source files, included by Makefiles

# Linux 6.18.44 drivers/gpu/drm: the core subset, display helpers and TTM,
# with the Linux compatibility layer (drm-compat) and the AROS side of the
# core (drm-aros). The same base as the nouveau hidd carries; kept as a
# copy so that neither driver's tree reaches into the other's.
AROS_VMWGFX_DRMCORE_SOURCES = \
            drm/display/drm_dp_helper \
            drm/display/drm_dp_mst_topology \
            drm/display/drm_hdmi_helper \
            drm/display/drm_scdc_helper \
            drm/drm_atomic \
            drm/drm_atomic_helper \
            drm/drm_atomic_state_helper \
            drm/drm_atomic_uapi \
            drm/drm_blend \
            drm/drm_bridge \
            drm/drm_color_mgmt \
            drm/drm_connector \
            drm/drm_crtc \
            drm/drm_crtc_helper \
            drm/drm_damage_helper \
            drm/drm_displayid \
            drm/drm_edid \
            drm/drm_eld \
            drm/drm_encoder \
            drm/drm_fourcc \
            drm/drm_framebuffer \
            drm-aros/drm_mm \
            drm/drm_gem \
            drm/drm_mode_config \
            drm/drm_mode_object \
            drm/drm_modes \
            drm/drm_modeset_helper \
            drm/drm_modeset_lock \
            drm/drm_plane \
            drm/drm_plane_helper \
            drm/drm_print \
            drm/drm_probe_helper \
            drm/drm_property \
            drm/drm_rect \
            drm/drm_self_refresh_helper \
            drm/ttm/ttm_bo \
            drm/ttm/ttm_bo_util \
            drm/ttm/ttm_device \
            drm/ttm/ttm_module \
            drm/ttm/ttm_execbuf_util \
            drm/ttm/ttm_range_manager \
            drm/ttm/ttm_resource \
            drm/ttm/ttm_sys_manager \
            drm/ttm/ttm_tt \
            drm-compat/linux_compat \
            drm-compat/linux_dma \
            drm-compat/linux_dma_fence \
            drm-compat/linux_dma_fence_array \
            drm-compat/linux_dma_resv \
            drm-compat/linux_firmware \
            drm-compat/linux_hdmi \
            drm-compat/linux_i2c \
            drm-compat/linux_idr \
            drm-compat/linux_mmio \
            drm-compat/linux_irq \
            drm-compat/linux_mem \
            drm-compat/linux_pci \
            drm-compat/linux_printk \
            drm-compat/linux_sched \
            drm-compat/linux_string \
            drm-compat/linux_time \
            drm-compat/linux_workqueue \
            drm-aros/drm_drv \
            drm-aros/drm_aros_pci \
            drm-aros/drm_stubs \
            drm-aros/ttm_pool_aros \

# Linux 6.18.44 drivers/gpu/drm/vmwgfx, with the AROS glue that stands in
# for the PCI/module/file layers.
AROS_VMWGFX_DRM_SOURCES = \
            drm/vmwgfx/ttm_object \
            drm/vmwgfx/vmwgfx_binding \
            drm/vmwgfx/vmwgfx_blit \
            drm/vmwgfx/vmwgfx_bo \
            drm/vmwgfx/vmwgfx_cmd \
            drm/vmwgfx/vmwgfx_cmdbuf \
            drm/vmwgfx/vmwgfx_cmdbuf_res \
            drm/vmwgfx/vmwgfx_context \
            drm/vmwgfx/vmwgfx_cotable \
            drm/vmwgfx/vmwgfx_cursor_plane \
            drm/vmwgfx/vmwgfx_devcaps \
            drm/vmwgfx/vmwgfx_drv \
            drm/vmwgfx/vmwgfx_execbuf \
            drm/vmwgfx/vmwgfx_fence \
            drm/vmwgfx/vmwgfx_gem \
            drm/vmwgfx/vmwgfx_gmr \
            drm/vmwgfx/vmwgfx_gmrid_manager \
            drm/vmwgfx/vmwgfx_ioctl \
            drm/vmwgfx/vmwgfx_irq \
            drm/vmwgfx/vmwgfx_kms \
            drm/vmwgfx/vmwgfx_ldu \
            drm/vmwgfx/vmwgfx_mob \
            drm/vmwgfx/vmwgfx_msg \
            drm/vmwgfx/vmwgfx_overlay \
            drm-aros/vmwgfx_page_dirty \
            drm/vmwgfx/vmwgfx_prime \
            drm/vmwgfx/vmwgfx_resource \
            drm/vmwgfx/vmwgfx_scrn \
            drm/vmwgfx/vmwgfx_shader \
            drm/vmwgfx/vmwgfx_simple_resource \
            drm/vmwgfx/vmwgfx_so \
            drm/vmwgfx/vmwgfx_stdu \
            drm/vmwgfx/vmwgfx_streamoutput \
            drm/vmwgfx/vmwgfx_surface \
            drm/vmwgfx/vmwgfx_system_manager \
            drm/vmwgfx/vmwgfx_ttm_buffer \
            drm/vmwgfx/vmwgfx_va \
            drm/vmwgfx/vmwgfx_validation \
            drm/vmwgfx/vmwgfx_vkms \
            drm-aros/vmwgfx_aros \

# The in-process libdrm: the ioctl shim and the KMS wrappers.
AROS_VMWGFX_LIBDRM_SOURCES = \
            libdrm/arosdrm \
            libdrm/arosdrmmode \

# Mesa 26's winsys/svga/drm, built from the Mesa port tree (its AROS changes
# live in workbench/libs/mesa/mesa-26.0.0-aros.diff).
AROS_VMWGFX_WINSYS_SOURCES = \
            pb_buffer_simple_fenced \
            vmw_buffer \
            vmw_context \
            vmw_fence \
            vmw_msg \
            vmw_screen \
            vmw_screen_dri \
            vmw_screen_ioctl \
            vmw_screen_pools \
            vmw_screen_svga \
            vmw_surface \
            vmw_shader \
            vmw_query \

