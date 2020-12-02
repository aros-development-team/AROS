#ifndef _PCI_H
#define _PCI_H

/*
    Copyright © 2004-2020, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/types.h>
#include <exec/libraries.h>
#include <exec/execbase.h>
#include <exec/nodes.h>
#include <exec/lists.h>
#include <exec/semaphores.h>
#include <dos/bptr.h>

#include <aros/libcall.h>
#include <aros/asmcall.h>

#include <oop/oop.h>

#include <exec/execbase.h>

#include LC_LIBDEFS_FILE

/* Private data and structures unavailable outside the pci base classes */

struct DrvInstData {
    BOOL                        DirectBus;
    IPTR                        IOBase;
};

typedef struct DeviceData {
    struct MinNode              node;           /* Accessed directly by PCI subsystem class */
    OOP_Object                  *driver;
    UBYTE                       bus,dev,sub;
    UBYTE                       isBridge;
    UBYTE                       secbus;
    UWORD                       VendorID;
    UWORD                       ProductID;
    UBYTE                       RevisionID;
    UBYTE                       Interface;
    UBYTE                       SubClass;
    UBYTE                       Class;
    UWORD                       SubsysVID;
    UWORD                       SubsystemID;
    UBYTE                       INTLine;
    UBYTE                       IRQLine;
    UBYTE                       HeaderType;
    struct {
        IPTR                    addr;
        IPTR                    size;
    } BaseReg[6];
    ULONG                       RomBase;
    ULONG                       RomSize;

    STRPTR                      strClass;
    STRPTR                      strSubClass;
    STRPTR                      strInterface;

    struct SignalSemaphore      ownerLock;

    /* If Extended Configuration exists, the HW driver fills this with a
       non-NULL value */
    IPTR                        extendedconfig;

} tDeviceData;

struct pci_staticdata {
    struct SignalSemaphore      dev_lock;
    struct MinList              devices;        /* List of devices */

    APTR                        kernelBase;
    struct Library              *oopBase;

    OOP_AttrBase                hiddAB;
    OOP_AttrBase                hwAttrBase;
    OOP_AttrBase                hiddPCIAB;
    OOP_AttrBase                hiddPCIDriverAB;
    OOP_AttrBase                hiddPCIDeviceAB;
    OOP_AttrBase                hiddPCIBusAB;
    OOP_MethodID                hiddPCIDriverMB;
    OOP_MethodID                hwMethodBase;

    OOP_Class                   *pciClass;
    OOP_Class                   *pciDeviceClass;
    OOP_Class                   *pciDriverClass;
    OOP_Object                  *pciObject;

    BPTR                        segList;
};

struct pcibase {
    struct Library              LibNode;
    struct pci_staticdata       psd;
};

OOP_Class *init_pcideviceclass(struct pci_staticdata *);
void free_pcideviceclass(struct pci_staticdata *, OOP_Class *cl);

#define BASE(lib)               ((struct pcibase*)(lib))

#define PSD(cl)                 (&BASE(cl->UserData)->psd)

/*
    There are no static AttrBases in this class. Therefore it might be placed
    directly in ROM without any harm
*/
#undef HiddAttrBase
#undef HiddPCIAttrBase
#undef HiddPCIDeviceAttrBase
#undef HiddPCIDriverAttrBase
#undef HWAttrBase
#undef HiddPCIDriverBase
#undef HWBase

#define HiddAttrBase            (PSD(cl)->hiddAB)
#define HiddPCIAttrBase         (PSD(cl)->hiddPCIAB)
#define HiddPCIDeviceAttrBase   (PSD(cl)->hiddPCIDeviceAB)
#define HiddPCIDriverAttrBase   (PSD(cl)->hiddPCIDriverAB)
#define HWAttrBase              (PSD(cl)->hwAttrBase)
#define HiddPCIDriverBase       (PSD(cl)->hiddPCIDriverMB)
#define HWBase                  (PSD(cl)->hwMethodBase)

#define KernelBase              (PSD(cl)->kernelBase)
#define OOPBase                 (PSD(cl)->oopBase)

#endif /* _PCI_H */

