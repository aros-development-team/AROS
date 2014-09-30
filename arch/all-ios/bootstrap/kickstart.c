/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/kernel.h>
#include <runtime.h>

#include "kickstart.h"

/*
 * Unfortunately it's impossible to fork() on iOS. fork()ed program can't register
 * itself with Springboard and fails to run.
 * So, well, currently we have no way to reboot AROS... Until we invent something clever. :)
 */
int kick(int (*addr)(), struct TagItem *msg)
{
    int i = addr(msg, AROS_BOOT_MAGIC);

    DisplayError("Kernel exited with code %d", i);
    return i;
}
