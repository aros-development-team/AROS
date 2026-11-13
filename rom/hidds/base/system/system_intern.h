#ifndef HIDDSYSTEM_INTERN_H
#define HIDDSYSTEM_INTERN_H

#include <exec/libraries.h>
#include <dos/bptr.h>
#include <oop/oop.h>
#include <hidd/hidd.h>
#include <hidd/system.h>

struct HIDDSystemData
{
    ULONG   sd_Private;
};

struct class_static_data
{
    struct Library	        *cs_OOPBase;
    BPTR                        cs_SegList;

    OOP_Class                   *oopclass;
    OOP_Object                  *instance;

    OOP_AttrBase                hwAttrBase;

    OOP_MethodID                hwMethodBase;
};

/* Library base */

struct HiddSystemIntBase
{
    struct Library              hsi_LibNode;

    struct class_static_data    hsi_csd;
};

#define CSD(x) (&((struct HiddSystemIntBase *)x->UserData)->hsi_csd)

#undef HWAttrBase
#define HWAttrBase 	                (CSD(cl)->hwAttrBase)

#undef HWBase
#define HWBase                          (CSD(cl)->hwMethodBase)

#endif
