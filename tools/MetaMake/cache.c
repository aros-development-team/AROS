/* MetaMake - A Make extension
   Copyright © 1995-2009, The AROS Development Team. All rights reserved.

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

//#define DEBUG_CACHE

#include "config.h"

#ifdef HAVE_STRING_H
#   include <string.h>
#else
#   include <strings.h>
#endif
#ifdef HAVE_SYS_STAT_H
#   include <sys/stat.h>
#endif
#ifdef HAVE_NETINET_IN_H
#   include <netinet/in.h> /* for htonl/ntohl() */
#endif
#include <errno.h>
#include <assert.h>
#include <unistd.h>
#include <stdlib.h>
#ifndef PATH_MAX
#include <limits.h>
#endif

#include "cache.h"
#include "mem.h"
#include "io_.h"
#include "var.h"
#include "dep.h"
#include "mmake.h"
#include "win32.h"

#if defined(DEBUG_CACHE)
#define debug(a) a
#else
#define debug(v)
#endif

#define MAJOR		0L
#define MINOR		9L
#define REVISION	0L
#define ID		((MAJOR << 24) | (MINOR << 16) | REVISION)
#define CHECK_ID(id)    (((id) & 0xFFFF0000) == ((ID) & 0xFFFF0000))

struct Cache_priv
{
    struct Cache publicpart;
    struct Project * project;
    struct DirNode * topdir;
    struct List addedfiles;
};

struct Regenerate
{
    struct Node node;

    char *dir;
    char *src;
    char *dest;
};


static void
freetarget (struct Target * target)
{
    xfree (target->node.name);

    freelist (&target->makefiles);

    xfree (target);
}

static void
freetargetlist (struct Cache_priv * cache)
{
    struct Node * node, * next;

    ForeachNodeSafe(&cache->publicpart.targets,node,next)
    {
	Remove (node);
	freetarget ((struct Target *)node);
    }
}

static void
printtargetlist (struct List * l)
{
    struct Target * n;

    ForeachNode (l,n)
    {
	struct List deps;
	struct MakefileRef * mfref;
	struct MakefileTarget * mftarget;
	struct Node * node;

	printf ("target %s:\n", n->node.name);
	printf ("  updated=%d\n", n->updated);
	printf ("  makefiles=\n");
	ForeachNode (&n->makefiles, mfref)
	    printf("    \"%s/%s\"\n",
		    buildpath(mfref->makefile->dir),
		    mfref->makefile->node.name
		  );

	printf ("  deps=\n");
	NewList (&deps);
	ForeachNode (&n->makefiles, mfref)
	{
	    mftarget = FindNode (&mfref->makefile->targets, n->node.name);
	    ForeachNode (&mftarget->deps, node)
		addnodeonce (&deps, node->name);
	}

	printlist (&deps);
	freelist (&deps);
    }
}


static int progcount;

static void
progress_reset (FILE * fh)
{
    progcount = 0;
    fprintf(fh, "\n");
}

static void
progress (FILE * fh)
{
    progcount++;
    if (progcount == 20)
    {
	progcount = 0;
	fprintf(fh, ".");
	fflush (fh);
    }
}


void
readcache (struct Cache_priv * cache)
{
    char path[256];
    FILE * fh;
    uint32_t id;

    strcpy (path, cache->project->buildtop);
    strcat (path, "/mmake.cache");
    assert (strlen(path) < sizeof(path));

    fh = fopen (path, "rb");

    if (fh)
    {
	if (!readuint32 (fh, &id) || !CHECK_ID(id))
	{
	    fclose (fh);
	    fh = NULL;
	}
    }

    if (fh)
    {
	char * name;

	do
	{
	    if (!readstring (fh, &name))
	    {
		fh = NULL;
		break;
	    }

	    if (name == NULL)
		continue;

	    addnodeonce (&cache->addedfiles, name);
	    xfree (name);
	}
	while (name != NULL);

	if (!(cache->topdir = readcachedir (fh)))
	{
	    fclose (fh);
	    fh = NULL;
	}
    }

    if (!fh)
    {
	cache->topdir = newnodesize (cache->project->srctop, sizeof (struct DirNode));
	cache->topdir->parent = NULL;
	NewList(&cache->topdir->subdirs);
	NewList(&cache->topdir->makefiles);

	/* Force a check the first time */
	cache->topdir->time = 0;
    }

    if (fh)
	fclose (fh);

    if (debug)
    {
	printf ("readcache()\n");
	printdirnode (cache->topdir, 1);
    }
}

