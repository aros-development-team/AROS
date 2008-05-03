/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id: screen.c 23064 2005-03-08 21:43:00Z stegerg $
*/

#define SCREEN_SERIAL_DEBUG

#include <strings.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "font8x14.c"

#undef __save_flags
#undef __restore_flags
#undef __cli
#undef __sti

#define __save_flags(x)		__asm__ __volatile__("pushfq ; popq %0":"=g" (x): /* no input */)
#define __restore_flags(x) 	__asm__ __volatile__("pushq %0 ; popfq": /* no output */ :"g" (x):"memory", "cc")
#define __cli() 		__asm__ __volatile__("cli": : :"memory")
#define __sti()			__asm__ __volatile__("sti": : :"memory")

static int x,y, dead, vesa, w, wc=80, h, hc=25, bpp;
static int scr_rpclock;

void *fb;

struct scr
{
    unsigned char sign;
    unsigned char attr;
};

static struct scr *view = (struct scr *)0xb8000;

void vesa_init(int width, int height, int depth, void *base)
{
    vesa = 1;
    w = width;
    wc = width >> 3;
    h = height;
    hc = height / 14;
    
    bpp = (depth < 24) ? 2 : 4;
    fb = base;
}

void clr()
{
    unsigned long flags;
    int i;
    unsigned long *ptr = fb;
    
    __save_flags(flags);
    __cli();

    if (!dead) 
    {
        if (vesa) for (i = 0; i < bpp*w*h/8; i++)
        {
            *ptr++ = 0;
        }
        else for (i=0; i<80*25; i++)

        {
            view[i].sign = ' ';
            view[i].attr = 7;
        }
        x=0;
        y=0;
    }

    __restore_flags(flags);
}

void RenderChar(unsigned char c, void *ptr, int line_width)
{
    int i;
    for (i=0; i < 14; i++, ptr += line_width*bpp)
    {
        unsigned char in = vga8x14[c][i];
        int j;
        
        for (j=0; j < 8; j++)
        {
            if (in & (0x80 >> j))
            {
                if (bpp == 4)
                    ((unsigned int *)ptr)[j] = 0xffffffff;
                else
                    ((unsigned short *)ptr)[j] = 0xffff;
            }
            else
            {
                if (bpp == 4)
                    ((unsigned int *)ptr)[j] = 0;
                else
                    ((unsigned short *)ptr)[j] = 0;
            }
        }
    }
}

void Putc(char chr)
{
    unsigned long flags;

    __save_flags(flags);
    __cli();

#warning "TODO: Enable screen debug to serial at config time, and note that it doesnt work properly on real hardware since it doesnt initialise the serial port to a given baud_rate"
#if defined(SCREEN_SERIAL_DEBUG)
    if (chr != 3)
    {
#if AROS_SERIAL_DEBUG == 1
        asm volatile ("outb %b0,%w1"::"a"(chr),"Nd"(0x3F8));
#endif
#if AROS_SERIAL_DEBUG == 2
        asm volatile ("outb %b0,%w1"::"a"(chr),"Nd"(0x2F8));
#endif    
    }
#endif

    if (chr == 3) /* die / CTRL-C / "signal" */
    {
        dead = 1;
    }
    else if (!dead && !vesa)
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
    else if (!dead && vesa)
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
                void *ptr = fb + ((bpp*w*y) * 14) + bpp*(x << 3);

                RenderChar(chr, ptr, w);

                x++;
                if (x == wc)
                {
                    x = 0;
                    y++;
                }
            }
            if (y>(hc-1))
            {
                int i;
                unsigned long *ptr = fb;

                y=hc-1;

                for (i=0; i<bpp*w*(14*(hc-1))/8; i++)
                    ptr[i] = ptr[i+2*bpp*w];

                for (i=bpp*w*(14*(hc-1))/8; i<bpp*w*h/8; i++)
                    ptr[i] = 0;
            }
        }
    }
    __restore_flags(flags);
}

void scr_RawPutChars(char *chr, int lim)
{
    int i;

    asm volatile ( "mov     $1, %%eax\n\t"
                  "loop:"
                   "xchg    %0, %%eax\n\t"
                   "test    %%eax, %%eax\n\t"  
                   "jnz     loop":"=m"(scr_rpclock):"m"(scr_rpclock));

    for (i=0; i<lim; i++)
        Putc(*chr++);

    asm volatile ( "mov     $0, %%eax\n\t"
                   "xchg    %0, %%eax":"=m"(scr_rpclock):"m"(scr_rpclock));
}

