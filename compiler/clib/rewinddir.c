/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <dirent.h>

void rewinddir(DIR *dir)
{
    seekdir(dir, 0);
}
