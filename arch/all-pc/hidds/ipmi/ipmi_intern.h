#ifndef HWIPMI_INTERN_H
#define HWIPMI_INTERN_H

#include <exec/libraries.h>
#include <dos/bptr.h>
#include <oop/oop.h>

#include <hidd/hidd.h>
#include <hidd/ipmi.h>

/* Discovery source */
#define HWIPMI_SOURCE_SMBIOS    1
#define HWIPMI_SOURCE_ACPI      2

struct HWIPMIData
{
    ULONG hwipmi_Source;
};

struct ipmiclass_staticdata
{
    struct Library              *cs_OOPBase;
    struct Library              *cs_ACPICABase;
    BPTR                        cs_SegList;

    OOP_Class                   *oopclass;

    OOP_AttrBase                hwAB;
    OOP_AttrBase                hiddAB;
    OOP_AttrBase                hiddIPMIAB;
    OOP_MethodID                hwMethodBase;

    ULONG                       cs_Count;       /* interfaces registered */
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
