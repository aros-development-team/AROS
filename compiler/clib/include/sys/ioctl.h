#ifndef _SYS_IOCTL_H
#define _SYS_IOCTL_H

/*
    Copyright © 2004, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <sys/types.h>

__BEGIN_DECLS
int ioctl(int fd, int request, ...);
__END_DECLS

#endif /* _SYS_IOCTL_H */

