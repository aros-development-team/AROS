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
#ifdef HAVE_STRING_H
#   include <string.h>
#else
#   include <strings.h>
#endif
#ifdef HAVE_SYS_STAT_H
#   include <sys/stat.h>
#endif
#include <errno.h>
#include <assert.h>

#include "cache.h"
#include "mem.h"
#include "var.h"
#include "dep.h"

#define MAJOR		0L
#define MINOR		6L
#define REVISION	1L
#define ID		((MAJOR << 24) | (MINOR << 16) | REVISION)
#define CHECK_ID(id)    (((id) & 0xFFFF0000) == ((ID) & 0xFFFF0000))

#define FLAG_VIRTUAL	0x0001

extern int debug;

typedef struct {
    Cache publicpart;
    
    Project * project;

    DirNode * topdir;

    int buildmflist;
    int buildtargetlist;

    List makefiles;
}
Cache_priv;

typedef struct
{
    Node node;

    char *dir;
    char *src;
    char *dest;
}
Regenerate;


static void
freetarget (Target * target)
{
    xfree (target->node.name);

    freelist (&target->makefiles);
    freelist (&target->deps);

    xfree (target);
}

static void
freetargetlist (Cache_priv * cache)
{
    Node * node, * next;

    ForeachNodeSafe(&cache->publicpart.targets,node,next)
    {
	Remove (node);
	freetarget ((Target *)node);
    }
}

static void
printtargetlist (List * l)
{
    Target * n;

    ForeachNode (l,n)
    {
	printf ("target %s:\n", n->node.name);
	printf ("  updated=%d\n", n->updated);
	printf ("  makefiles=\n");
	printlist (&n->makefiles);
	printf ("  deps=\n");
	printlist (&n->deps);
    }
}


void
progress (int max, int curr, int * data)
{
    int x = curr*10/max;

    if (x != *data)
    {
	*data = x;
	putchar ('.');
	fflush (stdout);
    }
}


void
readcache (Cache_priv * cache)
{
    char path[256];
    FILE * fh;
    long id;

    strcpy (path, cache->project->top);
    strcat (path, "/mmake.cache");
    assert (strlen(path) < sizeof(path));

    fh = fopen (path, "r");

    if (fh)
    {
	fread (&id, sizeof(id), 1, fh);
	if (!CHECK_ID(id))
	{
	    fclose (fh);
	    fh = NULL;
	}
    }

    if (fh)
    {
	cache->topdir = readcachedir (fh);

	if (!cache->topdir)
	{
	    fclose (fh);
	    fh = NULL;
	}
    }

    if (!fh)
    {
	cache->topdir = new (DirNode);
	NewList(&cache->topdir->subdirs);
	cache->topdir->node.name = xstrdup ("");
	cache->topdir->parent = NULL;

	/* Force a check the first time */
	cache->topdir->time = 0;
    }

    if (fh)
	fclose (fh);

#if 0
    printf ("readcache()\n");
    printdirnode (cache->topdir);
#endif
}

void
writecache (Cache_priv * cache)
{
    char path[256];
    FILE * fh;
    int ret;
    long id;

    if (!cache->topdir)
	return;

    strcpy (path, cache->project->top);
    strcat (path, "/mmake.cache");
    assert (strlen(path) < sizeof(path));

    fh = fopen (path, "w");

    if (!fh)
	return;

    id = ID;
    fwrite (&id, sizeof (id), 1, fh);

    ret = writecachedir (fh, cache->topdir);

    fclose (fh);

    if (ret <= 0)
    {
	unlink (path);

	printf ("Warning: Creating the cache failed\n");
    }
}


void
updatemakefile (Cache_priv * cache, const char * path, List * regeneratefiles)
{
    char * mf = xmalloc (strlen (path) + 4);
    char * ptr, * dir, * file, * ext;
    char * dest, * src;
    int len;
    struct stat sst, dst;

    strcpy (mf, path);
    len = strlen (mf) - 4;
    if (len > 0 && strcmp (mf+len, ".src"))
	strcat (mf, ".src");

    ptr = dir = mf;
    file = NULL;
    while (*ptr)
    {
	if (*ptr == '/')
	    file = ptr+1;

	ptr ++;
    }
    if (!file)
    {
	dir = ".";
	file = mf;
    }
    ptr = file;
    ext = NULL;
    while (*ptr)
    {
	if (*ptr == '.')
	    ext = ptr+1;

	ptr ++;
    }

    chdir (cache->project->top);
    if (file != mf)
    {
	file[-1] = 0;
	chdir (dir);
    }

    ext[-1] = 0;
    dest = xstrdup (file);
    ext[-1] = '.';

    src = xstrdup (file);
    if (stat (src, &sst) == -1)
    {
	xfree (dest);
	xfree (mf);
	xfree (src);
	return;
    }

    if (stat (dest, &dst) == -1
	|| sst.st_mtime > dst.st_mtime
	|| checkdeps (&cache->project->genmakefiledeps, dst.st_mtime)
    )
    {
	Regenerate *reg = new (Regenerate);

	reg->dir = xstrdup (dir);
	reg->src = src;
	reg->dest = dest;

	AddTail (regeneratefiles, reg);
    }
    else
    {
	xfree (dest);
	xfree (src);
    }

    xfree (mf);
}


