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
#ifdef HAVE_NETINET_IN_H
#   include <netinet/in.h> /* for htonl/ntohl() */
#endif
#include <errno.h>
#include <assert.h>
#include <unistd.h>
#include <stdlib.h>

#include "cache.h"
#include "mem.h"
#include "var.h"
#include "dep.h"
#include "mmake.h"

#define MAJOR		0L
#define MINOR		9L
#define REVISION	0L
#define ID		((MAJOR << 24) | (MINOR << 16) | REVISION)
#define CHECK_ID(id)    (((id) & 0xFFFF0000) == ((ID) & 0xFFFF0000))


typedef struct {
    Cache publicpart;
    
    Project * project;

    DirNode * topdir;

    List addedfiles;
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
	List deps;
	MakefileRef * mfref;
	MakefileTarget * mftarget;
	Node * node;
	
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
static int token;
static char tokens[]="|/-\\";
			 
static void
progress_reset (FILE * fh)
{
    progcount = 0;
    token = 0;
    fprintf (fh, "\r|\r");
    fflush (fh);
}

static void
progress (FILE * fh)
{
    progcount++;
    if (progcount == 13)
    {
	progcount = 0;
	token++;
	if (token == 4)
	    token = 0;
	fprintf (fh, "%c\r", tokens[token]);
	fflush (fh);
    }
}


void
readcache (Cache_priv * cache)
{
    char path[256];
    FILE * fh;
    uint32_t id;

    strcpy (path, cache->project->top);
    strcat (path, "/mmake.cache");
    assert (strlen(path) < sizeof(path));

    fh = fopen (path, "r");

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
	
	while (name != NULL)
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

	if (fh)
	    cache->topdir = readcachedir (fh);
	else
	    cache->topdir = NULL;

	if (!cache->topdir)
	{
	    fclose (fh);
	    fh = NULL;
	}
    }

    if (!fh)
    {
	cache->topdir = newnodesize ("", sizeof (DirNode));
	cache->topdir->parent = NULL;
	NewList(&cache->topdir->subdirs);
	NewList(&cache->topdir->makefiles);
	
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
    int ok = 1;
    char path[256];
    FILE * fh = NULL;
    uint32_t id;
    Node *addedfile;
    
    if (!cache->topdir)
	return;

    strcpy (path, cache->project->top);
    strcat (path, "/mmake.cache");
    assert (strlen(path) < sizeof(path));

    fh = fopen (path, "w");

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

	printf ("Warning: Creating the cache failed\n");
    }
}


void
checknewsrc (Cache_priv * cache, Makefile * makefile, List * regeneratefiles)
{
    char * mfsrc = xmalloc (strlen (makefile->node.name) + 5);
    struct stat sst, dst;

    strcpy (mfsrc, makefile->node.name);
    strcat (mfsrc, ".src");

    if (stat (mfsrc, &sst) == -1)
    {
	xfree (mfsrc);
	return;
    }

    if (stat (makefile->node.name, &dst) == -1
	|| sst.st_mtime > dst.st_mtime
	|| checkdeps (&cache->project->genmakefiledeps, dst.st_mtime)
    )
    {
	static char currdir[PATH_MAX];
	Regenerate *reg = new (Regenerate);

	getcwd(currdir, PATH_MAX);
	reg->dir = xstrdup (currdir);
	reg->src = mfsrc;
	reg->dest = xstrdup (makefile->node.name);

	AddTail (regeneratefiles, reg);
    }
    else
    {
	xfree (mfsrc);
    }
}


int
updatemflist (Cache_priv * cache, DirNode * node, List * regeneratefiles)
{
    DirNode * subdir;
    Makefile * makefile;
    int goup = 0, reread = 0;

    if (strlen(node->node.name) != 0)
    {
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
	reread += updatemflist(cache, subdir, regeneratefiles);
    
    ForeachNode(&node->makefiles, makefile)
	checknewsrc(cache, makefile, regeneratefiles);

    if (goup)
	chdir("..");

    progress (stdout);
    
    return reread;
}

int
updatetargetlist (Cache_priv * cache, DirNode * node)
{
    DirNode * subdir;
    int goup = 0, reread = 0;

    if (strlen(node->node.name) != 0)
    {
	if (chdir(node->node.name) < 0)
	{
	    error("Could not change to dir '%s'", node->node.name);
	    exit (20);
	}
	goup = 1;
    }
    
    reread = scanmakefiles(node, &cache->project->vars);
    
    ForeachNode(&node->subdirs, subdir)
	reread += updatetargetlist(cache, subdir);

    progress (stdout);
    
    if (goup)
	chdir("..");
    
    return reread;
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

void 
buildtargetlist (Cache_priv * cache, DirNode * node)
{
    Makefile * makefile;
    MakefileRef * mfref;
    MakefileTarget * mftarget;
    DirNode * subdir;
    Target * target;
    Node * n;
    
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
		target = newnodesize (mftarget->node.name, sizeof(Target));
		target->updated = 0;
		NewList (&target->makefiles);
		AddTail (&cache->publicpart.targets, target);
	    }
	    
	    mfref = newnodesize ("", sizeof(MakefileRef));
	    mfref->virtualtarget = mftarget->virtualtarget;
	    mfref->makefile = makefile;
	    AddTail (&target->makefiles, mfref);
	}
    }
    
    ForeachNode (&node->subdirs, subdir)
	buildtargetlist (cache, subdir);
}

Cache *
activatecache (Project *prj)
{
    Cache_priv * cache;
    List regeneratefiles;
    Node * addedfile, * extrafile;
    Makefile * makefile;
    List newadded;
    int reread;
    
    cache = new (Cache_priv);
    if (!cache)
	return NULL;
    NewList (&regeneratefiles);
    
    cache->project = prj;

    NewList (&cache->addedfiles);
    NewList (&cache->publicpart.targets);

    readcache (cache);

    progress_reset (stdout);
    printf ("Scanning dirs...\n");
    reread = updatemflist (cache, cache->topdir, &regeneratefiles);
    if (verbose)
	printf ("Reread %d dirs\n", reread);
    if (debug)
    {
	printf ("Directory tree for project %s\n", prj->node.name);
	printdirnode (cache->topdir, 0);
    }

    /* Add the extra makefiles to the tree if needed */
    chdir (cache->project->top);
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

    progress_reset (stdout);
    printf ("Scanning makefiles...\n");
    reread = updatetargetlist (cache, cache->topdir);
    if (verbose)
	printf ("Reread %d makefiles\n", reread);
    if (debug)
    {
	printf ("Makefile and target tree for project %s\n", prj->node.name);
	printdirnodemftarget (cache->topdir);
    }
 
    writecache (cache);
    
    printf ("Collecting targets...\n");
    buildtargetlist (cache, cache->topdir);
    if (debug)
    {
	printf ("Targetlist of project %s\n", prj->node.name);
	printtargetlist (&cache->publicpart.targets);
    }
    
    return (Cache *)cache;
}

void
closecache (Cache * gl_cache)
{
    Cache_priv * cache = (Cache_priv *)gl_cache;
    
    freetargetlist (cache);
}
