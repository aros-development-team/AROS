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
    STRPTR      hd_Name;
    STRPTR      hd_HWName;
    BOOL        hd_Active;
    UWORD       hd_Locking;
    ULONG       hd_Status;
    ULONG       hd_ErrorCode;
};


/* Static Data for the hiddclass. */
struct class_static_data
{
    OOP_AttrBase                hiddAttrBase;  // keep lower case so it does not clash with define.

    OOP_Class                   *hiddclass;

    struct MinList               hiddList;
    struct SignalSemaphore       listLock;
};


/* Library base */

struct IntHIDDClassBase
{
    struct Library            hd_LibNode;

    struct class_static_data  hd_csd;
};


#define CSD(cl) (&((struct IntHIDDClassBase *)cl->UserData)->hd_csd)
#define csd CSD(cl)

#undef HiddAttrBase
#define HiddAttrBase	(csd->hiddAttrBase)

#endif /* HIDD_CLASS_INTERN_H */
