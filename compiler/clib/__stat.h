#ifndef ___STAT_H
#define ___STAT_H

/*
    Copyright 2001 AROS - The Amiga Research OS
    $Id$

    Desc: stat() internals - header file
    Lang: english
*/

#include <sys/stat.h>
#include <exec/types.h>

mode_t __prot_a2u(ULONG protect);
uid_t  __amiga2unixid(UWORD id);
int    __stat(BPTR lock, struct stat *sb);

#endif
