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

#include "config.h"

#include <assert.h>
#ifdef HAVE_SYS_STAT_H
#   include <sys/stat.h>
#endif

#include "dep.h"
#include "mem.h"

Dep *
newdepnode (const char * path)
{
    Dep * n;
    struct stat st;

    assert (path);

    n = new (Dep);

    n->node.name = xstrdup (path);
    lstat (path, &st);
    n->time = st.st_mtime;

    return n;
}

int
checkdeps (List * deps, time_t desttime)
{
    Dep * dep;
    int newer = 0;

    ForeachNode (deps, dep)
    {
	if (dep->time > desttime)
	{
/*printf ("%s is newer\n", dep->node.name);*/
	    newer = 1;
	    break;
	}
    }

    return newer;
}
