/* Include before #ifndef so that Cache is always defined before Project */
#include "cache.h"

#ifndef __MMAKE_PROJECT_H
#define __MMAKE_PROJECT_H

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

typedef struct
{
    Node node;

    char * maketool;
    char * defaultmakefilename;
    char * top;
    char * defaulttarget;
    char * genmakefilescript;
    char * globalvarfile;
    char * genglobalvarfile;

    int readvars;

    List genmakefiledeps;
    List ignoredirs;
    List vars;
    List extramakefiles;
    
    Cache * cache;
}
Project;

extern void initprojects (void);
extern void expungeprojects (void);
extern Project * findproject (const char * pname);
extern Project * getfirstproject (void);
extern void maketarget (Project * prj, char * tname);
extern int execute (Project * prj, const char * cmd, const char * in,
		     const char * out, const char * args);

#endif /* __MMAKE_PROJECT_H */
