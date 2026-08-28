#ifndef AROS_IRQTYPES_H
#define AROS_IRQTYPES_H

/*
    Copyright © 2017-2022, The AROS Development Team. All rights reserved.
    $Id$

*/

#define IIC_ID_I8259A           (AROS_MAKE_ID('8','2','5','9'))
#define IIC_ID_APIC             (AROS_MAKE_ID('A','P','I','C'))
#define IIC_ID_IOAPIC           (AROS_MAKE_ID('I','O','9','3'))

#define IRQTYPE_STANDARD        (1 << 0)
#define IRQTYPE_APIC            (1 << 3)

#endif /* !AROS_IRQTYPES_H */
