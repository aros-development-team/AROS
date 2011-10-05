#ifndef _AGP_PRIVATE_H
#define _AGP_PRIVATE_H

/*
    Copyright 2010, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/lists.h>
#include <hidd/agp.h>
#include <exec/semaphores.h>

#include LC_LIBDEFS_FILE

struct HIDDAGPData
{
    struct List classes;    /* List of AGPBridgeDevice classes */
};

struct agpstaticdata 
{
    OOP_Class       *AGPClass;
    OOP_Class       *genericBridgeDeviceClass;
    OOP_Class       *agp3BridgeDeviceClass;
    OOP_Class       *sisAgp3BridgeDeviceClass;
    OOP_Class       *sisBridgeDeviceClass;
    OOP_Class       *viaBridgeDeviceClass;
    OOP_Class       *viaAgp3BridgeDeviceClass;
    OOP_Class       *i8XXBridgeDeviceClass;
    OOP_Class       *i845BridgeDeviceClass;
    OOP_Class       *i7505BridgeDeviceClass;
    OOP_Class       *i915BridgeDeviceClass;
    OOP_Class       *i965BridgeDeviceClass;
    OOP_Class       *g33BridgeDeviceClass;
    
    OOP_AttrBase    hiddAGPBridgeDeviceAB;
    OOP_AttrBase    hiddPCIDeviceAB;

    OOP_Object      *bridgedevice;
    OOP_Object      *pcibus;
};

LIBBASETYPE 
{
    struct Library              LibNode;
    struct agpstaticdata        sd;
};

#define METHOD(base, id, name) \
  base ## __ ## id ## __ ## name (OOP_Class *cl, OOP_Object *o, struct p ## id ## _ ## name *msg)

#define BASE(lib) ((LIBBASETYPEPTR)(lib))

#define SD(cl) (&BASE(cl->UserData)->sd)

/* Non-public methods of AGPBridgeDevice interface*/
enum
{
    moHidd_AGPBridgeDevice_ScanAndDetectDevices = NUM_AGPBRIDGEDEVICE_METHODS,
    moHidd_AGPBridgeDevice_CreateGattTable,
    moHidd_AGPBridgeDevice_FlushGattTable,
};

struct pHidd_AGPBridgeDevice_ScanAndDetectDevices
{
    OOP_MethodID    mID;
};

struct pHidd_AGPBridgeDevice_CreateGattTable
{
    OOP_MethodID    mID;
};

struct pHidd_AGPBridgeDevice_FlushGattTable
{
    OOP_MethodID    mID;
};

/* This is an abstract class. Contains usefull code but is not functional */
#define CLID_Hidd_GenericBridgeDevice   "hidd.agp.genericbridgedevice"

struct PciAgpDevice
{
    struct Node node;
    OOP_Object  *PciDevice;
    UBYTE       AgpCapability;          /* Offset to AGP capability in config */
    UWORD       VendorID;
    UWORD       ProductID;
    UBYTE       Class;
};

struct HIDDGenericBridgeDeviceData
{
    struct SignalSemaphore lock;        /* Lock for device operations */
    struct List         devices;        /* Bridges and AGP devices in system */

    /* Bridge data */  
    struct PciAgpDevice *bridge;        /* Selected AGP bridge */
    ULONG               bridgemode;     /* Mode of AGP bridge */
    IPTR                bridgeaperbase; /* Base address for aperture */
    IPTR                bridgeapersize; /* Size of aperture */
    APTR                gatttablebuffer;/* Buffer for gatt table */
    ULONG               *gatttable;     /* 4096 aligned gatt table */
    APTR                scratchmembuffer;/* Buffer for scratch mem */
    ULONG               *scratchmem;    /* 4096 aligned scratch mem */
    ULONG               memmask;        /* Mask for binded memorory */

    ULONG               state;          /* State of the device */

    /* Video card data */
    struct PciAgpDevice *videocard;     /* Selected AGP card */
};

#define STATE_UNKNOWN       0x00
#define STATE_INITIALIZED   0x01
#define STATE_ENABLED       0x02

/* This is an abstract class. Contains usefull code but is not functional */
#define CLID_Hidd_Agp3BridgeDevice   "hidd.agp.agp3bridgedevice"

struct HIDDAgp3BridgeDeviceData
{
};

/* Supports SiS chipsets compatible with AGP3 */
#define CLID_Hidd_SiSAgp3BridgeDevice   "hidd.agp.sisagp3bridgedevice"

struct HIDDSiSAgp3BridgeDeviceData
{
};

/* Supports SiS chipsets not compatible with AGP3 */
#define CLID_Hidd_SiSBridgeDevice   "hidd.agp.sisbridgedevice"

struct HIDDSiSBridgeDeviceData
{
};

/* Supports VIA chipsets not compatible with AGP3 */
#define CLID_Hidd_VIABridgeDevice   "hidd.agp.viabridgedevice"

struct HIDDVIABridgeDeviceData
{
};

