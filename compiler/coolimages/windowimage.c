/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "coolimages.h"

#define WINDOWIMAGE_WIDTH     15
#define WINDOWIMAGE_HEIGHT    15
#define WINDOWIMAGE_COLORS    16

static const UBYTE windowimage_data[] =
{
        15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,
        15,1, 1, 15,1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 15,
        15,1, 1, 15,1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 15,
        15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,
        15,14,13,13,13,12,11,11,10,9, 9, 8, 8, 7, 15,
        15,13,13,13,12,12,11,10,10,9, 8, 8, 7, 6, 15,
        15,13,13,12,12,11,11,10,9, 9, 8, 7, 7, 6, 15,
        15,13,12,12,12,11,10,9, 9, 8, 8, 7, 6, 5, 15,
        15,13,12,12,11,10,10,9, 8, 8, 7, 6, 6, 5, 15,
        15,12,12,11,11,10,9, 9, 8, 7, 7, 6, 5, 4, 15,
        15,12,12,11,10,10,9, 8, 8, 7, 6, 5, 4, 4, 15,
        15,12,11,10,10,9, 8, 8, 7, 6, 6, 5, 4, 3, 15,
        15,11,11,10,9, 9, 8, 7, 7, 6, 5, 4, 4, 3, 15,
        15,11,10,10,9, 8, 8, 7, 6, 5, 5, 4, 3, 2, 15,
        15,15,15,15,15,15,15,15,15,15,15,15,15,15,15
};

static const UBYTE windowimage_pal[] =
{
    0x00, 0x00, 0x00,
    0x77, 0x77, 0xff,
    0x11, 0x11, 0x11,
    0x22, 0x22, 0x22,
    0x33, 0x33, 0x33,
    0x44, 0x44, 0x44,
    0x55, 0x55, 0x55,
    0x66, 0x66, 0x66,
    0x77, 0x77, 0x77,
    0x88, 0x88, 0x88,
    0x99, 0x99, 0x99,
    0xaa, 0xaa, 0xaa,
    0xbb, 0xbb, 0xbb,
    0xcc, 0xcc, 0xcc,
    0xdd, 0xdd, 0xdd,
    0x00, 0x00, 0x00
};

const struct CoolImage cool_windowimage =
{
	windowimage_data,
	windowimage_pal,
	WINDOWIMAGE_WIDTH,
	WINDOWIMAGE_HEIGHT,
	WINDOWIMAGE_COLORS
};

