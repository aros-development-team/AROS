/*
    Copyright (C) 1995-2014, The AROS Development Team. All rights reserved.
*/

#include <errno.h>

#include "__termios.h"

/****************************************************************************
    NAME
        int cfsetispeed(

    SYNOPSIS
        struct termios *__termios_p, speed_t speed)

    FUNCTION
        Store the input speed code in a terminal-attribute object.

    INPUTS
        __termios_p - Writable terminal-attribute object.
        speed - Speed code, normally a B-prefixed constant from
                <termios.h>, such as B9600; not a baud count.

    RESULT
        0 after storing speed. Returns -1 and sets errno to
        EINVAL if __termios_p is NULL. Success does not clear errno.

    NOTES
        Include <termios.h>. This function accesses the supplied
        object only; it performs no terminal or device I/O. Only
        the input speed is changed.

    BUGS
        The implementation accepts any speed_t value without
        checking whether it names a supported rate. tcsetattr()
        does not apply the stored speed to the terminal.

        B0 is stored literally; the input speed is not resolved
        to the output speed.

    SEE ALSO
        cfgetispeed(), cfsetospeed(), tcsetattr()

*****************************************************************************/
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
