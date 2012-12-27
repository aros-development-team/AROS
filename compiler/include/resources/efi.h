#ifndef RESOURCES_EFI_H
#define RESOURCES_EFI_H

/*
    Copyright © 2011-2012, The AROS Development Team. All rights reserved.
    $Id$

    Desc: efi.resource definitions
    Lang: english
*/

#include <exec/lists.h>
#include <exec/interrupts.h>
#include <hardware/efi/efi.h>

struct EFIBase
{
    struct Node		    node;    /* Resource node		 */
    struct Interrupt        reset_handler; /* Reboots or powers off */
    struct EFI_SystemTable *System;  /* System table pointer	 */
    struct EFI_Runtime	   *Runtime; /* Runtime services pointer */
};

#endif
