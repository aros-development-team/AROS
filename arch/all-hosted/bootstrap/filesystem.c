/*
    Copyright © 2010-2014, The AROS Development Team. All rights reserved.
    $Id$
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
    int i, ret;

    /*
     * If AROSBootstrap.exe is found in the current directory, this means the bootstrap
     * was started in its own dir. Go one or two levels up in order to reach the root.
     */
    if (!stat(APPNAME, &st))
    {
        for (i = 0; i < 2; i++)
        {
            ret = chdir("..");
            if (ret)
                return ret;

            if (!stat("AROS.boot", &st))
                break;
        }
    }

    return 0;
}
