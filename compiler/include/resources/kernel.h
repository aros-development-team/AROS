/*
    Copyright © 2020, The AROS Development Team. All rights reserved.
    $Id$

    Desc: AROS kernel resource definitions.
    Lang: english
*/

#ifndef RESOURCES_KERNEL_H
#define RESOURCES_KERNEL_H

#include <utility/tagitem.h>

#define KERNEL_TAG_BASE	                (TAG_USER + 0x0ABA0000)

#define KERNELTAG_IRQ_AFFINITY	        (KERNEL_TAG_BASE + 0x00000001)
#define KERNELTAG_IRQ_POLARITY	        (KERNEL_TAG_BASE + 0x00000002)
#define KERNELTAG_IRQ_TRIGGERLEVEL      (KERNEL_TAG_BASE + 0x00000003)

#endif /* !RESOURCES_KERNEL_H */
