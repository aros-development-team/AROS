/*
    c_start.c - description
    $Id$
*/

/*
    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your opinion) any later version.
*/

#include <exec/types.h>
#include <aros/abi.h>
#include <aros/multiboot.h>
#include "of1275.h"

void com1_putc(char c);
extern char *core_id;

/* Convert the integer D to a string and save the string in BUF. If
   BASE is equal to 'd', interpret that D is decimal, and if BASE is
   equal to 'x', interpret that D is hexadecimal. */
static void itoa (char *buf, int base, int d)
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

static void _com1_putc(char c)
{
    if (c=='\n')
    {
    	com1_putc('\r');
    }
    com1_putc(c);
}

/* Format a string and print it on the screen, just like the libc
   function printf. */
void com1_printf(const char *format, ...)
{
    __builtin_va_list ap;
    __builtin_varargs_start(ap);
    
    int c;
    char buf[20];

    while ((c = *format++) != 0)
    {
	if (c != '%')
             _com1_putc(c);
	else
        {
	    char *p;
	    int n;

            c = *format++;
            switch (c)
            {
		case 'd':
                case 'u':
                case 'x':
		    itoa (buf, c, __builtin_va_arg(ap, int));
                    p = buf;
                    goto string;
                    break;

		case 's':
		    p = __builtin_va_arg(ap, char*);
                    if (! p)
			p = "(null)";

		string:
		    while (*p)
			_com1_putc(*p++);
			break;

		default:
		    _com1_putc(__builtin_va_arg(ap, int));
		    break;
	    }
	}
    }
    __builtin_va_end(ap);
}

ULONG findMem(APTR ofw, ULONG orig_MSR)
{
    ULONG volatile *mem=(APTR)(16*1024*1024);
    ULONG msr;
    int i;

    ULONG dev_handle, res, size;
    ULONG mem_info[2];
    
    asm volatile("mfmsr %0":"=r"(msr));

    while(ofw && ((ULONG)ofw < 0x10000000))
    {
	asm volatile("mtmsr %0"::"r"(orig_MSR));
	of_init(ofw);

	res = of_finddevice("/memory@0", &dev_handle);

	if (res)
		break;

	res = of_getprop(dev_handle, "reg", mem_info, sizeof(mem_info), &size);
	if (res) break;

	mem = (ULONG *)mem_info[1];

	break;
    }

    asm volatile("mtmsr %0"::"r"(msr));
    return (ULONG)mem;
}

void cstart(APTR load_addr, APTR real_addr, ULONG kernel_size,
	APTR residual, APTR ofw, ULONG orig_MSR)
{
    int i=1;
    struct CallOS callos;
    
    /*
	Define CallOS on stack temporarly. It is allowed since Launch kernel
	from this function while local variables remains reserved
    */
    
    CallOS = &callos;

    ULONG mem_avail; //=findMem();	/* Assume 16MB of ram */
    com1_printf("\n%s\n",&core_id);

    com1_printf("\n[startup] load_addr  : $%x"
    		"\n[startup] real_addr  : $%x"
      		"\n[startup] kernel_size: %dKB"
        	"\n[startup] residual   : $%x"
         	"\n[startup] ofw_iface  : $%x\n",
    	load_addr, real_addr, (kernel_size+1023) >> 10, residual, ofw);

    mem_avail = findMem(ofw, orig_MSR);

    com1_printf("[startup] memory detected: %dMB\n",
    	mem_avail >> 20);
            
    /*
	Multiboot structure is forced to appear at physical address
	location 0x00000008. It is allowable since the exception
	vectors are at 0xfff00100 and will be fixed later. By this
	time we create multiboot here.
    */
    struct multiboot *mb = (struct multiboot*)0x8;
    struct mb_mmap *mmap = (struct mb_mmap*)(0x8 + sizeof(struct multiboot));

    /* Prepare multiboot structure */
    mb->flags =   MB_FLAGS_LDRNAME
    		| MB_FLAGS_CMDLINE
      		| MB_FLAGS_MMAP;
    mb->loader_name = "PowerPC bootloader";
    mb->cmdline = "";
    mb->mmap_addr = (ULONG)mmap;
    mb->mmap_length = ((mem_avail > 16*1024*1024) ? 2 : 1) * sizeof(struct mb_mmap);

    /*
    	Do mmap setup. Usually we have 1 or 2 entries, the
     	first is the DMA memory and the second is everything
	else.
    */
    mmap[0].size = sizeof(struct mb_mmap) - 4;
    mmap[0].type = MMAP_TYPE_RAM;
    mmap[0].addr      = 0x4000;
    mmap[0].addr_high = 0;
    mmap[0].len       = (ULONG)real_addr - 0x4000;
    mmap[0].len_high  = 0;

    if (mem_avail > 16*1024*1024)
    {
	mmap[1].size = sizeof(struct mb_mmap) - 4;
	mmap[1].type = MMAP_TYPE_RAM;
	mmap[1].addr      = 16*1024*1024;
	mmap[1].addr_high = 0;
	mmap[1].len       = mem_avail - 16*1024*1024;
	mmap[1].len_high  = 0;
    }

    

//    cmain(MULTIBOOT_BOOTLOADER_MAGIC, (ULONG)mb);
    
    do {} while(1);
}
