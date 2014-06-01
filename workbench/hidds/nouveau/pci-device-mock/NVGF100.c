/*
    Copyright 2011, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/debug.h>
#include <proto/oop.h>

#include "pcimockhardware.h"
#include "pcimockhardware_intern.h"
#include "pcimock_intern.h"

/* chip GF100, model GTX 470 */

OOP_Object * METHOD(NVGF100MockHardware, Root, New)
{
    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);

    GET_PCIMOCKHWDATA
    DEF_NEXTCAPADDR
    
    /* Allocate address space regions */
    ALLOC_ASR(hwdata, PCI_CONFIG_SPACE, 256);
    ALLOC_ASR(hwdata, PCI_BAR0, 0x1000000);
    ALLOC_ASR(hwdata, PCI_BAR1, 0x10000000);
    ALLOC_ASR(hwdata, PCI_BAR3, 0x2000000);
    ALLOC_ASR(hwdata, PCI_ROM, 0x20000);

    /* Fill in pci config space */
    SET_ASR_DWORD(hwdata, PCI_CONFIG_SPACE, 0x00, 0x06cd10de); /* PCICS_PRODUCT, PCICS_VENDOR */
    SET_ASR_DWORD(hwdata, PCI_CONFIG_SPACE, 0x04, 0x00300006); /* PCICS_STATUS, PCICS_COMMAND */
    SET_ASR_DWORD(hwdata, PCI_CONFIG_SPACE, 0x08, 0x030000a2); /* PCICS_CLASS, PCICS_SUBCLASS, PCICS_PROGIF, PCICS_REVISION */
    SET_ASR_DWORD(hwdata, PCI_CONFIG_SPACE, 0x0c, 0x00000000); /* PCICS_BIST, PCICS_HEADERTYPE, PCICS_LATENCY, PCICS_CACHELS */
    SET_ASR_DWORD(hwdata, PCI_CONFIG_SPACE, 0x10, hwdata->Regions[PCI_BAR0].Address); /* PCICS_BAR0 */
    SET_ASR_DWORD(hwdata, PCI_CONFIG_SPACE, 0x14, hwdata->Regions[PCI_BAR1].Address); /* PCICS_BAR1 */
    SET_ASR_DWORD(hwdata, PCI_CONFIG_SPACE, 0x1c, hwdata->Regions[PCI_BAR3].Address); /* PCICS_BAR3 */
    SET_ASR_DWORD(hwdata, PCI_CONFIG_SPACE, 0x2c, 0x510319da); /* PCICS_SUBSYSTEM, PCICS_SUBVENDOR */
    SET_ASR_DWORD(hwdata, PCI_CONFIG_SPACE, 0x30, hwdata->Regions[PCI_ROM].Address); /* PCICS_EXPROM_BASE */
    SET_ASR_DWORD(hwdata, PCI_CONFIG_SPACE, 0x34, 0x00000080); /* PCICS_CAP_PTR */
    SET_ASR_DWORD(hwdata, PCI_CONFIG_SPACE, 0x3c, 0x0000010b); /* PCICS_INT_PIN, PCICS_INT_LINE */

    /* Capabilities */
    ADD_PCI_CAP(hwdata, 0x10); /* CAPABILITY: PCICAP_PCIE */
    ADD_PCI_CAP(hwdata, 0xff); /* CAPABILITY: END */

    /* Fill in BAR 0 */
    SET_ASR_DWORD(hwdata, PCI_BAR0, 0x00000000, 0x0c0000a2); /* NV03_PMC_BOOT_0 */
    SET_ASR_DWORD(hwdata, PCI_BAR0, 0x0010020c, 0x10000000); /* NV04_PFB_FIFO_DATA */
    SET_ASR_DWORD(hwdata, PCI_BAR0, 0x0010f20c, 0x00000100); /* bsize */
    SET_ASR_DWORD(hwdata, PCI_BAR0, 0x00121c74, 0x00000001); /* parts */
    SET_ASR_DWORD(hwdata, PCI_BAR0, 0x0011020c, 0x00000100); /* psize */
    SET_ASR_DWORD(hwdata, PCI_BAR0, 0x00001540, 0x01010001); /* NV40_PMC_GRAPH_UNITS */
    SET_ASR_DWORD(hwdata, PCI_BAR0, 0x00409604, 0x00050004); /* rop_nr, gpc_nr */
    SET_ASR_DWORD(hwdata, PCI_BAR0, 0x00502608, 0x00000003); /* GPC 0 */
    SET_ASR_DWORD(hwdata, PCI_BAR0, 0x0050a608, 0x00000003); /* GPC 1 */
    SET_ASR_DWORD(hwdata, PCI_BAR0, 0x00512608, 0x00000004); /* GPC 2 */
    SET_ASR_DWORD(hwdata, PCI_BAR0, 0x0051a608, 0x00000004); /* GPC 3 */
    SET_ASR_DWORD(hwdata, PCI_BAR0, 0x00409804, 0x000037f0); /* GRCTX size */

    return o;
}

VOID METHOD(NVGF100MockHardware, Hidd_PCIMockHardware, MemoryReadAtAddress)
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
            return;
        }
    }

    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

VOID METHOD(NVGF100MockHardware, Hidd_PCIMockHardware, MemoryChangedAtAddress)
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
            case(0x00000004):
            return;
        }
    }

    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}
