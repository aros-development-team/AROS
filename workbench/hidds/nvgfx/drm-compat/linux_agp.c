/*
    Copyright 2009-2026, The AROS Development Team. All rights reserved.
*/

#include <aros/debug.h>
#include <proto/exec.h>
#include <proto/oop.h>
#include <hidd/hidd.h>

#include <linux/kernel.h>
#include <linux/agp_backend.h>
#include <linux/pci.h>
#include <linux/slab.h>
#include <linux/mm.h>

#include <drm-compat/drm_compat_mem.h>
#include <drm-compat/drm_compat_funcs.h>

#include <hidd/agp.h>

/* AGP handling */
struct agp_bridge_data * global_agp_bridge = NULL; /* TODO: implement freeing */
struct Library * HiddAgpBase = NULL; /* TODO: Implement  freeing */
OOP_AttrBase HiddAGPBridgeDeviceAttrBase = 0; /* TODO: Implement  freeing */

struct agp_bridge_data *agp_backend_acquire(struct pci_dev *dev)
{
    /* TODO:
       if no bridge return NULL
       if already acquired return NULL, else acquire
     */
    return agp_find_bridge(dev);
}

void agp_backend_release(struct agp_bridge_data * bridge)
{
    /* TODO: release acquired lock */
}

void agp_free_memory(struct agp_memory * mem)
{
    HIDDNouveauFree(mem->pages);
    HIDDNouveauFree(mem);
}

struct agp_memory *agp_allocate_memory(struct agp_bridge_data * bridge, 
    size_t num_pages , u32 type)
{
    if ((type != AGP_USER_MEMORY) && (type != AGP_USER_CACHED_MEMORY))
    {
        IMPLEMENT("Unsupported memory type: %d\n", type);
        return NULL;
    }
    
    struct agp_memory * mem = HIDDNouveauAlloc(sizeof(struct agp_memory));
    mem->pages = HIDDNouveauAlloc(sizeof(struct page *) * num_pages);
    mem->page_count = 0; /* Not a typo, will be filled later */
    mem->type = type;
    mem->is_flushed = FALSE;
    mem->is_bound = FALSE;
    mem->pg_start = 0;
    return mem;
}

int agp_copy_info(struct agp_bridge_data * bridge, struct agp_kern_info * info)
{
    info->cant_use_aperture = 0;
    info->page_mask = ~0UL;
    if (bridge->mode & (1<<3) /* AGPSTAT_MODE_3_0 */)
        info->mode = bridge->mode & ~(0x00ff00c4); /* AGP3_RESERVED_MASK */
    else
        info->mode = bridge->mode & ~(0x00fffcc8); /* AGP2_RESERVED_MASK */

    info->aper_base = (unsigned long)bridge->aperturebase;
    info->aper_size = (unsigned long)bridge->aperturesize;    
    
    return 0;
}

struct agp_bridge_data * agp_find_bridge(struct pci_dev *dev)
{
    OOP_Object * agpbus = NULL;

    if (global_agp_bridge)
        return global_agp_bridge;

    if (!HiddAgpBase)
    {
        HiddAgpBase = OpenLibrary("agp.hidd", 1);
        HiddAGPBridgeDeviceAttrBase = OOP_ObtainAttrBase((STRPTR)IID_Hidd_AGPBridgeDevice);
    }

    /* Get AGP bus object */
    agpbus = OOP_NewObject(NULL, CLID_Hidd_AGP, NULL);
    
