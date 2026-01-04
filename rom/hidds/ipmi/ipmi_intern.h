#ifndef HIDDIPMI_INTERN_H
#define HIDDIPMI_INTERN_H

#include <exec/libraries.h>
#include <dos/bptr.h>
#include <oop/oop.h>

#include <hidd/ipmi.h>

struct HIDDIPMIData
{
    ULONG ipmi_InterfaceType;
    ULONG ipmi_SpecVersionMajor;
    ULONG ipmi_SpecVersionMinor;
    ULONG ipmi_I2CSlaveAddress;
    ULONG ipmi_NVStorageAddress;
    IPTR  ipmi_BaseAddress;
    ULONG ipmi_BaseAddressModifier;
    ULONG ipmi_AddressSpace;
    ULONG ipmi_RegisterSpacing;
    ULONG ipmi_InterruptNumber;
};

struct class_static_data
{
    struct Library              *cs_OOPBase;
    BPTR                        cs_SegList;

    OOP_Class                   *ipmiClass;

    OOP_AttrBase                ipmiAttrBase;
};

#undef HiddIPMIAB
#define HiddIPMIAB               (base->ipmiAttrBase)

struct HiddIPMIIntBase
{
    struct Library              hbi_LibNode;

    struct class_static_data    hbi_csd;
};

#define CSD(x) (&((struct HiddIPMIIntBase *)x->UserData)->hbi_csd)

#define OOPBase                         (base->cs_OOPBase)

#endif /* !HIDDIPMI_INTERN_H */
