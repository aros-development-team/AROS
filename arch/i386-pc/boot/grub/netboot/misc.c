/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2000  Free Software Foundation, Inc.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/* Based on "src/misc.c" in etherboot-4.5.8.  */

/**************************************************************************
MISC Support Routines
**************************************************************************/

#include "etherboot.h"

/**************************************************************************
SLEEP
**************************************************************************/
void sleep(int secs)
{
	unsigned long tmo;

	for (tmo = currticks()+secs*TICKS_PER_SEC; currticks() < tmo; )
		/* Nothing */;
}

/**************************************************************************
TWIDDLE
**************************************************************************/
void twiddle()
{
	static unsigned long lastticks = 0;
	static int count=0;
	static char tiddles[]="-\\|/";
	unsigned long ticks;
	if ((ticks = currticks()) == lastticks)
		return;
	lastticks = ticks;
	putchar(tiddles[(count++)&3]);
	putchar('\b');
}

#ifndef GRUB
/**************************************************************************
STRCASECMP (not entirely correct, but this will do for our purposes)
**************************************************************************/
int strcasecmp(a,b)
	char *a, *b;
{
	while (*a && *b && (*a & ~0x20) == (*b & ~0x20)) {a++; b++; }
	return((*a & ~0x20) - (*b & ~0x20));
}

/**************************************************************************
PRINTF and friends

	Formats:
		%X	- 4 byte ASCII (8 hex digits)
		%x	- 2 byte ASCII (4 hex digits)
		%b	- 1 byte ASCII (2 hex digits)
		%d	- decimal (also %i)
		%c	- ASCII char
		%s	- ASCII string
		%I	- Internet address in x.x.x.x notation
**************************************************************************/
static char hex[]="0123456789ABCDEF";
static char *do_printf(char *buf, const char *fmt, const int *dp)
{
	register char *p;
	char tmp[16];
	while (*fmt) {
		if (*fmt == '%') {	/* switch() uses more space */
			fmt++;

			if (*fmt == 'X') {
				const long *lp = (const long *)dp;
				register long h = *lp++;
				dp = (const int *)lp;
				*(buf++) = hex[(h>>28)& 0x0F];
				*(buf++) = hex[(h>>24)& 0x0F];
				*(buf++) = hex[(h>>20)& 0x0F];
				*(buf++) = hex[(h>>16)& 0x0F];
				*(buf++) = hex[(h>>12)& 0x0F];
				*(buf++) = hex[(h>>8)& 0x0F];
				*(buf++) = hex[(h>>4)& 0x0F];
				*(buf++) = hex[h& 0x0F];
			}
			if (*fmt == 'x') {
				register int h = *(dp++);
				*(buf++) = hex[(h>>12)& 0x0F];
				*(buf++) = hex[(h>>8)& 0x0F];
				*(buf++) = hex[(h>>4)& 0x0F];
				*(buf++) = hex[h& 0x0F];
			}
			if (*fmt == 'b') {
				register int h = *(dp++);
				*(buf++) = hex[(h>>4)& 0x0F];
				*(buf++) = hex[h& 0x0F];
			}
			if ((*fmt == 'd') || (*fmt == 'i')) {
				register int dec = *(dp++);
				p = tmp;
				if (dec < 0) {
					*(buf++) = '-';
					dec = -dec;
				}
				do {
					*(p++) = '0' + (dec%10);
					dec = dec/10;
				} while(dec);
				while ((--p) >= tmp) *(buf++) = *p;
			}
			if (*fmt == 'I') {
				union {
					long		l;
					unsigned char	c[4];
				} u;
				const long *lp = (const long *)dp;
				u.l = *lp++;
				dp = (const int *)lp;
				buf = sprintf(buf,"%d.%d.%d.%d",
					u.c[0], u.c[1], u.c[2], u.c[3]);
			}
			if (*fmt == 'c')
				*(buf++) = *(dp++);
			if (*fmt == 's') {
				p = (char *)*dp++;
				while (*p) *(buf++) = *p++;
			}
		} else *(buf++) = *fmt;
		fmt++;
	}
	*buf = 0;
	return(buf);
}

char *sprintf(char *buf, const char *fmt, ...)
{
	return do_printf(buf, fmt, ((const int *)&fmt)+1);
}

void printf(const char *fmt, ...)
{
	char buf[120],*p;

	p = buf;
	do_printf(buf, fmt, ((const int *)&fmt)+1);
	while (*p) putchar(*p++);
}
#endif /* ! GRUB */

