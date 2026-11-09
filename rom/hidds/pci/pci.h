#ifndef _PCI_H
#define _PCI_H

/*
    Copyright © 2004-2026, The AROS Development Team. All rights reserved.
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
    ULONG                       segment;        /* Accessed directly by PCI subsystem class */
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
    struct MinList              devices;        /* List of devices (their tDeviceData) */
    ULONG                       segments;       /* Number of hardware drivers created so far */

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

/*
 * The device list links the instance data of the device objects.
 * These convert between an entry of the list and its object.
 */
#define PCIDEV_DATA(psd, obj)   ((tDeviceData *)OOP_INST_DATA((psd)->pciDeviceClass, obj))
#define PCIDEV_OBJECT(psd, dev) ((OOP_Object *)((UBYTE *)(dev) - (psd)->pciDeviceClass->InstOffset))

#define ForeachDevice(psd, dev, obj)                                        \
    for (dev = (tDeviceData *)(psd)->devices.mlh_Head;                      \
         dev->node.mln_Succ && ((obj = PCIDEV_OBJECT(psd, dev)), TRUE);     \
         dev = (tDeviceData *)dev->node.mln_Succ)

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

