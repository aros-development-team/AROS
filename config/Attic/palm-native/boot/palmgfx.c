/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <Pilot.h>

#include <registers.h>

void pause(Long x)
{
	Long i = 0,a = 1;
	while (i < x) {
		a *=a*a;
		i++;
	}
}

void drawlinehoriz(int y)
{	
	Long lssa = RREG_L(LSSA);
	Long height = RREG_W(LYMAX);
	Long width = RREG_W(LXMAX)>>3;
	Long x = 0;
	
	if (y < 0 || y >= height)
		return;
	
	while (x <= width) {
		*(Byte *)(lssa+((width+1)*y)+x) = 0xff;
		x++;
	}
	
}

void drawlinevert(int x)
{	
	Long lssa = RREG_L(LSSA);
	Long height = RREG_W(LYMAX);
	Long width  = RREG_W(LXMAX);
	Long y = 0;
	
	if (x < 0 || x >= width)
		return;
	width >>= 3;
	while (y < height) {
		*(Byte *)(lssa+((width+1)*y)+(x>>3)) |= 1<<(7-(x&7));
		y++;
	}
	
}

void setpixel(int x, int y)
{
	Long lssa = RREG_L(LSSA);
	Long height = RREG_W(LYMAX);
	Long width  = RREG_W(LXMAX);
	
	if (x < 0 || x >= width || y < 0 || y >= height)
		return;
		
	width>>=3;
	
	*(Byte *)(lssa+((width+1)*y)+(x>>3)) |= 1<<(7-(x&7));
	
}

void clearpixel(int x, int y)
{
	Long lssa = RREG_L(LSSA);
	Long height = RREG_W(LYMAX);
	Long width  = RREG_W(LXMAX);
	
	if (x < 0 || x >= width || y < 0 || y >= height)
		return;
		
	width>>=3;
	
	*(Byte *)(lssa+((width+1)*y)+(x>>3)) &= ~(1<<(7-(x&7)));
	
}

void clearscreen(int value)
{
	Long lssa = RREG_L(LSSA);
	Long height = RREG_W(LYMAX);
	Long width = RREG_W(LXMAX)>>3;
	Long y = 0,x;
	
	while (y < height) {
		x = 0;
		while (x <= width) {
			*(Byte *)(lssa+((width+1)*y)+x) = value;
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
	Long height = RREG_W(LXMAX);

	Long c = 0;
	while (c < height) {
		drawlinehoriz(c);
		c++;
		pause(10000);
	}
	clearscreen(0);
	
}
