#ifndef _SYS_UTSNAME_H
#define _SYS_UTSNAME_H

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: POSIX header file sys/utsname.h
    Lang: english
*/

#include <sys/cdefs.h>

#define _UTS_LEN 65

struct utsname
{
  char sysname[_UTS_LEN]; /* OS name */
  char nodename[_UTS_LEN]; /* network node name */
  char release[_UTS_LEN];
  char version[_UTS_LEN];
  char machine[_UTS_LEN]; /* hardware type */
#ifdef _GNU_SOURCE
    char domainname[_UTS_LEN];
#endif
};

__BEGIN_DECLS

int uname (struct utsname *name);

__END_DECLS

#endif /* _SYS_UTSNAME_H */
