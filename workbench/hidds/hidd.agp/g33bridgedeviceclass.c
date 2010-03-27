/*
    Copyright 2010, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <hidd/agp.h>
#include <hidd/pci.h>
#include <proto/oop.h>
#include <proto/exec.h>
#define DEBUG 1
#include <aros/debug.h>

#include "agp_private.h"

#undef HiddAGPBridgeDeviceAttrBase
#define HiddAGPBridgeDeviceAttrBase (SD(cl)->hiddAGPBridgeDeviceAB)

/* Rething object model g33<->i965<->i915 so that code is not duplicated */

BOOL METHOD(g33BridgeDevice, Hidd_AGPBridgeDevice, CreateGattTable)
{
    /* TODO: IMPLEMENT */
    /* Note: identical as i915 */
    return FALSE;
}

BOOL METHOD(g33BridgeDevice, Hidd_AGPBridgeDevice, ScanAndDetectDevices)
{
    /* TODO: IMPLEMENT */
    /* TODO: Check if interrupt is set */
    /* Note: new method */
    return FALSE;
}

/* PUBLIC METHODS */
OOP_Object * METHOD(g33BridgeDevice, Root, New)
{
    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg) msg);
    
    return o;
}


VOID METHOD(g33BridgeDevice, Hidd_AGPBridgeDevice, BindMemory)
{
    /* TODO: Implement */
    /* Note : identical as i965 */
}

BOOL METHOD(g33BridgeDevice, Hidd_AGPBridgeDevice, Initialize)
{
    /* TODO: Implement */
    /* Note: specific detection, but initialization code identical as i915 */   
    
    /* THIS CLASS IS NOT FULLY IMPLEMENT AND NOT TESTED AT ALL */
    return FALSE;
}
