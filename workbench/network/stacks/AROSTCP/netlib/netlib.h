/*
 *      $Id$
 *
 *      netlib.h - common Network Support Library definitions
 *
 *      Copyright © 1994 AmiTCP/IP Group, 
 *                       Network Solutions Development Inc.
 *                       All rights reserved.
 */

/*
 * This is supposed to be compiler-independent error setting interface
 */
#ifdef __SASC
extern int _OSERR;
extern int errno;
extern int __io2errno(int);
#define __seterrno(x) do { errno = __io2errno(_OSERR = (x)); } while (0)
#define SET_OSERR(code) do { _OSERR = (code); } while (0)
#else
void __seterrno(UBYTE code);
#define SET_OSERR(code) do { } while (0)
#endif

#define set_errno __seterrno
#define OSERR _OSERR
