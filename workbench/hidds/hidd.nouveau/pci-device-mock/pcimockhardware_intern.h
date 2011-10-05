#ifndef PCIMOCKHARDWARE_INTERN_H
#define PCIMOCKHARDWARE_INTERN_H

/*
    Copyright 2011, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/types.h>

#define PCI_BAR0            0
#define PCI_BAR1            1
#define PCI_BAR2            2
#define PCI_BAR3            3
#define PCI_BAR4            4
#define PCI_BAR5            5
#define PCI_CONFIG_SPACE    6
#define PCI_ROM             7
#define PCI_REGIONS_COUNT   8

struct AddressSpaceRegion
{
    ULONG   Size;
    IPTR    Address;
};

struct HIDDPCIMockHardwareData
{
    struct AddressSpaceRegion   Regions[PCI_REGIONS_COUNT];
};


#define GET_PCIMOCKHWDATA \
    struct HIDDPCIMockHardwareData * hwdata = OOP_INST_DATA((SD(cl))->pciMockHardwareClass, o);

#define ALLOC_ASR(hwdata, n, size)                                                  \
    {                                                                               \
        hwdata->Regions[n].Size = size;                                             \
        hwdata->Regions[n].Address = (IPTR)AllocVec(size, MEMF_ANY | MEMF_CLEAR);   \
    }                                                                               \

#define ALLOC_ASR_NULL(hwdata, n)                                               \
    {                                                                           \
        hwdata->Regions[n].Size = 0;                                            \
        hwdata->Regions[n].Address = (IPTR)NULL;                                \
    }                                                                           \

#define SET_ASR_DWORD(hwdata, n, address, value) \
    *((ULONG *)(hwdata->Regions[n].Address + address)) = value;

#define GET_ASR_DWORD(hwdata, n, address) \
    *((ULONG *)(hwdata->Regions[n].Address + address))

#define DEF_NEXTCAPADDR         \
    ULONG nextcapaddr = 0x80;   \

#define ADD_PCI_CAP(hwdata, pcicap)                                 \
    {                                                               \
        ULONG val = ((nextcapaddr + 4) << 8) | pcicap;              \
        SET_ASR_DWORD(hwdata, PCI_CONFIG_SPACE, nextcapaddr, val);  \
        nextcapaddr += 4;                                           \
    }                                                               \
    

struct RegionAndOffset
{
    LONG    Region;
    IPTR    Offset;
};

struct RegionAndOffset HIDDPCIMockHardwareDetectRegionAndOffset(struct HIDDPCIMockHardwareData * hwdata, IPTR address);

/* Emulated hardware */

#define CLID_Hidd_PCIMockHardware_NV44A   "hidd.pcimockhardware.nv44a"

struct HIDDNV44AMockHardwareData
{
};

#define CLID_Hidd_PCIMockHardware_NVG86   "hidd.pcimockhardware.nvg86"

struct HIDDNVG86MockHardwareData
{
};

#define CLID_Hidd_PCIMockHardware_NVGTS250   "hidd.pcimockhardware.nvgts250"

struct HIDDNVGTS250MockHardwareData
{
};

#define CLID_Hidd_PCIMockHardware_NVGF100   "hidd.pcimockhardware.nvgf100"

struct HIDDNVGF100MockHardwareData
{
};

#define CLID_Hidd_PCIMockHardware_SIS661FX  "hidd.pcimockhardware.sis661fx"

struct HIDDSIS661FXMockHardwareData
{
};

#endif /* PCIMOCKHARDWARE_INTERN_H */

