/*
    Copyright 2011, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/debug.h>
#include <proto/oop.h>

#include "pcimockhardware.h"
#include "pcimockhardware_intern.h"
#include "pcimock_intern.h"

#include "pci_registers.h" /* From hidd.pci */

#undef HiddPCIMockHardwareAttrBase
#define HiddPCIMockHardwareAttrBase (SD(cl)->hiddPCIMockHardwareAB)

OOP_Object * METHOD(PCIMockHardware, Root, New)
{
    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    struct HIDDPCIMockHardwareData * hwdata = OOP_INST_DATA(cl, o);
    ULONG i;
    
    for (i = 0; i < PCI_REGIONS_COUNT; i++)
        ALLOC_ASR_NULL(hwdata, i);

    return o;
}

VOID PCIMockHardware__Root__Dispose(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    struct HIDDPCIMockHardwareData * hwdata = OOP_INST_DATA(cl, o);
    ULONG i;

    for (i = 0; i < PCI_REGIONS_COUNT; i++)
        if (hwdata->Regions[i].Address != (IPTR)0) FreeVec((APTR)hwdata->Regions[i].Address);
}

VOID METHOD(PCIMockHardware, Root, Get)
{
    struct HIDDPCIMockHardwareData * hwdata = OOP_INST_DATA(cl, o);
    ULONG idx;

    if (IS_PCIMOCKHARDWARE_ATTR(msg->attrID, idx))
    {
        switch (idx)
        {
        case aoHidd_PCIMockHardware_ConfigSpaceAddr:
            *msg->storage = hwdata->Regions[PCI_CONFIG_SPACE].Address;
            return;
        }
    }

    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

#define SUPPORT_BAR_SIZE_QUERY(hwdata, offset, n)                       \
    if (GET_ASR_DWORD(hwdata, PCI_CONFIG_SPACE, offset) == 0xffffffff)  \
    {                                                                   \
        ULONG tmp = hwdata->Regions[n].Size; tmp -= 1; tmp = ~tmp;      \
        SET_ASR_DWORD(hwdata, PCI_CONFIG_SPACE, offset, tmp);           \
    }                                                                   \


VOID METHOD(PCIMockHardware, Hidd_PCIMockHardware, MemoryChangedAtAddress)
{
    struct HIDDPCIMockHardwareData * hwdata = OOP_INST_DATA(cl, o);

    /* Detect in which address space the address is located and get relative offset */
    struct RegionAndOffset rao = HIDDPCIMockHardwareDetectRegionAndOffset(hwdata, msg->memoryaddress);

    if (rao.Region == -1)
        return;    
    
    /* Perform action based on type of address space and offset/register */
    if (rao.Region == PCI_CONFIG_SPACE)
    {
        switch(rao.Offset)
        {
            case(PCICS_BAR0) :
                SUPPORT_BAR_SIZE_QUERY(hwdata, PCICS_BAR0, PCI_BAR0);
                break;
            case(PCICS_BAR1) :
                SUPPORT_BAR_SIZE_QUERY(hwdata, PCICS_BAR1, PCI_BAR1);
                break;
            case(PCICS_BAR2) :
                SUPPORT_BAR_SIZE_QUERY(hwdata, PCICS_BAR2, PCI_BAR2);
                break;
            case(PCICS_BAR3) :
                SUPPORT_BAR_SIZE_QUERY(hwdata, PCICS_BAR3, PCI_BAR3);
                break;
            case(PCICS_BAR4) :
                SUPPORT_BAR_SIZE_QUERY(hwdata, PCICS_BAR4, PCI_BAR4);
                break;
            case(PCICS_BAR5) :
                SUPPORT_BAR_SIZE_QUERY(hwdata, PCICS_BAR5, PCI_BAR5);
                break;
            case(PCICS_EXPROM_BASE) :
                SUPPORT_BAR_SIZE_QUERY(hwdata, PCICS_EXPROM_BASE, PCI_ROM);
                break;
            default :
                bug("[PCIMockHardware(0x%x:0x%x)->MemoryChangedAtAddress - %d:0x%x Unhandled\n", 
                    (GET_ASR_DWORD(hwdata, PCI_CONFIG_SPACE, 0x00) & 0xffff),
                    ((GET_ASR_DWORD(hwdata, PCI_CONFIG_SPACE, 0x00) >> 16) & 0xffff),
                    rao.Region, rao.Offset);
                break;
        }
    }
}

VOID METHOD(PCIMockHardware, Hidd_PCIMockHardware, MemoryReadAtAddress)
{
    struct HIDDPCIMockHardwareData * hwdata = OOP_INST_DATA(cl, o);

    /* Detect in which address space the address is located and get relative offset */
    struct RegionAndOffset rao = HIDDPCIMockHardwareDetectRegionAndOffset(hwdata, msg->memoryaddress);

    if (rao.Region == -1)
        return;    
    
    bug("[PCIMockHardware(0x%x:0x%x)->MemoryReadAtAddress - %d:0x%x Unhandled\n", 
        (GET_ASR_DWORD(hwdata, PCI_CONFIG_SPACE, 0x00) & 0xffff),
        ((GET_ASR_DWORD(hwdata, PCI_CONFIG_SPACE, 0x00) >> 16) & 0xffff),
        rao.Region, rao.Offset);
}

struct RegionAndOffset HIDDPCIMockHardwareDetectRegionAndOffset(struct HIDDPCIMockHardwareData * hwdata, IPTR address)
{
    ULONG i = 0;
    struct RegionAndOffset rao = {-1, ~0};
    
    for (i = 0; i < PCI_REGIONS_COUNT; i++)
        if ((hwdata->Regions[i].Address <= address) && ((hwdata->Regions[i].Address + hwdata->Regions[i].Size) >= address))
        {
            rao.Region = i;
            rao.Offset = address - hwdata->Regions[i].Address;
            break;
        }

    return rao;
}

