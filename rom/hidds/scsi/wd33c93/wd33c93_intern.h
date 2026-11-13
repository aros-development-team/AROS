#define MAX_DEVICEBUSES 2

struct wd33c93ProbedBus
{
    struct Node             wdpb_Node;
	OOP_Object              *wdpb_Parent;
    UWORD                   wdpb_Vendor;
    UWORD                   wdpb_Product;
    UBYTE                   wdpb_BusNo;
    IPTR                    wdpb_IOBase;
    IPTR                    wdpb_DMABase;
    IPTR                    wdpb_INTLine;
};

struct SCSIWD33C93BusData
{
    struct wd33c93ProbedBus *bus;
    APTR                    dmaBuf;
    void                    (*ata_HandleIRQ)(UBYTE, APTR);
    APTR                    irqData;
    APTR                    irqHandle;
};

struct scsiwd33c93Base
{
    struct Library          lib;

    struct MinList          probedbuses;
    ULONG                   buscount;

    OOP_Class               *scsiClass;
    OOP_Class               *busClass;

    OOP_Object	            *storageRoot;

#if defined(__OOP_NOATTRBASES__)
    OOP_AttrBase            hiddAttrBase;
    OOP_AttrBase            busAttrBase;
    OOP_AttrBase            SCSIBusAttrBase;
    OOP_AttrBase            hwAttrBase;
#endif
#if defined(__OOP_NOMETHODBASES__)
    OOP_MethodID            HWMethodBase;
    OOP_MethodID            HiddSCMethodBase;
#endif

    APTR                    cs_KernelBase;
    struct Library          *cs_OOPBase;
    struct Library          *cs_UtilityBase;
};

#if defined(__OOP_NOATTRBASES__)
/* Attribute Bases ... */
#undef HiddAttrBase
#undef HiddBusAB
#undef HiddSCSIBusAB
#undef HWAttrBase
#define HiddAttrBase                    (base->hiddAttrBase)
#define HiddBusAB                       (base->busAttrBase)
#define HiddSCSIBusAB                   (base->SCSIBusAttrBase)
#define HWAttrBase                      (base->hwAttrBase)
#endif

#if defined(__OOP_NOMETHODBASES__)
/* Method Bases ... */
#undef HWBase
#undef HiddStorageControllerBase
#define HWBase                          (base->HWMethodBase)
#define HiddStorageControllerBase       (base->HiddSCMethodBase)
#endif

/* Libraries ... */
#define KernelBase                      (base->cs_KernelBase)
#define OOPBase                         (base->cs_OOPBase)
#define UtilityBase                     (base->cs_UtilityBase)
