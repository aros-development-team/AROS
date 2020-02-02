#ifndef ___STAT_H
#define ___STAT_H

/*
    Copyright © 1995-2020, The AROS Development Team. All rights reserved.
    $Id$

    Desc: stat() internals - header file
    Lang: english
*/

#include <sys/stat.h>
#include <exec/types.h>

#include <dos/dos.h>

int __stat(BPTR lock, struct stat *sb, BOOL filehandle);
int __stat64(BPTR lock, struct stat64 *sb, BOOL filehandle);
int __stat_from_path(const char *path, struct stat *sb);
int __stat64_from_path(const char *path, struct stat64 *sb);

#endif
