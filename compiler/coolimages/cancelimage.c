/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/

#include "coolimages.h"

#define CANCELIMAGE_WIDTH   14
#define CANCELIMAGE_HEIGHT  16
#define CANCELIMAGE_COLORS  3

static const UBYTE cancelimage_data[] =
{
	00,00,00,00,00,00,00,00,00,00,02,02,02,00,
	00,02,02,01,00,00,00,00,00,02,02,02,02,01,
	02,02,02,02,01,00,00,00,02,02,02,02,02,01,
	02,02,02,02,02,01,00,02,02,02,02,02,01,00,
	00,02,02,02,02,02,02,02,02,02,02,01,00,00,
	00,00,02,02,02,02,02,02,02,02,01,00,00,00,
	00,00,00,02,02,02,02,02,02,01,00,00,00,00,
	00,00,00,00,02,02,02,02,01,00,00,00,00,00,
	00,00,00,02,02,02,02,02,02,02,00,00,00,00,
	00,00,02,02,02,02,02,02,02,02,02,00,00,00,
	00,02,02,02,02,02,01,01,02,02,02,02,00,00,
	00,02,02,02,02,01,00,00,01,02,02,02,02,00,
	02,02,02,02,01,00,00,00,00,01,02,02,02,01,
	02,02,02,01,00,00,00,00,00,00,01,02,02,01,
	02,02,02,01,00,00,00,00,00,00,00,01,01,00,
	00,01,01,00,00,00,00,00,00,00,00,00,00,00
};

static const UBYTE cancelimage_pal[] =
{
	0xb3,0xb3,0xb3,0x00,0x00,0x00,
	0xdb,0x20,0x26
};

const struct CoolImage cool_cancelimage =
{
	cancelimage_data,
	cancelimage_pal,
	CANCELIMAGE_WIDTH,
	CANCELIMAGE_HEIGHT,
	CANCELIMAGE_COLORS
};
