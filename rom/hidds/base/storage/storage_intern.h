#ifndef HIDDSTORAGE_INTERN_H
#define HIDDSTORAGE_INTERN_H

#include <exec/semaphores.h>
#include <exec/libraries.h>
#include <dos/bptr.h>
#include <oop/oop.h>
#include <hidd/hidd.h>
#include <hidd/storage.h>

struct HIDDStorageData
{
};

struct HIDDStorageControllerData
{
    struct SignalSemaphore	scd_BusLock;
    struct MinList         	scd_Buses;
};

struct BusNode
{
    struct MinNode node;
    OOP_Object     *busObject;  /* Bus object */
};

struct class_static_data
{
    struct Library              *cs_OOPBase;
    BPTR                        cs_SegList;
    APTR                        cs_MemPool;

    struct List                 cs_IDs;

    OOP_Class                   *storageClass;          /* Storage Subsystem Class  */
    OOP_Class                   *controllerClass;	/* Storage "Controller" BaseClass */
    OOP_Class                   *busClass;		/* Storage "Bus" BaseClass */
    OOP_Class                   *unitClass;		/* Storage "Unit" BaseClass */

    OOP_Object                  *instance;

    OOP_AttrBase                hwAttrBase;

    OOP_MethodID                hwMethodBase;
    OOP_MethodID                hiddSCMethodBase;
};

/* Library base */

struct HiddStorageIntBase
{
    struct Library              hsi_LibNode;

    struct class_static_data    hsi_csd;
};

#define CSD(x) (&((struct HiddStorageIntBase *)x->UserData)->hsi_csd)

#undef HWAttrBase
#define HWAttrBase 	                (CSD(cl)->hwAttrBase)

#undef HWBase
#define HWBase                          (CSD(cl)->hwMethodBase)
#undef HiddStorageControllerBase
#define HiddStorageControllerBase       (CSD(cl)->hiddSCMethodBase)

#define OOPBase                         (CSD(cl)->cs_OOPBase)

/* ID Namespace structures */

struct Storage_IDFamily
{
    struct Node                                 SIDF_Node;                      /* ln_Name = IDBase (e.g "CD") */
    struct List                                 SIDF_IDs;
};

struct Storage_IDNode
{
    struct Node                                 SIDN_Node;                      /* ln_Name = ID (e.g. "CD0") */
};

#endif
