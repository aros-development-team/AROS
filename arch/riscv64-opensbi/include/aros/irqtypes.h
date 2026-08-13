#ifndef AROS_IRQTYPES_H
#define AROS_IRQTYPES_H

/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
    $Id$

*/

#define IIC_ID_PLIC             (AROS_MAKE_ID('P','L','I','C'))

/* Sources the platform's interrupt controller drives itself */
#define IRQTYPE_STANDARD        (1 << 0)

/*
 * Sources behind a controller that collects several interrupts onto
 * one of the platform's, as the message controller in a DesignWare
 * PCIe bridge does. They are numbers like any other - a driver adds
 * handlers to them in the usual way - but nothing raises them until
 * the collecting source is served and its status register says which
 * of them arrived.
 */
#define IRQTYPE_MSI             (1 << 1)

#endif /* !AROS_IRQTYPES_H */
