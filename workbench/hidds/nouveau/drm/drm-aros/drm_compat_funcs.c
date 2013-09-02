/*
    Copyright 2009, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "drm_compat_funcs.h"
#include "drm_aros_config.h"

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

/* Bit operations */
void clear_bit(int nr, volatile void * addr)
{
    unsigned long mask = 1 << nr;
    
    *(unsigned long*)addr &= ~mask;
}

void set_bit(int nr, volatile void *addr)
{
    unsigned long mask = 1 << nr;
    
    *(unsigned long*)addr |= mask;
}

int test_bit(int nr, volatile void *addr)
{
    unsigned long mask = 1 << nr;
    
    return (*(unsigned long*)addr) & mask;
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
    timerio.tr_time.tv_micro                    = usecs - (timerio.tr_time.tv_secs * 1000000);

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

struct page * create_page_helper()
{
    struct page * p;
    p = HIDDNouveauAlloc(sizeof(*p));
    p->allocated_buffer = HIDDNouveauAlloc(PAGE_SIZE + PAGE_SIZE - 1);
    p->address = PAGE_ALIGN(p->allocated_buffer);
    return p;
}

/* IDR handling */
int idr_pre_get_internal(struct idr *idp)
{
    if (idp->size <= idp->occupied + idp->last_starting_id)
    {
        /* Create new table */
        ULONG newsize = idp->size ? idp->size * 2 : 128;
        IPTR * newtab = HIDDNouveauAlloc(newsize * sizeof(IPTR));
        
        if (newtab == NULL)
            return 0;
        
        
        if (idp->pointers)
        {
            /* Copy old table into new */
            CopyMem(idp->pointers, newtab, idp->size * sizeof(IPTR));
            
            /* Release old table */
            HIDDNouveauFree(idp->pointers);
        }
        
        idp->pointers = newtab;
        idp->size = newsize;
    }
    
    return 1;
}

int idr_get_new_above(struct idr *idp, void *ptr, int starting_id, int *id)
{
    int i = starting_id;
    idp->last_starting_id = starting_id;

    for(;i < idp->size;i++)
    {
        if (idp->pointers[i] == (IPTR)NULL)
        {
            *id = i;
            idp->pointers[i] = (IPTR)ptr;
            idp->occupied++;
            return 0;
        }
    }
    
    return -EAGAIN;
}

void *idr_find(struct idr *idp, int id)
{
    if ((id < idp->size) && (id >= 0))
        return (APTR)idp->pointers[id];
    else
        return NULL;
}

void idr_remove(struct idr *idp, int id)
{
    if ((id < idp->size) && (id >= 0))
    {
        idp->pointers[id] = (IPTR)NULL;
        idp->occupied--;
    }
}

void idr_init(struct idr *idp)
{
    idp->size = 0;
    idp->pointers = NULL;
    idp->occupied = 0;
    idp->last_starting_id = 0;
}

/* PCI handling */
#include "drm_aros.h"
#include <aros/libcall.h>
#include <proto/oop.h>
#include <hidd/pci.h>
#include <hidd/hidd.h>

void *ioremap(resource_size_t offset, unsigned long size)
{
    if (pciDriver)
    {
        struct pHidd_PCIDriver_MapPCI mappci,*msg = &mappci;
        mappci.mID = OOP_GetMethodID(IID_Hidd_PCIDriver, moHidd_PCIDriver_MapPCI);
        mappci.PCIAddress = (APTR)offset;
        mappci.Length = size;
        return (APTR)OOP_DoMethod(pciDriver, (OOP_Msg)msg);
    }
    else
    {
        bug("BUG: ioremap used without acquiring pciDriver\n");
        return NULL;
    }
}

void iounmap(void * addr)
{
    if (pciDriver)
    {
        struct pHidd_PCIDriver_UnmapPCI unmappci,*msg=&unmappci;

        unmappci.mID = OOP_GetMethodID(IID_Hidd_PCIDriver, moHidd_PCIDriver_UnmapPCI);
        unmappci.CPUAddress = addr;
        unmappci.Length = 0; /* This works on i386 but may create problems on other archs */

        OOP_DoMethod(pciDriver, (OOP_Msg)msg);
    }
}

resource_size_t pci_resource_start(struct pci_dev * pdev, unsigned int resource)
{
    APTR start = (APTR)NULL;
    switch(resource)
    {
        case(0): OOP_GetAttr((OOP_Object *)pdev->oopdev, aHidd_PCIDevice_Base0, (APTR)&start); break;
        case(1): OOP_GetAttr((OOP_Object *)pdev->oopdev, aHidd_PCIDevice_Base1, (APTR)&start); break;
        case(2): OOP_GetAttr((OOP_Object *)pdev->oopdev, aHidd_PCIDevice_Base2, (APTR)&start); break;
        case(3): OOP_GetAttr((OOP_Object *)pdev->oopdev, aHidd_PCIDevice_Base3, (APTR)&start); break;
        case(4): OOP_GetAttr((OOP_Object *)pdev->oopdev, aHidd_PCIDevice_Base4, (APTR)&start); break;
        case(5): OOP_GetAttr((OOP_Object *)pdev->oopdev, aHidd_PCIDevice_Base5, (APTR)&start); break;
        default: bug("ResourceID %d not supported\n", resource);
    }
    
    return (resource_size_t)start;
}

unsigned long pci_resource_len(struct pci_dev * pdev, unsigned int resource)
{
    IPTR len = (IPTR)0;
    
    if (pci_resource_start(pdev, resource) != 0)
    {
        /* 
         * FIXME:
         * The length reading is only correct when the resource actually exists.
         * pci.hidd can however return a non 0 lenght for a resource that does
         * not exsist. Possible fix in pci.hidd needed
         */
        
        switch(resource)
        {
            case(0): OOP_GetAttr((OOP_Object *)pdev->oopdev, aHidd_PCIDevice_Size0, (APTR)&len); break;
            case(1): OOP_GetAttr((OOP_Object *)pdev->oopdev, aHidd_PCIDevice_Size1, (APTR)&len); break;
            case(2): OOP_GetAttr((OOP_Object *)pdev->oopdev, aHidd_PCIDevice_Size2, (APTR)&len); break;
            case(3): OOP_GetAttr((OOP_Object *)pdev->oopdev, aHidd_PCIDevice_Size3, (APTR)&len); break;
            case(4): OOP_GetAttr((OOP_Object *)pdev->oopdev, aHidd_PCIDevice_Size4, (APTR)&len); break;
            case(5): OOP_GetAttr((OOP_Object *)pdev->oopdev, aHidd_PCIDevice_Size5, (APTR)&len); break;
            default: bug("ResourceID %d not supported\n", resource);
        }
    }
    
    return len;
}

struct GetBusSlotEnumeratorData
{
    IPTR Bus;
    IPTR Dev;
    IPTR Sub;
    OOP_Object ** pciDevice;
};

AROS_UFH3(void, GetBusSlotEnumerator,
    AROS_UFHA(struct Hook *, hook, A0),
    AROS_UFHA(OOP_Object *, pciDevice, A2),
    AROS_UFHA(APTR, message, A1))
{
    AROS_USERFUNC_INIT
    
    struct GetBusSlotEnumeratorData * data = hook->h_Data;
    
    IPTR Bus;
    IPTR Dev;
    IPTR Sub;    

    if (*data->pciDevice)
        return; /* Already found, should not happen */
    
    /* Get the Device's properties */
    OOP_GetAttr(pciDevice, aHidd_PCIDevice_Bus, &Bus);
    OOP_GetAttr(pciDevice, aHidd_PCIDevice_Dev, &Dev);
    OOP_GetAttr(pciDevice, aHidd_PCIDevice_Sub, &Sub);

    if (data->Bus == Bus &&
        data->Dev == Dev &&
        data->Sub == Sub)
    {
        (*data->pciDevice) = pciDevice;
    }
    
    AROS_USERFUNC_EXIT
}   
void * pci_get_bus_and_slot(unsigned int bus, unsigned int dev, unsigned int fun)
{
    OOP_Object * pciDevice = NULL;

    if (pciBus)
    {
        struct GetBusSlotEnumeratorData data = {
        Bus: bus,
        Dev: dev,
        Sub: fun,
        pciDevice: &pciDevice,
        };
        
        struct Hook FindHook = {
        h_Entry:    (IPTR (*)())GetBusSlotEnumerator,
        h_Data:     &data,
        };

        struct TagItem Requirements[] = {
        { TAG_DONE,             0UL }
        };
    
        struct pHidd_PCI_EnumDevices enummsg = {
        mID:        OOP_GetMethodID(IID_Hidd_PCI, moHidd_PCI_EnumDevices),
        callback:   &FindHook,
        requirements:   (struct TagItem*)&Requirements,
        }, *msg = &enummsg;
        
        OOP_DoMethod(pciBus, (OOP_Msg)msg);
    }
    
    return pciDevice;
}

int pci_read_config_word(struct pci_dev * pdev, int where, u16 *val)
{
    struct pHidd_PCIDevice_ReadConfigWord rcwmsg = {
    mID: OOP_GetMethodID(IID_Hidd_PCIDevice, moHidd_PCIDevice_ReadConfigWord),
    reg: (UBYTE)where,
    }, *msg = &rcwmsg;
    
    *val = (UWORD)OOP_DoMethod((OOP_Object*)pdev->oopdev, (OOP_Msg)msg);
    D(bug("pci_read_config_word: %d -> %d\n", where, *val));
    
    return 0;
}

int pci_read_config_dword(struct pci_dev * pdev, int where, u32 *val)
{
    struct pHidd_PCIDevice_ReadConfigLong rclmsg = {
    mID: OOP_GetMethodID(IID_Hidd_PCIDevice, moHidd_PCIDevice_ReadConfigLong),
    reg: (UBYTE)where,
    }, *msg = &rclmsg;
    
    *val = (ULONG)OOP_DoMethod((OOP_Object*)pdev->oopdev, (OOP_Msg)msg);
    D(bug("pci_read_config_dword: %d -> %d\n", where, *val));
    
    return 0;
}

int pci_write_config_dword(struct pci_dev * pdev, int where, u32 val)
{
    struct pHidd_PCIDevice_WriteConfigLong wclmsg = {
    mID: OOP_GetMethodID(IID_Hidd_PCIDevice, moHidd_PCIDevice_ReadConfigLong),
    reg: (UBYTE)where,
    val: val,
    }, *msg = &wclmsg;
    
    OOP_DoMethod((OOP_Object*)pdev->oopdev, (OOP_Msg)msg);
    D(bug("pci_write_config_dword: %d -> %d\n", where, val));
    
    return 0;
}

int pci_is_pcie(struct pci_dev * pdev)
{
    IPTR PCIECap;
    OOP_GetAttr((OOP_Object *)pdev->oopdev, aHidd_PCIDevice_CapabilityPCIE, (APTR)&PCIECap);
    return PCIECap;
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

    gettimeofday(&tv, NULL);

    val = tv.tv_sec * 1000000 + tv.tv_usec; /* Yes, overflow */

    /* TODO: Maybe make sure that each call to get_jiffies returns a different value? */

    return val;
}

/* I2C handling */
#include <hidd/i2c.h>

OOP_AttrBase HiddI2CDeviceAttrBase = 0; /* TODO: Implement  freeing */

int i2c_transfer(struct i2c_adapter *adap, struct i2c_msg *msgs, int num)
{
    /* FIXME: This function is not generic. It has hardcoded cases that are present in nouveau */
    if (adap->i2cdriver == (IPTR)0)
    {
        bug("ERROR: i2c_transfer called without driver present\n");
        return 0;
    }
    
    if (HiddI2CDeviceAttrBase == 0)
        HiddI2CDeviceAttrBase = OOP_ObtainAttrBase((STRPTR)IID_Hidd_I2CDevice);

    if (HiddI2CDeviceAttrBase == 0)
    {
        bug("ERROR: i2c_trasfer not able to obtain HiddI2CDeviceAttrBase\n");
        return 0;
    }

    /* Go through supported cases */
    if ((num == 2) && (msgs[0].addr == 0x50) && (msgs[1].addr == 0x50) && (msgs[0].len == 1) && (msgs[1].len == 1))
    {
        /* This is probing for DDC */
        D(bug("i2c_transfer - probing for DDC\n"));
        if (HIDD_I2C_ProbeAddress((OOP_Object *)adap->i2cdriver, 0xa0)) /* AROS has shifted addresses (<< 1) */
            return 2;
        else
            return 0;
    }
    else if ((num == 2) && (msgs[0].addr == 0x75) && (msgs[1].addr == 0x75) && (msgs[0].len == 1) && (msgs[1].len == 1))
    {
        /* This is probing for some hardware related to TV output on nv04 */
        D(bug("i2c_transfer - probing for some hardware related to TV output on nv04\n"));
        if (HIDD_I2C_ProbeAddress((OOP_Object *)adap->i2cdriver, 0xea)) /* AROS has shifted addresses (<< 1) */
            return 2;
        else
            return 0;
    }
    else if ((num == 2) && (msgs[0].addr == 0x50) && (msgs[1].addr == 0x50) && (msgs[0].len == 1) && (msgs[1].len != 1))
    {
        /* This is reading data from DDC */
        struct pHidd_I2CDevice_WriteRead msg;
        BOOL result = FALSE;
        
        struct TagItem attrs[] = 
        {
            { aHidd_I2CDevice_Driver,   (IPTR)adap->i2cdriver   },
            { aHidd_I2CDevice_Address,  0xa0                    },
            { aHidd_I2CDevice_Name,     (IPTR)"Read DDC"        },
            { TAG_DONE, 0UL }
        };

        D(bug("i2c_transfer - reading from DDC\n"));

        OOP_Object *obj = OOP_NewObject(NULL, CLID_Hidd_I2CDevice, attrs);
        
        msg.mID = OOP_GetMethodID((STRPTR)IID_Hidd_I2CDevice, moHidd_I2CDevice_WriteRead);
        msg.readBuffer = msgs[1].buf;
        msg.readLength = msgs[1].len;
        msg.writeBuffer = msgs[0].buf;
        msg.writeLength = msgs[0].len;

        result = OOP_DoMethod(obj, &msg.mID);
        
        OOP_DisposeObject(obj);
        
        if (result)
            return 2;
        else
            return 0;
    }
    else if ((num == 2) && (msgs[0].addr == 0x54) && (msgs[1].addr == 0x54) && (msgs[0].len == 1) && (msgs[1].len == 1))
    {
        /* This is reading data from register 0x54 */
        struct pHidd_I2CDevice_WriteRead msg;
        BOOL result = FALSE;
        
        struct TagItem attrs[] = 
        {
            { aHidd_I2CDevice_Driver,   (IPTR)adap->i2cdriver   },
            { aHidd_I2CDevice_Address,  0xa8                    },
            { aHidd_I2CDevice_Name,     (IPTR)"Read Register"   },
            { TAG_DONE, 0UL }
        };

        D(bug("i2c_transfer - reading from register 0x54\n"));

        OOP_Object *obj = OOP_NewObject(NULL, CLID_Hidd_I2CDevice, attrs);
        
        msg.mID = OOP_GetMethodID((STRPTR)IID_Hidd_I2CDevice, moHidd_I2CDevice_WriteRead);
        msg.readBuffer = msgs[1].buf;
        msg.readLength = msgs[1].len;
        msg.writeBuffer = msgs[0].buf;
        msg.writeLength = msgs[0].len;

        result = OOP_DoMethod(obj, &msg.mID);
        
        OOP_DisposeObject(obj);
        
        if (result)
            return 2;
        else
            return 0;
    }
    else
    {
        /* Not supported case */
        bug("i2c_transfer case not supported: num = %d\n", num);
    }
    
    /* Failure */
    return 0;
}

int i2c_del_adapter(struct i2c_adapter * adap)
{
    IMPLEMENT("\n");
    return 0;
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