void
updatemflist (Cache_priv * cache, List * regeneratefiles)
{
    char mfnsrc[256];
    Node * makefile;
    struct stat st;
  
    ForeachNode(&cache->makefiles, makefile)
    {
        strcpy(mfnsrc, makefile->name);
        strcat(mfnsrc, ".src");
        assert(strlen(mfnsrc)<256);
      
        if (!stat(mfnsrc, &st))
	    updatemakefile(cache, mfnsrc, regeneratefiles);
    }
}


void
buildmflist (Cache_priv * cache, List * regeneratefiles)
{
    char * mfn, * mfnsrc;
    struct stat st;
    char path[256];
    int len, offset;
    List dirs;
    Node * cd;
    DIR * dirh;
    struct dirent * dirent;
    int foundmf;
    DirNode * dnode;
    int done, todo, nummfs, reread;
    Node * tmpnode;
    time_t tt, now;

    if (!cache->buildmflist)
	return;

    cache->buildmflist = 0;

    printf ("Collecting makefiles...\n");

    mfnsrc = xmalloc (strlen(mfn=cache->project->defaultmakefilename)+5);
    strcpy (mfnsrc, mfn);
    len = strlen (mfn);
    strcpy (mfnsrc+len, ".src");

    NewList(&dirs);
    cd = newnode (".");
    AddTail(&dirs,cd);

    done = nummfs = reread = 0;

    time (&tt);
    now = 0;

    while ((cd = GetHead(&dirs)))
    {
	todo = 0;
	ForeachNode (&dirs, tmpnode)
	    todo ++;

	time (&now);
	if (now != tt)
	{
	    printf ("Done: %4d   Todo: %4d\r", done, todo);
	    fflush (stdout);
	    tt = now;
	}

	Remove (cd);

	chdir (cache->project->top);

	strcpy (path, cd->name);
	offset = strlen (path);
	path[offset ++] = '/';
	path[offset] = 0;

#if 0
    printf ("Entering \"%s\"\n", path);
#endif

	dnode = finddirnode (cache->topdir, path);

	if (dnode)
	{
	    stat (path, &st);
	}

	foundmf = 0;

	if (!dnode || st.st_mtime > dnode->time)
	{
#if 0
    printf ("Updating cache in %s\n", path);
#endif
	    if (!dnode)
		dnode = adddirnode (cache->topdir, path);

	    dnode->time = st.st_mtime;

	    reread ++;

	    dirh = opendir (path);
	    if (!dirh)
	    {
		error ("opendir(%s)", path);
		exit (10);
	    }

	    while ((dirent = readdir (dirh)))
	    {
		if (!strcmp (dirent->d_name, mfnsrc))
		{
		    foundmf = 2;
		    continue;
		}

		if (!foundmf)
		{
		    mfnsrc[len] = 0;

		    if (!strcmp (dirent->d_name, mfnsrc))
		    {
			foundmf = 1;
			mfnsrc[len] = '.';
			continue;
		    }

		    mfnsrc[len] = '.';
		}

		strcpy (path+offset, dirent->d_name);

		if (lstat (path, &st) == -1)
		{
		    error ("stat(%s)", path);
		    exit (10);
		}

		if (S_ISDIR (st.st_mode)
		    && strcmp (dirent->d_name, ".")
		    && strcmp (dirent->d_name, "..")
		    && !S_ISLNK (st.st_mode)
		    && !FindNode (&cache->project->ignoredirs, dirent->d_name)
		)
		{
		    addnodeonce (&dirs, path);
#if 0
    printf ("Adding %s for later\n", path);
#endif
		}

		path[offset] = 0;
	    }

	    closedir (dirh);
	}
	else
	{
	    DirNode * subdir, * nextsubdir;

	    chdir (path);

	    if (stat (mfnsrc, &st) != -1)
	    {
		foundmf = 2;
	    }
	    else
	    {
		mfnsrc[len] = 0;

		if (stat (mfnsrc, &st) != -1)
		    foundmf = 1;

		mfnsrc[len] = '.';
	    }

	    chdir (cache->project->top);

	    ForeachNodeSafe (&dnode->subdirs, subdir, nextsubdir)
	    {
		strcpy (path+offset, subdir->node.name);

		if (stat (path, &st) == -1 && errno == ENOENT)
		{
#if 0
    printf ("Removing %s from cache (%s)\n", path, subdir->node.name);
    printf ("top=%s\n", cache->project->top);
    printdirnode (cache->topdir, 1);
#endif
		    Remove (subdir);
		    freecachenodes (subdir);
		}
		else
		{
#if 0
    printf ("Adding %s for later\n", path);
#endif
		    addnodeonce (&dirs, path);
		}
	    }

	    path[offset] = 0;
	}

	if (foundmf == 2)
	{
	    strcpy (path+offset, mfnsrc);
	    updatemakefile (cache, path, regeneratefiles);
	    foundmf --;
	}

	if (foundmf == 1)
	{
	    mfnsrc[len] = 0;
	    strcpy (path+offset, mfnsrc);
	    addnodeonce (&cache->makefiles, path+2);
	    path[offset] = 0;
	    mfnsrc[len] = '.';
	    nummfs ++;
	}

	free (cd->name);
	free (cd);
	done ++;
    }

    todo = 0;
    printf ("Done: %4d   Todo: %4d\n", done, todo);
    printf ("Found %d makefiles, reread %d dirs\n", nummfs, reread);

    chdir (cache->project->top);

    xfree (mfnsrc);

    if (debug)
    {
	printf ("project %s.makefiles=\n", cache->project->node.name);
	printlist (&cache->makefiles);
    }
    writecache (cache);
}


