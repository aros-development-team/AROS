/*
    (C) 2001 AROS - The Amiga Research OS
    $Id$

    Desc:
    Lang: English
*/

/*********************************************************************************************/

#include "global.h"

#include "earthmap_small_image.c"
#include "timezones_small_image.c"

/*********************************************************************************************/

#define CONTINENT_RED 	    	18
#define CONTINENT_GREEN     	114
#define CONTINENT_BLUE      	58

#define OCEAN_RED   	    	21
#define OCEAN_GREEN 	    	18
#define OCEAN_BLUE  	    	114

#define SELECTED_INTENSITY_INC	50

/*********************************************************************************************/

static WORD domleft, domtop, domwidth, domheight;

static UBYTE earthmap_chunky[EARTHMAP_SMALL_WIDTH * EARTHMAP_SMALL_HEIGHT];
static UBYTE timezones_chunky[TIMEZONES_SMALL_WIDTH * TIMEZONES_SMALL_HEIGHT];

static ULONG earthmap_coltab[256];

/*********************************************************************************************/

#if EARTHMAP_SMALL_PACKED || TIMEZONES_SMALL_PACKED

static UBYTE *unpack_byterun1(UBYTE *source, UBYTE *dest, LONG unpackedsize)
{
    UBYTE r;
    BYTE c;
    
    for(;;)
    {
	c = (BYTE)(*source++);
	if (c >= 0)
	{
    	    while(c-- >= 0)
	    {
		*dest++ = *source++;
		if (--unpackedsize <= 0) return source;
	    }
	}
	else if (c != -128)
	{
    	    c = -c;
	    r = *source++;

	    while(c-- >= 0)
	    {
		*dest++ = r;
		if (--unpackedsize <= 0) return source;
	    }
	}
    }
    
}

#endif

/*********************************************************************************************/

static LONG timezone_init(void)
{
    WORD i;
    
#if EARTHMAP_SMALL_PACKED
    unpack_byterun1(earthmap_small_data, earthmap_chunky, EARTHMAP_SMALL_WIDTH * EARTHMAP_SMALL_HEIGHT);
#endif
#if TIMEZONES_SMALL_PACKED
    unpack_byterun1(timezones_small_data, timezones_chunky, TIMEZONES_SMALL_WIDTH * TIMEZONES_SMALL_HEIGHT);
#endif

    for(i = 0; i < EARTHMAP_SMALL_COLORS; i++)
    {
    	ULONG rgb = earthmap_small_pal[i];
	ULONG r = (rgb & 0xFF0000) >> 16;
	ULONG g = (rgb & 0x00FF00) >> 8;
	ULONG b = (rgb & 0x0000FF);
	ULONG a = (r + g + b) / 3;
	
	r = (a * OCEAN_RED   + (255 - a) * CONTINENT_RED  ) / 255;
	g = (a * OCEAN_GREEN + (255 - a) * CONTINENT_GREEN) / 255;
	b = (a * OCEAN_BLUE  + (255 - a) * CONTINENT_BLUE ) / 255;
	
	rgb = (r << 16) + (g << 8) + b;
	
	earthmap_coltab[i] = rgb;
	
	r += SELECTED_INTENSITY_INC; if (r > 255) r = 255;
	g += SELECTED_INTENSITY_INC; if (g > 255) g = 255;
	b += SELECTED_INTENSITY_INC; if (b > 255) b = 255;
	
	rgb = (r << 16) + (g << 8) + b;
	
	earthmap_coltab[128 + i] = rgb;
	
    }
    
    return TRUE;
}

/*********************************************************************************************/

LONG page_timezone_handler(LONG cmd, LONG param)
{
    LONG retval = TRUE;
    
    switch(cmd)
    {
    	case PAGECMD_INIT:
	    retval = timezone_init();
	    break;
	    
	case PAGECMD_LAYOUT:
	    break;
	    
	case PAGECMD_GETMINWIDTH:
	    retval = EARTHMAP_SMALL_WIDTH + 2;
	    break;
	    
	case PAGECMD_GETMINHEIGHT:
	    retval = EARTHMAP_SMALL_HEIGHT + 2;
	    break;
	    
	case PAGECMD_SETDOMLEFT:
	    domleft = param;
	    break;
	    
	case PAGECMD_SETDOMTOP:
	    domtop = param;
	    break;
	    
	case PAGECMD_SETDOMWIDTH:
	    domwidth = param;
	    break;
	    
	case PAGECMD_SETDOMHEIGHT:
	    domheight = param;
	    break;
	    
	case PAGECMD_MAKEGADGETS:
	case PAGECMD_ADDGADGETS:
	case PAGECMD_REMGADGETS:
	case PAGECMD_HANDLEINPUT:
	case PAGECMD_CLEANUP:
	    break;
    }
    
    return retval;
    
}
