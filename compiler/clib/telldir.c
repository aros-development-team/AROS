/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <dirent.h>

off_t telldir(const DIR *dir)
{
    return dir->pos;
}
