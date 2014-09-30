/*
    Copyright � 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <errno.h>

#include "__termios.h"

/* FIXME: Add autodoc */
int cfsetispeed(struct termios *__termios_p, speed_t speed)
{
    if (__termios_p)
    {
        struct termios_intern *t = (struct termios_intern *)__termios_p;
        t->c_ispeed = speed;
        return 0;
    }
    else
    {
        errno = EINVAL;
        return -1;
    }
}