    if(agpbus)
    {
        struct pHidd_AGP_GetBridgeDevice gbdmsg = {
        mID : OOP_GetMethodID(IID_Hidd_AGP, moHidd_AGP_GetBridgeDevice)
        };
        OOP_Object * bridgedevice = NULL;

        bridgedevice = (OOP_Object*)OOP_DoMethod(agpbus, (OOP_Msg)&gbdmsg);
        
        OOP_DisposeObject(agpbus);

        /* AGP bridge was found and initialized */        
        if (bridgedevice)
        {
            IPTR mode = 0, aperbase = 0, apersize = 0;

            global_agp_bridge = HIDDNouveauAlloc(sizeof(struct agp_bridge_data));
            global_agp_bridge->agpbridgedevice = (IPTR)bridgedevice;

            OOP_GetAttr(bridgedevice, aHidd_AGPBridgeDevice_Mode, (APTR)&mode);
            global_agp_bridge->mode = mode;
            
            OOP_GetAttr(bridgedevice, aHidd_AGPBridgeDevice_ApertureBase, (APTR)&aperbase);
            global_agp_bridge->aperturebase = aperbase;
            
            OOP_GetAttr(bridgedevice, aHidd_AGPBridgeDevice_ApertureSize, (APTR)&apersize);
            global_agp_bridge->aperturesize = apersize;
        }
    }

    return global_agp_bridge;
}

void agp_enable(struct agp_bridge_data * bridge, u32 mode)
{
    if (!bridge || !bridge->agpbridgedevice)
        return;

    struct pHidd_AGPBridgeDevice_Enable emsg = {
    mID:            OOP_GetMethodID(IID_Hidd_AGPBridgeDevice, moHidd_AGPBridgeDevice_Enable),
    requestedmode:  mode
    };
    
    OOP_DoMethod((OOP_Object *)bridge->agpbridgedevice, (OOP_Msg)&emsg);
}

int agp_bind_memory(struct agp_memory * mem, off_t offset)
{
    if (!mem || mem->is_bound)
        return -EINVAL;
    
    if ((mem->type != AGP_USER_MEMORY) && (mem->type != AGP_USER_CACHED_MEMORY))
    {
        IMPLEMENT("Unsupported memory type: %d\n", mem->type);
        return -EINVAL;
    }

    if (!mem->is_flushed)
    {
        /* TODO: Flush memory */
        mem->is_flushed = TRUE;
    }
    
    /* TODO: agp_map_memory */
    /* TODO: Move flush/map into bind call on the side of agp.hidd */

    struct pHidd_AGPBridgeDevice_BindMemory bmmsg = {
    mID:        OOP_GetMethodID(IID_Hidd_AGPBridgeDevice, moHidd_AGPBridgeDevice_BindMemory),
    address:    (IPTR)page_address(mem->pages[0]),
    size:       mem->page_count * PAGE_SIZE,
    offset:     offset,
    type:       (mem->type == AGP_USER_MEMORY ? vHidd_AGP_NormalMemory : vHidd_AGP_CachedMemory)
    };
    
    OOP_DoMethod((OOP_Object *)global_agp_bridge->agpbridgedevice, (OOP_Msg)&bmmsg);

    mem->is_bound = TRUE;
    mem->pg_start = offset;
    return 0;
}

int agp_unbind_memory(struct agp_memory * mem)
{
    if (!mem || !mem->is_bound)
        return -EINVAL;

    struct pHidd_AGPBridgeDevice_UnBindMemory ubmmsg = {
    mID:        OOP_GetMethodID(IID_Hidd_AGPBridgeDevice, moHidd_AGPBridgeDevice_UnBindMemory),
    offset:     mem->pg_start,
    size:       mem->page_count * PAGE_SIZE,
    };
    
    OOP_DoMethod((OOP_Object *)global_agp_bridge->agpbridgedevice, (OOP_Msg)&ubmmsg);

    /* TODO: agp_unmap_memory */

    mem->is_bound = FALSE;
    mem->pg_start = 0;
    return 0;
}

void agp_flush_chipset(struct agp_bridge_data * bridge)
{
    if (!bridge || !bridge->agpbridgedevice)
        return;

    struct pHidd_AGPBridgeDevice_FlushChipset fcmsg = {
    mID:        OOP_GetMethodID(IID_Hidd_AGPBridgeDevice, moHidd_AGPBridgeDevice_FlushChipset),
    };
    
    OOP_DoMethod((OOP_Object *)bridge->agpbridgedevice, (OOP_Msg)&fcmsg);
}