void
writecache (struct Cache_priv * cache)
{
    int ok = 1;
    char path[256];
    FILE * fh = NULL;
    uint32_t id;
    struct Node *addedfile;

    debug(printf("MMAKE:cache.c->writecache()\n"));

    if (!cache->topdir)
	return;

    strcpy (path, cache->project->buildtop);
    strcat (path, "/mmake.cache");
    assert (strlen(path) < sizeof(path));

    fh = fopen (path, "wb");

    if (!fh)
    {
	ok = 0;
	goto writecacheend;
    }

    ok = writeuint32 (fh, ID);
    if (!ok)
	goto writecacheend;

    ForeachNode (&cache->addedfiles, addedfile)
    {
	ok = writestring (fh, addedfile->name);
	if (!ok)
	{
	    error ("writecache/writestring():%d", __LINE__);
	    goto writecacheend;
	}
    }

    ok = writestring (fh, NULL);
    if (!ok)
    {
	error("writecache/fwrite():%d", __LINE__);
	goto writecacheend;
    }

    ok = writecachedir (fh, cache->topdir);

writecacheend:
    if (fh)
	fclose (fh);

    if (!ok)
    {
	unlink (path);

	printf ("[MMAKE] Warning: Creating the cache failed\n");
    }
}


void
checknewsrc (struct Cache_priv * cache, struct Makefile * makefile, struct List * regeneratefiles)
{
    char * mfsrc = xmalloc (strlen (makefile->node.name) + 5);
    char * mfdst = xmalloc (strlen (mm_builddir) + 1 + strlen (buildpath(makefile->dir)) + 1 + strlen (makefile->node.name) + 1);
    struct stat sst, dst;

    debug(printf("MMAKE:cache.c->checknewsrc('%s')\n", makefile->node.name));

    strcpy (mfsrc, makefile->node.name);
    strcat (mfsrc, ".src");

    if (stat (mfsrc, &sst) == -1)
    {
        debug(printf("MMAKE/cache.c:stat(\"%s\", ...) failed\n", mfsrc));
	xfree (mfsrc);
	return;
    }

    strcpy (mfdst, mm_builddir);
    strcat (mfdst, "/");
    strcat (mfdst, buildpath(makefile->dir));
    strcat (mfdst, "/");
    strcat (mfdst, makefile->node.name);

    if (stat (mfdst, &dst) == -1
	    || sst.st_mtime > dst.st_mtime
	    || checkdeps (&cache->project->genmakefiledeps, dst.st_mtime)
       )
    {
	static char currdir[PATH_MAX];
	struct Regenerate *reg = new (struct Regenerate);

	ASSERT(getcwd(currdir, PATH_MAX) != NULL);
	reg->dir = xstrdup (buildpath(makefile->dir));
	reg->src = mfsrc;
	reg->dest = xstrdup (makefile->node.name);

        debug(printf("MMAKE/cache.c:Added \"%s\" to be regenerated from \"%s\" in \"%s\"\n",
                     reg->dest, reg->src, reg->dir
        ));
	AddTail (regeneratefiles, reg);
    }
    else
    {
        debug(printf("MMAKE/cache.c:\"%s\" Not regenerated\n", makefile->node.name));
	xfree (mfsrc);
    }
}


int
updatemflist (struct Cache_priv * cache, struct DirNode * node, struct List * regeneratefiles)
{
    struct DirNode * subdir;
    struct Makefile * makefile;
    int goup = 0, reread = 0;
    char curdir[1024];

    debug(printf("MMAKE:cache.c->updatemflist(\"%s\")\n", node->node.name));

    if (strlen(node->node.name) != 0)
    {
	if (getcwd(curdir, sizeof(curdir)) == NULL)
	{
	    error("Could not get current directory");
	    exit (20);
	}
	if (chdir(node->node.name) < 0)
	{
	    error("Could not change to dir '%s'", node->node.name);
	    exit (20);
	}
	goup = 1;
    }

    if (scandirnode(node, cache->project->defaultmakefilename, &cache->project->ignoredirs))
	reread ++;

    ForeachNode(&node->subdirs, subdir)
    {
	debug(printf("MMAKE:cache.c->updatemflist: checking subdir ..\n"));
	reread += updatemflist(cache, subdir, regeneratefiles);
    }

    ForeachNode(&node->makefiles, makefile)
    {
	debug(printf("MMAKE:cache.c->updatemflist: checking makefile ..\n"));
	checknewsrc(cache, makefile, regeneratefiles);
    }

    if (goup)
	ASSERT(chdir(curdir) == 0);

    progress (stdout);

    return reread;
}

