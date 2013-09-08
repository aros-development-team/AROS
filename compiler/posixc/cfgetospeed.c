#include "__termios.h"

/* FIXME: Add autodocs */
speed_t cfgetospeed(const struct termios *__termios_p)
{
    if (__termios_p)
    {
        struct termios_intern * t = (struct termios_intern *)__termios_p;
        return t->c_ospeed;
    }

    /* According to man pages and online standard documents providing an
     * invalid pointer does not result in an error. Tests on Linux resulted
     * in segmentation faults, we can not do this so we retun a speed of 0.
     */
    return B0;
}
