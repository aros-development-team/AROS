/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "coolimages.h"

#define FLAGIMAGE_WIDTH     13
#define FLAGIMAGE_HEIGHT    15
#define FLAGIMAGE_COLORS    5

static const UBYTE flagimage_data[] =
{
	00,00,01,01,01,00,00,00,00,00,00,00,00,
	00,01,02,02,02,01,00,00,00,00,00,00,00,
	01,02,02,02,02,03,01,00,00,00,00,00,01,
	01,02,02,02,02,03,04,01,00,00,00,01,01,
	01,02,02,02,02,03,04,04,01,01,01,04,01,
	01,02,02,02,02,03,04,04,03,02,03,04,01,
	01,02,02,02,02,03,04,04,03,02,03,04,01,
	01,02,02,02,02,03,04,04,03,02,03,04,01,
	01,02,02,02,02,03,04,04,03,02,03,04,01,
	01,02,02,02,02,03,04,04,03,02,03,04,01,
	01,02,01,01,01,03,04,04,03,02,03,04,01,
	01,01,00,00,00,01,04,04,03,02,03,04,01,
	01,00,00,00,00,00,01,04,03,02,03,04,01,
	00,00,00,00,00,00,00,01,03,02,03,01,00,
	00,00,00,00,00,00,00,00,01,01,01,00,00
};

static const UBYTE flagimage_pal[] =
{
	0xb3,0xb3,0xb3,0x00,0x00,0x00,
	0xdf,0xdf,0xfa,0xb8,0xb8,0xf3,
	0x63,0x63,0x00
};

const struct CoolImage cool_flagimage =
{
	flagimage_data,
	flagimage_pal,
	FLAGIMAGE_WIDTH,
	FLAGIMAGE_HEIGHT,
	FLAGIMAGE_COLORS
};

