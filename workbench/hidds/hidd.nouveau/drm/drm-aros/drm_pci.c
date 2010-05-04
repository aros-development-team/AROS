/*
    Copyright 2009, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "drmP.h"

drm_dma_handle_t *drm_pci_alloc(struct drm_device *dev, size_t size,
				       size_t align, dma_addr_t maxaddr)
{
    drm_dma_handle_t *dmah;

	dmah = kmalloc(sizeof(drm_dma_handle_t), GFP_KERNEL);
	if (!dmah)
		return NULL;

	dmah->size = size;
	dmah->vaddr = AllocVec(size + align - 1, MEMF_PUBLIC | MEMF_CLEAR);
    /* FIXME: address for FreeVec is lost */
	dmah->vaddr = (APTR)(((IPTR)(dmah->vaddr) + align - 1) & ~(align - 1));
	dmah->busaddr = (dma_addr_t)dmah->vaddr;

	if (dmah->vaddr == NULL) {
		kfree(dmah);
		return NULL;
	}
	
	return dmah;   
}

void drm_pci_free(struct drm_device * dev, drm_dma_handle_t * dmah)
{
    /* FIXME: Free dmah->vaddr */
    kfree(dmah);
}
				       
