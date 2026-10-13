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

/*****************************************************************************

    NAME */
        AROS_LH2(void, GPIOSet,

/*  SYNOPSIS */
                        AROS_LHA( unsigned int, pin, D0),
                        AROS_LHA( unsigned int, val, D1),

/*  LOCATION */
                        struct GPIOBase *, GPIOBase, 1, Gpio)
/*  FUNCTION
        Write the output latch for a GPIO pin.

    INPUTS
        pin - GPIO controller number, not a connector pin number. The
              legacy controller path accepts 0 through 53.
        val - Zero requests low; any nonzero value requests high.

    RESULT
        None. A pin above 53 is ignored. No hardware or range error is
        reported, and the requested level is not read back.

    NOTES
        Open gpio.resource before calling. The caller must coordinate use
        of the pin with other drivers; this resource does not reserve pins.
        Changing the latch does not select output mode or configure pulls.
        The resulting electrical level also depends on the pin's function
        and attached hardware.

        Call from task context where waiting is permitted. The write is
        protected by the resource semaphore; a sequence of GPIO calls is
        not one atomic operation. There is no pulse-width or timing guarantee.

    BUGS
        BCM2712/RP1 operation is not supported correctly in this version.
        The peripheral base is stored in a 32-bit field, so comparison with
        the BCM2712 base cannot select the RP1 path. GPIOSetFunc() describes
        the shared platform limitation. Do not use this call on BCM2712.

    SEE ALSO
        GPIOSetFunc(), GPIOGet(), GPIOSetPull()

*****************************************************************************/
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

/*****************************************************************************

    NAME */
        AROS_LH2(void, GPIOSetFunc,

/*  SYNOPSIS */
                        AROS_LHA( unsigned int, pin, D0),
                        AROS_LHA( unsigned int, func, D1),

/*  LOCATION */
                        struct GPIOBase *, GPIOBase, 2, Gpio)
/*  FUNCTION
        Select the hardware function assigned to a GPIO pin.

    INPUTS
        pin - GPIO controller number from 0 through 53 on the legacy path.
        func - Legacy function code: 0 input, 1 output, 4 ALT0, 5 ALT1,
               6 ALT2, 7 ALT3, 3 ALT4 or 2 ALT5. Only the low three bits
               are used. Select an alternate function valid for this pin.

    RESULT
        None. A pin above 53 is ignored. Unsupported function/pin
        combinations are not detected or reported.

    NOTES
        Open gpio.resource before calling. Function selection changes
        hardware routing and may disconnect a peripheral already using
        the pin. The caller must own or otherwise coordinate that pin;
        there is no resource-level pin allocation.

        The legacy path preserves the other pins in the shared function
        register by a semaphore-protected read-modify-write. It does not
        change the output latch or pull configuration. Call from task
        context where waiting is permitted. Direct register writers must
        be coordinated separately.

    BUGS
        The 32-bit saved peripheral base truncates the BCM2712 address
        before RP1 detection. All GPIO calls consequently take the wrong
        controller path on that platform. The dormant RP1 code also uses
        a different function model without translating these legacy codes.
        Its presence must not be taken as working BCM2712/RP1 support.

    SEE ALSO
        GPIOSet(), GPIOGet(), GPIOSetPull()

*****************************************************************************/
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
        unsigned int val;

        reg = GPFSEL0;
        while (pin >= 10) {
            pin -= 10;
            reg += 4;
        }

        pin += (pin << 1);

        ObtainSemaphore(&GPIOBase->gpio_Sem);

        /*
         * Read-modify-write: one GPFSEL selects the function of ten pins,
         * three bits each, so a plain store of this pin's field sets the
         * other nine to input.
         */
        val = AROS_LE2LONG(*(volatile unsigned int *)reg);
        val &= ~(7 << pin);
        val |= (func & 7) << pin;
        *(volatile unsigned int *)reg = AROS_LONG2LE(val);

        ReleaseSemaphore(&GPIOBase->gpio_Sem);
    }

    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH1(unsigned int, GPIOGet,

/*  SYNOPSIS */
                        AROS_LHA( unsigned int, pin, D0),

/*  LOCATION */
                        struct GPIOBase *, GPIOBase, 3, Gpio)
/*  FUNCTION
        Read the sampled level of a GPIO pin on the legacy controller.

    INPUTS
        pin - GPIO controller number from 0 through 53, not a connector
              pin number.

    RESULT
        1 for a high sampled level or 0 for a low sampled level. A pin
        above 53, or a missing peripheral base, also returns 0; the result
        does not distinguish an invalid request from a low level.

    NOTES
        Open gpio.resource before calling. The call does not select a
        function, reserve the pin, change its latch or configure its pulls.
        It performs a single register read without acquiring the resource
        semaphore. Concurrent function changes are not excluded. The value
        is a sample, not an edge count, debounced state or future guarantee.

    BUGS
        The BCM2712 peripheral base is truncated before RP1 detection.
        Do not use the returned value as a GPIO measurement on BCM2712.
        See GPIOSetFunc() for the platform limitation.

    SEE ALSO
        GPIOSet(), GPIOSetFunc(), GPIOSetPull()

*****************************************************************************/
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

/*****************************************************************************

    NAME */
        AROS_LH2(void, GPIOSetPull,

/*  SYNOPSIS */
                        AROS_LHA( unsigned int, pin, D0),
                        AROS_LHA( unsigned int, pull, D1),

/*  LOCATION */
                        struct GPIOBase *, GPIOBase, 4, Gpio)
/*  FUNCTION
        Configure the internal pull resistor using the BCM2711 register layout.

    INPUTS
        pin - GPIO controller number from 0 through 53.
        pull - 0 disables both pulls, 1 selects pull-up, and 2 selects
               pull-down on BCM2711. Other values also disable both pulls.

    RESULT
        None. A pin above 53 is ignored. The function does not report
        unsupported hardware or verify the resulting pin level.

    NOTES
        Open gpio.resource before calling. Coordinate pin ownership with
        other drivers. Pull selection does not change the output latch or
        function. On BCM2711, the function updates the selected two-bit
        field while preserving other pins in the same register.

        Call from task context where waiting is permitted. The BCM2711
        read-modify-write is protected by the resource semaphore.

    BUGS
        The legacy branch always uses the BCM2711 pull-register layout.
        It does not implement the GPPUD/GPPUDCLK sequence required by
        BCM2835-class controllers. Do not use it to configure their pulls.

        BCM2712/RP1 selection is broken as described in GPIOSetFunc(). The
        dormant RP1 branch also reverses the pull-up and pull-down bits and
        reads the pad register before acquiring the semaphore.

    SEE ALSO
        GPIOSetFunc(), GPIOSet(), GPIOGet()

*****************************************************************************/
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
