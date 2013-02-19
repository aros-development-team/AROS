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

    asm volatile("mrc p15, 0, %[scp_reg], c0, c0, 0" : [scp_reg] "=X" (scp_reg) );

    info->Vendor = (scp_reg >> 24) & 0x7F;
    info->Family = (scp_reg >> 16) & 0xF;

    asm volatile("mrc p15, 0, %[scp_reg], c0, c0, 1" : [scp_reg] "=X" (scp_reg) );

    info->L1DataCacheSize = (scp_reg >> 18) & 0xF;
    info->L1InstructionCacheSize = (scp_reg >> 6) & 0xF;

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
