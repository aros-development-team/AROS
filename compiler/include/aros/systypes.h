#ifndef _AROS_SYSTYPES_H
#define _AROS_SYSTYPES_H
/*
    Copyright © 1995-2002, The AROS Development Team. All rights reserved.
    $Id$

    AROS Header to define many important types without declaring them.
*/

/* For the fixed-length types */
#include <exec/types.h>

/*
    Many types are expected to be declared in multiple header files. This
    can cause many problems with multiple redefinition. The solution to this
    is twofold.

    1.	Provide a system global version of this type that the headers which
        declare the type can use.
    2.	Make the headers that declare the type declare it safely as
        follows.

	#ifdef	_AROS_SIZE_T_
	typedef	_AROS_SIZE_T_	size_t;
	#undef	_AROS_SIZE_T_
	#endif

	This is the same method that *BSD uses. I do not know where it was
	first applied.
*/

#define _AROS_CLOCKID_T_    int
#define _AROS_CLOCK_T_	    unsigned long
#define _AROS_OFF_T_	    LONG	/* XXX Large Files? */
#define _AROS_PID_T_	    IPTR
#define _AROS_PTRDIFF_T_    signed   long
#define _AROS_SIZE_T_	    unsigned int
#define _AROS_SOCKLEN_T_    ULONG
#define _AROS_SSIZE_T_	    int
#define _AROS_TIMER_T_	    int
#define _AROS_TIME_T_	    ULONG	/* XXX Limiting */
#define _AROS_VA_LIST_	    char *

/*
    These should be the same as the locale.library usage
    They are not though. locale.library uses ULONG for it's character type.
    That is a reasonable assumption, however it makes it difficult for us
    to have the regular assumption about WEOF.

    Not to mention that ISO 10646 defines a 31 bit character set, which
    means that signed representation is quite reasonable.

    The downside of all this is that we will end up with quite a few
    signed/unsigned problems.
*/

#define _AROS_WINT_T_	    LONG

/*
    There are also some types which are used in multiple places, but only
    declared in one place. The user of these types are not supposed to
    declare the type.

    In this case you can simply use the define without undefining it.
*/

#define _AROS_UID_T_	    ULONG


/*
    Both <stddef.h> and <sys/types.h> define this!
*/

#define __offsetof(type,field)	    ((size_t)(&((type *)0)->field))

#endif /* _AROS_SYSTYPES_H */
