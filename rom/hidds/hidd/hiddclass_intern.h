#ifndef HIDD_CLASS_INTERN_H
#define HIDD_CLASS_INTERN_H

/* Include files */

#ifndef EXEC_LIBRARIES_H
#   include <exec/libraries.h>
#endif
#ifndef EXEC_SEMAPHORES_H
#   include <exec/semaphores.h>
#endif
#ifndef OOP_OOP_H
#   include <oop/oop.h>
#endif
#ifndef HIDD_HIDD_H
#   include <hidd/hidd.h>
#endif
#ifndef DOS_DOS_H
#   include <dos/dos.h>
#endif


struct HIDDData
{
    UWORD       hd_Type;
    UWORD       hd_SubType;
    ULONG       hd_Producer;
    ULONG	hd_Product;
    STRPTR      hd_Name;
    STRPTR      hd_HWName;
    STRPTR	hd_ProducerName;
    BOOL        hd_Active;
    UWORD       hd_Locking;
    ULONG       hd_Status;
    ULONG       hd_ErrorCode;
};

struct DriverNode
{
    struct MinNode node;
    OOP_Object     *driverObject;  /* Driver object */
};

struct HWData
{
    const char            *name;
    struct MinList         drivers;
    struct SignalSemaphore driver_lock;
};

/* Static Data for the hiddclass. */
struct class_static_data
{
    OOP_AttrBase                hiddAttrBase;  // keep lower case so it does not clash with define.
    OOP_AttrBase                hwAttrBase;
    OOP_MethodID                hwMethodBase;

    OOP_Class                   *hiddclass;
    OOP_Class                   *hwclass;
    OOP_Class                   *rootclass;

    OOP_Object                  *hwroot;
    APTR                        MemPool;

    struct Library              *cs_OOPBase;
    struct Library              *cs_UtilityBase;
};


/* Library base */

struct IntHIDDClassBase
{
    struct Library            hd_LibNode;

    struct class_static_data  hd_csd;
};


#define CSD(cl) (&((struct IntHIDDClassBase *)cl->UserData)->hd_csd)

#undef HiddAttrBase
#undef HWAttrBase
#undef HWBase
#define HiddAttrBase (CSD(cl)->hiddAttrBase)
#define HWAttrBase   (CSD(cl)->hwAttrBase)
#define HWBase       (CSD(cl)->hwMethodBase)

#endif /* HIDD_CLASS_INTERN_H */
