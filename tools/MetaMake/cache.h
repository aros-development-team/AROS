#ifndef __MMAKE_CACHE_H
#define __MMAKE_CACHE_H

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

#include "dirnode.h"

typedef struct
{
    Node node;
    int virtualtarget : 1;
    
    Makefile * makefile;
}
MakefileRef;
   
typedef struct
{
    Node node;
    int  updated : 1;

    List makefiles;
}
Target;

/* The public part of the Cache structure */
typedef struct {
    List targets;
} Cache;

#include "project.h"

Cache *activatecache(Project *);
void closecache(Cache * cache);

#endif /* __MMAKE_CACHE_H */
