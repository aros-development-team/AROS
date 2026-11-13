/*
 * Copyright (C) 2011, The AROS Development Team.  All rights reserved.
 * Author: Jason S. McMullan <jason.mcmullan@gmail.com>
 *
 * Licensed under the AROS PUBLIC LICENSE (APL) Version 1.1
 *
 * Data for the test icon
 */

#include <proto/icon.h>
#include <proto/graphics.h>
#include <proto/intuition.h>

#define test_depth     3
#define test_palsize   8
#define test_width    15
#define test_height   17
#define test_transcolor 4

const struct ColorRegister test_pal[] = {
    { 0xff, 0xff, 0xff },
    { 0x00, 0x00, 0x00 },
    { 0x00, 0x00, 0xff },
    { 0x00, 0xff, 0x00 },
    { 0x00, 0xff, 0xff },
    { 0xff, 0x00, 0x00 },
    { 0xff, 0x00, 0xff },
    { 0xff, 0xff, 0x00 },
};

const struct ColorSpec test_colorspec[] = {
    { 0, 0xffff, 0xffff, 0xffff },
    { 1, 0x0000, 0x0000, 0x0000 },
    { 2, 0x0000, 0x0000, 0xffff },
    { 3, 0x0000, 0xffff, 0x0000 },
    { 4, 0x0000, 0xffff, 0xffff },
    { 5, 0xffff, 0x0000, 0x0000 },
    { 6, 0xffff, 0x0000, 0xffff },
    { 7, 0xffff, 0xffff, 0x0000 },
    { -1 },
};

/* NOTE: The first line should be 0..N, where N is the
 *       size of the palette. The read-back test uses
 *       that to identify the screen's icon to pen mapping.
 */
const UBYTE test_img[test_height * test_width] = {
    0, 1, 2, 3, 4, 5, 6, 7, 0, 0, 0, 0, 1, 1, 1,

    1, 1, 2, 0, 0, 0, 0, 0, 4, 4, 4, 4, 4, 0, 1,

    1, 2, 1, 2, 3, 0, 0, 0, 4, 4, 0, 4, 4, 0, 1,

    0, 3, 2, 1, 2, 3, 0, 0, 4, 4, 4, 4, 4, 0, 0,

    0, 4, 3, 2, 1, 3, 4, 0, 0, 0, 0, 0, 0, 0, 0,

    0, 5, 4, 3, 2, 1, 2, 3, 4, 5, 0, 0, 0, 0, 0,

    0, 6, 5, 4, 3, 2, 1, 2, 3, 4, 5, 6, 0, 0, 0,

    0, 7, 6, 5, 4, 3, 2, 1, 2, 3, 4, 5, 6, 7, 0,

    0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0,

    0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0,

    0, 7, 7, 7, 7, 7, 7, 7, 7, 7, 1, 0, 0, 0, 0,

    0, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 1, 0, 0, 0,

    0, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 1, 0, 0,

    0, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 1, 0,

    1, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 1,

    1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1,
 
    1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1,
};

const LONG test_type = WBPROJECT;

CONST_STRPTR test_defaulttool = "diskobj";

CONST_STRPTR test_tooltypes[] = {
    "BUNNY=FooFoo",
    "EAGLE=Nest",
    NULL
};
