/*
    Copyright © 2010-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Filesystem control routines
    Lang: english
*/

/*
 * A temporary workaround to get the bootstrap compiled for WinCE.
 * In fact Windows CE port requires emulation of current directory.
 */
#ifndef UNDER_CE

#include <dirent.h>
#include <sys/stat.h>

#ifdef _WIN32
#define APPNAME "AROSBootstrap.exe"
#else
#include <unistd.h>
#define APPNAME "AROSBootstrap"
#endif

#include "filesystem.h"

int SetRootDirectory(void)
{
    struct stat st;

    /* If AROSBootstrap.exe is found in the current directory, this means the bootstrap
     was started in its own dir. Go one level up in order to reach the root */
    if (!stat(APPNAME, &st))
	return chdir("..");

    return 0;
}

#endif
