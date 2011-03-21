/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: screen output functions.
*/

#include <aros/multiboot.h>

#include <stdarg.h>

#include "screen.h"
#include "support.h"

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
    if (mb->flags & MB_FLAGS_CMDLINE)
    {
        char *debug = __bs_strstr((const char *)mb->cmdline, "debug=serial");
        
        if (debug)
        {
            use_serial = 1;
            initSerial(&debug[12]);
        }
    }

    if (mb->flags & MB_FLAGS_FB)
    {
    	/* Framebuffer was given, use it */
	fb    = (void *)(unsigned long)mb->framebuffer_addr;
	pitch = mb->framebuffer_pitch;
	bpp   = mb->framebuffer_bpp >> 3;

    	switch (mb->framebuffer_type)
    	{
    	case MB_FRAMEBUFFER_TEXT:
    	   /* Text framebuffer, size in characters */
	   wc   = mb->framebuffer_width;
	   hc   = mb->framebuffer_height;
	   type = SCR_TEXT;
	   break;

	default:
	   /* Graphical framebuffer, size in pixels */
	   wc   = mb->framebuffer_width  / fontWidth;
	   hc   = mb->framebuffer_height / fontHeight;
	   type = SCR_GFX;
	}
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

unsigned int format_int (char *buf, char base, int d)
{
    char *p = buf;
    char minus = 0;
    int ud = d;

    switch (base)
    {
    case 'p':
    case 'x':
    	do
    	{
	    char v = d & 0xf;

	    *p++ = (v < 10) ? v + '0' : v + 'A' - 10;
	    d >>= 4;
	} while (d);
    	break;

    case 'd':
    	/* If %d is specified and D is minus, put `-' in the head. */
        if (d < 0)
    	{
            Putc('-');
	    ud = -d;
    	}

    case 'u':
	/* Divide UD by 10 until UD == 0 */
    	do
    	{
            int remainder = ud % 10;

            *p++ = (remainder < 10) ? remainder + '0' : remainder + 'a' - 10;
    	}
    	while (ud /= 10);
    	break;
    }

    if (minus)
    	*p++ = '-';

    return p - buf;
}

unsigned int format_longlong (char *buf, char base, long long d)
{
    char *p = buf;
    char minus = 0;
    unsigned long long ud = d;

    switch (base)
    {
    case 'p':
    case 'x':
    	do
    	{
	    char v = d & 0xf;

	    *p++ = (v < 10) ? v + '0' : v + 'A' - 10;
	    d >>= 4;
	} while (d);
    	break;

    case 'd':
    	/* If %d is specified and D is minus, put `-' in the head. */
        if (d < 0)
    	{
            minus = 1;
	    ud = -d;
    	}

    case 'u':
	/*
	 * Divide UD by 10 until UD == 0
	 * FIXME: implement these division routines.
	 * Otherwise this causes linking with 32-bit libgcc
	 * which 64-bit compiler doesn't have.
	 * Until implemented, %lld will not work.
	 *
    	do
    	{
            int remainder = ud % 10;

            *p++ = (remainder < 10) ? remainder + '0' : remainder + 'a' - 10;
    	}
    	while (ud /= 10);*/
    	break;
    }

    if (minus)
    	*p++ = '-';

    return p - buf;
}

#ifdef __x86_64__
#define format_long format_longlong
#else
#define format_long format_int
#endif

void kprintf(const char *format, ...)
{
    va_list ap;
    char c;

    va_start(ap, format);

    while ((c = *format++) != 0)
    {
        if (c != '%')
             Putc(c);
        else
        {
            char buf[21];	/* This buffer is enough to hold -1ULL in decimal form */
            char *p;
            char fill = ' ';
            unsigned char size = 0;
            unsigned int ptrlen = sizeof(int) * 2;
            unsigned int len = 0;
            unsigned int l;

            c = *format++;
            if (c == '0')
            {
            	fill = c;
            	c = *format++;
            }
            /* TODO: support explicit length specification */
            if (c == 'l')
            {
            	size++;
            	c = *format++;
            	ptrlen = sizeof(long) * 2;
            }
            if (c == 'l')
            {
            	size++;
            	c = *format++;
            	ptrlen = sizeof(long long) * 2;
            }
            switch (c)
            {
            case 'p':
                fill = '0';
                if (len < ptrlen)
            	    len = ptrlen;
            case 'u':
            case 'x':
            case 'd':
                switch (size)
                {
                case 2:
                    l = format_longlong(buf, c, va_arg(ap, long long));
                    break;

                case 1:
                    l = format_long(buf, c, va_arg(ap, long));
                    break;

		default:
                    l = format_int(buf, c, va_arg(ap, int));
                    break;
                }

		/* Print padding if needed */
                while (len > l)
                {
                    Putc(fill);
                    len--;
                }
                /* Now print our buffer in reverse order */
                while (l)
                    Putc(buf[--l]);

                break;

            case 's':
                p = va_arg(ap, char *);
                if (!p)
                    p = "(null)";
		while (*p)
                    Putc(*p++);
                break;

            default:
                Putc(va_arg(ap, int));
            }
        }
    }
    
    va_end(ap);
}
