#ifndef HARDWARE_EFI_EFI_H
#define HARDWARE_EFI_EFI_H

/*
    Copyright © 2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: EFI firmware main interface
    Lang: english
*/

#include <hardware/efi/config.h>
#include <hardware/efi/console.h>
#include <hardware/efi/runtime.h>

/* System table */
struct EFI_SystemTable
{
    struct EFI_TableHeader  Hdr;

    UWORD		   *Vendor;		/* Unicode string */
    ULONG		    Revision;
    APTR		    ConInHandle;
    struct EFI_SimpleIn	   *ConIn;
    APTR		    ConOutHandle;
    struct EFI_SimpleOut   *ConOut;
    APTR		    StdErrHandle;
    struct EFI_SimpleOut   *StdErr;
    struct EFI_Runtime	   *RuntimeServices;
    APTR		    BootServices;
    IPTR		    NumEntries;
    struct EFI_Config	   *ConfigTable;
};

/* Signature of the EFI system table */
#define EFI_SYSTEM_TABLE_SIGNATURE 0x5453595320494249

#endif
