/*
    Copyright (C) 2015-2026, The AROS Development Team. All rights reserved.
*/

#define DEBUG 0

#include <aros/debug.h>
#include <aros/macros.h>
#include <aros/symbolsets.h>
#include <aros/libcall.h>
#include <proto/kernel.h>
#include <proto/exec.h>
#include <proto/gpio.h>

#include "gpio_private.h"

APTR KernelBase __attribute__((used)) = NULL;

static int gpio_init(struct GPIOBase *GPIOBase)
{
    int retval = FALSE;

    D(bug("[GPIO] %s()\n", __PRETTY_FUNCTION__));

    KernelBase = OpenResource("kernel.resource");

    if ((GPIOBase->gpio_periiobase = KrnGetSystemAttr(KATTR_PeripheralBase)) != 0)
    {
        InitSemaphore(&GPIOBase->gpio_Sem);
        GPIOBase->gpio_rp1_bar1 = 0;

        if (GPIOBase->gpio_periiobase == BCM2712_PERIIOBASE)
        {
            GPIOBase->gpio_rp1_bar1 = 0x1F00000000ULL;
        }

        D(bug("[GPIO] %s: Initialised Semaphore @ 0x%p\n", __PRETTY_FUNCTION__, &GPIOBase->gpio_Sem));

        retval = TRUE;
    }
    return retval;
}

AROS_LH2(void, GPIOSet,
                AROS_LHA( unsigned int, pin, D0),
                AROS_LHA( unsigned int, val, D1),
                struct GPIOBase *, GPIOBase, 1, Gpio)
{
    AROS_LIBFUNC_INIT

    volatile unsigned int reg;
    unsigned int bit;

    D(bug("[GPIO] %s(#%d,%d)\n", __PRETTY_FUNCTION__, pin, val));

    if (GPIOBase->gpio_rp1_bar1 && (pin <= 27))
    {
        /* RP1 SYS_RIO0: SET=0x04, CLR=0x08 */
        IPTR rio_base = GPIOBase->gpio_rp1_bar1 + 0x0E0000;
        reg = val ? (rio_base + 0x04) : (rio_base + 0x08);
        bit = 1 << pin;

        ObtainSemaphore(&GPIOBase->gpio_Sem);
        *(volatile unsigned int *)reg = AROS_LONG2LE(bit);
        ReleaseSemaphore(&GPIOBase->gpio_Sem);
    }
    else if (GPIOBase->gpio_periiobase && (pin <= 53))
    {
        reg = val? GPSET0: GPCLR0;
        reg += (pin >> 5) << 2; /* (pin / 32) << 2 */

        bit = 1 << (pin & 0x1F);

        ObtainSemaphore(&GPIOBase->gpio_Sem);

        *(volatile unsigned int *)reg = AROS_LONG2LE(bit);

        ReleaseSemaphore(&GPIOBase->gpio_Sem);
    }

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, GPIOSetFunc,
                AROS_LHA( unsigned int, pin, D0),
                AROS_LHA( unsigned int, func, D1),
                struct GPIOBase *, GPIOBase, 2, Gpio)
{
    AROS_LIBFUNC_INIT

    volatile unsigned int reg;

    D(bug("[GPIO] %s(#%d,%d)\n", __PRETTY_FUNCTION__, pin, func));

    if (GPIOBase->gpio_rp1_bar1 && (pin <= 27))
    {
        /* RP1 IO_BANK0 pin ctrl: offset (pin * 8) + 4; SYS_RIO0 offset 0x0E0000 */
        IPTR bank0_ctrl = GPIOBase->gpio_rp1_bar1 + 0x0D0000 + (pin * 8) + 4;
        IPTR rio_base = GPIOBase->gpio_rp1_bar1 + 0x0E0000;

        ObtainSemaphore(&GPIOBase->gpio_Sem);

        *(volatile unsigned int *)bank0_ctrl = AROS_LONG2LE(func & 0x1F);

        if (func == 1)
            *(volatile unsigned int *)(rio_base + 0x14) = AROS_LONG2LE(1 << pin); /* RIO_OE_SET */
        else if (func == 0)
            *(volatile unsigned int *)(rio_base + 0x18) = AROS_LONG2LE(1 << pin); /* RIO_OE_CLR */

        ReleaseSemaphore(&GPIOBase->gpio_Sem);
    }
    else if (GPIOBase->gpio_periiobase && (pin <= 53))
    {
        reg = GPFSEL0;
        while (pin >= 10) {
            pin -= 10;
            reg += 4;
        }

        pin += (pin << 1);
        func <<= pin;

        ObtainSemaphore(&GPIOBase->gpio_Sem);

        *(volatile unsigned int *)reg = AROS_LONG2LE(func);

        ReleaseSemaphore(&GPIOBase->gpio_Sem);
    }

