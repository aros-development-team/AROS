/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <intuition/intuition.h>
#include <graphics/gfx.h>
#include <cybergraphx/cybergraphics.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/intuition.h>
#include <proto/cybergraphics.h>
#include <stdio.h>
#include <string.h>

struct IntuitionBase *IntuitionBase;
struct GfxBase *GfxBase;
struct Library *CyberGfxBase;
struct Window *win;
struct RastPort *rp;

#include "ilbmtoc_image.c"

UBYTE unpacked_image[EARTHMAP_SMALL_WIDTH * EARTHMAP_SMALL_HEIGHT];

#define CONTINENT_RED 	18
#define CONTINENT_GREEN 114
#define CONTINENT_BLUE  58

#define OCEAN_RED   	21
#define OCEAN_GREEN 	18
#define OCEAN_BLUE  	114

static unsigned char *unpack_byterun1(unsigned char *source, unsigned char *dest, LONG unpackedsize)
{
    unsigned char r;
    signed char c;
    
    for(;;)
    {
	c = (signed char)(*source++);
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

int main(void)
{
    IntuitionBase = (struct IntuitionBase *)OpenLibrary("intuition.library", 39);
    GfxBase = (struct GfxBase *)OpenLibrary("graphics.library", 39);
    CyberGfxBase = OpenLibrary("cybergraphics.library", 39);
    
    if (IntuitionBase && GfxBase && CyberGfxBase)
    {
    	printf("All libraries opened\n");
	
	win = OpenWindowTags(0, WA_InnerWidth, EARTHMAP_SMALL_WIDTH,
	    	    	    	WA_InnerHeight, EARTHMAP_SMALL_HEIGHT,
				WA_CloseGadget, TRUE,
				WA_DepthGadget, TRUE,
				WA_DragBar, TRUE,
				WA_IDCMP, IDCMP_CLOSEWINDOW,
				WA_Activate, TRUE,
				TAG_DONE);
				
	rp = win->RPort;
	
	unpack_byterun1(earthmap_small_data, unpacked_image, EARTHMAP_SMALL_WIDTH * EARTHMAP_SMALL_HEIGHT);
	
	{
	    WORD i;
	    
	    for(i = 0; i < EARTHMAP_SMALL_COLORS; i++)
	    {
	    	ULONG rgb   = earthmap_small_pal[i];
	    	ULONG red   = (rgb & 0xFF0000) >> 16;
		ULONG green = (rgb & 0x00FF00) >> 8;
		ULONG blue  = (rgb & 0x0000FF);
		ULONG alpha = (red + green + blue) / 3;
		
		red   = (alpha * OCEAN_RED   + (255 - alpha) * CONTINENT_RED)   / 255;
		green = (alpha * OCEAN_GREEN + (255 - alpha) * CONTINENT_GREEN) / 255;
		blue  = (alpha * OCEAN_BLUE  + (255 - alpha) * CONTINENT_BLUE)  / 255;
		
		rgb = (red << 16) + (green << 8) + blue;
		
		earthmap_small_pal[i] = rgb;
		
	    }
	    
	}
	WriteLUTPixelArray(unpacked_image,
			   0,
			   0,
			   EARTHMAP_SMALL_WIDTH,
			   rp,
			   earthmap_small_pal,
			   win->BorderLeft,
			   win->BorderTop,
			   EARTHMAP_SMALL_WIDTH,
			   EARTHMAP_SMALL_HEIGHT,
			   CTABFMT_XRGB8);
	
	WaitPort(win->UserPort);
	
	CloseWindow(win);
    }
    
    return 0;
}
