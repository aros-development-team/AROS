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

#include <stdio.h>
#include <assert.h>
#ifdef HAVE_STRING_H
#   include <string.h>
#else
#   include <strings.h>
#endif
#ifdef HAVE_SYS_STAT_H
#   include <sys/stat.h>
#endif
#ifdef HAVE_SYS_TYPES_H
#   include <sys/types.h>
#endif

#include "dirnode.h"
#include "mem.h"

void
printdirnode (DirNode * node, int level)
{
    DirNode * subdir;
    int t;

    for (t=0; t<level; t++)
	printf ("    ");

    printf ("%s\n", node->node.name);

    level ++;

    ForeachNode (&node->subdirs, subdir)
	printdirnode (subdir, level);
}

void
freecachenodes (DirNode * node)
{
    DirNode * subnode;

    while ((subnode = GetHead (&node->subdirs)))
    {
	Remove (subnode);
	freecachenodes (subnode);
    }

    xfree (node->node.name);
    xfree (node);
}

DirNode *
finddirnode (DirNode * topnode, const char * path)
{
    const char * ptr;
    char dirname[256];
    int len;
    DirNode * node = topnode, * subdir;

    ptr = path+2;

    if (!*ptr)
	return node;

    subdir = NULL;

    while (*ptr)
    {
	for (len=0; ptr[len] && ptr[len] != '/'; len++);

	strncpy (dirname, ptr, len);
	dirname[len] = 0;
	ptr += len;
	while (*ptr == '/')
	    ptr ++;

	subdir = FindNode (&node->subdirs, dirname);

	if (!subdir)
	    break;

	node = subdir;
    }

    return subdir;
}

DirNode *
adddirnode (DirNode * topnode, const char * path)
{
    char pathcopy[256], * pathptr;
    const char * ptr;
    char dirname[256];
    int len;
    DirNode * node = topnode, * subdir;
    struct stat st;

    ptr = path+2;
    pathptr = pathcopy;

    if (!*ptr)
	return node;

    printf ("adddirnode(): adding %s\n", path);

    subdir = NULL;

    while (*ptr)
    {
	for (len=0; ptr[len] && ptr[len] != '/'; len++);

	strncpy (dirname, ptr, len);
	dirname[len] = 0;
	if (pathptr != pathcopy)
	    *pathptr ++ = '/';
	strncpy (pathptr, ptr, len);
	pathptr[len] = 0;
	ptr += len;
	while (*ptr == '/')
	    ptr ++;

	subdir = FindNode (&node->subdirs, dirname);

	if (!subdir)
	{
	    subdir = new (DirNode);
	    NewList (&subdir->subdirs);
	    subdir->node.name = xstrdup (dirname);
	    subdir->parent = node;
	    stat (pathcopy, &st);
	    subdir->time = st.st_mtime;

	    AddTail(&node->subdirs, subdir);

	    node = subdir;
	}

	node = subdir;
    }

#if 0
    printf ("adddirnode()\n");
    printdirnode (topnode);
#endif

    return subdir;
}

DirNode *
readcachedir (FILE * fh)
{
    int len, ret;
    DirNode * node, * subnode;
    time_t tt;

    ret = fread (&len, sizeof(len), 1, fh);
    if (ferror (fh))
    {
	error ("readcachedir:fread():%d",
	    __LINE__
	);
	return NULL;
    }
    len = ntohl (len);

    if (len < 0)
	return NULL;

    node = new (DirNode);
    NewList(&node->subdirs);
    node->node.name = xmalloc (len+1);
    node->parent = NULL;

    if (len)
    {
	ret = fread (node->node.name, len, 1, fh);
	if (ferror (fh))
	{
	    error ("readcachedir:fread():%d",
		__LINE__
	    );
	    free (node);
	    return NULL;
	}
    }
    node->node.name[len] = 0;
    ret = fread (&tt, sizeof (tt), 1, fh);
    if (ferror (fh))
    {
	error ("readcachedir:fread():%d",
	    __LINE__
	);
	free (node);
	return NULL;
    }
    node->time = ntohl (tt);

    while ((subnode = readcachedir (fh)))
    {
	subnode->parent = node;
	AddTail (&node->subdirs, subnode);
    }

    return node;
}

int
writecachedir (FILE * fh, DirNode * node)
{
    int len, ret, out;
    DirNode * subnode;

    len = strlen (node->node.name);
    out = htonl (len);
    ret = fwrite (&out, sizeof(out), 1, fh);
    if (ret <= 0)
    {
	error ("writecachedir/fwrite():%d",
	    __LINE__
	);
	return ret;
    }

    if (len)
    {
	ret = fwrite (node->node.name, len, 1, fh);
	if (ret <= 0)
	{
	    error ("writecachedir/fwrite():%d",
		__LINE__
	    );
	    return ret;
	}
    }
    out = htonl (node->time);
    ret = fwrite (&out, sizeof (out), 1, fh);
    if (ret <= 0)
    {
	error ("writecachedir/fwrite():%d",
	    __LINE__
	);
	return ret;
    }

    ForeachNode (&node->subdirs, subnode)
    {
	ret = writecachedir (fh, subnode);
	if (ret <= 0)
	    return ret;
    }

    out = htonl (-1);
    ret = fwrite (&out, sizeof (out), 1, fh);
    if (ret <= 0)
    {
	error ("writecachedir/fwrite():%d",
	    __LINE__
	);
	return ret;
    }

    return ret;
}
