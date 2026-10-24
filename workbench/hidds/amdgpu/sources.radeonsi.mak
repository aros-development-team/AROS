AROS_AMDGPU_LIBDRM_AMDGPU_SOURCES := \
                libdrm/amdgpu/amdgpu_asic_id \
                libdrm/amdgpu/amdgpu_bo \
                libdrm/amdgpu/amdgpu_cs \
                libdrm/amdgpu/amdgpu_device \
                libdrm/amdgpu/amdgpu_gpu_info \
                libdrm/amdgpu/amdgpu_userq \
                libdrm/amdgpu/amdgpu_vamgr \
                libdrm/amdgpu/amdgpu_vm \
                libdrm/amdgpu/handle_table

AROS_AMDGPU_RADEONSI_C_SOURCES := \
                gfx11_query \
                si_barrier \
                si_buffer \
                si_clear \
                si_context \
                si_cp_dma \
                si_cp_utils \
                si_debug \
                si_descriptors \
                si_fence \
                si_get \
                si_gfx_cs \
                si_gpu_load \
                si_perfcounter \
                si_pipe \
                si_pm4 \
                si_query \
                si_sdma_copy_image \
                si_state \
                si_state_binning \
                si_state_msaa \
                si_state_streamout \
                si_state_viewport \
                si_texture \
                si_utrace \
                gfx/si_blit \
                gfx/si_cp_reg_shadowing \
                gfx/si_compute_blit \
                gfx/si_compute \
                gfx/si_debug_gfx_compute \
                gfx/si_gfx_context \
                gfx/si_gfx_screen \
                gfx/si_shader_cache \
                gfx/si_nir_mediump \
                gfx/si_mesh_shader \
                gfx/si_shader_aco \
                gfx/si_shader_args \
                gfx/si_shader_binary \
                gfx/si_shader_info \
                gfx/si_shader_nir \
                gfx/si_shader_variant_info \
                gfx/si_shader \
                gfx/si_shaderlib_nir \
                gfx/si_sqtt \
                gfx/si_nir_clamp_shadow_comparison_value \
                gfx/si_nir_kill_outputs \
                gfx/si_nir_lower_abi \
                gfx/si_nir_lower_intrinsics_early \
                gfx/si_nir_lower_polygon_stipple \
                gfx/si_nir_lower_color_flatshade_twoside \
                gfx/si_nir_lower_resource \
                gfx/si_nir_lower_vs_inputs \
                gfx/si_nir_mark_divergent_texture_non_uniform \
                gfx/si_nir_optim \
                tests/si_tests \
                tests/si_test_oa_bandwidth \
                tests/si_test_dma_perf \
                tests/si_test_image_copy_region \
                tests/si_test_vm_fault

AROS_AMDGPU_RADEONSI_CXX_SOURCES := \
                si_perfetto \
                gfx/si_state_shaders

AROS_AMDGPU_RADEONSI_DRAW_SOURCES := \
                radeonsi/si_state_draw_gfx10 \
                radeonsi/si_state_draw_gfx103 \
                radeonsi/si_state_draw_gfx11 \
                radeonsi/si_state_draw_gfx115 \
                radeonsi/si_state_draw_gfx117 \
                radeonsi/si_state_draw_gfx12 \
                radeonsi/si_state_draw_gfx6 \
                radeonsi/si_state_draw_gfx7 \
                radeonsi/si_state_draw_gfx8 \
                radeonsi/si_state_draw_gfx9

AROS_AMDGPU_WINSYS_C_SOURCES := \
                amdgpu_bo \
                amdgpu_surface \
                amdgpu_userq \
                amdgpu_winsys

AROS_AMDGPU_WINSYS_CXX_SOURCES := \
                amdgpu_cs

AROS_AMDGPU_RADEONSI_VIDEO_SOURCES := \
                mm/radeon_bitstream \
                mm/radeon_uvd_enc \
                mm/radeon_vce \
                mm/radeon_vcn \
                mm/radeon_vcn_enc \
                mm/radeon_vcn_enc_1_2 \
                mm/radeon_vcn_enc_2_0 \
                mm/radeon_vcn_enc_3_0 \
                mm/radeon_vcn_enc_4_0 \
                mm/radeon_vcn_enc_5_0 \
                mm/si_mm_context \
                mm/si_mm_screen \
                mm/si_video \
                mm/si_video_dec
# mm/si_vpe needs vpelib + gmlib + lanczoslib, which are not built; the
# radeonsi_aros.c stub stands in for si_vpe_create_processor.
