/*
    Copyright © 2009, The AROS Development Team. All rights reserved.
    $Id$

    Desc: dos support functions for internal use
*/

#include <aros/debug.h>

#include <proto/dos.h>

#include "__filesystem_support.h"

CONST_STRPTR StripVolume(CONST_STRPTR name) {
    const char *path = strchr(name, ':');
    if (path != NULL)
        path++;
    else
        path = name;
    return path;
}
