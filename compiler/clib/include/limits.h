/*
 *  ISO C99 Standard: Sizes of integer types <limits.h>
 */

#ifndef LIMITS_H
#define LIMITS_H 

/*
 *  Define ANSI <limits.h> for standard 32-bit words.
 *  We're assuming
 *  8-bit 'char',
 *  16-bit 'short int', and
 *  32-bit 'int' and 'long int'.
 */

/* Maximum length of any multibyte character in any locale.
 * Gcc's own limit.h dosn't define this value correctly, that's
 * why we define it here.
 */
#define MB_LEN_MAX      16

/* Number of bits in a 'char' */
#define CHAR_BIT	8

/* Minimum and maximum number displayable with 'signed char' */
#define SCHAR_MIN	(-128)
#define SCHAR_MAX	127

/* Minimum and maximum number displayable with 'char' */
#define CHAR_MIN	SCHAR_MIN
#define CHAR_MAX	SCHAR_MAX

/* Maximum number displayable with 'unsigned char'. Minimum is 0. */
#define UCHAR_MAX	255


/* Minimum and maximum number displayable with 'signed short int' */
#define SHRT_MIN	(-32768)
#define SHRT_MAX	32767

/* Maximum number displayable with 'unsigned short int'. Minimum is 0. */
#define USHRT_MAX	65535


/* Minimum and maximum displayable with 'signed int' */
#define INT_MAX		2147483647
#define INT_MIN		(-INT_MAX - 1)

/* Maximum value displayable with 'unsigned int'. Minimum is 0. */
#  define UINT_MAX	4294967295U

/* Minimum and maximum number displayable with `signed long int' */
#if __WORDSIZE == 64
#   define LONG_MAX	9223372036854775807L
#else
#   define LONG_MAX	2147483647L
#endif
#define LONG_MIN	(-LONG_MAX - 1L)

/* Maximum number displayable with `unsigned long int'. Minimum is 0. */
#if __WORDSIZE == 64
#   define ULONG_MAX    18446744073709551615UL
#else
#   define ULONG_MAX    4294967295UL
#endif

#define LLONG_MIN	(-0x7fffffffffffffffLL - 1)
#define LLONG_MAX	0x7fffffffffffffffLL
#define ULLONG_MAX	0xffffffffffffffffULL

/*
 *  Maximal length of a path name.
 *  This is value is borrowed from Linux and may be totally wrong for AROS,
 *  at least it is defined to make some apps happy.
 */
#ifndef PATH_MAX
#   define PATH_MAX	4095
#endif

#endif
