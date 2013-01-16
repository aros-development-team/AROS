/*
    Copyright © 2013, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef RASPI_GPIO_H
#define RASPI_GPIO_H

#include <exec/types.h>
#include <stdint.h>

#define RASPIGPIO_PHYSBASE (IPTR)0x20200000

enum {
    GPIOinput = 0,
    GPIOoutput,
    GPIOalt5,
    GPIOalt4,
    GPIOalt0,
    GPIOalt1,
    GPIOalt2,
    GPIOalt3,
    GPIOmask = 7
};

#define GPIOfsel(n,function) \
    function<<(3*(n%10))

enum {
    GPIOpullnone = 0,
    GPIOpulldown,
    GPIOpullup,
    GPIOpullmask
};

struct raspigpio {
    uint32_t gpfsel0;
    uint32_t gpfsel1;
    uint32_t gpfsel2;
    uint32_t gpfsel3;
    uint32_t gpfsel4;
    uint32_t gpfsel5;
    uint32_t    padding1;
    uint32_t gpset0;
    uint32_t gpset1;
    uint32_t    padding2;
    uint32_t gpclr0;
    uint32_t gpclr1;
    uint32_t    padding3;
    uint32_t gplev0;
    uint32_t gplev1;
    uint32_t    padding4;
    uint32_t gpeds0;
    uint32_t gpeds1;
    uint32_t    padding5;
    uint32_t gpren0;
    uint32_t gpren1;
    uint32_t    padding6;
    uint32_t gpfen0;
    uint32_t gpfen1;
    uint32_t    padding7;
    uint32_t gphen0;
    uint32_t gphen1;
    uint32_t    padding8;
    uint32_t gplen0;
    uint32_t gplen1;
    uint32_t    padding9;
    uint32_t gparen0;
    uint32_t gparen1;
    uint32_t    padding10;
    uint32_t gpafen0;
    uint32_t gpafen1;
    uint32_t    padding11;
    uint32_t gppud;
    uint32_t gppudclk0;
    uint32_t gppudclk1;
}__attribute__((packed));;

#endif /* RASPI_GPIO */
