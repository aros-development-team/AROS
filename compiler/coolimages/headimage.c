/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "coolimages.h"

#define HEADIMAGE_WIDTH     13
#define HEADIMAGE_HEIGHT    15
#define HEADIMAGE_COLORS    7

static const UBYTE headimage_data[] =
{
	00,00,00,00,01,01,01,01,01,01,00,00,00,
	00,00,00,01,02,02,02,02,02,02,01,00,00,
	00,00,01,02,02,02,02,02,02,02,02,01,00,
	00,01,02,02,02,02,02,02,02,02,02,02,01,
	00,01,02,02,02,02,02,02,03,03,03,01,00,
	01,02,02,02,02,02,03,03,03,03,03,01,00,
	01,02,02,02,02,02,03,03,04,01,03,05,01,
	01,02,02,02,03,02,03,03,05,05,03,06,01,
	01,02,02,05,03,03,03,03,03,03,03,03,01,
	01,02,02,05,05,03,03,03,03,03,03,03,01,
	01,02,02,03,03,03,03,03,03,03,05,05,01,
	01,01,01,05,05,03,03,03,03,03,03,01,00,
	00,00,00,01,05,05,03,03,03,03,03,01,00,
	00,00,00,00,01,05,05,05,03,01,01,00,00,
	00,00,00,00,01,01,01,01,01,00,00,00,00,
};

static const UBYTE headimage_pal[] =
{
	0xb3,0xb3,0xb3,0x00,0x00,0x00,
	0x72,0x1d,0x1d,0xf9,0xd1,0xd1,
	0xff,0xff,0xff,0xc9,0xa9,0xa9,
	0xdb,0xb8,0xb8
};

const struct CoolImage cool_headimage =
{
	headimage_data,
	headimage_pal,
	HEADIMAGE_WIDTH,
	HEADIMAGE_HEIGHT,
	HEADIMAGE_COLORS
};

