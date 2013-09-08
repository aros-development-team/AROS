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
};

__BEGIN_DECLS

struct StdCBase *__aros_getbase_StdCBase(void);

__END_DECLS

#endif /* _LIBRARIES_STDC_H */
