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
    struct ExecBase             *sysBase;
    struct Library              *UtilityBase;
    struct Library              *OOPBase;
    OOP_AttrBase                hiddAttrBase;  // keep lower case so it does not clash with define.

    OOP_Class                   *hiddclass;

    struct MinList               hiddList;
    struct SignalSemaphore       listLock;
};


/* Library base */

struct IntHIDDClassBase
{
    struct Library            hd_LibNode;
    BPTR                      hd_SegList;
    struct ExecBase          *hd_SysBase;

    struct class_static_data *hd_csd;
};


#define CSD(x) ((struct class_static_data *)x)

#undef SysBase
#define SysBase (CSD(cl->UserData)->sysBase)

#undef UtilityBase
#define UtilityBase (CSD(cl->UserData)->UtilityBase)

#undef OOPBase
#define OOPBase (CSD(cl->UserData)->OOPBase)

#undef HiddAttrBase
#define HiddAttrBase	(CSD(cl->UserData)->hiddAttrBase)

/* pre declarations */

ULONG init_hiddclass(struct IntHIDDClassBase *lh);
VOID  free_hiddclass(struct IntHIDDClassBase *lh);

#endif /* HIDD_CLASS_INTERN_H */
