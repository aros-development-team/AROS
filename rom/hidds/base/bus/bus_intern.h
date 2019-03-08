#ifndef HIDDBUS_INTERN_H
#define HIDDBUS_INTERN_H

#include <exec/semaphores.h>
#include <exec/libraries.h>
#include <dos/bptr.h>
#include <oop/oop.h>

#include <hidd/bus.h>

struct HIDDBusData
{
};

struct class_static_data
{
    struct Library              *cs_OOPBase;
    BPTR                        cs_SegList;

    OOP_Class                   *busClass;		/* "Bus" BaseClass */
};

/* Library base */

struct HiddBusIntBase
{
    struct Library              hbi_LibNode;

    struct class_static_data    hbi_csd;
};

#define CSD(x) (&((struct HiddStorageIntBase *)x->UserData)->hbi_csd)

#define OOPBase                         (CSD(cl)->cs_OOPBase)

#endif /* !HIDDBUS_INTERN_H */
