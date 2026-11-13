#ifndef ATA_PCI_INTERN_H
#define ATA_PCI_INTERN_H

#include <exec/types.h>
#include <hidd/hidd.h>
#include <hidd/bus.h>
#include <hidd/pci.h>
#include <hidd/storage.h>
#include <oop/oop.h>

struct atapci_static_data
{
    OOP_Class                   *ataClass;
    OOP_Class                   *ataPCIClass;
    OOP_Class                   *ataPCIBusClass;

    OOP_Object	                *storageRoot;

#if defined(__OOP_NOATTRBASES__)
    OOP_AttrBase                PCIDeviceAttrBase;
    OOP_AttrBase                PCIDriverAttrBase;
    OOP_AttrBase                hiddAttrBase;
    OOP_AttrBase                busAttrBase;
    OOP_AttrBase                ATABusAttrBase;
    OOP_AttrBase                hwAttrBase;
#endif
#if defined(__OOP_NOMETHODBASES__)
    OOP_MethodID                PCIMethodBase;
    OOP_MethodID                PCIDeviceMethodBase;
    OOP_MethodID                PCIDriverMethodBase;
    OOP_MethodID                HWMethodBase;
    OOP_MethodID                HiddSCMethodBase;
#endif

    APTR                        cs_KernelBase;
    struct Library              *cs_OOPBase;
    struct Library              *cs_UtilityBase;
};

struct atapciBase
{
    struct Library              lib;

    struct MinList              probedbuses;
    ULONG                       ata__buscount;
    ULONG                       legacycount;

    struct atapci_static_data   psd;
};

#if defined(__OOP_NOATTRBASES__)
/* Attribute Bases ... */
#undef HiddPCIDeviceAttrBase
#undef HiddPCIDriverAttrBase
#undef HiddAttrBase
#undef HiddBusAB
#undef HiddATABusAB
#undef HWAttrBase
#define HiddPCIDeviceAttrBase           (base->psd.PCIDeviceAttrBase)
#define HiddPCIDriverAttrBase           (base->psd.PCIDriverAttrBase)
#define HiddAttrBase                    (base->psd.hiddAttrBase)
#define HiddBusAB                       (base->psd.busAttrBase)
#define HiddATABusAB                    (base->psd.ATABusAttrBase)
#define HWAttrBase                      (base->psd.hwAttrBase)
#endif

#if defined(__OOP_NOMETHODBASES__)
/* Method Bases ... */
#undef HiddPCIBase
#undef HiddPCIDeviceBase
#undef HiddPCIDriverBase
#undef HWBase
#undef HiddStorageControllerBase
#define HiddPCIBase                     (base->psd.PCIMethodBase)
#define HiddPCIDeviceBase               (base->psd.PCIDeviceMethodBase)
#define HiddPCIDriverBase               (base->psd.PCIDriverMethodBase)
#define HWBase                          (base->psd.HWMethodBase)
#define HiddStorageControllerBase       (base->psd.HiddSCMethodBase)
#endif

/* Libraries ... */
#define KernelBase                      (base->psd.cs_KernelBase)
#define OOPBase                         (base->psd.cs_OOPBase)
#define UtilityBase                     (base->psd.cs_UtilityBase)

#endif
