#ifndef HIDDPOWER_INTERN_H
#define HIDDPOWER_INTERN_H

#include <exec/semaphores.h>
#include <exec/libraries.h>
#include <dos/bptr.h>
#include <oop/oop.h>

#include <hidd/power.h>

struct HIDDPowerData
{
    ULONG power_Flags;                                    /* Private power flags */
    ULONG power_MaxUnit;                                  /* Maximum supported units byt the power
                                                            Initial value is the max supported by the base
                                                            power class */
    APTR  power_IRQHandler;                               /* Pointer to IRQ handler function */
    APTR  power_IRQData;                                  /* Caller-supplied data to pass to IRQ handler */
};

// Keep empty power
#define BFB_KeepEmpty       0
#define BFF_KeepEmpty       (1 << BFB_KeepEmpty)

struct class_static_data
{
    struct Library              *cs_OOPBase;
    BPTR                        cs_SegList;

    OOP_Class                   *powerClass;              /* "Power" BaseClass */

    OOP_AttrBase                powerAttrBase;
};

#undef HiddPowerAB
#define HiddPowerAB               (base->powerAttrBase)

/* Library base */

struct HiddPowerIntBase
{
    struct Library              hbi_LibNode;

    struct class_static_data    hbi_csd;
};

#define CSD(x) (&((struct HiddStorageIntBase *)x->UserData)->hbi_csd)

#define OOPBase                         (base->cs_OOPBase)

#endif /* !HIDDPOWER_INTERN_H */
