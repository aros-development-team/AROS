#ifndef KERNEL_I8259A_H
#define KERNEL_I8259A_H
/*
    Copyright © 2017-2022, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Intel 8259a "XT-PIC" definitions.
    Lang: english
*/

#include <aros/macros.h>
#include <inttypes.h>

#include <hardware/i8259a.h>

BOOL i8259a_Probe();
void i8259a_Disable();

extern struct IntrController i8259a_IntrController;

#endif /* !KERNEL_I8259A_H */
