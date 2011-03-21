/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: screen output functions.
*/

#include <aros/multiboot.h>

#include "screen.h"

static unsigned char type = SCR_UNKNOWN;

/* Display buffer parameters */
void         *fb = 0;		/* VRAM address			*/
unsigned int  wc = 0;		/* Display width in characters	*/
unsigned int  hc = 0;		/* Display height in characters	*/
unsigned int  pitch = 0;	/* Bytes per line		*/
unsigned int  bpp = 0;		/* Bytes per pixel		*/

/* Current output position (in characters) */
unsigned int x = 0;
unsigned int y = 0;

/*
 * Change this to 1 to enable serial output.
 * TODO: set in runtime according to command line arguments.
 */
unsigned char use_serial = 0;

void initScreen(struct multiboot *mb)
{
    if (mb->flags & MB_FLAGS_FB)
    {
	/* Framebuffer was given, use it */
	fb    = (void *)(unsigned long)mb->framebuffer_addr;
	pitch = mb->framebuffer_pitch;
        wc    = mb->framebuffer_width  / fontWidth;
	hc    = mb->framebuffer_height / fontHeight;
    	bpp   = mb->framebuffer_bpp >> 3;

    	type = SCR_GFX;
    }
    /* TODO: detect VESA modes (both text and graphics) */
    else
    {
    	/* Fallback to default, VGA text mode */
    	fb = (void *)0xb8000;
    	wc = 80;
    	hc = 25;

    	type = SCR_TEXT;
    }

    clr();
}

void clr()
{
    x = 0;
    y = 0;

    switch (type)
    {
    case SCR_TEXT:
    	txtClear();
    	break;
    
    case SCR_GFX:
    	gfxClear();
    	break;
    }
}

void Putc(char c)
{
    if (use_serial)
    {
    	if (c == '\n')
	    serPutC('\r');
	serPutC(c);
    }

    switch (type)
    {
    case SCR_TEXT:
    	txtPutc(c);
    	break;

    case SCR_GFX:
    	gfxPutc(c);
    	break;
    }
}

/* Convert the integer D to a string and save the string in BUF. If
   BASE is equal to 'd', interpret that D is decimal, and if BASE is
   equal to 'x', interpret that D is hexadecimal. */
static void __itoa (char *buf, int base, int d)
{
    char *p = buf;
    char *p1, *p2;
    unsigned long ud = d;
    int divisor = 10;

    /* If %d is specified and D is minus, put `-' in the head. */
    if (base == 'd' && d < 0)
    {
        *p++ = '-';
        buf++;
        ud = -d;
    }
    else if (base == 'x')
        divisor = 16;
    else if (base == 'p')
    {
	int i;
	for (i=0; i<8; i++)
	{
	    char v = (d >> (28-i*4)) & 0xf;
	    *p++ = (v < 10) ? v + '0' : v + 'A' - 10;
	}
	*p=0;
	return;
    }

    /* Divide UD by DIVISOR until UD == 0. */
    do
    {
        int remainder = ud % divisor;

        *p++ = (remainder < 10) ? remainder + '0' : remainder + 'a' - 10;
    }
    while (ud /= divisor);

    /* Terminate BUF. */
    *p = 0;

    /* Reverse BUF. */
    p1 = buf;
    p2 = p - 1;
    while (p1 < p2)
    {
        char tmp = *p1;
        *p1 = *p2;
        *p2 = tmp;
        p1++;
        p2--;
    }
}

void kprintf(const char *format, ...)
{
    unsigned long *ptr = (unsigned long *)&format + 1;
    int c;
    char buf[20];

    while ((c = *format++) != 0)
    {
        if (c != '%')
             Putc(c);
        else
        {
            char *p;

            c = *format++;
            switch (c)
            {
                case 'd':
                case 'u':
                case 'x':
                case 'p':
                    __itoa (buf, c, (int)*ptr++);
                    p = buf;
                    goto string;
                    break;

                case 's':
                    p = (char*)*ptr++;
                    if (! p)
                        p = "(null)";

                string:
                    while (*p)
                        Putc(*p++);
                        break;

                default:
                    Putc((char)*ptr++);
                    break;
            }
        }
    }
}
