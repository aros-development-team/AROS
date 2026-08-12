/*
    Copyright 2009-2026, The AROS Development Team. All rights reserved.
*/

#include <drm-compat/drm_compat_funcs.h>

#define PIO_RESERVED                (IPTR)0x40000UL

void iowrite32(u32 val, void * addr)
{
    if ((IPTR)addr >= PIO_RESERVED)
        writel(val, addr);
    else
        IMPLEMENT("PIO\n");
}

unsigned int ioread32(void * addr)
{
    if ((IPTR)addr >= PIO_RESERVED)
        return readl(addr);
    else
        IMPLEMENT("PIO\n");
    
    return 0xffffffff;
}

void iowrite16(u16 val, void * addr)
{
    if ((IPTR)addr >= PIO_RESERVED)
        writew(val, addr);
    else
        IMPLEMENT("PIO\n");
}

unsigned int ioread16(void * addr)
{
    if ((IPTR)addr >= PIO_RESERVED)
        return readw(addr);
    else
        IMPLEMENT("PIO\n");
    
    return 0xffff;
}

void iowrite8(u8 val, void * addr)
{
    if ((IPTR)addr >= PIO_RESERVED)
        writeb(val, addr);
    else
        IMPLEMENT("PIO\n");
}

unsigned int ioread8(void * addr)
{
    if ((IPTR)addr >= PIO_RESERVED)
        return readb(addr);
    else
        IMPLEMENT("PIO\n");
    
    return 0xff;
}

/* Delay handling */
#include <aros/symbolsets.h>
#include <devices/timer.h>
struct MsgPort *p = NULL;
struct timerequest *io = NULL;

void udelay(unsigned long usecs)
{
    /* Create local clone so that the function is task safe */
    struct timerequest  timerio;
    struct MsgPort 	timermp;

    memset(&timermp, 0, sizeof(timermp));

    timermp.mp_Node.ln_Type = NT_MSGPORT;
    timermp.mp_Flags 	    = PA_SIGNAL;
    timermp.mp_SigBit	    = SIGB_SINGLE;
    timermp.mp_SigTask	    = FindTask(NULL);    
    NEWLIST(&timermp.mp_MsgList);

    timerio = *io;

    timerio.tr_node.io_Message.mn_Node.ln_Type  = NT_REPLYMSG;
    timerio.tr_node.io_Message.mn_ReplyPort     = &timermp;    
    timerio.tr_node.io_Command                  = TR_ADDREQUEST;
    timerio.tr_time.tv_secs                     = usecs / 1000000;
    timerio.tr_time.tv_micro                    = usecs % 1000000;

    SetSignal(0, SIGF_SINGLE);

    DoIO((struct IORequest*)&timerio);
}

static BOOL init_timer()
{
    p = CreateMsgPort();
    
    if (!p)
        return FALSE;

    io = CreateIORequest(p, sizeof(struct timerequest));
    
    if(!io)
    {
        DeleteMsgPort(p);
        return FALSE;
    }
    
    if (OpenDevice("timer.device", UNIT_MICROHZ, (struct IORequest *)io, 0) != 0)
    {
        DeleteIORequest(io);
        DeleteMsgPort(p);
    }

    return TRUE;
}

static BOOL deinit_timer()
{
    if (io)
    {
        CloseDevice((struct IORequest *)io);
        DeleteIORequest((struct IORequest *)io);
        io = NULL;
    }
    
    if (p)
    {
        DeleteMsgPort(p);
        p = NULL;
    }
    
    return TRUE;
}

ADD2INIT(init_timer, 5);
ADD2EXIT(deinit_timer, 5);

/* Page handling */
void __free_page(struct page * p)
{
    if (p->allocated_buffer)
        HIDDNouveauFree(p->allocated_buffer);
    p->allocated_buffer = NULL;
    p->address = NULL;
    HIDDNouveauFree(p);
}

static struct page * create_page_helper()
{
    struct page * p;
    p = HIDDNouveauAlloc(sizeof(*p));
    p->allocated_buffer = HIDDNouveauAlloc(PAGE_SIZE + PAGE_SIZE - 1);
    p->address = PAGE_ALIGN(p->allocated_buffer);
    return p;
}

struct page *alloc_page(ULONG mask)
{
    return create_page_helper();
}

#include <hidd/agp.h>

/* AGP handling */
struct agp_bridge_data * global_agp_bridge = NULL; /* TODO: implement freeing */
struct Library * HiddAgpBase = NULL; /* TODO: Implement  freeing */
OOP_AttrBase HiddAGPBridgeDeviceAttrBase = 0; /* TODO: Implement  freeing */

struct agp_bridge_data *agp_backend_acquire(void * dev)
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
    info->chipset = SUPPORTED;
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

struct agp_bridge_data * agp_find_bridge(void * dev)
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
    address:    (IPTR)(mem->pages[0]->address),
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

/* jiffies handling */
#include <sys/time.h>

/* jiffies are supposed to be very precise time value. They are implemented
   as rather very unprecise */
unsigned long get_jiffies()
{
    struct timeval tv;
    unsigned long val = 0;
NOT_IMPLEMENTED_STOP
    gettimeofday(&tv, NULL);

    val = tv.tv_sec * 1000000 + tv.tv_usec; /* Yes, overflow */

    /* TODO: Maybe make sure that each call to get_jiffies returns a different value? */

    return val;
}

unsigned int jiffies_to_usecs(const unsigned long j)
{
    return j * (1000 / HZ) /* ms */ * 1000;
}

unsigned long usecs_to_jiffies(unsigned int us) /* this function rounds up the result */
{
    return (us + ((1000 / HZ) * 1000) - 1) / ((1000 / HZ) /* ms */ * 1000);
}

/* Other */
unsigned int hweight32(unsigned int number)
{
    unsigned int result = 0;
    int i = 0;
    
    for (i = 0; i < 32; i++)
    {
        if (number & 0x1) result++;
        number >>= 1;
    }
    
    return result;
}

unsigned int hweight8(unsigned int number)
{
    unsigned int result = 0;
    int i = 0;
    
    for (i = 0; i < 8; i++)
    {
        if (number & 0x1) result++;
        number >>= 1;
    }
    
    return result;
}

bool drm_mode_object_lease_required(uint32_t type)
{
    return false;
}

#include <drm-compat/drm_compat_mem.h>

void *kmalloc(size_t size, gfp_t flags)
{
    if (size == 0)
    {
bug("FIXME: kmalloc: implemented ZERO_SIZE_PTR\n");
        size = 1;
    }

    return HIDDNouveauAlloc(size);
}