/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Broadcom RNG back-end for entropy.resource on Raspberry Pi.

    The BCM2835 family provides a hardware random-number generator at
    peripheral offset 0x104000. BCM2711 and later put an RNG200 with a
    different register layout at that address, so the device tree decides
    whether this back-end drives it at all. Raw values are only folded into
    entropy.resource's ChaCha20 state; they are never exposed directly.
*/

#include <aros/macros.h>
#include <exec/types.h>
#include <proto/exec.h>
#include <proto/kernel.h>
#include <proto/openfirmware.h>

#include "entropy_intern.h"

#define BCM_RNG_COMPATIBLE      "brcm,bcm2835-rng"

#define BCM_RNG_OFFSET          0x104000
#define BCM_RNG_CTRL            0x00
#define BCM_RNG_STATUS          0x04
#define BCM_RNG_DATA            0x08
#define BCM_RNG_INT_MASK        0x10

#define BCM_RNG_CTRL_ENABLE     (1U << 0)
#define BCM_RNG_INT_DISABLE     (1U << 0)
#define BCM_RNG_STATUS_COUNT    0xff000000U
#define BCM_RNG_STATUS_SHIFT    24

/* Do not make GetEntropy() wait for hardware: the software source remains
   available whenever the FIFO is temporarily empty. */
#define BCM_RNG_POLLS           1024

/* KrnGetSystemAttr() is an interface of kernel.resource. */
APTR KernelBase __attribute__((used)) = NULL;

static BOOL bcm_rng_streq(CONST_STRPTR a, CONST_STRPTR b)
{
    while (*a && *a == *b)
    {
        a++;
        b++;
    }
    return (*a == *b);
}

/* The device tree says what really sits at 0x104000. An RNG200 answers there
   on BCM2711 and later, and a machine whose firmware disabled the node may
   not decode the address at all - QEMU's Pi 4 aborts the first access. Only
   drive the block whose register layout this file implements. */
static BOOL bcm_rng_present(void)
{
    void *OpenFirmwareBase = OpenResource("openfirmware.resource");
    void *key, *prop;
    CONST_STRPTR value;

    if (OpenFirmwareBase == NULL)
        return FALSE;

    /* Look the block up by its binding, not by a path: the unit address in
       a node name is a bus address and moves between SoC generations. The
       BCM2711 and later carry an RNG200 with different registers, and it
       does not answer to this name. */
    key = OF_FindNodeByCompatible(NULL, BCM_RNG_COMPATIBLE);
    if (key == NULL)
        return FALSE;

    prop = OF_FindProperty(key, "status");
    if (prop != NULL)
    {
        value = OF_GetPropValue(prop);
        if (value != NULL && !bcm_rng_streq(value, "okay") &&
            !bcm_rng_streq(value, "ok"))
            return FALSE;
    }

    return TRUE;
}

static ULONG bcm_rng_read(volatile UBYTE *base, ULONG offset)
{
    return AROS_LE2LONG(*(volatile ULONG *)(base + offset));
}

static void bcm_rng_write(volatile UBYTE *base, ULONG offset, ULONG value)
{
    *(volatile ULONG *)(base + offset) = AROS_LONG2LE(value);
}

static ULONG bcm_rng_gather(struct EntropyBase *EntropyBase,
                            UBYTE *buffer, ULONG length)
{
    volatile UBYTE *base = (volatile UBYTE *)EntropyBase->eb_HWData;
    ULONG got = 0;
    ULONG polls = BCM_RNG_POLLS;

    while (got < length && polls--)
    {
        ULONG available = (bcm_rng_read(base, BCM_RNG_STATUS) &
                           BCM_RNG_STATUS_COUNT) >> BCM_RNG_STATUS_SHIFT;

        while (available-- && got < length)
        {
            ULONG value = bcm_rng_read(base, BCM_RNG_DATA);
            ULONG take = length - got;
            ULONG i;

            if (take > sizeof(value))
                take = sizeof(value);

            for (i = 0; i < take; i++)
                buffer[got + i] = (UBYTE)(value >> (i * 8));
            got += take;
        }
    }

    return got;
}

void Entropy_HW_Init(struct EntropyBase *EntropyBase)
{
    IPTR peripheral_base;
    volatile UBYTE *base;
    UBYTE probe[sizeof(ULONG)];

    EntropyBase->eb_HWGather = NULL;
    EntropyBase->eb_HWData = NULL;
    EntropyBase->eb_Flags |= EIF_SOFTWARE;

    KernelBase = OpenResource("kernel.resource");
    if (KernelBase == NULL)
        return;

    peripheral_base = KrnGetSystemAttr(KATTR_PeripheralBase);
    if (peripheral_base == 0 || peripheral_base == -1)
        return;

    if (!bcm_rng_present())
        return;

    base = (volatile UBYTE *)peripheral_base + BCM_RNG_OFFSET;

    /* Keep the RNG interrupt masked: this resource polls the small FIFO. */
    bcm_rng_write(base, BCM_RNG_INT_MASK, BCM_RNG_INT_DISABLE);
    bcm_rng_write(base, BCM_RNG_CTRL,
                  bcm_rng_read(base, BCM_RNG_CTRL) | BCM_RNG_CTRL_ENABLE);

    /* Confirm that the block produces data before advertising it. */
    EntropyBase->eb_HWData = (APTR)base;
    if (bcm_rng_gather(EntropyBase, probe, sizeof(probe)) == 0)
    {
        EntropyBase->eb_HWData = NULL;
        Entropy_Wipe(probe, sizeof(probe));
        return;
    }

    Entropy_Wipe(probe, sizeof(probe));
    EntropyBase->eb_HWGather = bcm_rng_gather;
    EntropyBase->eb_Flags |= EIF_HARDWARE;
}