/* Supports VIA chipsets compatible with AGP3 */
#define CLID_Hidd_VIAAgp3BridgeDevice   "hidd.agp.viaagp3bridgedevice"

struct HIDDVIAAgp3BridgeDeviceData
{
};

/* Abstract class with support for most i8XX chipsets */
#define CLID_Hidd_i8XXBridgeDevice   "hidd.agp.i8xxbridgedevice"

struct HIDDi8XXBridgeDeviceData
{
};

/* Supports i845 chipsets */
#define CLID_Hidd_i845BridgeDevice   "hidd.agp.i845bridgedevice"

struct HIDDi845BridgeDeviceData
{
};

/* Supports i7505 chipsets */
#define CLID_Hidd_i7505BridgeDevice   "hidd.agp.i7505bridgedevice"

struct HIDDi7505BridgeDeviceData
{
};

/* Intel IGPs support: i915, i965, g33 */
/* These classes implement the AGPBridgeDevice interface but do not
 * inherit from GenericBridgeDevice */
 
/* FIXME: these classes are not completly implemented and are not tested.
 * They are also not registered with the AGP class. */
 
#define CLID_Hidd_i915BridgeDevice   "hidd.agp.i915bridgedevice"

struct HIDDi915BridgeDeviceData
{
    struct SignalSemaphore lock;        /* Lock for device operations */

    /* Bridge data */
    ULONG               bridgemode;     /* Mode of AGP bridge */
    IPTR                bridgeaperbase; /* Base address for aperture */
    IPTR                bridgeapersize; /* Size of aperture */
    APTR                flushpage;
    ULONG               *gatttable;
    APTR                scratchmembuffer;/* Buffer for scratch mem */
    ULONG               *scratchmem;    /* 4096 aligned scratch mem */
    UBYTE               *regs;
    ULONG               firstgattentry;

    ULONG               state;          /* State of the device */


    /* IGP data */
    OOP_Object          *igp;           /* IGP video device */
};

#define CLID_Hidd_i965BridgeDevice   "hidd.agp.i965bridgedevice"

struct HIDDi965BridgeDeviceData
{
};

#define CLID_Hidd_g33BridgeDevice   "hidd.agp.g33bridgedevice"

struct HIDDg33BridgeDeviceData
{
};
/* Registers defines */
#define AGP_APER_BASE                   0x10                /* BAR0 */
#define AGP_VERSION_REG                 0x02
#define AGP_STATUS_REG                  0x04
#define AGP_STATUS_REG_AGP_3_0          (1<<3)
#define AGP_STATUS_REG_FAST_WRITES      (1<<4)
#define AGP_STATUS_REG_AGP_ENABLED      (1<<8)
#define AGP_STATUS_REG_SBA              (1<<9)
#define AGP_STATUS_REG_CAL_MASK         (1<<12|1<<11|1<<10)
#define AGP_STATUS_REG_ARQSZ_MASK       (1<<15|1<<14|1<<13)
#define AGP_STATUS_REG_RQ_DEPTH_MASK    0xff000000
#define AGP_STATUS_REG_AGP2_X1          (1<<0)
#define AGP_STATUS_REG_AGP2_X2          (1<<1)
#define AGP_STATUS_REG_AGP2_X4          (1<<2)
#define AGP_STATUS_REG_AGP3_X4          (1<<0)
#define AGP_STATUS_REG_AGP3_X8          (1<<1)
#define AGP_COMMAND_REG                 0x08
#define AGP_CTRL_REG                    0x10
#define AGP_CTRL_REG_GTBLEN             (1<<7)
#define AGP_CTRL_REG_APEREN             (1<<8)
#define AGP_APER_SIZE_REG               0x14
#define AGP_GATT_CTRL_LO_REG            0x18

#define AGP2_RESERVED_MASK              0x00fffcc8
#define AGP3_RESERVED_MASK              0x00ff00c4


#define ALIGN(val, align)               (val + align - 1) & (~(align - 1))

#define writel(val, addr)               (*(volatile ULONG*)(addr) = (val))
#define readl(addr)                     (*(volatile ULONG*)(addr))
#define min(a,b)                        ((a) < (b) ? (a) : (b))
#define max(a,b)                        ((a) > (b) ? (a) : (b))

/* Note: Both Linux and BSD codes use full wbinvd. CacheClearU gurantees a full flush */
#define flushcpucache()                 CacheClearU()

/* Config area access */
UBYTE readconfigbyte(OOP_Object * pciDevice, UBYTE where);
UWORD readconfigword(OOP_Object * pciDevice, UBYTE where);
ULONG readconfiglong(OOP_Object * pciDevice, UBYTE where);
VOID writeconfigbyte(OOP_Object * pciDevice, UBYTE where, UBYTE val);
VOID writeconfigword(OOP_Object * pciDevice, UBYTE where, UWORD val);
VOID writeconfiglong(OOP_Object * pciDevice, UBYTE where, ULONG val);

#endif
