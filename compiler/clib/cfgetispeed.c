#include <termios.h>

speed_t cfgetispeed(const struct termios *__termios_p)
{
    if (__termios_p)
        return __termios_p->c_ispeed;

    /* According to man pages and online standard documents providing an
     * invalid pointer does not result in an error. Tests on Linux resulted
     * in segmentation faults, we can not do this so we retun a speed of 0.
     */
    return B0;
}
