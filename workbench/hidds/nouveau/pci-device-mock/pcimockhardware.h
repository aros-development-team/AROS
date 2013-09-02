#ifndef HIDD_PCIMOCKHARDWARE_H
#define HIDD_PCIMOCKHARDWARE_H

/*
    Copyright 2011, The AROS Development Team. All rights reserved.
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

#define CLID_Hidd_PCIMockHardware   "hidd.pcimockhardware"
#define IID_Hidd_PCIMockHardware    "hidd.pcimockhardware"

#define HiddPCIMockHardwareAttrBase __IHidd_PCIMockHardware

#ifndef __OOP_NOATTRBASES__
extern OOP_AttrBase HiddPCIMockHardwareAttrBase;
#endif

enum
{
    moHidd_PCIMockHardware_MemoryChangedAtAddress = 0,
    moHidd_PCIMockHardware_MemoryReadAtAddress,

    NUM_PCIMOCKHARDWARE_METHODS
};

enum
{
    aoHidd_PCIMockHardware_ConfigSpaceAddr, /* [..G] Address of PCI config space */
    
    num_Hidd_PCIMockHardware_Attrs
};

#define aHidd_PCIMockHardware_ConfigSpaceAddr   (HiddPCIMockHardwareAttrBase + aoHidd_PCIMockHardware_ConfigSpaceAddr)

#define IS_PCIMOCKHARDWARE_ATTR(attr, idx) \
    (((idx) = (attr) - HiddPCIMockHardwareAttrBase) < num_Hidd_PCIMockHardware_Attrs)

struct pHidd_PCIMockHardware_MemoryChangedAtAddress
{
    OOP_MethodID    mID;
    IPTR            memoryaddress;
};

struct pHidd_PCIMockHardware_MemoryReadAtAddress
{
    OOP_MethodID    mID;
    IPTR            memoryaddress;
};

#endif
