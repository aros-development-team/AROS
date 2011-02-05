/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: screen support functions ripped from the 32-bit native target.
*/

//#define SCREEN_SERIAL_DEBUG

#include <stdarg.h>
#include <stdio.h>

#undef __save_flags
#undef __restore_flags
#undef __cli
#undef __sti

#define __save_flags(x)		__asm__ __volatile__("pushfl ; popl %0":"=g" (x): /* no input */)
#define __restore_flags(x) 	__asm__ __volatile__("pushl %0 ; popfl": /* no output */ :"g" (x):"memory", "cc")
#define __cli() 		__asm__ __volatile__("cli": : :"memory")
#define __sti()			__asm__ __volatile__("sti": : :"memory")

#define SCREEN_SERIAL_DEBUG
#define AROS_SERIAL_DEBUG 1

static int x,y, dead;

struct scr
{
    unsigned char sign;
    unsigned char attr;
};

static struct scr *view = (struct scr *)0xb8000;

void clr()
{
    unsigned long flags;
    int i;
    
    __save_flags(flags);
    __cli();
	
    if (!dead) for (i=0; i<80*25; i++)
    {
        view[i].sign = ' ';
        view[i].attr = 7;
    }
    x=0;
    y=0;
    
    __restore_flags(flags);
}

void Putc(char chr)
{
    unsigned long flags;
    
    __save_flags(flags);
    __cli();
    if (chr == 3) /* die / CTRL-C / "signal" */
    {
    	dead = 1;
    }
    else if (!dead)
    {
#warning "TODO: Enable screen debug to serial at config time, and note that it doesnt work properly on real hardware since it doesnt initialise the serial port to a given baud_rate"
#if defined(SCREEN_SERIAL_DEBUG)
#if AROS_SERIAL_DEBUG == 1
        asm volatile ("outb %b0,%w1"::"a"(chr),"Nd"(0x3F8));
#endif
#if AROS_SERIAL_DEBUG == 2
        asm volatile ("outb %b0,%w1"::"a"(chr),"Nd"(0x2F8));
#endif    
#endif
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
    __restore_flags(flags);
}

static char tmpbuf[512];

void kprintf(const char *format, ...)
{
	char *out = tmpbuf;
	va_list vp;
	va_start(vp, format);

	vsnprintf(tmpbuf, 511, format, vp);

	va_end(vp);

	while (*out)
		Putc(*out++);
}

//void __kprintf(const char *format, ...)
//{
//    unsigned long *ptr = (unsigned long *)&format + 1;
//    int c;
//    char buf[20];
//
//    while ((c = *format++) != 0)
//    {
//        if (c != '%')
//             Putc(c);
//        else
//        {
//            char *p;
//
//            c = *format++;
//            switch (c)
//            {
//                case 'd':
//                case 'u':
//                case 'x':
//                case 'p':
//                    __itoa (buf, c, (int)*ptr++);
//                    p = buf;
//                    goto string;
//                    break;
//
//                case 's':
//                    p = (char*)*ptr++;
//                    if (! p)
//                        p = "(null)";
//
//                string:
//                    while (*p)
//                        Putc(*p++);
//                        break;
//
//                default:
//                    Putc((char)*ptr++);
//                    break;
//            }
//        }
//    }
//}

