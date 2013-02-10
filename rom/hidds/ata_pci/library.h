struct ataBase
{
    struct Library  lib;

    struct MinList  probedbuses;
    ULONG	    ata__buscount;
    ULONG           legacycount;

    OOP_Class      *busClass;

    OOP_AttrBase    PCIDeviceAttrBase;
    OOP_AttrBase    PCIDriverAttrBase;
    OOP_MethodID    PCIDriverMethodBase;

    APTR            cs_KernelBase;
    struct Library *cs_OOPBase;
    struct Library *cs_UtilityBase;
};

#undef HiddPCIDeviceAttrBase
#undef HiddPCIDriverAttrBase
#undef PCIDriverBase
#define HiddPCIDeviceAttrBase (base->PCIDeviceAttrBase)
#define HiddPCIDriverAttrBase (base->PCIDriverAttrBase)
#define PCIDriverBase         (base->PCIDriverMethodBase)
#define KernelBase            (base->cs_KernelBase)
#define OOPBase               (base->cs_OOPBase)
#define UtilityBase           (base->cs_UtilityBase)
