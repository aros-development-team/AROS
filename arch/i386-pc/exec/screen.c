/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <strings.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#undef __save_flags
#undef __restore_flags
#undef __cli
#undef __sti

#define __save_flags(x)		__asm__ __volatile__("pushfl ; popl %0":"=g" (x): /* no input */)
#define __restore_flags(x) 	__asm__ __volatile__("pushl %0 ; popfl": /* no output */ :"g" (x):"memory", "cc")
#define __cli() 		__asm__ __volatile__("cli": : :"memory")
#define __sti()			__asm__ __volatile__("sti": : :"memory")

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

void scr_RawPutChars(char *chr, int lim)
{
    int i;

    for (i=0; i<lim; i++)
	Putc(*chr++);
}

