#ifndef _GRP_H_
#define	_GRP_H_

/*
    Copyright © 2003-2004, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <sys/types.h>
#include <sys/cdefs.h>

struct group
{
    char   *gr_name;      /* Group name */
    char   *gr_passwd;    /* Group password */
    gid_t   gr_gid;       /* Group ID */
    char  **gr_mem;       /* Group members */
};

__BEGIN_DECLS

struct group *getgrgid(gid_t gid);

__END_DECLS

#endif /* _GRP_H_ */
