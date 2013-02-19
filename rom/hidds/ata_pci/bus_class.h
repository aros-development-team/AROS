struct ata_ProbedBus
{
    struct Node atapb_Node;
    OOP_Object *atapb_Device;
    UWORD       atapb_Vendor;
    UWORD       atapb_Product;
    IPTR        atapb_IOBase;
    IPTR        atapb_IOAlt;
    IPTR        atapb_INTLine;
    IPTR        atapb_DMABase;
};

struct ATA_BusData
{
    struct ata_ProbedBus *bus;
    OOP_Object           *pciDriver;
    APTR                  dmaBuf;
};

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
