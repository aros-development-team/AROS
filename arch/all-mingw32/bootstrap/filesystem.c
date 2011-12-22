/*
    Copyright © 2010-2011, The AROS Development Team. All rights reserved.
    $Id: filesystem.c 43148 2011-12-21 11:18:56Z sonic $

    Desc: Filesystem control routines, Windows version
    Lang: english
*/

#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include "bootstrap.h"
#include "filesystem.h"
#include "shutdown.h"
#include "support.h"

#ifdef UNDER_CE

/* This comes from shutdown.c */
extern char *bootstrapname;

int SetRootDirectory(void)
{
    int l;
    char *name = namepart(bootstrapname) - 1;

    if (name == bootstrapname)
        return -1;
 
    /* Go one more level up (to get into parent of our directory) */
    while((name[-1] != '\\') && (name[-1] != '/'))
	name--;

    l = name - bootstrapname;
    memcpy(bootstrapdir, bootstrapname, l);
    bootstrapdir[l] = 0;

    return 0;
}

FILE *file_open(const char *filename, const char *mode)
{
    FILE *res;
    char *absname;
    int l1;
 
    if (*filename == '\\')
    {
        /* The path is given as absolute, just use it */
        return fopen(filename, mode);
    }

    l1 = strlen(bootstrapdir);
    absname = malloc(l1 + strlen(filename) + 1);

    if (!absname)
        return NULL;

    memcpy(absname, bootstrapdir, l1);
    strcpy(&absname[l1], filename);

    res = fopen(absname, mode);
    free(absname);
    return res;
}

#else

int SetRootDirectory(void)
{
    struct stat st;

    /*
     * If AROSBootstrap.exe is found in the current directory, this means the bootstrap
     * was started in its own dir. Go one level up in order to reach the root
     */
    if (!stat("AROSBootstrap.exe", &st))
	return chdir("..");

    return 0;
}

#endif
