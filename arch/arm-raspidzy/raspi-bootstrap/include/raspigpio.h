/*
    Copyright © 2013, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef RASPI_GPIO_H
#define RASPI_GPIO_H

#include <exec/types.h>
#include <stdint.h>

#define RASPIGPIO_PHYSBASE (IPTR)0x20200000

struct raspigpio {
    uint32_t gpfsel0;
    uint32_t gpfsel1;
    uint32_t gpfsel2;
    uint32_t gpfsel3;
    uint32_t gpfsel4;
    uint32_t gpfsel5;
    uint32_t padding1;
    uint32_t gpset0;
    uint32_t gpset1;
    uint32_t padding2;
    uint32_t gpclr0;
    uint32_t gpclr1;
    uint32_t padding3;
    uint32_t gplev0;
    uint32_t gplev1;
    uint32_t padding4;
    uint32_t gpeds0;
    uint32_t gpeds1;
    uint32_t padding5;
    uint32_t gpren0;
    uint32_t gpren1;

}__attribute__((packed));;

#endif /* RASPI_GPIO */