int
updatetargetlist (struct Project * prj, struct Cache_priv * cache, struct DirNode * node)
{
    struct DirNode * subdir;
    int reread = 0;

    debug(printf("MMAKE:cache.c->updatetargetlist('%s')\n", node->node.name));

    reread = scanmakefiles(prj, node, &cache->project->vars);

    ForeachNode(&node->subdirs, subdir)
	reread += updatetargetlist(prj, cache, subdir);

    progress (stdout);

    return reread;
}

#ifdef _WIN32

int mktempfile(char **name)
{
    char *temp = getenv("TEMP");
    size_t pathlen = strlen(temp) + 13;
    char *path = xmalloc(pathlen);

    strcpy(path, temp);
    strcat(path, "\\genmfXXXXXX");
    *name = path;
    return mkstemps(path, 0);
}

#else

int mktempfile(char **name)
{
    static char tmpname[20];

    strcpy(tmpname, "/tmp/genmfXXXXXX");
    *name = tmpname;
    return mkstemp(tmpname);
}

#endif


void
regeneratemf (struct Cache_priv * cache, struct List * regeneratefiles)
{
    struct Regenerate * reg,* reg2;
    char *tmpname;
    int fd;
    FILE *f;

    debug(printf("MMAKE:cache.c->regeneratemf()\n"));

    if (GetHead (regeneratefiles) == NULL)
	return;

    fd = mktempfile(&tmpname);
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
	char * mfsrc = xmalloc (strlen(cache->project->srctop) + strlen(reg->dir) + strlen(reg->src) + 3);
	char * mfdst = xmalloc (strlen(cache->project->buildtop) + strlen(reg->dir) + strlen(reg->dest) + 3);

	strcpy (mfsrc, cache->project->srctop);
	if (strlen(reg->dir) > 0)
	{
	    strcat (mfsrc, "/");
	    strcat (mfsrc, reg->dir);
	}
	strcat (mfsrc, "/");
	strcat (mfsrc, reg->src);

	strcpy (mfdst, cache->project->buildtop);
	if (strlen(reg->dir) > 0)
	{
	    strcat (mfdst, "/");
	    strcat (mfdst, reg->dir);
	}
	strcat (mfdst, "/");
	strcat (mfdst, reg->dest);

	debug(printf("MMAKE:cache.c->regeneratemf: regenerate '%s' as '%s'\n", mfsrc, mfdst));

	fprintf (f, "%s %s\n", mfsrc, mfdst);
	Remove (reg);
	xfree (mfsrc);
	xfree (mfdst);

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

    debug(printf("MMAKE:cache.c->regeneratemf: Finished\n"));

    unlink (tmpname);
}

void 
buildtargetlist (struct Cache_priv * cache, struct DirNode * node)
{
    struct Makefile * makefile;
    struct MakefileRef * mfref;
    struct MakefileTarget * mftarget;
    struct DirNode * subdir;
    struct Target * target;
    struct Node * n;

    debug(printf("MMAKE:cache.c->buildtargetlist()\n"));

    ForeachNode (&node->makefiles, makefile)
    {
	ForeachNode (&makefile->targets, mftarget)
	{
	    if (strchr (mftarget->node.name, '$') != NULL)
	    {
		char * s = substvars(&cache->project->vars, mftarget->node.name);
		SETSTR (mftarget->node.name, s);
	    }

	    ForeachNode (&mftarget->deps, n)
	    {
		if (strchr (n->name, '$') != NULL)
		{
		    char * s = substvars(&cache->project->vars, n->name);
		    SETSTR (n->name, s);
		}
	    }

	    target = FindNode (&cache->publicpart.targets, mftarget->node.name);

	    if (target ==  NULL)
	    {
		target = newnodesize (mftarget->node.name, sizeof(struct Target));
		target->updated = 0;
		NewList (&target->makefiles);
		AddTail (&cache->publicpart.targets, target);
	    }

	    mfref = newnodesize ("", sizeof(struct MakefileRef));
	    mfref->virtualtarget = mftarget->virtualtarget;
	    mfref->makefile = makefile;
	    AddTail (&target->makefiles, mfref);
	}
    }

    ForeachNode (&node->subdirs, subdir)
	buildtargetlist (cache, subdir);
}

