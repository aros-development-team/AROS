#ifndef HIDDSTORAGE_INTERN_H
#define HIDDSTORAGE_INTERN_H

#include <exec/libraries.h>
#include <dos/bptr.h>
#include <oop/oop.h>
#include <hidd/hidd.h>
#include <hidd/storage.h>

struct HIDDStorageData
{
};

struct class_static_data
{
    struct Library	        *cs_OOPBase;
    BPTR                        cs_SegList;

    OOP_Class                   *oopclass;
    OOP_Object                  *instance;
    OOP_AttrBase                hwAttrBase;
};

/* Library base */

struct HiddStorageIntBase
{
    struct Library              hsi_LibNode;

    struct class_static_data    hsi_csd;
};

#define CSD(x) (&((struct HiddStorageIntBase *)x->UserData)->hsi_csd)

#define __IHW 	                (CSD(cl)->hwAttrBase)

#endif
