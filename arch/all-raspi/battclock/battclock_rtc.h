/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Raspberry Pi hardware RTC via the VideoCore mailbox.
          Not in battclock_intern.h, which rom/battclock also includes.
*/

#ifndef BATTCLOCK_RTC_H
#define BATTCLOCK_RTC_H

#include <aros/macros.h>
#include <exec/types.h>
#include <exec/memory.h>
#include <proto/exec.h>
#include <proto/kernel.h>
#include <proto/mbox.h>
#include <proto/openfirmware.h>

#include <hardware/bcm2708.h>
#include <hardware/videocore.h>

/* Pi 5 RTC is reachable only via firmware tags; value = { register, data }. */
#define PROPTAG_GET_RTC                 0x00030087UL
#define PROPTAG_SET_RTC                 0x00038087UL
#define RPI_RTC_REG_TIME                0       /* selector: POSIX seconds */
#define RPI_RTC_COMPATIBLE              "raspberrypi,rpi-rtc"

/* BCM2712 moved the mailbox within the peripheral window. */
#define VCMB_OFFSET_BCM2712             0x013880
#define RPI_VCMB_BASE(peri)             ((void *)(IPTR)((peri) + \
    (((peri) == BCM2712_PERIIOBASE) ? VCMB_OFFSET_BCM2712 : VCMB_OFFSET)))

static inline BOOL rpi_rtc_streq(CONST_STRPTR a, CONST_STRPTR b)
{
    while (*a && *a == *b)
    {
        a++;
        b++;
    }
    return (*a == *b);
}

/* A DT node with no status, or status "okay", is enabled. */
static inline BOOL rpi_of_node_ok(APTR OpenFirmwareBase, void *key)
{
    void *prop = OF_FindProperty(key, "status");
    CONST_STRPTR status;

    if (prop)
    {
        status = OF_GetPropValue(prop);
        if (status && !rpi_rtc_streq(status, "okay") &&
            !rpi_rtc_streq(status, "ok"))
            return FALSE;
    }

    return TRUE;
}

static inline BOOL rpi_rtc_present(void)
{
    APTR OpenFirmwareBase = OpenResource("openfirmware.resource");
    void *key;

    if (!OpenFirmwareBase)
        return FALSE;

    key = OF_FindNodeByCompatible(NULL, RPI_RTC_COMPATIBLE);
    return key && rpi_of_node_ok(OpenFirmwareBase, key);
}

/*
 * FALSE if there is no RTC or the firmware did not respond.
 * The message must own its cache line (see <proto/mbox.h>).
 */
static inline int rpi_rtc_reg(APTR KernelBase, APTR MBoxBase,
                              ULONG tag, ULONG reg, ULONG *value)
{
    IPTR peri = (IPTR)KrnGetSystemAttr(KATTR_PeripheralBase);
    unsigned int *raw, *msg;
    int ok = FALSE;

    if (!peri || !rpi_rtc_present())
        return FALSE;

    raw = AllocMem(MBOX_MSG_ALIGN + (MBOX_MSG_ALIGN - 1), MEMF_PUBLIC | MEMF_CLEAR);
    if (!raw)
        return FALSE;
    msg = (unsigned int *)((((IPTR)raw) + (MBOX_MSG_ALIGN - 1)) & ~(IPTR)(MBOX_MSG_ALIGN - 1));

    msg[0] = AROS_LONG2LE(8 * sizeof(unsigned int));
    msg[1] = AROS_LONG2LE(VCTAG_REQ);
    msg[2] = AROS_LONG2LE(tag);
    msg[3] = AROS_LONG2LE(2 * sizeof(unsigned int));    /* value buffer size */
    msg[4] = 0;                                         /* req/resp code */
    msg[5] = AROS_LONG2LE(reg);
    msg[6] = AROS_LONG2LE(*value);
    msg[7] = 0;                                         /* end tag */

    if (MBoxCall(RPI_VCMB_BASE(peri), VCMB_PROPCHAN, msg) == (volatile unsigned int *)msg &&
        (AROS_LE2LONG(msg[4]) & VCTAG_RESP))
    {
        *value = AROS_LE2LONG(msg[6]);
        ok = TRUE;
    }

    FreeMem(raw, MBOX_MSG_ALIGN + (MBOX_MSG_ALIGN - 1));
    return ok;
}

#endif /* BATTCLOCK_RTC_H */
