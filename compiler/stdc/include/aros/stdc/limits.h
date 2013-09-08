/*
 *  Copyright © 2004-2012 The AROS Developmemt Team. All rights reserved.
 *  $Id$
 *
 *  C99 header file limits.h
 */

#ifndef _STDC_LIMITS_H_
#define _STDC_LIMITS_H_


#include <aros/cpu.h> /* For __WORDSIZE */

/* Sizes of integer types <limits.h> */

/*
 *  Define ANSI <limits.h> for standard 32-bit words.
 *  We're assuming
 *  8-bit 'char',
 *  16-bit 'short int', and
 *  32-bit 'int' and 'long int'.
 */

/* FIXME: Are all these values 64 bit OK ? */

/* Number of bits for smallest object that is not a bit-field (byte) */
#define CHAR_BIT	8
/* minimum value for an object of type signed char */
#define SCHAR_MIN	(-128)
/* maximum value for an object of type signed char */
#define SCHAR_MAX	127
/* maximum value for an object of type unsigned char */
#define UCHAR_MAX	255
/* minimum value for an object of type char */
#define CHAR_MIN	SCHAR_MIN
/* maximum value for an object of type char */
#define CHAR_MAX	SCHAR_MAX
/* maximum number of bytes in a multibyte character, for any supported locale */
#define	MB_LEN_MAX	1	/* At the moment only "C" locale supported */
/* minimum value for an object of type short int */
#define SHRT_MIN	(-32768)
/* maximum value for an object of type short int */
#define SHRT_MAX	32767
/* maximum value for an object of type unsigned short int */
#define USHRT_MAX	65535
/* minimum value for an object of type int */
#define INT_MIN		(-INT_MAX - 1)
/* maximum value for an object of type int */
#define INT_MAX		2147483647
/* maximum value for an object of type unsigned int */
#define UINT_MAX	4294967295U
/* minimum value for an object of type long int */
#define LONG_MIN	(-LONG_MAX - 1L)
/* maximum value for an object of type long int */
#if __WORDSIZE == 64
#   define LONG_MAX	9223372036854775807L
#else
#   define LONG_MAX	2147483647L
#endif
/* maximum value for an object of type unsigned long int */
#if __WORDSIZE == 64
#   define ULONG_MAX    18446744073709551615UL
#else
#   define ULONG_MAX    4294967295UL
#endif
/* minimum value for an object of type long long int */
#define LLONG_MIN	(-0x7fffffffffffffffLL - 1)
/* maximum value for an object of type long long int */
#define LLONG_MAX	0x7fffffffffffffffLL
/* maximum value for an object of type unsigned long long int */
#define ULLONG_MAX	0xffffffffffffffffULL

#endif /* _STDC_LIMITS_H_ */
