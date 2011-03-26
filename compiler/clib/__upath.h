/*
    Copyright � 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: header file for libupath.a
    Lang: english
*/

#ifndef __UPATH_H__
#define __UPATH_H__

#include <aros/system.h>

__BEGIN_DECLS

const char * __path_u2a(const char *upath);
const char * __path_a2u(const char *apath);

__END_DECLS

#endif /* !__UPATH_H */
