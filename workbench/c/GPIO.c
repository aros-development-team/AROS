/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
    Author: Fabian Schmieder (@metaneutrons)

    Desc: Universal GPIO CLI command for AROS (C:GPIO).
*/

/******************************************************************************

    NAME

        GPIO

    SYNOPSIS

        PIN/A/N,SET/N,GET/S,MODE/K,PULL/K

    LOCATION

        C:

    FUNCTION

        Controls and queries hardware GPIO pins via gpio.resource.

    INPUTS

        PIN   -- (Required) Target GPIO pin number (e.g. 12).
        SET   -- Set pin output value to 0 (LOW) or 1 (HIGH).
        GET   -- Read and print current digital input level (0 or 1).
        MODE  -- Configure pin mode: IN (Input), OUT (Output), ALT (Alternate).
        PULL  -- Configure pull resistor: NONE, UP, DOWN.

    EXAMPLES

        GPIO 12 MODE OUT
        GPIO 12 SET 1
        GPIO 12 GET
        GPIO 13 PULL UP

******************************************************************************/

#include <exec/types.h>
#include <dos/dos.h>
#include <dos/rdargs.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <aros/libcall.h>
#include <string.h>

#define GPIOSet(pin, val) \
    AROS_LC2NR(void, GPIOSet, \
        AROS_LCA(unsigned int, (pin), D0), \
        AROS_LCA(unsigned int, (val), D1), \
        struct Library *, GPIOBase, 1, Gpio)

#define GPIOSetFunc(pin, val) \
    AROS_LC2NR(void, GPIOSetFunc, \
        AROS_LCA(unsigned int, (pin), D0), \
        AROS_LCA(unsigned int, (val), D1), \
        struct Library *, GPIOBase, 2, Gpio)

#define GPIOGet(pin) \
    AROS_LC1(unsigned int, GPIOGet, \
        AROS_LCA(unsigned int, (pin), D0), \
        struct Library *, GPIOBase, 3, Gpio)

#define GPIOSetPull(pin, pull) \
    AROS_LC2NR(void, GPIOSetPull, \
        AROS_LCA(unsigned int, (pin), D0), \
        AROS_LCA(unsigned int, (pull), D1), \
        struct Library *, GPIOBase, 4, Gpio)

static const char version_tag[] = "$VER: GPIO 0.1 (19.08.2026) by Fabian Schmieder\r\n";

#define TEMPLATE "PIN/A/N,SET/N,GET/S,MODE/K,PULL/K"

enum {
    ARG_PIN,
    ARG_SET,
    ARG_GET,
    ARG_MODE,
    ARG_PULL,
    NUM_ARGS
};

int main(int argc, char **argv)
{
    struct RDArgs *rdargs;
    IPTR args[NUM_ARGS] = {0};
    LONG ret = RETURN_OK;

    rdargs = ReadArgs(TEMPLATE, args, NULL);
    if (!rdargs) {
        PrintFault(IoErr(), "GPIO");
        return RETURN_ERROR;
    }

    struct Library *GPIOBase = (struct Library *)OpenResource("gpio.resource");
    if (!GPIOBase) {
        PutStr("ERROR: gpio.resource is not available on this platform.\n");
        FreeArgs(rdargs);
        return RETURN_FAIL;
    }

    ULONG pin = *(LONG *)args[ARG_PIN];

    /* Mode configuration */
    if (args[ARG_MODE]) {
        CONST_STRPTR mode_str = (CONST_STRPTR)args[ARG_MODE];
        ULONG mode = 0; /* GPIO_FUNC_INPUT */
        if (mode_str[0] == 'O' || mode_str[0] == 'o') mode = 1; /* GPIO_FUNC_OUTPUT */
        else if (mode_str[0] == 'A' || mode_str[0] == 'a') mode = 2; /* GPIO_FUNC_ALT */

        GPIOSetFunc(pin, mode);
    }

    /* Pull configuration */
    if (args[ARG_PULL]) {
        CONST_STRPTR pull_str = (CONST_STRPTR)args[ARG_PULL];
        ULONG pull = 0; /* NONE */
        if (pull_str[0] == 'U' || pull_str[0] == 'u') pull = 1; /* UP */
        else if (pull_str[0] == 'D' || pull_str[0] == 'd') pull = 2; /* DOWN */

        GPIOSetPull(pin, pull);
    }

    /* Set level */
    if (args[ARG_SET]) {
        LONG val = *(LONG *)args[ARG_SET];
        GPIOSet(pin, (val != 0) ? 1 : 0);
    }

    /* Get level */
    if (args[ARG_GET] || (!args[ARG_SET] && !args[ARG_MODE] && !args[ARG_PULL])) {
        ULONG val = GPIOGet(pin);
        if (val) PutStr("1\n");
        else PutStr("0\n");
    }

    FreeArgs(rdargs);
    return ret;
}