#if	defined(IMAGE_MENU) || defined(GRUB)
/**************************************************************************
INET_ATON - Convert an ascii x.x.x.x to binary form
**************************************************************************/
int inet_aton(char *p, in_addr *i)
{
	unsigned long ip = 0;
	int val;
	if (((val = getdec(&p)) < 0) || (val > 255)) return(0);
	if (*p != '.') return(0);
	p++;
	ip = val;
	if (((val = getdec(&p)) < 0) || (val > 255)) return(0);
	if (*p != '.') return(0);
	p++;
	ip = (ip << 8) | val;
	if (((val = getdec(&p)) < 0) || (val > 255)) return(0);
	if (*p != '.') return(0);
	p++;
	ip = (ip << 8) | val;
	if (((val = getdec(&p)) < 0) || (val > 255)) return(0);
	i->s_addr = htonl((ip << 8) | val);
	return(1);
}
#endif	/* IMAGE_MENU || GRUB */

int getdec(char **ptr)
{
	char *p = *ptr;
	int ret=0;
	if ((*p < '0') || (*p > '9')) return(-1);
	while ((*p >= '0') && (*p <= '9')) {
		ret = ret*10 + (*p - '0');
		p++;
	}
	*ptr = p;
	return(ret);
}

#ifndef GRUB
#define K_RDWR		0x60		/* keyboard data & cmds (read/write) */
#define K_STATUS	0x64		/* keyboard status */
#define K_CMD		0x64		/* keybd ctlr command (write-only) */

#define K_OBUF_FUL	0x01		/* output buffer full */
#define K_IBUF_FUL	0x02		/* input buffer full */

#define KC_CMD_WIN	0xd0		/* read  output port */
#define KC_CMD_WOUT	0xd1		/* write output port */
#define KB_SET_A20	0xdf		/* enable A20,
					   enable output buffer full interrupt
					   enable data line
					   disable clock line */
#define KB_UNSET_A20	0xdd		/* enable A20,
					   enable output buffer full interrupt
					   enable data line
					   disable clock line */
#ifndef	IBM_L40
static void empty_8042(void)
{
	unsigned long time;
	char st;

	slowdownio();
	time = currticks() + TICKS_PER_SEC;	/* max wait of 1 second */
	while ((((st = inb(K_CMD)) & K_OBUF_FUL) ||
	       (st & K_IBUF_FUL)) &&
	       currticks() < time)
		inb(K_RDWR);
}
#endif	IBM_L40

/*
 * Gate A20 for high memory
 */
void gateA20_set(void)
{
#ifdef	IBM_L40
	outb(0x2, 0x92);
#else	/* IBM_L40 */
	empty_8042();
	outb(KC_CMD_WOUT, K_CMD);
	empty_8042();
	outb(KB_SET_A20, K_RDWR);
	empty_8042();
#endif	/* IBM_L40 */
}

#ifdef	TAGGED_IMAGE
/*
 * Unset Gate A20 for high memory - some operating systems (mainly old 16 bit
 * ones) don't expect it to be set by the boot loader.
 */
void gateA20_unset(void)
{
#ifdef	IBM_L40
	outb(0x0, 0x92);
#else	/* IBM_L40 */
	empty_8042();
	outb(KC_CMD_WOUT, K_CMD);
	empty_8042();
	outb(KB_UNSET_A20, K_RDWR);
	empty_8042();
#endif	/* IBM_L40 */
}
#endif

#ifdef	ETHERBOOT32
/* Serial console is only implemented in ETHERBOOT32 for now */
void
putchar(int c)
{
#ifndef	ANSIESC
	if (c == '\n')
		putchar('\r');
#endif

#ifdef	CONSOLE_CRT
#ifdef	ANSIESC
	handleansi(c);
#else
	putc(c);
#endif
#endif
#ifdef	CONSOLE_SERIAL
#ifdef	ANSIESC
	if (c == '\n')
		serial_putc('\r');
#endif
	serial_putc(c);
#endif
}

/**************************************************************************
GETCHAR - Read the next character from the console WITHOUT ECHO
**************************************************************************/
int
getchar(void)
{
	int c = 256;

	do {
#ifdef	CONSOLE_CRT
		if (ischar())
			c = getc();
#endif
#ifdef	CONSOLE_SERIAL
		if (serial_ischar())
			c = serial_getc();
#endif
	} while (c==256);
	if (c == '\r')
		c = '\n';
	return c;
}

int
iskey(void)
{
#ifdef	CONSOLE_CRT
	if (ischar())
		return 1;
#endif
#ifdef	CONSOLE_SERIAL
	if (serial_ischar())
		return 1;
#endif
	return 0;
}
#endif	/* ETHERBOOT32 */
#endif /* ! GRUB */

/*
 * Local variables:
 *  c-basic-offset: 8
 * End:
 */
