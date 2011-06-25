/*
 *  filesystem.c
 *  AROS
 *
 *  Created by Pavel Fedin on 10/13/10.
 *  Copyright 2010 AROS Development Team. All rights reserved.
 *
 */

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
