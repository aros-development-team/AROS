/*
    Copyright © 2014, The AROS Development Team. All rights reserved.
    $Id$

    Desc: sun4i platform specific definitions
    Lang: english
*/

#ifndef PLATFORM_SUN4I
#define PLATFORM_SUN4I

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

#ifndef _INTTYPES_H
#include <inttypes.h>
#endif

struct parameters_ddr3 {
    uint32_t tAA;
    uint32_t tRCD;
    uint32_t tRP;
    uint32_t tRC;
    uint32_t tRAS;
    uint32_t tRFC;
    uint32_t tREFI;
};

extern const struct parameters_ddr3 platform_ddr3[];

#define PLATFORM_PCDUINO
#ifdef PLATFORM_PCDUINO

/*
* Take some value that fits the scale and is easily computable with integers
* These are not exact values as timings are computed with integers, calculating the result back gives a bit different value
*/
const struct parameters_ddr3 platform_ddr3[] = {
    14,  /* tAA   13.125ns-15.000ns */
    14,  /* tRCD  13.125ns-15.000ns */
    14,  /* tRP   13.125ns-15.000ns */
    51,  /* tRC   49.500ns-52.500ns */
    37,  /* tRAS  36.000ns-37.500ns */
    161, /* tRFC  160.000ns-161.336ns */
    7800 /* tREFI 7800ns */
};

#endif

#endif
