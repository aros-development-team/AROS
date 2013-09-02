/*
    Copyright 2011, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/debug.h>
#include <proto/oop.h>

#include "pcimockhardware.h"
#include "pcimockhardware_intern.h"
#include "pcimock_intern.h"

OOP_Object * METHOD(SIS661FXMockHardware, Root, New)
{
    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);

    GET_PCIMOCKHWDATA
    
    /* Allocate address space regions */
    ALLOC_ASR(hwdata, PCI_CONFIG_SPACE, 256);
    ALLOC_ASR(hwdata, PCI_BAR0, 0x4000000);

    /* Fill in pci config space */
    SET_ASR_DWORD(hwdata, PCI_CONFIG_SPACE, 0x00, 0x06611039); /* PCICS_PRODUCT, PCICS_VENDOR */
    SET_ASR_DWORD(hwdata, PCI_CONFIG_SPACE, 0x04, 0x00100007); /* PCICS_STATUS, PCICS_COMMAND */
    SET_ASR_DWORD(hwdata, PCI_CONFIG_SPACE, 0x08, 0x06000011); /* PCICS_CLASS, PCICS_SUBCLASS, PCICS_PROGIF, PCICS_REVISION */
    SET_ASR_DWORD(hwdata, PCI_CONFIG_SPACE, 0x0c, 0x00000000); /* PCICS_BIST, PCICS_HEADERTYPE, PCICS_LATENCY, PCICS_CACHELS */
    SET_ASR_DWORD(hwdata, PCI_CONFIG_SPACE, 0x10, hwdata->Regions[PCI_BAR0].Address); /* PCICS_BAR0 */
    SET_ASR_DWORD(hwdata, PCI_CONFIG_SPACE, 0x2c, 0x00000000); /* PCICS_SUBSYSTEM, PCICS_SUBVENDOR */
    SET_ASR_DWORD(hwdata, PCI_CONFIG_SPACE, 0x34, 0x00000080); /* PCICS_CAP_PTR */
    SET_ASR_DWORD(hwdata, PCI_CONFIG_SPACE, 0x3c, 0x00000000); /* PCICS_INT_PIN, PCICS_INT_LINE */

    /* Capabilities */
    SET_ASR_DWORD(hwdata, PCI_CONFIG_SPACE, 0x80, 0x0030ff02); /* AGP VERSION 3.0, CAPABILITY: END, CAPABILITY: PCICAP_AGP */

    /* AGP Registers */
    SET_ASR_DWORD(hwdata, PCI_CONFIG_SPACE, 0x84, 0x1f004e1b); /* bridgemode */
    SET_ASR_DWORD(hwdata, PCI_CONFIG_SPACE, 0x94, 0x00000040); /* AGP_SIS_APER_SIZE */
    
    return o;
}

VOID METHOD(SIS661FXMockHardware, Hidd_PCIMockHardware, MemoryReadAtAddress)
{
    GET_PCIMOCKHWDATA

    /* Detect in which address space the address is located and get relative offset */
    struct RegionAndOffset rao = HIDDPCIMockHardwareDetectRegionAndOffset(hwdata, msg->memoryaddress);

    if (rao.Region == -1)
        return;    

    if (rao.Region == PCI_CONFIG_SPACE)
    {
        switch(rao.Offset)
        {
            case(0x00000000):
            case(0x00000004):
            case(0x00000008):
            case(0x0000000c):
            case(0x00000010):
            case(0x00000014):
            case(0x00000018):
            case(0x0000001c):
            case(0x00000020):
            case(0x00000024):
            case(0x0000002c):
            case(0x00000030):
            case(0x00000034):
            case(0x0000003c):
            case(0x00000080):
            case(0x00000084):
            case(0x00000094):
            case(0x00000098):
            return;
        }
    }        
        
    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

VOID METHOD(SIS661FXMockHardware, Hidd_PCIMockHardware, MemoryChangedAtAddress)
{
    GET_PCIMOCKHWDATA

    /* Detect in which address space the address is located and get relative offset */
    struct RegionAndOffset rao = HIDDPCIMockHardwareDetectRegionAndOffset(hwdata, msg->memoryaddress);

    if (rao.Region == -1)
        return;    

    if (rao.Region == PCI_CONFIG_SPACE)
    {
        switch(rao.Offset)
        {
            case(0x00000088):
            case(0x00000090):
            case(0x00000094):
            case(0x00000098):
            return;
        }
    }        
        
    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

