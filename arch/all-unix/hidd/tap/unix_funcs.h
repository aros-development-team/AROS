/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id: unix_funcs.h 19289 2003-08-18 15:19:59Z verhaegs $
*/

#ifndef UNIX_FUNCS_H
#define UNIX_FUNCS_H

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

/* int unix_open_nonblock(const char * pathname); */
#define unix_open_nonblock(pathname) open(pathname, O_NONBLOCK|O_RDWR)

#endif
