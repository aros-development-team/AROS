
#ifndef BUS_CLASS_H
#define BUS_CLASS_H

struct ata_ProbedBus
{
    struct Node atapb_Node;
    UBYTE *port;
    UBYTE *altport;
    UBYTE *gayleirqbase;
    UBYTE doubler;
    BOOL a4000;
};

struct ATA_BusData
{
    struct ata_ProbedBus *bus;
    void (*ata_HandleIRQ)(UBYTE, APTR);
    APTR irqData;
    struct Interrupt ideint;
    UBYTE *gaylebase;
    UBYTE *gayleirqbase;
    UBYTE *gayleintbase;
    BOOL ideintadded;
};

struct ataBase
{
    struct Library  lib;

    OOP_Class      *GayleBusClass;
    OOP_Class      *FastATABusClass;

    OOP_AttrBase    hiddAttrBase;
    OOP_AttrBase    ATABusAttrBase;
    OOP_MethodID    HWMethodBase;

    struct Library *cs_OOPBase;
    struct Library *cs_UtilityBase;
};

#undef HiddAttrBase
#undef HiddATABusAB
#undef HWBase
#define HiddAttrBase          (base->hiddAttrBase)
#define HiddATABusAB          (base->ATABusAttrBase)
#define HWBase                (base->HWMethodBase)
#define OOPBase               (base->cs_OOPBase)
#define UtilityBase           (base->cs_UtilityBase)

#endif /* !BUS_CLASS_H */
