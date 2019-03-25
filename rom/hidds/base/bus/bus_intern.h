#ifndef HIDDBUS_INTERN_H
#define HIDDBUS_INTERN_H

#include <exec/semaphores.h>
#include <exec/libraries.h>
#include <dos/bptr.h>
#include <oop/oop.h>

#include <hidd/bus.h>

struct HIDDBusData
{
    ULONG bus_Flags;                                    /* Private bus flags */
    ULONG bus_MaxUnit;                                  /* Maximum supported units byt the bus
                                                            Initial value is the max supported by the base
                                                            bus class */
    APTR  bus_IRQHandler;                               /* Pointer to IRQ handler function */
    APTR  bus_IRQData;                                  /* Caller-supplied data to pass to IRQ handler */
};

// Keep empty bus
#define BFB_KeepEmpty       0
#define BFF_KeepEmpty       (1 << BFB_KeepEmpty)

struct class_static_data
{
    struct Library              *cs_OOPBase;
    BPTR                        cs_SegList;

    OOP_Class                   *busClass;              /* "Bus" BaseClass */

    OOP_AttrBase                busAttrBase;
};

#undef HiddBusAB
#define HiddBusAB               (base->busAttrBase)

/* Library base */

struct HiddBusIntBase
{
    struct Library              hbi_LibNode;

    struct class_static_data    hbi_csd;
};

#define CSD(x) (&((struct HiddStorageIntBase *)x->UserData)->hbi_csd)

#define OOPBase                         (base->cs_OOPBase)

#endif /* !HIDDBUS_INTERN_H */
