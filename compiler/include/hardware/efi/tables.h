#ifndef HARDWARE_EFI_TABLES_H
#define HARDWARE_EFI_TABLES_H

/*
    Copyright © 2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Basic definitions for EFI tables
    Lang: english
*/

#include <exec/types.h>

#ifdef __x86_64__
/* On x86-64 EFI uses Microsoft calling convention */
#define __eficall __attribute((ms_abi))
#else
#define __eficall
#endif

/* Common header of all tables */
struct EFI_TableHeader
{
    UQUAD Signature;
    ULONG Revision;
    ULONG HeaderSize;
    ULONG CRC32;
    ULONG Reserved;
};

#endif
