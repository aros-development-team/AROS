/*
    Copyright © 2010-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Filesystem control routines
    Lang: english
*/

#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <windows.h>
#include <sys/stat.h>

#include "bootstrap.h"
#include "filesystem.h"

char *namepart(char *name)
{
    while (*name)
	name++;

    while((name[-1] != ':') && (name[-1] != '\\') && (name[-1] != '/'))
	name--;

    return name;
}

static char *GetAbsName(const char *filename)
{
    int l1 = strlen(bootstrapdir);
    char *absname = malloc(l1 + strlen(filename) + 1);

    if (absname)
    {
        memcpy(absname, bootstrapdir, l1);
        strcpy(&absname[l1], filename);
    }

    return absname;
}

FILE *file_open(const char *filename, const char *mode)
{
    FILE *res;
    char *absname;
 
    if (*filename == '\\')
    {
        /* The path is given as absolute, just use it */
        return fopen(filename, mode);
    }

    absname = GetAbsName(filename);
    if (!absname)
        return NULL;

    res = fopen(absname, mode);
    free(absname);

    return res;
}

int SetLog(const char *filename)
{
    FILE *res;

    if (*filename != '\\')
    {
        char *absname = GetAbsName(filename);

        if (!absname)
            return GetLastError();

        res = freopen(absname, "a", stderr);
        free(absname);
    }
    else
    {
        /* The path is given as absolute, just use it */
        res = freopen(filename, "a", stderr);
    }
    return res ? 0 : GetLastError();
}
