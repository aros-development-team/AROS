/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <dirent.h>

long
telldir(DIR *dir)
{
    return dir->pos;
}
