/*
    (C) 2001 AROS - The Amiga Research OS
    $Id$

    Desc:
    Lang: english
*/

#include "coolimages.h"

#define DOTIMAGE_WIDTH 9 
#define DOTIMAGE_HEIGHT 9
#define DOTIMAGE_DEPTH 4

static const UBYTE dotimage_data[] =
{
	00,00,00,03,03,03,00,00,00,
	00,03,03,03,03,03,03,01,00,
	00,03,03,03,03,03,03,01,00,
	03,03,03,03,03,03,03,03,01,
	03,03,03,03,03,03,03,03,01,
	03,03,03,03,03,03,03,03,01,
	00,03,03,03,03,03,03,01,00,
	00,01,01,03,03,03,01,01,00,
	00,00,00,01,01,01,00,00,00
};

static const UBYTE dotimage_pal[] =
{
	0xb3,0xb3,0xb3,0x00,0x00,0x00,
	0xff,0xff,0xff,0x00,0x76,0x00
};

static const UBYTE dotimage2_pal[] =
{
	0xb3,0xb3,0xb3,0x00,0x00,0x00,
	0xff,0xff,0xff,0x00,0x00,0xA0
};

const struct CoolImage cool_dotimage =
{
	dotimage_data,
	dotimage_pal,
	DOTIMAGE_WIDTH,
	DOTIMAGE_HEIGHT,
	DOTIMAGE_DEPTH
};

const struct CoolImage cool_dotimage2 =
{
	dotimage_data,
	dotimage2_pal,
	DOTIMAGE_WIDTH,
	DOTIMAGE_HEIGHT,
	DOTIMAGE_DEPTH
};
