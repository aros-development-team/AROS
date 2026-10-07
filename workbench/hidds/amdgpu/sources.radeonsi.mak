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
                si_blit \
                si_buffer \
                si_clear \
                si_compute \
                si_compute_blit \
                si_cp_dma \
                si_cp_reg_shadowing \
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
                si_mesh_shader \
                si_nir_clamp_shadow_comparison_value \
                si_nir_kill_outputs \
                si_nir_lower_abi \
                si_nir_lower_intrinsics_early \
                si_nir_lower_polygon_stipple \
                si_nir_lower_color_flatshade_twoside \
                si_nir_lower_resource \
                si_nir_lower_vs_inputs \
                si_nir_mark_divergent_texture_non_uniform \
                si_nir_optim \
                si_sdma_copy_image \
                si_shader \
                si_shader_aco \
                si_shader_args \
                si_shader_binary \
                si_shader_info \
                si_shader_nir \
                si_shader_variant_info \
                si_shaderlib_nir \
                si_sqtt \
                si_state \
                si_state_binning \
                si_state_msaa \
                si_state_streamout \
                si_state_viewport \
                si_test_blit_perf \
                si_test_dma_perf \
                si_test_image_copy_region \
                si_texture \
                si_utrace

AROS_AMDGPU_RADEONSI_CXX_SOURCES := \
                si_perfetto \
                si_state_shaders

AROS_AMDGPU_RADEONSI_DRAW_SOURCES := \
                radeonsi/si_state_draw_gfx10 \
                radeonsi/si_state_draw_gfx103 \
                radeonsi/si_state_draw_gfx11 \
                radeonsi/si_state_draw_gfx115 \
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
                si_uvd \
                radeon_uvd \
                radeon_uvd_enc \
                radeon_vce \
                radeon_vcn \
                radeon_vcn_dec \
                radeon_vcn_dec_jpeg \
                radeon_vcn_enc \
                radeon_vcn_enc_1_2 \
                radeon_vcn_enc_2_0 \
                radeon_vcn_enc_3_0 \
                radeon_vcn_enc_4_0 \
                radeon_vcn_enc_5_0 \
                radeon_video \
                radeon_bitstream
