#ifndef _SYS__POSIX_H_
#define _SYS__POSIX_H_
/*
    Copyright © 1995-2002, The AROS Development Team. All rights reserved.
    $Id$

    Header that provides the macros for determining whether various
    declarations are visible.

    Inspired by the same file from FreeBSD.
*/

/*
    Whilst not strictly true, this is the version that we appear to
    provide.
*/
#ifndef _POSIX_VERSION
#define _POSIX_VERSION	    199009L
#endif

/*
    When are the P1003.1B features visible:

    1.	If _POSIX_SOURCE and _POSIX_C_SOURCE are not defined.
    2.	If _POSIX_SOURCE or _POSIX_C_SOURCE specify a version that provides
        these features.
*/
#if (!defined(_POSIX_SOURCE) && !defined(_POSIX_C_SOURCE)) ||	\
    (POSIX_VERSION >= 199309L && defined(_POSIX_C_SOURCE) &&	\
	_POSIX_C_SOURCE >= 199309L)

#define _P1003_1B_VISIBLE
#endif

#endif /* _SYS__POSIX_H_ */
