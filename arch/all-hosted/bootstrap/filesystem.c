/*
    Copyright © 2010-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Filesystem control routines
    Lang: english
*/

#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>

#include "filesystem.h"

int SetRootDirectory(void)
{
    struct stat st;

    /* If AROSBootstrap.exe is found in the current directory, this means the bootstrap
     was started in its own dir. Go one level up in order to reach the root */
    if (!stat("AROSBootstrap", &st))
	return chdir("..");

    return 0;
}
