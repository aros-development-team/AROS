/* Include before #ifndef so that Cache is always defined before Project */
#include "cache.h"

#ifndef __MMAKE_PROJECT_H
#define __MMAKE_PROJECT_H

/* MetaMake - A Make extension
   Copyright © 1995-2010, The AROS Development Team. All rights reserved.

This file is part of MetaMake.

MetaMake is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

MetaMake is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GNU CC; see the file COPYING.  If not, write to
the Free Software Foundation, 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.  */

#include "list.h"
#include "cache.h"

struct Project
{
    struct Node node;

    char * maketool;
    char * defaultmakefilename;
    char * srctop;
    char * buildtop;
    char * defaulttarget;
    char * genmakefilescript;
    char * genglobalvarfile;

    int readvars;

    struct List globalvarfiles;
    struct List genmakefiledeps;
    struct List ignoredirs;
    struct List vars;
    struct List extramakefiles;
    
    struct Cache * cache;
};

void initprojects (void);
void expungeprojects (void);
struct Project * findproject (const char * pname);
struct Project * getfirstproject (void);
void maketarget (struct Project * prj, char * tname);
int execute (struct Project * prj, const char * cmd, const char * in,
             const char * out, const char * args);

#endif /* __MMAKE_PROJECT_H */
