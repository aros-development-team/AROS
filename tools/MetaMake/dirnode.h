#ifndef __MMAKE_DIRNODE_H
#define __MMAKE_DIRNODE_H

/* MetaMake - A Make extension
   Copyright © 1995-2004, The AROS Development Team. All rights reserved.

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

#include <time.h>
#include <stdio.h>

#include "list.h"
#include "project.h"

struct DirNode
{
    struct Node node;
    
    struct DirNode * parent;
    time_t	time;
    struct List subdirs;
    struct List makefiles;
};

struct MakefileTarget
{
    struct Node node;

    int virtualtarget;
    struct List deps;
};

struct Makefile
{
    struct Node node;

    struct DirNode * dir; /* The directory where this makefile is located */
    time_t time; /* Last time this Makefile was scanned for targets */
    struct List targets; /* list of MakefileTargets: targets present in this makefile */
    int  generated;
};

void printdirnode (struct DirNode * node, int level);
void printdirnodemftarget (struct DirNode * node);
void freedirnode (struct DirNode * node);
void freemakefile (struct Makefile * makefile);
struct DirNode * finddirnode (struct DirNode * node, const char * path);
int scandirnode (struct DirNode * node, const char * mfname, struct List * ignoredirs);
int scanmakefiles (struct Project * prj, struct DirNode * node, struct List * vars);
struct Makefile * addmakefile (struct DirNode * node, const char * filename);
struct Makefile * findmakefile (struct DirNode * node, const char * filename);
const char * buildpath (struct DirNode * node);
struct DirNode * readcachedir (FILE * fh);
int writecachedir (FILE * fh, struct DirNode * node);
void freemakefiletargetlist (struct List * targets);

#endif /* __MMAKE_DIRNODE_H */
