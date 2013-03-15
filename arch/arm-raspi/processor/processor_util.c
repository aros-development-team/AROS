/*
    Copyright © 2013, The AROS Development Team. All rights reserved.
    $Id$
*/

#define DEBUG 0
#include <aros/debug.h>

#include <resources/processor.h>
#include <string.h>

#include "processor_arch_intern.h"

#define DPROBE(a)

static const char *ARMCPUVendors[] =
{
    "Unknown",
    "ARM Ltd.",
    "DEC",
    "Motorola",
    "Texas Instruments",
    "Qualcomm",
    "Marvell",
    "Intel",
    NULL
};

static const char *ARMCPUFamilies[] =
{
    "v3",
    "v4",
    "v4T",
    "v5",
    "v5T",
    "v5TE",
    "v5TEJ",
    "v6",
    "v7",
    NULL
};

VOID ReadProcessorInformation(struct ARMProcessorInformation * info)
{
    register unsigned int scp_reg, cache_reg;
    APTR ssp;

    D(bug("[processor.ARM] %s()\n", __PRETTY_FUNCTION__));

    D(bug("[processor.ARM] %s: Probing CPU ..\n", __PRETTY_FUNCTION__));

    ssp = SuperState();

    DPROBE(bug("[processor.ARM] %s: Checking Main ID Register..\n", __PRETTY_FUNCTION__));
    asm volatile("mrc p15, 0, %[scp_reg], c0, c0, 0" : [scp_reg] "=r" (scp_reg) );

    info->VendorID = (scp_reg >> 24) & 0x7F;

    if ((scp_reg & 0x8F000) == 0)
        info->Family = CPUFAMILY_UNKNOWN;
    else if ((scp_reg & 0x8F000) == 0x7000)
        info->Family = (scp_reg & (1 << 23)) ? CPUFAMILY_ARM_4T : CPUFAMILY_ARM_3;
    else if ((scp_reg & 0x80000) == 0)
    {
        info->Family = CPUFAMILY_ARM_3;
        if ((scp_reg >> 16) & 7)
            info->Family += ((scp_reg >> 16) & 7);
    }
    else if ((scp_reg & 0xF0000) == 0xF0000)
    {
        info->Family = CPUFAMILY_ARM_6;

        DPROBE(bug("[processor.ARM] %s: Checking Memory Model Feature Register..\n", __PRETTY_FUNCTION__));
        asm volatile("mrc p15, 0, %[scp_reg], c0, c1, 4" : [scp_reg] "=r" (scp_reg) );

        if (((((scp_reg >> 4) & 0xF) >= 1) && ((scp_reg & 0xF) >= 3)) || (((scp_reg >> 4) & 0xF) >= 3))
            info->Family = CPUFAMILY_ARM_7;

        DPROBE(bug("[processor.ARM] %s:  - %02d:%02d\n", __PRETTY_FUNCTION__, (scp_reg >> 4) & 0xF, scp_reg & 0xF));
    } 
    else
        info->Family = CPUFAMILY_UNKNOWN;

    DPROBE(bug("[processor.ARM] %s: Checking Feature Register #1 ..\n", __PRETTY_FUNCTION__));
    asm volatile("mrc p15, 0, %[scp_reg], c0, c1, 1" : [scp_reg] "=r" (scp_reg) );

    if (scp_reg & (0xF << 4))
        info->Features1 |= FEATF_SECURE;

    DPROBE(bug("[processor.ARM] %s: Checking Feature Register #0 ..\n", __PRETTY_FUNCTION__));
    asm volatile("mrc p15, 0, %[scp_reg], c0, c1, 1" : [scp_reg] "=r" (scp_reg) );

    DPROBE(bug("[processor.ARM] %s: Checking Co-Processor Access Control Register..\n", __PRETTY_FUNCTION__));
    asm volatile("mrc p15,0,%[scp_reg], c1, c0, 2\n" : [scp_reg] "=r" (scp_reg));
    if (scp_reg & ((3 << 20)|(3 << 22)))
        info->Features1 |= FEATF_FPU_VFP;

    if (info->Features1 & FEATF_FPU_VFP)
    {
        DPROBE(bug("[processor.ARM] %s: Checking Floating Point System ID Register..\n", __PRETTY_FUNCTION__));
        asm volatile("fmrx %[scp_reg], FPSID\n" : [scp_reg] "=r" (scp_reg));
        if (((scp_reg >> 16) & 0xF) >= 2)
        {
            info->Features1  |= FEATF_FPU_VFP3;
        }
        else
        {
/*
            info->Features1  |= FEATF_FPU_VFP2;
*/
        }

        DPROBE(bug("[processor.ARM] %s: Checking Media and VFP Feature Register #0..\n", __PRETTY_FUNCTION__));
        asm volatile("fmrx %[scp_reg], MVFR0\n" : [scp_reg] "=r" (scp_reg));
        if ((scp_reg & 0xF) == 1)
        {
            info->Features1 |= FEATF_FPU_VFP3_16;
        }

        DPROBE(bug("[processor.ARM] %s: Checking Media and VFP Feature Register #1..\n", __PRETTY_FUNCTION__));
        asm volatile("fmrx %[scp_reg], MVFR1\n" : [scp_reg] "=r" (scp_reg));
        if ((scp_reg & 0x000FFF00) == 0x00011100)
        {
            info->Features1 |= FEATF_NEON;
        }
        if ((scp_reg & 0xF0000000) == 0x10000000)
        {
            info->Features1 |= FEATF_FPU_VFP4;
        }
    }

    DPROBE(bug("[processor.ARM] %s: Checking Instruction Set Attributes Register #3..\n", __PRETTY_FUNCTION__));
    asm volatile("mrc p15,0,%[scp_reg], c0, c2, 3\n" : [scp_reg] "=r" (scp_reg));
    if (((scp_reg >> 28) & 0xF) > 0)
        info->Features1 |= FEATF_THUMBEX;

    DPROBE(bug("[processor.ARM] %s: Checking System Control Register..\n", __PRETTY_FUNCTION__));
    asm volatile("mrc p15, 0, %[scp_reg], c1, c0, 0" : [scp_reg] "=r" (scp_reg) );

    if (scp_reg & (1 << 11))
        info->Features1 |= FEATF_BRANCHP;

    if (scp_reg & (1 << 31))
        info->Features1 |= FEATF_BIGEND;

    DPROBE(bug("[processor.ARM] %s: Checking Cache Type Register..\n", __PRETTY_FUNCTION__));
    asm volatile("mrc p15, 0, %[cache_reg], c0, c0, 1" : [cache_reg] "=r" (cache_reg) );

    if (scp_reg & (1 << 2))
    {    
        switch((cache_reg >> 18) & 0xF) {
            case 3:
                info->L1DataCacheSize = 4;
                break;
            case 4:
                info->L1DataCacheSize = 8;
                break;
            case 5:
                info->L1DataCacheSize = 16;
                break;
            case 6:
                info->L1DataCacheSize = 32;
                break;
            case 7:
                info->L1DataCacheSize = 64;
                break;
            case 8:
                info->L1DataCacheSize = 128;
                break;
            case 9:
                info->L1DataCacheSize = 256;
                break;
            case 10:
                info->L1DataCacheSize = 512;
                break;
            case 11:
                info->L1DataCacheSize = 1024;
                break;
            default:
                info->L1DataCacheSize = 0;
                break;
        }
    }
    if (scp_reg & (1 << 12))
    {    
        switch((cache_reg >> 6) & 0xF) {
            case 3:
                info->L1InstructionCacheSize = 4;
                break;
            case 4:
                info->L1InstructionCacheSize = 8;
                break;
            case 5:
                info->L1InstructionCacheSize = 16;
                break;
            case 6:
                info->L1InstructionCacheSize = 32;
                break;
            case 7:
                info->L1InstructionCacheSize = 64;
                break;
            case 8:
                info->L1InstructionCacheSize = 128;
                break;
            case 9:
                info->L1InstructionCacheSize = 256;
                break;
            case 10:
                info->L1InstructionCacheSize = 512;
                break;
            case 11:
                info->L1InstructionCacheSize = 1024;
                break;
            default:
                info->L1InstructionCacheSize = 0;
                break;
        }
    }
    D(bug("[processor.ARM] %s: .. Done\n", __PRETTY_FUNCTION__));

    UserState(ssp);

    switch (info->VendorID) {
        case 'A':
            info->Vendor = ARMCPUVendors[1];
            break;
        case 'D':
            info->Vendor = ARMCPUVendors[2];
            break;
        case 'M':
            info->Vendor = ARMCPUVendors[3];
            break;
        case 'T':
            info->Vendor = ARMCPUVendors[4];
            break;
        case 'Q':
            info->Vendor = ARMCPUVendors[5];
            break;
        case 'V':
            info->Vendor = ARMCPUVendors[6];
            break;
        case 'i':
            info->Vendor = ARMCPUVendors[7];
            break;
        default:
            info->Vendor = ARMCPUVendors[0];
            info->Vendor = 0;
            break;
    }

    if (info->Family >= CPUFAMILY_ARM_3)
        info->FamilyString = ARMCPUFamilies[info->Family - CPUFAMILY_ARM_3];

    D(bug("[processor.ARM] %s: CPU Details Read\n", __PRETTY_FUNCTION__));
}
