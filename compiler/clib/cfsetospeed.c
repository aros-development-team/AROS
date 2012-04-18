#include <errno.h>

#include "__arosc_termios.h"

/* FIXME: Add autodoc */
int cfsetospeed(struct termios *__termios_p, speed_t speed)
{
    if (__termios_p)
    {
        struct termios_intern * t = (struct termios_intern *)__termios_p;
        t->c_ospeed = speed;
        return 0;
    }
    else
    {
        errno = EINVAL;
        return -1;
    }
}
