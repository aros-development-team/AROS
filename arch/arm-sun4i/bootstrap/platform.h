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
    uint16_t speedbin;
};

extern const struct parameters_ddr3 platform_ddr3[];

#define PLATFORM_PCDUINO
#ifdef PLATFORM_PCDUINO

const struct parameters_ddr3 platform_ddr3[] = {
    { 800 },
    { 1066 },
    { 1333 },
    { 1666 },
    { 0 }
};

#endif

#endif
