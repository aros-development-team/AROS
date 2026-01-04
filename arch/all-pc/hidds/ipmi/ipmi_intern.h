#ifndef HWIPMI_INTERN_H
#define HWIPMI_INTERN_H

#include <exec/libraries.h>
#include <dos/bptr.h>
#include <oop/oop.h>

#include <hidd/hidd.h>
#include <hidd/ipmi.h>

struct HWIPMIData
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

struct ipmiclass_staticdata
{
    struct Library              *cs_OOPBase;
    BPTR                        cs_SegList;

    OOP_Class                   *oopclass;

    OOP_AttrBase                hwAB;
    OOP_AttrBase                hiddAB;
    OOP_AttrBase                hiddIPMIAB;
    OOP_MethodID                hwMethodBase;
};

struct HWIPMIIntBase
{
    struct Library              hsi_LibNode;

    struct ipmiclass_staticdata hsi_csd;
};

#define CSD(x) (&((struct HWIPMIIntBase *)x->UserData)->hsi_csd)
#define _csd    CSD(cl)

#define __IHW            (_csd->hwAB)
#define __IHidd          (_csd->hiddAB)
#define __IHidd_IPMI     (_csd->hiddIPMIAB)
#define HWBase           (_csd->hwMethodBase)

#define OOPBase          (_csd->cs_OOPBase)

#endif /* !HWIPMI_INTERN_H */
