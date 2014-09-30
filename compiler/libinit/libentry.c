/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

/*
 * This file is added to every disk-based resident module in order to make it not executable.
 * Its location is defined in make.cfg (RESIDENT_BEGIN variable).
 * Dependencies are listed in make.tail.
 */

#include <aros/system.h>

int __startup __resident_entry(void)
{
    return -1;
}