    AROS_LIBFUNC_EXIT
}

AROS_LH1(unsigned int, GPIOGet,
                AROS_LHA( unsigned int, pin, D0),
                struct GPIOBase *, GPIOBase, 3, Gpio)
{
    AROS_LIBFUNC_INIT

    unsigned int val = 0;

    D(bug("[GPIO] %s(#%d)\n", __PRETTY_FUNCTION__, pin));

    if (GPIOBase->gpio_rp1_bar1 && (pin <= 27))
    {
        /* RP1 SYS_RIO0: IN is at offset 0x00 */
        IPTR rio_in = GPIOBase->gpio_rp1_bar1 + 0x0E0000;
        unsigned int reg_val = AROS_LE2LONG(*(volatile unsigned int *)rio_in);
        val = (reg_val >> pin) & 1;
    }
    else if (GPIOBase->gpio_periiobase && (pin <= 53))
    {
        IPTR reg = GPLEV0 + ((pin >> 5) << 2);
        unsigned int reg_val = AROS_LE2LONG(*(volatile unsigned int *)reg);
        val = (reg_val >> (pin & 0x1F)) & 1;
    }

    return val;

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, GPIOSetPull,
                AROS_LHA( unsigned int, pin, D0),
                AROS_LHA( unsigned int, pull, D1),
                struct GPIOBase *, GPIOBase, 4, Gpio)
{
    AROS_LIBFUNC_INIT

    D(bug("[GPIO] %s(#%d,%d)\n", __PRETTY_FUNCTION__, pin, pull));

    if (GPIOBase->gpio_rp1_bar1 && (pin <= 27))
    {
        /* RP1 PADS_BANK0: offset 0x0F0000 + (pin * 4) + 4 */
        IPTR pad_reg = GPIOBase->gpio_rp1_bar1 + 0x0F0000 + (pin * 4) + 4;
        unsigned int val = AROS_LE2LONG(*(volatile unsigned int *)pad_reg);

        /* Bit 2 = PullUp (PUE), Bit 3 = PullDown (PDE) */
        val &= ~(0x0C);
        if (pull == 1) val |= (1 << 2);      /* Pull-Up */
        else if (pull == 2) val |= (1 << 3); /* Pull-Down */

        ObtainSemaphore(&GPIOBase->gpio_Sem);
        *(volatile unsigned int *)pad_reg = AROS_LONG2LE(val);
        ReleaseSemaphore(&GPIOBase->gpio_Sem);
    }
    else if (GPIOBase->gpio_periiobase && (pin <= 53))
    {
        /* BCM2711 / BCM2835 Pull configuration */
        IPTR gppup_reg = GPIO_BASE + 0xE4 + ((pin >> 4) << 2); /* GPPUPPDN0..3 on BCM2711 */
        unsigned int shift = (pin & 0x0F) << 1;
        unsigned int pull_val = (pull == 1) ? 1 : ((pull == 2) ? 2 : 0);

        ObtainSemaphore(&GPIOBase->gpio_Sem);
        unsigned int cur = AROS_LE2LONG(*(volatile unsigned int *)gppup_reg);
        cur &= ~(3 << shift);
        cur |= (pull_val << shift);
        *(volatile unsigned int *)gppup_reg = AROS_LONG2LE(cur);
        ReleaseSemaphore(&GPIOBase->gpio_Sem);
    }

    AROS_LIBFUNC_EXIT
}

ADD2INITLIB(gpio_init, 0)
