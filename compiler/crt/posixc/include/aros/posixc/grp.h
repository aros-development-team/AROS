#ifndef _POSIXC_GRP_H_
#define	_POSIXC_GRP_H_

/*
    Copyright © 2003-2012, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/types/size_t.h>
#include <aros/types/gid_t.h>

struct group
{
    char   *gr_name;      /* Group name */
    gid_t   gr_gid;       /* Group ID */
    char  **gr_mem;       /* Group members */
    char   *gr_passwd;    /* Group password */
};

__BEGIN_DECLS

void endgrent(void);
struct group *getgrent(void);
struct group *getgrgid(gid_t gid);
struct group *getgrnam(const char *name);
/* NOTIMPL int getgrgid_r(gid_t gid, struct group *grp, char *buffer, size_t bufsize,
        struct group **result); */
/* NOTIMPL int getgrnam_r(const char *name, struct group *grp, char *buffer,
        size_t bufsize, struct group **resule); */
void setgrent(void);

__END_DECLS

#endif /* _POSIXC_GRP_H_ */
