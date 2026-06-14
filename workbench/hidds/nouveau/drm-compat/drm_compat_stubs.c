/*
    Copyright 2026, The AROS Development Team. All rights reserved.
*/

#include <drm-compat/drm_compat_funcs.h>

unsigned long clk_get_rate(struct clk *c) { NOT_IMPLEMENTED_STOP }




void dma_free_attrs(struct device *dev, size_t size, void *cpuaddr, dma_addr_t dma_handle, unsigned long attrs) { NOT_IMPLEMENTED_STOP }
void *dma_alloc_attrs(struct device *dev, size_t size, dma_addr_t *dma_handle, ULONG flags, unsigned long attrs) { NOT_IMPLEMENTED_STOP }
void *dma_alloc_coherent(struct device *dev, size_t size, dma_addr_t *dma_handle, ULONG flags) { NOT_IMPLEMENTED_STOP }
void dma_free_coherent(struct device *dev, size_t size, void *cpuaddr, dma_addr_t dma_handle) { NOT_IMPLEMENTED_STOP }

int request_firmware(const struct firmware **fw, const char *name, struct device *device) { NOT_IMPLEMENTED_STOP }
int firmware_request_nowarn(const struct firmware **fw, const char *name, struct device *device) { NOT_IMPLEMENTED_STOP }
void release_firmware(const struct firmware *fw) { NOT_IMPLEMENTED_STOP }

struct scatterlist *sg_next(struct scatterlist *s) { NOT_IMPLEMENTED_STOP }
dma_addr_t sg_dma_address(struct scatterlist *s) { NOT_IMPLEMENTED_STOP }
IPTR sg_dma_len(struct scatterlist *s) { NOT_IMPLEMENTED_STOP }

struct page *pfn_to_page(unsigned long pfn) { NOT_IMPLEMENTED_STOP }






unsigned long find_first_zero_bit(const UBYTE *addr, unsigned long size) { NOT_IMPLEMENTED_STOP }

size_t iommu_unmap(struct iommu_domain *domain, unsigned long iova, size_t size) { NOT_IMPLEMENTED_STOP }
int iommu_map(struct iommu_domain *domain, unsigned long iova, phys_addr_t paddr, size_t size, int prot) { NOT_IMPLEMENTED_STOP }

int dma_resv_copy_fences(struct dma_resv *dst, struct dma_resv *src) { NOT_IMPLEMENTED_STOP }

void drm_clflush_pages(struct page *pages[], unsigned long num_pages) { NOT_IMPLEMENTED_STOP }

unsigned int drm_debug;






















void drm_atomic_helper_legacy_gamma_set() { NOT_IMPLEMENTED_STOP }
void drm_atomic_helper_page_flip() { NOT_IMPLEMENTED_STOP }




















void drm_connector_set_path_property() { NOT_IMPLEMENTED_STOP }

void drm_crtc_cleanup() { NOT_IMPLEMENTED_STOP }
void drm_crtc_helper_set_config() { NOT_IMPLEMENTED_STOP }
void drm_crtc_helper_set_mode() { NOT_IMPLEMENTED_STOP }

void drm_cvt_mode() { NOT_IMPLEMENTED_STOP }
void drm_dp_atomic_find_vcpi_slots() { NOT_IMPLEMENTED_STOP }
void drm_dp_atomic_release_vcpi_slots() { NOT_IMPLEMENTED_STOP }
void drm_dp_aux_register() { NOT_IMPLEMENTED_STOP }
void drm_dp_aux_unregister() { NOT_IMPLEMENTED_STOP }
void drm_dp_calc_pbn_mode() { NOT_IMPLEMENTED_STOP }
void drm_dp_check_act_status() { NOT_IMPLEMENTED_STOP }
void drm_dp_dpcd_read() { NOT_IMPLEMENTED_STOP }
void drm_dp_dpcd_write() { NOT_IMPLEMENTED_STOP }
void drm_dp_mst_allocate_vcpi() { NOT_IMPLEMENTED_STOP }

void drm_dp_mst_deallocate_vcpi() { NOT_IMPLEMENTED_STOP }
void drm_dp_mst_detect_port() { NOT_IMPLEMENTED_STOP }
void drm_dp_mst_get_port_malloc() { NOT_IMPLEMENTED_STOP }
void drm_dp_mst_hpd_irq() { NOT_IMPLEMENTED_STOP }
void drm_dp_mst_put_port_malloc() { NOT_IMPLEMENTED_STOP }
void drm_dp_mst_reset_vcpi_slots() { NOT_IMPLEMENTED_STOP }
void drm_dp_mst_topology_mgr_destroy() { NOT_IMPLEMENTED_STOP }
void drm_dp_mst_topology_mgr_init() { NOT_IMPLEMENTED_STOP }
void drm_dp_mst_topology_mgr_resume() { NOT_IMPLEMENTED_STOP }
void drm_dp_mst_topology_mgr_set_mst() { NOT_IMPLEMENTED_STOP }
void drm_dp_mst_topology_mgr_suspend() { NOT_IMPLEMENTED_STOP }
void drm_dp_update_payload_part1() { NOT_IMPLEMENTED_STOP }
void drm_dp_update_payload_part2() { NOT_IMPLEMENTED_STOP }
void drm_encoder_cleanup() { NOT_IMPLEMENTED_STOP }



void drm_helper_connector_dpms() { NOT_IMPLEMENTED_STOP }
void drm_helper_hpd_irq_event() { NOT_IMPLEMENTED_STOP }


void drm_i2c_encoder_detect() { NOT_IMPLEMENTED_STOP }
void drm_i2c_encoder_mode_fixup() { NOT_IMPLEMENTED_STOP }
void drm_i2c_encoder_restore() { NOT_IMPLEMENTED_STOP }
void drm_i2c_encoder_save() { NOT_IMPLEMENTED_STOP }
void drm_mode_config_cleanup() { NOT_IMPLEMENTED_STOP }





void drm_mode_create_tv_properties() { NOT_IMPLEMENTED_STOP }










void drm_plane_cleanup() { NOT_IMPLEMENTED_STOP }
void drm_plane_create_alpha_property() { NOT_IMPLEMENTED_STOP }
void drm_plane_create_blend_mode_property() { NOT_IMPLEMENTED_STOP }


void drm_primary_helper_funcs() { NOT_IMPLEMENTED_STOP }




