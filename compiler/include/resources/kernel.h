/*
    Copyright (C) 2020-2026, The AROS Development Team. All rights reserved.

    Desc: AROS kernel resource definitions.
*/

#ifndef RESOURCES_KERNEL_H
#define RESOURCES_KERNEL_H

#include <utility/tagitem.h>

#define KERNEL_TAG_BASE	                (TAG_USER + 0x0ABA0000)

#define KERNELTAG_IRQ_AFFINITY	        (KERNEL_TAG_BASE + 0x00000001)
#define KERNELTAG_IRQ_POLARITY	        (KERNEL_TAG_BASE + 0x00000002)
#define KERNELTAG_IRQ_TRIGGERLEVEL      (KERNEL_TAG_BASE + 0x00000003)

/*
 * A controller that collects interrupts onto the source being
 * modified, as the message controller of a PCIe bridge does. Serving
 * that source then fans out to the sources its vectors were given.
 */
#define KERNELTAG_IRQ_MSISTATUS         (KERNEL_TAG_BASE + 0x00000004)
#define KERNELTAG_IRQ_MSIBASE           (KERNEL_TAG_BASE + 0x00000005)
#define KERNELTAG_IRQ_MSICOUNT          (KERNEL_TAG_BASE + 0x00000006)

#endif /* !RESOURCES_KERNEL_H */