void
regeneratemf (Cache_priv * cache, List * regeneratefiles)
{
    Regenerate * reg,* reg2;
    char tmpname[20];
    int fd;
    FILE *f;

    if (GetHead (regeneratefiles) == NULL)
	return;
    
    strcpy (tmpname, "/tmp/genmfXXXXXX");
    fd = mkstemp (tmpname);
    if (fd < 0)
    {
	error ("Could not create temporary file %s", tmpname);
	exit (10);
    }
    else
    {
	f = fdopen (fd, "w");
	if (f == NULL)
	{
	    error ("Could not open temporary file %s", tmpname);
	    exit (10);
	}
    }
    
    ForeachNodeSafe (regeneratefiles, reg, reg2)
    {
	fprintf (f, "%s/%s %s/%s\n", reg->dir, reg->src, reg->dir, reg->dest);
	Remove (reg);
	xfree (reg->dir);
	xfree (reg->src);
	xfree (reg->dest);
	xfree (reg);
    }

    fclose (f);

    setvar (&cache->project->vars, "MMLIST", tmpname);
    if (!execute (cache->project, cache->project->genmakefilescript,"-","-",""))
    {
	error ("Error regenerating makefile");
	exit (10);
    }

    unlink (tmpname);
}


Target *
appendtarget (Cache_priv * cache, const char * tname, const char * mf, char ** deps, int flags)
{
    Target   * target;
    Makefile * makefile;

    assert (tname);
    assert (mf);

    target = FindNode (&cache->publicpart.targets, tname);

    if (!target)
    {
	target = new (Target);
	target->node.name = strdup (tname);
	AddTail(&cache->publicpart.targets, target);
	NewList(&target->makefiles);
	NewList(&target->deps);
	target->updated = 0;
    }

    if (!(makefile = FindNode (&target->makefiles, mf)) )
    {
	makefile = new (Makefile);
	makefile->node.name = xstrdup (mf);
	makefile->virtualtarget = ((flags & FLAG_VIRTUAL) != 0);

	AddTail (&target->makefiles, makefile);
    }
    else
    {
	makefile->virtualtarget = ((flags & FLAG_VIRTUAL) != 0);
    }

#if 0
printf ("add %s.%s mf=%s\n", prj->node.name, target->node.name, mf);
#endif

    if (deps)
    {
	while (*deps)
	{
#if 0
printf ("   add dep %s\n", *deps);
#endif
	    addnodeonce (&target->deps, *deps);
	    deps ++;
	}
    }

    return target;
}


