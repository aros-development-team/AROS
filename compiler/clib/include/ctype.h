#ifndef _CTYPE_H_
#define _CTYPE_H_

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: ANSI-C header file ctype.h
    Lang: english
*/

#include <sys/arosc.h>

#define _ISupper    0x0001  /* UPPERCASE */
#define _ISlower    0x0002  /* lowercase */
#define _ISalpha    0x0004  /* a-y */
#define _ISdigit    0x0008  /* 0-9 */
#define _ISxdigit   0x0010  /* 0-9, a-f, A-F */
#define _ISspace    0x0020  /* Space, Tab, CR, LF, FF */
#define _ISprint    0x0040  /* 32-126, 160-255 */
#define _ISgraph    0x0080  /* [] */
#define _ISblank    0x0100  /* Space, Tab */
#define _IScntrl    0x0200  /* 0-31, 127 */
#define _ISpunct    0x0400  /* .,:;!? */
#define _ISalnum    (_ISalpha | _ISdigit)

#define __ctype_b       (__get_arosc_userdata()->acud_ctype_b)
#define __ctype_toupper (__get_arosc_userdata()->acud_ctype_toupper)
#define __ctype_tolower (__get_arosc_userdata()->acud_ctype_tolower)

#define _istype(c,type) \
    (__ctype_b[(int) (c)] & (unsigned short int) (type))

#define isascii(c)      (((c) & ~0x7F) == 0)
#define toascii(c)      ((c) & 0x7F)

#define _isupper(c)     _istype(c,_ISupper)
#define _islower(c)     _istype(c,_ISlower)
#define _isalpha(c)     _istype(c,_ISalpha)
#define _isdigit(c)     _istype(c,_ISdigit)
#define _isxdigit(c)    _istype(c,_ISxdigit)
#define _isspace(c)     _istype(c,_ISspace)
#define _isprint(c)     _istype(c,_ISprint)
#define _isgraph(c)     _istype(c,_ISgraph)
#define _isblank(c)	_istype(c,_ISblank)
#define _iscntrl(c)     _istype(c,_IScntrl)
#define _ispunct(c)     _istype(c,_ISpunct)
#define _isalnum(c)     _istype(c,_ISalnum)
#define _iscsym(c)	(isalnum(c) || (((c) & 127) == 0x5F))	/* SAS C */
#define _iscsymf(c)	(isalpha(c) || (((c) & 127) == 0x5F)) 	/* SAS C */

#define _toupper(c)      (__ctype_toupper[(int)(c)])
#define _tolower(c)      (__ctype_tolower[(int)(c)])

#define __ctype_make_func(__name__)                         \
static __inline__ typeof(_ ## __name__(0)) __name__(int c); \
static __inline__ typeof(_ ## __name__(0)) __name__(int c)  \
{ return _ ## __name__(c); }

__ctype_make_func(isupper)
__ctype_make_func(islower)
__ctype_make_func(isalpha)
__ctype_make_func(isdigit)
__ctype_make_func(isxdigit)
__ctype_make_func(isspace)
__ctype_make_func(isprint)
__ctype_make_func(isgraph)
__ctype_make_func(isblank)
__ctype_make_func(iscntrl)
__ctype_make_func(ispunct)
__ctype_make_func(isalnum)
__ctype_make_func(iscsym)
__ctype_make_func(iscsymf)
__ctype_make_func(toupper)
__ctype_make_func(tolower)

#endif /* _CTYPE_H_ */
