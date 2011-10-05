/*
    Copyright 2010, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/symbolsets.h>
#include <hidd/agp.h>
#include <hidd/pci.h>
#include <proto/oop.h>
#include <proto/exec.h>
#include <aros/debug.h>

#include "agp_private.h"

static int AgpHidd_ExpungeLib(LIBBASETYPEPTR LIBBASE)
{
    if (LIBBASE->sd.bridgedevice)
        OOP_DisposeObject(LIBBASE->sd.bridgedevice);

    if (LIBBASE->sd.hiddAGPBridgeDeviceAB)
        OOP_ReleaseAttrBase((STRPTR)IID_Hidd_AGPBridgeDevice);

    if (LIBBASE->sd.hiddPCIDeviceAB)
        OOP_ReleaseAttrBase((STRPTR)IID_Hidd_PCIDevice);

    if (LIBBASE->sd.pcibus)
        OOP_DisposeObject(LIBBASE->sd.pcibus);

    return TRUE;
}

static int AgpHidd_InitLib(LIBBASETYPEPTR LIBBASE)
{
    LIBBASE->sd.bridgedevice = NULL;

    LIBBASE->sd.hiddAGPBridgeDeviceAB = OOP_ObtainAttrBase((STRPTR)IID_Hidd_AGPBridgeDevice);
    LIBBASE->sd.hiddPCIDeviceAB = OOP_ObtainAttrBase((STRPTR)IID_Hidd_PCIDevice);

    LIBBASE->sd.pcibus = OOP_NewObject(NULL, CLID_Hidd_PCI, NULL); 

    if (LIBBASE->sd.hiddAGPBridgeDeviceAB && LIBBASE->sd.pcibus && LIBBASE->sd.hiddPCIDeviceAB)
        return TRUE;

    return FALSE;
}

ADD2INITLIB(AgpHidd_InitLib, 0)
ADD2EXPUNGELIB(AgpHidd_ExpungeLib, 0)

