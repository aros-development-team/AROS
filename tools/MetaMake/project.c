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

List projects;
static Project * defaultprj = NULL;

static void
readvars (Project * prj)
{
    List deps;
    Node * node, * next;
    Dep * dep;

    if (!prj->readvars)
	return;

    prj->readvars = 0;

    printf ("Read vars...\n");

    setvar (&prj->vars, "TOP", prj->top);
    setvar (&prj->vars, "CURDIR", "");

    if (prj->globalvarfile)
    {
	char * fn;
	FILE * fh;
	char line[256];
	char * name, * value, * ptr;

	fn = xstrdup (substvars (&prj->vars, prj->globalvarfile));
	fh = fopen (fn, "r");

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

#if 0
    printf ("%s=%s\n", name, substvars (&prj->vars, value));
#endif

	    setvar (&prj->vars, name, substvars (&prj->vars, value));
	}

	fclose (fh);
    }

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

static Project *
initproject (char * name)
{
    Project * prj = new (Project);

    memset (prj, 0, sizeof(Project));

    if (!defaultprj)
    {
	prj->maketool = xstrdup ("make \"TOP=$(TOP)\" \"CURDIR=$(CURDIR)\"");
	prj->defaultmakefilename = xstrdup ("Makefile");
	prj->top = getcwd (NULL, 1024);
	prj->defaulttarget = xstrdup ("all");
	prj->genmakefilescript = NULL;
	prj->globalvarfile = NULL;
	prj->genglobalvarfile = NULL;
    }
    else
    {
	prj->maketool = xstrdup (defaultprj->maketool);
	prj->defaultmakefilename = xstrdup (defaultprj->defaultmakefilename);
	prj->top = xstrdup (defaultprj->top);
	prj->defaulttarget = xstrdup (defaultprj->defaulttarget);
	SETSTR (prj->genmakefilescript, defaultprj->genmakefilescript);
	SETSTR (prj->globalvarfile, defaultprj->globalvarfile);
	SETSTR (prj->genglobalvarfile, defaultprj->genglobalvarfile);
    }

    prj->node.name = xstrdup (name);

    prj->readvars = 1;

    NewList(&prj->genmakefiledeps);
    NewList(&prj->ignoredirs);
    NewList(&prj->vars);
    NewList(&prj->extramakefiles);

    return prj;
}

static void
freeproject (Project * prj)
{
    assert (prj);

    cfree (prj->node.name);
    cfree (prj->maketool);
    cfree (prj->defaultmakefilename);
    cfree (prj->top);
    cfree (prj->defaulttarget);
    cfree (prj->genmakefilescript);
    cfree (prj->globalvarfile);
    cfree (prj->genglobalvarfile);

    if (prj->cache)
	closecache (prj->cache);
    
    freelist (&prj->genmakefiledeps);
    freelist (&prj->ignoredirs);
    freevarlist (&prj->vars);
    freelist (&prj->extramakefiles);

    xfree (prj);
}

static void
callmake (Project * prj, const char * tname, Makefile * makefile)
{
    static char buffer[4096];
    const char * path = buildpath (makefile->dir);
    int t;
    
    chdir (prj->top);
    chdir (path);

    setvar (&prj->vars, "CURDIR", path);
    setvar (&prj->vars, "TARGET", tname);

    strcpy (buffer, "");

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
    Project * project;
    
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

	if (*line == '[')
	{
	    char * name, * ptr;

	    name = ptr = line+1;
	    while (*ptr && *ptr != ']')
		ptr ++;

	    *ptr = 0;

#if 0
printf ("name=%s\n", name);
#endif

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
		Node * n;
		n = newnode(args);
		AddTail(&project->extramakefiles, n);
	    }
	    else if (!strcmp (cmd, "ignoredir"))
	    {
		Node * n;
		n = newnode(args);
		AddTail(&project->ignoredirs, n);
	    }
	    else if (!strcmp (cmd, "defaultmakefilename"))
	    {
		SETSTR(project->defaultmakefilename,args);
	    }
	    else if (!strcmp (cmd, "top"))
	    {
		SETSTR(project->top,args);
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
		Node * dep;
		int depc, t;
		char ** deps = getargs (args, &depc, NULL);

		for (t=0; t<depc; t++)
		{
		    dep = addnodeonce (&project->genmakefiledeps, deps[t]);
		}
	    }
	    else if (!strcmp (cmd, "globalvarfile"))
	    {
		SETSTR(project->globalvarfile,args);
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
    Project *prj, *next;
    
    ForeachNodeSafe (&projects, prj, next)
    {
	Remove (prj);
	freeproject (prj);
    }
}

Project *
findproject (const char * pname)
{
    return FindNode (&projects, pname);
}
		     
Project *
getfirstproject (void)
{
    Project * prj = GetHead (&projects);
    
    if (prj && prj == defaultprj)
	prj = GetNext (prj);
    
    return prj;
}

int
execute (Project * prj, const char * cmd, const char * in,
	 const char * out, const char * args)
{
    char buffer[4096];
    char * cmdstr;
    int rc;

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
maketarget (Project * prj, char * tname)
{
    Target * target, * subtarget;
    Node * node;
    MakefileRef * mfref;
    MakefileTarget * mftarget;
    List deps;
    
    printf ("Building %s.%s\n", prj->node.name, tname);

    NewList (&deps);
    
    chdir (prj->top);

    readvars (prj);

    if (!prj->cache)
	prj->cache = activatecache (prj);
    
    if (!*tname)
	tname = prj->defaulttarget;

    target = FindNode (&prj->cache->targets, tname);

    if (!target)
    {
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
