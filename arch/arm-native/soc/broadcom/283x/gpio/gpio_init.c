/*
    Copyright © 2015, The AROS Development Team. All rights reserved.
    $Id$
*/

#define DEBUG 0

#include <aros/debug.h>
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

    if (GPIOBase->gpio_periiobase && (pin <= 53))
    { 
        reg = val? GPSET0: GPCLR0;
        reg += (pin >> 5) << 2; /* (pin / 32) << 2 */

        bit = 1 << (pin & 0x1F);

        ObtainSemaphore(&GPIOBase->gpio_Sem);

        *(volatile unsigned int *)reg = bit;

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

    if (GPIOBase->gpio_periiobase && (pin <= 53))
    {
        reg = GPFSEL0;
        while (pin >= 10) {
            pin -= 10;
            reg += 4;
        }

        pin += (pin << 1);
        func <<= pin;

        ObtainSemaphore(&GPIOBase->gpio_Sem);

        *(volatile unsigned int *)reg = func;

        ReleaseSemaphore(&GPIOBase->gpio_Sem);
    }

    AROS_LIBFUNC_EXIT
}

ADD2INITLIB(gpio_init, 0)
