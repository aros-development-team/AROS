/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <registers.h>
#include "../palmgfx.h"

void pause(LONG x)
{
	LONG i = 0,a = 1;
	while (i < x) {
		a *=a*a;
		i++;
	}
}

void drawlinehoriz(int y)
{	
	LONG lssa = RREG_L(LSSA);
	LONG height = RREG_W(LYMAX);
	LONG width = RREG_W(LXMAX)>>3;
	LONG x = 0;
	
	if (y < 0 || y >= height)
		return;

	while (x <= width) {
		*(BYTE *)(lssa+((width+1)*y)+x) = 0xff;
		x++;
	}
	
}

void drawlinevert(int x)
{	
	LONG lssa = RREG_L(LSSA);
	LONG height = RREG_W(LYMAX);
	LONG width  = RREG_W(LXMAX);
	LONG y = 0;
	
	if (x < 0 || x >= width)
		return;
	width >>= 3;
	while (y < height) {
		*(BYTE *)(lssa+((width+1)*y)+(x>>3)) |= 1<<(7-(x&7));
		y++;
	}
	
}

void setpixel(int x, int y)
{
	LONG lssa = RREG_L(LSSA);
	LONG height = RREG_W(LYMAX);
	LONG width  = RREG_W(LXMAX);
	
	if (x < 0 || x >= width || y < 0 || y >= height)
		return;
		
	width>>=3;
	
	*(BYTE *)(lssa+((width+1)*y)+(x>>3)) |= 1<<(7-(x&7));
	
}

void clearpixel(int x, int y)
{
	LONG lssa = RREG_L(LSSA);
	LONG height = RREG_W(LYMAX);
	LONG width  = RREG_W(LXMAX);
	
	if (x < 0 || x >= width || y < 0 || y >= height)
		return;
		
	width>>=3;
	
	*(BYTE *)(lssa+((width+1)*y)+(x>>3)) &= ~(1<<(7-(x&7)));
	
}

void clearscreen(int value)
{
	LONG lssa = RREG_L(LSSA);
	LONG height = RREG_W(LYMAX);
	LONG width = RREG_W(LXMAX)>>3;
	LONG y = 0;
	
	while (y < height) {
		LONG x = 0;
		while (x <= width) {
			*(BYTE *)(lssa+((width+1)*y)+x) = value;
			x++;
		}
		y++;
	}
}

void flashscreen(int loop)
{
	int i = 0, d;
	while (i < loop) {
		clearscreen(0);
		d = 0;
		while (d < 10000)
			d++;
		clearscreen(255);
		d = 0;
		while (d < 10000)
			d++;
		i++;
	}
}

void drawcross(void)
{
	clearscreen(0);
	drawlinehoriz(80);
	drawlinevert(80);
	pause(100000);
}

void showsuccess(void)
{
	LONG height = RREG_W(LXMAX);

	LONG c = 0;
	while (c < height) {
		drawlinehoriz(c);
		c++;
		pause(10000);
	}
	clearscreen(0);
		
}

void forever(void)
{
	int c = 0;
	while (1) {
		showsuccess();
		clearscreen(c);
		c += 10;
	}
}
