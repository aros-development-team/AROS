#define MAX_DEVICEBUSES 2

/*
 * A single PCI device is shared between two buses.
 * The driver is designed as unloadable, so our bus objects can be
 * destroyed. We need to release the device only when both objects
 * are disposed, so we maintain this structure with reference
 * counter.
 * It raises a question if our PCI subsystem needs to support this.
 * However, we'll wait until more use cases pop up.
 */
struct PCIDeviceRef
{
    OOP_Object *ref_Device;
    ULONG       ref_Count;
};

struct ata_ProbedBus
{
    struct Node         atapb_Node;
	OOP_Object          *atapb_Parent;
    struct PCIDeviceRef *atapb_Device;
    UWORD               atapb_Vendor;
    UWORD               atapb_Product;
    UBYTE               atapb_BusNo;
    IPTR                atapb_IOBase;
    IPTR                atapb_IOAlt;
    IPTR                atapb_INTLine;
    IPTR                atapb_DMABase;
};

/* These values are used also for ln_Type */
#define ATABUSNODEPRI_PROBED       50
#define ATABUSNODEPRI_PROBEDLEGACY 100
#define ATABUSNODEPRI_LEGACY       0

struct PCIATABusData
{
    struct ata_ProbedBus *bus;
    OOP_Object           *pciDriver;
    APTR                  dmaBuf;
    void                (*ata_HandleIRQ)(UBYTE, APTR);
    APTR                  irqData;
    APTR                  irqHandle;
};

struct atapciBase
{
    struct Library  lib;

    struct MinList  probedbuses;
    ULONG           ata__buscount;
    ULONG           legacycount;

    OOP_Class      *ataClass;
    OOP_Class      *busClass;

    OOP_Object	   *storageRoot;

#if defined(__OOP_NOATTRBASES__)
    OOP_AttrBase    PCIDeviceAttrBase;
    OOP_AttrBase    PCIDriverAttrBase;
    OOP_AttrBase    hiddAttrBase;
    OOP_AttrBase    busAttrBase;
    OOP_AttrBase    ATABusAttrBase;
    OOP_AttrBase    hwAttrBase;
#endif
#if defined(__OOP_NOMETHODBASES__)
    OOP_MethodID    PCIMethodBase;
    OOP_MethodID    PCIDeviceMethodBase;
    OOP_MethodID    PCIDriverMethodBase;
    OOP_MethodID    HWMethodBase;
    OOP_MethodID    HiddSCMethodBase;
#endif

    APTR            cs_KernelBase;
    struct Library *cs_OOPBase;
    struct Library *cs_UtilityBase;
};

#if defined(__OOP_NOATTRBASES__)
/* Attribute Bases ... */
#undef HiddPCIDeviceAttrBase
#undef HiddPCIDriverAttrBase
#undef HiddAttrBase
#undef HiddBusAB
#undef HiddATABusAB
#undef HWAttrBase
#define HiddPCIDeviceAttrBase (base->PCIDeviceAttrBase)
#define HiddPCIDriverAttrBase (base->PCIDriverAttrBase)
#define HiddAttrBase          (base->hiddAttrBase)
#define HiddBusAB             (base->busAttrBase)
#define HiddATABusAB          (base->ATABusAttrBase)
#define HWAttrBase            (base->hwAttrBase)
#endif

#if defined(__OOP_NOMETHODBASES__)
/* Method Bases ... */
#undef HiddPCIBase
#undef HiddPCIDeviceBase
#undef HiddPCIDriverBase
#undef HWBase
#undef HiddStorageControllerBase
#define HiddPCIBase                     (base->PCIMethodBase)
#define HiddPCIDeviceBase               (base->PCIDeviceMethodBase)
#define HiddPCIDriverBase               (base->PCIDriverMethodBase)
#define HWBase                          (base->HWMethodBase)
#define HiddStorageControllerBase       (base->HiddSCMethodBase)
#endif

/* Libraries ... */
#define KernelBase                      (base->cs_KernelBase)
#define OOPBase                         (base->cs_OOPBase)
#define UtilityBase                     (base->cs_UtilityBase)

void DeviceFree(struct PCIDeviceRef *ref, struct atapciBase *base);
void DeviceUnref(struct PCIDeviceRef *ref, struct atapciBase *base);
