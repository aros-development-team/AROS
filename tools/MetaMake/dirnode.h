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

typedef struct _DirNode DirNode;

struct _DirNode
{
    Node node;
    
    DirNode * parent;
    time_t	time;
    List subdirs;
    List makefiles;
};

typedef struct
{
    Node node;

    int virtualtarget;
    List deps;
}
MakefileTarget;

typedef struct
{
    Node node;

    DirNode * dir; /* The directory where this makefile is located */
    time_t time; /* Last time this Makefile was scanned for targets */
    List targets; /* list of MakefileTargets: targets present in this makefile */
}
Makefile;

void printdirnode (DirNode * node, int level);
void printdirnodemftarget (DirNode * node);
void freedirnode (DirNode * node);
void freemakefile (Makefile * makefile);
DirNode * finddirnode (DirNode * node, const char * path);
int scandirnode (DirNode * node, const char * mfname, List * ignoredirs);
int scanmakefiles (DirNode * node, List * vars);
Makefile * addmakefile (DirNode * node, const char * filename);
Makefile * findmakefile (DirNode * node, const char * filename);
const char * buildpath (DirNode * node);
DirNode * readcachedir (FILE * fh);
int writecachedir (FILE * fh, DirNode * node);

#endif /* __MMAKE_DIRNODE_H */
