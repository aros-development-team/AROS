/*
    Copyright 2009-2026, The AROS Development Team. All rights reserved.
*/

#include <aros/libcall.h>
#include <proto/oop.h>
#include <hidd/pci.h>
#include <hidd/hidd.h>
#include <aros/debug.h>

#include <drm-compat/drm_compat_funcs.h>
#include <drm-compat/drm_compat_pci.h>
#include <drm-aros/drm_aros_pci.h>

/* PCI handling */

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
        struct GetBusSlotEnumeratorData data =
        {
            Bus: bus,
            Dev: dev,
            Sub: fun,
            pciDevice: &pciDevice,
        };
        
        struct Hook FindHook =
        {
            h_Entry:    (IPTR (*)())GetBusSlotEnumerator,
            h_Data:     &data,
        };

        struct TagItem Requirements[] =
        {
            { TAG_DONE,             0UL }
        };
    
        struct pHidd_PCI_EnumDevices enummsg =
        {
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
    struct pHidd_PCIDevice_ReadConfigWord rcwmsg =
    {
        mID: OOP_GetMethodID(IID_Hidd_PCIDevice, moHidd_PCIDevice_ReadConfigWord),
        reg: (UBYTE)where,
    }, *msg = &rcwmsg;
    
    *val = (UWORD)OOP_DoMethod((OOP_Object*)pdev->oopdev, (OOP_Msg)msg);
    D(bug("pci_read_config_word: %d -> %d\n", where, *val));
    
    return 0;
}

int pci_read_config_dword(struct pci_dev * pdev, int where, u32 *val)
{
    struct pHidd_PCIDevice_ReadConfigLong rclmsg =
    {
        mID: OOP_GetMethodID(IID_Hidd_PCIDevice, moHidd_PCIDevice_ReadConfigLong),
        reg: (UBYTE)where,
    }, *msg = &rclmsg;
    
    *val = (ULONG)OOP_DoMethod((OOP_Object*)pdev->oopdev, (OOP_Msg)msg);
    D(bug("pci_read_config_dword: %d -> %d\n", where, *val));
    
    return 0;
}

int pci_write_config_dword(struct pci_dev * pdev, int where, u32 val)
{
    struct pHidd_PCIDevice_WriteConfigLong wclmsg =
    {
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

void pci_disable_device(struct pci_dev *pdev)
{
    NOT_IMPLEMENTED_CONTINUE
}

int pci_enable_device(struct pci_dev *pdev)
{
    NOT_IMPLEMENTED_CONTINUE
    return 0;
}

int pci_set_master(struct pci_dev *pdev)
{
    NOT_IMPLEMENTED_CONTINUE
    return 0;
}

int pci_enable_msi(struct pci_dev *pdev)
{
    NOT_IMPLEMENTED_STOP;
    return 0;
}

int pci_enable_rom(struct pci_dev *pdev)
{
    NOT_IMPLEMENTED_CONTINUE
    return 0;
}
void pci_disable_rom(struct pci_dev *pdev)
{
    NOT_IMPLEMENTED_CONTINUE
}

void *pci_map_rom(struct pci_dev *pdev, size_t *size)
{

    IPTR _return;
    IPTR romsize;

    OOP_GetAttr((OOP_Object *)pdev->oopdev, aHidd_PCIDevice_RomBase, (IPTR *)&_return);
    OOP_GetAttr((OOP_Object *)pdev->oopdev, aHidd_PCIDevice_RomSize, (IPTR *)&romsize);;
    *size = romsize;

    pdev->rom       = _return;
    pdev->romlen    = romsize;

    return (void *)_return;
}

void pci_unmap_rom(struct pci_dev *pdev, void *mem)
{
    /* No implementation needed */
}
