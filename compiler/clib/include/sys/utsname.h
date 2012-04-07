#ifndef _SYS_UTSNAME_H
#define _SYS_UTSNAME_H

/*
    Copyright © 2008-2012, The AROS Development Team. All rights reserved.
    $Id$

    Desc: POSIX.1-2008 header file sys/utsname.h
*/

#include <aros/system.h>

#define _UTS_LEN 65

struct utsname
{
  char sysname[_UTS_LEN]; /* OS name */
  char nodename[_UTS_LEN]; /* network node name */
  char release[_UTS_LEN];
  char version[_UTS_LEN];
  char machine[_UTS_LEN]; /* hardware type */
};

__BEGIN_DECLS

int uname(struct utsname *name);

__END_DECLS

#endif /* _SYS_UTSNAME_H */
