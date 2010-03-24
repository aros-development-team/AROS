#ifndef HIDD_AGP_H
#define HIDD_AGP_H

/*
    Copyright 2010, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

#ifndef HIDD_HIDD_H
#include <hidd/hidd.h>
#endif

#ifndef OOP_OOP_H
#include <oop/oop.h>
#endif

/* Base AGP class */

#define CLID_Hidd_AGP   "hidd.agp"
#define IID_Hidd_AGP    "hidd.agp"

#define HiddAGPAttrBase __IHidd_AGP

#ifndef __OOP_NOATTRBASES__
extern OOP_AttrBase HiddAGPAttrBase;
#endif

/* AGP Class methods */
enum
{
    moHidd_AGP_GetBridgeDevice = 0,

    NUM_AGP_METHODS
};

struct pHidd_AGP_GetBridgeDevice
{
    OOP_MethodID    mID;
};

enum
{
    vHidd_AGP_NormalMemory = 1,
    vHidd_AGP_CachedMemory
};

#define IID_Hidd_AGPBridgeDevice    "hidd.agp.bridgedevice"

#define HiddAGPBridgeDeviceAttrBase	__IHidd_AGPBridgeDev

#ifndef __OOP_NOATTRBASES__
extern OOP_AttrBase HiddAGPBridgeDeviceAttrBase;
#endif

enum
{
    moHidd_AGPBridgeDevice_Initialize = 0,    
    moHidd_AGPBridgeDevice_Enable,
    moHidd_AGPBridgeDevice_BindMemory,
    moHidd_AGPBridgeDevice_UnBindMemory,
    moHidd_AGPBridgeDevice_FlushChipset,

    NUM_AGPBRIDGEDEVICE_METHODS
};

enum
{
    aoHidd_AGPBridgeDevice_Mode,    /* [..G] Mode of the bridge */
    aoHidd_AGPBridgeDevice_ApertureBase,    /* [..G] Base of the aperture */
    aoHidd_AGPBridgeDevice_ApertureSize,    /* [..G] Size of the aperture */
    
    num_Hidd_AGPBridgeDevice_Attrs
};

#define aHidd_AGPBridgeDevice_Mode          (HiddAGPBridgeDeviceAttrBase + aoHidd_AGPBridgeDevice_Mode)
#define aHidd_AGPBridgeDevice_ApertureBase  (HiddAGPBridgeDeviceAttrBase + aoHidd_AGPBridgeDevice_ApertureBase)
#define aHidd_AGPBridgeDevice_ApertureSize  (HiddAGPBridgeDeviceAttrBase + aoHidd_AGPBridgeDevice_ApertureSize)

#define IS_AGPBRIDGEDEV_ATTR(attr, idx) \
    (((idx) = (attr) - HiddAGPBridgeDeviceAttrBase) < num_Hidd_AGPBridgeDevice_Attrs)

struct pHidd_AGPBridgeDevice_Initialize
{
    OOP_MethodID    mID;
};

struct pHidd_AGPBridgeDevice_Enable
{
    OOP_MethodID    mID;
    ULONG           requestedmode;
};

struct pHidd_AGPBridgeDevice_BindMemory
{
    OOP_MethodID    mID;
    IPTR            address;
    ULONG           size;
    ULONG           offset;
    UBYTE           type;
};

struct pHidd_AGPBridgeDevice_UnBindMemory
{
    OOP_MethodID    mID;
    ULONG           offset;
    ULONG           size;
};

struct pHidd_AGPBridgeDevice_FlushChipset
{
    OOP_MethodID    mID;
};

#endif
