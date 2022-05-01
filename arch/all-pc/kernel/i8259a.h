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

#define I8259A_IRQCOUNT         16

#define MASTER8259_CMDREG       0x20
#define MASTER8259_MASKREG      0x21
#define MASTER8259_ELCREG       0x4d0
#define SLAVE8259_CMDREG        0xA0
#define SLAVE8259_MASKREG       0xA1
#define SLAVE8259_ELCREG        0x4d1

BOOL i8259a_Probe();
void i8259a_Disable();

extern struct IntrController i8259a_IntrController;

#endif /* !KERNEL_I8259A_H */
