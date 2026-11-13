/*
    Copyright 2009-2026, The AROS Development Team. All rights reserved.
*/

#ifndef _LINUX_IOMMU_H_
#define _LINUX_IOMMU_H_

#include <linux/types.h>
#include <linux/device.h>
#include <linux/scatterlist.h>
#include <linux/errno.h>
#include <linux/err.h>
#include <linux/of.h>
struct iommu_domain { unsigned long pgsize_bitmap; };
struct iommu_group;
#define IOMMU_READ              (1 << 0)
#define IOMMU_WRITE             (1 << 1)
#define IOMMU_CACHE             (1 << 2)
#define IOMMU_DOMAIN_UNMANAGED  0
static inline struct iommu_domain *iommu_domain_alloc(void *bus) { return NULL; }
static inline struct iommu_domain *iommu_paging_domain_alloc(struct device *dev) { return ERR_PTR(-ENODEV); }
static inline void iommu_domain_free(struct iommu_domain *d) { }
static inline int iommu_attach_device(struct iommu_domain *d, struct device *dev) { return -ENODEV; }
static inline void iommu_detach_device(struct iommu_domain *d, struct device *dev) { }
static inline int iommu_map(struct iommu_domain *d, unsigned long iova, phys_addr_t p, size_t s, int prot, gfp_t gfp) { return -ENODEV; }
static inline size_t iommu_unmap(struct iommu_domain *d, unsigned long iova, size_t s) { return 0; }
static inline struct iommu_domain *iommu_get_domain_for_dev(struct device *dev) { return NULL; }
static inline bool iommu_present(void *bus) { return false; }
static inline bool device_iommu_mapped(struct device *dev) { return false; }
static inline int tegra_dev_iommu_get_stream_id(struct device *dev, u32 *stream_id) { return -ENODEV; }
static inline unsigned long iommu_domain_pgsize_bitmap(struct iommu_domain *d) { return 0; }

#endif /* _LINUX_IOMMU_H_ */