struct Cache *
activatecache (struct Project *prj)
{
    struct Cache_priv * cache;
    struct List regeneratefiles;
    struct Node * addedfile, * extrafile;
    struct Makefile * makefile;
    struct List newadded;
    int reread;

    debug(printf("MMAKE:cache.c->activatecache(Project @ %x)\n", prj));

    cache = new (struct Cache_priv);
    if (!cache)
	return NULL;
    NewList (&regeneratefiles);

    cache->project = prj;

    debug(printf("MMAKE:cache.c->activatecache: Cache @ %x for Project @ %x\n", cache, prj));

    NewList (&cache->addedfiles);
    NewList (&cache->publicpart.targets);

    readcache (cache);

    debug(printf("MMAKE:cache.c->activatecache: Cache read.\n"));

    progress_reset (stdout);
    printf ("[MMAKE] Scanning dirs...\n");
    reread = updatemflist (cache, cache->topdir, &regeneratefiles);

    debug(printf("MMAKE:cache.c->activatecache: Updated MF list.\n"));

    if (verbose)
	printf ("[MMAKE] Reread %d dirs\n", reread);

    if (debug)
    {
	printf ("[MMAKE] Directory tree for project %s\n", prj->node.name);
	printdirnode (cache->topdir, 0);
    }

    /* Add the extra makefiles to the tree if needed */
    ASSERT(chdir (cache->project->buildtop) == 0);
    NewList (&newadded);
    ForeachNode (&cache->project->extramakefiles, extrafile)
    {
	addedfile = FindNode (&cache->addedfiles, extrafile->name);
	if (addedfile == NULL)
	{
	    makefile = addmakefile (cache->topdir, extrafile->name);

	    if (makefile == NULL)
	    {
		error("Could not add makefile \"%s\"", extrafile->name);
		exit (20);
	    }

	    addnodeonce (&newadded, extrafile->name);
	}
	else /* addedfile != NULL => was already added before */
	{
	    makefile = findmakefile (cache->topdir, extrafile->name);

	    if (makefile == NULL)
	    {
		error("Makefile \"%s\" has disappeared", extrafile->name);
		exit (20);
	    }

	    Remove (addedfile);
	    AddTail (&newadded, addedfile);
	}
    }
    ForeachNode (&cache->addedfiles, addedfile)
    {
	makefile = findmakefile (cache->topdir, addedfile->name);
	if (makefile != NULL)
	{
	    Remove (makefile);
	    freemakefile (makefile);
	}
    }
    AssignList (&cache->addedfiles, &newadded);

    regeneratemf (cache, &regeneratefiles);

    debug(printf("MMAKE:cache.c->activatecache: Regenerated MFs\n"));

    progress_reset (stdout);
    printf ("[MMAKE] Scanning makefiles...\n");
    reread = updatetargetlist (prj, cache, cache->topdir);
    if (verbose)
	printf ("[MMAKE] Reread %d makefiles\n", reread);
    if (debug)
    {
	printf ("[MMAKE] Makefile and target tree for project %s\n", prj->node.name);
	printdirnodemftarget (cache->topdir);
    }

    writecache (cache);

    progress_reset (stdout);
    printf ("[MMAKE] Collecting targets...\n");
    buildtargetlist (cache, cache->topdir);
    if (debug)
    {
	printf ("[MMAKE] Targetlist of project %s\n", prj->node.name);
	printtargetlist (&cache->publicpart.targets);
    }

    debug(printf("MMAKE:cache.c->activatecache: Finished\n"));

    return (struct Cache *)cache;
}

void
closecache (struct Cache * gl_cache)
{
    struct Cache_priv * cache = (struct Cache_priv *)gl_cache;

    freetargetlist (cache);
}
