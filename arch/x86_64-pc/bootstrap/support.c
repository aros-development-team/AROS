/*
    Copyright (C) 2006-2011 The AROS Development Team. All rights reserved.
    $Id$
    
    Desc: Miscellaneous support functions.
    Lang: English
*/

#include <bootconsole.h>
#include <stdarg.h>

#include "bootstrap.h"
#include "support.h"

const char *__bs_remove_path(const char *in)
{
    const char *p;

    /* Go to the end of string */
    for (p = in; *p; p++);
    /* Now go backwards until we find a separator */
    while (p > in && p[-1] != '/' && p[-1] != ':') p--;

    return p;
}

/* Own memcpy(), because librom's one can use CopyMem() */
void *__bs_memcpy(void *dest, const void *src, long len)
{
    while (len >= 4)
    {
        *(unsigned long *)dest = *(unsigned long *)src;
        len-=4;
        dest+=4;
        src+=4;
    }
    if (len >= 2)
    {
        *(unsigned short *)dest = *(unsigned short *)src;
        dest+=2;
        src+=2;
        len-=2;
    }
    if (len == 1)
    {
        *(unsigned char *)dest = *(unsigned char *)src;
        dest += 1;
    }

    /* Return next byte in the destination, useful in some cases */
    return dest;
}

/*
 * Our extremely simple working memory allocator.
 * We can't just use some memory region because kickstart modules are placed there by GRUB.
 * We risk clobbering loaded kickstart in such a case.
 * The only 100% usable memory is memory contained in our own file.
 * So we reserve some workspace here. I hope 1MB is more than enough for out needs.
 * This space ends up in .bss section, so it does not occupy this megabyte on disk.
 */
static char workspace[0x1000000];

static char *MemPtr = workspace;

void *__bs_malloc(unsigned long size)
{
    char *start = MemPtr;
    char *end;

    /* Longword-align the size */
    size = (size + sizeof(void *) - 1) & ~sizeof(void *);
    end  = start + size;

    /*
     * _start is provided by linker script, it marks start of
     * our executable. This is the end of allocatable region.
     * We also count reserved space for stack which is placed in the
     * end of our working memory.
     */
    if (end > workspace + sizeof(workspace))
    	return 0;

    MemPtr = end;

    return start;
}

/* This routine resets the allocator and releases all previously allocated memory */
void __bs_free(void)
{
    MemPtr = workspace;
}

static unsigned int format_int (char *buf, char base, int d)
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
	    minus = 1;
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

static unsigned int format_longlong (char *buf, char base, long long d)
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
	 * P. S. If we fix this, we can get rid of own formatting implementation at all,
	 * instead using __vcformat() from librom32.
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
             con_Putc(c);
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
                    con_Putc(fill);
                    len--;
                }
                /* Now print our buffer in reverse order */
                while (l)
                    con_Putc(buf[--l]);

                break;

            case 's':
                p = va_arg(ap, char *);
                if (!p)
                    p = "(null)";
		while (*p)
                    con_Putc(*p++);
                break;

            default:
                con_Putc(va_arg(ap, int));
            }
        }
    }
    
    va_end(ap);
}
