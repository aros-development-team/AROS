#ifndef	__OSDEP_H__
#define __OSDEP_H__

/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2, or (at
 * your option) any later version.
 */

#if	defined(__linux__) || defined(__FreeBSD__) || defined(GRUB)
#define	ETHERBOOT32
#define ntohl(x) swap32(x)
#define htonl(x) swap32(x)
#define ntohs(x) swap16(x)
#define htons(x) swap16(x)

static inline unsigned long int swap32(unsigned long int x)
{
	__asm__("xchgb %b0,%h0\n\t"
		"rorl $16,%0\n\t"
		"xchgb %b0,%h0"
		: "=q" (x)
		: "0" (x));
	return x;
}

static inline unsigned short int swap16(unsigned short int x)
{
	__asm__("xchgb %b0,%h0"
		: "=q" (x)
		: "0" (x));
	return x;
}

#ifndef GRUB
# include "linux-asm-string.h"
#endif /* ! GRUB */
#include "linux-asm-io.h"
#ifndef GRUB
#define	_edata	edata			/* ELF does not prepend a _ */
#define	_end	end
#endif /* ! GRUB */
#endif

#ifdef	__BCC__
#define	ETHERBOOT16
#define	inline
#define	const
#define	volatile
#define	setjmp	_setjmp		/* they are that way in libc.a */
#define	longjmp	_longjmp

/* BCC include files are missing these. */
typedef unsigned char u_char;
typedef unsigned short u_short;
typedef unsigned int u_int;
typedef unsigned long u_long;
#endif

#if	!defined(ETHERBOOT16) && !defined(ETHERBOOT32)
Error, neither ETHERBOOT16 nor ETHERBOOT32 defined
#endif

#if	defined(ETHERBOOT16) && defined(ETHERBOOT32)
Error, both ETHERBOOT16 and ETHERBOOT32 defined
#endif

typedef	unsigned long Address;

/* ANSI prototyping macro */
#ifdef	__STDC__
#define	P(x)	x
#else
#define	P(x)	()
#endif

#endif

/*
 * Local variables:
 *  c-basic-offset: 8
 * End:
 */
