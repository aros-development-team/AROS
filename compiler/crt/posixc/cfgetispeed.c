/*
    Copyright (C) 1995-2014, The AROS Development Team. All rights reserved.
*/

#include "__termios.h"

/****************************************************************************
    NAME
        speed_t cfgetispeed(

    SYNOPSIS
        const struct termios *__termios_p)

    FUNCTION
        Read the input speed code in a terminal-attribute object.

    INPUTS
        __termios_p - Readable terminal-attribute object whose
                      input speed has been initialized.

    RESULT
        The stored input speed code. A NULL pointer returns B0.
        The function does not change errno; B0 is not an error
        indicator.

    NOTES
        Include <termios.h>. This function accesses the supplied
        object only; it performs no terminal or device I/O.

    BUGS
        tcgetattr() does not initialize the stored speeds. Set
        the input speed with cfsetispeed() before reading it;
        a successful tcgetattr() call alone is insufficient.

    SEE ALSO
        cfsetispeed(), cfgetospeed(), tcgetattr()

*****************************************************************************/
speed_t cfgetispeed(const struct termios *__termios_p)
{
    if (__termios_p)
    {
        struct termios_intern *t = (struct termios_intern *)__termios_p;
        return t->c_ispeed;
    }

    /* According to man pages and online standard documents providing an
     * invalid pointer does not result in an error. Tests on Linux resulted
     * in segmentation faults, we can not do this so we return a speed of 0.
     */
    return B0;
}
