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
#include <stdio.h>
#include <stdlib.h>
#ifdef HAVE_STRING_H
#   include <string.h>
#else
#   include <strings.h>
#endif

#include "var.h"
#include "mem.h"

/* Functions */
char *
getvar (List * varlist, const char * varname)
{
    static char buffer[256];
    char *env_val;
    Var * var = FindNode (varlist, varname);

    if (var)
	return var->value;

    env_val = getenv(varname);
    if(env_val)
    {
	return env_val;
    }
    sprintf (buffer, "?$(%s)", varname);
    return buffer;
}

char *
substvars (List * varlist, const char * str)
{
    static char buffer[4096];
    char varname[256];
    const char * src;
    char * dest, * vptr;

    assert (str);

    src = str;
    dest = buffer;

    while (*src)
    {
	if (*src == '$')
	{
	    src += 2;
	    vptr = varname;

	    while (*src && *src != ')')
	    {
		*vptr ++ = *src ++;
	    }
	    if (*src)
		src ++;

	    *vptr = 0;

	    strcpy (dest, getvar (varlist, varname));
	    dest += strlen (dest);
	}
	else
	    *dest ++ = *src ++;

	assert (dest<buffer+sizeof(buffer));
    }

    *dest = 0;

    return buffer;
}

void
setvar (List * varlist, const char * name, const char * val)
{
    Var * var;

    assert (name);

#if 0
    printf ("assign %s=%s\n", name, val);
#endif

    var = addnodeoncesize (varlist, name, sizeof(Var));
    SETSTR (var->value, val);

#if 0
    printf ("vars=");
    printvarlist (varlist);
#endif
}

void
printvarlist (List * l)
{
    Var * n;

    ForeachNode (l,n)
    {
	printf ("    %s=%s\n", n->node.name, n->value);
    }
}

void
freevarlist (List * l)
{
    Var * node, * next;

    ForeachNodeSafe(l,node,next)
    {
	Remove (node);

	xfree (node->node.name);
	cfree (node->value);
	xfree (node);
    }
}

char **
getargs (const char * line, int * argc, List * vars)
{
    static char * argv[64];
    static char * buffer = NULL;
    char * src;
    int arg;

    cfree (buffer);
    buffer = NULL;
    
    if (!line)
	return NULL;

    if (vars)
	buffer = xstrdup (substvars (vars, line));
    else
	buffer = xstrdup (line);

    assert (buffer);

    src = buffer;
    arg = 0;

    while (*src)
    {
	while (isspace (*src))
	    src ++;

	if (!*src)
	    break;

	assert (arg < 63);
	argv[arg++] = src;

	if (*src == '"')
	{
	    while (*src && *src != '"')
		src ++;
	}
	else
	{
	    while (*src && !isspace (*src))
		src ++;
	}

	if (*src)
	    *src++ = 0;
    }

    argv[arg] = NULL;

    if (argc)
	*argc = arg;

    return argv;
}
