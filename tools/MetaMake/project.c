/* MetaMake - A Make extension
   Copyright © 1995-2011, The AROS Development Team. All rights reserved.

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

//#define DEBUG_PROJECT

#include "config.h"

#include <unistd.h>
#include <stdlib.h>
#include <assert.h>
#ifdef HAVE_STRING_H
#   include <string.h>
#else
#   include <strings.h>
#endif

#include "project.h"
#include "var.h"
#include "mem.h"
#include "dep.h"
#include "mmake.h"

#if defined(DEBUG_PROJECT)
#define debug(a) a
#else
#define debug(v)
#endif

struct List projects;
static struct Project * defaultprj = NULL;

static void
readvars (struct Project * prj)
{
    struct List deps;
    struct Node * node, * next;
    struct Dep * dep;

    debug(printf("MMAKE:project.c->readvars(Project @ 0x%p)\n", prj));

    if (!prj->readvars)
	return;

    prj->readvars = 0;

    printf ("Read vars...\n");

    setvar (&prj->vars, "TOP", prj->buildtop);
    setvar (&prj->vars, "SRCDIR", prj->srctop);
    setvar (&prj->vars, "CURDIR", "");

    ForeachNode(&prj->globalvarfiles, node)
    {
	char * fn;
	FILE * fh;
	char line[256];
	char * name, * value, * ptr;

	fn = xstrdup (substvars (&prj->vars, node->name));
	fh = fopen (fn, "r");

	/* if the file doesn't exist execute prj->genglobalvarfile */
	if (!fh && prj->genglobalvarfile)
	{
	    char * gen = xstrdup (substvars (&prj->vars, prj->genglobalvarfile));

	    printf ("Generating %s...\n", fn);

	    if (!execute (prj, gen, "-", "-", ""))
	    {
		error ("Error while creating \"%s\" with \"%s\"", fn, gen);
		exit (10);
	    }
	    else
		fh = fopen (fn, "r");

	    xfree (gen);
	}

	if (!fh)
	{
	    error ("readvars():fopen(): Opening \"%s\" for reading", fn);
	    return;
	}

	xfree (fn);

	while (fgets (line, sizeof(line), fh))
	{
	    if (*line == '\n' || *line == '#') continue;
	    line[strlen(line)-1] = 0;

	    ptr = line;
	    while (isspace (*ptr)) ptr++;
	    name = ptr;
	    while (*ptr && !isspace(*ptr) && *ptr != ':' && *ptr != '=')
		ptr ++;

	    if (*ptr)
		*ptr++ = 0;

	    while (isspace(*ptr) || *ptr == ':' || *ptr == '=')
		ptr ++;

	    value = ptr;

	    while (*ptr && *ptr != '#')
		ptr ++;

	    *ptr = 0;

	    if (debug)
		printf ("%s=%s\n", name, substvars (&prj->vars, value));

	    setvar (&prj->vars, name, substvars (&prj->vars, value));
	}

	fclose (fh);
    }

    /* handle prj->genmakefiledeps */
    NewList(&deps);
    ForeachNodeSafe (&prj->genmakefiledeps, node, next)
    {
	Remove (node);
	AddTail (&deps, node);
    }

    ForeachNodeSafe (&deps, node, next)
    {
	Remove (node);
	dep = newdepnode (substvars (&prj->vars, node->name));
	AddTail (&prj->genmakefiledeps, dep);
	xfree (node->name);
	xfree (node);
    }

    if (debug)
    {
	printf ("project %s.genmfdeps=\n", prj->node.name);
	printlist (&prj->genmakefiledeps);
    }

    if (debug)
    {
	printf ("project %s.vars=", prj->node.name);
	printvarlist (&prj->vars);
    }
}

static struct Project *
initproject (char * name)
{
    struct Project * prj = new (struct Project);

    memset (prj, 0, sizeof(struct Project));

    debug(printf("MMAKE:project.c->initproject('%s')\n", name));
    debug(printf("MMAKE:project.c->initproject: Project node @ 0x%p\n", prj));

    if (!defaultprj)
    {
	prj->maketool = xstrdup ("make \"TOP=$(TOP)\" \"SRCDIR=$(SRCDIR)\" \"CURDIR=$(CURDIR)\"");
	prj->defaultmakefilename = xstrdup ("Makefile");
	prj->srctop = mm_srcdir;
	prj->buildtop = mm_builddir;
	prj->defaulttarget = xstrdup ("all");
	prj->genmakefilescript = NULL;
	prj->genglobalvarfile = NULL;
    }
    else
    {
	prj->maketool = xstrdup (defaultprj->maketool);
	prj->defaultmakefilename = xstrdup (defaultprj->defaultmakefilename);
	prj->srctop = xstrdup (defaultprj->srctop);
	prj->buildtop = xstrdup (defaultprj->buildtop);
	prj->defaulttarget = xstrdup (defaultprj->defaulttarget);
	SETSTR (prj->genmakefilescript, defaultprj->genmakefilescript);
	SETSTR (prj->genglobalvarfile, defaultprj->genglobalvarfile);
    }

    prj->node.name = xstrdup (name);

    prj->readvars = 1;

    NewList(&prj->globalvarfiles);
    NewList(&prj->genmakefiledeps);
    NewList(&prj->ignoredirs);
    NewList(&prj->vars);
    NewList(&prj->extramakefiles);

    return prj;
}

