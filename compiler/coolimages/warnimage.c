/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "coolimages.h"

#define WARNIMAGE_WIDTH     7
#define WARNIMAGE_HEIGHT    16
#define WARNIMAGE_COLORS    3

static const UBYTE warnimage_data[] =
{
	00,00,02,02,02,00,00,
	00,00,02,02,02,02,01,
	00,00,02,02,02,02,01,
	00,00,02,02,02,02,01,
	00,02,02,02,02,01,00,
	00,02,02,02,02,01,00,
	00,02,02,02,02,01,00,
	00,02,02,02,02,01,00,
	00,02,02,02,01,00,00,
	00,02,02,02,01,00,00,
	00,00,01,01,01,00,00,
	00,00,00,00,00,00,00,
	00,02,02,02,02,00,00,
	02,02,02,02,02,01,00,
	02,02,02,02,02,01,00,
	00,01,01,01,01,00,00
};

static const UBYTE warnimage_pal[] =
{
	0xb3,0xb3,0xb3,0x00,0x00,0x00,
	0xb6,0x37,0xb6
};

const struct CoolImage cool_warnimage =
{
	warnimage_data,
	warnimage_pal,
	WARNIMAGE_WIDTH,
	WARNIMAGE_HEIGHT,
	WARNIMAGE_COLORS
};

