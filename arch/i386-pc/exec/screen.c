#include <strings.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

static int x,y;

struct scr
{
	unsigned char sign;
	unsigned char attr;
};

struct scr	*view = (struct scr *)0xb8000;

void clr()
{
	int i;
	for (i=0; i<80*25; i++)
	{
		view[i].sign = ' ';
		view[i].attr = 15 + 1*16;
	}
	x=0;
	y=0;
}

void Putc(char chr)
{
	if (chr)
	{
		if (chr == 10)
		{
			x = 0;
			y++;
		}
		else
		{
			int i = 80*y+x;
			view[i].sign = chr;
			x++;
			if (x == 80)
			{
				x = 0;
				y++;
			}
		}
	}
	if (y>24)
	{
		int i;
		y=24;
		
		for (i=0; i<80*24; i++)
			view[i].sign = view[i+80].sign;
		for (i=80*24; i<80*25; i++)
			view[i].sign = ' ';
	}
}

#define RawPutChar(c) (putc(c))

void RawPutChars(char *chr, int lim)
{
	int i;
	
	for (i=0; i<lim; i++)
		RawPutChar(*chr++);
}