static void
freeproject (struct Project * prj)
{
    assert (prj);

    cfree (prj->node.name);
    cfree (prj->maketool);
    cfree (prj->defaultmakefilename);
    if (prj->srctop != mm_srcdir)
	cfree (prj->srctop);
    if (prj->buildtop != mm_builddir)
	cfree (prj->buildtop);
    cfree (prj->defaulttarget);
    cfree (prj->genmakefilescript);
    cfree (prj->genglobalvarfile);

    if (prj->cache)
	closecache (prj->cache);

    freelist(&prj->globalvarfiles);
    freelist (&prj->genmakefiledeps);
    freelist (&prj->ignoredirs);
    freevarlist (&prj->vars);
    freelist (&prj->extramakefiles);

    xfree (prj);
}

static void
callmake (struct Project * prj, const char * tname, struct Makefile * makefile)
{
    static char buffer[4096];
    const char * path = buildpath (makefile->dir);
    int t;

    debug(printf("MMAKE:project.c->callmake()\n"));

    if (makefile->generated)
	chdir (prj->buildtop);
    else
	chdir (prj->srctop);
    chdir (path);

    setvar (&prj->vars, "CURDIR", path);
    setvar (&prj->vars, "TARGET", tname);

    buffer[0] = '\0';

    for (t=0; t<mflagc; t++)
    {
	strcat (buffer, mflags[t]);
	strcat (buffer, " ");
    }

    if (strcmp (makefile->node.name, "Makefile")!=0 && strcmp (makefile->node.name, "makefile")!=0);
    {
	strcat (buffer, "--file=");
	strcat (buffer, makefile->node.name);
	strcat (buffer, " ");
    }

    strcat (buffer, tname);

    if (!quiet)
	printf ("Making %s in %s\n", tname, path);

    if (!execute (prj, prj->maketool, "-", "-", buffer))
    {
	error ("Error while running make in %s", path);
	exit (10);
    }
}


void
initprojects (void)
{
    char * optionfile;
    char * home;
    char line[256];
    FILE * optfh = NULL;
    struct Project * project;

    debug(printf("MMAKE:project.c->initprojects()\n"));

    NewList(&projects);
    defaultprj = project = initproject ("default");
    AddTail(&projects, project);


    /* Try "$MMAKE_CONFIG" */
    if ((optionfile = getenv ("MMAKE_CONFIG")))
	optfh = fopen (optionfile, "r");

    /* Try "$HOME/.mmake.config" */
    if (!optfh)
    {
	if ((home = getenv("HOME")))
	{
	    optionfile = xmalloc (strlen(home) + sizeof("/.mmake.config") + 1);
	    sprintf (optionfile, "%s/.mmake.config", home);
	    optfh = fopen (optionfile, "r");
	    free (optionfile);
	}
    }

    /* Try with $CWD/.mmake.config" */
    if (!optfh)
	optfh = fopen (".mmake.config", "r");

    /* Try with "$CWD/mmake.config */
    if (!optfh)
	optfh = fopen ("mmake.config", "r");

    /* Give up */
    if (!optfh)
    {
	fprintf (stderr,
		"Please set the HOME or MMAKE_CONFIG env var (with setenv or export)\n"
		);
	error ("Opening mmake.config for reading");
	exit (10);
    }

    while (fgets (line, sizeof(line), optfh))
    {
	if (*line == '\n' || *line == '#') continue;
	line[strlen(line)-1] = 0;

	if (*line == '[') /* look for project name */
	{
	    char * name, * ptr;

	    name = ptr = line+1;
	    while (*ptr && *ptr != ']')
		ptr ++;

	    *ptr = 0;

	    debug(printf("MMAKE:project.c->initprojects: Adding '%s' from MMAKE_CONFIG\n", name));

	    project = initproject (name);

	    AddTail(&projects,project);
	}
	else
	{
	    char * cmd, * args, * ptr;

	    cmd = line;
	    while (isspace (*cmd))
		cmd ++;

	    args = cmd;
	    while (*args && !isspace(*args))
	    {
		*args = tolower (*args);
		args ++;
	    }
	    if (*args)
		*args++ = 0;
	    while (isspace (*args))
		args ++;

	    ptr = args;

	    while (*ptr && *ptr != '\n')
		ptr ++;

	    *ptr = 0;

	    if (!strcmp (cmd, "add"))
	    {
		struct Node * n;
		n = newnode(args);
		AddTail(&project->extramakefiles, n);
	    }
	    else if (!strcmp (cmd, "ignoredir"))
	    {
		struct Node * n;
		n = newnode(args);
		AddTail(&project->ignoredirs, n);
	    }
	    else if (!strcmp (cmd, "defaultmakefilename"))
	    {
		SETSTR(project->defaultmakefilename,args);
	    }
	    else if (!strcmp (cmd, "top"))
	    {
		SETSTR(project->srctop,args);
	    }
	    else if (!strcmp (cmd, "defaulttarget"))
	    {
		SETSTR(project->defaulttarget,args);
	    }
	    else if (!strcmp (cmd, "genmakefilescript"))
	    {
		SETSTR(project->genmakefilescript,args);
	    }
	    else if (!strcmp (cmd, "genmakefiledeps"))
	    {
		struct Node * dep;
		int depc, t;
		char ** deps = getargs (args, &depc, NULL);

                debug(printf("MMAKE/project.c: genmakefiledeps depc=%d\n", depc));

		for (t=0; t<depc; t++)
		{
		    dep = addnodeonce (&project->genmakefiledeps, deps[t]);
		}
	    }
	    else if (!strcmp (cmd, "globalvarfile"))
	    {
	    	struct Node *n = newnode(args);
	    	
	    	if (n)
	    		AddTail(&project->globalvarfiles, n);
	    }
	    else if (!strcmp (cmd, "genglobalvarfile"))
	    {
		SETSTR(project->genglobalvarfile,args);
	    }
	    else if (!strcmp (cmd, "maketool"))
	    {
		SETSTR(project->maketool,args);
	    }
	    else
	    {
		setvar(&project->vars, cmd, args);
	    }
	}
    }

    fclose (optfh);

    /* Clean up memory from getargs */
    getargs (NULL, NULL, NULL);

    if (debug)
    {
	printf ("known projects: ");
	printlist (&projects);
    }
}

