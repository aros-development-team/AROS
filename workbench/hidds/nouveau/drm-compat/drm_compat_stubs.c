/*
    Copyright 2026, The AROS Development Team. All rights reserved.
*/

#include <drm-compat/drm_compat_funcs.h>

unsigned long clk_get_rate(struct clk *c) { NOT_IMPLEMENTED_STOP }

void dma_free_attrs(struct device *dev, size_t size, void *cpuaddr, dma_addr_t dma_handle, unsigned long attrs) { NOT_IMPLEMENTED_STOP }
void *dma_alloc_attrs(struct device *dev, size_t size, dma_addr_t *dma_handle, ULONG flags, unsigned long attrs) { NOT_IMPLEMENTED_STOP }
void *dma_alloc_coherent(struct device *dev, size_t size, dma_addr_t *dma_handle, ULONG flags) { NOT_IMPLEMENTED_STOP }
void dma_free_coherent(struct device *dev, size_t size, void *cpuaddr, dma_addr_t dma_handle) { NOT_IMPLEMENTED_STOP }

struct scatterlist *sg_next(struct scatterlist *s) { NOT_IMPLEMENTED_STOP }
dma_addr_t sg_dma_address(struct scatterlist *s) { NOT_IMPLEMENTED_STOP }
IPTR sg_dma_len(struct scatterlist *s) { NOT_IMPLEMENTED_STOP }

struct page *pfn_to_page(unsigned long pfn) { NOT_IMPLEMENTED_STOP }

size_t iommu_unmap(struct iommu_domain *domain, unsigned long iova, size_t size) { NOT_IMPLEMENTED_STOP }
int iommu_map(struct iommu_domain *domain, unsigned long iova, phys_addr_t paddr, size_t size, int prot) { NOT_IMPLEMENTED_STOP }

int dma_resv_copy_fences(struct dma_resv *dst, struct dma_resv *src) { NOT_IMPLEMENTED_STOP }

void drm_clflush_pages(struct page *pages[], unsigned long num_pages) { NOT_IMPLEMENTED_STOP }

unsigned int drm_debug;
