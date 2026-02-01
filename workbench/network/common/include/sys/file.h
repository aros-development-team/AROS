#ifndef SYS_FILE_H
#define SYS_FILE_H \
       "$Id: file.h,v 4.2 1994/10/05 23:16:17 ppessi Exp $"
/*
 *      Unix compatible include file.
 *
 *      Copyright © 1994 AmiTCP/IP Group,
 *                       Network Solutions Development, Inc.
 *                       All rights reserved.
 */

#ifndef UNISTD_H
#include <unistd.h>
#endif

#include <fcntl.h>

/* function declarations */

__BEGIN_DECLS

int flock (int fd, int operation);

__END_DECLS

#endif /* !SYS_FILE_H */
