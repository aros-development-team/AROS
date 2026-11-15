/* Copyright (C) 2026, The AROS Development Team. All rights reserved. */
/******************************************************************************

    NAME
        I2CTouchpad

    SYNOPSIS
        DEVICE/K,RUN/S,CHECK/S,REPORT/K

    LOCATION
        C:

    FUNCTION
        Diagnose a supported HID-over-I2C pointer. With no switches, discover
        an ACPI PNP0C50/ACPI0C50 device without writing controller registers.
        DEVICE selects an explicit ACPI path instead of automatic discovery.
        CHECK inspects controller state without register writes. RUN polls
        pointer input until Ctrl-C, then attempts controller restoration.
        REPORT names an optional HID report descriptor capture file; it is
        overwritten during RUN. No descriptor file is written by default.

    NOTES
        CHECK and RUN require exclusive ownership of the controller. Reboot
        without i2chid.hidd before testing a controller owned by that HIDD.
        Do not run older RAM-file-lock versions of this command concurrently.
        Polling releases pressed buttons on exit; restoration cannot be
        guaranteed if the hardware stops responding. Firmware AML execution
        has no guaranteed deadline. See i2chid.hidd for supported hardware.

    EXAMPLE
        I2CTouchpad
        I2CTouchpad CHECK
        I2CTouchpad RUN REPORT RAM:touchpad-report.bin

    SEE ALSO
        i2chid.hidd

******************************************************************************/

#include <proto/dos.h>
#include "i2chid_driver.h"
int main(void)
{
    SIPTR args[4] = {0};
    struct RDArgs *rda = ReadArgs((CONST_STRPTR) "DEVICE/K,RUN/S,CHECK/S,REPORT/K", args, NULL);
    if (!rda)
    {
        PrintFault(IoErr(), (CONST_STRPTR) "I2CTouchpad");
        return RETURN_FAIL;
    }
    int result =
        ih_driver_run((const char *)args[0], !!args[1], !!args[2], NULL, (const char *)args[3]);
    FreeArgs(rda);
    return result;
}
