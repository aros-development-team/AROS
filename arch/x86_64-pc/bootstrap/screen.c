/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id: screen.c 23064 2005-03-08 21:43:00Z stegerg $

    Desc: screen support functions ripped from the 32-bit native target.
*/

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

