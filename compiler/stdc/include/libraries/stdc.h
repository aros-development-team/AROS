#ifndef _LIBRARIES_STDC_H
#define _LIBRARIES_STDC_H

/*
    Copyright © 2012-2013, The AROS Development Team. All rights reserved.
    $Id$

    Public part of StdC libbase.
    Take care of backwards compatibility when changing something in this file.
*/

#include <exec/libraries.h>

struct StdCBase
{
    struct Library lib;

    /* ctype.h */
    const unsigned short int * __ctype_b;
    const unsigned char * __ctype_toupper;
    const unsigned char * __ctype_tolower;

    /* errno.h */
    int _errno;
};

__BEGIN_DECLS

struct StdCBase *__aros_getbase_StdCBase(void);

__END_DECLS

#endif /* _LIBRARIES_STDC_H */
