#ifndef _GRP_H_
#define	_GRP_H_

/*
    Copyright © 2003-2004, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <sys/_types.h>
#include <sys/cdefs.h>

/*
    Implementation Note:
    You do not need to define size_t in this file. (POSIX)
*/

#ifndef __AROS_GID_T_DECLARED
#define __AROS_GID_T_DECLARED
typedef __gid_t gid_t;
#endif

struct group
{
    char   *gr_name;      /* Group name */
    char   *gr_passwd;    /* Group password */
    gid_t   gr_gid;       /* Group ID */
    char  **gr_mem;       /* Group members */
};

__BEGIN_DECLS

struct group *getgrgid(gid_t gid);
/* NOTIMPL struct group *getgrnam(const char *name); */

#if __POSIX_VISIBLE >= 200112
/* NOTIMPL int getgrgid_r(gid_t gid, struct group *grp, char *buffer, size_t bufsize,
        struct group **result); */
/* NOTIMPL int getgrname_r(const char *name, struct group *grp, char *buffer,
        size_t bufsize, struct group **resule); */
#endif

#if __XSI_VISIBLE
/* NOTIMPL struct group *getgrent(void); */
/* NOTIMPL void endgrent(void); */
/* NOTIMPL void setgrent(void); */
#endif

__END_DECLS

#endif /* _GRP_H_ */
