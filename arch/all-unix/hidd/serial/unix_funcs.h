/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef UNIX_FUNCS_H
#define UNIX_FUNCS_H

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

/* int unix_open_nonblock(const char * pathname); */
#define unix_open_nonblock(pathname) open(pathname, O_NONBLOCK|O_RDWR)

#endif