void
expungeprojects (void)
{
    struct Project *prj, *next;

    ForeachNodeSafe (&projects, prj, next)
    {
	Remove (prj);
	freeproject (prj);
    }
}

struct Project *
findproject (const char * pname)
{
    return FindNode (&projects, pname);
}

struct Project *
getfirstproject (void)
{
    struct Project * prj = GetHead (&projects);

    if (prj && prj == defaultprj)
	prj = GetNext (prj);

    return prj;
}

int
execute (struct Project * prj, const char * cmd, const char * in,
	const char * out, const char * args)
{
    char buffer[4096];
    char * cmdstr;
    int rc;

    debug(printf("MMAKE:project.c->execute(cmd '%s')\n", cmd));

    strcpy (buffer, cmd);
    strcat (buffer, " ");

    if (strcmp (in, "-"))
    {
	strcat (buffer, "<");
	strcat (buffer, in);
	strcat (buffer, " ");
    }

    if (strcmp (out, "-"))
    {
	strcat (buffer, ">");
	strcat (buffer, out);
	strcat (buffer, " ");
    }

    strcat (buffer, args);

    cmdstr = substvars (&prj->vars, buffer);

    debug(printf("MMAKE:project.c->execute: parsed cmd '%s'\n", buffer));

    if (verbose)
	printf ("Executing %s...\n", cmdstr);

    rc = system (cmdstr);

    if (rc)
    {
	printf ("%s failed: %d\n", cmdstr, rc);
    }

    return !rc;
}

void
maketarget (struct Project * prj, char * tname)
{
    struct Target * target, * subtarget;
    struct Node * node;
    struct MakefileRef * mfref;
    struct MakefileTarget * mftarget;
    struct List deps;

    if (!quiet)
	printf ("Building %s.%s\n", prj->node.name, tname);

    NewList (&deps);

    chdir (prj->srctop);

    readvars (prj);

    if (!prj->cache)
	prj->cache = activatecache (prj);

    if (!*tname)
	tname = prj->defaulttarget;

    target = FindNode (&prj->cache->targets, tname);

    if (!target)
    {
	if (!quiet)
	    printf ("Nothing known about target %s in project %s\n", tname, prj->node.name);
	return;
    }

    target->updated = 1;

    ForeachNode (&target->makefiles, mfref)
    {
	mftarget = FindNode (&mfref->makefile->targets, tname);

	ForeachNode (&mftarget->deps, node)
	    addnodeonce (&deps, node->name);
    }

    ForeachNode (&deps, node)
    {
	subtarget = FindNode (&prj->cache->targets, node->name);

	if (!subtarget)
	{
	    if (!quiet)
		printf ("Nothing known about target %s in project %s\n", node->name, prj->node.name);
	}
	else if (!subtarget->updated)
	{
	    maketarget (prj, node->name);
	}
    }

    freelist (&deps);

    ForeachNode (&target->makefiles, mfref)
    {
	if (!mfref->virtualtarget)
	{
	    callmake (prj, tname, mfref->makefile);
	}
    }

    freelist (&deps);
}
