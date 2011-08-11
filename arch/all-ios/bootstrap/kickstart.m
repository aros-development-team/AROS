#include <aros/kernel.h>
#include <runtime.h>

#include "kickstart.h"

/*
 * TODO: This code is temporarily copied here to shut up the compilation.
 * UNIX kicker won't work with iOS in its current form. UIKit code needs
 * to be reworked.
 */
int kick(int (*addr)(), struct TagItem *msg)
{
    int i = addr(msg, AROS_BOOT_MAGIC);

    DisplayError("Kernel exited with code %d", i);
    return i;
}
