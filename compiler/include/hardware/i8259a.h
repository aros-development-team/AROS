
#ifndef HARDWARE_I8259A_H
#define HARDWARE_I8259A_H
/*
    Copyright © 2022, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Intel 8259a "XT-PIC" definitions.
    Lang: english
*/

#define I8259A_IRQCOUNT         16

#define MASTER8259_CMDREG       0x20
#define MASTER8259_MASKREG      0x21
#define MASTER8259_ELCREG       0x4d0
#define SLAVE8259_CMDREG        0xA0
#define SLAVE8259_MASKREG       0xA1
#define SLAVE8259_ELCREG        0x4d1

#endif /* !HARDWARE_I8259A_H */
