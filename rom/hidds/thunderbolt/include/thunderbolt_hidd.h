#ifndef HIDD_THUNDERBOLT_H
#define HIDD_THUNDERBOLT_H

/*
    Copyright © 2026, The AROS Development Team. All rights reserved.
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

#ifndef UTILITY_HOOKS_H
#include <utility/hooks.h>
#endif

#ifndef UTILITY_TAGITEM_H
#include <utility/tagitem.h>
#endif

/* Base Thunderbolt class */
#define CLID_Hidd_Thunderbolt           "hidd.thunderbolt"

#include <interface/Hidd_Thunderbolt.h>

/* Tags for EnumDevices method */
enum
{
    tHidd_Thunderbolt_VendorID          = TAG_USER,
    tHidd_Thunderbolt_DeviceID,
    tHidd_Thunderbolt_RevisionID,
    tHidd_Thunderbolt_Interface,
    tHidd_Thunderbolt_Class,
    tHidd_Thunderbolt_SubClass,
    tHidd_Thunderbolt_Domain,
    tHidd_Thunderbolt_Route,
    tHidd_Thunderbolt_Depth,
    tHidd_Thunderbolt_Port,
    tHidd_Thunderbolt_UID,
    tHidd_Thunderbolt_Driver
};

/* Thunderbolt device class */
#define CLID_Hidd_ThunderboltDevice     "hidd.thunderbolt.device"
#include <interface/Hidd_ThunderboltDevice.h>

#define IS_THUNDERBOLTDEV_ATTR(attr, idx) \
    (((idx) = (attr) - HiddThunderboltDeviceAttrBase) < num_Hidd_ThunderboltDevice_Attrs)

/* Thunderbolt driver class */
#define CLID_Hidd_ThunderboltDriver     "hidd.thunderbolt.driver"
#include <interface/Hidd_ThunderboltDriver.h>

#define IS_THUNDERBOLTDRV_ATTR(attr, idx) \
    (((idx) = (attr) - HiddThunderboltDriverAttrBase) < num_Hidd_ThunderboltDriver_Attrs)

struct Thunderbolt_RoutingEntry
{
    struct MinNode  re_Node;
    UWORD           re_Domain;
    ULONG           re_Route;
    UBYTE           re_Port;
    UBYTE           re_Depth;
};

#endif
