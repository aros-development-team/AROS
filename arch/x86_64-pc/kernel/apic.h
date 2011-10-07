/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: AROS APIC functions.
    Lang: english
*/

#ifndef __AROS_APIC_H__
#define __AROS_APIC_H__

#include <utility/hooks.h>

#include "apic_driver.h"

UBYTE core_APIC_GetNumber(struct KernelBase *KernelBase, IPTR __APICBase);

#endif /* __AROS_APIC_H__ */