void
buildtargetlist (Cache_priv * cache)
{
    int max, pos, data;
    Node * mfnode;
    FILE * fh;
    char line[256];
    int lineno;

    if (!cache->buildtargetlist)
	return;

    cache->buildtargetlist = 0;

    printf ("Collecting metatargets...");

    max=0;
    ForeachNode(&cache->makefiles,mfnode) max++;
    pos=data=0;

    ForeachNode(&cache->makefiles,mfnode)
    {
	int contline = 0;
	int flags = 0;

	pos++;
	progress (max,pos,&data);

#if 0
printf ("Opening %s\n", mfnode->name);
#endif

	fh = fopen (mfnode->name, "r");

	if (!fh)
	{
	    error ("buildtargetlist:fopen():%d: Opening %s for reading",
		__LINE__, mfnode->name
	    );
	}

	lineno = 0;

	while (fgets (line, sizeof(line), fh))
	{
	    char * targets[64], ** tptr;

	    lineno ++;

	    if (!strncmp (line, "#MM", 3))
	    {
		char * ptr;
		char * depptr, ** deps;
		int count, depc, t;
		int cl;

#if 0
printf ("found #MM in %s\n", mfnode->name);
#endif

		ptr = line+3;

		if (!contline)
		{
		    if (*ptr == '-')
		    {
			flags |= FLAG_VIRTUAL;
			ptr ++;
		    }
		    else
			flags &= ~FLAG_VIRTUAL;
		}

		depptr = ptr + strlen (ptr) - 2;

		cl = (*depptr == '\\');

		if (cl)
		    *depptr = 0;

		ptr = substvars (&cache->project->vars, ptr);

		/* Must be *after* substvars() or empty target lines
		   will cause problems. */
		while (isspace (*ptr))
		    ptr ++;

		if (!*ptr)
		{
		    char ** targets;
		    fgets (line, sizeof(line), fh);
		    ptr = substvars (&cache->project->vars, line);

		    while (*ptr != ':' && *ptr)
			ptr ++;

		    *ptr = 0;

		    targets = getargs (line, &count, &cache->project->vars);

		    if (count != 0)
			appendtarget (cache, targets[0], mfnode->name, NULL, flags);
		    else
			printf ("Warning: Can't find metatarget in %s:%d\n", mfnode->name, lineno);
		}
		else
		{
		    char * lptr = ptr;

		    if (!contline)
		    {
			while (*ptr != ':' && *ptr)
			    ptr ++;
			if (*ptr)
			    *ptr ++ = 0;
			depptr = ptr;

			tptr = getargs (lptr, &count, &cache->project->vars);
			for (t=0; t<count; t++)
			    targets[t] = xstrdup (tptr[t]);
		    }
		    else
			depptr = ptr;

		    deps = getargs (depptr, &depc, &cache->project->vars);

		    for (t=0; t<count; t++)
		    {
			appendtarget (cache, targets[t], mfnode->name, deps, flags);
		    }

		    contline = cl;

		    if (!contline)
		    {
			for (t=0; t<count; t++)
			    xfree (targets[t]);
		    }
		}
	    } /* If this is a MetaMake line in the makefile */
	} /* For all lines in a makefile */

#if 0
printf ("Read %d lines\n", lineno);
#endif

	fclose (fh);
    } /* For all makefiles in the project */

    putchar ('\n');

    /* Clean up memory */
    getargs(NULL, NULL, NULL);
	
    if (debug)
    {
	printf ("%s.targets=\n", cache->project->node.name);
	printtargetlist (&cache->publicpart.targets);
    }
}


Cache *
activatecache (Project *prj)
{
    Cache_priv * cache;
    Node * n;
    List regeneratefiles;
    
    cache = new (Cache_priv);
    if (!cache)
	return NULL;
    NewList (&regeneratefiles);
    
    cache->project = prj;
    
    NewList (&cache->publicpart.targets);
    NewList (&cache->makefiles);

    cache->buildmflist = 1;
    cache->buildtargetlist = 1;
    
    /* Add the makefiles from the config file to the list of
     * makefiles to be included in the cache */
    ForeachNode (&prj->extramakefiles, n)
	AddTail (&cache->makefiles, newnode(n->name));
    
    readcache (cache);
    updatemflist (cache, &regeneratefiles);
    buildmflist (cache, &regeneratefiles);
    regeneratemf (cache, &regeneratefiles);
    buildtargetlist (cache);
    
    return (Cache *)cache;
}

void
closecache (Cache * gl_cache)
{
    Cache_priv * cache = (Cache_priv *)gl_cache;
    
    freetargetlist (cache);
    freelist (&cache->makefiles);
    
}
