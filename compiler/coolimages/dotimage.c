/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "coolimages.h"

#define DOTIMAGE_WIDTH  9 
#define DOTIMAGE_HEIGHT 9
#define DOTIMAGE_COLORS 3

static const UBYTE dotimage_data[] =
{
	00,00,00,02,02,02,00,00,00,
	00,02,02,02,02,02,02,01,00,
	00,02,02,02,02,02,02,01,00,
	02,02,02,02,02,02,02,02,01,
	02,02,02,02,02,02,02,02,01,
	02,02,02,02,02,02,02,02,01,
	00,02,02,02,02,02,02,01,00,
	00,01,01,02,02,02,01,01,00,
	00,00,00,01,01,01,00,00,00
};

static const UBYTE dotimage_pal[] =
{
	0xb3,0xb3,0xb3,0x00,0x00,0x00,
	0x00,0x76,0x00
};

static const UBYTE dotimage2_pal[] =
{
	0xb3,0xb3,0xb3,0x00,0x00,0x00,
	0x00,0x00,0xA0
};

const struct CoolImage cool_dotimage =
{
	dotimage_data,
	dotimage_pal,
	DOTIMAGE_WIDTH,
	DOTIMAGE_HEIGHT,
	DOTIMAGE_COLORS
};

const struct CoolImage cool_dotimage2 =
{
	dotimage_data,
	dotimage2_pal,
	DOTIMAGE_WIDTH,
	DOTIMAGE_HEIGHT,
	DOTIMAGE_COLORS
};
