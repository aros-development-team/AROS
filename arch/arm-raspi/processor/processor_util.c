/*
    Copyright © 2013, The AROS Development Team. All rights reserved.
    $Id$
*/

#define DEBUG 1
#include <aros/debug.h>

#include <resources/processor.h>
#include <string.h>

#include "processor_arch_intern.h"

static const char *vendors[] =
{
    "Unknown",
    "ARM",
    "DEC",
    "Motorola",
    "Texas Instruments",
    "Qualcomm",
    "Marvell",
    "Intel",
    NULL
};


VOID ReadProcessorInformation(struct ARMProcessorInformation * info)
{
    register unsigned int scp_reg;
    APTR ssp;

    D(bug("[processor.ARM] %s()\n", __PRETTY_FUNCTION__));

    ssp = SuperState();

    D(bug("[processor.ARM] %s: Probing CPU ..\n", __PRETTY_FUNCTION__));

    asm volatile("mrc p15, 0, %[scp_reg], c0, c0, 0" : [scp_reg] "=r" (scp_reg) );

    info->Vendor = (scp_reg >> 24) & 0x7F;
    info->Family = (scp_reg >> 16) & 0xF;

    asm volatile("mrc p15, 0, %[scp_reg], c0, c0, 1" : [scp_reg] "=r" (scp_reg) );

    switch((scp_reg >> 18) & 0xF) {
        case 3:
            info->L1DataCacheSize = 4096;
            break;
        case 4:
            info->L1DataCacheSize = 8192;
            break;
        case 5:
            info->L1DataCacheSize = 16384;
            break;
        case 6:
            info->L1DataCacheSize = 32768;
            break;
        case 7:
            info->L1DataCacheSize = 65536;
            break;
        case 8:
            info->L1DataCacheSize = 131072;
            break;
        case 9:
            info->L1DataCacheSize = 262144;
            break;
        case 10:
            info->L1DataCacheSize = 524288;
            break;
        case 11:
            info->L1DataCacheSize = 1048576;
            break;
        default:
            info->L1DataCacheSize = 0;
            break;
    }

    switch((scp_reg >> 6) & 0xF) {
        case 3:
            info->L1InstructionCacheSize = 4096;
            break;
        case 4:
            info->L1InstructionCacheSize = 8192;
            break;
        case 5:
            info->L1InstructionCacheSize = 16384;
            break;
        case 6:
            info->L1InstructionCacheSize = 32768;
            break;
        case 7:
            info->L1InstructionCacheSize = 65536;
            break;
        case 8:
            info->L1InstructionCacheSize = 131072;
            break;
        case 9:
            info->L1InstructionCacheSize = 262144;
            break;
        case 10:
            info->L1InstructionCacheSize = 524288;
            break;
        case 11:
            info->L1InstructionCacheSize = 1048576;
            break;
        default:
            info->L1InstructionCacheSize = 0;
            break;
    }

    D(bug("[processor.ARM] %s: .. Done\n", __PRETTY_FUNCTION__));

    UserState(ssp);

    switch (info->Vendor) {
        case 'A':
            info->VendorID = vendors[1];
            break;
        case 'D':
            info->VendorID = vendors[2];
            break;
        case 'M':
            info->VendorID = vendors[3];
            break;
        case 'T':
            info->VendorID = vendors[4];
            break;
        case 'Q':
            info->VendorID = vendors[5];
            break;
        case 'V':
            info->VendorID = vendors[6];
            break;
        case 'i':
            info->VendorID = vendors[7];
            break;
        default:
            info->VendorID = vendors[0];
            break;
    }
    D(bug("[processor.ARM] %s: CPU Details Read\n", __PRETTY_FUNCTION__));
}
